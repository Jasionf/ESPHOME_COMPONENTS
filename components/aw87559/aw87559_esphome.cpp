#include "aw87559.h"

namespace esphome {
namespace aw87559 {

/* 
流程
  - 读取 ID 寄存器，确认芯片存在
  - 写入 0x01, 0x78 来启用 PA (BIT3: PA Enable)
*/


static const char *TAG = "AW87559";
static aw_ctx_t s_aw = {0};

void AW87559Component::setup() {
  ESP_LOGD(TAG, "Setting up AW87559...");
  
  // Handle GPIO reset if configured
  if (reset_gpio_ != nullptr) {
    reset_gpio_->setup();
    reset_gpio_->digital_write(false);
    delay(2);
    reset_gpio_->digital_write(true);
    delay(2);
  }
  
  // Read ID register (0x00) - should be 0x5A
  uint8_t id;
  if (!read_reg(AW87559_REG_CHIPID, &id)) {
    ESP_LOGE(TAG, "AW87559 not responding - read ID failed at address 0x%02X", address_);
    mark_failed();
    return;
  }
  
  if (id != AW87559_CHIPID) {
    ESP_LOGW(TAG, "AW87559 ID mismatch: expected 0x%02X, got 0x%02X", AW87559_CHIPID, id);
  }
  
  ESP_LOGI(TAG, "AW87559 found with ID: 0x%02X", id);
  
  // Enable PA: write 0x78 to register 0x01 (BIT3: PA Enable)
  if (!write_reg(AW87559_REG_SYSCTRL, 0x78)) {
    ESP_LOGE(TAG, "Failed to enable AW87559 PA");
    mark_failed();
    return;
  }
  
  ESP_LOGI(TAG, "AW87559 Audio Amplifier initialized @0x%02X", address_);
}

void AW87559Component::dump_config() {
  ESP_LOGCONFIG(TAG, "AW87559 Audio Amplifier:");
  LOG_I2C_DEVICE(this);
  if (reset_gpio_ != nullptr) {
    LOG_PIN("  Reset GPIO: ", reset_gpio_);
  }
}

bool AW87559Component::write_bytes(uint8_t reg, const uint8_t *data, size_t len) {
  // Construct buffer: [reg, data[0], data[1], ...]
  uint8_t buf[65];
  
  if ((len + 1) > sizeof(buf)) {
    ESP_LOGE(TAG, "Write buffer overflow: len=%zu", len);
    return false;
  }
  
  buf[0] = reg;
  if (len > 0 && data != nullptr) {
    memcpy(&buf[1], data, len);
  }
  
  if (!write_bytes_raw(buf, len + 1)) {
    ESP_LOGE(TAG, "Failed to write register 0x%02X", reg);
    return false;
  }
  
  return true;
}

bool AW87559Component::write_reg(uint8_t reg, uint8_t value) {
  return write_bytes(reg, &value, 1);
}

bool AW87559Component::read_reg(uint8_t reg, uint8_t *out_value) {
  if (out_value == nullptr) {
    return false;
  }
  
  if (!write_bytes_raw(&reg, 1)) {
    ESP_LOGE(TAG, "Failed to send register address 0x%02X", reg);
    return false;
  }
  
  if (!read_bytes_raw(out_value, 1)) {
    ESP_LOGE(TAG, "Failed to read register 0x%02X", reg);
    return false;
  }
  
  return true;
}

bool AW87559Component::apply_table(const uint8_t *pairs, size_t length) {
  if (pairs == nullptr || (length % 2) != 0) {
    ESP_LOGE(TAG, "Invalid table: length=%zu must be even", length);
    return false;
  }
  
  for (size_t i = 0; i < length; i += 2) {
    uint8_t reg = pairs[i];
    uint8_t val = pairs[i + 1];
    
    if (!write_reg(reg, val)) {
      ESP_LOGE(TAG, "Failed to write table entry at index %zu (reg=0x%02X)", i, reg);
      return false;
    }
  }
  
  return true;
}


// void AW87559::setup() {
//     ESP_LOGCONFIG(TAG, "Setting up AW87559...");

//     uint8_t chip_id;
//     AW87559_ERROR_CHECK(this->read_byte(AW87559_REG_CHIPID, &chip_id), "Failed to read AW87559 CHIP ID");

//     if (chip_id != 0x5A) {
//         ESP_LOGE(TAG, "Chip ID mismatch! Expected 0x5A, got 0x%02X", chip_id);
//         this->mark_failed();
//         return;
//     }
//     // //系统控制寄存器
//     // AW87559_ERROR_CHECK(this->write_byte(AW87559_REG_SYSCTRL, 0x38), "Failed to configure SYSCTRL register");
//     // //电池保护寄存器
//     // AW87559_ERROR_CHECK(this->write_byte(AW87559_REG_BATSAFE, 0x09), "Failed to configure BATSAFE register");
//     // //升压控制寄存器
//     // AW87559_ERROR_CHECK(this->write_byte(AW87559_REG_BATSAFE, 0x0c), "Failed to configure BATSAFE register");

//     ESP_LOGI(TAG, "AW87559 setup completed successfully.");


    
} // namespace aw87559
} // namespace esphome