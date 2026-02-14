#ifndef PLAYER_CONFIG_H
#define PLAYER_CONFIG_H

#include "player.h"

/* --- Data types --- */

typedef struct {
    uint16_t start;
    uint16_t count;
    float    freq;      /* 0 = melodic (scale), >0 = fixed pitch (percussive) */
    float    decay;     /* 0 = sustain, >0 = per-sample decay rate */
} group_def_t;

typedef struct {
    float damping;
    float brightness;
    float dispersion;
} ks_params_t;

/* --- Timing constants --- */

#define MELODY_NOTES     8
#define MELODY_ON_MS     1000
#define MELODY_OFF_MS    250

#define PERC_HITS        16
#define PERC_BEAT_MS     500

#define KS_NOTE_MS       1500
#define KS_PAUSE_MS      300
#define KS_PERC_BEAT_MS  500
#define KS_PERC_HITS     16

/* --- Data (defined in player_config.c) --- */

extern const group_def_t groups[GROUP_COUNT];
extern const ks_params_t ks_presets[];
extern const float c_major_freqs[MELODY_NOTES];
extern const float ks_scale_freqs[MELODY_NOTES];

#endif /* PLAYER_CONFIG_H */
