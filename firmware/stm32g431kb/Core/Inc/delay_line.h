#ifndef DELAY_LINE_H
#define DELAY_LINE_H

#include <stdint.h>

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

#endif /* DELAY_LINE_H */
