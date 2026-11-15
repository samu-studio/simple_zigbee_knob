#pragma once
#include <Adafruit_NeoPixel.h>
#include "config.h"

// Display on the 12-LED ring.
//   Bar   level 0..100 % in one color (value, reset progress)
// After LED_SHOW_MS without action the display fades into the idle glow (color of the active value).
// On top of that: status (spinner, breathing, error) and short effects (flash, wipe).
class Ring {
public:
  enum class Status : uint8_t { Normal, Pairing, Pulse, Error };

  void begin() {
    _px.begin();
    _px.setBrightness(255);  // brightness is scaled here
    _px.clear();
    _px.show();
  }

  // Overall brightness 0..100 % (100 % = LED_MAX_BRIGHT, 0 = ring off)
  void setBrightness(uint8_t pct) {
    pct = min<uint8_t>(pct, 100);
    _maxB = LED_MAX_BRIGHT * pct / 100;
    _idleB = pct ? max<uint8_t>(1, LED_IDLE_BRIGHT * pct / 100) : 0;
  }

  void setIdleColor(uint8_t r, uint8_t g, uint8_t b) { _ir = r; _ig = g; _ib = b; }
  void setStatus(Status s) { _status = s; }

  // Update the display; wake = wake the display up (otherwise only track the value in the background)
  void showBar(float pct, uint8_t r, uint8_t g, uint8_t b, bool wake = true) {
    _level = constrain(pct, 0.0f, 100.0f);
    _r = r; _g = g; _b = b;
    if (wake) touch();
  }
  void touch() { _activeUntil = millis() + LED_SHOW_MS; }

  // count flashes of ms each (double click = 2)
  void flash(uint8_t r, uint8_t g, uint8_t b, uint16_t ms = 250, uint8_t count = 1) {
    _fr = r; _fg = g; _fb = b;
    _flashMs = ms; _flashCount = count; _flashStart = millis();
  }

  // One wipe around the ring (value selected)
  void wipe(uint8_t r, uint8_t g, uint8_t b) {
    _wr = r; _wg = g; _wb = b;
    _wipeStart = millis(); _wipeOn = true;
    touch();
  }

  void update() {
    const uint32_t now = millis();
    if (now - _lastFrame < 16) return;  // ~60 fps
    _lastFrame = now;

    // Fade between "active" and "idle"
    const float target = now < _activeUntil ? 1.0f : 0.0f;
    _fade += (target - _fade) * (target > _fade ? 0.35f : 0.06f);

    if (flashing(now)) {
      fillAll(_fr, _fg, _fb, _maxB);
    } else if (_status == Status::Pairing) {
      drawSpinner(now);
    } else if (_status == Status::Error) {
      fillAll(255, 0, 0, (now / 300) % 2 ? _maxB : 0);
    } else {
      drawBar();
      if (_status == Status::Pulse) drawPulse(now);
      drawWipe(now);
    }
    _px.show();
  }

private:
  uint16_t idx(uint8_t i) const {
    uint8_t p = LED_REVERSE ? (LED_COUNT - i) % LED_COUNT : i;
    return (p + LED_OFFSET) % LED_COUNT;
  }

  void setPx(uint8_t i, uint8_t r, uint8_t g, uint8_t b, float bright) {
    const uint16_t br = (uint16_t)constrain(bright, 0.0f, 255.0f);
    _px.setPixelColor(idx(i), _px.Color((r * br) >> 8, (g * br) >> 8, (b * br) >> 8));
  }

  void fillAll(uint8_t r, uint8_t g, uint8_t b, uint8_t bright) {
    for (uint8_t i = 0; i < LED_COUNT; i++) setPx(i, r, g, b, bright);
  }

  bool flashing(uint32_t now) const {
    if (!_flashCount) return false;
    const uint32_t t = now - _flashStart;
    const uint32_t period = _flashMs + (_flashCount > 1 ? 90 : 0);  // pause between several flashes
    if (t >= period * _flashCount) return false;
    return (t % period) < _flashMs;
  }

  // Mix of the active display (color c, brightness a) and the idle glow
  void blendPx(uint8_t i, uint8_t r, uint8_t g, uint8_t b, float a) {
    const float f = _fade;
    const float ar = r * a, ag = g * a, ab = b * a;           // active, already scaled
    const float ir = _ir * _idleB, ig = _ig * _idleB, ib = _ib * _idleB;
    const float rr = ir + (ar - ir) * f, gg = ig + (ag - ig) * f, bb = ib + (ab - ib) * f;
    _px.setPixelColor(idx(i), _px.Color(scale(rr), scale(gg), scale(bb)));
  }
  uint8_t scale(float v) const { return (uint8_t)constrain(v / 255.0f, 0.0f, 255.0f); }

  void drawBar() {
    const float lit = _level / 100.0f * LED_COUNT;  // e.g. 7.4 LEDs
    const float base = _idleB * 0.4f;
    for (uint8_t i = 0; i < LED_COUNT; i++) {
      const float f = constrain(lit - i, 0.0f, 1.0f);  // partial brightness of the last LED
      blendPx(i, _r, _g, _b, base + f * (_maxB - base));
    }
  }

  void drawSpinner(uint32_t now) {
    const float pos = (now % 1200) / 1200.0f * LED_COUNT;
    for (uint8_t i = 0; i < LED_COUNT; i++) {
      float d = fmodf(pos - i + LED_COUNT, LED_COUNT);  // distance behind the head
      float b = d < 4 ? (1.0f - d / 4) * _maxB : 0;
      setPx(i, _ir, _ig, _ib, b);
    }
  }

  // Slow breathing in the idle color over the display (Zigbee connection lost)
  void drawPulse(uint32_t now) {
    if (_fade > 0.5f) return;  // normal display while in use
    const float ph = (now % 3000) / 3000.0f;
    const float tri = ph < 0.5f ? ph * 2 : (1 - ph) * 2;
    fillAll(_ir, _ig, _ib, (uint8_t)(_idleB + tri * (_maxB * 0.5f - _idleB)));
  }

  void drawWipe(uint32_t now) {
    if (!_wipeOn) return;
    constexpr uint32_t WIPE_MS = 350;
    const uint32_t t = now - _wipeStart;
    if (t >= WIPE_MS + 150) { _wipeOn = false; return; }
    const float head = min<float>(t, WIPE_MS) / WIPE_MS * LED_COUNT;
    const float out = t > WIPE_MS ? 1.0f - (t - WIPE_MS) / 150.0f : 1.0f;  // fade out at the end
    for (uint8_t i = 0; i < LED_COUNT; i++) {
      const float f = constrain(head - i, 0.0f, 1.0f) * out;
      if (f > 0.02f) setPx(i, _wr, _wg, _wb, f * _maxB);
    }
  }

  Adafruit_NeoPixel _px{LED_COUNT, PIN_LED_RING, NEO_GRB + NEO_KHZ800};
  Status _status = Status::Normal;
  bool _wipeOn = false;
  uint8_t _r = 0, _g = 120, _b = 255;
  uint8_t _ir = 0, _ig = 110, _ib = 255;
  uint8_t _fr = 0, _fg = 0, _fb = 0, _flashCount = 0;
  uint8_t _wr = 0, _wg = 0, _wb = 0;
  uint16_t _flashMs = 0;
  uint8_t _maxB = LED_MAX_BRIGHT, _idleB = LED_IDLE_BRIGHT;
  float _level = 0, _fade = 0;
  uint32_t _activeUntil = 0, _flashStart = 0, _wipeStart = 0, _lastFrame = 0;
};
