#include "player.h"
#include "synthesis.h"
#include "stm32g4xx_hal.h"

/* --- Constants --- */

#define TOTAL_WAVES      220
#define PERC_START       116

/* Melodic: C major scale C1–C2 */
static const float c_major_freqs[8] = {
    32.703f, 36.708f, 41.203f, 43.654f,
    48.999f, 55.000f, 61.735f, 65.406f
};
#define MELODY_NOTES     8
#define MELODY_ON_MS     1000
#define MELODY_OFF_MS    250

/* Percussive: fixed pitch, 120 BPM quarter pattern */
#define PERC_FREQ        65.406f
#define PERC_HITS        16
#define PERC_ON_MS       375
#define PERC_OFF_MS      125

/* --- Module state --- */

static uint16_t current_wave;
static uint8_t  current_note;
static uint32_t event_tick;
static uint8_t  note_on;

/* --- Helpers --- */

static uint8_t is_percussive(uint16_t wave)
{
    return wave >= PERC_START;
}

static void start_note(void)
{
    if (is_percussive(current_wave)) {
        synthesis_set_wave(current_wave);
        synthesis_set_frequency(PERC_FREQ);
    } else {
        synthesis_set_frequency(c_major_freqs[current_note]);
    }
    synthesis_set_mute(0);
    note_on = 1;
    event_tick = HAL_GetTick();
}

static void advance_wave(void)
{
    current_wave++;
    if (current_wave >= TOTAL_WAVES)
        current_wave = 0;
    current_note = 0;

    synthesis_set_wave(current_wave);
    if (!is_percussive(current_wave))
        synthesis_set_frequency(c_major_freqs[0]);

    start_note();
}

/* --- Public API --- */

void player_init(void)
{
    current_wave = 0;
    current_note = 0;

    synthesis_set_wave(0);
    synthesis_set_frequency(c_major_freqs[0]);
    synthesis_set_mute(0);

    note_on = 1;
    event_tick = HAL_GetTick();
}

void player_update(void)
{
    uint32_t now = HAL_GetTick();
    uint32_t elapsed = now - event_tick;

    if (is_percussive(current_wave)) {
        if (note_on) {
            if (elapsed >= PERC_ON_MS) {
                synthesis_set_mute(1);
                note_on = 0;
                event_tick = now;
            }
        } else {
            if (elapsed >= PERC_OFF_MS) {
                current_note++;
                if (current_note >= PERC_HITS) {
                    advance_wave();
                } else {
                    start_note();
                }
            }
        }
    } else {
        if (note_on) {
            if (elapsed >= MELODY_ON_MS) {
                synthesis_set_mute(1);
                note_on = 0;
                event_tick = now;
            }
        } else {
            if (elapsed >= MELODY_OFF_MS) {
                current_note++;
                if (current_note >= MELODY_NOTES) {
                    advance_wave();
                } else {
                    start_note();
                }
            }
        }
    }
}
