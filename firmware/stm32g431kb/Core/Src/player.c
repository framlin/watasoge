#include "player.h"
#include "player_config.h"
#include "synthesis.h"
#include "karplus.h"
#include "output.h"

/* --- Module state --- */

static uint8_t  current_group;
static const group_def_t *grp;
static uint16_t current_wave;
static uint8_t  current_note;
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
        }
        /* Melodische KS: Frequenz wird von player_set_pitch() gesetzt */
        karplus_trigger();
    } else if (is_percussive()) {
        synthesis_set_frequency(grp->freq);
        synthesis_trigger();
    } else {
        /* Melodische Wavetable: Frequenz wird von player_set_pitch() gesetzt */
        synthesis_set_mute(0);
    }
    beat_flag = 1;
}

static void advance_wave(void)
{
    current_wave++;
    if (current_wave >= grp->start + grp->count) {
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
    }
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
}

void player_set_pitch(float freq_hz)
{
    if (is_ks_group()) {
        if (!is_percussive()) {
            karplus_set_frequency(freq_hz);
        }
    } else {
        if (!is_percussive()) {
            synthesis_set_frequency(freq_hz);
        }
    }
}

void player_note_on(void)
{
    start_note();
}

void player_note_off(void)
{
    /* Melodic wavetable: mute (fade-out) */
    if (!is_ks_group() && !is_percussive()) {
        synthesis_set_mute(1);
    }
    /* Percussive and KS: natural decay, no stop */

    /* Advance to next note */
    current_note++;
    if (is_ks_group()) {
        uint8_t max_notes = is_percussive() ? KS_PERC_HITS : MELODY_NOTES;
        if (current_note >= max_notes) {
            advance_wave();
        }
    } else {
        uint8_t max_notes = is_percussive() ? PERC_HITS : MELODY_NOTES;
        if (current_note >= max_notes) {
            advance_wave();
        }
    }
}
