#include "watasoge_module.h"
#include "audio_config.h"
#include "synthesis.h"
#include <math.h>

watasoge_ctx_t watasoge_ctx;

/* --- ops.init --- */
void watasoge_init(fracanto_module_t *mod)
{
    (void)mod;
    synthesis_init();
    synthesis_set_frequency(440.0f);
    synthesis_set_wave(0);
    synthesis_set_attack(1.0f);
    synthesis_set_decay(1.0f);
    synthesis_set_sustain(0.7f);
    synthesis_set_release(1.0f);

    watasoge_ctx.frequency = 440.0f;
    watasoge_ctx.wave_index = 0;
    watasoge_ctx.gate_on = false;
    watasoge_ctx.volume = 1.0f;
    watasoge_ctx.last_gate_on_ms = 0;
}

/* --- ops.on_cv --- port=0: Pitch (1V/Oct, C4=261.63 Hz at 0V) */
void watasoge_on_cv(fracanto_module_t *mod, uint8_t port, int16_t value)
{
    (void)mod;
    if (port != 0)
        return;

    float volts = value * 5.0f / 32767.0f;
    float freq = 261.63f * exp2f(volts);

    if (freq < 8.0f) freq = 8.0f;
    if (freq > 12000.0f) freq = 12000.0f;

    watasoge_ctx.frequency = freq;
    synthesis_set_frequency(freq);
}

/* --- ops.on_cc --- port=0, param 0-5 */
void watasoge_on_cc(fracanto_module_t *mod, uint8_t port,
                    uint8_t param, int16_t value)
{
    (void)mod;
    if (port != 0)
        return;

    /* Normalize int16 to unsigned range [0..65534] */
    int32_t raw = (int32_t)value + 32767;
    if (raw < 0) raw = 0;

    switch (param) {
    case 0: {
        /* Wavetable index: 0..65534 → 0..219 */
        uint16_t index = (uint16_t)(raw * 219 / 65534);
        if (index > 219) index = 219;
        watasoge_ctx.wave_index = index;
        synthesis_set_wave(index);
        break;
    }
    case 1: {
        /* Attack: 0..65534 → 0..1000 ms → rate per sample */
        float ms = raw * 1000.0f / 65534.0f;
        float rate = (ms < 0.001f) ? 1.0f
                   : 1.0f / (ms * 0.001f * SAMPLE_RATE);
        synthesis_set_attack(rate);
        break;
    }
    case 2: {
        /* Decay: 0..65534 → 0..2000 ms → RT60-based factor */
        float ms = raw * 2000.0f / 65534.0f;
        float factor = (ms < 0.001f) ? 1.0f
                     : expf(-6.9f / (ms * 0.001f * SAMPLE_RATE));
        synthesis_set_decay(factor);
        break;
    }
    case 3: {
        /* Release: 0..65534 → 0..1000 ms → rate per sample */
        float ms = raw * 1000.0f / 65534.0f;
        float rate = (ms < 0.001f) ? 1.0f
                   : 1.0f / (ms * 0.001f * SAMPLE_RATE);
        synthesis_set_release(rate);
        break;
    }
    case 4: {
        /* Sustain level: 0..65534 → 0.0..1.0 */
        float level = raw / 65534.0f;
        synthesis_set_sustain(level);
        break;
    }
    case 5: {
        /* Volume: 0..65534 → 0.0..1.0 */
        float vol = raw / 65534.0f;
        watasoge_ctx.volume = vol;
        break;
    }
    default:
        break;
    }
}

/* --- ops.on_gate --- port=0: Note ON/OFF */
void watasoge_on_gate(fracanto_module_t *mod, uint8_t port, bool on)
{
    (void)mod;
    if (port != 0)
        return;

    watasoge_ctx.gate_on = on;
    if (on) {
        synthesis_gate_on();
        watasoge_ctx.last_gate_on_ms = watasoge_get_tick_ms();
    } else {
        synthesis_gate_off();
    }
}

/* --- ops.on_trigger --- port=0: Re-trigger */
void watasoge_on_trigger(fracanto_module_t *mod, uint8_t port)
{
    (void)mod;
    if (port != 0)
        return;
    synthesis_trigger();
}

/* --- ops.process_audio --- */
void watasoge_process_audio(fracanto_module_t *mod,
                            const fracanto_audio_buffer_t *input,
                            fracanto_audio_buffer_t *output)
{
    (void)mod;
    (void)input;

    uint16_t num_samples = output->block_size * output->num_channels;
    synthesis_fill_buffer(output->data, num_samples);

    if (watasoge_ctx.volume < 1.0f) {
        for (uint16_t i = 0; i < num_samples; i++)
            output->data[i] = (int16_t)(output->data[i] * watasoge_ctx.volume);
    }
}

/* --- ops.tick --- LED heartbeat + CAN timeout safety mute */
void watasoge_tick(fracanto_module_t *mod)
{
    uint32_t now = watasoge_get_tick_ms();

    /* LED heartbeat: toggle every 500 ms */
    static uint32_t last_toggle_ms;
    if (now - last_toggle_ms >= 500) {
        watasoge_toggle_led();
        last_toggle_ms = now;
    }

    /* CAN timeout safety mute: if node is in ERROR state and gate
       has been on for more than 5 seconds, force gate off to prevent
       endlessly sounding tones when CAN connectivity is lost. */
    if (mod && mod->node.state == FRACANTO_NODE_STATE_ERROR &&
        watasoge_ctx.gate_on &&
        (now - watasoge_ctx.last_gate_on_ms) >= 5000) {
        synthesis_gate_off();
        watasoge_ctx.gate_on = false;
    }
}

/* --- Module ops vtable --- */
const fracanto_module_ops_t watasoge_ops = {
    .init          = watasoge_init,
    .on_cv         = watasoge_on_cv,
    .on_cc         = watasoge_on_cc,
    .on_gate       = watasoge_on_gate,
    .on_trigger    = watasoge_on_trigger,
    .process_audio = watasoge_process_audio,
    .tick          = watasoge_tick,
};
