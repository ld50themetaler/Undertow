// Copyright 2021 Hayashi (@w_vwbw)
// SPDX-License-Identifier: GPL-2.0-or-later

#include "lib/common_undertow.h"
#include "analog.h"
#include "math.h"
#include "os_detection.h"
#include "joystick.h"
#include "lib/add_keycodes.h"
#include "lib/add_oled.h"

joystick_config_t joystick_axes[JOYSTICK_AXIS_COUNT] = {
    JOYSTICK_AXIS_VIRTUAL,
    JOYSTICK_AXIS_VIRTUAL
};

// ドリフト防止用の定数
#define TRACKBALL_DEADZONE 1      // センサーの生値がこの値以下なら無視
#define SMOOTHING_CUTOFF   0.05f  // 計算後の微小な移動量を0にするしきい値
#define JS_MIN_SPAN        5.0f   // ジョイスティックの稼働範囲がこれ以下なら無効（ゼロ除算防止）

/* ポインティングデバイス用変数 */
ut_config_t ut_config;         // eeprom保存用
bool force_scrolling, force_cursoring, force_key_input, force_gaming, slow_mode; // 一時的モード変更用
uint8_t joystick_attached;     // ジョイスティックの有無
bool joystick_initialized;
float prev_x_0, prev_y_0, prev_x_1, prev_y_1;
float x_accumulator, y_accumulator, h_accumulator, v_accumulator; // 端数保存用
int16_t gp29_newt, gp28_newt;               // ジョイスティックの初期値
int16_t gp29_max, gp28_max, gp29_min, gp28_min; // ジョイスティックの最大値、最小値
uint16_t joystick_offset_min, joystick_offset_max; // 無視する範囲

bool rgblayers;

// 仮想十字キー設定用
keypos_t key_up_0, key_down_0, key_left_0, key_right_0, key_up_1, key_down_1, key_left_1, key_right_1;
bool pressed_up, pressed_down, pressed_left, pressed_right;
int8_t layer;
int16_t keycode_up_0, keycode_down_0, keycode_left_0, keycode_right_0, keycode_up_1, keycode_down_1, keycode_left_1, keycode_right_1;
int16_t keycode_up_js, keycode_down_js, keycode_left_js, keycode_right_js;
int16_t key_timer_0, key_timer_1;

// 斜め入力防止用
bool dpad_exclusion;
uint8_t dpad_pressed_0, dpad_pressed_1;

/* eeprom 初期化 */
void eeconfig_init_kb(void) {
    ut_config.spd_0 = SPD_DEFAULT_SIDE0;
    ut_config.spd_1 = SPD_DEFAULT_SIDE1;
    ut_config.angle_0 = ANGLE_DEFAULT_SIDE0;
    ut_config.angle_1 = ANGLE_DEFAULT_SIDE1;
    ut_config.pd_mode_0 = TB_SIDE0_DEFAULT;
    ut_config.pd_mode_1 = TB_SIDE1_DEFAULT;
    ut_config.inv_0 = SIDE0_INVERT;
    ut_config.inv_1 = SIDE1_INVERT;
    ut_config.inv_sc = SCROLL_INVERT;
    ut_config.auto_mouse = AUTO_MOUSE_DEFAULT;
    ut_config.oled_mode = OLED_DEFAULT;
    ut_config.js_side = JS_SIDE_DEFAULT;
    ut_config.scrl_spd = SCRL_SPD_DEFAULT;
    eeconfig_update_kb(ut_config.raw);
    eeconfig_init_user();
}

/* キースキャン */
bool is_mouse_record_kb(uint16_t keycode, keyrecord_t* record) {
    switch(keycode) {
        case MOD_CUR: case MOD_SCRL: case MOD_SLOW: return true;
        default: return false;
    }
    return is_mouse_record_user(keycode, record);
}

bool process_record_kb(uint16_t keycode, keyrecord_t* record) {
    process_record_addedkeycodes(keycode, record);
    keypos_t key = record->event.key;
    if(key.col == 5 && dpad_exclusion){
        if(key.row <= 3){
            if(dpad_pressed_0 == 0 && record->event.pressed){
                dpad_pressed_0 = key.row;
            }else if(dpad_pressed_0 == key.row && !record->event.pressed){
                dpad_pressed_0 = 0;
            }else if(record->event.pressed){
                return false;
            }
        }else if(key.row == 6 && !joystick_initialized){
            joystick_attached = 0;
            ut_config.js_side = 0;
            eeconfig_update_kb(ut_config.raw);
        }
    }
    if(key.col == 7 && dpad_exclusion){
        if(key.row <= 3){
            if(dpad_pressed_1 == 0 && record->event.pressed){
                dpad_pressed_1 = key.row;
            }else if(dpad_pressed_1 == key.row && !record->event.pressed){
                dpad_pressed_1 = 0;
            }else if(record->event.pressed){
                return false;
            }
        }else if(key.row == 4 && !joystick_initialized){
            joystick_attached = 1;
            ut_config.js_side = 1;
            eeconfig_update_kb(ut_config.raw);
        }
    }
    return process_record_user(keycode, record);
}

/* マトリクス初期化 */
void matrix_init_kb(void) {
    key_up_0 = (keypos_t){.row = 3, .col = 5};
    key_down_0 = (keypos_t){.row = 1, .col = 5};
    key_left_0 = (keypos_t){.row = 2, .col = 5};
    key_right_0 = (keypos_t){.row = 0, .col = 5};
    key_up_1 = (keypos_t){.row = 3, .col = 7};
    key_down_1 = (keypos_t){.row = 1, .col = 7};
    key_left_1 = (keypos_t){.row = 2, .col = 7};
    key_right_1 = (keypos_t){.row = 0, .col = 7};
    dpad_pressed_0 = dpad_pressed_1 = 0;
    force_scrolling = force_cursoring = force_key_input = force_gaming = slow_mode = false;
    pressed_up = pressed_down = pressed_left = pressed_right = false;

    gp29_newt = analogReadPin(GP29);
    gp28_newt = analogReadPin(GP28);
    gp29_max = gp29_min = gp29_newt;
    gp28_max = gp28_min = gp28_newt;
    joystick_offset_min = JOYSTICK_OFFSET_MIN_DEFAULT;
    joystick_offset_max = JOYSTICK_OFFSET_MAX_DEFAULT;

    joystick_attached = (gp28_newt < NO_JOYSTICK_VAL || gp29_newt < NO_JOYSTICK_VAL) ? 2 : JS_SIDE_DEFAULT;
    joystick_initialized = false;
    key_timer_0 = key_timer_1 = timer_read();
    dpad_exclusion = DPAD_EX_DEFAULT;
    rgblayers = RGB_LAYER_DEFAULT;
    matrix_init_user();
}

void pointing_device_init_kb(void){
    ut_config.raw = eeconfig_read_kb();
    prev_x_0 = prev_y_0 = prev_x_1 = prev_y_1 = 0.0f;
    h_accumulator = v_accumulator = x_accumulator = y_accumulator = 0.0f;
    pmw33xx_init(1);
    pmw33xx_set_cpi(0, 1000 + ut_config.spd_0 * 250);
    pmw33xx_set_cpi(1, 1000 + ut_config.spd_1 * 250);
    if(joystick_attached != 2) joystick_attached = ut_config.js_side;
    set_auto_mouse_enable(ut_config.auto_mouse);
    pointing_device_init_user();
}

/* メインタスク */
#define constrain_hid(amt) ((amt) < -127 ? -127 : ((amt) > 127 ? 127 : (amt)))

report_mouse_t pointing_device_task_kb(report_mouse_t mouse_report) {
    float x_rev_0 = 0, y_rev_0 = 0, h_rev_0 = 0, v_rev_0 = 0;
    float x_rev_1 = 0, y_rev_1 = 0, h_rev_1 = 0, v_rev_1 = 0;
    float x_rev_js = 0, y_rev_js = 0, h_rev_js = 0, v_rev_js = 0;

    /* SIDE0 (Trackball) */
    if (abs(mouse_report.x) <= TRACKBALL_DEADZONE) mouse_report.x = 0;
    if (abs(mouse_report.y) <= TRACKBALL_DEADZONE) mouse_report.y = 0;
    float rad = (float)ut_config.angle_0 * 12.0f * (M_PI / 180.0f) * -1.0f;
    x_rev_0 = + mouse_report.x * cosf(rad) - mouse_report.y * sinf(rad);
    y_rev_0 = + mouse_report.x * sinf(rad) + mouse_report.y * cosf(rad);

    float smoothed_x_0 = prev_x_0 * SMOOTHING_FACTOR + x_rev_0 * (1.0f - SMOOTHING_FACTOR);
    float smoothed_y_0 = prev_y_0 * SMOOTHING_FACTOR + y_rev_0 * (1.0f - SMOOTHING_FACTOR);
    if (fabsf(smoothed_x_0) < SMOOTHING_CUTOFF) { smoothed_x_0 = 0; prev_x_0 = 0; } else { prev_x_0 = smoothed_x_0; }
    if (fabsf(smoothed_y_0) < SMOOTHING_CUTOFF) { smoothed_y_0 = 0; prev_y_0 = 0; } else { prev_y_0 = smoothed_y_0; }

    float dynamic_multiplier_0 = fminf(fmaxf(1.0f + sqrtf(smoothed_x_0*smoothed_x_0 + smoothed_y_0*smoothed_y_0) / 10.0f, 0.5f), 3.0f);
    x_rev_0 *= SENSITIVITY_MULTIPLIER * dynamic_multiplier_0;
    y_rev_0 *= SENSITIVITY_MULTIPLIER * dynamic_multiplier_0;
    if(ut_config.inv_0) x_rev_0 = -1.0f * x_rev_0;

    uint8_t cur_mode = ut_config.pd_mode_0;
    if(force_cursoring) cur_mode = CURSOR_MODE;
    else if(force_scrolling) cur_mode = SCROLL_MODE;
    else if(force_key_input) cur_mode = KEY_INPUT;
    else if(force_gaming) cur_mode = GAME_MODE;

    if(cur_mode == SCROLL_MODE){
        if (fabsf(x_rev_0) > fabsf(y_rev_0) * 1.2f) y_rev_0 = 0; else x_rev_0 = 0;
        if(!ut_config.inv_sc) { x_rev_0 *= -1.0f; y_rev_0 *= -1.0f; }
        h_rev_0 = x_rev_0; v_rev_0 = y_rev_0; x_rev_0 = y_rev_0 = 0;
    } else if(cur_mode == KEY_INPUT || cur_mode == GAME_MODE) {
        if (cur_mode == KEY_INPUT) {
            if (timer_elapsed(key_timer_0) > TIMEOUT_KEY) {
                if(x_rev_0 > KEY_OFFSET) tap_code16(keymap_key_to_keycode(layer_switch_get_layer(key_up_0), key_right_0));
                else if(x_rev_0 < -KEY_OFFSET) tap_code16(keymap_key_to_keycode(layer_switch_get_layer(key_up_0), key_left_0));
                if(y_rev_0 > KEY_OFFSET) tap_code16(keymap_key_to_keycode(layer_switch_get_layer(key_up_0), key_down_0));
                else if(y_rev_0 < -KEY_OFFSET) tap_code16(keymap_key_to_keycode(layer_switch_get_layer(key_up_0), key_up_0));
                key_timer_0 = timer_read();
            }
        }
        x_rev_0 = y_rev_0 = 0;
    }

    /* SIDE1 (Trackball) */
    pmw33xx_report_t report = pmw33xx_read_burst(1);
    if (abs(report.delta_x) <= TRACKBALL_DEADZONE) report.delta_x = 0;
    if (abs(report.delta_y) <= TRACKBALL_DEADZONE) report.delta_y = 0;
    rad = (float)ut_config.angle_1 * 12.0f * (M_PI / 180.0f) * -1.0f;
    float x_rev_1_raw = + report.delta_x * cosf(rad) - report.delta_y * sinf(rad);
    float y_rev_1_raw = + report.delta_x * sinf(rad) + report.delta_y * cosf(rad);

    float smoothed_x_1 = prev_x_1 * SMOOTHING_FACTOR + x_rev_1_raw * (1.0f - SMOOTHING_FACTOR);
    float smoothed_y_1 = prev_y_1 * SMOOTHING_FACTOR + y_rev_1_raw * (1.0f - SMOOTHING_FACTOR);
    if (fabsf(smoothed_x_1) < SMOOTHING_CUTOFF) { smoothed_x_1 = 0; prev_x_1 = 0; } else { prev_x_1 = smoothed_x_1; }
    if (fabsf(smoothed_y_1) < SMOOTHING_CUTOFF) { smoothed_y_1 = 0; prev_y_1 = 0; } else { prev_y_1 = smoothed_y_1; }

    float dynamic_multiplier_1 = fminf(fmaxf(1.0f + sqrtf(smoothed_x_1*smoothed_x_1 + smoothed_y_1*smoothed_y_1) / 10.0f, 0.5f), 3.0f);
    x_rev_1 = x_rev_1_raw * SENSITIVITY_MULTIPLIER * dynamic_multiplier_1;
    y_rev_1 = y_rev_1_raw * SENSITIVITY_MULTIPLIER * dynamic_multiplier_1;
    if(ut_config.inv_1) x_rev_1 = -1.0f * x_rev_1;

    uint8_t cur_mode_1 = ut_config.pd_mode_1;
    if(force_cursoring) cur_mode_1 = CURSOR_MODE;
    else if(force_scrolling) cur_mode_1 = SCROLL_MODE;
    else if(force_key_input) cur_mode_1 = KEY_INPUT;
    else if(force_gaming) cur_mode_1 = GAME_MODE;

    if(cur_mode_1 == SCROLL_MODE){
        if (fabsf(x_rev_1) > fabsf(y_rev_1) * 1.2f) y_rev_1 = 0; else x_rev_1 = 0;
        if(!ut_config.inv_sc) { x_rev_1 *= -1.0f; y_rev_1 *= -1.0f; }
        h_rev_1 = x_rev_1; v_rev_1 = y_rev_1; x_rev_1 = y_rev_1 = 0;
    } else if(cur_mode_1 == KEY_INPUT || cur_mode_1 == GAME_MODE) {
        if (cur_mode_1 == KEY_INPUT) {
            if (timer_elapsed(key_timer_1) > TIMEOUT_KEY) {
                if(x_rev_1 > KEY_OFFSET) tap_code16(keymap_key_to_keycode(layer_switch_get_layer(key_up_1), key_right_1));
                else if(x_rev_1 < -KEY_OFFSET) tap_code16(keymap_key_to_keycode(layer_switch_get_layer(key_up_1), key_left_1));
                if(y_rev_1 > KEY_OFFSET) tap_code16(keymap_key_to_keycode(layer_switch_get_layer(key_up_1), key_down_1));
                else if(y_rev_1 < -KEY_OFFSET) tap_code16(keymap_key_to_keycode(layer_switch_get_layer(key_up_1), key_up_1));
                key_timer_1 = timer_read();
            }
        }
        x_rev_1 = y_rev_1 = 0;
    }

    /* JOYSTICK */
    if(joystick_attached != 2){
        float amp_temp;
        bool inv_js;
        if(joystick_attached == 0){
            cur_mode = force_gaming ? GAME_MODE : (force_cursoring ? CURSOR_MODE : (force_scrolling ? SCROLL_MODE : (force_key_input ? KEY_INPUT : ut_config.pd_mode_0)));
            rad = (float)ut_config.angle_0 * 12.0f * (M_PI / 180.0f) * -1.0f;
            amp_temp = 16.0f + (float)ut_config.spd_0 * 3.0f;
            inv_js = ut_config.inv_0;
        }else{
            cur_mode = force_gaming ? GAME_MODE : (force_cursoring ? CURSOR_MODE : (force_scrolling ? SCROLL_MODE : (force_key_input ? KEY_INPUT : ut_config.pd_mode_1)));
            rad = (float)ut_config.angle_1 * 12.0f * (M_PI / 180.0f) * -1.0f;
            amp_temp = 16.0f + (float)ut_config.spd_1 * 3.0f;
            inv_js = ut_config.inv_1;
        }

        if(slow_mode) amp_temp = AMP_SLOW;
        amp_temp /= (cur_mode == CURSOR_MODE) ? 20.0f : ((cur_mode == SCROLL_MODE) ? 30.0f : 10.0f);

        int16_t gp29_val = analogReadPin(GP29);
        int16_t gp28_val = analogReadPin(GP28);
        int16_t temp_x_val = gp29_val - gp29_newt;
        int16_t temp_y_val = gp28_val - gp28_newt;

        // ドリフト対策：デッドゾーン時は蓄積も殺す
        if (abs(temp_x_val) < joystick_offset_min) { temp_x_val = 0; x_accumulator = 0; h_accumulator = 0; }
        if (abs(temp_y_val) < joystick_offset_min) { temp_y_val = 0; y_accumulator = 0; v_accumulator = 0; }

        if(gp29_val > gp29_max) gp29_max = gp29_val; else if(gp29_val < gp29_min) gp29_min = gp29_val;
        if(gp28_val > gp28_max) gp28_max = gp28_val; else if(gp28_val < gp28_min) gp28_min = gp28_val;

        if (cur_mode == GAME_MODE) {
            float x_val_gp = 0.0f, y_val_gp = 0.0f;
            // X軸正規化 (ガード付き)
            if (abs(temp_x_val) >= joystick_offset_min) {
                float span = (temp_x_val > 0) ? (float)(gp29_max - joystick_offset_max - (gp29_newt + joystick_offset_min))
                                             : (float)((gp29_newt - joystick_offset_min) - (gp29_min + joystick_offset_max));
                if (span > JS_MIN_SPAN) x_val_gp = (float)temp_x_val / span * 511.0f;
            }
            // Y軸正規化 (ガード付き)
            if (abs(temp_y_val) >= joystick_offset_min) {
                float span = (temp_y_val > 0) ? (float)(gp28_max - joystick_offset_max - (gp28_newt + joystick_offset_min))
                                             : (float)((gp28_newt - joystick_offset_min) - (gp28_min + joystick_offset_max));
                if (span > JS_MIN_SPAN) y_val_gp = (float)temp_y_val / span * 511.0f;
            }

            float x_rev_gp = + x_val_gp * cosf(rad) - y_val_gp * sinf(rad);
            float y_rev_gp = + x_val_gp * sinf(rad) + y_val_gp * cosf(rad);
            if (inv_js) x_rev_gp *= -1.0f;

            // 最終出力の極小カットと適用
            if (fabsf(x_rev_gp) < 1.0f) x_rev_gp = 0;
            if (fabsf(y_rev_gp) < 1.0f) y_rev_gp = 0;
            joystick_set_axis(0, (int16_t)x_rev_gp);
            joystick_set_axis(1, (int16_t)y_rev_gp);

            // GAME_MODE時はマウス移動はさせない
            x_rev_js = y_rev_js = 0;
        } else {
            // 通常モードのJS処理
            float x_val_js = ((float)temp_x_val / JOYSTICK_DIVISOR) * amp_temp;
            float y_val_js = ((float)temp_y_val / JOYSTICK_DIVISOR) * amp_temp;
            x_rev_js = + x_val_js * cosf(rad) - y_val_js * sinf(rad);
            y_rev_js = + x_val_js * sinf(rad) + y_val_js * cosf(rad);
            if (fabsf(x_rev_js) < 0.1f) x_rev_js = 0;
            if (fabsf(y_rev_js) < 0.1f) y_rev_js = 0;
            if (inv_js) x_rev_js *= -1.0f;

            if (cur_mode == SCROLL_MODE) {
                if (fabsf(x_rev_js) > fabsf(y_rev_js) * 1.2f) y_rev_js = 0; else x_rev_js = 0;
                if (!ut_config.inv_sc) { x_rev_js *= -1.0f; y_rev_js *= -1.0f; }
                h_rev_js = x_rev_js; v_rev_js = y_rev_js; x_rev_js = y_rev_js = 0;
            } else if (cur_mode == KEY_INPUT) {
                if (timer_elapsed(key_timer_0) > TIMEOUT_KEY) {
                    keypos_t ku = (joystick_attached == 0) ? key_up_0 : key_up_1;
                    keypos_t kd = (joystick_attached == 0) ? key_down_0 : key_down_1;
                    keypos_t kl = (joystick_attached == 0) ? key_left_0 : key_left_1;
                    keypos_t kr = (joystick_attached == 0) ? key_right_0 : key_right_1;
                    if (x_rev_js > KEY_OFFSET) tap_code16(keymap_key_to_keycode(layer_switch_get_layer(ku), kr));
                    else if (x_rev_js < -KEY_OFFSET) tap_code16(keymap_key_to_keycode(layer_switch_get_layer(ku), kl));
                    if (y_rev_js > KEY_OFFSET) tap_code16(keymap_key_to_keycode(layer_switch_get_layer(ku), kd));
                    else if (y_rev_js < -KEY_OFFSET) tap_code16(keymap_key_to_keycode(layer_switch_get_layer(ku), ku));
                    key_timer_0 = timer_read();
                }
                x_rev_js = y_rev_js = 0;
            }
        }
    }

    /* 最終合算と端数処理 */
    float scrl_mult = 0.5f + (float)ut_config.scrl_spd * 0.25f;
    x_accumulator += (x_rev_0 + x_rev_1) * SMOOTHING_FACTOR + x_rev_js;
    y_accumulator += (y_rev_0 + y_rev_1) * SMOOTHING_FACTOR + y_rev_js;
    h_accumulator += (((h_rev_0 + h_rev_1) * SMOOTHING_FACTOR + h_rev_js) * scrl_mult) / SCROLL_DIVISOR;
    v_accumulator += (((v_rev_0 + v_rev_1) * SMOOTHING_FACTOR + v_rev_js) * scrl_mult) / SCROLL_DIVISOR;

    // 1ピクセル以上の移動がある場合のみ出力
    if (fabsf(x_accumulator) >= 1.0f || fabsf(y_accumulator) >= 1.0f) {
        mouse_report.x = (int8_t)constrain_hid(x_accumulator);
        mouse_report.y = (int8_t)constrain_hid(y_accumulator);
        x_accumulator -= mouse_report.x;
        y_accumulator -= mouse_report.y;
    } else {
        // 微小すぎる蓄積は少しずつ減衰させて「いつの間にか1ピクセル動く」のを防ぐ
        x_accumulator *= 0.8f;
        y_accumulator *= 0.8f;
        mouse_report.x = 0;
        mouse_report.y = 0;
    }

    // スクロール報告
    mouse_report.h = (int8_t)constrain_hid(h_accumulator);
    mouse_report.v = (int8_t)constrain_hid(v_accumulator);
    h_accumulator -= mouse_report.h;
    v_accumulator -= mouse_report.v;

    return pointing_device_task_user(mouse_report);
}

/* OLED */
// 初期化
oled_rotation_t oled_init_kb(oled_rotation_t rotation) {
    // 追加OLED初期化
    oled_init_addedoled();

    return oled_init_user(rotation);
}
// 実タスク
bool oled_task_kb(void) {
    // 追加OLEDタスク
    oled_task_addedoled();

    return oled_task_user();
}


/* 諸関数 */
// モードチェンジ時端数削除
void clear_keyinput(void){
    unregister_code(keycode_up_0);
    unregister_code(keycode_down_0);
    unregister_code(keycode_left_0);
    unregister_code(keycode_right_0);
    unregister_code(keycode_up_1);
    unregister_code(keycode_down_1);
    unregister_code(keycode_left_1);
    unregister_code(keycode_right_1);
    v_accumulator = 0.0;
    h_accumulator = 0.0;
    prev_x_0 = 0.0;
    prev_y_0 = 0.0;
    prev_x_1 = 0.0;
    prev_y_1 = 0.0;
    x_accumulator = 0.0;
    y_accumulator = 0.0;
}
/* インターフェース */
// ジョイスティックの初期化
void reset_joystick(void){
    gp29_newt = analogReadPin(GP29);
    gp28_newt = analogReadPin(GP28);
    gp29_min = gp29_newt;
    gp28_min = gp28_newt;
    gp29_max = gp29_newt;
    gp28_max = gp28_newt;
    joystick_offset_min = JOYSTICK_OFFSET_MIN_DEFAULT;
    joystick_offset_max = JOYSTICK_OFFSET_MAX_DEFAULT;
}
// ジョイスティックの有無
uint8_t get_joystick_attached(void){ return joystick_attached; }
uint16_t get_joystick_offset_min(void){
    return joystick_offset_min;
}
uint16_t get_joystick_offset_max(void){
    return joystick_offset_max;
}
void set_joystick_offset_min(uint16_t min){
    joystick_offset_min = min;
}
void set_joystick_offset_max(uint16_t max){
    joystick_offset_max = max;
}
// モード変更
void cycle_mode(bool side){
    if(side){
        if(joystick_attached < 2 && (ut_config.js_side == side)){
            ut_config.pd_mode_1 = (ut_config.pd_mode_1 + 1) % 4;
        }else{
            ut_config.pd_mode_1 = (ut_config.pd_mode_1 + 1) % 3;
        }
        eeconfig_update_kb(ut_config.raw);
        clear_keyinput();
    }else{
        if(joystick_attached < 2 && (ut_config.js_side == side)){
            ut_config.pd_mode_0 = (ut_config.pd_mode_0 + 1) % 4;
        }else{
            ut_config.pd_mode_0 = (ut_config.pd_mode_0 + 1) % 3;
        }
        eeconfig_update_kb(ut_config.raw);
        clear_keyinput();
    }
}
// 一時的モード変更
void is_scroll_mode(bool is_force_scrolling){
    force_scrolling = is_force_scrolling;
    clear_keyinput();
}
void is_cursor_mode(bool is_force_cursoring){
    force_cursoring = is_force_cursoring;
    clear_keyinput();
}
void is_key_mode(bool is_force_key_input){
    force_key_input = is_force_key_input;
    clear_keyinput();
}
void is_slow_mode(bool is_slow_mode){
    slow_mode = is_slow_mode;
    if(is_slow_mode){
        pmw33xx_set_cpi(0, CPI_SLOW);
        pmw33xx_set_cpi(1, CPI_SLOW);
    }else{
        pmw33xx_set_cpi(0, 1000 + ut_config.spd_0 * 250);
        pmw33xx_set_cpi(1, 1000 + ut_config.spd_1 * 250);
    }
    clear_keyinput();
}
void is_game_mode(bool is_force_gaming){
    force_gaming = is_force_gaming;
    clear_keyinput();
}
bool get_dpad_exclusion(void){
    return dpad_exclusion;
}
void toggle_dpad_exclusion(void){
    dpad_exclusion = !dpad_exclusion;
}
bool get_rgblayers(void){
    return rgblayers;
}

void toggle_rgblayers(void){
    rgblayers = !rgblayers;
}
