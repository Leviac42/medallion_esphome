import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.const import (
    CONF_ID,
    CONF_WIDTH,
    CONF_HEIGHT,
    CONF_RESET_PIN,
    CONF_CS_PIN,
    CONF_BRIGHTNESS,
)

CODEOWNERS = ["@medallion"]

CONF_SCLK_PIN = "sclk_pin"
CONF_DATA0_PIN = "data0_pin"
CONF_DATA1_PIN = "data1_pin"
CONF_DATA2_PIN = "data2_pin"
CONF_DATA3_PIN = "data3_pin"

co5300_qspi_ns = cg.esphome_ns.namespace("co5300_qspi")
CO5300QSPIComponent = co5300_qspi_ns.class_("CO5300QSPIComponent", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(CO5300QSPIComponent),
        cv.Required(CONF_CS_PIN): pins.gpio_output_pin_schema,
        cv.Required(CONF_SCLK_PIN): pins.gpio_output_pin_schema,
        cv.Required(CONF_DATA0_PIN): pins.gpio_output_pin_schema,
        cv.Required(CONF_DATA1_PIN): pins.gpio_output_pin_schema,
        cv.Required(CONF_DATA2_PIN): pins.gpio_output_pin_schema,
        cv.Required(CONF_DATA3_PIN): pins.gpio_output_pin_schema,
        cv.Required(CONF_RESET_PIN): pins.gpio_output_pin_schema,
        cv.Optional(CONF_WIDTH, default=466): cv.int_range(min=1, max=1024),
        cv.Optional(CONF_HEIGHT, default=466): cv.int_range(min=1, max=1024),
        cv.Optional(CONF_BRIGHTNESS, default=255): cv.int_range(min=0, max=255),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cs_pin = await cg.gpio_pin_expression(config[CONF_CS_PIN])
    cg.add(var.set_cs_pin(cs_pin))

    sclk_pin = await cg.gpio_pin_expression(config[CONF_SCLK_PIN])
    cg.add(var.set_sclk_pin(sclk_pin))

    data0_pin = await cg.gpio_pin_expression(config[CONF_DATA0_PIN])
    cg.add(var.set_data0_pin(data0_pin))

    data1_pin = await cg.gpio_pin_expression(config[CONF_DATA1_PIN])
    cg.add(var.set_data1_pin(data1_pin))

    data2_pin = await cg.gpio_pin_expression(config[CONF_DATA2_PIN])
    cg.add(var.set_data2_pin(data2_pin))

    data3_pin = await cg.gpio_pin_expression(config[CONF_DATA3_PIN])
    cg.add(var.set_data3_pin(data3_pin))

    reset_pin = await cg.gpio_pin_expression(config[CONF_RESET_PIN])
    cg.add(var.set_reset_pin(reset_pin))

    cg.add(var.set_width(config[CONF_WIDTH]))
    cg.add(var.set_height(config[CONF_HEIGHT]))
    cg.add(var.set_brightness(config[CONF_BRIGHTNESS]))

    # Add required Arduino GFX library includes
    cg.add_library("moononournation/GFX Library for Arduino", "1.4.9")
