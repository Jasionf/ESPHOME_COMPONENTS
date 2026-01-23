/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ESP BSP: M5Stack AtomS3
 */

#pragma once

#include "sdkconfig.h"
#include "driver/gpio.h"
#include "esp_vfs_fat.h"
#include "bsp/config.h"
#include "bsp/display.h"
#include "bsp/bsp_codec.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "iot_button.h"


#ifdef __cplusplus
extern "C" {
#endif

#define BSP_BOARD_M5STACK_ATOMS3

// 内部 I2C 配置
#define BSP_SYS_I2C_NUM     I2C_NUM_0
#define BSP_SYS_I2C_SCL     GPIO_NUM_0
#define BSP_SYS_I2C_SDA     GPIO_NUM_45

// 外部 I2C 配置（用于 Echo Pyramid 底座）
#define BSP_EXT_I2C_NUM     I2C_NUM_1
#define BSP_EXT_I2C_SCL     GPIO_NUM_39
#define BSP_EXT_I2C_SDA     GPIO_NUM_38

// LCD 配置
#define BSP_LCD_MOSI        GPIO_NUM_21
#define BSP_LCD_MISO        GPIO_NUM_NC
#define BSP_LCD_PCLK        GPIO_NUM_15
#define BSP_LCD_CS          GPIO_NUM_14
#define BSP_LCD_DC          GPIO_NUM_42
#define BSP_LCD_RST         GPIO_NUM_48
#define BSP_LCD_BACKLIGHT   GPIO_NUM_NC

// Button 配置
#define BSP_BUTTON_NUM      GPIO_NUM_41


// EchoPyramid 配置
// STM32 RGB 配置
#define STM32_I2C_ADDR              0x1A
#define RGB1_BRIGHTNESS_REG_ADDR    0x10  // 灯带1亮度
#define RGB2_BRIGHTNESS_REG_ADDR    0x11  // 灯带2亮度
#define RGB_CH1_I1_COLOR_REG_ADDR   0x20  // Channel 0, 灯带1组1, LED 0, 每个LED 4字节
#define RGB_CH2_I1_COLOR_REG_ADDR   0x3C  // Channel 1, 灯带1组2, LED 0, 每个LED 4字节
#define RGB_CH3_I1_COLOR_REG_ADDR   0x60  // Channel 2, 灯带2组1, LED 0, 每个LED 4字节
#define RGB_CH4_I1_COLOR_REG_ADDR   0x7C  // Channel 3, 灯带2组2, LED 0, 每个LED 4字节
#define NUM_RGB_CHANNELS            4     // 4个通道（2条灯带 × 2组）
#define NUM_LEDS_PER_GROUP          7     // 每组7个LED
// Aw87559 Audio Amplifier 配置
#define AW87559_I2C_ADDR        0x5B
// Si5351 Clock Generator 配置
#define SI5351_I2C_ADDR         0x60
// I2S
#define BSP_I2S_NUM     I2S_NUM_0
#define BSP_I2S_MCLK    GPIO_NUM_NC  // 主时钟（NC表示不使用）
#define BSP_I2S_BCLK    GPIO_NUM_6   // 位时钟
#define BSP_I2S_WR      GPIO_NUM_8   // 字(声道)选择
#define BSP_I2S_DOUT    GPIO_NUM_7   // 数据输出
#define BSP_I2S_DIN     GPIO_NUM_5   // 数据输入
#define BSP_PA_PIN      GPIO_NUM_NC  // 功放控制（NC表示不使用）



//==============================================================================================
// I2C
//==============================================================================================
// I2C 扫描结果结构体
typedef struct {
    uint8_t addresses[128];  // 找到的设备地址列表
    size_t count;            // 找到的设备数量
    bool scanned;            // 是否已扫描
} bsp_i2c_scan_result_t;

esp_err_t bsp_i2c_init(i2c_port_t i2c_num, gpio_num_t i2c_scl, gpio_num_t i2c_sda);
esp_err_t bsp_i2c_deinit(i2c_port_t i2c_num);
i2c_master_bus_handle_t bsp_i2c_get_handle(i2c_port_t i2c_num);
esp_err_t bsp_i2c_scan(i2c_port_t i2c_num, bsp_i2c_scan_result_t* result);
bool bsp_i2c_is_device_found(i2c_port_t i2c_num, uint8_t device_addr);
esp_err_t bsp_i2c_write_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t data);
esp_err_t bsp_i2c_read_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t *data);
esp_err_t bsp_i2c_write_regs(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t *data, size_t len);
 

//==============================================================================================
// LCD
//==============================================================================================
#define BSP_LCD_PIXEL_CLOCK_HZ     (40 * 1000 * 1000)
#define BSP_LCD_SPI_NUM            (SPI2_HOST)
#define BSP_LCD_DRAW_BUFF_SIZE     (BSP_LCD_H_RES * 50)
#define BSP_LCD_DRAW_BUFF_DOUBLE   (1)

/**
 * @brief BSP display configuration structure
 */
typedef struct {
    lvgl_port_cfg_t lvgl_port_cfg;  /*!< LVGL port configuration */
    uint32_t        buffer_size;    /*!< Size of the buffer for the screen in pixels */
    bool            double_buffer;  /*!< True, if should be allocated two buffers */
    struct {
        unsigned int buff_dma: 1;    /*!< Allocated LVGL buffer will be DMA capable */
        unsigned int buff_spiram: 1; /*!< Allocated LVGL buffer will be in PSRAM */
    } flags;
} bsp_display_cfg_t;
/**
 * @brief Initialize display
 *
 * This function initializes SPI, display controller and starts LVGL handling task.
 * LCD backlight must be enabled separately by calling bsp_display_brightness_set()
 *
 * @return Pointer to LVGL display or NULL when error occured
 */
lv_display_t *bsp_display_start(void);

/**
 * @brief Initialize display
 *
 * This function initializes SPI, display controller and starts LVGL handling task.
 * LCD backlight must be enabled separately by calling bsp_display_brightness_set()
 *
 * @param cfg display configuration
 *
 * @return Pointer to LVGL display or NULL when error occured
 */
lv_display_t *bsp_display_start_with_config(const bsp_display_cfg_t *cfg);

/**
 * @brief Get pointer to input device (touch, buttons, ...)
 *
 * @note The LVGL input device is initialized in bsp_display_start() function.
 *
 * @return Pointer to LVGL input device or NULL when not initialized
 */
lv_indev_t *bsp_display_get_input_dev(void);

/**
 * @brief Take LVGL mutex
 *
 * @param timeout_ms Timeout in [ms]. 0 will block indefinitely.
 * @return true  Mutex was taken
 * @return false Mutex was NOT taken
 */
bool bsp_display_lock(uint32_t timeout_ms);

/**
 * @brief Give LVGL mutex
 *
 */
void bsp_display_unlock(void);

/**
 * @brief Rotate screen
 *
 * Display must be already initialized by calling bsp_display_start()
 *
 * @param[in] disp Pointer to LVGL display
 * @param[in] rotation Angle of the display rotation
 */
void bsp_display_rotate(lv_display_t *disp, lv_display_rotation_t rotation);


//==============================================================================================
// Button
//==============================================================================================

/**
 * @brief Initialize button
 *
 * @return
 *      - ESP_OK                On success
 *      - ESP_ERR_INVALID_STATE Button already initialized
 *      - ESP_FAIL              Failed to create button
 */
esp_err_t bsp_button_init(void);

/**
 * @brief Register button callback
 *
 * @param[in] event     Button event type
 * @param[in] cb        Callback function
 * @param[in] usr_data  User data passed to callback
 * @return
 *      - ESP_OK                On success
 *      - ESP_ERR_INVALID_STATE  Button not initialized
 */
esp_err_t bsp_button_register_cb(button_event_t event, button_cb_t cb, void *usr_data);

/**
 * @brief Get button handle
 *
 * @return Button handle or NULL if not initialized
 */
button_handle_t bsp_button_get_handle(void);



//==============================================================================================
// STM32 RGB
//==============================================================================================

/**
 * @brief Initialize STM32 RGB controller
 *
 * @return
 *      - ESP_OK                On success
 *      - ESP_ERR_INVALID_STATE I2C bus not initialized
 *      - ESP_FAIL              Failed to create STM32 device
 */
esp_err_t bsp_rgb_init(void);

/**
 * @brief Set RGB strip brightness
 *
 * @param[in] strip       Strip index (0-1)
 * @param[in] brightness  Brightness value (0-100)
 * @return
 *      - ESP_OK                On success
 *      - ESP_ERR_INVALID_STATE STM32 not initialized
 *      - ESP_ERR_INVALID_ARG   Invalid strip index
 */
esp_err_t bsp_set_rgb_brightness(uint8_t strip, uint8_t brightness);

/**
 * @brief Set RGB LED color
 *
 * @param[in] channel  Channel index (0-3)
 * @param[in] index    LED index within channel (0-6)
 * @param[in] color    Color value (32-bit, format: 0x00RRGGBB)
 * @return
 *      - ESP_OK                On success
 *      - ESP_ERR_INVALID_STATE STM32 not initialized
 *      - ESP_ERR_INVALID_ARG   Invalid channel or index
 */
esp_err_t bsp_set_rgb_color(uint8_t channel, uint8_t index, uint32_t color);


//==============================================================================================
// STM32 Touch
//==============================================================================================

// STM32 Touch register addresses
#define TOUCH1_STATUS_REG_ADDR      0x00
#define TOUCH2_STATUS_REG_ADDR      0x01
#define TOUCH3_STATUS_REG_ADDR      0x02
#define TOUCH4_STATUS_REG_ADDR      0x03

// Touch event types
typedef enum {
    BSP_TOUCH_EVENT_LEFT_UP,      // 左侧上滑 (Touch1→Touch2)
    BSP_TOUCH_EVENT_LEFT_DOWN,    // 左侧下滑 (Touch2→Touch1)
    BSP_TOUCH_EVENT_RIGHT_UP,     // 右侧上滑 (Touch4→Touch3)
    BSP_TOUCH_EVENT_RIGHT_DOWN    // 右侧下滑 (Touch3→Touch4)
} bsp_touch_event_type_t;

// Touch callback type
typedef void (*bsp_touch_callback_t)(bsp_touch_event_type_t event_type);

/**
 * @brief Initialize touch detection
 * 
 * This function creates a touch detection task that monitors STM32 touch buttons.
 *
 * @return
 *      - ESP_OK                On success
 *      - ESP_ERR_INVALID_STATE STM32 not initialized
 *      - ESP_FAIL              Failed to create touch task
 */
esp_err_t bsp_touch_init(void);

/**
 * @brief Add touch event callback
 * 
 * @param[in] cb Callback function: void callback(bsp_touch_event_type_t event_type)
 * @return
 *      - ESP_OK                On success
 *      - ESP_ERR_INVALID_ARG   Invalid callback
 *      - ESP_ERR_NO_MEM        Callback list is full
 */
esp_err_t bsp_touch_add_event_callback(bsp_touch_callback_t cb);

//==============================================================================================
// Audio
//==============================================================================================

/**
 * @brief Initialize AW87559 Audio Amplifier
 *
 * @return
 *      - ESP_OK                On success
 *      - ESP_ERR_INVALID_STATE I2C bus not initialized
 *      - ESP_FAIL              Failed to initialize AW87559
 */
esp_err_t bsp_pa_init(void);

/**
 * @brief Initialize Si5351 Clock Generator
 *
 * @return
 *      - ESP_OK                On success
 *      - ESP_ERR_INVALID_STATE I2C bus not initialized
 *      - ESP_FAIL              Failed to initialize Si5351
 */
esp_err_t bsp_clock_generator_init(void);

 


#ifdef __cplusplus
}
#endif
