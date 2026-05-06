#pragma once

#include "esphome/components/light/light_effect.h"
#include "esphome/components/light/light_state.h"
#include "stairs_light_output.h"

namespace esphome {
namespace stairs_light {

/// Base class for stairs light effects.
class StairsLightEffect : public light::LightEffect {
 public:
  explicit StairsLightEffect(const char *name) : LightEffect(name) {}

  void start() override {
    auto *output = this->get_stairs_output_();
    if (output != nullptr) {
      output->set_effect_active(true);
    }
  }

  void stop() override {
    auto *output = this->get_stairs_output_();
    if (output != nullptr) {
      output->set_effect_active(false);
    }
  }

 protected:
  StairsLightOutput *get_stairs_output_() {
    return static_cast<StairsLightOutput *>(this->state_->get_output());
  }

  /// Get the target brightness from the light state (gamma-corrected).
  float get_target_brightness_() {
    float brightness;
    this->state_->current_values_as_brightness(&brightness);
    return brightness;
  }

  /// Compute a linear fade value given elapsed time within a fade window.
  static float fade_value(float target, uint32_t elapsed_in_fade, uint32_t fade_duration) {
    if (fade_duration == 0)
      return target;
    float progress = static_cast<float>(elapsed_in_fade) / static_cast<float>(fade_duration);
    if (progress > 1.0f)
      progress = 1.0f;
    return target * progress;
  }
};

enum class WaveDirection : uint8_t {
  UP,           // bottom (last step) to top (first step)
  DOWN,         // top (first step) to bottom (last step)
  TO_MIDDLE,    // both ends converge to center simultaneously
  FROM_MIDDLE,  // center spreads to both ends simultaneously
};

/// Wave effect: steps light up sequentially, hold, then turn off sequentially.
class StairsWaveEffect : public StairsLightEffect {
 public:
  explicit StairsWaveEffect(const char *name) : StairsLightEffect(name) {}

  void set_direction(WaveDirection direction) { this->direction_ = direction; }
  void set_step_delay(uint32_t delay) { this->step_delay_ = delay; }
  void set_fade_duration(uint32_t duration) { this->fade_duration_ = duration; }
  void set_auto_off_delay(uint32_t delay) { this->auto_off_delay_ = delay; }
  void set_max_brightness(float max_brightness) { this->max_brightness_ = max_brightness; }

  void start() override {
    StairsLightEffect::start();
    this->start_time_ = millis();
    this->completed_ = false;
  }

  void apply() override {
    if (this->completed_)
      return;

    auto *output = this->get_stairs_output_();
    if (output == nullptr)
      return;

    const uint8_t num_steps = output->get_num_steps();
    if (num_steps == 0)
      return;

    const uint32_t now = millis();
    const uint32_t elapsed = now - this->start_time_;
    const float target = this->get_target_brightness_() * this->max_brightness_;
    const uint8_t max_seq = this->get_max_seq_(num_steps);

    // Calculate phase boundaries based on effective sequence length
    const uint32_t wave_on_end =
        static_cast<uint32_t>(max_seq) * this->step_delay_ + this->fade_duration_;
    const uint32_t hold_end = wave_on_end + this->auto_off_delay_;
    const uint32_t wave_off_end =
        hold_end + static_cast<uint32_t>(max_seq) * this->step_delay_ + this->fade_duration_;

    for (uint8_t i = 0; i < num_steps; i++) {
      uint8_t seq = this->get_seq_index_(i, num_steps);
      float level = this->compute_step_brightness_(seq, elapsed, target, wave_on_end, hold_end);
      output->set_step_level(i, level);
    }

    // Check if the full cycle has completed
    if (this->auto_off_delay_ > 0 && elapsed >= wave_off_end) {
      this->completed_ = true;
      auto call = this->state_->turn_off();
      call.set_transition_length(0);
      call.perform();
    }
  }

 protected:
  WaveDirection direction_{WaveDirection::DOWN};
  uint32_t step_delay_{150};
  uint32_t fade_duration_{500};
  uint32_t auto_off_delay_{5000};
  float max_brightness_{1.0f};
  uint32_t start_time_{0};
  bool completed_{false};

  float compute_step_brightness_(uint8_t seq_index, uint32_t elapsed,
                                  float target, uint32_t wave_on_end, uint32_t hold_end) {
    // Phase 1: Wave On
    uint32_t on_start = static_cast<uint32_t>(seq_index) * this->step_delay_;
    uint32_t on_end = on_start + this->fade_duration_;

    if (elapsed < on_start) {
      return 0.0f;
    }
    if (elapsed < on_end) {
      return fade_value(target, elapsed - on_start, this->fade_duration_);
    }

    // Phase 2: Hold (or stay lit if no auto-off)
    if (this->auto_off_delay_ == 0 || elapsed < hold_end) {
      return target;
    }

    // Phase 3: Wave Off
    uint32_t off_start = hold_end + static_cast<uint32_t>(seq_index) * this->step_delay_;
    uint32_t off_end = off_start + this->fade_duration_;

    if (elapsed < off_start) {
      return target;
    }
    if (elapsed < off_end) {
      return target * (1.0f - static_cast<float>(elapsed - off_start) / static_cast<float>(this->fade_duration_));
    }

    return 0.0f;
  }

  /// Maximum sequence index — determines total wave duration.
  uint8_t get_max_seq_(uint8_t num_steps) const {
    if (this->direction_ == WaveDirection::TO_MIDDLE || this->direction_ == WaveDirection::FROM_MIDDLE) {
      return (num_steps - 1) / 2;
    }
    return num_steps - 1;
  }

  /// Map physical step index to timing sequence index.
  uint8_t get_seq_index_(uint8_t step, uint8_t num_steps) const {
    uint8_t mirror = num_steps - 1 - step;
    uint8_t dist_from_near_end = step < mirror ? step : mirror;
    switch (this->direction_) {
      case WaveDirection::DOWN:
        return step;
      case WaveDirection::UP:
        return mirror;
      case WaveDirection::TO_MIDDLE:
        return dist_from_near_end;
      case WaveDirection::FROM_MIDDLE:
        return (num_steps - 1) / 2 - dist_from_near_end;
      default:
        return step;
    }
  }
};


/// Pulse effect: all steps pulse simultaneously between min and max brightness.
class StairsPulseEffect : public StairsLightEffect {
 public:
  explicit StairsPulseEffect(const char *name) : StairsLightEffect(name) {}

  void set_transition_on_length(uint32_t length) { this->transition_on_length_ = length; }
  void set_transition_off_length(uint32_t length) { this->transition_off_length_ = length; }
  void set_update_interval(uint32_t interval) { this->update_interval_ = interval; }
  void set_min_max_brightness(float min, float max) {
    this->min_brightness_ = min;
    this->max_brightness_ = max;
  }

  void start() override {
    StairsLightEffect::start();
    this->last_transition_ = millis();
    this->rising_ = true;
  }

  void apply() override {
    auto *output = this->get_stairs_output_();
    if (output == nullptr)
      return;

    const uint32_t now = millis();
    const uint32_t elapsed = now - this->last_transition_;
    const uint32_t current_transition = this->rising_ ? this->transition_on_length_ : this->transition_off_length_;
    const uint32_t cycle_time = current_transition + this->update_interval_;

    if (elapsed >= cycle_time) {
      this->rising_ = !this->rising_;
      this->last_transition_ = now;
      float level = this->rising_ ? this->min_brightness_ : this->max_brightness_;
      output->set_all_levels(level);
      return;
    }

    float level;
    if (elapsed < current_transition) {
      float progress = static_cast<float>(elapsed) / static_cast<float>(current_transition);
      if (this->rising_) {
        level = this->min_brightness_ + (this->max_brightness_ - this->min_brightness_) * progress;
      } else {
        level = this->max_brightness_ - (this->max_brightness_ - this->min_brightness_) * progress;
      }
    } else {
      // In the hold period after transition
      level = this->rising_ ? this->max_brightness_ : this->min_brightness_;
    }

    output->set_all_levels(level);
  }

 protected:
  uint32_t transition_on_length_{1000};
  uint32_t transition_off_length_{1000};
  uint32_t update_interval_{1000};
  float min_brightness_{0.0f};
  float max_brightness_{1.0f};
  uint32_t last_transition_{0};
  bool rising_{true};
};

/// Shimmer effect: all steps lit at base brightness with a small brighter
/// highlight that bounces continuously up and down.
class StairsShimmerEffect : public StairsLightEffect {
 public:
  explicit StairsShimmerEffect(const char *name) : StairsLightEffect(name) {}

  void set_base_brightness(float base) { this->base_brightness_ = base; }
  void set_highlight_brightness(float highlight) { this->highlight_brightness_ = highlight; }
  void set_wave_width(uint8_t width) { this->wave_width_ = width; }
  void set_speed(uint32_t ms_per_step) { this->speed_ = ms_per_step; }

  void start() override {
    StairsLightEffect::start();
    this->start_time_ = millis();
  }

  void apply() override {
    auto *output = this->get_stairs_output_();
    if (output == nullptr)
      return;

    const uint8_t num_steps = output->get_num_steps();
    if (num_steps < 2)
      return;

    // Compute floating-point position that bounces between 0 and num_steps-1
    const uint32_t elapsed = millis() - this->start_time_;
    const float total_travel = static_cast<float>(num_steps - 1);
    // Full bounce cycle = down + up
    const uint32_t cycle_ms = static_cast<uint32_t>(2.0f * total_travel) * this->speed_;
    const uint32_t phase_ms = elapsed % cycle_ms;
    const float phase = static_cast<float>(phase_ms) / static_cast<float>(this->speed_);

    // Triangle wave: move down then back up
    float position;
    if (phase <= total_travel) {
      position = phase;
    } else {
      position = 2.0f * total_travel - phase;
    }

    const float half_width = static_cast<float>(this->wave_width_) * 0.5f;
    const float extra = this->highlight_brightness_ - this->base_brightness_;

    for (uint8_t i = 0; i < num_steps; i++) {
      float dist = static_cast<float>(i) - position;
      if (dist < 0.0f)
        dist = -dist;

      float level;
      if (dist >= half_width) {
        level = this->base_brightness_;
      } else {
        // Smooth cosine falloff: 1.0 at center, 0.0 at edge
        float t = dist / half_width;  // 0..1
        float factor = (1.0f - t) * (1.0f - t);  // quadratic falloff
        level = this->base_brightness_ + extra * factor;
      }
      output->set_step_level(i, level);
    }
  }

 protected:
  float base_brightness_{0.6f};
  float highlight_brightness_{1.0f};
  uint8_t wave_width_{3};
  uint32_t speed_{300};  // ms per step movement
  uint32_t start_time_{0};
};

}  // namespace stairs_light
}  // namespace esphome
