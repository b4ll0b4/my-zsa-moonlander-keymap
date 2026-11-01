// Copyright 2024-2025 Google LLC
// Copyright 2026 Christopher Rossetti
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Adapted from Pascal Getreuer's SOCD Cleaner. Local changes: gaming-only,
// default off, mode keys, status LEDs, and held-key reconciliation on changes.

#include QMK_KEYBOARD_H
#include "socd_cleaner.h"

ASSERT_COMMUNITY_MODULES_MIN_API_VERSION(1, 1, 0);

#ifndef SOCD_GAMING_LAYER
#    define SOCD_GAMING_LAYER 1
#endif

socd_cleaner_t socd_opposing_pairs[2] = {
    {{KC_SPACE, KC_D}, SOCD_CLEANER_LAST, {false, false}},
    {{KC_S, KC_F}, SOCD_CLEANER_LAST, {false, false}},
};

bool socd_cleaner_enabled = false;
static uint8_t last_pressed[2];
static bool up_priority;

static bool gaming(layer_state_t state) {
    return get_highest_layer(state) == SOCD_GAMING_LAYER;
}

static void update_leds(layer_state_t state) {
    uint8_t layer = get_highest_layer(state);
    // ZSA's macros cast their argument without enclosing it in parentheses.
    STATUS_LED_1((layer == 1));
    STATUS_LED_2((layer == 2));
    STATUS_LED_3((layer == 3));
    STATUS_LED_4(false);
    STATUS_LED_5((gaming(state) && socd_cleaner_enabled && up_priority));
    STATUS_LED_6((gaming(state) && socd_cleaner_enabled && !up_priority));
}

static void reconcile_pair(uint8_t pair_index) {
    socd_cleaner_t *pair = &socd_opposing_pairs[pair_index];
    bool down[2] = {pair->held[0], pair->held[1]};
    if (socd_cleaner_enabled && down[0] && down[1]) {
        switch (pair->resolution) {
            case SOCD_CLEANER_LAST:
                down[last_pressed[pair_index] ^ 1] = false;
                break;
            case SOCD_CLEANER_NEUTRAL:
                down[0] = down[1] = false;
                break;
            case SOCD_CLEANER_0_WINS:
                down[1] = false;
                break;
            case SOCD_CLEANER_1_WINS:
                down[0] = false;
                break;
        }
    }
    // Remove suppressed keys first so the report also works near the 6KRO limit.
    for (uint8_t i = 0; i < 2; ++i) {
        if (!down[i]) del_key(pair->keys[i]);
    }
    for (uint8_t i = 0; i < 2; ++i) {
        if (down[i]) add_key(pair->keys[i]);
    }
}

static void reconcile_all(void) {
    for (uint8_t i = 0; i < 2; ++i) reconcile_pair(i);
    send_keyboard_report();
}

static void set_enabled(bool enabled) {
    if (socd_cleaner_enabled != enabled) {
        socd_cleaner_enabled = enabled;
        // Keep physical holds while switching modes or bypassing the cleaner.
        // Clearing the state here would lose held keys or reactivate stale ones.
        reconcile_all();
    }
}

void keyboard_post_init_socd_cleaner(void) {
    keyboard_post_init_socd_cleaner_kb();
    socd_cleaner_enabled = false;
    update_leds(layer_state);
}

layer_state_t layer_state_set_socd_cleaner(layer_state_t state) {
    state = layer_state_set_socd_cleaner_kb(state);
    // Leaving gaming cancels the opt-in. Re-entering always starts disabled.
    if (!gaming(state)) set_enabled(false);
    update_leds(state);
    return state;
}

void housekeeping_task_socd_cleaner(void) {
    housekeeping_task_socd_cleaner_kb();
    update_leds(layer_state);
}

bool process_record_socd_cleaner(uint16_t keycode, keyrecord_t *record) {
    if (!process_record_socd_cleaner_kb(keycode, record)) return false;
    bool in_game = gaming(layer_state);
    if (!in_game) set_enabled(false);

    switch (keycode) {
        case SOCDON:
        case SOCDOFF:
        case SOCDTOG:
        case SOCD_LAST:
        case SOCD_UP:
            if (record->event.pressed && in_game) {
                if (keycode == SOCD_LAST || keycode == SOCD_UP) {
                    up_priority = keycode == SOCD_UP;
                    socd_opposing_pairs[0].resolution = up_priority ? SOCD_CLEANER_0_WINS : SOCD_CLEANER_LAST;
                    socd_opposing_pairs[1].resolution = up_priority ? SOCD_CLEANER_NEUTRAL : SOCD_CLEANER_LAST;
                    reconcile_all();
                } else {
                    set_enabled(keycode == SOCDON || (keycode == SOCDTOG && !socd_cleaner_enabled));
                }
                update_leds(layer_state);
            }
            return false;
    }

    for (uint8_t p = 0; p < 2; ++p) {
        socd_cleaner_t *pair = &socd_opposing_pairs[p];
        if (keycode != pair->keys[0] && keycode != pair->keys[1]) continue;
        uint8_t i = keycode == pair->keys[1];
        // Track releases even while disabled and outside gaming. Toggle/mode
        // changes can then rebuild the report without stuck or missing keys.
        pair->held[i] = record->event.pressed;
        if (record->event.pressed) last_pressed[p] = i;
        if (socd_cleaner_enabled) {
            reconcile_pair(p);
            send_keyboard_report();
            return false;
        }
        break;
    }
    return true;
}
