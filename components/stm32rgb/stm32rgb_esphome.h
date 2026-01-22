#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/core/gpio.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace stm32rgb {


typedef enum Channel {
    CHANNEL_B = 0,
    CHANNEL_G = 1,
    CHANNEL_R = 2,
    CHANNEL_W = 3
} LED_Channel_t;

static const uint8_t STM32_I2C_ADDR              = 0x1A;
static const uint8_t RGB1_BRIGHTNESS_REG_ADDR    = 0x10  // 灯带1亮度
static const uint8_t RGB2_BRIGHTNESS_REG_ADDR    = 0x11  // 灯带2亮度
static const uint8_t RGB_CH1_I1_COLOR_REG_ADDR   = 0x20  // Channel 0, 灯带1组1, LED 0, 每个LED 4字节
static const uint8_t RGB_CH2_I1_COLOR_REG_ADDR   = 0x3C  // Channel 1, 灯带1组2, LED 0, 每个LED 4字节
static const uint8_t RGB_CH3_I1_COLOR_REG_ADDR   = 0x60  // Channel 2, 灯带2组1, LED 0, 每个LED 4字节
static const uint8_t RGB_CH4_I1_COLOR_REG_ADDR   = 0x7C  // Channel 3, 灯带2组2, LED 0, 每个LED 4字节

static const uint8_t NUM_RGB_CHANNELS            = 4     // 4个通道（2条灯带 × 2组）
static const uint8_t NUM_LEDS_PER_GROUP          = 7     // 每组7个LED


class STM32RGBLight : public light::AddressableLight, public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return esphome::setup_priority::HARDWARE; }
  bool setup_complete_{false};

  light::LightTraits get_traits() override;

  /// ESPHome 会调用此方法来更新单个像素
  light::ESPColorView operator[](int32_t index) const override;

  int32_t size() const override { return this->num_leds_; }

  void clear_effect_data() override;

  /// 在 write_state() 后调用，真正发送数据给 STM32
  void show() override;

  void set_num_leds(uint16_t num_leds) { this->num_leds_ = num_leds; }
  void set_strip(uint8_t strip) { this->strip_index_ = strip; }  // 0 或 1
  void set_pixel_order(PixelOrder order);

 protected:
  uint16_t num_leds_{0};
  uint8_t strip_index_{0};               // 0 = strip1, 1 = strip2
  PixelOrder pixel_order_{PixelOrder::GRB};

  std::vector<Color> pixels_;            // 颜色缓存
  std::vector<uint8_t> effect_data_;     // 效果数据

  uint8_t rgb_offsets_[4]{0, 1, 2, 3};   // R,G,B,W 偏移

  bool write_brightness(uint8_t brightness);
  bool send_pixel_data();
  uint8_t get_channel_reg(uint8_t group) const;  // group 0 or 1 for the strip
};


}  // namespace stm32rgb
}  // namespace esphome
