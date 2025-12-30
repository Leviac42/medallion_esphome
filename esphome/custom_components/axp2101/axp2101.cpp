#include "axp2101.h"
#include "esphome/core/log.h"

namespace esphome {
namespace axp2101 {

static const char *const TAG = "axp2101";

void AXP2101Component::setup() {
  ESP_LOGI(TAG, "Setting up AXP2101 PMIC...");

  // Check chip ID
  uint8_t chip_id = this->read_register_(reg::CHIP_ID);
  if (chip_id != 0x4A && chip_id != 0x4B) {
    ESP_LOGE(TAG, "Invalid chip ID: 0x%02X (expected 0x4A or 0x4B)", chip_id);
    this->mark_failed();
    return;
  }
  ESP_LOGI(TAG, "AXP2101 detected (chip ID: 0x%02X)", chip_id);

  // Setup IRQ pin if configured
  if (this->irq_pin_ != nullptr) {
    this->irq_pin_->setup();
  }

  // Enable ADC for battery and VBUS measurements
  this->enable_adc_();

  // Configure power rails
  this->configure_power_rails_();

  // Clear any pending IRQs
  this->clear_irq();

  this->initialized_ = true;
  ESP_LOGI(TAG, "AXP2101 PMIC initialized successfully");
}

void AXP2101Component::loop() {
  if (!this->initialized_) return;

  // Update sensors every 5 seconds
  uint32_t now = millis();
  if (now - this->last_sensor_update_ >= 5000) {
    this->last_sensor_update_ = now;
    this->update_sensors_();
  }
}

void AXP2101Component::dump_config() {
  ESP_LOGCONFIG(TAG, "AXP2101 PMIC:");
  LOG_I2C_DEVICE(this);
  if (this->irq_pin_ != nullptr) {
    LOG_PIN("  IRQ Pin: ", this->irq_pin_);
  }
  ESP_LOGCONFIG(TAG, "  DC1 Voltage: %d mV", this->dc1_voltage_);
  ESP_LOGCONFIG(TAG, "  ALDO1 Voltage: %d mV", this->aldo1_voltage_);
  ESP_LOGCONFIG(TAG, "  ALDO2 Voltage: %d mV", this->aldo2_voltage_);
  ESP_LOGCONFIG(TAG, "  ALDO3 Voltage: %d mV", this->aldo3_voltage_);
  ESP_LOGCONFIG(TAG, "  ALDO4 Voltage: %d mV", this->aldo4_voltage_);
  ESP_LOGCONFIG(TAG, "  BLDO1 Voltage: %d mV", this->bldo1_voltage_);
  ESP_LOGCONFIG(TAG, "  BLDO2 Voltage: %d mV", this->bldo2_voltage_);
}

bool AXP2101Component::write_register_(uint8_t reg, uint8_t value) {
  return this->write_byte(reg, value);
}

uint8_t AXP2101Component::read_register_(uint8_t reg) {
  uint8_t value = 0;
  this->read_byte(reg, &value);
  return value;
}

void AXP2101Component::enable_adc_() {
  // Enable battery voltage and VBUS voltage ADC
  // ADC_CTRL: bit 0 = VBAT, bit 2 = VBUS, bit 3 = VSYS
  uint8_t adc_ctrl = this->read_register_(reg::ADC_CTRL);
  adc_ctrl |= 0x0D;  // Enable VBAT, VBUS, VSYS
  this->write_register_(reg::ADC_CTRL, adc_ctrl);
  ESP_LOGD(TAG, "ADC enabled: 0x%02X", adc_ctrl);
}

bool AXP2101Component::set_voltage_rail_(uint8_t reg, uint16_t voltage_mv, uint16_t min_mv, uint16_t max_mv, uint16_t step_mv) {
  if (voltage_mv < min_mv) voltage_mv = min_mv;
  if (voltage_mv > max_mv) voltage_mv = max_mv;
  
  uint8_t steps = (voltage_mv - min_mv) / step_mv;
  return this->write_register_(reg, steps);
}

void AXP2101Component::configure_power_rails_() {
  ESP_LOGD(TAG, "Configuring power rails...");

  // Enable DC-DC converters (bit 0 = DC1, etc.)
  uint8_t dcdc_ctrl = this->read_register_(reg::DCDC_CTRL);
  dcdc_ctrl |= 0x01;  // Enable DC1
  this->write_register_(reg::DCDC_CTRL, dcdc_ctrl);

  // Set DC1 voltage (500mV - 3400mV, 10mV steps, starting at 1500mV for upper range)
  // DC1 has two ranges: 500-1200mV (10mV) and 1220-3400mV (20mV)
  if (this->dc1_voltage_ <= 1200) {
    uint8_t val = (this->dc1_voltage_ - 500) / 10;
    this->write_register_(reg::DC1_VOLTAGE, val);
  } else {
    uint8_t val = 71 + (this->dc1_voltage_ - 1220) / 20;
    this->write_register_(reg::DC1_VOLTAGE, val);
  }

  // Enable all ALDOs and BLDOs
  // LDO_CTRL0: ALDO1-4 enable (bits 0-3)
  // LDO_CTRL1: BLDO1-2, DLDO1-2 enable (bits 0-3)
  uint8_t ldo_ctrl0 = this->read_register_(reg::LDO_CTRL0);
  ldo_ctrl0 |= 0x0F;  // Enable ALDO1-4
  this->write_register_(reg::LDO_CTRL0, ldo_ctrl0);

  uint8_t ldo_ctrl1 = this->read_register_(reg::LDO_CTRL1);
  ldo_ctrl1 |= 0x03;  // Enable BLDO1-2
  this->write_register_(reg::LDO_CTRL1, ldo_ctrl1);

  // ALDOx: 500mV - 3500mV, 100mV steps
  this->set_voltage_rail_(reg::ALDO1_VOLTAGE, this->aldo1_voltage_, 500, 3500, 100);
  this->set_voltage_rail_(reg::ALDO2_VOLTAGE, this->aldo2_voltage_, 500, 3500, 100);
  this->set_voltage_rail_(reg::ALDO3_VOLTAGE, this->aldo3_voltage_, 500, 3500, 100);
  this->set_voltage_rail_(reg::ALDO4_VOLTAGE, this->aldo4_voltage_, 500, 3500, 100);

  // BLDOx: 500mV - 3500mV, 100mV steps
  this->set_voltage_rail_(reg::BLDO1_VOLTAGE, this->bldo1_voltage_, 500, 3500, 100);
  this->set_voltage_rail_(reg::BLDO2_VOLTAGE, this->bldo2_voltage_, 500, 3500, 100);

  // Small delay for rails to stabilize
  delay(10);
  ESP_LOGD(TAG, "Power rails configured");
}

float AXP2101Component::get_battery_voltage() {
  uint8_t h = this->read_register_(reg::VBAT_H);
  uint8_t l = this->read_register_(reg::VBAT_L);
  // 14-bit ADC, 1mV per step
  uint16_t raw = ((uint16_t)h << 8) | l;
  raw &= 0x3FFF;
  return raw / 1000.0f;
}

uint8_t AXP2101Component::get_battery_level() {
  return this->read_register_(reg::BAT_PERCENT) & 0x7F;
}

float AXP2101Component::get_vbus_voltage() {
  uint8_t h = this->read_register_(reg::VBUS_H);
  uint8_t l = this->read_register_(reg::VBUS_L);
  // 14-bit ADC, 1mV per step
  uint16_t raw = ((uint16_t)h << 8) | l;
  raw &= 0x3FFF;
  return raw / 1000.0f;
}

void AXP2101Component::clear_irq() {
  // Clear all IRQ status registers by writing 0xFF
  this->write_register_(reg::IRQ_STATUS0, 0xFF);
  this->write_register_(reg::IRQ_STATUS1, 0xFF);
  this->write_register_(reg::IRQ_STATUS2, 0xFF);
}

void AXP2101Component::update_sensors_() {
  if (this->battery_voltage_sensor_ != nullptr) {
    float voltage = this->get_battery_voltage();
    this->battery_voltage_sensor_->publish_state(voltage);
  }

  if (this->battery_level_sensor_ != nullptr) {
    uint8_t level = this->get_battery_level();
    this->battery_level_sensor_->publish_state(level);
  }

  if (this->vbus_voltage_sensor_ != nullptr) {
    float voltage = this->get_vbus_voltage();
    this->vbus_voltage_sensor_->publish_state(voltage);
  }
}

}  // namespace axp2101
}  // namespace esphome
