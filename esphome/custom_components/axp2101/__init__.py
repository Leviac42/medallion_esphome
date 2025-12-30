import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, sensor
from esphome.const import (
    CONF_ID,
    CONF_ADDRESS,
    CONF_IRQ_PIN,
    DEVICE_CLASS_VOLTAGE,
    DEVICE_CLASS_BATTERY,
    STATE_CLASS_MEASUREMENT,
    UNIT_VOLT,
    UNIT_PERCENT,
)
from esphome import pins

DEPENDENCIES = ["i2c"]
AUTO_LOAD = ["sensor"]
MULTI_CONF = False

CONF_AXP2101_ID = "axp2101_id"

# Voltage rail configs
CONF_DC1_VOLTAGE = "dc1_voltage"
CONF_ALDO1_VOLTAGE = "aldo1_voltage"
CONF_ALDO2_VOLTAGE = "aldo2_voltage"
CONF_ALDO3_VOLTAGE = "aldo3_voltage"
CONF_ALDO4_VOLTAGE = "aldo4_voltage"
CONF_BLDO1_VOLTAGE = "bldo1_voltage"
CONF_BLDO2_VOLTAGE = "bldo2_voltage"

# Sensor configs
CONF_BATTERY_VOLTAGE = "battery_voltage"
CONF_BATTERY_LEVEL = "battery_level"
CONF_VBUS_VOLTAGE = "vbus_voltage"

axp2101_ns = cg.esphome_ns.namespace("axp2101")
AXP2101Component = axp2101_ns.class_("AXP2101Component", cg.Component, i2c.I2CDevice)

# Voltage validation (in millivolts internally)
def voltage_mv(value):
    """Convert voltage string like '3.3V' to millivolts."""
    if isinstance(value, str):
        value = value.strip().upper()
        if value.endswith("V"):
            value = value[:-1]
        value = float(value) * 1000
    return cv.int_range(min=500, max=5000)(int(value))

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(AXP2101Component),
            cv.Optional(CONF_IRQ_PIN): pins.gpio_input_pin_schema,
            cv.Optional(CONF_DC1_VOLTAGE): voltage_mv,
            cv.Optional(CONF_ALDO1_VOLTAGE): voltage_mv,
            cv.Optional(CONF_ALDO2_VOLTAGE): voltage_mv,
            cv.Optional(CONF_ALDO3_VOLTAGE): voltage_mv,
            cv.Optional(CONF_ALDO4_VOLTAGE): voltage_mv,
            cv.Optional(CONF_BLDO1_VOLTAGE): voltage_mv,
            cv.Optional(CONF_BLDO2_VOLTAGE): voltage_mv,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x34))
)

# Sensor platform schema
AXP2101_SENSOR_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_AXP2101_ID): cv.use_id(AXP2101Component),
        cv.Optional(CONF_BATTERY_VOLTAGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_BATTERY_LEVEL): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_BATTERY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_VBUS_VOLTAGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    if CONF_IRQ_PIN in config:
        irq_pin = await cg.gpio_pin_expression(config[CONF_IRQ_PIN])
        cg.add(var.set_irq_pin(irq_pin))

    if CONF_DC1_VOLTAGE in config:
        cg.add(var.set_dc1_voltage(config[CONF_DC1_VOLTAGE]))
    if CONF_ALDO1_VOLTAGE in config:
        cg.add(var.set_aldo1_voltage(config[CONF_ALDO1_VOLTAGE]))
    if CONF_ALDO2_VOLTAGE in config:
        cg.add(var.set_aldo2_voltage(config[CONF_ALDO2_VOLTAGE]))
    if CONF_ALDO3_VOLTAGE in config:
        cg.add(var.set_aldo3_voltage(config[CONF_ALDO3_VOLTAGE]))
    if CONF_ALDO4_VOLTAGE in config:
        cg.add(var.set_aldo4_voltage(config[CONF_ALDO4_VOLTAGE]))
    if CONF_BLDO1_VOLTAGE in config:
        cg.add(var.set_bldo1_voltage(config[CONF_BLDO1_VOLTAGE]))
    if CONF_BLDO2_VOLTAGE in config:
        cg.add(var.set_bldo2_voltage(config[CONF_BLDO2_VOLTAGE]))
