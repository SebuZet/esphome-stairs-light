# ESPHome Stairs Light Component

An ESPHome external component that drives up to 20 individually controllable stair step lights as a single Light entity in Home Assistant.

## Features

- **Up to 20 steps** using any ESPHome `FloatOutput` (LEDC PWM, PCA9685, MCP4728, etc.)
- **Exposed as a Home Assistant Light entity** with on/off and brightness control
- **6 custom effects:**
  - `stairs_wave_up` — sequential bottom-to-top wave on, hold, wave off
  - `stairs_wave_down` — sequential top-to-bottom wave on, hold, wave off
  - `stairs_wave_to_middle` — both ends light up simultaneously, converging to center
  - `stairs_wave_from_middle` — center lights up first, spreading to both ends
  - `stairs_pulse` — all steps pulse simultaneously between min/max brightness
  - `stairs_shimmer` — bouncing highlight that moves up and down over lit stairs
- **Brightness scaling** via `max_power` — all output is scaled to a configurable maximum (default 30%)
- **Standard monochromatic effects** (pulse, flicker, strobe, lambda, etc.) also available
- **Configurable timing** per-effect: step delay, fade duration, auto-off delay

## Installation

Add to your ESPHome YAML:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/SebuZet/esphome-stairs-light
    components: [stairs_light]
```

Or use a local path during development:

```yaml
external_components:
  - source:
      type: local
      path: /path/to/esphome-stairs-light/components
    components: [stairs_light]
```

## Configuration

```yaml
output:
  - platform: ledc
    pin: GPIO16
    id: step_1
  - platform: ledc
    pin: GPIO17
    id: step_2
  - platform: ledc
    pin: GPIO18
    id: step_3
  - platform: ledc
    pin: GPIO19
    id: step_4
  - platform: ledc
    pin: GPIO21
    id: step_5

light:
  - platform: stairs_light
    name: "Staircase"
    max_power: 30%
    steps:
      - step_1
      - step_2
      - step_3
      - step_4
      - step_5
    effects:
      - stairs_wave_up:
          name: "Wave Up"
          step_delay: 200ms
          fade_duration: 500ms
          auto_off_delay: 5s
      - stairs_wave_down:
          name: "Wave Down"
      - stairs_wave_to_middle:
          name: "Wave To Middle"
      - stairs_wave_from_middle:
          name: "Wave From Middle"
      - stairs_shimmer:
          name: "Shimmer"
          base_brightness: 60%
          highlight_brightness: 100%
          wave_width: 3
          speed: 300ms
      - stairs_pulse:
          name: "Stair Pulse"
          transition_length: 1s
          min_brightness: 10%
          max_brightness: 100%
      - pulse:
          name: "Standard Pulse"
```

### Configuration Variables

#### Platform (`stairs_light`)

| Variable | Required | Default | Description |
|----------|----------|---------|-------------|
| `name` | Yes | — | Name of the light entity |
| `steps` | Yes | — | List of output IDs (1–20 FloatOutput references) |
| `max_power` | No | `0.3` (30%) | All output brightness is scaled by this factor. A slider at 100% produces `max_power` output; 50% slider produces half of `max_power`. |
| `effects` | No | — | List of effects (see below) |

All standard [ESPHome light options](https://esphome.io/components/light/#base-light-configuration) are supported (`gamma_correct`, `default_transition_length`, `restore_mode`, etc.)

#### Effect: `stairs_wave_up` / `stairs_wave_down` / `stairs_wave_to_middle` / `stairs_wave_from_middle`

All wave effects share the same parameters and 3-phase behavior (wave on → hold → wave off). They differ only in direction:

| Effect | Direction |
|--------|-----------|
| `stairs_wave_up` | Bottom → Top |
| `stairs_wave_down` | Top → Bottom |
| `stairs_wave_to_middle` | Both ends → Center (simultaneously) |
| `stairs_wave_from_middle` | Center → Both ends (simultaneously) |

| Variable | Default | Description |
|----------|---------|-------------|
| `name` | Effect-specific | Effect display name |
| `step_delay` | `150ms` | Delay between each step starting to fade |
| `fade_duration` | `500ms` | How long each step takes to fade in/out |
| `auto_off_delay` | `5s` | Hold time before wave-off phase. Set to `0s` to stay lit (manual off) |
| `max_brightness` | `100%` | Scale the wave brightness range. The wave animates between 0 and `max_brightness` × current brightness. At 100% the full output range (up to `max_power`) is used. |

#### Effect: `stairs_pulse`

| Variable | Default | Description |
|----------|---------|-------------|
| `name` | "Pulse" | Effect display name |
| `transition_length` | `1s` | Fade duration (or use `on_length`/`off_length` for asymmetric) |
| `update_interval` | `1s` | Hold time at min/max before next transition |
| `min_brightness` | `0%` | Minimum brightness level |
| `max_brightness` | `100%` | Maximum brightness level |

#### Effect: `stairs_shimmer`

| Variable | Default | Description |
|----------|---------|-------------|
| `name` | "Shimmer" | Effect display name |
| `base_brightness` | `60%` | Background brightness for all steps |
| `highlight_brightness` | `100%` | Peak brightness of the moving highlight |
| `wave_width` | `3` | Number of steps the highlight spans (1–10) |
| `speed` | `300ms` | Time for the highlight to travel one step |

## How It Works

### Wave Effects

All 4 wave effects follow the same 3-phase pattern:

1. **Wave On** — Steps light up sequentially with a configurable delay and fade
2. **Hold** — All steps stay at target brightness for `auto_off_delay`
3. **Wave Off** — Steps turn off sequentially in the same order
4. **Auto-off** — The light entity turns off properly via Home Assistant

The `to_middle` and `from_middle` variants run both halves simultaneously, so the total animation takes half the time compared to `up`/`down` for the same number of steps.

Set `auto_off_delay: 0s` to skip the wave-off phase and keep steps lit until manually turned off.

### Pulse Effect

All steps simultaneously fade between `min_brightness` and `max_brightness` in a continuous cycle. Supports asymmetric transition times:

```yaml
- stairs_pulse:
    transition_length:
      on_length: 2s
      off_length: 500ms
```

### Shimmer Effect

All steps stay lit at `base_brightness` while a small highlight (`wave_width` steps wide) bounces continuously up and down the staircase. The highlight peaks at `highlight_brightness` with a smooth quadratic falloff, creating a subtle "breathing wave" over the lit stairs.

```yaml
- stairs_shimmer:
    base_brightness: 60%
    highlight_brightness: 100%
    wave_width: 3
    speed: 300ms
```

### Triggering via Automation

Use Home Assistant automations or ESPHome binary sensors (PIR) to trigger effects:

```yaml
binary_sensor:
  - platform: gpio
    pin: GPIO4
    name: "PIR Bottom"
    on_press:
      - light.turn_on:
          id: staircase
          brightness: 80%
          effect: "Wave Up"

  - platform: gpio
    pin: GPIO5
    name: "PIR Top"
    on_press:
      - light.turn_on:
          id: staircase
          brightness: 80%
          effect: "Wave Down"
```

## Output Types

Any `FloatOutput` works as a step. Common options:

- **LEDC** — built-in ESP32 PWM (up to 16 channels)
- **PCA9685** — I²C 16-channel PWM driver (chain for 20+ channels)
- **MCP4728** — I²C 4-channel DAC
- **GPIO with slow_pwm** — Software PWM for any GPIO

## License

MIT
