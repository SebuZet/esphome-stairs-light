#pragma once

#include "esphome/core/component.h"
#include "esphome/components/output/float_output.h"
#include "esphome/components/light/light_output.h"

namespace esphome {
namespace stairs_light {

static const uint8_t MAX_STEPS = 20;

struct LightData {
  LightData() = default;
  LightData(output::FloatOutput *out_) {
    value_ = 0.0f;
    output_ = out_;
  }
  
  void set_level(float level_) {
    if (output_ && value_ != level_) {
      output_->set_level(value_ = level_);
    }
  }

  float value_{0.0f};
  output::FloatOutput *output_{nullptr};
};

class StairsLightOutput : public light::LightOutput {
 public:
  void add_step(output::FloatOutput *output) {
    if (this->num_steps_ < MAX_STEPS) {
      this->steps_[this->num_steps_++] = output;
    }
  }

  uint8_t get_num_steps() const { return this->num_steps_; }

  void set_max_brightness(float max_brightness) { this->max_brightness_ = max_brightness; }
  float get_max_brightness() const { return this->max_brightness_; }

  void set_step_level(uint8_t step, float level) {
    if (step < this->num_steps_) {
      this->steps_[step].set_level(this->scale_brightness_(level));
    }
  }

  void set_all_levels(float level) {
    float scaled = this->scale_brightness_(level);
    for (uint8_t i = 0; i < this->num_steps_; i++) {
      this->steps_[i].set_level(scaled);
    }
  }

  void set_effect_active(bool active) { this->effect_active_ = active; }
  bool is_effect_active() const { return this->effect_active_; }

  light::LightTraits get_traits() override {
    auto traits = light::LightTraits();
    traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS});
    return traits;
  }

  void write_state(light::LightState *state) override {
    if (this->effect_active_) {
      return;
    }
    float brightness;
    state->current_values_as_brightness(&brightness);
    this->set_all_levels(brightness);
  }

 protected:
  LightData steps_[MAX_STEPS]{};
  uint8_t num_steps_{0};
  float max_brightness_{0.3f};
  bool effect_active_{false};

  float scale_brightness_(float level) const {
    return level * this->max_brightness_;
  }
};

}  // namespace stairs_light
}  // namespace esphome
