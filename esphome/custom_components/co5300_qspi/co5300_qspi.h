#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/gpio.h"

namespace esphome {
namespace co5300_qspi {

class CO5300QSPIComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE - 1.0f; }

  void set_cs_pin(InternalGPIOPin *pin) { this->cs_pin_ = pin; }
  void set_sclk_pin(InternalGPIOPin *pin) { this->sclk_pin_ = pin; }
  void set_data0_pin(InternalGPIOPin *pin) { this->data0_pin_ = pin; }
  void set_data1_pin(InternalGPIOPin *pin) { this->data1_pin_ = pin; }
  void set_data2_pin(InternalGPIOPin *pin) { this->data2_pin_ = pin; }
  void set_data3_pin(InternalGPIOPin *pin) { this->data3_pin_ = pin; }
  void set_reset_pin(InternalGPIOPin *pin) { this->reset_pin_ = pin; }
  void set_width(uint16_t width) { this->width_ = width; }
  void set_height(uint16_t height) { this->height_ = height; }
  void set_brightness(uint8_t brightness);

  // Display control
  void fill_screen(uint16_t color);
  void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void draw_pixel(int16_t x, int16_t y, uint16_t color);
  
  // Get display dimensions
  uint16_t get_width() const { return this->width_; }
  uint16_t get_height() const { return this->height_; }

  // Get GFX pointer for external use
  void *get_gfx() { return this->gfx_; }

 protected:
  void reset_display_();
  void init_qspi_();

  InternalGPIOPin *cs_pin_{nullptr};
  InternalGPIOPin *sclk_pin_{nullptr};
  InternalGPIOPin *data0_pin_{nullptr};
  InternalGPIOPin *data1_pin_{nullptr};
  InternalGPIOPin *data2_pin_{nullptr};
  InternalGPIOPin *data3_pin_{nullptr};
  InternalGPIOPin *reset_pin_{nullptr};

  uint16_t width_{466};
  uint16_t height_{466};
  uint8_t brightness_{255};

  void *bus_{nullptr};
  void *gfx_{nullptr};
  bool initialized_{false};
};

}  // namespace co5300_qspi
}  // namespace esphome
