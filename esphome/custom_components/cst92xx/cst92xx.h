#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace cst92xx {

// Maximum number of touch points supported
constexpr uint8_t MAX_TOUCH_POINTS = 5;

struct TouchPoint {
  int16_t x;
  int16_t y;
  uint8_t id;
  bool pressed;
};

class CST92xxComponent : public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void set_interrupt_pin(GPIOPin *pin) { this->interrupt_pin_ = pin; }
  void set_reset_pin(GPIOPin *pin) { this->reset_pin_ = pin; }
  void set_width(uint16_t width) { this->max_x_ = width; }
  void set_height(uint16_t height) { this->max_y_ = height; }
  void set_mirror_x(bool mirror) { this->mirror_x_ = mirror; }
  void set_mirror_y(bool mirror) { this->mirror_y_ = mirror; }

  // Get current touch state
  uint8_t get_touch_count() const { return this->touch_count_; }
  const TouchPoint &get_touch_point(uint8_t index) const { return this->touch_points_[index]; }
  
  // Convenience method for single touch
  bool is_touched() const { return this->touch_count_ > 0; }
  int16_t get_touch_x() const { return this->touch_count_ > 0 ? this->touch_points_[0].x : -1; }
  int16_t get_touch_y() const { return this->touch_count_ > 0 ? this->touch_points_[0].y : -1; }

  // Set callback for touch events
  void add_on_touch_callback(std::function<void(uint8_t, int16_t, int16_t)> callback) {
    this->touch_callbacks_.add(std::move(callback));
  }

 protected:
  void reset_controller_();
  bool read_touch_data_();
  
  GPIOPin *interrupt_pin_{nullptr};
  GPIOPin *reset_pin_{nullptr};
  
  uint16_t max_x_{466};
  uint16_t max_y_{466};
  bool mirror_x_{true};
  bool mirror_y_{true};
  
  TouchPoint touch_points_[MAX_TOUCH_POINTS];
  uint8_t touch_count_{0};
  bool initialized_{false};
  
  CallbackManager<void(uint8_t, int16_t, int16_t)> touch_callbacks_;
};

}  // namespace cst92xx
}  // namespace esphome
