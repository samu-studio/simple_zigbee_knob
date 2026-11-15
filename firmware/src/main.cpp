// Simple Zigbee Knob – three values 0..100 % for Home Assistant
//
// The knob sets three percentage values. A long press on button 1/2/3 selects the value,
// rotating changes the selected one. Click and double click are sent to Home Assistant as
// events (action). The ring shows the value as a bar in the value's color
// (1 orange, 2 green, 3 purple). Values, active value and ring brightness are stored.
// No lamp bindings: Home Assistant decides what the values do.
//
// Zigbee endpoints (Z2M converter: zigbee2mqtt/magnet_knob_simple.mjs):
//   EP1..EP3 Analog In/Output – values 1..3, 0..100 % (Output: set from HA, Input: report)
//   EP4      Analog In/Output – active value 1..3
//   EP5      Analog In/Output – ring brightness 0..100 %
//   EP6      Analog Input     – event: button n*10 + 1 click / 2 double click (Z2M: action 1_click, 1_double_click ...)
//
// Button 3 > 6 s: Zigbee factory reset (the ring shows the progress in red from 1.5 s).
// Serial output for diagnostics only: HELLO knob-simple 1 | VAL <1-3> <0-100> | SEL <1-3> | EVT <code> | ZB <ok|lost|search>

#include <Arduino.h>
#include <Wire.h>
#include <Preferences.h>
#include "config.h"
#include "as5600.h"
#include "buttons.h"
#include "ring.h"
#include "status_led.h"
#include "zb_common.h"

using KnobAnalog = WithSwBuild<ZigbeeAnalog>;

AS5600Knob knob;
Button btn[3];
Ring ring;
StatusLed statusLed;
Preferences prefs;

KnobAnalog zbValue[3] = {KnobAnalog(SIMPLE_EP_VALUE1), KnobAnalog(SIMPLE_EP_VALUE1 + 1), KnobAnalog(SIMPLE_EP_VALUE1 + 2)};
KnobAnalog zbActive(SIMPLE_EP_ACTIVE);
KnobAnalog zbRing(SIMPLE_EP_RING);
ZigbeeAnalog zbEvent(SIMPLE_EP_EVENT);

bool encoderOk = false;
uint8_t value[3] = {50, 50, 50};  // 0..100 %
uint8_t active = 0;               // 0..2
uint8_t ringBright = 100;
uint32_t lastInput = 0;           // last user input (spinner only while idle)
uint32_t saveAt = 0;              // 0 = nothing to save

// Reports to Home Assistant pending
bool valueDirty[3] = {true, true, true};
bool activeDirty = true, ringDirty = true;
uint32_t lastSend = 0;

// Set by the Zigbee callbacks (stack task), picked up in loop()
volatile int16_t rxValue[3] = {-1, -1, -1};
volatile int8_t rxActive = -1;
volatile int16_t rxRing = -1;
volatile bool zbStarted = false;

// Watchdog: radio silence -> sign of life, no answer -> restart with backoff
volatile uint32_t zbLastRx = 0;
volatile bool zbArmed = false, zbRxSeen = false;
bool zbLost = false;
uint32_t zbLastProbe = 0;
RTC_NOINIT_ATTR uint32_t wdMagic;
RTC_NOINIT_ATTR uint32_t wdRejoins;
constexpr uint32_t WD_MAGIC = 0x4B4E4F53;  // "KNOS"

void zbMarkRx() { zbLastRx = millis(); zbArmed = true; zbRxSeen = true; }
bool zbJoined() { return zbStarted && Zigbee.connected(); }
bool zbReady() { return zbJoined() && !zbLost; }

// ---------------------------------------------------------------------------
void refreshRing(bool wake = true) {
  const uint8_t *c = SIMPLE_COLORS[active];
  ring.setIdleColor(c[0], c[1], c[2]);
  ring.showBar(value[active], c[0], c[1], c[2], wake);
}

void scheduleSave() { saveAt = millis() + SIMPLE_SAVE_MS; }

void saveNow() {
  saveAt = 0;
  prefs.putBytes("val", value, 3);
  prefs.putUChar("active", active);
}

void setValue(uint8_t i, int v) {
  v = constrain(v, 0, 100);
  if (v == value[i]) return;
  value[i] = v;
  valueDirty[i] = true;
  scheduleSave();
  Serial.printf("VAL %u %u\n", i + 1, value[i]);
}

void selectValue(uint8_t i) {
  if (i != active) {
    active = i;
    activeDirty = true;
    scheduleSave();
    Serial.printf("SEL %u\n", i + 1);
  }
  refreshRing();
  const uint8_t *c = SIMPLE_COLORS[active];
  ring.wipe(c[0], c[1], c[2]);
}

// ---------------------------------------------------------------------------
void setupAnalog(KnobAnalog &ep, const char *desc, float mn, float mx) {
  ep.setManufacturerAndModel("DIY", "MagnetKnobSimple");
  ep.addAnalogInput();
  ep.setAnalogInputDescription(desc);
  ep.setAnalogInputMinMax(mn, mx);
  ep.setAnalogInputResolution(1);
  ep.addAnalogOutput();
  ep.setAnalogOutputDescription(desc);
  ep.setAnalogOutputMinMax(mn, mx);
  ep.setAnalogOutputResolution(1);
}

void zigbeeTask(void *) {
  static const char *const names[3] = {"Value 1", "Value 2", "Value 3"};
  for (uint8_t i = 0; i < 3; i++) {
    setupAnalog(zbValue[i], names[i], 0, 100);
    if (i == 0) {
      zbValue[i].setPowerSource(ZB_POWER_SOURCE_MAINS);
      zbValue[i].setSwBuild(SIMPLE_FW_VERSION);
    }
    Zigbee.addEndpoint(&zbValue[i]);
  }
  zbValue[0].onAnalogOutputChange([](float v) { rxValue[0] = (int16_t)constrain(lroundf(v), 0L, 100L); });
  zbValue[1].onAnalogOutputChange([](float v) { rxValue[1] = (int16_t)constrain(lroundf(v), 0L, 100L); });
  zbValue[2].onAnalogOutputChange([](float v) { rxValue[2] = (int16_t)constrain(lroundf(v), 0L, 100L); });

  setupAnalog(zbActive, "Active value", 1, 3);
  zbActive.onAnalogOutputChange([](float v) { rxActive = (int8_t)constrain(lroundf(v), 1L, 3L) - 1; });
  Zigbee.addEndpoint(&zbActive);

  setupAnalog(zbRing, "Ring brightness", 0, 100);
  zbRing.onAnalogOutputChange([](float v) { rxRing = (int16_t)constrain(lroundf(v), 0L, 100L); });
  Zigbee.addEndpoint(&zbRing);

  zbEvent.setManufacturerAndModel("DIY", "MagnetKnobSimple");
  zbEvent.addAnalogInput();
  zbEvent.setAnalogInputDescription("Event");
  zbEvent.setAnalogInputMinMax(0, 99);
  zbEvent.setAnalogInputResolution(1);
  Zigbee.addEndpoint(&zbEvent);

  // Every default response (to reports) shows that the network answers
  Zigbee.onGlobalDefaultResponse([](zb_cmd_type_t, esp_zb_zcl_status_t, uint8_t, uint16_t) { zbMarkRx(); });

  Zigbee.setRxOnWhenIdle(true);  // end device: never switch the receiver off (USB powered)
  while (!Zigbee.begin(ZB_ROLE)) {
    log_e("Zigbee start failed, retrying in 10 s");
    vTaskDelay(pdMS_TO_TICKS(10000));
  }
  zbStarted = true;
  vTaskDelete(nullptr);
}

// Apply changes from HA
void zigbeeReceive() {
  for (uint8_t i = 0; i < 3; i++) {
    int16_t v = rxValue[i];
    if (v < 0) continue;
    rxValue[i] = -1;
    setValue(i, v);
    valueDirty[i] = true;  // confirm even if unchanged
    if (i == active) refreshRing();
  }
  int8_t a = rxActive;
  if (a >= 0) {
    rxActive = -1;
    selectValue(a);
    activeDirty = true;
  }
  int16_t r = rxRing;
  if (r >= 0) {
    rxRing = -1;
    if (r != ringBright) {
      ringBright = r;
      prefs.putUChar("ring", ringBright);
      ring.setBrightness(ringBright);
      refreshRing();  // preview
    }
    ringDirty = true;
  }
}

// Report own values to the coordinator (throttled). Only the input cluster is set:
// setAnalogOutput() calls our own callback and would bring back old values while rotating.
void zigbeeReport() {
  if (!zbReady() || millis() - lastSend < ZB_SEND_INTERVAL) return;
  bool sent = false;
  for (uint8_t i = 0; i < 3; i++) {
    if (!valueDirty[i]) continue;
    valueDirty[i] = false;
    zbValue[i].setAnalogInput(value[i]);
    zbReportAnalogInput(SIMPLE_EP_VALUE1 + i);
    sent = true;
  }
  if (activeDirty) {
    activeDirty = false;
    zbActive.setAnalogInput(active + 1);
    zbReportAnalogInput(SIMPLE_EP_ACTIVE);
    sent = true;
  }
  if (ringDirty) {
    ringDirty = false;
    zbRing.setAnalogInput(ringBright);
    zbReportAnalogInput(SIMPLE_EP_RING);
    sent = true;
  }
  if (sent) lastSend = millis();
}

void zigbeeWatchdog() {
  if (!zbJoined()) return;
  const uint32_t now = millis();
  const uint32_t silent = now - zbLastRx;

  const uint32_t probeEvery = zbLost ? ZB_ALIVE_MS / 4 : ZB_ALIVE_MS;
  if (silent >= probeEvery && now - zbLastProbe >= probeEvery) {
    zbLastProbe = now;
    zbReportAnalogInput(SIMPLE_EP_ACTIVE);  // coordinator answers with a default response
  }

  const bool lost = zbArmed && silent >= ZB_LOST_MS;
  if (lost != zbLost) {
    zbLost = lost;
    if (!lost) { valueDirty[0] = valueDirty[1] = valueDirty[2] = activeDirty = ringDirty = true; }
  }
  if (!lost) {
    if (zbRxSeen) wdRejoins = 0;
    return;
  }
  const uint32_t wait = min<uint32_t>(ZB_REJOIN_MS << min<uint32_t>(wdRejoins, 5), ZB_REJOIN_MAX_MS);
  if (silent >= ZB_LOST_MS + wait) {
    wdRejoins++;
    Serial.printf("[zb] restarting to rejoin (attempt %u)\n", (unsigned)wdRejoins);
    Serial.flush();
    ESP.restart();
  }
}

void printZbStatus() {
  static int8_t last = -1;
  const int8_t s = zbReady() ? 2 : zbJoined() ? 1 : 0;
  if (s == last) return;
  last = s;
  static const char *const N[] = {"search", "lost", "ok"};
  Serial.printf("ZB %s\n", N[s]);
}

// Button event to Home Assistant (EP6). Also sent for the same code twice in a row (same button
// twice); Z2M configures reporting with a large threshold so the stack does not report on its own.
void sendEvent(uint8_t code) {
  Serial.printf("EVT %u\n", code);
  if (!zbReady()) return;
  zbEvent.setAnalogInput(code);
  zbReportAnalogInput(SIMPLE_EP_EVENT);
}

// ---------------------------------------------------------------------------
void handleRotation(int steps) {
  if (steps == 0) return;
  lastInput = millis();
  statusLed.blip(SIMPLE_COLORS[active][0], SIMPLE_COLORS[active][1], SIMPLE_COLORS[active][2]);
  setValue(active, value[active] + steps * SIMPLE_STEP);
  refreshRing();
}

void handleButton(uint8_t i, BtnEvent e) {
  if (e == BtnEvent::None) return;
  lastInput = millis();
  if (i == 2 && e == BtnEvent::VeryLong) {
    ring.setIdleColor(255, 90, 0);
    ring.setStatus(Ring::Status::Pairing);
    ring.update();
    if (zbStarted) Zigbee.factoryReset();  // restarts and searches for a network
    return;
  }
  const uint8_t *c = SIMPLE_COLORS[i];
  statusLed.blip(c[0], c[1], c[2]);
  if (e == BtnEvent::Long) {
    selectValue(i);
  } else if (e == BtnEvent::Click || e == BtnEvent::Double) {
    sendEvent((i + 1) * 10 + (e == BtnEvent::Click ? 1 : 2));
    ring.flash(c[0], c[1], c[2], 60, e == BtnEvent::Double ? 2 : 1);  // confirmation in the button's color
  }
}

// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  // Without a reader on the PC the TX buffer fills up and every print() would wait up to
  // 20 x 100 ms -> knob and ring freeze. This way surplus lines are simply dropped.
  Serial.setTxTimeoutMs(0);

  if (wdMagic != WD_MAGIC || esp_reset_reason() != ESP_RST_SW) { wdMagic = WD_MAGIC; wdRejoins = 0; }
  if (wdRejoins > 0) zbArmed = true;

  prefs.begin("knobs", false);
  if (prefs.getBytesLength("val") == 3) {
    prefs.getBytes("val", value, 3);
    for (uint8_t i = 0; i < 3; i++) value[i] = min<uint8_t>(value[i], 100);
  }
  active = min<uint8_t>(prefs.getUChar("active", 0), 2);
  ringBright = min<uint8_t>(prefs.getUChar("ring", 100), 100);

  ring.begin();
  ring.setBrightness(ringBright);
  statusLed.begin();
  for (uint8_t i = 0; i < 3; i++) { btn[i].begin(PIN_BTN[i]); btn[i].setDouble(true); }

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
  encoderOk = knob.begin(Wire, STEPS_PER_REV, STEP_HYSTERESIS, INVERT_ROTATION);
  if (!encoderOk) {
    log_e("AS5600 not found (I2C 0x36)");
    ring.setStatus(Ring::Status::Error);
  } else if (!knob.magnetDetected()) {
    log_w("No magnet detected – check distance / magnetization (must be diametric!)");
  }

  xTaskCreate(zigbeeTask, "zb_init", 6144, nullptr, 5, nullptr);

  Serial.println("HELLO knob-simple 1");
  refreshRing();
}

void loop() {
  if (encoderOk) handleRotation(knob.poll());
  for (uint8_t i = 0; i < 3; i++) handleButton(i, btn[i].poll());

  // Progress display while holding button 3 (reset warning)
  static bool resetShown = false;
  if (btn[2].held() && btn[2].heldMs() > 1500) {
    resetShown = true;
    ring.showBar((btn[2].heldMs() - 1500) * 100.0f / (BTN_RESET_MS - 1500), 255, 0, 0);
  } else if (resetShown) {
    resetShown = false;
    refreshRing(false);
  }

  // Without a network: spinner while idle; connection lost: breathing
  if (encoderOk) {
    using RS = Ring::Status;
    RS st = RS::Normal;
    const bool idle = millis() - lastInput > LED_SHOW_MS;
    if (!btn[2].held()) st = !zbJoined() ? (idle ? RS::Pairing : RS::Normal) : zbLost ? RS::Pulse : RS::Normal;
    ring.setStatus(st);
  }

  using S = StatusLed::State;
  if (btn[2].held() && btn[2].heldMs() > 1500) statusLed.setState(S::ResetPending);
  else if (!encoderOk)                         statusLed.setState(S::SensorError);
  else if (zbReady())                          statusLed.setState(S::Connected);
  else                                         statusLed.setState(S::Searching);

  if (saveAt && (int32_t)(millis() - saveAt) >= 0) saveNow();

  zigbeeReceive();
  zigbeeReport();
  zigbeeWatchdog();
  printZbStatus();
  ring.update();
  statusLed.update();
  delay(2);
}
