#include "player.h"
#include "synthesis.h"
#include "karplus.h"
#include "output.h"
#include "stm32g4xx_hal.h"

/* --- Group definitions --- */

typedef struct {
    uint16_t start;
    uint16_t count;
    float    freq;      /* 0 = melodic (scale), >0 = fixed pitch (percussive) */
    float    decay;     /* 0 = sustain, >0 = per-sample decay rate */
} group_def_t;

/* Karplus-Strong group parameters */
typedef struct {
    float damping;
    float brightness;
    float dispersion;
} ks_params_t;

static const group_def_t groups[GROUP_COUNT] = {
    {   0, 64, 0.0f,      0.0f    },  /* ADDITIVE */
    {  64, 13, 0.0f,      0.0f    },  /* SUB_BASS */
    {  77, 20, 0.0f,      0.0f    },  /* BASS_ONESHOTS */
    {  97,  2, 0.0f,      0.0f    },  /* BASS_STABS */
    {  99, 13, 0.0f,      0.0f    },  /* SYNTH_PADS */
    { 112,  4, 0.0f,      0.0f    },  /* GUITAR_CHORDS */
    { 116, 20, 65.406f,   0.9995f },  /* KICKS — C2 */
    { 136,  7, 65.406f,   0.9995f },  /* LOFI_KICKS — C2 */
    { 143, 22, 261.626f,  0.9993f },  /* CLAPS — C4 */
    { 165, 50, 261.626f,  0.9993f },  /* LOFI_SNARES — C4 */
    { 215,  5, 1046.502f, 0.9985f },  /* LOFI_HIHATS — C6 */
    /* KS groups: dummy wave entries (not used), count=8 for 8-note scale */
    {   0,  8, 0.0f,      0.0f    },  /* KS_STRING */
    {   0,  8, 0.0f,      0.0f    },  /* KS_BRIGHT */
    {   0,  8, 0.0f,      0.0f    },  /* KS_INHARMONIC */
    {   0,  8, 0.0f,      0.0f    },  /* KS_SITAR */
    {   0,  8, 65.406f,   0.0f    },  /* KS_PERCUSSION — fixed pitch C2 */
};

/* KS parameter presets per KS group */
static const ks_params_t ks_presets[] = {
    { 0.70f, 0.50f,  0.00f },  /* KS_STRING: warm plucked string */
    { 0.80f, 0.90f,  0.00f },  /* KS_BRIGHT: bright string */
    { 0.75f, 0.60f,  0.60f },  /* KS_INHARMONIC: bell/gamelan */
    { 0.70f, 0.50f, -0.70f },  /* KS_SITAR: curved bridge buzz */
    { 0.30f, 0.80f,  0.00f },  /* KS_PERCUSSION: short decay drum */
};

/* Melodic: C major scale C1–C2 */
static const float c_major_freqs[8] = {
    32.703f, 36.708f, 41.203f, 43.654f,
    48.999f, 55.000f, 61.735f, 65.406f
};
/* KS melodic: C major scale C3–C4 (better range for strings) */
static const float ks_scale_freqs[8] = {
    130.813f, 146.832f, 164.814f, 174.614f,
    195.998f, 220.000f, 246.942f, 261.626f
};
#define MELODY_NOTES     8
#define MELODY_ON_MS     1000
#define MELODY_OFF_MS    250

/* Percussive: 120 BPM quarter pattern */
#define PERC_HITS        16
#define PERC_BEAT_MS     500

/* KS note timing */
#define KS_NOTE_MS       1500
#define KS_PAUSE_MS      300
#define KS_PERC_BEAT_MS  500
#define KS_PERC_HITS     16

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
    return current_group >= GROUP_KS_STRING && current_group <= GROUP_KS_PERCUSSION;
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
