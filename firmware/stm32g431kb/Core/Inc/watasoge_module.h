#ifndef WATASOGE_MODULE_H
#define WATASOGE_MODULE_H

#include <stdint.h>
#include <stdbool.h>
#include <fracanto/audio_pipeline.h>
#include <fracanto/module.h>

typedef struct {
    float    frequency;
    uint16_t wave_index;
    bool     gate_on;
    float    volume;
    uint32_t last_gate_on_ms;
} watasoge_ctx_t;

extern watasoge_ctx_t watasoge_ctx;
extern const fracanto_module_ops_t watasoge_ops;

/* Platform hooks (implemented in main.c for firmware, stubbed in tests) */
uint32_t watasoge_get_tick_ms(void);
void     watasoge_toggle_led(void);

void watasoge_init(fracanto_module_t *mod);
void watasoge_on_cv(fracanto_module_t *mod, uint8_t port, int16_t value);
void watasoge_on_cc(fracanto_module_t *mod, uint8_t port,
                    uint8_t param, int16_t value);
void watasoge_on_gate(fracanto_module_t *mod, uint8_t port, bool on);
void watasoge_on_trigger(fracanto_module_t *mod, uint8_t port);
void watasoge_process_audio(fracanto_module_t *mod,
                            const fracanto_audio_buffer_t *input,
                            fracanto_audio_buffer_t *output);
void watasoge_tick(fracanto_module_t *mod);

#endif /* WATASOGE_MODULE_H */
