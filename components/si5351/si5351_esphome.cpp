#include "si5351_esphome.h"

namespace esphome {
namespace si5351 {

static const char *const TAG = "si5351";

bool Si5351Component::write_reg(uint8_t reg, uint8_t value) {
  return this->write_byte(reg, value);
}

bool Si5351Component::read_reg(uint8_t reg, uint8_t *out_value) {
  return this->read_byte(reg, out_value);
}

void Si5351Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Si5351 Clock Generator...");

  // 1. 先关闭所有输出（最安全）
  if (!this->write_reg(REG_OUTPUT_ENABLE_CONTROL, 0xFF)) {  // 0xFF = 全禁用
    ESP_LOGE(TAG, "Failed to disable all outputs");
    this->mark_failed();
    return;
  }

  // 2. 关闭输出驱动器 (CLK0, CLK1, CLK2)
  uint8_t output_drivers[3] = {0x80, 0x80, 0x80};  // bit7=1 表示 power down
  if (!this->write_register(REG_OUTPUT_DRIVERS, output_drivers, 3)) {
    ESP_LOGE(TAG, "Failed to power down output drivers");
    this->mark_failed();
    return;
  }

  // 3. 设置晶振负载电容（默认 10pF，如果你板子不同可改）
  // 0xC0 = 10pF (bits 7:6 = 11)
  if (!this->write_reg(REG_XTAL_LOAD_CAP, 0xC0)) {
    ESP_LOGE(TAG, "Failed to set crystal load capacitance");
    this->mark_failed();
    return;
  }

  // 4. Multisynth NA (通常控制 CLK0)
  uint8_t multisynth_na[8] = {0xFF, 0xFD, 0x00, 0x09, 0x26, 0xF7, 0x4F, 0x72};
  if (!this->write_register(REG_MULTISYNTH_NA, multisynth_na, 8)) {
    ESP_LOGE(TAG, "Failed to configure Multisynth NA");
    this->mark_failed();
    return;
  }

  // 5. Multisynth1 (CLK1)
  uint8_t multisynth1[8] = {0x00, 0x01, 0x00, 0x2F, 0x00, 0x00, 0x00, 0x00};
  if (!this->write_register(REG_MULTISYNTH1, multisynth1, 8)) {
    ESP_LOGE(TAG, "Failed to configure Multisynth1");
    this->mark_failed();
    return;
  }

  // 6. CLK1 Control Register
  // 0x4C 示例：整数模式 + Multisynth1 驱动 CLK1
  if (!this->write_reg(REG_CLK1_CONTROL, 0x4C)) {
    ESP_LOGE(TAG, "Failed to configure CLK1 control");
    this->mark_failed();
    return;
  }

  // 7. 复位 PLLA 和 PLLB（非常重要）
  if (!this->write_reg(REG_PLL_RESET, 0xA0)) {  // 0xA0 = reset both PLLA & PLLB
    ESP_LOGE(TAG, "Failed to reset PLLs");
    this->mark_failed();
    return;
  }

  delay(15);  // 等待 PLL 锁定

  // 8. 最后打开所有输出
  if (!this->write_reg(REG_OUTPUT_ENABLE_CONTROL, 0x00)) {  // 0x00 = 全启用
    ESP_LOGE(TAG, "Failed to enable outputs");
    this->mark_failed();
    return;
  }

  ESP_LOGI(TAG, "Si5351 Clock Generator initialized successfully");
}

void Si5351Component::dump_config() {
  ESP_LOGCONFIG(TAG, "Si5351 Clock Generator:");
  LOG_I2C_DEVICE(this);

  if (this->is_failed()) {
    ESP_LOGE(TAG, "  Setup failed!");
  } else {
    ESP_LOGCONFIG(TAG, "  Status: OK");
    if (this->reset_gpio_ != nullptr) {
      ESP_LOGCONFIG(TAG, "  Reset pin: configured");
    } else {
      ESP_LOGCONFIG(TAG, "  Reset pin: not used");
    }
  }
}

}  // namespace si5351
}  // namespace esphome
