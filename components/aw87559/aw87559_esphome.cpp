#include "aw87559_esphome.h"

namespace esphome {
namespace aw87559 {

static const char *const TAG = "aw87559";

void AW87559Component::setup() {
  ESP_LOGD(TAG, "Setting up AW87559...");

  if (this->reset_gpio_ != nullptr) {
    this->reset_gpio_->setup();
    this->reset_gpio_->digital_write(false);
    esphome::delay(2);
    this->reset_gpio_->digital_write(true);
    esphome::delay(2);
  }

  uint8_t id;
  if (!this->read_reg(AW87559_REG_CHIPID, &id)) {
    ESP_LOGE(TAG, "AW87559 not responding - read ID failed at address 0x%02X", this->address_);
    this->mark_failed();
    return;
  }

  if (id != AW87559_CHIPID) {
    ESP_LOGW(TAG, "AW87559 ID mismatch: expected 0x%02X, got 0x%02X", AW87559_CHIPID, id);
  } else {
    ESP_LOGI(TAG, "AW87559 found with ID: 0x%02X", id);
  }

  // 启用 PA
  if (!this->write_reg(AW87559_REG_SYSCTRL, 0x78)) {
    ESP_LOGE(TAG, "Failed to enable AW87559 PA");
    this->mark_failed();
    return;
  }

  ESP_LOGI(TAG, "AW87559 Audio Amplifier initialized @0x%02X", this->address_);
}

void AW87559Component::dump_config() {
  ESP_LOGCONFIG(TAG, "AW87559 Audio Amplifier:");
  LOG_I2C_DEVICE(this);
  if (this->reset_gpio_ != nullptr) {
    LOG_PIN("  Reset GPIO: ", this->reset_gpio_);
  }
}

bool AW87559Component::write_bytes(uint8_t reg, const uint8_t *data, size_t len) {
  uint8_t buf[65];
  if ((len + 1) > sizeof(buf)) {
    ESP_LOGE(TAG, "Write buffer overflow: len=%zu", len);
    return false;
  }

  buf[0] = reg;
  if (len > 0 && data != nullptr) {
    memcpy(&buf[1], data, len);
  }

  return this->write_bytes_16(buf, len + 1);
}

bool AW87559Component::write_reg(uint8_t reg, uint8_t value) {
  return this->write_bytes(reg, &value, 1);
}

bool AW87559Component::read_reg(uint8_t reg, uint8_t *out_value) {
  if (out_value == nullptr) return false;

  if (!this->write_bytes_16(&reg, 1)) {
    ESP_LOGE(TAG, "Failed to send register address 0x%02X", reg);
    return false;
  }

  if (!this->read_bytes_16(out_value, 1)) {
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

    if (!this->write_reg(reg, val)) {
      ESP_LOGE(TAG, "Failed to write table entry at index %zu (reg=0x%02X)", i, reg);
      return false;
    }
  }

  return true;
}

}  // namespace aw87559
}  // namespace esphome
