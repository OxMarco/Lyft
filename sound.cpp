#include <sound.h>
#include <math.h>
#include "ESP_I2S.h"
#include "esp_check.h"
#include "Wire.h"
#include "es8311.h"
#include "config.h"

static const char *TAG = "audio";
static uint8_t volume = AUDIO_VOLUME;
static es8311_handle_t es_handle = nullptr;

I2SClass i2s;

uint8_t getVolume() {
  return volume;
}

void setVolume(uint8_t _volume) {
  // Clamp low values to 0 (codec is inaudible below ~30%)
  volume = _volume <= 30 ? 0 : constrain(_volume, 0, 100);

  // Apply volume to codec
  if (es_handle) {
    es8311_voice_volume_set(es_handle, volume, NULL);
  }

  // Mute PA when volume is 0
  if (volume == 0) {
    digitalWrite(PA_CTRL_PIN, LOW);
  } else {
    digitalWrite(PA_CTRL_PIN, HIGH);
  }
}

static esp_err_t es8311_codec_init(void) {
  es_handle = es8311_create(I2C_NUM_0, ES8311_ADDRRES_0);
  ESP_RETURN_ON_FALSE(es_handle, ESP_FAIL, TAG, "es8311 create failed");

  const es8311_clock_config_t es_clk = {
    .mclk_inverted = false,
    .sclk_inverted = false,
    .mclk_from_mclk_pin = true,
    .mclk_frequency = AUDIO_MCLK_FREQ_HZ,
    .sample_frequency = AUDIO_SAMPLE_RATE
  };

  ESP_ERROR_CHECK(es8311_init(es_handle, &es_clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16));
  ESP_RETURN_ON_ERROR(es8311_voice_volume_set(es_handle, volume, NULL), TAG, "set volume failed");
  ESP_RETURN_ON_ERROR(es8311_microphone_config(es_handle, false), TAG, "mic config failed");

  return ESP_OK;
}

// ---------------- Simple synth helpers ----------------
static inline void writeSilenceMs(uint32_t ms) {
  const uint32_t total = (uint64_t)AUDIO_SAMPLE_RATE * ms / 1000;
  const int CHUNK = 256;
  int16_t buf[CHUNK];
  memset(buf, 0, sizeof(buf));

  uint32_t remaining = total;
  while (remaining) {
    uint32_t n = remaining > CHUNK ? CHUNK : remaining;
    i2s.write((uint8_t*)buf, n * 2);
    remaining -= n;
  }
}

// Sine tone with tiny attack/release to avoid clicks
static void playToneHz(uint16_t freq, uint32_t ms, int16_t amp = 11000, uint32_t attackMs = 5, uint32_t releaseMs = 8) {
  // Skip if muted
  if (volume == 0) return;

  const uint32_t total = (uint64_t)AUDIO_SAMPLE_RATE * ms / 1000;
  if (total == 0) return;

  const uint32_t attackS = (uint64_t)AUDIO_SAMPLE_RATE * attackMs / 1000;
  const uint32_t releaseS = (uint64_t)AUDIO_SAMPLE_RATE * releaseMs / 1000;

  const int CHUNK = 256;
  int16_t buf[CHUNK];

  double phase = 0.0;
  const double dphi = (2.0 * M_PI * (double)freq) / (double)AUDIO_SAMPLE_RATE;

  uint32_t written = 0;
  while (written < total) {
    uint32_t n = (total - written) > CHUNK ? CHUNK : (total - written);

    for (uint32_t i = 0; i < n; i++) {
      uint32_t idx = written + i;

      float env = 1.0f;
      if (attackS > 0 && idx < attackS) env = (float)idx / (float)attackS;
      if (releaseS > 0 && idx > (total - releaseS)) {
        float r = (float)(total - idx) / (float)releaseS;
        if (r < env) env = r;
      }
      if (env < 0) env = 0;

      int16_t sample = (int16_t)((float)amp * env * sin(phase));
      buf[i] = sample;

      phase += dphi;
      if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;
    }

    i2s.write((uint8_t*)buf, n * 2);
    written += n;
  }
}

// helper: a bell-ish hit = main tone + a soft harmonic
static void bellHit(uint16_t f, uint32_t ms, int16_t aMain, int16_t aHarm) {
  if (volume == 0) return;

  // main
  playToneHz(f, ms, aMain, 8, 60);

  // harmonic overlay (2x freq) quieter to prevent saturation
  playToneHz(f * 2, ms, (int16_t)(aHarm * 0.55f), 6, 80);
}

bool audioInit() {
  es8311_codec_init();

  i2s.setPins(I2S_BCK_PIN, I2S_LRCK_PIN, I2S_DOUT_PIN, I2S_DIN_PIN, I2S_MCK_PIN);
  if (!i2s.begin(I2S_MODE_STD, AUDIO_SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT,
                 I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT)) {
    Serial.println("Failed to initialize I2S bus!");
    return false;
  }

  pinMode(PA_CTRL_PIN, OUTPUT);
  digitalWrite(PA_CTRL_PIN, volume > 0 ? HIGH : LOW);

  return true;
}

void playPowerOnSound() {
  if (volume == 0) return;
  bellHit(659, 80, 21000, 4000);     // E5 (major 3rd)
  writeSilenceMs(40);
  bellHit(784, 120, 21000, 4000);    // G5 (perfect 5th)
}

void playPowerOffSound() {
  if (volume == 0) return;
  bellHit(659, 80, 19000, 3500);     // E5
  writeSilenceMs(40);
  bellHit(523, 140, 19000, 3200);    // C5 (longer tail = "settle")
}

void playStartWorkoutSound() {
  if (volume == 0) return;
  // Quick punchy double-beep: A5 → D6 (fourth interval, higher register)
  playToneHz(880, 50, 20000, 4, 10);   // A5 - short, punchy
  writeSilenceMs(25);
  playToneHz(1175, 80, 21000, 4, 15);  // D6 - slightly longer
}

void playStopWorkoutSound() {
  if (volume == 0) return;
  // Resolving drop: D6 → A5 (mirror of start)
  playToneHz(1175, 60, 19000, 4, 12);  // D6
  writeSilenceMs(30);
  playToneHz(880, 140, 18000, 4, 40);  // A5 - longer decay = "finished"
}
