#include "pyramidrgb.h"

namespace esphome {
namespace pyramidrgb {

static const char *const TAG = "pyramidrgb";

void PyramidRGBComponent::setup() {
  ESP_LOGI(TAG, "PyramidRGB init (STM32 RGB controller at 0x%02X)", this->address_);
  // 此处不做强制探测，避免不同固件下读寄存器失败导致误判。

  // 应用初始亮度（在 I2C 就绪后执行）
  if (!this->set_strip_brightness(initial_strip_, initial_brightness_)) {
    ESP_LOGW(TAG, "Failed to set initial brightness for strip %u", initial_strip_);
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
  bool ok = this->write_byte(reg, b);
  ESP_LOGD(TAG, "Set brightness: strip=%u reg=0x%02X b=%u (%%=%u) -> %s",
           strip, reg, b, brightness, ok ? "OK" : "FAIL");
  return ok;
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
  // 组装寄存器+数据的写入缓冲区
  const size_t total = len + 1;
  uint8_t *buf = (uint8_t *) malloc(total);
  if (buf == nullptr) return false;
  buf[0] = base_reg_addr;
  memcpy(buf + 1, color_bytes, len);
  bool ok = this->write(buf, total);
  free(buf);
  return ok;
}

bool PyramidRGBComponent::set_channel_color(uint8_t channel, uint8_t r, uint8_t g, uint8_t b) {
  if (channel >= NUM_RGB_CHANNELS) return false;
  channel_colors_[channel][0] = r;
  channel_colors_[channel][1] = g;
  channel_colors_[channel][2] = b;

  // 每个 LED 4 字节：B, G, R, reserved（按硬件定义）
  // 逐 LED 写入，避免设备不支持连续自增写导致失败
  uint8_t base = channel_base_addr_(channel);
  bool all_ok = true;
  for (uint8_t i = 0; i < NUM_LEDS_PER_GROUP; i++) {
    // 通道 0 和 1 的 LED 顺序需要反转（索引 0..6 -> 6..0）
    uint8_t hardware_index = i;
    if (channel == 0 || channel == 1) {
      hardware_index = NUM_LEDS_PER_GROUP - 1 - i;
    }
    uint8_t reg = base + (hardware_index * 4);
    uint8_t led_bytes[4] = {b, g, r, 0x00};
    bool ok = write_color_block_(reg, led_bytes, sizeof(led_bytes));
    ESP_LOGD(TAG, "Set color LED: ch=%u led=%u reg=0x%02X RGB=(%u,%u,%u) -> %s",
             channel, i, reg, r, g, b, ok ? "OK" : "FAIL");
    all_ok = all_ok && ok;
  }
  return all_ok;
}

bool PyramidRGBComponent::set_channel_color_component(uint8_t channel, RGBColorChannel color, uint8_t value) {
  if (channel >= NUM_RGB_CHANNELS) return false;
  switch (color) {
    case COLOR_R: channel_colors_[channel][0] = value; break;
    case COLOR_G: channel_colors_[channel][1] = value; break;
    case COLOR_B: channel_colors_[channel][2] = value; break;
    default: return false;
  }
  ESP_LOGD(TAG, "Set component: ch=%u comp=%d val=%u", channel, (int) color, value);
  return set_channel_color(channel,
                           channel_colors_[channel][0],
                           channel_colors_[channel][1],
                           channel_colors_[channel][2]);
}

}  // namespace pyramidrgb
}  // namespace esphome