#pragma once
#include <Adafruit_NeoPixel.h>
#include "config.h"

// Onboard WS2812 of the Super Mini as a status indicator.
//   red blinking          AS5600 missing
//   red fast blinking     button 3 held -> Zigbee reset pending
//   orange breathing      Zigbee searching for a network / stack still starting
//   green                 Zigbee connected
//   short blink           rotation or button event (color of the active value)
class StatusLed {
public:
  enum class State : uint8_t { Searching, Connected, SensorError, ResetPending };

  void begin() {
    _px.begin();
    _px.setBrightness(255);
    _px.clear();
    _px.show();
  }

  void setState(State s) { _state = s; }

  void blip(uint8_t r, uint8_t g, uint8_t b) {
    _br = r; _bg = g; _bb = b;
    _blipUntil = millis() + STATUS_BLIP_MS;
  }

  void update() {
    const uint32_t now = millis();
    if (now - _lastFrame < 20) return;
    _lastFrame = now;

    uint8_t r = 0, g = 0, b = 0, bright = STATUS_BRIGHT;
    if (now < _blipUntil) {
      r = _br; g = _bg; b = _bb;
    } else {
      switch (_state) {
        case State::SensorError:
          r = 255; bright = (now / 400) % 2 ? STATUS_BRIGHT : 0; break;
        case State::ResetPending:
          r = 255; bright = (now / 100) % 2 ? STATUS_BRIGHT : 0; break;
        case State::Connected:
          g = 255; break;
        case State::Searching: {
          // soft "breathing", period 2 s
          float ph = (now % 2000) / 2000.0f;
          float tri = ph < 0.5f ? ph * 2 : (1 - ph) * 2;
          r = 255; g = 90;
          bright = (uint8_t)(STATUS_BRIGHT * (0.1f + 0.9f * tri));
          break;
        }
      }
    }
    _px.setPixelColor(0, _px.Color((r * bright) >> 8, (g * bright) >> 8, (b * bright) >> 8));
    _px.show();
  }

private:
  Adafruit_NeoPixel _px{1, PIN_STATUS_LED, NEO_GRB + NEO_KHZ800};
  State _state = State::Searching;
  uint8_t _br = 0, _bg = 0, _bb = 0;
  uint32_t _blipUntil = 0, _lastFrame = 0;
};
