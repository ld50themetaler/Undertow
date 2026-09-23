// Copyright 2021 Hayashi (@w_vwbw)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "lib/add_keycodes.h"

// キーマップ
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        // Side0 (Right)
        MS_BTN3, MS_BTN2,
        KC_D, KC_A, KC_B, KC_C,
        XXXXXXX,
        // Side1 (Left)
        MO(1), MS_BTN1,
        MS_BTN3, MS_BTN1, MOD_SCRL, MS_BTN2,
        XXXXXXX
    ),
    [1] = LAYOUT(
        // Side0 (Right)
        MO(3), MO(4),
        _______, _______, _______, _______,
        _______,
        // Side1 (Left)
        _______, MO(2),
        ANG_D_0, SPD_D_0, ANG_I_0, SPD_I_0,
        _______
    ),
    [2] = LAYOUT(
        // Side0 (Right)
        _______, _______,
        _______, _______, _______, _______,
        _______,
        // Side1 (Left)
        _______, _______,
        OFFSET_MIN_I, OLED_MOD, OFFSET_MIN_D, AUTO_MOUSE,
        _______
    ),
    [3] = LAYOUT(
        // Side0 (Right)
        _______, _______,
        _______, _______, _______, _______,
        _______,
        // Side1 (Left)
        _______, _______,
        SCRL_SPD_D, _______, SCRL_SPD_I, _______,
        _______
    ),
    [4] = LAYOUT(
        // Side0 (Right)
        _______, MO(8),
        _______, _______, _______, _______,
        _______,
        // Side1 (Left)
        SPD_I_1, SPD_D_1,
        _______, _______, _______, _______,
        _______
    ),
    [5] = LAYOUT(
        // Side0 (Right)
        SPD_I_0, SPD_D_0,
        _______, _______, _______, _______,
        _______,
        // Side1 (Left)
        _______, MO(9),
        _______, _______, _______, _______,
        _______
    ),
    [6] = LAYOUT(
        // Side0 (Right)
        MO(8), _______,
        _______, _______, _______, _______,
        _______,
        // Side1 (Left)
        ANG_I_1, ANG_D_1,
        _______, _______, _______, _______,
        _______
    ),
    [7] = LAYOUT(
        // Side0 (Right)
        ANG_I_0, ANG_D_0,
        _______, _______, _______, _______,
        _______,
        // Side1 (Left)
        MO(9), _______,
        _______, _______, _______, _______,
        _______
    ),
    [8] = LAYOUT(
        // Side0 (Right)
        _______, _______,
        _______, _______, _______, _______,
        _______,
        // Side1 (Left)
        CHMOD_1, INV_1,
        _______, _______, _______, _______,
        _______
    ),
    [9] = LAYOUT(
        // Side0 (Right)
        CHMOD_0, INV_0,
        _______, _______, _______, _______,
        _______,
        // Side1 (Left)
        _______, _______,
        _______, _______, _______, _______,
        _______
    )
};

const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][2] = {
    [0] =   {
        ENCODER_CCW_CW(MS_WHLU, MS_WHLD)
    },
    [1] =   {
        ENCODER_CCW_CW(MS_WHLU, MS_WHLD)
    },
    [2] =   {
        ENCODER_CCW_CW(MS_WHLU, MS_WHLD)
    },
    [3] =   {
        ENCODER_CCW_CW(MS_WHLU, MS_WHLD)
    },
    [4] =   {
        ENCODER_CCW_CW(MS_WHLU, MS_WHLD)
    },
    [5] =   {
        ENCODER_CCW_CW(MS_WHLU, MS_WHLD)
    },
    [6] =   {
        ENCODER_CCW_CW(MS_WHLU, MS_WHLD)
    },
    [7] =   {
        ENCODER_CCW_CW(MS_WHLU, MS_WHLD)
    },
    [8] =   {
        ENCODER_CCW_CW(MS_WHLU, MS_WHLD)
    },
    [9] =   {
        ENCODER_CCW_CW(MS_WHLU, MS_WHLD)
    }
};
