#include "synthesis.h"
#include "wavetables_integrated.h"
#include <math.h>

#define SAMPLE_RATE  44100.0f
#define OUTPUT_GAIN  24000.0f

/* --- Module state --- */
static iwt_differentiator_t diff;
static float phase;
static float f0;
static float scale;
static float coeff;
static const int16_t *wave;
static uint8_t muted;
static float gain;
static float decay_rate;

#define FADE_STEP  (1.0f / 256.0f)  /* ~5.8 ms fade at 44.1 kHz */

void synthesis_set_mute(uint8_t mute) { muted = mute; }
void synthesis_set_decay(float rate) { decay_rate = rate; }

void synthesis_trigger(void)
{
    diff.previous = (float)wave[0];
    diff.lp = 0.0f;
    phase = 0.0f;
    gain = 1.0f;
    muted = 0;
}

void synthesis_init(void)
{
    wave = &iwt_waves[0];
    synthesis_set_frequency(440.0f);
    diff.previous = (float)wave[0];
    diff.lp = 0.0f;
    phase = 0.0f;
    gain = 1.0f;
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
        /* Envelope: decay or sustain mode */
        if (decay_rate > 0.0f) {
            gain *= decay_rate;
            if (gain < 0.001f) gain = 0.0f;
        } else {
            float target = muted ? 0.0f : 1.0f;
            if (gain < target) {
                gain += FADE_STEP;
                if (gain > 1.0f) gain = 1.0f;
            } else if (gain > target) {
                gain -= FADE_STEP;
                if (gain < 0.0f) gain = 0.0f;
            }
        }

        /* Phase → table index + fraction */
        float p = phase * 128.0f;
        int32_t p_integral = (int32_t)p;
        float p_fractional = p - (float)p_integral;

        /* Hermite interpolation over integrated wavetable */
        float s = iwt_interpolate_hermite(wave, p_integral, p_fractional);

        /* Differentiation + one-pole LP */
        float out = iwt_diff_process(&diff, coeff, s);

        /* Envelope: smoothstep for sustain, raw gain for decay */
        float g = (decay_rate > 0.0f) ? gain
                : gain * gain * (3.0f - 2.0f * gain);

        /* Scale, apply gain envelope, convert to int16 */
        float sample_f = out * scale * OUTPUT_GAIN * g;
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
