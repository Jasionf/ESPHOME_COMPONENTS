#pragma once

#include "esphome/components/output/float_output.h"
#include "../pyramidrgb.h"

namespace esphome {
namespace pyramidrgb {

static const char *TAG = "pyramidrgb.output";

class PyramidRGBOutput : public output::FloatOutput,
                         public Parented<PyramidRGBComponent>
{
public:
    void set_channel(LED_Channel_t channel) { channel_ = channel; }
    void write_state(float state) override {
        this->parent_->echo_pyramid_rgb_set_brightness(this->channel_, uint8_t val);

    }

protected:
    LED_Channel_t channel_;
};

} // namespace pyramidrgb
} // namespace esphome