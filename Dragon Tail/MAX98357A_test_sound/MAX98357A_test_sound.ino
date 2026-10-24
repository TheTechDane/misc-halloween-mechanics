/*
 * Native ESP32-C3 I2S Hardware Diagnostic Test
 * Generates a direct 440 Hz square wave tone over I2S to MAX98357A
 * 
 * Pin Mapping (XIAO ESP32-C3):
 * - I2S LRC / WS  : D3 (GPIO5)
 * - I2S BCLK      : D4 (GPIO6)
 * - I2S DOUT      : D5 (GPIO7)
 * - MAX98357A VIN : VBUS (5V)
 * - MAX98357A GND : GND
 */

#include <Arduino.h>
#include "driver/i2s.h"

#define I2S_LRC   GPIO_NUM_5  // D3
#define I2S_BCLK  GPIO_NUM_6  // D4
#define I2S_DOUT  GPIO_NUM_7  // D5

#define SAMPLE_RATE 44100
#define TONE_FREQ   440

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== Native ESP32-C3 I2S Hardware Test ===");

  // Configure I2S driver settings
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = true
  };

  // Configure pin layout
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  // Install and start I2S driver
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);

  Serial.println("[TEST] Generating 440 Hz Tone over I2S...");
}

void loop() {
  const int half_period = SAMPLE_RATE / TONE_FREQ / 2;
  static int16_t sample = 8000; // Amplitude (volume)
  static int count = 0;

  // Buffer for stereo 16-bit samples
  int16_t buffer[128];
  size_t bytes_written;

  for (int i = 0; i < 64; i++) {
    count++;
    if (count >= half_period) {
      sample = -sample; // Flip wave direction
      count = 0;
    }
    buffer[i * 2]     = sample; // Left channel
    buffer[i * 2 + 1] = sample; // Right channel
  }

  // Push PCM samples directly to hardware DMA
  i2s_write(I2S_NUM_0, buffer, sizeof(buffer), &bytes_written, portMAX_DELAY);
}