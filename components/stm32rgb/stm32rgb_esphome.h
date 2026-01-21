#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/core/gpio.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace stm32rgb {


static const uint8_t STM32_I2C_ADDR              = 0x1A;
static const uint8_t RGB1_BRIGHTNESS_REG_ADDR    = 0x10  // 灯带1亮度
static const uint8_t RGB2_BRIGHTNESS_REG_ADDR    = 0x11  // 灯带2亮度
static const uint8_t RGB_CH1_I1_COLOR_REG_ADDR   = 0x20  // Channel 0, 灯带1组1, LED 0, 每个LED 4字节
static const uint8_t RGB_CH2_I1_COLOR_REG_ADDR   = 0x3C  // Channel 1, 灯带1组2, LED 0, 每个LED 4字节
static const uint8_t RGB_CH3_I1_COLOR_REG_ADDR   = 0x60  // Channel 2, 灯带2组1, LED 0, 每个LED 4字节
static const uint8_t RGB_CH4_I1_COLOR_REG_ADDR   = 0x7C  // Channel 3, 灯带2组2, LED 0, 每个LED 4字节

static const uint8_t NUM_RGB_CHANNELS            = 4     // 4个通道（2条灯带 × 2组）
static const uint8_t NUM_LEDS_PER_GROUP          = 7     // 每组7个LED


class STM32RGBLight : public light::LightOutput, public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return esphome::setup_priority::HARDWARE; }
  bool setup_complete_{false};

  void write_state(light::LightState *state) override;

  // 用于 yaml 配置多个独立灯带（strip）
  void set_strip(uint8_t strip) { this->strip_ = strip; }

  protected:
  uint8_t strip_{0};  // 0 = 第一条灯带，1 = 第二条灯带

  bool write_brightness(uint8_t brightness);
  bool write_leds(const uint8_t *colors_buffer, size_t len);

  uint8_t get_channel_base_addr(uint8_t logical_channel) const;
  uint8_t remap_led_index(uint8_t logical_channel, uint8_t index) const;
};


}  // namespace stm32rgb
}  // namespace esphome
