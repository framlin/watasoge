#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "synthesis.h"

/* --- Constants --- */

#define BUF_FRAMES    64
#define BUF_SAMPLES   (BUF_FRAMES * 2)  /* stereo */
#define SMALL_SAMPLES 4

#define ATTACK_INSTANT  1.0f
#define ATTACK_SLOW     (1.0f / 256.0f)   /* ~5.8 ms at 44.1 kHz */
#define RELEASE_INSTANT 1.0f
#define RELEASE_SLOW    (1.0f / 256.0f)
#define RELEASE_VSLOW   (1.0f / 1024.0f)  /* ~23.2 ms at 44.1 kHz */
#define DECAY_NONE      1.0f
#define DECAY_GRADUAL   0.999f
#define DECAY_KICK      0.9995f
#define DECAY_FAST      0.99f

/* --- Test helpers --- */

static int16_t buf[BUF_SAMPLES];

static int is_silent(const int16_t *b, uint16_t n)
{
    for (uint16_t i = 0; i < n; i++)
        if (b[i] != 0) return 0;
    return 1;
}

static int16_t peak_amplitude(const int16_t *b, uint16_t n)
{
    int16_t peak = 0;
    for (uint16_t i = 0; i < n; i++) {
        int16_t a = b[i] < 0 ? (int16_t)(-b[i]) : b[i];
        if (a > peak) peak = a;
    }
    return peak;
}

static void fill(void)
{
    memset(buf, 0, sizeof(buf));
    synthesis_fill_buffer(buf, BUF_SAMPLES);
}

static void fill_small(int16_t *sb)
{
    memset(sb, 0, (size_t)(SMALL_SAMPLES) * sizeof(int16_t));
    synthesis_fill_buffer(sb, SMALL_SAMPLES);
}

static int passed;
static int failed;

#define TEST(name) \
    do { printf("  %-55s ", name); } while (0)

#define PASS() \
    do { printf("OK\n"); passed++; } while (0)

#define FAIL(msg) \
    do { printf("FAIL (line %d): %s\n", __LINE__, msg); failed++; } while (0)

#define CHECK(cond, msg) \
    do { if (cond) { PASS(); } else { FAIL(msg); } } while (0)

/* --- Tests: State transitions --- */

static void test_init_produces_silence(void)
{
    TEST("init → IDLE → silence");
    synthesis_init();
    fill();
    CHECK(is_silent(buf, BUF_SAMPLES), "output not silent after init");
}

static void test_gate_on_instant_attack(void)
{
    TEST("gate_on + instant attack → non-zero output");
    synthesis_init();
    synthesis_set_attack(ATTACK_INSTANT);
    synthesis_gate_on();
    fill();
    CHECK(!is_silent(buf, BUF_SAMPLES), "output silent after gate_on");
}

static void test_gate_off_instant_release(void)
{
    TEST("gate_off + instant release → silence");
    synthesis_init();
    synthesis_set_attack(ATTACK_INSTANT);
    synthesis_set_release(RELEASE_INSTANT);
    synthesis_gate_on();
    fill();
    synthesis_gate_off();
    fill();
    CHECK(is_silent(buf, BUF_SAMPLES), "output not silent after gate_off");
}

static void test_slow_attack_ramps_up(void)
{
    TEST("slow attack → output ramps up");
    synthesis_init();
    synthesis_set_attack(ATTACK_SLOW);
    synthesis_gate_on();

    int16_t small[SMALL_SAMPLES];
    fill_small(small);
    int16_t early_peak = peak_amplitude(small, SMALL_SAMPLES);

    for (int i = 0; i < 10; i++) fill();
    int16_t late_peak = peak_amplitude(buf, BUF_SAMPLES);
    CHECK(late_peak > early_peak, "late output not louder than early");
}

static void test_slow_release_ramps_down(void)
{
    TEST("slow release → output ramps down");
    synthesis_init();
    synthesis_set_attack(ATTACK_INSTANT);
    synthesis_set_release(RELEASE_SLOW);
    synthesis_gate_on();
    fill();
    int16_t pre_peak = peak_amplitude(buf, BUF_SAMPLES);

    synthesis_gate_off();
    for (int i = 0; i < 5; i++) fill();
    int16_t post_peak = peak_amplitude(buf, BUF_SAMPLES);

    CHECK(post_peak < pre_peak, "output not quieter during release");
}

static void test_decay_to_sustain(void)
{
    TEST("decay → gain settles at sustain_level");
    synthesis_init();
    synthesis_set_attack(ATTACK_INSTANT);
    synthesis_set_decay(DECAY_GRADUAL);
    synthesis_set_sustain(0.5f);
    synthesis_set_release(RELEASE_INSTANT);
    synthesis_gate_on();

    for (int i = 0; i < 100; i++) fill();
    int16_t sustain_peak = peak_amplitude(buf, BUF_SAMPLES);

    /* Compare with full-gain reference */
    synthesis_init();
    synthesis_set_attack(ATTACK_INSTANT);
    synthesis_set_decay(DECAY_NONE);
    synthesis_gate_on();
    fill();
    int16_t full_peak = peak_amplitude(buf, BUF_SAMPLES);

    float ratio = (float)sustain_peak / (float)full_peak;
    CHECK(ratio > 0.45f && ratio < 0.55f,
          "sustain ratio not near 0.5 (expected 0.45-0.55)");
}

static void test_no_decay_skips_to_sustain(void)
{
    TEST("decay_factor=1.0 → skip DECAY, gain stays at 1.0");
    synthesis_init();
    synthesis_set_attack(ATTACK_INSTANT);
    synthesis_set_decay(DECAY_NONE);
    synthesis_set_sustain(0.3f);  /* should NOT affect gain */
    synthesis_gate_on();
    fill();
    int16_t with_no_decay = peak_amplitude(buf, BUF_SAMPLES);

    synthesis_init();
    synthesis_set_attack(ATTACK_INSTANT);
    synthesis_set_decay(DECAY_NONE);
    synthesis_set_sustain(1.0f);
    synthesis_gate_on();
    fill();
    int16_t with_full_sustain = peak_amplitude(buf, BUF_SAMPLES);

    float ratio = (float)with_no_decay / (float)with_full_sustain;
    CHECK(ratio > 0.95f && ratio < 1.05f,
          "decay_factor=1.0 did not skip to full gain");
}

static void test_trigger_resets_diff_and_attacks(void)
{
    TEST("trigger → diff reset + ATTACK");
    synthesis_init();
    synthesis_set_attack(ATTACK_INSTANT);
    synthesis_gate_on();
    fill();
    synthesis_gate_off();
    fill();
    CHECK(is_silent(buf, BUF_SAMPLES), "not silent before trigger");

    synthesis_trigger();
    fill();
    CHECK(!is_silent(buf, BUF_SAMPLES), "output silent after trigger");
}

static void test_gate_on_during_release(void)
{
    TEST("gate_on during release → re-attack from current gain");
    synthesis_init();
    synthesis_set_attack(ATTACK_INSTANT);
    synthesis_set_release(RELEASE_VSLOW);
    synthesis_gate_on();
    fill();

    synthesis_gate_off();
    int16_t small[SMALL_SAMPLES];
    fill_small(small);
    int16_t mid_release = peak_amplitude(small, SMALL_SAMPLES);

    synthesis_gate_on();
    fill();
    int16_t after_reattack = peak_amplitude(buf, BUF_SAMPLES);

    CHECK(after_reattack > mid_release,
          "re-attack output not louder than mid-release");
}

static void test_percussive_envelope(void)
{
    TEST("percussive: sustain=0 + decay → decays to silence");
    synthesis_init();
    synthesis_set_attack(ATTACK_INSTANT);
    synthesis_set_decay(DECAY_KICK);
    synthesis_set_sustain(0.0f);
    synthesis_trigger();

    fill();
    int16_t early = peak_amplitude(buf, BUF_SAMPLES);

    /* 0.9995^19200 < 0.001 → threshold reached */
    for (int i = 0; i < 300; i++) fill();
    int16_t late = peak_amplitude(buf, BUF_SAMPLES);

    CHECK(early > 0 && late <= 1,
          "percussive did not decay to silence");
}

static void test_pipeline_runs_during_idle(void)
{
    TEST("pipeline runs during IDLE (diff stays warm)");
    synthesis_init();
    for (int i = 0; i < 10; i++) fill();
    CHECK(is_silent(buf, BUF_SAMPLES), "not silent during IDLE");

    synthesis_set_attack(ATTACK_INSTANT);
    synthesis_gate_on();
    fill();
    CHECK(!is_silent(buf, BUF_SAMPLES), "silent after gate_on (diff cold)");
}

static void test_default_values(void)
{
    TEST("defaults: instant attack/release, no decay, sustain=0.7");
    synthesis_init();
    /* With defaults: decay_factor=1.0 → skip DECAY → gain stays 1.0 */
    synthesis_gate_on();
    fill();
    int16_t at_default = peak_amplitude(buf, BUF_SAMPLES);

    synthesis_init();
    synthesis_set_attack(ATTACK_INSTANT);
    synthesis_set_decay(DECAY_NONE);
    synthesis_set_sustain(1.0f);
    synthesis_gate_on();
    fill();
    int16_t at_full = peak_amplitude(buf, BUF_SAMPLES);

    float ratio = (float)at_default / (float)at_full;
    CHECK(ratio > 0.95f,
          "default envelope did not reach full gain");
}

static void test_sustain_holds_level(void)
{
    TEST("sustain phase holds steady level");
    synthesis_init();
    synthesis_set_attack(ATTACK_INSTANT);
    synthesis_set_decay(DECAY_GRADUAL);
    synthesis_set_sustain(0.5f);
    synthesis_gate_on();

    for (int i = 0; i < 200; i++) fill();
    int16_t peak1 = peak_amplitude(buf, BUF_SAMPLES);

    for (int i = 0; i < 100; i++) fill();
    int16_t peak2 = peak_amplitude(buf, BUF_SAMPLES);

    CHECK(peak1 > 0, "sustain peak1 is zero");

    float ratio = (float)peak2 / (float)peak1;
    CHECK(ratio > 0.95f && ratio < 1.05f,
          "sustain level not stable (expected 0.95-1.05)");
}

static void test_zero_sustain_decays_to_idle(void)
{
    TEST("sustain=0 + decay → reaches IDLE");
    synthesis_init();
    synthesis_set_attack(ATTACK_INSTANT);
    synthesis_set_decay(DECAY_FAST);
    synthesis_set_sustain(0.0f);
    synthesis_gate_on();

    for (int i = 0; i < 500; i++) fill();

    CHECK(is_silent(buf, BUF_SAMPLES),
          "not silent after full decay to sustain=0");
}

/* --- Tests: Edge cases --- */

static void test_gate_on_during_attack(void)
{
    TEST("gate_on during ATTACK → continues attack");
    synthesis_init();
    synthesis_set_attack(ATTACK_SLOW);
    synthesis_gate_on();

    for (int i = 0; i < 5; i++) fill();
    int16_t mid_attack = peak_amplitude(buf, BUF_SAMPLES);

    /* Re-trigger gate_on during attack */
    synthesis_gate_on();
    for (int i = 0; i < 5; i++) fill();
    int16_t after_regate = peak_amplitude(buf, BUF_SAMPLES);

    /* Gain should still be at least at the same level (within phase jitter) */
    CHECK(after_regate > 0 && after_regate * 10 >= mid_attack * 9,
          "gate_on during ATTACK broke envelope");
}

static void test_gate_off_during_attack(void)
{
    TEST("gate_off during ATTACK → transition to RELEASE");
    synthesis_init();
    synthesis_set_attack(ATTACK_SLOW);
    synthesis_set_release(RELEASE_SLOW);
    synthesis_gate_on();

    for (int i = 0; i < 5; i++) fill();
    int16_t mid_attack = peak_amplitude(buf, BUF_SAMPLES);

    synthesis_gate_off();
    for (int i = 0; i < 20; i++) fill();
    int16_t after_release = peak_amplitude(buf, BUF_SAMPLES);

    CHECK(after_release < mid_attack,
          "gate_off during ATTACK did not start RELEASE");
}

static void test_trigger_during_sustain(void)
{
    TEST("trigger during SUSTAIN → restart from current gain");
    synthesis_init();
    synthesis_set_attack(ATTACK_INSTANT);
    synthesis_set_decay(DECAY_GRADUAL);
    synthesis_set_sustain(0.5f);
    synthesis_gate_on();

    for (int i = 0; i < 100; i++) fill();
    int16_t at_sustain = peak_amplitude(buf, BUF_SAMPLES);

    synthesis_trigger();
    fill();
    int16_t after_trigger = peak_amplitude(buf, BUF_SAMPLES);

    /* trigger with instant attack → gain back to 1.0 → louder */
    CHECK(after_trigger > at_sustain,
          "trigger during SUSTAIN did not re-attack");
}

static void test_clamp_negative_attack(void)
{
    TEST("set_attack(-0.1) → clamped to 0, gain stays 0");
    synthesis_init();
    synthesis_set_attack(-0.1f);
    synthesis_gate_on();

    int16_t small[SMALL_SAMPLES];
    fill_small(small);

    CHECK(is_silent(small, SMALL_SAMPLES),
          "negative attack not clamped to 0");
}

static void test_clamp_sustain_over_one(void)
{
    TEST("set_sustain(1.5) → clamped to 1.0");
    synthesis_init();
    synthesis_set_attack(ATTACK_INSTANT);
    synthesis_set_decay(DECAY_NONE);
    synthesis_set_sustain(1.5f);
    synthesis_gate_on();
    fill();
    int16_t peak = peak_amplitude(buf, BUF_SAMPLES);

    synthesis_init();
    synthesis_set_attack(ATTACK_INSTANT);
    synthesis_set_decay(DECAY_NONE);
    synthesis_set_sustain(1.0f);
    synthesis_gate_on();
    fill();
    int16_t ref = peak_amplitude(buf, BUF_SAMPLES);

    float ratio = (float)peak / (float)ref;
    CHECK(ratio > 0.95f && ratio < 1.05f,
          "sustain(1.5) not clamped to 1.0");
}

static void test_clamp_negative_decay(void)
{
    TEST("set_decay(-0.5) → clamped to 0, instant decay");
    synthesis_init();
    synthesis_set_attack(ATTACK_INSTANT);
    synthesis_set_decay(-0.5f);
    synthesis_set_sustain(0.5f);
    synthesis_gate_on();
    fill();

    /* decay_factor=0 → gain *= 0 → immediately at sustain_level */
    int16_t peak = peak_amplitude(buf, BUF_SAMPLES);

    synthesis_init();
    synthesis_set_attack(ATTACK_INSTANT);
    synthesis_set_decay(DECAY_NONE);
    synthesis_set_sustain(0.5f);
    synthesis_gate_on();
    /* Let it decay to sustain normally for reference */
    synthesis_set_decay(DECAY_GRADUAL);
    for (int i = 0; i < 200; i++) fill();
    int16_t ref = peak_amplitude(buf, BUF_SAMPLES);

    /* Both should be at sustain_level=0.5 */
    float ratio = (ref > 0) ? (float)peak / (float)ref : 0.0f;
    CHECK(ratio > 0.8f && ratio < 1.2f,
          "negative decay not clamped correctly");
}

static void test_clamp_release_over_one(void)
{
    TEST("set_release(2.0) → clamped to 1.0, instant release");
    synthesis_init();
    synthesis_set_attack(ATTACK_INSTANT);
    synthesis_set_release(2.0f);
    synthesis_gate_on();
    fill();
    synthesis_gate_off();
    fill();

    CHECK(is_silent(buf, BUF_SAMPLES),
          "release(2.0) not clamped to instant");
}

/* --- Main --- */

int main(void)
{
    printf("ADSR Envelope Tests\n");
    printf("===================\n\n");

    /* State transitions */
    test_init_produces_silence();
    test_gate_on_instant_attack();
    test_gate_off_instant_release();
    test_slow_attack_ramps_up();
    test_slow_release_ramps_down();
    test_decay_to_sustain();
    test_no_decay_skips_to_sustain();
    test_trigger_resets_diff_and_attacks();
    test_gate_on_during_release();
    test_percussive_envelope();
    test_pipeline_runs_during_idle();
    test_default_values();
    test_sustain_holds_level();
    test_zero_sustain_decays_to_idle();

    /* Edge cases */
    test_gate_on_during_attack();
    test_gate_off_during_attack();
    test_trigger_during_sustain();

    /* Parameter clamping */
    test_clamp_negative_attack();
    test_clamp_sustain_over_one();
    test_clamp_negative_decay();
    test_clamp_release_over_one();

    printf("\n--- Results: %d passed, %d failed ---\n",
           passed, failed);

    return failed > 0 ? 1 : 0;
}
