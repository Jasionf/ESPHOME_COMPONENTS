#include "pyramidrgb.h"

namespace esphome {
namespace pyramidrgb {

static const char *const TAG = "pyramidrgb";

void PyramidRGBComponent::setup() {
  ESP_LOGI(TAG, "PyramidRGB init (STM32 RGB controller at 0x%02X)", this->address_);
  // 基本探测：尝试读取一个寄存器（亮度寄存器）以确认 I2C 工作
  uint8_t dummy = 0;
  if (!this->read_bytes(RGB1_BRIGHTNESS_REG_ADDR, &dummy, 1)) {
    ESP_LOGW(TAG, "Failed to communicate with STM32 RGB controller");
  }
}

void PyramidRGBComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "PyramidRGB Component");
  LOG_I2C_DEVICE(this);
}

bool PyramidRGBComponent::set_strip_brightness(uint8_t strip, uint8_t brightness) {
  if (strip < 1 || strip > 2) return false;
  if (brightness > 100) brightness = 100;
  uint8_t reg = (strip == 1) ? RGB1_BRIGHTNESS_REG_ADDR : RGB2_BRIGHTNESS_REG_ADDR;
  uint8_t b = (uint8_t)((brightness * 255) / 100);
  return this->write_bytes(reg, &b, 1);
}

uint8_t PyramidRGBComponent::channel_base_addr_(uint8_t channel) const {
  switch (channel) {
    case 0: return RGB_CH1_I1_COLOR_REG_ADDR; // Channel 0 -> 灯带1组1
    case 1: return RGB_CH2_I1_COLOR_REG_ADDR; // Channel 1 -> 灯带1组2
    case 2: return RGB_CH4_I1_COLOR_REG_ADDR; // 设备映射：channel 2 -> CH4
    case 3: return RGB_CH3_I1_COLOR_REG_ADDR; // 设备映射：channel 3 -> CH3
    default: return RGB_CH1_I1_COLOR_REG_ADDR;
  }
}

bool PyramidRGBComponent::write_color_block_(uint8_t base_reg_addr, const uint8_t *color_bytes, size_t len) {
  return this->write_bytes(base_reg_addr, color_bytes, len);
}

bool PyramidRGBComponent::set_channel_color(uint8_t channel, uint8_t r, uint8_t g, uint8_t b) {
  if (channel >= NUM_RGB_CHANNELS) return false;
  channel_colors_[channel][0] = r;
  channel_colors_[channel][1] = g;
  channel_colors_[channel][2] = b;

  // 每个 LED 4 字节：B, G, R, reserved（按硬件定义）
  // 共写入 7 个 LED 的数据块（连续地址）
  uint8_t buf[NUM_LEDS_PER_GROUP * 4];
  for (uint8_t i = 0; i < NUM_LEDS_PER_GROUP; i++) {
    // 通道 0 和 1 的 LED 顺序需要反转（0..6 -> 6..0），但由于一次性批量写入，硬件寄存器布局已按 LED 递增
    // 这里直接统一填充颜色，硬件内部按 LED 索引递增写入
    buf[i * 4 + 0] = b; // B
    buf[i * 4 + 1] = g; // G
    buf[i * 4 + 2] = r; // R
    buf[i * 4 + 3] = 0x00; // reserved
  }
  uint8_t base = channel_base_addr_(channel);
  return write_color_block_(base, buf, sizeof(buf));
}

bool PyramidRGBComponent::set_channel_color_component(uint8_t channel, RGBColorChannel color, uint8_t value) {
  if (channel >= NUM_RGB_CHANNELS) return false;
  switch (color) {
    case COLOR_R: channel_colors_[channel][0] = value; break;
    case COLOR_G: channel_colors_[channel][1] = value; break;
    case COLOR_B: channel_colors_[channel][2] = value; break;
    default: return false;
  }
  return set_channel_color(channel,
                           channel_colors_[channel][0],
                           channel_colors_[channel][1],
                           channel_colors_[channel][2]);
}

}  // namespace pyramidrgb
}  // namespace esphome