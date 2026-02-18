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

/* --- Note counts --- */

#define MELODY_NOTES     8
#define PERC_HITS        16
#define KS_PERC_HITS     16

/* --- Data (defined in player_config.c) --- */

extern const group_def_t groups[GROUP_COUNT];
extern const ks_params_t ks_presets[];
extern const float c_major_freqs[MELODY_NOTES];
extern const float ks_scale_freqs[MELODY_NOTES];

#endif /* PLAYER_CONFIG_H */
