#ifndef KARPLUS_H
#define KARPLUS_H

#include <stdint.h>

void karplus_init(void);
void karplus_fill_buffer(int16_t *buf, uint16_t num_samples);
void karplus_set_frequency(float freq_hz);
void karplus_set_damping(float damping);        /* [0..1], 0=short decay, 1=infinite */
void karplus_set_brightness(float brightness);  /* [0..1], 0=dark, 1=bright */
void karplus_set_dispersion(float dispersion);  /* [-1..+1], <0=curved bridge, >0=allpass */
void karplus_trigger(void);

#endif /* KARPLUS_H */
