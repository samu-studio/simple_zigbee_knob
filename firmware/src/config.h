#pragma once
#include <Arduino.h>

// ---------------------------------------------------------------------------
// Pins – ESP32-C6 Super Mini
// Avoids the strapping pins (4, 5, 8, 9, 15) and USB (12, 13).
// ---------------------------------------------------------------------------
constexpr uint8_t PIN_I2C_SDA  = 6;   // AS5600 SDA
constexpr uint8_t PIN_I2C_SCL  = 7;   // AS5600 SCL
constexpr uint8_t PIN_LED_RING = 18;  // NeoPixel DIN (through 330 ohm)
constexpr uint8_t PIN_BTN[3]   = {0, 1, 20}; // buttons to GND, internal pull-up
// Onboard WS2812 of the Super Mini. GPIO8 is a strapping pin, but the LED is already
// on the board – it is only used as an output after boot.
constexpr uint8_t PIN_STATUS_LED = 8;

// ---------------------------------------------------------------------------
// Rotary encoder
// ---------------------------------------------------------------------------
constexpr int   STEPS_PER_REV   = 24;     // virtual detents per revolution
constexpr float STEP_HYSTERESIS = 0.6f;   // fraction of a step needed to trigger
constexpr bool  INVERT_ROTATION = false;  // reverse the direction of rotation

// ---------------------------------------------------------------------------
// LED ring
// ---------------------------------------------------------------------------
constexpr uint8_t  LED_COUNT        = 12;
constexpr uint8_t  LED_OFFSET       = 6;     // which LED is "bottom/front" (bar start); 6 = ring mounted rotated by 180°
constexpr bool     LED_REVERSE      = false; // counting direction of the ring
constexpr uint8_t  LED_MAX_BRIGHT   = 90;    // 0..255 – limits the current (12 LEDs * 60 mA!), equals 100 %
constexpr uint8_t  LED_IDLE_BRIGHT  = 8;     // idle glow
constexpr uint32_t LED_SHOW_MS      = 2500;  // how long the level stays visible after an action

// ---------------------------------------------------------------------------
// Status LED (onboard WS2812 on GPIO8)
// ---------------------------------------------------------------------------
constexpr uint8_t  STATUS_BRIGHT    = 24;    // 0..255 – the onboard LED is very bright
constexpr uint16_t STATUS_BLIP_MS   = 60;    // short blink on rotation/button events

// ---------------------------------------------------------------------------
// Buttons
// ---------------------------------------------------------------------------
constexpr uint32_t BTN_DEBOUNCE_MS  = 25;
constexpr uint32_t BTN_LONG_MS      = 600;
constexpr uint32_t BTN_DOUBLE_MS    = 250;   // how long a click waits for a second one
constexpr uint32_t BTN_RESET_MS     = 6000;  // hold button 3 this long -> Zigbee reset

// ---------------------------------------------------------------------------
// Zigbee
// ---------------------------------------------------------------------------
constexpr const char *SIMPLE_FW_VERSION = "S1.1.0";  // shown as firmware version in Z2M/HA (max. 16 chars)
constexpr uint8_t  SIMPLE_STEP      = 2;     // % per detent (24 detents = 48 % per revolution)
constexpr uint8_t  SIMPLE_EP_VALUE1 = 1;     // EP1..EP3: values 1..3, Analog In/Output 0..100 %
constexpr uint8_t  SIMPLE_EP_ACTIVE = 4;     // Analog In/Output: active value 1..3
constexpr uint8_t  SIMPLE_EP_RING   = 5;     // Analog In/Output: ring brightness 0..100 %
constexpr uint8_t  SIMPLE_EP_EVENT  = 6;     // Analog Input: button event n*10 + 1 click / 2 double click
constexpr uint32_t SIMPLE_SAVE_MS   = 2000;  // save values to flash this long after the last change
constexpr uint32_t ZB_SEND_INTERVAL = 120;   // ms – throttle reports to the coordinator
// Connection watchdog: Zigbee.connected() stays true forever after the first join,
// so the only thing that counts is whether the network answers our own packets (default responses).
constexpr uint32_t ZB_ALIVE_MS      = 60000;   // silent this long -> send a sign of life (report to the coordinator)
constexpr uint32_t ZB_LOST_MS       = 180000;  // no answer this long -> connection counts as lost
constexpr uint32_t ZB_REJOIN_MS     = 120000;  // lost this long -> restart, device rejoins
constexpr uint32_t ZB_REJOIN_MAX_MS = 3600000; // the wait doubles after every failed attempt up to this
// Ring color per value
constexpr uint8_t  SIMPLE_COLORS[3][3] = {{255, 90, 0}, {0, 200, 120}, {150, 40, 255}};  // orange, green, purple
