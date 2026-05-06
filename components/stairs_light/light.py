import esphome.codegen as cg
from esphome.components import light, output
from esphome.components.light.effects import (
    MONOCHROMATIC_EFFECTS,
    register_effect,
    validate_effects,
)
from esphome.components.light.types import LightEffect
import esphome.config_validation as cv
from esphome.const import (
    CONF_EFFECTS,
    CONF_MAX_BRIGHTNESS,
    CONF_MIN_BRIGHTNESS,
    CONF_NAME,
    CONF_OUTPUT_ID,
    CONF_TRANSITION_LENGTH,
    CONF_UPDATE_INTERVAL,
)

stairs_light_ns = cg.esphome_ns.namespace("stairs_light")
StairsLightOutput = stairs_light_ns.class_("StairsLightOutput", light.LightOutput)
StairsWaveEffect = stairs_light_ns.class_("StairsWaveEffect", LightEffect)
StairsPulseEffect = stairs_light_ns.class_("StairsPulseEffect", LightEffect)
StairsShimmerEffect = stairs_light_ns.class_("StairsShimmerEffect", LightEffect)
WaveDirection = stairs_light_ns.enum("WaveDirection", is_class=True)

CONF_STEPS = "steps"
CONF_STEP_DELAY = "step_delay"
CONF_FADE_DURATION = "fade_duration"
CONF_AUTO_OFF_DELAY = "auto_off_delay"
CONF_ON_LENGTH = "on_length"
CONF_OFF_LENGTH = "off_length"
CONF_MAX_POWER = "max_power"
CONF_BASE_BRIGHTNESS = "base_brightness"
CONF_HIGHLIGHT_BRIGHTNESS = "highlight_brightness"
CONF_WAVE_WIDTH = "wave_width"
CONF_SPEED = "speed"

# --- Custom effect registrations ---

STAIRS_WAVE_SCHEMA = {
    cv.Optional(
        CONF_STEP_DELAY, default="150ms"
    ): cv.positive_time_period_milliseconds,
    cv.Optional(
        CONF_FADE_DURATION, default="500ms"
    ): cv.positive_time_period_milliseconds,
    cv.Optional(
        CONF_AUTO_OFF_DELAY, default="5s"
    ): cv.positive_time_period_milliseconds,
    cv.Optional(CONF_MAX_BRIGHTNESS, default="100%"): cv.percentage,
}


@register_effect(
    "stairs_wave_up",
    StairsWaveEffect,
    "Wave Up",
    STAIRS_WAVE_SCHEMA,
)
async def stairs_wave_up_effect_to_code(config, effect_id):
    effect = cg.new_Pvariable(effect_id, config[CONF_NAME])
    cg.add(effect.set_direction(WaveDirection.UP))
    cg.add(effect.set_step_delay(config[CONF_STEP_DELAY]))
    cg.add(effect.set_fade_duration(config[CONF_FADE_DURATION]))
    cg.add(effect.set_auto_off_delay(config[CONF_AUTO_OFF_DELAY]))
    cg.add(effect.set_max_brightness(config[CONF_MAX_BRIGHTNESS]))
    return effect


@register_effect(
    "stairs_wave_down",
    StairsWaveEffect,
    "Wave Down",
    STAIRS_WAVE_SCHEMA,
)
async def stairs_wave_down_effect_to_code(config, effect_id):
    effect = cg.new_Pvariable(effect_id, config[CONF_NAME])
    cg.add(effect.set_direction(WaveDirection.DOWN))
    cg.add(effect.set_step_delay(config[CONF_STEP_DELAY]))
    cg.add(effect.set_fade_duration(config[CONF_FADE_DURATION]))
    cg.add(effect.set_auto_off_delay(config[CONF_AUTO_OFF_DELAY]))
    cg.add(effect.set_max_brightness(config[CONF_MAX_BRIGHTNESS]))
    return effect


@register_effect(
    "stairs_wave_to_middle",
    StairsWaveEffect,
    "Wave To Middle",
    STAIRS_WAVE_SCHEMA,
)
async def stairs_wave_to_middle_effect_to_code(config, effect_id):
    effect = cg.new_Pvariable(effect_id, config[CONF_NAME])
    cg.add(effect.set_direction(WaveDirection.TO_MIDDLE))
    cg.add(effect.set_step_delay(config[CONF_STEP_DELAY]))
    cg.add(effect.set_fade_duration(config[CONF_FADE_DURATION]))
    cg.add(effect.set_auto_off_delay(config[CONF_AUTO_OFF_DELAY]))
    cg.add(effect.set_max_brightness(config[CONF_MAX_BRIGHTNESS]))
    return effect


@register_effect(
    "stairs_wave_from_middle",
    StairsWaveEffect,
    "Wave From Middle",
    STAIRS_WAVE_SCHEMA,
)
async def stairs_wave_from_middle_effect_to_code(config, effect_id):
    effect = cg.new_Pvariable(effect_id, config[CONF_NAME])
    cg.add(effect.set_direction(WaveDirection.FROM_MIDDLE))
    cg.add(effect.set_step_delay(config[CONF_STEP_DELAY]))
    cg.add(effect.set_fade_duration(config[CONF_FADE_DURATION]))
    cg.add(effect.set_auto_off_delay(config[CONF_AUTO_OFF_DELAY]))
    cg.add(effect.set_max_brightness(config[CONF_MAX_BRIGHTNESS]))
    return effect


@register_effect(
    "stairs_pulse",
    StairsPulseEffect,
    "Pulse",
    {
        cv.Optional(CONF_TRANSITION_LENGTH, default="1s"): cv.Any(
            cv.positive_time_period_milliseconds,
            cv.Schema(
                {
                    cv.Required(CONF_ON_LENGTH): cv.positive_time_period_milliseconds,
                    cv.Required(CONF_OFF_LENGTH): cv.positive_time_period_milliseconds,
                }
            ),
        ),
        cv.Optional(
            CONF_UPDATE_INTERVAL, default="1s"
        ): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_MIN_BRIGHTNESS, default="0%"): cv.percentage,
        cv.Optional(CONF_MAX_BRIGHTNESS, default="100%"): cv.percentage,
    },
)
async def stairs_pulse_effect_to_code(config, effect_id):
    effect = cg.new_Pvariable(effect_id, config[CONF_NAME])
    if isinstance(config[CONF_TRANSITION_LENGTH], dict):
        cg.add(
            effect.set_transition_on_length(
                config[CONF_TRANSITION_LENGTH][CONF_ON_LENGTH]
            )
        )
        cg.add(
            effect.set_transition_off_length(
                config[CONF_TRANSITION_LENGTH][CONF_OFF_LENGTH]
            )
        )
    else:
        transition_length = config[CONF_TRANSITION_LENGTH]
        cg.add(effect.set_transition_on_length(transition_length))
        cg.add(effect.set_transition_off_length(transition_length))
    cg.add(effect.set_update_interval(config[CONF_UPDATE_INTERVAL]))
    cg.add(
        effect.set_min_max_brightness(
            config[CONF_MIN_BRIGHTNESS], config[CONF_MAX_BRIGHTNESS]
        )
    )
    return effect


@register_effect(
    "stairs_shimmer",
    StairsShimmerEffect,
    "Shimmer",
    {
        cv.Optional(CONF_BASE_BRIGHTNESS, default="60%"): cv.percentage,
        cv.Optional(CONF_HIGHLIGHT_BRIGHTNESS, default="100%"): cv.percentage,
        cv.Optional(CONF_WAVE_WIDTH, default=3): cv.int_range(min=1, max=10),
        cv.Optional(CONF_SPEED, default="300ms"): cv.positive_time_period_milliseconds,
    },
)
async def stairs_shimmer_effect_to_code(config, effect_id):
    effect = cg.new_Pvariable(effect_id, config[CONF_NAME])
    cg.add(effect.set_base_brightness(config[CONF_BASE_BRIGHTNESS]))
    cg.add(effect.set_highlight_brightness(config[CONF_HIGHLIGHT_BRIGHTNESS]))
    cg.add(effect.set_wave_width(config[CONF_WAVE_WIDTH]))
    cg.add(effect.set_speed(config[CONF_SPEED]))
    return effect


# Stairs lights support all standard monochromatic effects plus custom stairs effects.
STAIRS_EFFECTS = list(MONOCHROMATIC_EFFECTS) + [
    "stairs_wave_up",
    "stairs_wave_down",
    "stairs_wave_to_middle",
    "stairs_wave_from_middle",
    "stairs_pulse",
    "stairs_shimmer",
]

CONFIG_SCHEMA = light.BRIGHTNESS_ONLY_LIGHT_SCHEMA.extend(
    {
        cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(StairsLightOutput),
        cv.Required(CONF_STEPS): cv.All(
            cv.ensure_list(cv.use_id(output.FloatOutput)),
            cv.Length(min=1, max=20),
        ),
        cv.Optional(CONF_MAX_POWER, default=0.3): cv.percentage,
        cv.Optional(CONF_EFFECTS): validate_effects(STAIRS_EFFECTS),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await light.register_light(var, config)

    cg.add(var.set_max_brightness(config[CONF_MAX_POWER]))

    for step_id in config[CONF_STEPS]:
        step_output = await cg.get_variable(step_id)
        cg.add(var.add_step(step_output))
