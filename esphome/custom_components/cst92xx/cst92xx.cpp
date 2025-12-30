#include "cst92xx.h"
#include "esphome/core/log.h"

namespace esphome {
namespace cst92xx {

static const char *const TAG = "cst92xx";

// CST92xx Register definitions
namespace reg {
  constexpr uint8_t TOUCH_DATA = 0x00;
  constexpr uint8_t GESTURE = 0x01;
  constexpr uint8_t TOUCH_NUM = 0x02;
  constexpr uint8_t TOUCH_XH = 0x03;
  constexpr uint8_t TOUCH_XL = 0x04;
  constexpr uint8_t TOUCH_YH = 0x05;
  constexpr uint8_t TOUCH_YL = 0x06;
  constexpr uint8_t CHIP_ID = 0xA7;
  constexpr uint8_t PROJ_ID = 0xA8;
  constexpr uint8_t FW_VERSION = 0xA9;
}

void CST92xxComponent::setup() {
  ESP_LOGI(TAG, "Setting up CST92xx touch controller...");

  // Reset the touch controller
  this->reset_controller_();

  // Setup interrupt pin if configured
  if (this->interrupt_pin_ != nullptr) {
    this->interrupt_pin_->setup();
  }

  // Try to read chip ID to verify communication
  uint8_t chip_id[3] = {0};
  if (this->read_register(reg::CHIP_ID, chip_id, 3)) {
    ESP_LOGI(TAG, "CST92xx Chip ID: 0x%02X%02X%02X", chip_id[0], chip_id[1], chip_id[2]);
  } else {
    ESP_LOGW(TAG, "Could not read chip ID, but continuing...");
  }

  // Read firmware version
  uint8_t fw_version = 0;
  if (this->read_register(reg::FW_VERSION, &fw_version, 1)) {
    ESP_LOGD(TAG, "Firmware version: 0x%02X", fw_version);
  }

  this->initialized_ = true;
  ESP_LOGI(TAG, "CST92xx touch controller initialized (max: %dx%d)", this->max_x_, this->max_y_);
}

void CST92xxComponent::loop() {
  if (!this->initialized_) return;

  // Check for touch data
  this->read_touch_data_();
}

void CST92xxComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "CST92xx Touch Controller:");
  LOG_I2C_DEVICE(this);
  if (this->interrupt_pin_ != nullptr) {
    LOG_PIN("  Interrupt Pin: ", this->interrupt_pin_);
  }
  if (this->reset_pin_ != nullptr) {
    LOG_PIN("  Reset Pin: ", this->reset_pin_);
  }
  ESP_LOGCONFIG(TAG, "  Max X: %d", this->max_x_);
  ESP_LOGCONFIG(TAG, "  Max Y: %d", this->max_y_);
  ESP_LOGCONFIG(TAG, "  Mirror X: %s", this->mirror_x_ ? "Yes" : "No");
  ESP_LOGCONFIG(TAG, "  Mirror Y: %s", this->mirror_y_ ? "Yes" : "No");
}

void CST92xxComponent::reset_controller_() {
  if (this->reset_pin_ == nullptr) return;

  this->reset_pin_->setup();
  this->reset_pin_->digital_write(true);
  delay(5);
  this->reset_pin_->digital_write(false);
  delay(20);
  this->reset_pin_->digital_write(true);
  delay(20);

  ESP_LOGD(TAG, "Touch controller reset complete");
}

bool CST92xxComponent::read_touch_data_() {
  // CST92xx touch data format:
  // Byte 0: Gesture ID
  // Byte 1: Number of touch points
  // For each point (starting at byte 2):
  //   Byte 0: Event flag (4 bits) + X high (4 bits)
  //   Byte 1: X low (8 bits)
  //   Byte 2: Touch ID (4 bits) + Y high (4 bits)
  //   Byte 3: Y low (8 bits)
  //   Byte 4: Weight
  //   Byte 5: Area

  uint8_t data[32] = {0};
  
  // Read touch data starting from register 0
  if (!this->read_register(reg::TOUCH_DATA, data, 8 + MAX_TOUCH_POINTS * 6)) {
    return false;
  }

  // Parse number of touch points
  uint8_t num_points = data[reg::TOUCH_NUM] & 0x0F;
  if (num_points > MAX_TOUCH_POINTS) {
    num_points = MAX_TOUCH_POINTS;
  }

  uint8_t prev_count = this->touch_count_;
  this->touch_count_ = num_points;

  // Parse each touch point
  for (uint8_t i = 0; i < num_points; i++) {
    uint8_t *point_data = &data[3 + i * 6];
    
    uint8_t event = (point_data[0] >> 4) & 0x0F;
    int16_t x = ((point_data[0] & 0x0F) << 8) | point_data[1];
    uint8_t id = (point_data[2] >> 4) & 0x0F;
    int16_t y = ((point_data[2] & 0x0F) << 8) | point_data[3];

    // Apply mirroring
    if (this->mirror_x_) {
      x = this->max_x_ - x;
    }
    if (this->mirror_y_) {
      y = this->max_y_ - y;
    }

    // Clamp to valid range
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= this->max_x_) x = this->max_x_ - 1;
    if (y >= this->max_y_) y = this->max_y_ - 1;

    this->touch_points_[i].x = x;
    this->touch_points_[i].y = y;
    this->touch_points_[i].id = id;
    this->touch_points_[i].pressed = (event == 0 || event == 2);  // Down or Contact

    // Fire callback for new touches
    if (i == 0 && num_points > 0 && prev_count == 0) {
      this->touch_callbacks_.call(num_points, x, y);
    }
  }

  // Clear remaining points
  for (uint8_t i = num_points; i < MAX_TOUCH_POINTS; i++) {
    this->touch_points_[i].pressed = false;
  }

  return true;
}

}  // namespace cst92xx
}  // namespace esphome
