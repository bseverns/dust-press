// Dust Press on PlatformIO — Horizon-ish control surface sketch
// This is a playable front end for the DSP core using the same vibes as the Horizon rig:
// a ring of knobs for the tone/drive spine plus two pushbuttons to step the discrete states
// (curve bank + chaos depth). Swap pins to match your panel harness; the parameter ranges
// stay clamped to docs/CONTROL_MAP.csv so you can't blow past the intended limits.

#include <Arduino.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include "DustPress.h"

// --- Audio graph ---
AudioInputI2S        i2sIn;
AudioDustPress       dustpress;
AudioOutputI2S       i2sOut;
AudioConnection      patchCord1(i2sIn, 0, dustpress, 0);
AudioConnection      patchCord2(i2sIn, 1, dustpress, 1);
AudioConnection      patchCord3(dustpress, 0, i2sOut, 0);
AudioConnection      patchCord4(dustpress, 1, i2sOut, 1);
AudioControlSGTL5000 codec;

// --- Horizon-ish control surface pins ---
// Ten knobs in a horseshoe, two buttons for stepping discrete states. Rename these to
// whatever your harness/panel uses; the logic below assumes 10-bit ADC reads.
constexpr int POT_DRIVE_PIN      = A0;
constexpr int POT_BIAS_PIN       = A1;
constexpr int POT_ENV_DRIVE_PIN  = A2;
constexpr int POT_GATE_PIN       = A3;
constexpr int POT_PRE_TILT_PIN   = A4;
constexpr int POT_POST_AIR_PIN   = A5;
constexpr int POT_MIX_PIN        = A6;
constexpr int POT_DIRT_PIN       = A7;
constexpr int POT_CEILING_PIN    = A8;
constexpr int POT_OUTPUT_PIN     = A9;

constexpr int BUTTON_CURVE_PIN   = 2;  // steps through the 0..3 curve bank
constexpr int BUTTON_CHAOS_PIN   = 3;  // steps 0..7 chaos levels (int-friendly for crackle depth)

// --- Control-map limits (mirrors docs/CONTROL_MAP.csv) ---
constexpr float DRIVE_MIN_DB       = 0.0f;
constexpr float DRIVE_MAX_DB       = 36.0f;
constexpr float BIAS_MIN           = -1.0f;
constexpr float BIAS_MAX           = 1.0f;
constexpr float ENV_DRIVE_MIN_DB   = -12.0f;
constexpr float ENV_DRIVE_MAX_DB   = 12.0f;
constexpr float GATE_MIN           = 0.0f;
constexpr float GATE_MAX           = 1.0f;
constexpr float PRE_TILT_MIN       = -6.0f;
constexpr float PRE_TILT_MAX       = 6.0f;
constexpr float POST_AIR_MIN       = -6.0f;
constexpr float POST_AIR_MAX       = 6.0f;
constexpr float MIX_MIN            = 0.0f;
constexpr float MIX_MAX            = 1.0f;
constexpr float DIRT_MIN           = 0.0f;
constexpr float DIRT_MAX           = 1.0f;
constexpr float CEILING_MIN_DB     = -6.0f;
constexpr float CEILING_MAX_DB     = 0.0f;
constexpr float OUTPUT_MIN_DB      = -12.0f;
constexpr float OUTPUT_MAX_DB      = 6.0f;

// --- Utility helpers ---
template <typename T>
T clampValue(T v, T lo, T hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

float readNorm(int pin) {
  return clampValue(static_cast<float>(analogRead(pin)) / 1023.0f, 0.0f, 1.0f);
}

float mapDrive(int pin) {
  float norm = readNorm(pin);
  float logish = norm * norm;  // mimic audio taper if you used a linear pot
  return DRIVE_MIN_DB + logish * (DRIVE_MAX_DB - DRIVE_MIN_DB);
}

float mapBipolar(int pin, float minVal, float maxVal) {
  float norm = readNorm(pin);
  float bipolar = (norm * 2.0f) - 1.0f;
  float span = (maxVal - minVal) * 0.5f;
  return clampValue(bipolar * span + (minVal + span), minVal, maxVal);
}

float mapLinear(int pin, float minVal, float maxVal) {
  float norm = readNorm(pin);
  return clampValue(minVal + norm * (maxVal - minVal), minVal, maxVal);
}

// Button edge detector without extra deps; returns true on rising edge.
bool readRisingEdge(int pin) {
  static bool lastStates[10] = {false};  // indexed by pin number; assumes pins < 10 here
  bool current = digitalRead(pin) == LOW;  // active-low with pullups
  bool rising = current && !lastStates[pin];
  lastStates[pin] = current;
  return rising;
}

uint8_t curveIndex = 0;
uint8_t chaosIndex = 0;

void setup() {
  AudioMemory(80);
  analogReadResolution(10);

  pinMode(BUTTON_CURVE_PIN, INPUT_PULLUP);
  pinMode(BUTTON_CHAOS_PIN, INPUT_PULLUP);

  codec.enable();
  codec.inputSelect(AUDIO_INPUT_LINEIN);
  codec.volume(0.6);

  // Safe defaults lifted straight from the control map.
  dustpress.setDriveDb(12.0f);
  dustpress.setBias(0.0f);
  dustpress.setCurveIndex(curveIndex);
  dustpress.setEnvToDriveDb(6.0f);
  dustpress.setGateComp(0.2f);
  dustpress.setPreTilt(0.0f);
  dustpress.setPostAir(0.0f);
  dustpress.setMix(0.5f);
  dustpress.setChaos(0.0f);
  dustpress.setDirt(0.1f);
  dustpress.setCeiling(-1.0f);
  dustpress.setOutputTrimDb(0.0f);
}

void applyKnobs() {
  dustpress.setDriveDb(mapDrive(POT_DRIVE_PIN));
  dustpress.setBias(mapBipolar(POT_BIAS_PIN, BIAS_MIN, BIAS_MAX));
  dustpress.setEnvToDriveDb(mapBipolar(POT_ENV_DRIVE_PIN, ENV_DRIVE_MIN_DB, ENV_DRIVE_MAX_DB));
  dustpress.setGateComp(mapLinear(POT_GATE_PIN, GATE_MIN, GATE_MAX));
  dustpress.setPreTilt(mapLinear(POT_PRE_TILT_PIN, PRE_TILT_MIN, PRE_TILT_MAX));
  dustpress.setPostAir(mapLinear(POT_POST_AIR_PIN, POST_AIR_MIN, POST_AIR_MAX));
  dustpress.setMix(mapLinear(POT_MIX_PIN, MIX_MIN, MIX_MAX));
  dustpress.setDirt(mapLinear(POT_DIRT_PIN, DIRT_MIN, DIRT_MAX));
  dustpress.setCeiling(mapLinear(POT_CEILING_PIN, CEILING_MIN_DB, CEILING_MAX_DB));
  dustpress.setOutputTrimDb(mapLinear(POT_OUTPUT_PIN, OUTPUT_MIN_DB, OUTPUT_MAX_DB));
}

void applyButtons() {
  if (readRisingEdge(BUTTON_CURVE_PIN)) {
    curveIndex = (curveIndex + 1) % 4;  // 0..3 curve bank
    dustpress.setCurveIndex(curveIndex);
  }

  if (readRisingEdge(BUTTON_CHAOS_PIN)) {
    chaosIndex = (chaosIndex + 1) % 8;  // 0..7 crackle steps
    dustpress.setChaos(static_cast<float>(chaosIndex));
  }
}

void loop() {
  applyKnobs();
  applyButtons();
}
