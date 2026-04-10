#!/bin/bash
# play_scale.sh — Spielt C-Dur-Tonleitern über CAN-Bus auf watasoge
#
# Nutzt fraclacan bin/can_send_semantic (SSH zu fraclacanpi).
# Voraussetzung: fraclacan gebaut, CAN-Bus aktiv, watasoge geflasht.
#
# Usage: bash tools/play_scale.sh [WAVES] [NOTE_MS]
#   WAVES:   Anzahl Wavetables (default: 5)
#   NOTE_MS: Notenlänge in ms (default: 400)

set -euo pipefail

FRACLACAN_DIR="${HOME}/tinker/fraclacan"
SEND="${FRACLACAN_DIR}/bin/can_send_semantic"
WATASOGE_NODE=2
MASTER_NODE=0

WAVES="${1:-5}"
NOTE_MS="${2:-400}"
NOTE_SEC=$(echo "scale=3; $NOTE_MS / 1000" | bc)
PAUSE_SEC=$(echo "scale=3; $NOTE_SEC * 0.3" | bc)

# --- Hilfsfunktionen ---

send_msg() {
    echo "$1" | "$SEND"
}

patch_connect() {
    send_msg "{\"priority\":\"config\",\"node\":${MASTER_NODE},\"subtype\":\"patch\",\"opcode\":\"connect\",\"srcNode\":${MASTER_NODE},\"srcPort\":0,\"dstNode\":${WATASOGE_NODE},\"dstPort\":0}"
}

cv_pitch() {
    local value=$1
    send_msg "{\"priority\":\"cv\",\"node\":${MASTER_NODE},\"subtype\":\"cv\",\"port\":0,\"value\":${value}}"
}

gate_on() {
    send_msg "{\"priority\":\"cv\",\"node\":${MASTER_NODE},\"subtype\":\"gate\",\"port\":0,\"value\":\"on\"}"
}

gate_off() {
    send_msg "{\"priority\":\"cv\",\"node\":${MASTER_NODE},\"subtype\":\"gate\",\"port\":0,\"value\":\"off\"}"
}

cc_param() {
    local param=$1
    local value=$2
    send_msg "{\"priority\":\"cv\",\"node\":${MASTER_NODE},\"subtype\":\"cc\",\"port\":0,\"param\":${param},\"value\":${value}}"
}

# CV-Wert für Halbtöne ab C4
# watasoge: freq = 261.63 * exp2(volts), volts = value * 5.0 / 32767.0
# Halbtöne → volts = semitones / 12.0
# → value = semitones / 12.0 * 32767.0 / 5.0 = semitones * 546
note_cv() {
    local semitones=$1
    echo $(( semitones * 546 ))
}

# Wavetable-Index → CC-Wert
# watasoge: index = (value + 32767) * 219 / 65534
# → value = index * 65534 / 219 - 32767
wave_cc() {
    local index=$1
    echo $(( index * 65534 / 219 - 32767 ))
}

# --- C-Dur-Tonleiter ---

NOTES=(0 2 4 5 7 9 11 12)
NAMES=("C4" "D4" "E4" "F4" "G4" "A4" "B4" "C5")

play_scale() {
    local wave_index=$1
    local wave_name=$2

    # Wavetable setzen
    cc_param 0 "$(wave_cc "$wave_index")"
    sleep 0.02

    printf "  Wave %3d (%s): " "$wave_index" "$wave_name"

    for i in "${!NOTES[@]}"; do
        printf "%s " "${NAMES[$i]}"

        # Pitch setzen
        cv_pitch "$(note_cv "${NOTES[$i]}")"
        sleep 0.005

        # Gate ON
        gate_on
        sleep "$NOTE_SEC"

        # Gate OFF
        gate_off
        sleep "$PAUSE_SEC"
    done

    echo ""
}

# --- Hauptprogramm ---

echo "=== watasoge CAN-Bus Audio-Test ==="
echo "Notenlänge: ${NOTE_MS} ms, Wavetables: ${WAVES}"
echo ""

# 1. Route etablieren
echo "Route: Master:0 → watasoge:0 (PATCH_CONNECT)"
patch_connect
sleep 0.2

# 2. ADSR konfigurieren
echo "ADSR: Attack=10ms, Sustain=1.0, Release=100ms"
cc_param 1 -31461    # Attack 10ms: 10 * 65534/1000 - 32767 ≈ -32112
cc_param 4 32767     # Sustain = 1.0 (max)
cc_param 3 -26220    # Release 100ms: 100 * 65534/1000 - 32767 ≈ -26214
cc_param 5 32767     # Volume = 1.0 (max)
sleep 0.05

# 3. Tonleitern mit verschiedenen Wavetables
WAVE_INDICES=(0 10 30 64 99 116 143 165 200 215)
WAVE_NAMES=("Sine" "Comb" "Drawbar" "SubBass" "SynthPad" "Kick" "Clap" "Snare" "Lofi" "HiHat")

echo ""
echo "Spiele ${WAVES} Tonleitern (C-Dur):"
echo ""

for (( w=0; w<WAVES && w<${#WAVE_INDICES[@]}; w++ )); do
    play_scale "${WAVE_INDICES[$w]}" "${WAVE_NAMES[$w]}"
    sleep 0.5
done

# Safety: Gate OFF
gate_off

echo ""
echo "Fertig."
