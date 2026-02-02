#pragma once

#include "esphome/components/output/float_output.h"
#include "../unit_step16_sensor.h"

namespace esphome {
namespace unit_step16 {

// Output channel enumeration
enum OutputChannel {
  LED_BRIGHTNESS = 0,
  RGB_BRIGHTNESS = 1,
  RGB_RED = 2,
  RGB_GREEN = 3,
  RGB_BLUE = 4,
};

class UnitStep16Output : public output::FloatOutput, public Component {
 public:
  void set_parent(UnitStep16Component *parent) { parent_ = parent; }
  void set_channel(OutputChannel channel) { channel_ = channel; }

  void write_state(float state) override {
    if (parent_ == nullptr) {
      return;
    }

    // Convert 0.0-1.0 to appropriate range
    switch (channel_) {
      case LED_BRIGHTNESS:
      case RGB_BRIGHTNESS: {
        // Convert to 0-100
        uint8_t brightness = static_cast<uint8_t>(state * 100.0f);
        if (channel_ == LED_BRIGHTNESS) {
          parent_->set_led_brightness(brightness);
        } else {
          parent_->set_rgb_brightness(brightness);
        }
        break;
      }
      case RGB_RED:
      case RGB_GREEN:
      case RGB_BLUE: {
        // Convert to 0-255 and update the corresponding color component
        uint8_t value = static_cast<uint8_t>(state * 255.0f);
        
        // Get current RGB values
        uint8_t r, g, b;
        if (!parent_->get_rgb(&r, &g, &b)) {
          r = g = b = 0;
        }
        
        // Update the specific channel
        if (channel_ == RGB_RED) {
          r = value;
        } else if (channel_ == RGB_GREEN) {
          g = value;
        } else if (channel_ == RGB_BLUE) {
          b = value;
        }
        
        // Write back all three values
        parent_->set_rgb(r, g, b);
        break;
      }
    }
  }

 protected:
  UnitStep16Component *parent_{nullptr};
  OutputChannel channel_{LED_BRIGHTNESS};
};

}  // namespace unit_step16
}  // namespace esphome
