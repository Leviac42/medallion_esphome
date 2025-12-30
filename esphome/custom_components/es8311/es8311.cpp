#include "es8311.h"
#include "esphome/core/log.h"

namespace esphome {
namespace es8311 {

static const char *const TAG = "es8311";

void ES8311Component::setup() {
  ESP_LOGI(TAG, "Setting up ES8311 Audio Codec...");

  // Enable power amplifier if configured
  if (this->pa_enable_pin_ != nullptr) {
    this->pa_enable_pin_->setup();
    this->pa_enable_pin_->digital_write(true);
    ESP_LOGD(TAG, "PA enabled");
  }

  // Initialize codec via I2C
  if (!this->init_codec_()) {
    ESP_LOGE(TAG, "Failed to initialize ES8311 codec");
    this->mark_failed();
    return;
  }

  // Initialize I2S
  if (!this->init_i2s_()) {
    ESP_LOGE(TAG, "Failed to initialize I2S");
    this->mark_failed();
    return;
  }

  this->initialized_ = true;
  ESP_LOGI(TAG, "ES8311 Audio Codec initialized");
}

void ES8311Component::loop() {
  // Nothing to do in loop - audio is read on demand
}

void ES8311Component::dump_config() {
  ESP_LOGCONFIG(TAG, "ES8311 Audio Codec:");
  LOG_I2C_DEVICE(this);
  if (this->pa_enable_pin_ != nullptr) {
    LOG_PIN("  PA Enable Pin: ", this->pa_enable_pin_);
  }
  ESP_LOGCONFIG(TAG, "  I2S MCLK Pin: %d", this->i2s_mclk_pin_);
  ESP_LOGCONFIG(TAG, "  I2S BCLK Pin: %d", this->i2s_bclk_pin_);
  ESP_LOGCONFIG(TAG, "  I2S WS Pin: %d", this->i2s_ws_pin_);
  ESP_LOGCONFIG(TAG, "  I2S DOUT Pin: %d", this->i2s_dout_pin_);
  ESP_LOGCONFIG(TAG, "  I2S DIN Pin: %d", this->i2s_din_pin_);
  ESP_LOGCONFIG(TAG, "  Sample Rate: %d Hz", this->sample_rate_);
  ESP_LOGCONFIG(TAG, "  Bits Per Sample: %d", this->bits_per_sample_);
  ESP_LOGCONFIG(TAG, "  Mic Gain: %d (x6 dB)", this->mic_gain_);
}

bool ES8311Component::write_reg_(uint8_t reg, uint8_t value) {
  return this->write_byte(reg, value);
}

uint8_t ES8311Component::read_reg_(uint8_t reg) {
  uint8_t value = 0;
  this->read_byte(reg, &value);
  return value;
}

bool ES8311Component::init_codec_() {
  // Soft reset
  this->write_reg_(reg::RESET, 0x1F);
  delay(20);
  this->write_reg_(reg::RESET, 0x00);
  delay(20);

  // Check chip ID
  uint8_t id1 = this->read_reg_(reg::CHIP_ID1);
  uint8_t id2 = this->read_reg_(reg::CHIP_ID2);
  ESP_LOGD(TAG, "ES8311 Chip ID: 0x%02X 0x%02X", id1, id2);
  
  // ES8311 should return 0x83 0x11
  if (id1 != 0x83 || id2 != 0x11) {
    ESP_LOGW(TAG, "Unexpected chip ID (expected 0x83 0x11), continuing anyway...");
  }

  // Configure clocks
  this->configure_clock_();

  // Configure SDP (Serial Data Port)
  // I2S format, 16-bit
  uint8_t sdp_format = 0x00;  // I2S, normal polarity
  if (this->bits_per_sample_ == 24) {
    sdp_format |= 0x0C;  // 24-bit
  } else if (this->bits_per_sample_ == 32) {
    sdp_format |= 0x10;  // 32-bit
  }
  // else 16-bit is 0x00
  
  this->write_reg_(reg::SDP_IN, sdp_format);
  this->write_reg_(reg::SDP_OUT, sdp_format);

  // Configure ADC
  this->write_reg_(reg::SYSTEM, 0x00);  // Power up ADC and DAC
  this->write_reg_(reg::ADC, 0x00);     // ADC normal operation
  
  // Set microphone gain
  this->write_reg_(reg::ADC_GAIN, this->mic_gain_);
  
  // Set ADC volume (0-255, default 0xBF is 0dB)
  this->write_reg_(reg::ADC_VOLUME, 0xBF);
  
  // Configure DAC
  this->write_reg_(reg::DAC, 0x00);
  
  // Set DAC volume
  uint8_t vol = (this->volume_ * 255) / 100;
  this->write_reg_(reg::DAC_VOLUME, vol);

  // Enable analog mic input
  this->write_reg_(reg::GP, 0x00);  // Analog mic
  this->write_reg_(reg::GPIO, 0x00);

  ESP_LOGD(TAG, "ES8311 codec configured");
  return true;
}

void ES8311Component::configure_clock_() {
  // MCLK from MCLK pin, not from SCLK
  this->write_reg_(reg::CLK_MANAGER1, 0x3F);  // MCLK enable
  
  // For 16kHz sample rate with 256*fs MCLK (4.096MHz):
  // MCLK_DIV = 1, BCLK divider based on bits
  
  // Calculate dividers for the sample rate
  uint32_t mclk_freq = this->sample_rate_ * 256;
  
  // CLK_MANAGER2: MCLK divider
  this->write_reg_(reg::CLK_MANAGER2, 0x00);
  
  // CLK_MANAGER3-5: PLL config (bypassed when using direct MCLK)
  this->write_reg_(reg::CLK_MANAGER3, 0x10);
  this->write_reg_(reg::CLK_MANAGER4, 0x00);
  this->write_reg_(reg::CLK_MANAGER5, 0x00);
  
  // CLK_MANAGER6: ADC/DAC clock dividers
  this->write_reg_(reg::CLK_MANAGER6, 0x00);
  
  // CLK_MANAGER7-8: Additional dividers
  this->write_reg_(reg::CLK_MANAGER7, 0x00);
  this->write_reg_(reg::CLK_MANAGER8, 0xFF);  // Enable ADC/DAC clocks
  
  ESP_LOGD(TAG, "Clock configured for %d Hz sample rate", this->sample_rate_);
}

bool ES8311Component::init_i2s_() {
  i2s_config_t i2s_config = {};
  i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
  i2s_config.sample_rate = this->sample_rate_;
  i2s_config.bits_per_sample = (i2s_bits_per_sample_t)this->bits_per_sample_;
  i2s_config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  i2s_config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  i2s_config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  i2s_config.dma_buf_count = 4;
  i2s_config.dma_buf_len = 1024;
  i2s_config.use_apll = true;
  i2s_config.tx_desc_auto_clear = false;
  i2s_config.fixed_mclk = this->sample_rate_ * 256;

  esp_err_t err = i2s_driver_install(this->i2s_port_, &i2s_config, 0, nullptr);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "I2S driver install failed: %s", esp_err_to_name(err));
    return false;
  }

  i2s_pin_config_t pin_config = {};
  pin_config.mck_io_num = this->i2s_mclk_pin_;
  pin_config.bck_io_num = this->i2s_bclk_pin_;
  pin_config.ws_io_num = this->i2s_ws_pin_;
  pin_config.data_out_num = this->i2s_dout_pin_;
  pin_config.data_in_num = this->i2s_din_pin_;

  err = i2s_set_pin(this->i2s_port_, &pin_config);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "I2S set pin failed: %s", esp_err_to_name(err));
    return false;
  }

  ESP_LOGD(TAG, "I2S configured");
  return true;
}

bool ES8311Component::start_recording() {
  if (!this->initialized_) {
    ESP_LOGW(TAG, "Cannot start recording - not initialized");
    return false;
  }
  if (this->recording_) {
    ESP_LOGW(TAG, "Already recording");
    return false;
  }
  
  this->recording_ = true;
  ESP_LOGI(TAG, "Recording started");
  return true;
}

void ES8311Component::stop_recording() {
  if (!this->recording_) return;
  
  this->recording_ = false;
  ESP_LOGI(TAG, "Recording stopped");
}

size_t ES8311Component::read_samples(uint8_t *buffer, size_t max_size) {
  if (!this->initialized_ || !this->recording_) return 0;
  
  size_t bytes_read = 0;
  esp_err_t err = i2s_read(this->i2s_port_, buffer, max_size, &bytes_read, pdMS_TO_TICKS(20));
  
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "I2S read error: %s", esp_err_to_name(err));
    return 0;
  }
  
  return bytes_read;
}

void ES8311Component::set_volume(uint8_t volume) {
  if (volume > 100) volume = 100;
  this->volume_ = volume;
  
  if (this->initialized_) {
    uint8_t vol = (volume * 255) / 100;
    this->write_reg_(reg::DAC_VOLUME, vol);
  }
}

}  // namespace es8311
}  // namespace esphome
