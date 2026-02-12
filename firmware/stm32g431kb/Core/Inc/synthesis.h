#ifndef SYNTHESIS_H
#define SYNTHESIS_H

#include <stdint.h>

void synthesis_init(void);
void synthesis_fill_buffer(int16_t *buf, uint16_t num_samples);
void synthesis_set_frequency(float freq_hz);
void synthesis_set_wave(uint16_t wave_index);

#endif /* SYNTHESIS_H */
