#include <math.h>
#include <stdio.h>
#include <string.h>

#include "watasoge_module.h"

/* --- Platform stubs (replace HAL on host) --- */
static uint32_t fake_tick_ms;
uint32_t watasoge_get_tick_ms(void) { return fake_tick_ms; }
void     watasoge_toggle_led(void)  { /* no-op */ }

/* --- Test infrastructure --- */
static int passed, failed;

#define TEST(name) do { printf("  %-55s ", name); } while (0)
#define PASS()     do { printf("OK\n"); passed++; } while (0)
#define FAIL(msg)  do { printf("FAIL (line %d): %s\n", __LINE__, msg); failed++; } while (0)
#define CHECK(cond, msg) do { if (cond) { PASS(); } else { FAIL(msg); } } while (0)

#define BUF_SIZE (64 * 2)
static int16_t test_buf[BUF_SIZE];

static int16_t peak_amplitude(const int16_t *b, uint16_t n)
{
    int16_t peak = 0;
    for (uint16_t i = 0; i < n; i++) {
        int16_t a = b[i] < 0 ? (int16_t)(-b[i]) : b[i];
        if (a > peak) peak = a;
    }
    return peak;
}

/* --- Tests --- */

static void test_init_sets_defaults(void)
{
    TEST("init sets default frequency, wave, volume");
    watasoge_init(NULL);
    CHECK(fabsf(watasoge_ctx.frequency - 440.0f) < 0.01f &&
          watasoge_ctx.wave_index == 0 &&
          fabsf(watasoge_ctx.volume - 1.0f) < 0.01f &&
          watasoge_ctx.gate_on == false,
          "defaults not set correctly");
}

static void test_cv_pitch_0v_is_c4(void)
{
    TEST("CV value=0 → ~261.63 Hz (C4 at 0V)");
    watasoge_init(NULL);
    watasoge_on_cv(NULL, 0, 0);
    CHECK(fabsf(watasoge_ctx.frequency - 261.63f) < 1.0f,
          "frequency not near C4 at 0V");
}

static void test_cv_pitch_1v_is_c5(void)
{
    TEST("CV +1V → ~523 Hz (C5, one octave up)");
    watasoge_init(NULL);
    watasoge_on_cv(NULL, 0, 6553);
    float ratio = watasoge_ctx.frequency / (261.63f * 2.0f);
    CHECK(ratio > 0.95f && ratio < 1.05f, "frequency not near C5 at 1V");
}

static void test_cv_negative_voltage(void)
{
    TEST("CV -1V → ~130.8 Hz (C3, one octave down)");
    watasoge_init(NULL);
    watasoge_on_cv(NULL, 0, -6553);
    float ratio = watasoge_ctx.frequency / (261.63f / 2.0f);
    CHECK(ratio > 0.95f && ratio < 1.05f, "frequency not near C3 at -1V");
}

static void test_cv_clamping_low(void)
{
    TEST("CV extreme negative → clamped to 8 Hz");
    watasoge_init(NULL);
    watasoge_on_cv(NULL, 0, -32767);
    CHECK(watasoge_ctx.frequency >= 8.0f, "frequency below 8 Hz");
}

static void test_cv_clamping_high(void)
{
    TEST("CV extreme positive → clamped to 12000 Hz");
    watasoge_init(NULL);
    watasoge_on_cv(NULL, 0, 32767);
    CHECK(watasoge_ctx.frequency <= 12000.0f, "frequency above 12000 Hz");
}

static void test_cv_wrong_port_ignored(void)
{
    TEST("CV port=1 → ignored");
    watasoge_init(NULL);
    float before = watasoge_ctx.frequency;
    watasoge_on_cv(NULL, 1, 10000);
    CHECK(watasoge_ctx.frequency == before, "port 1 was not ignored");
}

static void test_cc_wave_min(void)
{
    TEST("CC param=0, value=-32767 → wave index 0");
    watasoge_init(NULL);
    watasoge_on_cc(NULL, 0, 0, -32767);
    CHECK(watasoge_ctx.wave_index == 0, "min CC did not map to wave 0");
}

static void test_cc_wave_max(void)
{
    TEST("CC param=0, value=+32767 → wave index 219");
    watasoge_init(NULL);
    watasoge_on_cc(NULL, 0, 0, 32767);
    CHECK(watasoge_ctx.wave_index == 219, "max CC did not map to wave 219");
}

static void test_cc_volume_min(void)
{
    TEST("CC param=5, value=-32767 → volume ~0.0");
    watasoge_init(NULL);
    watasoge_on_cc(NULL, 0, 5, -32767);
    CHECK(watasoge_ctx.volume < 0.01f, "min CC did not set volume near 0");
}

static void test_cc_volume_max(void)
{
    TEST("CC param=5, value=+32767 → volume ~1.0");
    watasoge_init(NULL);
    watasoge_on_cc(NULL, 0, 5, 32767);
    CHECK(watasoge_ctx.volume > 0.99f, "max CC did not set volume near 1");
}

static void test_cc_wrong_port_ignored(void)
{
    TEST("CC port=1 → ignored");
    watasoge_init(NULL);
    watasoge_on_cc(NULL, 1, 0, 32767);
    CHECK(watasoge_ctx.wave_index == 0, "CC port 1 was not ignored");
}

static void test_gate_on_off(void)
{
    TEST("gate on/off → sets gate_on flag");
    watasoge_init(NULL);
    fake_tick_ms = 1000;
    watasoge_on_gate(NULL, 0, true);
    CHECK(watasoge_ctx.gate_on == true, "gate_on not set");
    watasoge_on_gate(NULL, 0, false);
    CHECK(watasoge_ctx.gate_on == false, "gate_on not cleared");
}

static void test_gate_records_timestamp(void)
{
    TEST("gate on → records timestamp");
    watasoge_init(NULL);
    fake_tick_ms = 4242;
    watasoge_on_gate(NULL, 0, true);
    CHECK(watasoge_ctx.last_gate_on_ms == 4242, "timestamp not recorded");
}

static void test_gate_wrong_port_ignored(void)
{
    TEST("gate port=1 → ignored");
    watasoge_init(NULL);
    watasoge_on_gate(NULL, 1, true);
    CHECK(watasoge_ctx.gate_on == false, "gate port 1 was not ignored");
}

static void test_trigger_wrong_port_ignored(void)
{
    TEST("trigger port=1 → no crash");
    watasoge_init(NULL);
    watasoge_on_trigger(NULL, 1);
    PASS();
}

static void test_process_audio_produces_output(void)
{
    TEST("process_audio → non-silent after gate_on");
    watasoge_init(NULL);
    fake_tick_ms = 0;
    watasoge_on_gate(NULL, 0, true);

    fracanto_audio_buffer_t output = {
        .data = test_buf,
        .block_size = 64,
        .num_channels = 2
    };
    memset(test_buf, 0, sizeof(test_buf));
    watasoge_process_audio(NULL, NULL, &output);

    CHECK(peak_amplitude(test_buf, BUF_SIZE) > 0,
          "output silent after gate_on");
}

static void test_process_audio_volume_scaling(void)
{
    TEST("process_audio → volume=0.5 reduces output ~50%");
    watasoge_init(NULL);
    fake_tick_ms = 0;
    watasoge_on_gate(NULL, 0, true);

    fracanto_audio_buffer_t output = {
        .data = test_buf,
        .block_size = 64,
        .num_channels = 2
    };
    memset(test_buf, 0, sizeof(test_buf));
    watasoge_process_audio(NULL, NULL, &output);
    int16_t full_peak = peak_amplitude(test_buf, BUF_SIZE);

    /* Re-init for fair comparison (reset phase) */
    watasoge_init(NULL);
    fake_tick_ms = 0;
    watasoge_on_gate(NULL, 0, true);
    watasoge_ctx.volume = 0.5f;
    memset(test_buf, 0, sizeof(test_buf));
    watasoge_process_audio(NULL, NULL, &output);
    int16_t half_peak = peak_amplitude(test_buf, BUF_SIZE);

    float ratio = (full_peak > 0) ? (float)half_peak / (float)full_peak : 0.0f;
    CHECK(ratio > 0.4f && ratio < 0.6f,
          "volume=0.5 did not reduce to ~50%");
}

static void test_ops_vtable_complete(void)
{
    TEST("watasoge_ops has all required callbacks set");
    CHECK(watasoge_ops.init != NULL &&
          watasoge_ops.on_cv != NULL &&
          watasoge_ops.on_cc != NULL &&
          watasoge_ops.on_gate != NULL &&
          watasoge_ops.on_trigger != NULL &&
          watasoge_ops.process_audio != NULL &&
          watasoge_ops.tick != NULL,
          "missing callback in vtable");
}

/* --- Main --- */

int main(void)
{
    printf("Watasoge Module Tests\n");
    printf("=====================\n\n");

    test_init_sets_defaults();
    test_cv_pitch_0v_is_c4();
    test_cv_pitch_1v_is_c5();
    test_cv_negative_voltage();
    test_cv_clamping_low();
    test_cv_clamping_high();
    test_cv_wrong_port_ignored();
    test_cc_wave_min();
    test_cc_wave_max();
    test_cc_volume_min();
    test_cc_volume_max();
    test_cc_wrong_port_ignored();
    test_gate_on_off();
    test_gate_records_timestamp();
    test_gate_wrong_port_ignored();
    test_trigger_wrong_port_ignored();
    test_process_audio_produces_output();
    test_process_audio_volume_scaling();
    test_ops_vtable_complete();

    printf("\n--- Results: %d passed, %d failed ---\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
