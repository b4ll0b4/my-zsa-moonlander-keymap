#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef uint8_t layer_state_t;
typedef struct { struct { bool pressed; } event; } keyrecord_t;
enum { KC_D = 7, KC_F = 9, KC_S = 22, KC_SPACE = 44,
       SOCDON = 0x7e00, SOCDOFF, SOCDTOG, SOCD_LAST, SOCD_UP };
extern layer_state_t layer_state;
extern bool leds[6];
#define QMK_KEYBOARD_H "quantum.h"
#define ASSERT_COMMUNITY_MODULES_MIN_API_VERSION(a, b, c)
// Preserve ZSA's unparenthesized cast to catch expression-precedence mistakes.
#define STATUS_LED_1(on) (leds[0] = (bool)on)
#define STATUS_LED_2(on) (leds[1] = (bool)on)
#define STATUS_LED_3(on) (leds[2] = (bool)on)
#define STATUS_LED_4(on) (leds[3] = (bool)on)
#define STATUS_LED_5(on) (leds[4] = (bool)on)
#define STATUS_LED_6(on) (leds[5] = (bool)on)
uint8_t get_highest_layer(layer_state_t state);
void add_key(uint8_t code);
void del_key(uint8_t code);
void send_keyboard_report(void);
void keyboard_post_init_socd_cleaner_kb(void);
layer_state_t layer_state_set_socd_cleaner_kb(layer_state_t state);
void housekeeping_task_socd_cleaner_kb(void);
bool process_record_socd_cleaner_kb(uint16_t code, keyrecord_t *record);
