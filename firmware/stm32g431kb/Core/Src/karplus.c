#include "karplus.h"
#include <math.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Constants                                                         */
/* ------------------------------------------------------------------ */

#define SAMPLE_RATE     44100.0f
#define KS_DELAY_SIZE   1024
#define KS_AP_SIZE      256
#define OUTPUT_GAIN     24000.0f
#define PI_F            3.14159265f

/* ------------------------------------------------------------------ */
/*  XorShift32 PRNG                                                   */
/* ------------------------------------------------------------------ */

static uint32_t prng_state = 0x12345678u;

static inline float prng_float(void)
{
    prng_state ^= prng_state << 13;
    prng_state ^= prng_state >> 17;
    prng_state ^= prng_state << 5;
    /* Map to [-1, +1] */
    return (float)(int32_t)prng_state * (1.0f / 2147483648.0f);
}

/* ------------------------------------------------------------------ */
/*  Semitones-to-ratio (powf(2, semi/12) approximation via LUT)       */
/* ------------------------------------------------------------------ */

/* 257-entry LUT: index i -> 2^(i/256), covers 0..1 octave */
#define SEMI_LUT_SIZE 257
static float semi_lut[SEMI_LUT_SIZE];

static void semi_lut_init(void)
{
    for (int i = 0; i < SEMI_LUT_SIZE; i++)
        semi_lut[i] = powf(2.0f, (float)i / 256.0f);
}

static inline float semitones_to_ratio(float semi)
{
    /* 2^(semi/12) = 2^(semi * 256 / (12*256))
       Let x = semi / 12.0. Then 2^x.
       We split x into integer octave + fractional for LUT lookup. */
    float octaves = semi * (1.0f / 12.0f);
    int oct_int = (int)octaves;
    float oct_frac = octaves - (float)oct_int;
    if (oct_frac < 0.0f) { oct_frac += 1.0f; oct_int--; }

    /* LUT lookup with linear interpolation */
    float idx_f = oct_frac * 256.0f;
    int idx = (int)idx_f;
    float frac = idx_f - (float)idx;
    if (idx < 0) { idx = 0; frac = 0.0f; }
    if (idx >= 256) { idx = 255; frac = 1.0f; }
    float val = semi_lut[idx] + frac * (semi_lut[idx + 1] - semi_lut[idx]);

    /* Apply integer octave shift via ldexpf (exact, no rounding) */
    return ldexpf(val, oct_int);
}

/* ------------------------------------------------------------------ */
/*  SVF shift LUT: delay compensation for IIR lowpass phase delay     */
/* ------------------------------------------------------------------ */

#define SVF_SHIFT_LUT_SIZE 129
static float svf_shift_lut[SVF_SHIFT_LUT_SIZE];

static void svf_shift_lut_init(void)
{
    for (int i = 0; i < SVF_SHIFT_LUT_SIZE; i++) {
        /* i maps to 0..128 semitones above fundamental */
        float semi = (float)i;
        float ratio = semitones_to_ratio(semi);
        /* Phase delay of ZDF-SVF at this ratio: 2*atan(1/ratio) / (2*pi) */
        svf_shift_lut[i] = 2.0f * atanf(1.0f / ratio) / (2.0f * PI_F);
    }
}

static inline float svf_shift_lookup(float semitones)
{
    if (semitones < 0.0f) semitones = 0.0f;
    if (semitones > 128.0f) semitones = 128.0f;
    int idx = (int)semitones;
    float frac = semitones - (float)idx;
    if (idx >= SVF_SHIFT_LUT_SIZE - 1) return svf_shift_lut[SVF_SHIFT_LUT_SIZE - 1];
    return svf_shift_lut[idx] + frac * (svf_shift_lut[idx + 1] - svf_shift_lut[idx]);
}

/* ------------------------------------------------------------------ */
/*  Delay-Line helpers                                                */
/* ------------------------------------------------------------------ */

static inline void dl_write(float *line, uint16_t *pos, uint16_t size, float sample)
{
    line[*pos] = sample;
    if (*pos == 0)
        *pos = size - 1;
    else
        (*pos)--;
}

static inline float dl_read_hermite(const float *line, uint16_t write_pos,
                                    uint16_t size, float delay)
{
    /* Split delay into integer + fractional */
    int32_t d_int = (int32_t)delay;
    float   d_frac = delay - (float)d_int;

    int32_t base = (int32_t)write_pos + d_int;

    float xm1 = line[(base)     % size];
    float x0  = line[(base + 1) % size];
    float x1  = line[(base + 2) % size];
    float x2  = line[(base + 3) % size];

    /* Hermite 4-point interpolation */
    float c = (x1 - xm1) * 0.5f;
    float v = x0 - x1;
    float w = c + v;
    float a = w + v + (x2 - x0) * 0.5f;
    float b_neg = w + a;
    return (((a * d_frac) - b_neg) * d_frac + c) * d_frac + x0;
}

static inline float dl_allpass(float *line, uint16_t *write_pos,
                               uint16_t size, float sample,
                               uint16_t delay, float coeff)
{
    float read = line[(*write_pos + delay) % size];
    float write = sample + coeff * read;
    dl_write(line, write_pos, size, write);
    return -write * coeff + read;
}

/* ------------------------------------------------------------------ */
/*  Module state                                                      */
/* ------------------------------------------------------------------ */

static float delay_line[KS_DELAY_SIZE];
static float ap_line[KS_AP_SIZE];
static uint16_t dl_pos;
static uint16_t ap_pos;

/* Target parameters (set by API calls) */
static float target_frequency;       /* Hz */
static float target_damping;         /* [0..1] */
static float target_brightness;      /* [0..1] */
static float target_dispersion;      /* [-1..+1] */

/* Current (interpolated) parameters */
static float cur_frequency;
static float cur_damping;
static float cur_brightness;
static float cur_dispersion;

/* Derived values */
static float cur_delay;              /* Delay-line length in samples */

/* SVF state (ZDF-SVF lowpass, Q=0.5) */
static float svf_ic1eq;
static float svf_ic2eq;

/* Excitation SVF state */
static float exc_svf_ic1eq;
static float exc_svf_ic2eq;
static float exc_remaining;         /* Remaining excitation samples */

/* DC-blocker state */
static float dc_x_prev;
static float dc_y_prev;

/* Curved bridge state */
static float curved_bridge;

/* Trigger pending flag */
static volatile uint8_t trigger_pending;

/* ------------------------------------------------------------------ */
/*  SVF processing (ZDF, trapezoidal integration, lowpass output)     */
/* ------------------------------------------------------------------ */

static inline float svf_process(float *ic1eq, float *ic2eq,
                                float g, float r, float h,
                                float input)
{
    float hp = (input - r * *ic1eq - g * *ic1eq - *ic2eq) * h;
    float v1 = g * hp;
    float bp = v1 + *ic1eq;
    *ic1eq = v1 + bp;
    float v2 = g * bp;
    float lp = v2 + *ic2eq;
    *ic2eq = v2 + lp;
    return lp;
}

/* ------------------------------------------------------------------ */
/*  Compute SVF coefficients for a given normalized frequency         */
/* ------------------------------------------------------------------ */

typedef struct {
    float g, r, h;
} svf_coeff_t;

static inline svf_coeff_t svf_compute_coeff(float freq_norm)
{
    svf_coeff_t c;
    c.g = tanf(PI_F * freq_norm);
    c.r = 2.0f;              /* Q = 0.5 => r = 1/Q = 2 */
    c.h = 1.0f / (1.0f + c.r * c.g + c.g * c.g);
    return c;
}

/* ------------------------------------------------------------------ */
/*  Init                                                              */
/* ------------------------------------------------------------------ */

void karplus_init(void)
{
    semi_lut_init();
    svf_shift_lut_init();

    memset(delay_line, 0, sizeof(delay_line));
    memset(ap_line, 0, sizeof(ap_line));
    dl_pos = 0;
    ap_pos = 0;

    target_frequency = 220.0f;
    target_damping = 0.7f;
    target_brightness = 0.5f;
    target_dispersion = 0.0f;

    cur_frequency = target_frequency;
    cur_damping = target_damping;
    cur_brightness = target_brightness;
    cur_dispersion = target_dispersion;
    cur_delay = SAMPLE_RATE / cur_frequency;

    svf_ic1eq = 0.0f;
    svf_ic2eq = 0.0f;
    exc_svf_ic1eq = 0.0f;
    exc_svf_ic2eq = 0.0f;
    exc_remaining = 0.0f;

    dc_x_prev = 0.0f;
    dc_y_prev = 0.0f;
    curved_bridge = 0.0f;
    trigger_pending = 0;
}

/* ------------------------------------------------------------------ */
/*  API: parameter setters                                            */
/* ------------------------------------------------------------------ */

void karplus_set_frequency(float freq_hz)
{
    if (freq_hz < 20.0f) freq_hz = 20.0f;
    if (freq_hz > 10000.0f) freq_hz = 10000.0f;
    target_frequency = freq_hz;
}

void karplus_set_damping(float damping)
{
    if (damping < 0.0f) damping = 0.0f;
    if (damping > 1.0f) damping = 1.0f;
    target_damping = damping;
}

void karplus_set_brightness(float brightness)
{
    if (brightness < 0.0f) brightness = 0.0f;
    if (brightness > 1.0f) brightness = 1.0f;
    target_brightness = brightness;
}

void karplus_set_dispersion(float dispersion)
{
    if (dispersion < -1.0f) dispersion = -1.0f;
    if (dispersion > 1.0f) dispersion = 1.0f;
    target_dispersion = dispersion;
}

void karplus_trigger(void)
{
    trigger_pending = 1;
}

/* ------------------------------------------------------------------ */
/*  Fill buffer — main DSP routine                                    */
/* ------------------------------------------------------------------ */

void karplus_fill_buffer(int16_t *buf, uint16_t num_samples)
{
    /* ---- Handle trigger: fill delay line with noise burst -------- */
    if (trigger_pending) {
        trigger_pending = 0;

        /* Snap current parameters to target */
        cur_frequency = target_frequency;
        cur_damping = target_damping;
        cur_brightness = target_brightness;
        cur_dispersion = target_dispersion;

        cur_delay = SAMPLE_RATE / cur_frequency;
        if (cur_delay > (float)(KS_DELAY_SIZE - 4))
            cur_delay = (float)(KS_DELAY_SIZE - 4);
        if (cur_delay < 4.0f)
            cur_delay = 4.0f;

        /* Fill delay line with noise burst (length = one period) */
        uint16_t burst_len = (uint16_t)cur_delay;

        /* Excitation brightness filter: SVF lowpass on the noise */
        float exc_cutoff = cur_brightness * cur_brightness * 0.4f + 0.05f;
        svf_coeff_t ec = svf_compute_coeff(exc_cutoff);
        exc_svf_ic1eq = 0.0f;
        exc_svf_ic2eq = 0.0f;

        /* Write noise into delay line using dl_write (circular convention) */
        memset(delay_line, 0, sizeof(delay_line));
        memset(ap_line, 0, sizeof(ap_line));
        dl_pos = 0;
        ap_pos = 0;
        curved_bridge = 0.0f;
        dc_x_prev = 0.0f;
        dc_y_prev = 0.0f;
        svf_ic1eq = 0.0f;
        svf_ic2eq = 0.0f;

        for (uint16_t i = 0; i < burst_len; i++) {
            float noise = prng_float();
            float filtered = svf_process(&exc_svf_ic1eq, &exc_svf_ic2eq,
                                         ec.g, ec.r, ec.h, noise);
            dl_write(delay_line, &dl_pos, KS_DELAY_SIZE, filtered);
        }

        /* Also set up remaining excitation for input-additive mode */
        exc_remaining = 0.0f;
    }

    /* ---- Compute target derived values for this block ------------ */
    float tgt_delay = SAMPLE_RATE / target_frequency;
    if (tgt_delay > (float)(KS_DELAY_SIZE - 4))
        tgt_delay = (float)(KS_DELAY_SIZE - 4);
    if (tgt_delay < 4.0f)
        tgt_delay = 4.0f;

    float tgt_damping = target_damping;
    float tgt_brightness = target_brightness;
    float tgt_dispersion = target_dispersion;

    /* ---- RT60-based damping coefficient -------------------------- */
    float lf_damping = tgt_damping * (2.0f - tgt_damping);
    float rt60 = 0.07f * semitones_to_ratio(lf_damping * 96.0f) * SAMPLE_RATE;
    float damping_coefficient = semitones_to_ratio(
        fmaxf(-120.0f * tgt_delay / rt60, -127.0f));

    /* ---- Brightness -> SVF cutoff -------------------------------- */
    float freq_norm = target_frequency / SAMPLE_RATE;
    float damping_cutoff_semi = 24.0f
        + tgt_damping * tgt_damping * 48.0f
        + tgt_brightness * tgt_brightness * 24.0f;
    if (damping_cutoff_semi > 84.0f) damping_cutoff_semi = 84.0f;

    float damping_f = freq_norm * semitones_to_ratio(damping_cutoff_semi);
    if (damping_f > 0.499f) damping_f = 0.499f;

    /* Infinite sustain crossfade (damping >= 0.95) */
    if (tgt_damping >= 0.95f) {
        float to_inf = 20.0f * (tgt_damping - 0.95f);
        damping_coefficient += to_inf * (1.0f - damping_coefficient);
        damping_f += to_inf * (0.499f - damping_f);
        damping_cutoff_semi += to_inf * (128.0f - damping_cutoff_semi);
    }

    svf_coeff_t sc = svf_compute_coeff(damping_f);

    /* ---- SVF delay compensation ---------------------------------- */
    float damping_compensation = 1.0f - svf_shift_lookup(damping_cutoff_semi);

    /* ---- Per-sample parameter interpolation increments ----------- */
    /* num_samples = total int16_t values (stereo), frames = mono samples */
    uint16_t frames = num_samples / 2;
    float inv_n = 1.0f / (float)frames;
    float delay_inc = (tgt_delay * damping_compensation - cur_delay * damping_compensation) * inv_n;
    float delay_now = cur_delay * damping_compensation;

    float disp_inc = (tgt_dispersion - cur_dispersion) * inv_n;
    float disp_now = cur_dispersion;

    /* ---- Sample loop --------------------------------------------- */
    for (uint16_t i = 0; i < frames; i++) {
        float delay = delay_now + delay_inc * (float)i;
        float dispersion = disp_now + disp_inc * (float)i;

        /* Subtract 1 sample for filter group delay */
        float d = delay - 1.0f;
        if (d < 4.0f) d = 4.0f;

        float s;

        if (dispersion > 0.01f) {
            /* --- Allpass dispersion (inharmonic stretch) --- */
            float stretch_point = dispersion * (2.0f - dispersion) * 0.475f;
            float ap_delay_f = d * stretch_point;
            float main_delay = d - ap_delay_f;
            float ap_coeff = -0.618f * dispersion / (0.15f + fabsf(dispersion));

            uint16_t ap_d = (uint16_t)ap_delay_f;
            if (ap_d < 1) ap_d = 1;
            if (ap_d >= KS_AP_SIZE - 1) ap_d = KS_AP_SIZE - 2;

            if (main_delay >= 4.0f) {
                s = dl_read_hermite(delay_line, dl_pos, KS_DELAY_SIZE, main_delay);
                s = dl_allpass(ap_line, &ap_pos, KS_AP_SIZE, s, ap_d, ap_coeff);
            } else {
                s = dl_read_hermite(delay_line, dl_pos, KS_DELAY_SIZE, d);
            }
        } else if (dispersion < -0.01f) {
            /* --- Curved bridge (sitar/buzz nonlinearity) --- */
            float bridge_curving = -dispersion;
            bridge_curving = bridge_curving * bridge_curving * 0.01f;
            float d_mod = d * (1.0f - curved_bridge * bridge_curving);
            if (d_mod < 4.0f) d_mod = 4.0f;
            if (d_mod > (float)(KS_DELAY_SIZE - 4)) d_mod = (float)(KS_DELAY_SIZE - 4);

            s = dl_read_hermite(delay_line, dl_pos, KS_DELAY_SIZE, d_mod);

            /* Update curved bridge state */
            float value = fabsf(s) - 0.025f;
            float sign = s > 0.0f ? 1.0f : -1.5f;
            curved_bridge = (fabsf(value) + value) * sign;
        } else {
            /* --- No dispersion --- */
            s = dl_read_hermite(delay_line, dl_pos, KS_DELAY_SIZE, d);
        }

        /* IIR damping filter (ZDF-SVF lowpass) */
        s = svf_process(&svf_ic1eq, &svf_ic2eq, sc.g, sc.r, sc.h, s);

        /* Loop gain (damping coefficient) */
        s *= damping_coefficient;

        /* DC blocker: y = x - x_prev + 0.9995 * y_prev */
        float dc_out = s - dc_x_prev + 0.9995f * dc_y_prev;
        dc_x_prev = s;
        dc_y_prev = dc_out;
        s = dc_out;

        /* Hard clamp for stability */
        if (s > 20.0f) s = 20.0f;
        if (s < -20.0f) s = -20.0f;

        /* Write back to delay line */
        dl_write(delay_line, &dl_pos, KS_DELAY_SIZE, s);

        /* Scale to output */
        float out_f = s * OUTPUT_GAIN;
        int32_t out_i = (int32_t)out_f;
        if (out_i > 32767) out_i = 32767;
        if (out_i < -32768) out_i = -32768;

        /* Stereo: L = R */
        int16_t sample = (int16_t)out_i;
        buf[i * 2]     = sample;
        buf[i * 2 + 1] = sample;
    }

    /* Update current parameters for next block */
    cur_delay = tgt_delay;
    cur_frequency = target_frequency;
    cur_damping = tgt_damping;
    cur_brightness = tgt_brightness;
    cur_dispersion = tgt_dispersion;
}
