#include "player_config.h"

const group_def_t groups[GROUP_COUNT] = {
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
    /* KS groups: one "wave" per group (sound defined by KS parameters) */
    {   0,  1, 0.0f,      0.0f    },  /* KS_STRING */
    {   0,  1, 0.0f,      0.0f    },  /* KS_BRIGHT */
    {   0,  1, 0.0f,      0.0f    },  /* KS_INHARMONIC */
    {   0,  1, 0.0f,      0.0f    },  /* KS_SITAR */
    {   0,  1, 65.406f,   0.0f    },  /* KS_KICK — C2, deep thud */
    {   0,  1, 220.000f,  0.0f    },  /* KS_SNARE — A3, buzzy */
    {   0,  1, 1318.510f, 0.0f    },  /* KS_HIHAT — E6, metallic */
    {   0,  1, 97.999f,   0.0f    },  /* KS_TOM — G2, resonant */
    {   0,  1, 659.255f,  0.0f    },  /* KS_COWBELL — E5, metallic */
    {   0,  1, 880.000f,  0.0f    },  /* KS_CLAVE — A5, wooden */
};

const ks_params_t ks_presets[] = {
    { 0.70f, 0.50f,  0.00f },  /* KS_STRING: warm plucked string */
    { 0.80f, 0.90f,  0.00f },  /* KS_BRIGHT: bright string */
    { 0.75f, 0.60f,  0.60f },  /* KS_INHARMONIC: bell/gamelan */
    { 0.70f, 0.50f, -0.70f },  /* KS_SITAR: curved bridge buzz */
    { 0.50f, 0.50f,  0.00f },  /* KS_KICK: deep thud */
    { 0.40f, 0.90f, -0.50f },  /* KS_SNARE: bright, curved bridge buzz */
    { 0.30f, 1.00f,  0.80f },  /* KS_HIHAT: metallic, inharmonic */
    { 0.50f, 0.50f,  0.00f },  /* KS_TOM: warm, resonant */
    { 0.45f, 0.70f,  0.70f },  /* KS_COWBELL: metallic, inharmonic */
    { 0.35f, 0.60f,  0.30f },  /* KS_CLAVE: wooden click */
};

const float c_major_freqs[MELODY_NOTES] = {
    32.703f, 36.708f, 41.203f, 43.654f,
    48.999f, 55.000f, 61.735f, 65.406f
};

const float ks_scale_freqs[MELODY_NOTES] = {
    130.813f, 146.832f, 164.814f, 174.614f,
    195.998f, 220.000f, 246.942f, 261.626f
};
