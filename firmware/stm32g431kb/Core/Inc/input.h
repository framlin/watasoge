#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

void input_init(void);
uint8_t input_gate_on_pending(void);
uint8_t input_gate_off_pending(void);

#endif /* INPUT_H */
