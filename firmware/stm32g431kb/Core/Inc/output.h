#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdint.h>

typedef void (*fill_buffer_fn)(int16_t *buf, uint16_t num_samples);

void output_init(fill_buffer_fn default_source);
void output_set_source(fill_buffer_fn fn);

#endif /* OUTPUT_H */
