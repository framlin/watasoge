#ifndef SYNTHESIS_H
#define SYNTHESIS_H

#include <stdint.h>

void synthesis_init(void);
void synthesis_fill_buffer(int16_t *buf, uint16_t num_samples);

#endif /* SYNTHESIS_H */
