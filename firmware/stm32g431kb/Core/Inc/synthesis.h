#ifndef SYNTHESIS_H
#define SYNTHESIS_H

#include <stdint.h>

void synthesis_init(void);
void synthesis_fill_buffer(int16_t *buf, uint16_t num_samples);
void synthesis_set_frequency(float freq_hz);
void synthesis_set_wave(uint16_t wave_index);

/* ADSR envelope parameters:
 *   attack:  gain increment per sample, 0..1 (1.0 = instant)
 *   decay:   per-sample multiplicative factor, 0..1 (1.0 = no decay)
 *   sustain: target level after decay, 0..1
 *   release: gain decrement per sample, 0..1 (1.0 = instant)
 */
void synthesis_set_attack(float rate);
void synthesis_set_decay(float factor);
void synthesis_set_sustain(float level);
void synthesis_set_release(float rate);

/* Gate / Trigger */
void synthesis_gate_on(void);
void synthesis_gate_off(void);
void synthesis_trigger(void);

#endif /* SYNTHESIS_H */
