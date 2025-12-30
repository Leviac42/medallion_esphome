import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins, automation
from esphome.components import spi
from esphome.const import CONF_ID

DEPENDENCIES = ["es8311"]
CODEOWNERS = ["@medallion"]

CONF_AUDIO_CODEC_ID = "audio_codec_id"
CONF_SD_CS_PIN = "sd_cs_pin"
CONF_SD_SPI_ID = "sd_spi_id"
CONF_UPLOAD_URL = "upload_url"

medallion_voice_ns = cg.esphome_ns.namespace("medallion_voice")
MedallionVoiceComponent = medallion_voice_ns.class_("MedallionVoiceComponent", cg.Component)

# Import ES8311 component
es8311_ns = cg.esphome_ns.namespace("es8311")
ES8311Component = es8311_ns.class_("ES8311Component")

# Actions
StartRecordingAction = medallion_voice_ns.class_("StartRecordingAction", automation.Action)
StopRecordingAction = medallion_voice_ns.class_("StopRecordingAction", automation.Action)
UploadAction = medallion_voice_ns.class_("UploadAction", automation.Action)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(MedallionVoiceComponent),
        cv.Required(CONF_AUDIO_CODEC_ID): cv.use_id(ES8311Component),
        cv.Required(CONF_SD_CS_PIN): pins.gpio_output_pin_schema,
        cv.Optional(CONF_SD_SPI_ID): cv.use_id(spi.SPIComponent),
        cv.Required(CONF_UPLOAD_URL): cv.string,
    }
).extend(cv.COMPONENT_SCHEMA)

# Action schemas
START_RECORDING_ACTION_SCHEMA = automation.maybe_simple_id(
    {
        cv.GenerateID(): cv.use_id(MedallionVoiceComponent),
    }
)

STOP_RECORDING_ACTION_SCHEMA = automation.maybe_simple_id(
    {
        cv.GenerateID(): cv.use_id(MedallionVoiceComponent),
    }
)

UPLOAD_ACTION_SCHEMA = automation.maybe_simple_id(
    {
        cv.GenerateID(): cv.use_id(MedallionVoiceComponent),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # Link audio codec
    audio_codec = await cg.get_variable(config[CONF_AUDIO_CODEC_ID])
    cg.add(var.set_audio_codec(audio_codec))

    # SD card CS pin
    cs_pin = await cg.gpio_pin_expression(config[CONF_SD_CS_PIN])
    cg.add(var.set_sd_cs_pin(cs_pin))

    # Upload URL
    cg.add(var.set_upload_url(config[CONF_UPLOAD_URL]))

    # Add SdFat library
    cg.add_library("greiman/SdFat", "2.2.2")


@automation.register_action(
    "medallion_voice.start_recording", StartRecordingAction, START_RECORDING_ACTION_SCHEMA
)
async def start_recording_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


@automation.register_action(
    "medallion_voice.stop_recording", StopRecordingAction, STOP_RECORDING_ACTION_SCHEMA
)
async def stop_recording_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


@automation.register_action(
    "medallion_voice.upload", UploadAction, UPLOAD_ACTION_SCHEMA
)
async def upload_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var
