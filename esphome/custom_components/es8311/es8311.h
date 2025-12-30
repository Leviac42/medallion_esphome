#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/i2c/i2c.h"
#include <driver/i2s.h>

namespace esphome {
namespace es8311 {

// ES8311 Register definitions
namespace reg {
  constexpr uint8_t RESET = 0x00;
  constexpr uint8_t CLK_MANAGER1 = 0x01;
  constexpr uint8_t CLK_MANAGER2 = 0x02;
  constexpr uint8_t CLK_MANAGER3 = 0x03;
  constexpr uint8_t CLK_MANAGER4 = 0x04;
  constexpr uint8_t CLK_MANAGER5 = 0x05;
  constexpr uint8_t CLK_MANAGER6 = 0x06;
  constexpr uint8_t CLK_MANAGER7 = 0x07;
  constexpr uint8_t CLK_MANAGER8 = 0x08;
  constexpr uint8_t SDP_IN = 0x09;
  constexpr uint8_t SDP_OUT = 0x0A;
  constexpr uint8_t SYSTEM = 0x0B;
  constexpr uint8_t ADC = 0x0D;
  constexpr uint8_t DAC = 0x12;
  constexpr uint8_t GPIO = 0x0F;
  constexpr uint8_t GP = 0x10;
  constexpr uint8_t ADC_VOLUME = 0x17;
  constexpr uint8_t DAC_VOLUME = 0x32;
  constexpr uint8_t ADC_GAIN = 0x16;
  constexpr uint8_t CHIP_ID1 = 0xFD;
  constexpr uint8_t CHIP_ID2 = 0xFE;
}

enum ES8311MicGain : uint8_t {
  MIC_GAIN_0DB = 0,
  MIC_GAIN_6DB = 1,
  MIC_GAIN_12DB = 2,
  MIC_GAIN_18DB = 3,
  MIC_GAIN_24DB = 4,
  MIC_GAIN_30DB = 5,
  MIC_GAIN_36DB = 6,
  MIC_GAIN_42DB = 7,
};

class ES8311Component : public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void set_pa_enable_pin(GPIOPin *pin) { this->pa_enable_pin_ = pin; }
  void set_i2s_mclk_pin(uint8_t pin) { this->i2s_mclk_pin_ = pin; }
  void set_i2s_bclk_pin(uint8_t pin) { this->i2s_bclk_pin_ = pin; }
  void set_i2s_ws_pin(uint8_t pin) { this->i2s_ws_pin_ = pin; }
  void set_i2s_dout_pin(uint8_t pin) { this->i2s_dout_pin_ = pin; }
  void set_i2s_din_pin(uint8_t pin) { this->i2s_din_pin_ = pin; }
  void set_sample_rate(uint32_t rate) { this->sample_rate_ = rate; }
  void set_bits_per_sample(uint8_t bits) { this->bits_per_sample_ = bits; }
  void set_mic_gain(uint8_t gain) { this->mic_gain_ = gain; }

  // Audio control
  bool start_recording();
  void stop_recording();
  bool is_recording() const { return this->recording_; }
  
  // Read audio samples into buffer
  // Returns number of bytes read
  size_t read_samples(uint8_t *buffer, size_t max_size);

  // Set volume (0-100)
  void set_volume(uint8_t volume);

 protected:
  bool write_reg_(uint8_t reg, uint8_t value);
  uint8_t read_reg_(uint8_t reg);
  bool init_codec_();
  bool init_i2s_();
  void configure_clock_();

  GPIOPin *pa_enable_pin_{nullptr};
  uint8_t i2s_mclk_pin_{0};
  uint8_t i2s_bclk_pin_{0};
  uint8_t i2s_ws_pin_{0};
  uint8_t i2s_dout_pin_{0};
  uint8_t i2s_din_pin_{0};
  uint32_t sample_rate_{16000};
  uint8_t bits_per_sample_{16};
  uint8_t mic_gain_{MIC_GAIN_30DB};
  uint8_t volume_{70};

  bool initialized_{false};
  bool recording_{false};
  i2s_port_t i2s_port_{I2S_NUM_0};
};

}  // namespace es8311
}  // namespace esphome
