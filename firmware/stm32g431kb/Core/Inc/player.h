#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>

typedef enum {
    GROUP_ADDITIVE,
    GROUP_SUB_BASS,
    GROUP_BASS_ONESHOTS,
    GROUP_BASS_STABS,
    GROUP_SYNTH_PADS,
    GROUP_GUITAR_CHORDS,
    GROUP_KICKS,
    GROUP_LOFI_KICKS,
    GROUP_CLAPS,
    GROUP_LOFI_SNARES,
    GROUP_LOFI_HIHATS,
    GROUP_COUNT
} player_group_t;

void player_init(player_group_t group);
void player_update(void);
uint8_t player_beat_pending(void);

#endif /* PLAYER_H */
