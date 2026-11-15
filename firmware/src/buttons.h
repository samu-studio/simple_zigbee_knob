#pragma once
#include <Arduino.h>
#include "config.h"

enum class BtnEvent : uint8_t { None, Click, Double, Long, VeryLong };

// Debounced button with click / double click / long / very long press.
// Very long fires while holding, all others on release.
// Double click only with setDouble(true): a click then waits up to BTN_DOUBLE_MS for the
// second one, otherwise it fires immediately. If the second press turns long, the first counts as a click.
class Button {
public:
  void begin(uint8_t pin) {
    _pin = pin;
    pinMode(pin, INPUT_PULLUP);
  }

  void setDouble(bool on) {
    _double = on;
  }

  BtnEvent poll() {
    const uint32_t now = millis();
    const bool raw = digitalRead(_pin) == LOW;
    if (raw != _rawLast) { _rawLast = raw; _changed = now; }

    // Pending click: no second press within the window -> single click
    if (_pending && !_pressed && now - _releasedAt >= BTN_DOUBLE_MS) {
      _pending = false;
      return BtnEvent::Click;
    }
    if (now - _changed < BTN_DEBOUNCE_MS) return BtnEvent::None;

    if (raw && !_pressed) {            // pressed
      _pressed = true; _pressedAt = now; _veryLongFired = false;
    } else if (!raw && _pressed) {     // released
      _pressed = false;
      _releasedAt = now;
      if (_veryLongFired) return BtnEvent::None;
      if (now - _pressedAt >= BTN_LONG_MS) return BtnEvent::Long;
      if (_pending) { _pending = false; return BtnEvent::Double; }
      if (_double) { _pending = true; return BtnEvent::None; }
      return BtnEvent::Click;
    } else if (raw && _pressed) {
      // Second press turns long -> the first press was a single click
      if (_pending && now - _pressedAt >= BTN_LONG_MS) { _pending = false; return BtnEvent::Click; }
      if (!_veryLongFired && now - _pressedAt >= BTN_RESET_MS) {
        _veryLongFired = true;
        return BtnEvent::VeryLong;
      }
    }
    return BtnEvent::None;
  }

  bool held() const { return _pressed; }
  uint32_t heldMs() const { return _pressed ? millis() - _pressedAt : 0; }

private:
  uint8_t _pin = 0;
  bool _rawLast = false, _pressed = false, _veryLongFired = false;
  bool _double = false, _pending = false;
  uint32_t _changed = 0, _pressedAt = 0, _releasedAt = 0;
};
