#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace axp2101 {

// AXP2101 Register Definitions
namespace reg {
  // Status registers
  constexpr uint8_t PMU_STATUS1 = 0x00;
  constexpr uint8_t PMU_STATUS2 = 0x01;
  constexpr uint8_t CHIP_ID = 0x03;
  
  // IRQ registers
  constexpr uint8_t IRQ_EN0 = 0x40;
  constexpr uint8_t IRQ_EN1 = 0x41;
  constexpr uint8_t IRQ_EN2 = 0x42;
  constexpr uint8_t IRQ_STATUS0 = 0x48;
  constexpr uint8_t IRQ_STATUS1 = 0x49;
  constexpr uint8_t IRQ_STATUS2 = 0x4A;
  
  // ADC Control
  constexpr uint8_t ADC_CTRL = 0x30;
  
  // Battery voltage ADC
  constexpr uint8_t VBAT_H = 0x34;
  constexpr uint8_t VBAT_L = 0x35;
  
  // VBUS voltage ADC
  constexpr uint8_t VBUS_H = 0x38;
  constexpr uint8_t VBUS_L = 0x39;
  
  // Battery percentage
  constexpr uint8_t BAT_PERCENT = 0xA4;
  
  // DC-DC control
  constexpr uint8_t DCDC_CTRL = 0x80;
  constexpr uint8_t DC1_VOLTAGE = 0x82;
  constexpr uint8_t DC2_VOLTAGE = 0x83;
  constexpr uint8_t DC3_VOLTAGE = 0x84;
  constexpr uint8_t DC4_VOLTAGE = 0x85;
  constexpr uint8_t DC5_VOLTAGE = 0x86;
  
  // LDO control
  constexpr uint8_t LDO_CTRL0 = 0x90;
  constexpr uint8_t LDO_CTRL1 = 0x91;
  constexpr uint8_t ALDO1_VOLTAGE = 0x92;
  constexpr uint8_t ALDO2_VOLTAGE = 0x93;
  constexpr uint8_t ALDO3_VOLTAGE = 0x94;
  constexpr uint8_t ALDO4_VOLTAGE = 0x95;
  constexpr uint8_t BLDO1_VOLTAGE = 0x96;
  constexpr uint8_t BLDO2_VOLTAGE = 0x97;
}

class AXP2101Component : public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::BUS; }

  void set_irq_pin(GPIOPin *pin) { this->irq_pin_ = pin; }
  
  // Voltage setters (in mV)
  void set_dc1_voltage(uint16_t mv) { this->dc1_voltage_ = mv; }
  void set_aldo1_voltage(uint16_t mv) { this->aldo1_voltage_ = mv; }
  void set_aldo2_voltage(uint16_t mv) { this->aldo2_voltage_ = mv; }
  void set_aldo3_voltage(uint16_t mv) { this->aldo3_voltage_ = mv; }
  void set_aldo4_voltage(uint16_t mv) { this->aldo4_voltage_ = mv; }
  void set_bldo1_voltage(uint16_t mv) { this->bldo1_voltage_ = mv; }
  void set_bldo2_voltage(uint16_t mv) { this->bldo2_voltage_ = mv; }

  // Sensor setters
  void set_battery_voltage_sensor(sensor::Sensor *sensor) { this->battery_voltage_sensor_ = sensor; }
  void set_battery_level_sensor(sensor::Sensor *sensor) { this->battery_level_sensor_ = sensor; }
  void set_vbus_voltage_sensor(sensor::Sensor *sensor) { this->vbus_voltage_sensor_ = sensor; }

  // Get battery voltage in V
  float get_battery_voltage();
  // Get battery level as percentage
  uint8_t get_battery_level();
  // Get VBUS voltage in V
  float get_vbus_voltage();
  
  // Clear IRQ status
  void clear_irq();

 protected:
  bool write_register_(uint8_t reg, uint8_t value);
  uint8_t read_register_(uint8_t reg);
  bool set_voltage_rail_(uint8_t reg, uint16_t voltage_mv, uint16_t min_mv, uint16_t max_mv, uint16_t step_mv);
  void configure_power_rails_();
  void enable_adc_();
  void update_sensors_();

  GPIOPin *irq_pin_{nullptr};
  
  // Configured voltages (in mV)
  uint16_t dc1_voltage_{3300};
  uint16_t aldo1_voltage_{1800};
  uint16_t aldo2_voltage_{2800};
  uint16_t aldo3_voltage_{3300};
  uint16_t aldo4_voltage_{3000};
  uint16_t bldo1_voltage_{3300};
  uint16_t bldo2_voltage_{3300};

  // Sensors
  sensor::Sensor *battery_voltage_sensor_{nullptr};
  sensor::Sensor *battery_level_sensor_{nullptr};
  sensor::Sensor *vbus_voltage_sensor_{nullptr};

  uint32_t last_sensor_update_{0};
  bool initialized_{false};
};

}  // namespace axp2101
}  // namespace esphome
