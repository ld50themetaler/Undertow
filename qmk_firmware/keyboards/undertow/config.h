// Copyright 2021 Hayashi (@w_vwbw)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#define KEY_INPUT 0
#define CURSOR_MODE 1
#define SCROLL_MODE 2
#define GAME_MODE 3

//////////////////////
/*    ユーザー設定    */
//////////////////////

// 入力モードデフォルト
#define TB_SIDE0_DEFAULT CURSOR_MODE
#define TB_SIDE1_DEFAULT CURSOR_MODE
#define JS_SIDE_DEFAULT 0 // 0: SIDE0, 1: SIDE1, 2: none

// CPI = 1000 + spd * 250 / AMP = 16.0 + (double)spd * 3.0
#define SPD_OPTION_MAX    7     // 最大値
#define SPD_DEFAULT_SIDE0 2
#define SPD_DEFAULT_SIDE1 2
// 角度 = angle * 12
#define ANGLE_OPTION_MAX    29  // 最大値
#define ANGLE_DEFAULT_SIDE0 8
#define ANGLE_DEFAULT_SIDE1 7
#define SIDE0_INVERT true     // X軸の反転
#define SIDE1_INVERT true
#define SCROLL_INVERT false     // スクロールの反転

// 移動量が小さい時の調節
#define SENSITIVITY_MULTIPLIER 1.1 // 一時的倍率
#define SENSITIVITY_DIVISOR 0.5    // 最終的な感度調整
#define SMOOTHING_FACTOR 0.7 // 前の動きの影響度

// スローモード時カーソル速度
#define CPI_SLOW 300
#define AMP_SLOW 4.0
// オートマウスの設定
#define AUTO_MOUSE_DEFAULT true
#define AUTO_MOUSE_DEFAULT_LAYER 1
#define AUTO_MOUSE_TIME 750
#define AUTO_MOUSE_DELAY 750
#define AUTO_MOUSE_DEBOUNCE 40
#define AUTO_MOUSE_THRESHOLD 80
// OLEDモードデフォルト
#define OLED_DEFAULT false //  true: show layer, false: show stats

#define NO_JOYSTICK_VAL 100         // JSの有無判定閾値
#define JOYSTICK_OFFSET_MIN_DEFAULT 70 // ジョイスティックの小さい値を無視する範囲 最大200
#define JOYSTICK_OFFSET_MAX_DEFAULT 0 // ジョイスティックの大きい値を無視する範囲 最大200
#define SCROLL_DIVISOR 50.0        // スクロール用数値調整
#define JOYSTICK_DIVISOR 40.0       // ジョイスティック用調整用
#define INTERRUPT_TIME 600          // OLED切り替え時間
#define KEY_OFFSET 5                // キー入力閾値
#define TIMEOUT_KEY 50              // キー入力間隔
#define TERM_TEMP 100               // 一時的モード変更タップ判定ms

// RGBレイヤーデフォルト
#define RGB_LAYER_DEFAULT true

// 斜め入力防止のデフォルト
#define DPAD_EX_DEFAULT true









/* ハードウェア設定（変更不要） */
// SPI
#define PMW33XX_CS_PINS  {GP1, GP5}
#define PMW33XX_CS_PINS_RIGHT  {GP1, GP5}
#define SPI_SCK_PIN GP2
#define SPI_MISO_PIN GP4
#define SPI_MOSI_PIN GP3
// I2C
#define I2C_DRIVER I2CD0
#define I2C1_SCL_PIN        GP9
#define I2C1_SDA_PIN        GP8
#define OLED_FONT_H "./lib/glcdfont.c"
// AUTO MOUSE
#define POINTING_DEVICE_AUTO_MOUSE_ENABLE
#define POINTING_DEVICE__COMBINED
#define POINTING_DEVICE_DRIVER_MANUAL_REPORT

// RGBLIGHT LAYERS
#define RGBLIGHT_LAYERS_RETAIN_VAL
// JOYSTICK
#define JOYSTICK_AXIS_COUNT 2
#define JOYSTICK_AXIS_RESOLUTION 10
