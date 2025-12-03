// DUST PRESS live control demo — Teensy 4.x + Audio Shield
// Reads analog pots + optional MIDI CC and maps them to the Dust Press engine using the
// ranges/tapers from docs/USAGE.md. Pot advice: run a log pot for Drive so most of the
// throw lives in the tasteful 0–18 dB zone; Bias and Env→Drive should be wired as bipolar
// (midpoint = 0) so you can sweep negative/positive; Mix can be linear. Values are clamped
// to the control-map limits from docs/USAGE.md so runaway knobs or rowdy MIDI stay safe.

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include "DustPress.h"

// --- Audio graph (same wiring as examples/minimal) ---
AudioInputI2S        i2sIn;
AudioDustPress       dustpress;
AudioOutputI2S       i2sOut;
AudioConnection      patchCord1(i2sIn, 0, dustpress, 0);
AudioConnection      patchCord2(i2sIn, 1, dustpress, 1);
AudioConnection      patchCord3(dustpress, 0, i2sOut, 0);
AudioConnection      patchCord4(dustpress, 1, i2sOut, 1);
AudioControlSGTL5000 codec;

// --- Hardware inputs ---
// Swap these to match your wiring. Drive likes a log/audio-taper pot; Bias and Env→Drive
// want a center-detent/bipolar vibe.
const int POT_DRIVE_PIN       = A0;
const int POT_BIAS_PIN        = A1;
const int POT_ENV_DRIVE_PIN   = A2;
const int POT_MIX_PIN         = A3;

// Optional MIDI CC bindings (Teensy usbMIDI or 5-pin). Use whatever numbers match your controller.
const uint8_t CC_DRIVE        = 20;
const uint8_t CC_BIAS         = 21;
const uint8_t CC_ENV_DRIVE    = 22;
const uint8_t CC_MIX          = 23;

// Control-map limits
constexpr float DRIVE_MIN_DB       = 0.0f;
constexpr float DRIVE_MAX_DB       = 36.0f;
constexpr float BIAS_MIN           = -1.0f;
constexpr float BIAS_MAX           = 1.0f;
constexpr float ENV_DRIVE_MIN_DB   = -12.0f;
constexpr float ENV_DRIVE_MAX_DB   = 12.0f;
constexpr float MIX_MIN            = 0.0f;
constexpr float MIX_MAX            = 1.0f;

// Utility clamp so pots + MIDI never outrun the map.
template <typename T>
T clampValue(T v, T lo, T hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

// Map a raw analogRead (0–1023) to 0..36 dB. If you used a linear pot instead of log, the
// squared curve below gives a pseudo-log feel; with an actual log pot this still behaves.
float mapDrive(int raw) {
  float norm = clampValue(raw / 1023.0f, 0.0f, 1.0f);
  float logish = norm * norm;  // gentle warp toward the low end
  float dB = DRIVE_MIN_DB + logish * (DRIVE_MAX_DB - DRIVE_MIN_DB);
  return clampValue(dB, DRIVE_MIN_DB, DRIVE_MAX_DB);
}

// Bipolar helper for Bias (-1..+1) and Env→Drive (-12..+12 dB). Center detent on the pot keeps 0.
float mapBipolar(int raw, float minVal, float maxVal) {
  float norm = clampValue(raw / 1023.0f, 0.0f, 1.0f);
  float bipolar = (norm * 2.0f) - 1.0f;  // -1 at CCW, +1 at CW
  float halfSpan = (maxVal - minVal) * 0.5f;
  float center = (minVal + maxVal) * 0.5f;
  return clampValue(center + bipolar * halfSpan, minVal, maxVal);
}

float mapMix(int raw) {
  float norm = clampValue(raw / 1023.0f, 0.0f, 1.0f);
  return clampValue(norm, MIX_MIN, MIX_MAX);
}

void applyPotReads() {
  dustpress.setDriveDb(mapDrive(analogRead(POT_DRIVE_PIN)));
  dustpress.setBias(mapBipolar(analogRead(POT_BIAS_PIN), BIAS_MIN, BIAS_MAX));
  dustpress.setEnvToDriveDb(mapBipolar(analogRead(POT_ENV_DRIVE_PIN), ENV_DRIVE_MIN_DB, ENV_DRIVE_MAX_DB));
  dustpress.setMix(mapMix(analogRead(POT_MIX_PIN)));
}

void handleMidiCc(uint8_t cc, uint8_t val) {
  float norm = val / 127.0f;
  switch (cc) {
    case CC_DRIVE: {
      float logish = norm * norm;  // mimic the log pot taper for MIDI
      float dB = DRIVE_MIN_DB + logish * (DRIVE_MAX_DB - DRIVE_MIN_DB);
      dustpress.setDriveDb(clampValue(dB, DRIVE_MIN_DB, DRIVE_MAX_DB));
      break;
    }
    case CC_BIAS: {
      float bipolar = (norm * 2.0f) - 1.0f;
      dustpress.setBias(clampValue(bipolar, BIAS_MIN, BIAS_MAX));
      break;
    }
    case CC_ENV_DRIVE: {
      float bipolar = (norm * 2.0f) - 1.0f;
      float halfSpan = (ENV_DRIVE_MAX_DB - ENV_DRIVE_MIN_DB) * 0.5f;
      float center = (ENV_DRIVE_MAX_DB + ENV_DRIVE_MIN_DB) * 0.5f;
      float dB = center + bipolar * halfSpan;
      dustpress.setEnvToDriveDb(clampValue(dB, ENV_DRIVE_MIN_DB, ENV_DRIVE_MAX_DB));
      break;
    }
    case CC_MIX: {
      dustpress.setMix(clampValue(norm, MIX_MIN, MIX_MAX));
      break;
    }
    default:
      break;
  }
}

void setup() {
  AudioMemory(64);
  codec.enable();
  codec.inputSelect(AUDIO_INPUT_LINEIN);
  codec.volume(0.6);

  // Defaults match the control map so you start in a safe zone.
  dustpress.setDriveDb(12.0f);
  dustpress.setBias(0.0f);
  dustpress.setEnvToDriveDb(6.0f);
  dustpress.setMix(0.5f);
}

void loop() {
  applyPotReads();

  // MIDI is optional; if nothing is connected this just no-ops.
  while (usbMIDI.read()) {
    if (usbMIDI.getType() == usbMIDI.ControlChange) {
      handleMidiCc(usbMIDI.getData1(), usbMIDI.getData2());
    }
  }
}
