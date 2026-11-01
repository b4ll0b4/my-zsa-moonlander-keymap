// Exercise the actual firmware module with a minimal QMK keyboard-report stub.
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "quantum.h"
#include "../modules/getreuer/socd_cleaner/socd_cleaner.c"

layer_state_t layer_state;
bool leds[6];
static bool report[256];
static unsigned checks;

uint8_t get_highest_layer(layer_state_t state) {
    uint8_t result = 0;
    while (state >>= 1) ++result;
    return result;
}
void add_key(uint8_t code) { report[code] = true; }
void del_key(uint8_t code) { report[code] = false; }
void send_keyboard_report(void) {}
void keyboard_post_init_socd_cleaner_kb(void) {}
layer_state_t layer_state_set_socd_cleaner_kb(layer_state_t state) { return state; }
void housekeeping_task_socd_cleaner_kb(void) {}
bool process_record_socd_cleaner_kb(uint16_t code, keyrecord_t *record) {
    (void)code; (void)record; return true;
}

static void event(uint16_t code, bool down) {
    keyrecord_t record = {{down}};
    if (process_record_socd_cleaner(code, &record)) {
        assert(code < 256);
        report[code] = down;
    }
    housekeeping_task_socd_cleaner();
}
static void tap(uint16_t code) { event(code, true); event(code, false); }
static void layer(uint8_t number) {
    layer_state = layer_state_set_socd_cleaner((layer_state_t)(1u << number));
    housekeeping_task_socd_cleaner();
}
static void expect(uint8_t a, bool down_a, uint8_t b, bool down_b) {
    assert(report[a] == down_a);
    assert(report[b] == down_b);
    ++checks;
}
static void release_directions(void) {
    event(KC_SPACE, false); event(KC_D, false);
    event(KC_S, false); event(KC_F, false);
}

int main(void) {
    keyboard_post_init_socd_cleaner();
    assert(!socd_cleaner_enabled);
    // Control keycodes cannot enable filtering outside gaming.
    tap(SOCDON); tap(SOCDTOG); tap(SOCD_UP);
    assert(!socd_cleaner_enabled);
    event(KC_S, true); event(KC_F, true);
    expect(KC_S, true, KC_F, true);
    release_directions();

    layer(1);
    assert(leds[0] && !leds[3] && !leds[4] && !leds[5]);
    tap(SOCD_LAST); // Selection alone must not enable it.
    assert(!socd_cleaner_enabled);
    event(KC_S, true); event(KC_F, true);
    expect(KC_S, true, KC_F, true);
    tap(SOCDTOG); // Enabling with both held resolves immediately.
    assert(socd_cleaner_enabled && !leds[3] && leds[4] && !leds[5]);
    expect(KC_S, false, KC_F, true);
    event(KC_F, false);
    expect(KC_S, true, KC_F, false);
    event(KC_F, true);
    tap(SOCD_UP); // Horizontal neutral, even when changed mid-hold.
    expect(KC_S, false, KC_F, false);
    assert(!leds[3] && !leds[4] && leds[5]);
    event(KC_F, false);
    expect(KC_S, true, KC_F, false);
    event(KC_SPACE, true); event(KC_D, true);
    expect(KC_SPACE, true, KC_D, false);
    event(KC_SPACE, false);
    expect(KC_SPACE, false, KC_D, true);
    tap(SOCDTOG);
    assert(!socd_cleaner_enabled && !leds[3] && !leds[4] && !leds[5]);
    expect(KC_SPACE, false, KC_D, true);
    // Releases while disabled must not leave stale state on re-enable.
    release_directions();
    tap(SOCDTOG);
    expect(KC_SPACE, false, KC_D, false);
    expect(KC_S, false, KC_F, false);
    event(KC_S, true); event(KC_F, true);
    expect(KC_S, false, KC_F, false);
    layer(0); // Layer exit restores physical holds and disables filtering.
    assert(!socd_cleaner_enabled && !leds[3] && !leds[4] && !leds[5]);
    expect(KC_S, true, KC_F, true);
    release_directions();
    layer(1);
    assert(!socd_cleaner_enabled);
    layer(2);
    assert(!leds[0] && leds[1] && !leds[2] && !leds[3] && !leds[4] && !leds[5]);
    layer(3);
    assert(!leds[0] && !leds[1] && leds[2] && !leds[3] && !leds[4] && !leds[5]);

    // Exhaustively check press/release ordering for both pairs and both modes.
    const uint8_t pairs[2][2] = {{KC_SPACE, KC_D}, {KC_S, KC_F}};
    for (uint8_t mode = 0; mode < 2; ++mode) {
        for (uint8_t p = 0; p < 2; ++p) {
            for (uint8_t first = 0; first < 2; ++first) {
                for (uint8_t released = 0; released < 2; ++released) {
                    release_directions();
                    layer(1); tap(mode ? SOCD_UP : SOCD_LAST); tap(SOCDON);
                    uint8_t a = pairs[p][0], b = pairs[p][1];
                    event(pairs[p][first], true);
                    event(pairs[p][first ^ 1], true);
                    if (!mode) expect(a, first == 1, b, first == 0);
                    else if (!p) expect(a, true, b, false);
                    else expect(a, false, b, false);
                    event(pairs[p][released], false);
                    expect(a, released != 0, b, released != 1);
                    event(pairs[p][released ^ 1], false);
                    expect(a, false, b, false);
                    tap(SOCDOFF);
                }
            }
        }
    }
    printf("SOCD: %u report checks passed, including modes, layers, LEDs, and held-key transitions.\n", checks);
    return 0;
}
