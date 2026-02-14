#include "player.h"
#include "player_config.h"
#include "synthesis.h"
#include "karplus.h"
#include "output.h"
#include "stm32g4xx_hal.h"

/* --- Module state --- */

static uint8_t  current_group;
static const group_def_t *grp;
static uint16_t current_wave;
static uint8_t  current_note;
static uint32_t event_tick;
static uint8_t  note_on;
static uint8_t  beat_flag;

/* --- Helpers --- */

static uint8_t is_ks_group(void)
{
    return current_group >= GROUP_KS_STRING && current_group <= GROUP_KS_CLAVE;
}

static uint8_t is_percussive(void)
{
    return grp->freq > 0.0f;
}

static void enter_group(void)
{
    if (is_ks_group()) {
        uint8_t ks_idx = current_group - GROUP_KS_STRING;
        karplus_set_damping(ks_presets[ks_idx].damping);
        karplus_set_brightness(ks_presets[ks_idx].brightness);
        karplus_set_dispersion(ks_presets[ks_idx].dispersion);
        output_set_source(karplus_fill_buffer);
    } else {
        output_set_source(synthesis_fill_buffer);
        synthesis_set_decay(grp->decay);
    }
}

static void start_note(void)
{
    if (is_ks_group()) {
        if (is_percussive()) {
            karplus_set_frequency(grp->freq);
        } else {
            karplus_set_frequency(ks_scale_freqs[current_note]);
        }
        karplus_trigger();
    } else if (is_percussive()) {
        synthesis_set_frequency(grp->freq);
        synthesis_trigger();
    } else {
        synthesis_set_frequency(c_major_freqs[current_note]);
        synthesis_set_mute(0);
    }
    note_on = 1;
    beat_flag = 1;
    event_tick = HAL_GetTick();
}

static void advance_wave(void)
{
    current_wave++;
    if (current_wave >= grp->start + grp->count) {
        /* Advance to next group, wrap at end */
        current_group++;
        if (current_group >= GROUP_COUNT)
            current_group = 0;
        grp = &groups[current_group];
        current_wave = grp->start;
        enter_group();
    }
    current_note = 0;

    if (!is_ks_group()) {
        synthesis_set_wave(current_wave);
        if (!is_percussive())
            synthesis_set_frequency(c_major_freqs[0]);
    }

    start_note();
}

/* --- Public API --- */

uint8_t player_beat_pending(void)
{
    if (beat_flag) {
        beat_flag = 0;
        return 1;
    }
    return 0;
}

void player_init(player_group_t group)
{
    current_group = (uint8_t)group;
    grp = &groups[group];
    current_wave = grp->start;
    current_note = 0;

    enter_group();

    if (!is_ks_group()) {
        synthesis_set_wave(grp->start);
    }

    start_note();
}

void player_update(void)
{
    uint32_t now = HAL_GetTick();
    uint32_t elapsed = now - event_tick;

    if (is_ks_group()) {
        /* Karplus-Strong groups */
        if (is_percussive()) {
            /* KS percussion: trigger hits at tempo */
            if (elapsed >= KS_PERC_BEAT_MS) {
                current_note++;
                if (current_note >= KS_PERC_HITS) {
                    advance_wave();
                } else {
                    karplus_trigger();
                    beat_flag = 1;
                    event_tick = now;
                }
            }
        } else {
            /* KS melodic: trigger + wait for decay, then next note */
            if (note_on) {
                if (elapsed >= KS_NOTE_MS) {
                    note_on = 0;
                    event_tick = now;
                }
            } else {
                if (elapsed >= KS_PAUSE_MS) {
                    current_note++;
                    if (current_note >= MELODY_NOTES) {
                        advance_wave();
                    } else {
                        start_note();
                    }
                }
            }
        }
    } else if (is_percussive()) {
        if (elapsed >= PERC_BEAT_MS) {
            current_note++;
            if (current_note >= PERC_HITS) {
                advance_wave();
            } else {
                synthesis_trigger();
                beat_flag = 1;
                event_tick = now;
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
