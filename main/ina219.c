/**
 * @file ina219.c
 * @brief Hiện thực giao tiếp cảm biến INA219 với I2C Master Driver ESP-IDF v5.5
 */

#include "ina219.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "INA219";

#define INA219_REG_CONFIG       0x00
#define INA219_REG_SHUNTVOLTAGE 0x01
#define INA219_REG_BUSVOLTAGE   0x02
#define INA219_REG_POWER        0x03
#define INA219_REG_CURRENT      0x04
#define INA219_REG_CALIBRATION  0x05

#define INA219_CONFIG_32V_2A    0x399F  // Cấu hình dải 32V, PGA 320mV, 12-bit mẫu
#define INA219_CALIBRATION_VAL  4096    // Calib cho Shunt 0.1 ohm, Current LSB 0.1mA

#define I2C_TIMEOUT_MS          100

static i2c_master_dev_handle_t s_ina219_dev = NULL;
static bool s_is_initialized = false;

static esp_err_t write_register(uint8_t reg, uint16_t value) {
    if (s_ina219_dev == NULL) return ESP_ERR_INVALID_STATE;
    uint8_t buf[3] = {
        reg,
        (uint8_t)((value >> 8) & 0xFF),
        (uint8_t)(value & 0xFF)
    };
    return i2c_master_transmit(s_ina219_dev, buf, sizeof(buf), I2C_TIMEOUT_MS);
}

static esp_err_t read_register(uint8_t reg, int16_t *value) {
    if (s_ina219_dev == NULL || value == NULL) return ESP_ERR_INVALID_STATE;
    uint8_t rx_buf[2] = {0};
    esp_err_t ret = i2c_master_transmit_receive(s_ina219_dev, &reg, 1, rx_buf, 2, I2C_TIMEOUT_MS);
    if (ret == ESP_OK) {
        *value = (int16_t)((rx_buf[0] << 8) | rx_buf[1]);
    }
    return ret;
}

esp_err_t ina219_init(i2c_master_bus_handle_t bus_handle, uint8_t i2c_addr) {
    if (bus_handle == NULL) return ESP_ERR_INVALID_ARG;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = i2c_addr,
        .scl_speed_hz    = 100000,
    };

    esp_err_t ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &s_ina219_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Them INA219 vao I2C bus that bai: %s", esp_err_to_name(ret));
        return ret;
    }

    // Ghi hệ số hiệu chuẩn và cấu hình hoạt động
    ret = write_register(INA219_REG_CALIBRATION, INA219_CALIBRATION_VAL);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Khong the ghi vao INA219 CALIBRATION (kiem tra ket noi phan cung 0x%02X)", i2c_addr);
        return ret;
    }

    ret = write_register(INA219_REG_CONFIG, INA219_CONFIG_32V_2A);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Khong the ghi vao INA219 CONFIG");
        return ret;
    }

    s_is_initialized = true;
    ESP_LOGI(TAG, "Khoi tao cam bien INA219 (0x%02X) thanh cong!", i2c_addr);
    return ESP_OK;
}

esp_err_t ina219_read_all(ina219_data_t *data) {
    if (!s_is_initialized || data == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    int16_t raw_bus = 0;
    int16_t raw_shunt = 0;
    int16_t raw_current = 0;
    int16_t raw_power = 0;

    // Đọc Bus Voltage: bit [15:3] nhân với 4mV
    esp_err_t ret = read_register(INA219_REG_BUSVOLTAGE, &raw_bus);
    if (ret != ESP_OK) return ret;
    data->bus_voltage_v = ((raw_bus >> 3) * 4) * 0.001f;

    // Đọc Shunt Voltage: 1 LSB = 10uV = 0.01mV
    ret = read_register(INA219_REG_SHUNTVOLTAGE, &raw_shunt);
    if (ret != ESP_OK) return ret;
    data->shunt_voltage_mv = raw_shunt * 0.01f;

    // Ghi lại hệ số calib trước khi đọc dòng (theo datasheet INA219 phòng trường hợp chip reset)
    write_register(INA219_REG_CALIBRATION, INA219_CALIBRATION_VAL);

    // Đọc Current: 1 LSB = 0.1mA
    ret = read_register(INA219_REG_CURRENT, &raw_current);
    if (ret != ESP_OK) return ret;
    data->current_ma = raw_current * 0.1f;

    // Đọc Power: 1 LSB = 2mW (hoặc tự tính: Bus_V * Current_mA)
    ret = read_register(INA219_REG_POWER, &raw_power);
    if (ret == ESP_OK) {
        data->power_mw = raw_power * 2.0f;
    } else {
        data->power_mw = data->bus_voltage_v * data->current_ma;
    }

    return ESP_OK;
}
