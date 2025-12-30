import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import i2c
from esphome.const import (
    CONF_ID,
    CONF_ADDRESS,
    CONF_INTERRUPT_PIN,
    CONF_RESET_PIN,
    CONF_WIDTH,
    CONF_HEIGHT,
)

DEPENDENCIES = ["i2c"]
CODEOWNERS = ["@medallion"]

CONF_MIRROR_X = "mirror_x"
CONF_MIRROR_Y = "mirror_y"

cst92xx_ns = cg.esphome_ns.namespace("cst92xx")
CST92xxComponent = cst92xx_ns.class_("CST92xxComponent", cg.Component, i2c.I2CDevice)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(CST92xxComponent),
            cv.Optional(CONF_INTERRUPT_PIN): pins.gpio_input_pin_schema,
            cv.Optional(CONF_RESET_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_WIDTH, default=466): cv.int_range(min=1, max=1024),
            cv.Optional(CONF_HEIGHT, default=466): cv.int_range(min=1, max=1024),
            cv.Optional(CONF_MIRROR_X, default=True): cv.boolean,
            cv.Optional(CONF_MIRROR_Y, default=True): cv.boolean,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x5A))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    if CONF_INTERRUPT_PIN in config:
        int_pin = await cg.gpio_pin_expression(config[CONF_INTERRUPT_PIN])
        cg.add(var.set_interrupt_pin(int_pin))

    if CONF_RESET_PIN in config:
        rst_pin = await cg.gpio_pin_expression(config[CONF_RESET_PIN])
        cg.add(var.set_reset_pin(rst_pin))

    cg.add(var.set_width(config[CONF_WIDTH]))
    cg.add(var.set_height(config[CONF_HEIGHT]))
    cg.add(var.set_mirror_x(config[CONF_MIRROR_X]))
    cg.add(var.set_mirror_y(config[CONF_MIRROR_Y]))
