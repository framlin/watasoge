#include "synthesis.h"
#include "audio_config.h"
#include "wavetables_integrated.h"
#include <math.h>

/* --- ADSR envelope --- */
typedef enum {
    ADSR_IDLE,
    ADSR_ATTACK,
    ADSR_DECAY,
    ADSR_SUSTAIN,
    ADSR_RELEASE
} adsr_state_t;

static adsr_state_t adsr_state;
static float attack_rate;
static float decay_factor;
static float sustain_level;
static float release_rate;

/* --- Wavetable playback state --- */
static iwt_differentiator_t diff;
static float phase;
static float f0;
static float scale;
static float coeff;
static const int16_t *wave;
static float gain;

/* --- ADSR parameter setters --- */

void synthesis_set_attack(float rate)
{
    if (rate < 0.0f) rate = 0.0f;
    if (rate > 1.0f) rate = 1.0f;
    attack_rate = rate;
}

void synthesis_set_decay(float factor)
{
    if (factor < 0.0f) factor = 0.0f;
    if (factor > 1.0f) factor = 1.0f;
    decay_factor = factor;
}

void synthesis_set_sustain(float level)
{
    if (level < 0.0f) level = 0.0f;
    if (level > 1.0f) level = 1.0f;
    sustain_level = level;
}

void synthesis_set_release(float rate)
{
    if (rate < 0.0f) rate = 0.0f;
    if (rate > 1.0f) rate = 1.0f;
    release_rate = rate;
}

/* --- Gate / Trigger --- */

void synthesis_gate_on(void)
{
    adsr_state = ADSR_ATTACK;
}

void synthesis_gate_off(void)
{
    adsr_state = ADSR_RELEASE;
}

void synthesis_trigger(void)
{
    diff.previous = (float)wave[0];
    diff.lp = 0.0f;
    phase = 0.0f;
    adsr_state = ADSR_ATTACK;
}

/* --- Init --- */

void synthesis_init(void)
{
    wave = &iwt_waves[0];
    synthesis_set_frequency(440.0f);
    diff.previous = (float)wave[0];
    diff.lp = 0.0f;
    phase = 0.0f;
    gain = 0.0f;
    adsr_state = ADSR_IDLE;

    /* Defaults */
    attack_rate = 1.0f;
    decay_factor = 1.0f;
    sustain_level = 0.7f;
    release_rate = 1.0f;
}

void synthesis_set_frequency(float freq_hz)
{
    f0 = freq_hz / SAMPLE_RATE;
    scale = 1.0f / (f0 * 131072.0f);
    coeff = fminf(128.0f * f0, 1.0f);
}

void synthesis_set_wave(uint16_t wave_index)
{
    if (wave_index >= IWT_WAVE_COUNT)
        return;
    wave = &iwt_waves[wave_index * IWT_WAVE_STRIDE];
    diff.previous = (float)wave[0];
    diff.lp = 0.0f;
}

void synthesis_fill_buffer(int16_t *buf, uint16_t num_samples)
{
    for (uint16_t i = 0; i < num_samples; i += 2)
    {
        /* ADSR envelope update */
        switch (adsr_state) {
        case ADSR_IDLE:
            gain = 0.0f;
            break;

        case ADSR_ATTACK:
            gain += attack_rate;
            if (gain >= 1.0f) {
                gain = 1.0f;
                if (decay_factor >= 1.0f) {
                    adsr_state = ADSR_SUSTAIN;
                } else {
                    adsr_state = ADSR_DECAY;
                }
            }
            break;

        case ADSR_DECAY:
            gain *= decay_factor;
            if (gain <= sustain_level || gain < 0.001f) {
                gain = sustain_level;
                adsr_state = (sustain_level > 0.0f)
                    ? ADSR_SUSTAIN : ADSR_IDLE;
            }
            break;

        case ADSR_SUSTAIN:
            break;

        case ADSR_RELEASE:
            gain -= release_rate;
            if (gain <= 0.0f) {
                gain = 0.0f;
                adsr_state = ADSR_IDLE;
            }
            break;
        }

        /* Phase → table index + fraction */
        float p = phase * 128.0f;
        int32_t p_integral = (int32_t)p;
        float p_fractional = p - (float)p_integral;

        /* Hermite interpolation over integrated wavetable */
        float s = iwt_interpolate_hermite(wave, p_integral, p_fractional);

        /* Differentiation + one-pole LP */
        float out = iwt_diff_process(&diff, coeff, s);

        /* Scale, apply gain envelope, convert to int16 */
        float sample_f = out * scale * OUTPUT_GAIN * gain;
        if (sample_f > 32767.0f) sample_f = 32767.0f;
        if (sample_f < -32768.0f) sample_f = -32768.0f;
        int16_t sample = (int16_t)sample_f;

        buf[i]     = sample;  /* L */
        buf[i + 1] = sample;  /* R */

        /* Advance phase */
        phase += f0;
        if (phase >= 1.0f)
            phase -= 1.0f;
    }
}
