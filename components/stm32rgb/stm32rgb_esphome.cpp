#include "stm32rgb_esphome.h"

namespace esphome {
namespace stm32rgb {

void STM32RGBLight::setup() {
  if (this->num_leds_ == 0) {
    ESP_LOGE(TAG, "num_leds not set!");
    this->mark_failed();
    return;
  }

  this->pixels_.resize(this->num_leds_);
  this->effect_data_.resize(this->num_leds_, 0);

  ESP_LOGCONFIG(TAG, "STM32 RGB bridge ready - %d LEDs on strip %d", this->num_leds_, this->strip_index_);
}

void STM32RGBLight::dump_config() {
  ESP_LOGCONFIG(TAG, "STM32 RGB:");
  ESP_LOGCONFIG(TAG, "  LEDs: %d", this->num_leds_);
  ESP_LOGCONFIG(TAG, "  Strip: %d", this->strip_index_);
  ESP_LOGCONFIG(TAG, "  I2C Addr: 0x%02X", this->address_);
}

light::LightTraits STM32RGBLight::get_traits() {
  auto traits = light::LightTraits();
  traits.set_supported_color_modes({light::ColorMode::RGB});
  traits.set_supports_brightness(true);
  return traits;
}

light::ESPColorView STM32RGBLight::operator[](int32_t index) const {
  if (index < 0 || index >= this->size()) {
    return light::ESPColorView(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
  }

  uint8_t *r = reinterpret_cast<uint8_t *>(&this->pixels_[index].red);
  uint8_t *g = reinterpret_cast<uint8_t *>(&this->pixels_[index].green);
  uint8_t *b = reinterpret_cast<uint8_t *>(&this->pixels_[index].blue);
  uint8_t *w = nullptr;  // 无白通道

  return light::ESPColorView(
      r + this->rgb_offsets_[0],
      g + this->rgb_offsets_[1],
      b + this->rgb_offsets_[2],
      w,
      &this->effect_data_[index],
      &this->correction_);
}

void STM32RGBLight::clear_effect_data() {
  std::fill(this->effect_data_.begin(), this->effect_data_.end(), 0);
}

void STM32RGBLight::show() {
  if (!this->should_show_()) return;

  uint8_t brightness = static_cast<uint8_t>(this->state_->current_values.get_brightness() * 255.0f);
  write_brightness(brightness);

  if (!send_pixel_data()) {
    ESP_LOGW(TAG, "Failed to send pixel data to STM32");
  }

  this->mark_shown_();
}

bool STM32RGBLight::write_brightness(uint8_t brightness) {
  uint8_t reg = (this->strip_index_ == 0) ? REG_BRIGHTNESS_STRIP1 : REG_BRIGHTNESS_STRIP2;
  return this->write_byte(reg, brightness);
}

bool STM32RGBLight::send_pixel_data() {
  // 每组 7 LEDs × 4 bytes = 28 bytes
  const size_t bytes_per_group = LEDS_PER_GROUP * BYTES_PER_LED;

  // 假设两条组（即使只有一条灯带，也可发两组或只发需要的）
  uint8_t group0_reg = get_channel_reg(0);
  uint8_t group1_reg = get_channel_reg(1);

  // 准备 raw buffer（GRB 顺序示例）
  std::vector<uint8_t> buffer(bytes_per_group * 2, 0);

  for (uint16_t i = 0; i < this->num_leds_; ++i) {
    const auto &px = this->pixels_[i];
    size_t offset = i * BYTES_PER_LED;

    // 根据 pixel_order 排列字节顺序
    buffer[offset + rgb_offsets_[0]] = px.green;  // 示例 GRB
    buffer[offset + rgb_offsets_[1]] = px.red;
    buffer[offset + rgb_offsets_[2]] = px.blue;
    buffer[offset + 3] = 0;  // 白或保留
  }

  // 分组发送（I2C 一次最多 ~32-256 字节，视芯片而定）
  bool ok = true;
  ok &= this->write_bytes(group0_reg, buffer.data(), bytes_per_group);
  ok &= this->write_bytes(group1_reg, buffer.data() + bytes_per_group, bytes_per_group);

  // 可选：发送刷新命令
  // this->write_byte(REG_REFRESH, 0x01);

  return ok;
}

uint8_t STM32RGBLight::get_channel_reg(uint8_t group) const {
  if (this->strip_index_ == 0) {
    return (group == 0) ? REG_COLOR_CH0 : REG_COLOR_CH1;
  } else {
    return (group == 0) ? REG_COLOR_CH2 : REG_COLOR_CH3;
  }
}

void STM32RGBLight::set_pixel_order(PixelOrder order) {
  this->pixel_order_ = order;
  uint8_t val = static_cast<uint8_t>(order);
  this->rgb_offsets_[0] = (val >> 6) & 0b11;  // R
  this->rgb_offsets_[1] = (val >> 4) & 0b11;  // G
  this->rgb_offsets_[2] = (val >> 2) & 0b11;  // B
  this->rgb_offsets_[3] = (val >> 0) & 0b11;  // W (如果有)
}

}  // namespace stm32gb
}  // namespace esphome