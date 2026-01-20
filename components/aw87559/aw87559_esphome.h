#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/core/gpio.h"

namespace esphome {
namespace aw87559 {
// AW87559 Register Addresses
static const uint8_t AW87559_REG_CHIPID = 0x00;
static const uint8_t AW87559_REG_SYSCTRL = 0x01;
static const uint8_t AW87559_REG_BATSAFE = 0x02;
static const uint8_t AW87559_REG_BSTOVR = 0x03;
static const uint8_t AW87559_REG_BSTVPR = 0x04;
static const uint8_t AW87559_REG_PAGR = 0x05;
static const uint8_t AW87559_REG_PAGC3OPR = 0x06;
static const uint8_t AW87559_REG_PAGC3PR = 0x07;
static const uint8_t AW87559_REG_PAGC2OPR = 0x08;
static const uint8_t AW87559_REG_PAGC2PR = 0x09;
static const uint8_t AW87559_REG_PAGC1PR = 0x0A;

// static const uint8_t AW87559_REG_PAGC1PR = 0x0B;  
// static const uint8_t AW87559_REG_ADP_MODE = 0x0C;   
// static const uint8_t AW87559_REG_ADPBST_TIME1  = 0x0D;   
// static const uint8_t AW87559_REG_ADPBST_VTH  = 0x0F;       

static const uint8_t AW87559_CHIPID  = 0x5A;

/********************************************
 * Register Access
 *******************************************/
static const uint8_t AW87559_REG_MAX = 16;

static const uint8_t AW87559_REG_NONE_ACCESS = 0;
static const uint8_t AW87559_REG_RD_ACCESS = (1 << 0);
static const uint8_t AW87559_REG_WR_ACCESS = (1 << 1);


class AW87559Component : public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::PRE_HARDWARE; }
  
  void set_reset_gpio(GPIOPin *pin) { reset_gpio_ = pin; }
  
  // Write a single register
  bool write_reg(uint8_t reg, uint8_t value);
  
  // Read a single register
  bool read_reg(uint8_t reg, uint8_t *out_value);
  
  // Apply register sequence: [reg0, val0, reg1, val1, ...]
  bool apply_table(const uint8_t *pairs, size_t length);

 protected:
  GPIOPin *reset_gpio_{nullptr};
  
  // Internal I2C write helper
  bool write_bytes(uint8_t reg, const uint8_t *data, size_t len);
};


// 下面这个数组就是核心：每个元素对应一个寄存器的访问权限
// const unsigned char AW87559_reg_access[AW87559_REG_MAX] = {
//     [AW87559_REG_CHIPID]    = AW87559_REG_RD_ACCESS | AW87559_REG_WR_ACCESS,  // 可读可写
//     [AW87559_REG_SYSCTRL]   = AW87559_REG_RD_ACCESS | AW87559_REG_WR_ACCESS,
//     [AW87559_REG_BATSAFE]   = AW87559_REG_RD_ACCESS | AW87559_REG_WR_ACCESS,
//     [AW87559_REG_BSTOVR]    = AW87559_REG_RD_ACCESS | AW87559_REG_WR_ACCESS,
//     [AW87559_REG_BSTVPR]    = AW87559_REG_RD_ACCESS | AW87559_REG_WR_ACCESS,
//     [AW87559_REG_PAGR]      = AW87559_REG_RD_ACCESS | AW87559_REG_WR_ACCESS,
//     [AW87559_REG_PAGC3OPR]  = AW87559_REG_RD_ACCESS | AW87559_REG_WR_ACCESS,
//     [AW87559_REG_PAGC3PR]   = AW87559_REG_RD_ACCESS | AW87559_REG_WR_ACCESS,
//     [AW87559_REG_PAGC2OPR]  = AW87559_REG_RD_ACCESS | AW87559_REG_WR_ACCESS,
//     [AW87559_REG_PAGC2PR]   = AW87559_REG_RD_ACCESS | AW87559_REG_WR_ACCESS,
//     [AW87559_REG_PAGC1PR]   = AW87559_REG_RD_ACCESS | AW87559_REG_WR_ACCESS,
// };


}  // namespace aw87559
}  // namespace esphome

