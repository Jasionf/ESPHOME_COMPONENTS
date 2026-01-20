#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"

static const uint8_t SI5351_I2C_ADDR = 0x60;
  // Register addresses
  static const uint8_t REG_OUTPUT_ENABLE_CONTROL = 3;
  static const uint8_t REG_OEB_PIN_ENABLE_CONTROL = 9;
  static const uint8_t REG_OUTPUT_DRIVERS = 16;
  static const uint8_t REG_CLK1_CONTROL = 17;
  static const uint8_t REG_XTAL_LOAD_CAP = 183;
  static const uint8_t REG_MULTISYNTH_NA = 26;
  static const uint8_t REG_MULTISYNTH1 = 50;
  static const uint8_t REG_PLL_RESET = 177;

  
namespace esphome {
namespace si5351 {

class Si5351Component : public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::PRE_HARDWARE; }
  

};

}  // namespace si5351
}  // namespace esphome
