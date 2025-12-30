import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import i2c
from esphome.const import (
    CONF_ID,
    CONF_ADDRESS,
)

DEPENDENCIES = ["i2c"]
CODEOWNERS = ["@medallion"]

CONF_PA_ENABLE_PIN = "pa_enable_pin"
CONF_I2S_MCLK_PIN = "i2s_mclk_pin"
CONF_I2S_BCLK_PIN = "i2s_bclk_pin"
CONF_I2S_WS_PIN = "i2s_ws_pin"
CONF_I2S_DOUT_PIN = "i2s_dout_pin"
CONF_I2S_DIN_PIN = "i2s_din_pin"
CONF_SAMPLE_RATE = "sample_rate"
CONF_BITS_PER_SAMPLE = "bits_per_sample"
CONF_MIC_GAIN = "mic_gain"

es8311_ns = cg.esphome_ns.namespace("es8311")
ES8311Component = es8311_ns.class_("ES8311Component", cg.Component, i2c.I2CDevice)

MIC_GAIN_OPTIONS = {
    "0dB": 0,
    "6dB": 1,
    "12dB": 2,
    "18dB": 3,
    "24dB": 4,
    "30dB": 5,
    "36dB": 6,
    "42dB": 7,
}

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(ES8311Component),
            cv.Optional(CONF_PA_ENABLE_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_I2S_MCLK_PIN): cv.int_range(min=0, max=48),
            cv.Required(CONF_I2S_BCLK_PIN): cv.int_range(min=0, max=48),
            cv.Required(CONF_I2S_WS_PIN): cv.int_range(min=0, max=48),
            cv.Required(CONF_I2S_DOUT_PIN): cv.int_range(min=0, max=48),
            cv.Required(CONF_I2S_DIN_PIN): cv.int_range(min=0, max=48),
            cv.Optional(CONF_SAMPLE_RATE, default=16000): cv.int_range(min=8000, max=48000),
            cv.Optional(CONF_BITS_PER_SAMPLE, default=16): cv.one_of(16, 24, 32, int=True),
            cv.Optional(CONF_MIC_GAIN, default="30dB"): cv.enum(MIC_GAIN_OPTIONS, upper=False),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x18))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    if CONF_PA_ENABLE_PIN in config:
        pa_pin = await cg.gpio_pin_expression(config[CONF_PA_ENABLE_PIN])
        cg.add(var.set_pa_enable_pin(pa_pin))

    cg.add(var.set_i2s_mclk_pin(config[CONF_I2S_MCLK_PIN]))
    cg.add(var.set_i2s_bclk_pin(config[CONF_I2S_BCLK_PIN]))
    cg.add(var.set_i2s_ws_pin(config[CONF_I2S_WS_PIN]))
    cg.add(var.set_i2s_dout_pin(config[CONF_I2S_DOUT_PIN]))
    cg.add(var.set_i2s_din_pin(config[CONF_I2S_DIN_PIN]))
    cg.add(var.set_sample_rate(config[CONF_SAMPLE_RATE]))
    cg.add(var.set_bits_per_sample(config[CONF_BITS_PER_SAMPLE]))
    cg.add(var.set_mic_gain(config[CONF_MIC_GAIN]))
