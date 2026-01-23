/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include <stdlib.h>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_spiffs.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_vfs_fat.h"
#include "bsp_err_check.h"
#include "button_gpio.h"
#include "driver/i2s_std.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "esp_lcd_gc9107.h"
#include "bsp/m5stack_atoms3r_echopyramid.h"
#include "bsp/display.h"
#include "bsp/bsp_codec.h"
 

static const char *TAG = "M5Stack";


//==============================================================================================
// I2C
//==============================================================================================
static bool i2c_initialized[2] = {false, false};
static i2c_master_bus_handle_t bsp_i2c_bus_handle[2] = {NULL, NULL};
// I2C 扫描结果
static bsp_i2c_scan_result_t i2c_scan_result[2] = {0};

esp_err_t bsp_i2c_init(i2c_port_t i2c_num, gpio_num_t i2c_scl, gpio_num_t i2c_sda)
{
    if (i2c_num < I2C_NUM_0 || i2c_num > I2C_NUM_1) {
        return ESP_ERR_INVALID_ARG;
    }

    if (i2c_initialized[i2c_num]) {
        return ESP_OK;
    }

    const i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = i2c_num,
        .scl_io_num = i2c_scl,
        .sda_io_num = i2c_sda,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,
    };

    esp_err_t ret = i2c_new_master_bus(&i2c_bus_config, &bsp_i2c_bus_handle[i2c_num]);
    if (ret != ESP_OK) {
        return ret;
    }

    i2c_initialized[i2c_num] = true;
    return ESP_OK;
}

esp_err_t bsp_i2c_deinit(i2c_port_t i2c_num)
{
    if (i2c_num < I2C_NUM_0 || i2c_num > I2C_NUM_1) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!i2c_initialized[i2c_num]) {
        return ESP_OK;
    }

    esp_err_t ret = i2c_del_master_bus(bsp_i2c_bus_handle[i2c_num]);
    if (ret == ESP_OK) {
        bsp_i2c_bus_handle[i2c_num] = NULL;
        i2c_initialized[i2c_num] = false;
    }
    return ret;
}

i2c_master_bus_handle_t bsp_i2c_get_handle(i2c_port_t i2c_num)
{
    if (i2c_num < I2C_NUM_0 || i2c_num > I2C_NUM_1) {
        return NULL;
    }
    return bsp_i2c_bus_handle[i2c_num];
}

esp_err_t bsp_i2c_scan(i2c_port_t i2c_num, bsp_i2c_scan_result_t* result)
{
    if (i2c_num < I2C_NUM_0 || i2c_num > I2C_NUM_1) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_master_bus_handle_t bus = bsp_i2c_bus_handle[i2c_num];
    if (bus == NULL) {
        ESP_LOGE(TAG, "I2C%d not initialized!\n", i2c_num);
        return ESP_ERR_INVALID_STATE;
    }

    // 清空之前的扫描结果
    memset(&i2c_scan_result[i2c_num], 0, sizeof(bsp_i2c_scan_result_t));

    esp_err_t ret;
    uint8_t address;
    size_t count = 0;

    ESP_LOGI(TAG, "Scanning I2C%d bus...", i2c_num);
    printf("\n     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\r\n");

    for (int i = 0; i < 128; i += 16) {
        printf("%02x: ", i);
        for (int j = 0; j < 16; j++) {
            fflush(stdout);
            address = i + j;
            ret = i2c_master_probe(bus, address, 50);
            if (ret == ESP_OK) {
                printf("%02X ", address);
                if (count < 128) {
                    i2c_scan_result[i2c_num].addresses[count++] = address;
                }
            } else if (ret == ESP_ERR_TIMEOUT) {
                printf("UU ");
            } else {
                printf("-- ");
            }
        }
        printf("\r\n");
    }

    printf("\nI2C%d scan finished\n", i2c_num);
    
    // 保存扫描结果
    i2c_scan_result[i2c_num].count = count;
    i2c_scan_result[i2c_num].scanned = true;
    
    // 如果提供了输出参数，复制结果
    if (result) {
        memcpy(result, &i2c_scan_result[i2c_num], sizeof(bsp_i2c_scan_result_t));
    }
    
    ESP_LOGI(TAG, "I2C%d scan found %d devices", i2c_num, count);
    return ESP_OK;
}

bool bsp_i2c_is_device_found(i2c_port_t i2c_num, uint8_t device_addr)
{
    if (i2c_num < I2C_NUM_0 || i2c_num > I2C_NUM_1) {
        return false;
    }

    if (!i2c_scan_result[i2c_num].scanned) {
        ESP_LOGW(TAG, "I2C%d not scanned yet, scanning now...", i2c_num);
        // 自动执行扫描
        esp_err_t ret = bsp_i2c_scan(i2c_num, NULL);
        if (ret != ESP_OK) {
            return false;
        }
    }

    if (device_addr >= 128) {
        return false;
    }

    // 在地址列表中查找
    for (size_t i = 0; i < i2c_scan_result[i2c_num].count; i++) {
        if (i2c_scan_result[i2c_num].addresses[i] == device_addr) {
            return true;
        }
    }

    return false;
}

esp_err_t bsp_i2c_write_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t data)
{
    uint8_t write_buf[2] = {reg, data};
    return i2c_master_transmit(dev_handle, write_buf, 2, 200);
}

esp_err_t bsp_i2c_read_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t *data)
{
    return i2c_master_transmit_receive(dev_handle, &reg, 1, data, 1, 200);
}

esp_err_t bsp_i2c_write_regs(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t *data, size_t len)
{
    uint8_t *write_buf = malloc(len + 1);
    if (write_buf == NULL) {
        return ESP_ERR_NO_MEM;
    }
    write_buf[0] = reg;
    memcpy(write_buf + 1, data, len);
    esp_err_t ret = i2c_master_transmit(dev_handle, write_buf, len + 1, 200);
    free(write_buf);
    return ret;
}

//==============================================================================================
// LCD
//==============================================================================================
static lv_display_t *disp;


// Bit number used to represent command and parameter
#define LCD_CMD_BITS           8
#define LCD_PARAM_BITS         8


#define LP5562_I2C_ADDR        0x30
static i2c_master_dev_handle_t lp5562_dev_handle = NULL;

esp_err_t bsp_display_brightness_init(void)
{
    esp_err_t ret = ESP_OK;

    i2c_master_bus_handle_t i2c_bus = bsp_i2c_get_handle(BSP_SYS_I2C_NUM);
    if (i2c_bus == NULL) {
        ESP_LOGE(TAG, "I2C bus not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // Create I2C device for LP5562
    i2c_device_config_t lp5562_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = LP5562_I2C_ADDR,
        .scl_speed_hz = 400000,
    };
    
    ret = i2c_master_bus_add_device(i2c_bus, &lp5562_cfg, &lp5562_dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add LP5562 device: %s", esp_err_to_name(ret));
        return ret;
    }

    ret |= bsp_i2c_write_reg(lp5562_dev_handle, 0x00, 0x40);
    ret |= bsp_i2c_write_reg(lp5562_dev_handle, 0x08, 0x01);
    ret |= bsp_i2c_write_reg(lp5562_dev_handle, 0x70, 0x00);
    uint8_t data;
    // PWM clock frequency 558 Hz
    ret |= bsp_i2c_read_reg(lp5562_dev_handle, 0x08, &data);
    data = data | 0B01000000;
    ret |= bsp_i2c_write_reg(lp5562_dev_handle, 0x08, data);

    return ret;
}

esp_err_t bsp_display_brightness_set(int brightness_percent)
{
    if (brightness_percent > 100) {
        brightness_percent = 100;
    }
    if (brightness_percent < 0) {
        brightness_percent = 0;
    }

    // Map 0~100 to 0~255
    uint8_t brightness = (brightness_percent * 255) / 100;
 
    return bsp_i2c_write_reg(lp5562_dev_handle, 0x0E, brightness);
}

esp_err_t bsp_display_backlight_off(void)
{
    return bsp_display_brightness_set(0);
}

esp_err_t bsp_display_backlight_on(void)
{
    return bsp_display_brightness_set(100);
}

esp_err_t bsp_display_new(const bsp_display_config_t *config, esp_lcd_panel_handle_t *ret_panel, esp_lcd_panel_io_handle_t *ret_io)
{
    esp_err_t ret = ESP_OK;
    assert(config != NULL && config->max_transfer_sz > 0);

    /* Initialize SPI */
    ESP_LOGI(TAG, "Initialize SPI bus");
    spi_bus_config_t bus_cfg = {
        .sclk_io_num = BSP_LCD_PCLK,
        .mosi_io_num = BSP_LCD_MOSI,
        .miso_io_num = BSP_LCD_MISO,
        .quadwp_io_num = -1,  
        .quadhd_io_num = -1,  
        .max_transfer_sz = config->max_transfer_sz, //EXAMPLE_LCD_H_RES * PARALLEL_LINES * sizeof(uint16_t),
    };
    ret = spi_bus_initialize(BSP_LCD_SPI_NUM, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed");
        return ret;
    }

    /* Attach the LCD to the SPI bus */
    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = BSP_LCD_DC,
        .cs_gpio_num = BSP_LCD_CS,
        .pclk_hz = BSP_LCD_PIXEL_CLOCK_HZ,    
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    }; 
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)BSP_LCD_SPI_NUM, &io_config, ret_io), err, TAG, "Panel IO init failed");

    /* 显示屏控制器 gc9107 初始化 */
    ESP_LOGI(TAG, "Install gc9107 panel driver");
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = BSP_LCD_RST,
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
        .color_space = ESP_LCD_COLOR_SPACE_BGR,
#else
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .data_endian = LCD_RGB_DATA_ENDIAN_BIG,
#endif
        .bits_per_pixel = 16,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_gc9107(*ret_io, &panel_config, ret_panel), err, TAG, "Panel init failed");
   
    /* 显示屏显示设置 */
    ESP_GOTO_ON_ERROR(esp_lcd_panel_reset(*ret_panel), err, TAG, "Panel reset failed");
    ESP_GOTO_ON_ERROR(esp_lcd_panel_init(*ret_panel), err, TAG, "Panel init failed");
    ESP_GOTO_ON_ERROR(esp_lcd_panel_mirror(*ret_panel, false, true), err, TAG, "Panel mirror failed");
    ESP_GOTO_ON_ERROR(esp_lcd_panel_invert_color(*ret_panel, true), err, TAG, "Panel invert color failed");
    ESP_GOTO_ON_ERROR(esp_lcd_panel_set_gap(*ret_panel, 2, 1), err, TAG, "Panel set gap failed");
    ESP_GOTO_ON_ERROR(esp_lcd_panel_disp_on_off(*ret_panel, true), err, TAG, "Panel disp on failed");

    return ret;

err:
    if (*ret_panel) {
        esp_lcd_panel_del(*ret_panel);
        *ret_panel = NULL;
    }
    if (*ret_io) {
        esp_lcd_panel_io_del(*ret_io);
        *ret_io = NULL;
    }
    spi_bus_free(BSP_LCD_SPI_NUM);

    return ret;
}

static lv_display_t *bsp_display_lcd_init(const bsp_display_cfg_t *cfg)
{
    assert(cfg != NULL);
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_handle_t panel_handle = NULL;
    const bsp_display_config_t bsp_disp_cfg = {
        .max_transfer_sz = BSP_LCD_DRAW_BUFF_SIZE * sizeof(uint16_t),
    };
    BSP_ERROR_CHECK_RETURN_NULL(bsp_display_new(&bsp_disp_cfg, &panel_handle, &io_handle));

    esp_lcd_panel_disp_on_off(panel_handle, true);

    /* Add LCD screen */
    ESP_LOGD(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = cfg->buffer_size,
        .double_buffer = cfg->double_buffer,
        .hres = BSP_LCD_H_RES,
        .vres = BSP_LCD_V_RES,
        .monochrome = false,
        /* Rotation values must be same as used in esp_lcd for initial settings of the screen */
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = cfg->flags.buff_dma,
            .buff_spiram = cfg->flags.buff_spiram,
#if LVGL_VERSION_MAJOR >= 9
            .swap_bytes = (BSP_LCD_BIGENDIAN ? true : false),
#endif
        }
    };

    return lvgl_port_add_disp(&disp_cfg);
}

lv_display_t *bsp_display_start(void)
{
    esp_err_t ret = ESP_OK;

    bsp_i2c_init(BSP_SYS_I2C_NUM, BSP_SYS_I2C_SCL, BSP_SYS_I2C_SDA);

    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_DRAW_BUFF_SIZE,
        .double_buffer = BSP_LCD_DRAW_BUFF_DOUBLE,
        .flags = {
            .buff_dma = true,
            .buff_spiram = false,
        }
    };
    cfg.lvgl_port_cfg.task_affinity = 1; /* For camera */
    return bsp_display_start_with_config(&cfg);
}

lv_display_t *bsp_display_start_with_config(const bsp_display_cfg_t *cfg)
{
    assert(cfg != NULL);
    BSP_ERROR_CHECK_RETURN_NULL(lvgl_port_init(&cfg->lvgl_port_cfg));

    BSP_ERROR_CHECK_RETURN_NULL(bsp_display_brightness_init());

    BSP_NULL_CHECK(disp = bsp_display_lcd_init(cfg), NULL);

    return disp;
}

void bsp_display_rotate(lv_display_t *disp, lv_display_rotation_t rotation)
{
    lv_disp_set_rotation(disp, rotation);
}

bool bsp_display_lock(uint32_t timeout_ms)
{
    return lvgl_port_lock(timeout_ms);
}

void bsp_display_unlock(void)
{
    lvgl_port_unlock();
}


//==============================================================================================
// Button
//==============================================================================================
static button_handle_t button_handle = NULL;

esp_err_t bsp_button_init(void)
{
    esp_err_t ret = ESP_OK;
    const button_config_t btn_cfg = {0};

    const button_gpio_config_t gpio_cfg = {
        .gpio_num = BSP_BUTTON_NUM,
        .active_level = 0,
    };
    
    ret = iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &button_handle);
    if (ret != ESP_OK || button_handle == NULL) {
        ESP_LOGE(TAG, "Failed to create button");
        return ESP_FAIL;
    }
 
    return ESP_OK;
}

esp_err_t bsp_button_register_cb(button_event_t event, button_cb_t cb, void *usr_data)
{
    if (button_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    return iot_button_register_cb(button_handle, event, NULL, cb, usr_data);
}

button_handle_t bsp_button_get_handle(void)
{
    return button_handle;
}

//==============================================================================================
// STM32 RGB
//==============================================================================================
static i2c_master_dev_handle_t stm32_dev_handle = NULL;

esp_err_t bsp_rgb_init(void)
{
    esp_err_t ret = ESP_OK;

    i2c_master_bus_handle_t i2c_bus = bsp_i2c_get_handle(BSP_EXT_I2C_NUM);
    if (i2c_bus == NULL) {
        ret = bsp_i2c_init(BSP_EXT_I2C_NUM, BSP_EXT_I2C_SCL, BSP_EXT_I2C_SDA);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to init external I2C bus");
            return ret;
        }
        i2c_bus = bsp_i2c_get_handle(BSP_EXT_I2C_NUM);
    }

    // Create I2C device for STM32
    i2c_device_config_t stm32_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = STM32_I2C_ADDR,
        .scl_speed_hz = 400000,
    };
    
    ret = i2c_master_bus_add_device(i2c_bus, &stm32_cfg, &stm32_dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add STM32 device: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "STM32 RGB controller initialized");
    return ESP_OK;
}

esp_err_t bsp_set_rgb_brightness(uint8_t strip, uint8_t brightness)
{
    if (stm32_dev_handle == NULL) {
        ESP_LOGE(TAG, "STM32 not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (strip >= 2) {
        ESP_LOGE(TAG, "Invalid strip: %d (max: 1)", strip);
        return ESP_ERR_INVALID_ARG;
    }

    if (brightness > 100) {
        brightness = 100;
    }

    uint8_t reg_addr = (strip == 0) ? RGB1_BRIGHTNESS_REG_ADDR : RGB2_BRIGHTNESS_REG_ADDR;
    return bsp_i2c_write_reg(stm32_dev_handle, reg_addr, brightness);
}

esp_err_t bsp_set_rgb_color(uint8_t channel, uint8_t index, uint32_t color)
{
    if (stm32_dev_handle == NULL) {
        ESP_LOGE(TAG, "STM32 not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (channel >= NUM_RGB_CHANNELS) {
        ESP_LOGE(TAG, "Invalid channel: %d (max: %d)", channel, NUM_RGB_CHANNELS - 1);
        return ESP_ERR_INVALID_ARG;
    }

    if (index >= NUM_LEDS_PER_GROUP) {
        ESP_LOGE(TAG, "Invalid LED index: %d (max: %d)", index, NUM_LEDS_PER_GROUP - 1);
        return ESP_ERR_INVALID_ARG;
    }

    // 通道0和1的LED顺序需要反转（索引0-6变为6-0）
    uint8_t hardware_index = index;
    if (channel == 0 || channel == 1) {
        hardware_index = NUM_LEDS_PER_GROUP - 1 - index; // 反转：0->6, 1->5, 2->4, 3->3, 4->2, 5->1, 6->0
    }

    // 计算寄存器地址：基址 + LED索引 * 4字节
    // 通道2和3交换：channel 2 -> RGB_CH4, channel 3 -> RGB_CH3
    uint8_t reg_addr;
    switch (channel) {
        case 0:
            reg_addr = RGB_CH1_I1_COLOR_REG_ADDR + hardware_index * 4;
            break;
        case 1:
            reg_addr = RGB_CH2_I1_COLOR_REG_ADDR + hardware_index * 4;
            break;
        case 2:
            reg_addr = RGB_CH4_I1_COLOR_REG_ADDR + hardware_index * 4; // 通道2映射到CH4
            break;
        case 3:
            reg_addr = RGB_CH3_I1_COLOR_REG_ADDR + hardware_index * 4; // 通道3映射到CH3
            break;
        default:
            ESP_LOGE(TAG, "Invalid channel: %d", channel);
            return ESP_ERR_INVALID_ARG;
    }

    // 写入颜色数据（4字节：B, G, R, reserved）
    // color 格式：0x00RRGGBB
    uint8_t color_bytes[4];
    color_bytes[0] = (color >> 0) & 0xFF;   // B
    color_bytes[1] = (color >> 8) & 0xFF;   // G
    color_bytes[2] = (color >> 16) & 0xFF; // R
    color_bytes[3] = 0x00;                  // reserved

    return bsp_i2c_write_regs(stm32_dev_handle, reg_addr, color_bytes, sizeof(color_bytes));
}

//==============================================================================================
// AW87559 Audio Amplifier
//==============================================================================================
static i2c_master_dev_handle_t aw87559_dev_handle = NULL;

esp_err_t bsp_pa_init(void)
{
    esp_err_t ret = ESP_OK;

    // 确保外部I2C总线已初始化
    i2c_master_bus_handle_t i2c_bus = bsp_i2c_get_handle(BSP_EXT_I2C_NUM);
    if (i2c_bus == NULL) {
        ret = bsp_i2c_init(BSP_EXT_I2C_NUM, BSP_EXT_I2C_SCL, BSP_EXT_I2C_SDA);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to init external I2C bus");
            return ret;
        }
        i2c_bus = bsp_i2c_get_handle(BSP_EXT_I2C_NUM);
    }

    // 检查设备是否存在
    ret = i2c_master_probe(i2c_bus, AW87559_I2C_ADDR, 200);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "AW87559 not found on I2C bus (0x%02X), skipping initialization", AW87559_I2C_ADDR);
        return ESP_ERR_NOT_FOUND;
    }

    // Create I2C device for AW87559
    i2c_device_config_t aw87559_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = AW87559_I2C_ADDR,
        .scl_speed_hz = 400000,
    };
    
    ret = i2c_master_bus_add_device(i2c_bus, &aw87559_cfg, &aw87559_dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add AW87559 device: %s", esp_err_to_name(ret));
        return ret;
    }

    // 读取ID寄存器 (0x00)，应该是 0x5A
    uint8_t id;
    ret = bsp_i2c_read_reg(aw87559_dev_handle, 0x00, &id);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read AW87559 ID");
        return ret;
    }
    ESP_LOGI(TAG, "AW87559 ID: 0x%02X", id);

    // 写入 0x01, 0x78 来启用 PA (BIT3: PA Enable)
    ret = bsp_i2c_write_reg(aw87559_dev_handle, 0x01, 0x78);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable AW87559 PA");
        return ret;
    }

    ESP_LOGI(TAG, "AW87559 Audio Amplifier initialized");
    return ESP_OK;
}

//==============================================================================================
// Si5351 Clock Generator
//==============================================================================================
static i2c_master_dev_handle_t si5351_dev_handle = NULL;

esp_err_t bsp_clock_generator_init(void)
{
    esp_err_t ret = ESP_OK;
    uint8_t w_buffer[10] = {0};

    // 确保外部I2C总线已初始化
    i2c_master_bus_handle_t i2c_bus = bsp_i2c_get_handle(BSP_EXT_I2C_NUM);
    if (i2c_bus == NULL) {
        ret = bsp_i2c_init(BSP_EXT_I2C_NUM, BSP_EXT_I2C_SCL, BSP_EXT_I2C_SDA);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to init external I2C bus");
            return ret;
        }
        i2c_bus = bsp_i2c_get_handle(BSP_EXT_I2C_NUM);
    }

    // Create I2C device for Si5351
    i2c_device_config_t si5351_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SI5351_I2C_ADDR,
        .scl_speed_hz = 400000,
    };
    
    ret = i2c_master_bus_add_device(i2c_bus, &si5351_cfg, &si5351_dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add Si5351 device: %s", esp_err_to_name(ret));
        return ret;
    }

    // Disable all outputs
    ret = bsp_i2c_write_reg(si5351_dev_handle, 3, 0xff);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to disable Si5351 outputs");
        return ret;
    }

    // Power down output drivers
    w_buffer[0] = 0x80;  // 10000000
    w_buffer[1] = 0x80;
    w_buffer[2] = 0x80;
    ret = bsp_i2c_write_regs(si5351_dev_handle, 16, w_buffer, 3);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to power down Si5351 output drivers");
        return ret;
    }

    // Crystal Internal Load Capacitance
    ret = bsp_i2c_write_reg(si5351_dev_handle, 183, 0xC0);  // 11000000 // Internal CL = 10 pF (default)
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set Si5351 crystal load");
        return ret;
    }

    // Multisynth NA Parameters
    w_buffer[0] = 0xFF;
    w_buffer[1] = 0xFD;
    w_buffer[2] = 0x00;
    w_buffer[3] = 0x09;
    w_buffer[4] = 0x26;
    w_buffer[5] = 0xF7;
    w_buffer[6] = 0x4F;
    w_buffer[7] = 0x72;
    ret = bsp_i2c_write_regs(si5351_dev_handle, 26, w_buffer, 8);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure Si5351 Multisynth NA");
        return ret;
    }

    // Multisynth1 Parameters
    w_buffer[0] = 0x00;
    w_buffer[1] = 0x01;
    w_buffer[2] = 0x00;
    w_buffer[3] = 0x2F;
    w_buffer[4] = 0x00;
    w_buffer[5] = 0x00;
    w_buffer[6] = 0x00;
    w_buffer[7] = 0x00;
    ret = bsp_i2c_write_regs(si5351_dev_handle, 50, w_buffer, 8);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure Si5351 Multisynth1");
        return ret;
    }

    // CLK1 Control
    // Bit 6: MS1 operates in integer mode
    // Bits 5-4: Select MultiSynth 1 as the source for CLK1
    ret = bsp_i2c_write_reg(si5351_dev_handle, 17, ((3 << 2) | (1 << 6)));  // 01001100 = 0x4C
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure Si5351 CLK1 control");
        return ret;
    }

    // PLL Reset
    ret = bsp_i2c_write_reg(si5351_dev_handle, 177, 0xA0);  // 10100000
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reset Si5351 PLL");
        return ret;
    }

    // Enable all outputs
    ret = bsp_i2c_write_reg(si5351_dev_handle, 3, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable Si5351 outputs");
        return ret;
    }

    ESP_LOGI(TAG, "Si5351 Clock Generator initialized");
    return ESP_OK;
}

//==============================================================================================
// Touch Detection
//==============================================================================================

// Touch callback list
#define MAX_TOUCH_CALLBACKS 10
static bsp_touch_callback_t touch_callbacks[MAX_TOUCH_CALLBACKS] = {0};
static uint8_t touch_callback_count = 0;

// Touch swipe timeout constant
#define TOUCH_SWIPE_TIMEOUT_MS  500  // 滑动检测超时500ms

// Touch detection state
static struct {
    uint8_t touch_last_state[4];
    uint8_t left_swipe_first;   // 左侧第一个按下的触摸按键（1或2）
    uint8_t right_swipe_first;  // 右侧第一个按下的触摸按键（3或4）
    uint32_t left_swipe_time;   // 左侧第一个按键按下的时间
    uint32_t right_swipe_time;  // 右侧第一个按键按下的时间
    TaskHandle_t touch_task_handle;
    bool initialized;
} touch_state = {
    .touch_last_state = {0, 0, 0, 0},
    .left_swipe_first = 0,
    .right_swipe_first = 0,
    .left_swipe_time = 0,
    .right_swipe_time = 0,
    .touch_task_handle = NULL,
    .initialized = false
};

/**
 * @brief Read STM32 touch status
 * @param touch_num Touch number (1-4)
 * @return true if pressed, false if released or error
 */
static bool read_touch_status(uint8_t touch_num)
{
    if (touch_num < 1 || touch_num > 4) {
        return false;
    }
    
    if (stm32_dev_handle == NULL) {
        return false;
    }
    
    uint8_t reg_addr;
    switch (touch_num) {
        case 1: reg_addr = TOUCH1_STATUS_REG_ADDR; break;
        case 2: reg_addr = TOUCH2_STATUS_REG_ADDR; break;
        case 3: reg_addr = TOUCH3_STATUS_REG_ADDR; break;
        case 4: reg_addr = TOUCH4_STATUS_REG_ADDR; break;
        default: return false;
    }
    
    uint8_t data;
    esp_err_t ret = bsp_i2c_read_reg(stm32_dev_handle, reg_addr, &data);
    if (ret != ESP_OK) {
        return false;
    }
    return (data & 0x01) != 0;
}

/**
 * @brief Touch detection task
 */
static void app_touch_detect(void *arg)
{
    ESP_LOGI(TAG, "Touch detection task started");
    
    while (1) {
        if (stm32_dev_handle == NULL) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

        // 读取所有触摸状态
        bool touch1_state = read_touch_status(1);
        bool touch2_state = read_touch_status(2);
        bool touch3_state = read_touch_status(3);
        bool touch4_state = read_touch_status(4);

        // ========== 左侧滑动检测（Touch1和Touch2）==========
        if ((touch1_state && !touch_state.touch_last_state[0]) || 
            (touch2_state && !touch_state.touch_last_state[1])) {
            if (touch_state.left_swipe_first == 0) {
                // 记录第一个按下的按键
                if (touch1_state && !touch_state.touch_last_state[0]) {
                    touch_state.left_swipe_first = 1;
                    touch_state.left_swipe_time = current_time;
                } else if (touch2_state && !touch_state.touch_last_state[1]) {
                    touch_state.left_swipe_first = 2;
                    touch_state.left_swipe_time = current_time;
                }
            } else {
                // 检测第二个按键是否在超时内按下
                if (current_time - touch_state.left_swipe_time <= TOUCH_SWIPE_TIMEOUT_MS) {
                    if (touch_state.left_swipe_first == 1 && touch2_state && !touch_state.touch_last_state[1]) {
                        // 1→2滑动：左侧上滑
                        bsp_touch_event_type_t event = BSP_TOUCH_EVENT_LEFT_UP;
                        for (uint8_t i = 0; i < touch_callback_count; i++) {
                            if (touch_callbacks[i]) {
                                touch_callbacks[i](event);
                            }
                        }
                        touch_state.left_swipe_first = 0;
                    } else if (touch_state.left_swipe_first == 2 && touch1_state && !touch_state.touch_last_state[0]) {
                        // 2→1滑动：左侧下滑
                        bsp_touch_event_type_t event = BSP_TOUCH_EVENT_LEFT_DOWN;
                        for (uint8_t i = 0; i < touch_callback_count; i++) {
                            if (touch_callbacks[i]) {
                                touch_callbacks[i](event);
                            }
                        }
                        touch_state.left_swipe_first = 0;
                    }
                } else {
                    // 超时，重置滑动检测
                    touch_state.left_swipe_first = 0;
                }
            }
        }
        
        // 如果Touch1或Touch2都释放，重置左侧滑动检测
        if (!touch1_state && !touch2_state && touch_state.left_swipe_first != 0) {
            touch_state.left_swipe_first = 0;
        }
        
        // 左侧滑动检测超时处理
        if (touch_state.left_swipe_first != 0 && 
            (current_time - touch_state.left_swipe_time > TOUCH_SWIPE_TIMEOUT_MS)) {
            touch_state.left_swipe_first = 0;
        }

        // ========== 右侧滑动检测（Touch3和Touch4）==========
        if ((touch3_state && !touch_state.touch_last_state[2]) || 
            (touch4_state && !touch_state.touch_last_state[3])) {
            if (touch_state.right_swipe_first == 0) {
                // 记录第一个按下的按键
                if (touch3_state && !touch_state.touch_last_state[2]) {
                    touch_state.right_swipe_first = 3;
                    touch_state.right_swipe_time = current_time;
                } else if (touch4_state && !touch_state.touch_last_state[3]) {
                    touch_state.right_swipe_first = 4;
                    touch_state.right_swipe_time = current_time;
                }
            } else {
                // 检测第二个按键是否在超时内按下
                if (current_time - touch_state.right_swipe_time <= TOUCH_SWIPE_TIMEOUT_MS) {
                    if (touch_state.right_swipe_first == 3 && touch4_state && !touch_state.touch_last_state[3]) {
                        // 3→4滑动：右侧下滑
                        bsp_touch_event_type_t event = BSP_TOUCH_EVENT_RIGHT_DOWN;
                        for (uint8_t i = 0; i < touch_callback_count; i++) {
                            if (touch_callbacks[i]) {
                                touch_callbacks[i](event);
                            }
                        }
                        touch_state.right_swipe_first = 0;
                    } else if (touch_state.right_swipe_first == 4 && touch3_state && !touch_state.touch_last_state[2]) {
                        // 4→3滑动：右侧上滑
                        bsp_touch_event_type_t event = BSP_TOUCH_EVENT_RIGHT_UP;
                        for (uint8_t i = 0; i < touch_callback_count; i++) {
                            if (touch_callbacks[i]) {
                                touch_callbacks[i](event);
                            }
                        }
                        touch_state.right_swipe_first = 0;
                    }
                } else {
                    // 超时，重置滑动检测
                    touch_state.right_swipe_first = 0;
                }
            }
        }
        
        // 如果Touch3或Touch4都释放，重置右侧滑动检测
        if (!touch3_state && !touch4_state && touch_state.right_swipe_first != 0) {
            touch_state.right_swipe_first = 0;
        }
        
        // 右侧滑动检测超时处理
        if (touch_state.right_swipe_first != 0 && 
            (current_time - touch_state.right_swipe_time > TOUCH_SWIPE_TIMEOUT_MS)) {
            touch_state.right_swipe_first = 0;
        }

        // 更新所有触摸状态
        touch_state.touch_last_state[0] = touch1_state;
        touch_state.touch_last_state[1] = touch2_state;
        touch_state.touch_last_state[2] = touch3_state;
        touch_state.touch_last_state[3] = touch4_state;

        vTaskDelay(pdMS_TO_TICKS(50)); // 50ms检测间隔
    }
}

esp_err_t bsp_touch_init(void)
{
    if (touch_state.initialized) {
        ESP_LOGW(TAG, "Touch already initialized");
        return ESP_OK;
    }

    // 确保STM32已初始化（通过bsp_rgb_init）
    if (stm32_dev_handle == NULL) {
        ESP_LOGE(TAG, "STM32 not initialized, please call bsp_rgb_init() first");
        return ESP_ERR_INVALID_STATE;
    }

    // 创建触摸检测任务
    BaseType_t ret = xTaskCreate(app_touch_detect, "app_touch_detect", 2048, NULL, 5, &touch_state.touch_task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create touch task");
        return ESP_FAIL;
    }

    touch_state.initialized = true;
    ESP_LOGI(TAG, "Touch detection initialized");
    return ESP_OK;
}

esp_err_t bsp_touch_add_event_callback(bsp_touch_callback_t cb)
{
    if (cb == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (touch_callback_count >= MAX_TOUCH_CALLBACKS) {
        ESP_LOGE(TAG, "Touch callback list is full");
        return ESP_ERR_NO_MEM;
    }

    touch_callbacks[touch_callback_count++] = cb;
    ESP_LOGI(TAG, "Touch callback added, total: %d", touch_callback_count);
    return ESP_OK;
}

//==============================================================================================
// Audio
//==============================================================================================
static i2s_chan_handle_t i2s_tx_chan = NULL;
static i2s_chan_handle_t i2s_rx_chan = NULL;
static const audio_codec_data_if_t *i2s_data_if = NULL;  /* Codec data interface */

static esp_codec_dev_handle_t play_dev_handle = NULL;
static esp_codec_dev_handle_t record_dev_handle = NULL;
static bsp_codec_config_t g_codec_handle;

/* Can be used for i2s_std_gpio_config_t and/or i2s_std_config_t initialization */
#define BSP_I2S_GPIO_CFG       \
    {                          \
        .mclk = BSP_I2S_MCLK,  \
        .bclk = BSP_I2S_BCLK,  \
        .ws = BSP_I2S_WR,      \
        .dout = BSP_I2S_DOUT,  \
        .din = BSP_I2S_DIN,    \
        .invert_flags = {      \
            .mclk_inv = false, \
            .bclk_inv = false, \
            .ws_inv = false,   \
        },                     \
    }

/* This configuration is used by default in bsp_i2s_init() */
#define BSP_I2S_DUPLEX_MONO_CFG(_sample_rate)                                                         \
    {                                                                                                 \
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(_sample_rate),                                          \
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO), \
        .gpio_cfg = BSP_I2S_GPIO_CFG,                                                                 \
    }

/**
 * I2S 初始化 
 */
static esp_err_t bsp_i2s_init(const i2s_std_config_t *i2s_std_cfg)
{
    esp_err_t ret = ESP_FAIL;

    ESP_LOGI(TAG, "bsp i2s init");
    if (i2s_tx_chan && i2s_rx_chan) { // Audio was initialized before
        return ESP_OK;
    }

    /* Setup I2S peripheral */
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(BSP_I2S_NUM, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true; // Auto clear the legacy data in the DMA buffer
    BSP_ERROR_CHECK_RETURN_ERR(i2s_new_channel(&chan_cfg, &i2s_tx_chan, &i2s_rx_chan));

    /* Setup I2S channels */
    const i2s_std_config_t std_cfg_default = BSP_I2S_DUPLEX_MONO_CFG(22050);
    const i2s_std_config_t *p_i2s_cfg = &std_cfg_default;
    if (i2s_std_cfg != NULL) {
        p_i2s_cfg = i2s_std_cfg;
    }

    if (i2s_tx_chan != NULL) {
        ESP_LOGI(TAG, "I2S tx chan enable");
        ESP_GOTO_ON_ERROR(i2s_channel_init_std_mode(i2s_tx_chan, p_i2s_cfg), err, TAG, "I2S channel initialization failed");
        ESP_GOTO_ON_ERROR(i2s_channel_enable(i2s_tx_chan), err, TAG, "I2S enabling failed");
    }
    if (i2s_rx_chan != NULL) {
        ESP_LOGI(TAG, "I2S rx chan enable");
        ESP_GOTO_ON_ERROR(i2s_channel_init_std_mode(i2s_rx_chan, p_i2s_cfg), err, TAG, "I2S channel initialization failed");
        ESP_GOTO_ON_ERROR(i2s_channel_enable(i2s_rx_chan), err, TAG, "I2S enabling failed");
    }

    audio_codec_i2s_cfg_t audio_i2s_cfg = {
        .port = BSP_I2S_NUM,
        .rx_handle = i2s_rx_chan,
        .tx_handle = i2s_tx_chan,
    };
    i2s_data_if = audio_codec_new_i2s_data(&audio_i2s_cfg);
    BSP_NULL_CHECK_GOTO(i2s_data_if, err);

    return ESP_OK;

err:
    if (i2s_tx_chan) {
        i2s_del_channel(i2s_tx_chan);
    }
    if (i2s_rx_chan) {
        i2s_del_channel(i2s_rx_chan);
    }

    return ret;
}

/**
 * 获取音频编解码的 I2S 数据接口 
 */
const audio_codec_data_if_t *bsp_audio_get_codec_itf(void)
{
    return i2s_data_if;
}

/**
 * 音频编解码模块 "扬声器" 初始化 ES8311
 */
static esp_codec_dev_handle_t bsp_audio_speaker_init(void)
{
    if (i2s_data_if == NULL) {
        /* Initialize I2C */
        BSP_ERROR_CHECK_RETURN_NULL(bsp_i2c_init(BSP_EXT_I2C_NUM, BSP_EXT_I2C_SCL, BSP_EXT_I2C_SDA));
        /* Initialize I2S */
        BSP_ERROR_CHECK_RETURN_NULL(bsp_i2s_init(NULL));
    }
    assert(i2s_data_if);

    const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();

    i2c_master_bus_handle_t i2c_bus = bsp_i2c_get_handle(BSP_EXT_I2C_NUM);
    if (i2c_bus == NULL) {
        ESP_LOGE(TAG, "I2C bus not initialized");
        return NULL;
    }

    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = BSP_EXT_I2C_NUM,
        .addr = ES8311_CODEC_DEFAULT_ADDR,
        .bus_handle = i2c_bus,
    };
    const audio_codec_ctrl_if_t *i2c_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    BSP_NULL_CHECK(i2c_ctrl_if, NULL);

    esp_codec_dev_hw_gain_t gain = {
        .pa_voltage = 5.0,
        .codec_dac_voltage = 3.3,
    };

    es8311_codec_cfg_t es8311_cfg = {
        .ctrl_if = i2c_ctrl_if,
        .gpio_if = gpio_if,
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC,
        .pa_pin = GPIO_NUM_NC,  // PA control is handled by AW87559
        .pa_reverted = false,
        .master_mode = false,
        .use_mclk = true,
        .digital_mic = false,
        .invert_mclk = false,
        .invert_sclk = false,
        .hw_gain = gain,
    };
    const audio_codec_if_t *es8311_dev = es8311_codec_new(&es8311_cfg);
    BSP_NULL_CHECK(es8311_dev, NULL);

    esp_codec_dev_cfg_t codec_dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = es8311_dev,
        .data_if = i2s_data_if,
    };
    return esp_codec_dev_new(&codec_dev_cfg);
}

/**
 * 音频编解码模块 "麦克风" 初始化 ES7210
 */
static esp_codec_dev_handle_t bsp_audio_microphone_init(void)
{
    if (i2s_data_if == NULL) {
        /* Initialize I2C */
        BSP_ERROR_CHECK_RETURN_NULL(bsp_i2c_init(BSP_EXT_I2C_NUM, BSP_EXT_I2C_SCL, BSP_EXT_I2C_SDA));
        /* Initialize I2S */
        BSP_ERROR_CHECK_RETURN_NULL(bsp_i2s_init(NULL));
    }
    assert(i2s_data_if);

    i2c_master_bus_handle_t i2c_bus = bsp_i2c_get_handle(BSP_EXT_I2C_NUM);
    if (i2c_bus == NULL) {
        ESP_LOGE(TAG, "I2C bus not initialized");
        return NULL;
    }

    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = BSP_EXT_I2C_NUM,
        .addr = ES7210_CODEC_DEFAULT_ADDR,
        .bus_handle = i2c_bus,
    };
    const audio_codec_ctrl_if_t *i2c_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    BSP_NULL_CHECK(i2c_ctrl_if, NULL);

    es7210_codec_cfg_t es7210_cfg = {
        .ctrl_if = i2c_ctrl_if,
    };
    const audio_codec_if_t *es7210_dev = es7210_codec_new(&es7210_cfg);
    BSP_NULL_CHECK(es7210_dev, NULL);

    esp_codec_dev_cfg_t codec_es7210_dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_IN,
        .codec_if = es7210_dev,
        .data_if = i2s_data_if,
    };
    return esp_codec_dev_new(&codec_es7210_dev_cfg);
}

/**
 * I2S 读数据
 */
static esp_err_t bsp_i2s_read(void *audio_buffer, size_t len, size_t *bytes_read, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;
    ret = esp_codec_dev_read(record_dev_handle, audio_buffer, len);
    *bytes_read = len;
    return ret;
}

/**
 * I2S 写数据
 */
static esp_err_t bsp_i2s_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;
    ret = esp_codec_dev_write(play_dev_handle, audio_buffer, len);
    *bytes_written = len;
    return ret;
}

/**
 * 设置音量
 */
static esp_err_t bsp_codec_set_volume(int volume)
{
    esp_err_t ret = ESP_OK;
    float v = volume;
    ret = esp_codec_dev_set_out_vol(play_dev_handle, (int)v);
    return ret;
}

/**
 * 设置静音
 */
static esp_err_t bsp_codec_set_mute(bool enable)
{
    esp_err_t ret = ESP_OK;
    ret = esp_codec_dev_set_out_mute(play_dev_handle, enable);
    return ret;
}

/**
 * 编解码重新配置（ES8311 + ES7210）
 */
static esp_err_t bsp_codec_reconfig(uint32_t rate, uint32_t bps, i2s_slot_mode_t ch)
{
    esp_err_t ret = ESP_OK;

    esp_codec_dev_sample_info_t fs = {
        .sample_rate = rate,
        .channel = ch,
        .bits_per_sample = bps,
    };

    if (record_dev_handle) {
        ret = esp_codec_dev_close(record_dev_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to close record device");
        }
    }
    if (ret == ESP_OK) {
        ret = esp_codec_dev_open(record_dev_handle, &fs);
    }

    if (play_dev_handle) {
        esp_err_t ret_play = esp_codec_dev_close(play_dev_handle);
        if (ret_play != ESP_OK) {
            ESP_LOGE(TAG, "Failed to close play device");
        }
        if (ret == ESP_OK) {
            ret = esp_codec_dev_open(play_dev_handle, &fs);
        }
    }

    return ret;
}

/**
 * 编解码重新配置（无参数版本，用于 codec_reconfig_fn）
 */
static esp_err_t bsp_codec_reconfig_no_param(void)
{
    // 使用默认配置重新配置
    return bsp_codec_reconfig(16000, 16, I2S_SLOT_MODE_MONO);
}

/**
 * 获取 codec handle
 */
bsp_codec_config_t *bsp_get_codec_handle(void)
{
    return &g_codec_handle;
}

/**
 * 音频编解码模块初始化
 */
void bsp_codec_init(void)
{
    bsp_pa_init();
    bsp_clock_generator_init();
    vTaskDelay(pdMS_TO_TICKS(100));

    play_dev_handle = bsp_audio_speaker_init();
    record_dev_handle = bsp_audio_microphone_init();
    
    if (record_dev_handle) {
        esp_codec_dev_set_in_gain(record_dev_handle, 60.0); // Set codec input gain
    }

    // 默认配置：16kHz, 16bit, 单声道
    bsp_codec_reconfig(16000, 16, I2S_SLOT_MODE_MONO);

    /* 初始化 codec handle */
    bsp_codec_config_t *codec_cfg = bsp_get_codec_handle();     // 获取 codec handle 
    codec_cfg->i2s_read = bsp_i2s_read;                         // I2S 读数据
    codec_cfg->i2s_write = bsp_i2s_write;                       // I2S 写数据
    codec_cfg->set_volume = bsp_codec_set_volume;               // 音量设置
    codec_cfg->set_mute = bsp_codec_set_mute;                   // 静音设置 
    codec_cfg->codec_reconfig_fn = bsp_codec_reconfig_no_param; // 编解码重新配置
    codec_cfg->i2s_reconfig_clk_fn = bsp_codec_reconfig;        // I2S 时钟重新配置
}

/**
 * 获取输入通道数
 */
int bsp_get_feed_channel(void)
{
    return 1; // 单声道输入
}

// 保持向后兼容的函数
esp_codec_dev_handle_t bsp_audio_codec_speaker_init(void)
{
    if (play_dev_handle == NULL) {
        bsp_codec_init();
    }
    return play_dev_handle;
}

esp_codec_dev_handle_t bsp_audio_codec_microphone_init(void)
{
    if (record_dev_handle == NULL) {
        bsp_codec_init();
    }
    return record_dev_handle;
}
