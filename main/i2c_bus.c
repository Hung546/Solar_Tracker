/**
 * @file i2c_bus.c
 * @brief Hiện thực khởi tạo Bus I2C Master trên ESP32 DevKit V4
 */

#include "i2c_bus.h"
#include "esp_log.h"

static const char *TAG = "I2C_BUS";

esp_err_t i2c_bus_init(i2c_master_bus_handle_t *out_bus_handle) {
    if (out_bus_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_BUS_PORT,
        .sda_io_num = I2C_BUS_SDA_PIN,
        .scl_io_num = I2C_BUS_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t ret = i2c_new_master_bus(&bus_config, out_bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Khoi tao I2C Master that bai: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "I2C Master khoi tao thanh cong (SDA: GPIO %d, SCL: GPIO %d, Speed: %d Hz)",
             I2C_BUS_SDA_PIN, I2C_BUS_SCL_PIN, I2C_BUS_FREQ_HZ);
    return ESP_OK;
}
