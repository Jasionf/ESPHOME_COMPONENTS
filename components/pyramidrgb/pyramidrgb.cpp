#include "pyramidrgb.h"

namespace esphome {
namespace pyramidrgb {


#define pyramidrgb_ERROR_FAILED(func) \
  if(!(func)) {                                 \
 this->mark_failed(); \
    return; \
  } 

// Return false; use outside of setup
#define pyramidrgb_ERROR_CHECK(func) \
  if (!(func)) { \
    return false; \
  }


static const char *const TAG = "pyramidrgb";

void STM32RGBLight::setup() {

uint8_t echo_write(uint8_t reg, const uint8_t *data, size_t len){
  pyramidrgb_ERROR_FAILED(this->write_byte(reg, data, len));
}

//设置亮度
void echo_pyramid_rgb_set_brightness(int rgb_index, uint8_t brightness)
{
    if (rgb_index != 1 && rgb_index != 2) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t reg = (rgb_index == 1) ? REG_RGB1_BRIGHTNESS : REG_RGB2_BRIGHTNESS;
    uint8_t b = clamp_brightness(brightness);
    return echo_write(reg, &b, 1);//写入寄存器
}


}  // namespace pyramidrgb
}  // namespace esphome