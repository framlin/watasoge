#include "synthesis.h"
#include "../../../../data/wavetables_integrated.h"
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

void synthesis_init(void)
{
    wave = &iwt_waves[0];
    synthesis_set_frequency(440.0f);
    diff.previous = (float)wave[0];
    diff.lp = 0.0f;
    phase = 0.0f;
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
        /* Phase → table index + fraction */
        float p = phase * 128.0f;
        int32_t p_integral = (int32_t)p;
        float p_fractional = p - (float)p_integral;

        /* Hermite interpolation over integrated wavetable */
        float s = iwt_interpolate_hermite(wave, p_integral, p_fractional);

        /* Differentiation + one-pole LP */
        float out = iwt_diff_process(&diff, coeff, s);

        /* Scale and convert to int16 */
        float sample_f = out * scale * OUTPUT_GAIN;
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
