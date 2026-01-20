#include "si5351.h"
#include "esphome/core/log.h"

namespace esphome {
namespace si5351 {

static const char *TAG = "si5351";

void Si5351Component::setup() {
  ESP_LOGD(TAG, "Setting up Si5351 Clock Generator...");
  
  if (!is_failed()) {
    // Disable all outputs
    if (!write_reg(REG_OUTPUT_ENABLE_CONTROL, 0xFF)) {
      ESP_LOGE(TAG, "Failed to disable Si5351 outputs");
      mark_failed();
      return;
    }
    
    // Power down output drivers (registers 16, 17, 18)
    uint8_t output_drivers[3] = {0x80, 0x80, 0x80};
    if (!write_regs(REG_OUTPUT_DRIVERS, output_drivers, 3)) {
      ESP_LOGE(TAG, "Failed to power down Si5351 output drivers");
      mark_failed();
      return;
    }
    
    // Crystal Internal Load Capacitance
    // 0xC0 = 11000000 (Internal CL = 10 pF - default)
    if (!write_reg(REG_XTAL_LOAD_CAP, 0xC0)) {
      ESP_LOGE(TAG, "Failed to set Si5351 crystal load capacitance");
      mark_failed();
      return;
    }
    
    // Multisynth NA Parameters (8 bytes starting at register 26)
    uint8_t multisynth_na[8] = {0xFF, 0xFD, 0x00, 0x09, 0x26, 0xF7, 0x4F, 0x72};
    if (!write_regs(REG_MULTISYNTH_NA, multisynth_na, 8)) {
      ESP_LOGE(TAG, "Failed to configure Si5351 Multisynth NA");
      mark_failed();
      return;
    }
    
    // Multisynth1 Parameters (8 bytes starting at register 50)
    uint8_t multisynth1[8] = {0x00, 0x01, 0x00, 0x2F, 0x00, 0x00, 0x00, 0x00};
    if (!write_regs(REG_MULTISYNTH1, multisynth1, 8)) {
      ESP_LOGE(TAG, "Failed to configure Si5351 Multisynth1");
      mark_failed();
      return;
    }
    
    // CLK1 Control
    // Bit 6: MS1 operates in integer mode
    // Bits 5-4: Select MultiSynth 1 as the source for CLK1
    // 0x4C = 01001100
    if (!write_reg(REG_CLK1_CONTROL, 0x4C)) {
      ESP_LOGE(TAG, "Failed to configure Si5351 CLK1 control");
      mark_failed();
      return;
    }
    
    // PLL Reset
    // 0xA0 = 10100000 (Reset PLLA and PLLB)
    if (!write_reg(REG_PLL_RESET, 0xA0)) {
      ESP_LOGE(TAG, "Failed to reset Si5351 PLL");
      mark_failed();
      return;
    }
    
    // Enable all outputs
    if (!write_reg(REG_OUTPUT_ENABLE_CONTROL, 0x00)) {
      ESP_LOGE(TAG, "Failed to enable Si5351 outputs");
      mark_failed();
      return;
    }
    
    ESP_LOGI(TAG, "Si5351 Clock Generator initialized successfully");
  }
}

void Si5351Component::dump_config() {
  ESP_LOGCONFIG(TAG, "Si5351 Clock Generator:");
  LOG_I2C_DEVICE(this);
  
  if (is_failed()) {
    ESP_LOGE(TAG, "  Setup failed!");
  } else {
    ESP_LOGCONFIG(TAG, "  Output frequency: CLK1 enabled");
  }
}

bool Si5351Component::write_reg(uint8_t reg, uint8_t value) {
  return write_bytes_raw({reg, value});
}

bool Si5351Component::write_regs(uint8_t start_reg, const uint8_t *data, size_t length) {
  if (data == nullptr || length == 0) {
    return false;
  }
  
  // Create buffer: [reg_addr, data[0], data[1], ...]
  std::vector<uint8_t> buf;
  buf.reserve(length + 1);
  buf.push_back(start_reg);
  buf.insert(buf.end(), data, data + length);
  
  return write_bytes_raw(buf);
}

bool Si5351Component::read_reg(uint8_t reg, uint8_t *out_value) {
  if (out_value == nullptr) {
    return false;
  }
  
  return read_bytes_raw(out_value, 1);
}

}  // namespace si5351
}  // namespace esphome
