/**
 * @file ina219.h
 * @brief Thư viện giao tiếp cảm biến đo dòng, áp và công suất INA219 qua I2C (ESP-IDF v5.5)
 */

#ifndef INA219_H
#define INA219_H

#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

#define INA219_I2C_ADDR_DEFAULT     0x40    // Địa chỉ I2C mặc định (A0, A1 nối GND)

typedef struct {
    float bus_voltage_v;    // Điện áp tải/nguồn (V)
    float shunt_voltage_mv; // Điện áp rơi trên điện trở shunt (mV)
    float current_ma;       // Dòng điện tức thời (mA)
    float power_mw;         // Công suất tiêu thụ/phát (mW)
} ina219_data_t;

/**
 * @brief Khởi tạo cảm biến INA219 trên bus I2C
 * @param bus_handle Handle của I2C Master Bus đã được khởi tạo
 * @param i2c_addr Địa chỉ I2C của module INA219 (mặc định 0x40)
 * @return ESP_OK nếu khởi tạo thành công
 */
esp_err_t ina219_init(i2c_master_bus_handle_t bus_handle, uint8_t i2c_addr);

/**
 * @brief Đọc toàn bộ các thông số: Bus Voltage, Shunt Voltage, Current và Power
 * @param[out] data Con trỏ lưu trữ dữ liệu đọc được
 * @return ESP_OK nếu đọc thành công
 */
esp_err_t ina219_read_all(ina219_data_t *data);

#endif // INA219_H
