/**
 * @file i2c_bus.h
 * @brief Khởi tạo và quản lý Bus I2C dùng chung cho INA219 và OLED SSD1306 (ESP-IDF v5.5)
 */

#ifndef I2C_BUS_H
#define I2C_BUS_H

#include "esp_err.h"
#include "driver/i2c_master.h"

// Chân I2C mặc định trên ESP32 DevKit V4
#define I2C_BUS_PORT        I2C_NUM_0
#define I2C_BUS_SDA_PIN     GPIO_NUM_21
#define I2C_BUS_SCL_PIN     GPIO_NUM_22
#define I2C_BUS_FREQ_HZ     400000 // 400kHz Fast-mode

/**
 * @brief Khởi tạo Bus I2C Master
 * @param[out] out_bus_handle Con trỏ lưu handle của I2C master bus
 * @return ESP_OK nếu thành công
 */
esp_err_t i2c_bus_init(i2c_master_bus_handle_t *out_bus_handle);

#endif // I2C_BUS_H
