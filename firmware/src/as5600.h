#pragma once
#include <Arduino.h>
#include <Wire.h>

// Minimal AS5600 driver + conversion into virtual detents.
class AS5600Knob {
public:
  bool begin(TwoWire &wire, int stepsPerRev, float hysteresis, bool invert) {
    _wire = &wire;
    _stepSize = 4096.0f / stepsPerRev;
    _hyst = hysteresis;
    _invert = invert;
    int raw = readRaw();
    if (raw < 0) return false;
    _last = raw;
    return true;
  }

  // Status register: MD = magnet detected, ML = too weak, MH = too strong
  bool magnetDetected() { int s = read8(0x0B); return s >= 0 && (s & 0x20); }
  bool magnetTooWeak()  { int s = read8(0x0B); return s >= 0 && (s & 0x10); }
  bool magnetTooStrong(){ int s = read8(0x0B); return s >= 0 && (s & 0x08); }

  // 12-bit angle (0..4095) or -1 on I2C error
  int readRaw() {
    int hi = read8(0x0C);
    int lo = read8(0x0D);
    if (hi < 0 || lo < 0) return -1;
    return ((hi & 0x0F) << 8) | lo;
  }

  // Returns the detents counted since the last call (signed).
  int poll() {
    int raw = readRaw();
    if (raw < 0) return 0;
    int d = raw - _last;
    if (d > 2048) d -= 4096;   // wrap-around 4095 -> 0
    if (d < -2048) d += 4096;
    _last = raw;
    _accum += _invert ? -d : d;

    int steps = 0;
    const float thr = _stepSize * _hyst;
    while (_accum >= thr)  { _accum -= _stepSize; steps++; }
    while (_accum <= -thr) { _accum += _stepSize; steps--; }
    return steps;
  }

private:
  int read8(uint8_t reg) {
    _wire->beginTransmission(0x36);
    _wire->write(reg);
    if (_wire->endTransmission(false) != 0) return -1;
    if (_wire->requestFrom((uint8_t)0x36, (uint8_t)1) != 1) return -1;
    return _wire->read();
  }

  TwoWire *_wire = nullptr;
  float _stepSize = 4096.0f / 24;
  float _hyst = 0.6f;
  bool _invert = false;
  int _last = 0;
  float _accum = 0;
};
