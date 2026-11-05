/**
 * @file oled_ssd1306.h
 * @brief Thư viện hiển thị màn hình OLED SSD1306 128x64 qua I2C (ESP-IDF v5.5)
 */

#ifndef OLED_SSD1306_H
#define OLED_SSD1306_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

#define OLED_I2C_ADDR_DEFAULT   0x3C    // Địa chỉ I2C thông dụng của màn OLED SSD1306
#define OLED_WIDTH              128
#define OLED_HEIGHT             64

/**
 * @brief Khởi tạo màn hình OLED SSD1306 trên bus I2C
 * @param bus_handle Handle của I2C Master Bus
 * @param i2c_addr Địa chỉ I2C của OLED (mặc định 0x3C)
 * @return ESP_OK nếu thành công
 */
esp_err_t oled_ssd1306_init(i2c_master_bus_handle_t bus_handle, uint8_t i2c_addr);

/**
 * @brief Xóa toàn bộ bộ đệm khung hình (Buffer)
 */
void oled_clear(void);

/**
 * @brief Đẩy dữ liệu bộ đệm khung hình lên màn hình OLED
 * @return ESP_OK nếu gửi dữ liệu thành công
 */
esp_err_t oled_update(void);

/**
 * @brief Vẽ một điểm ảnh (Pixel)
 */
void oled_draw_pixel(int x, int y, bool on);

/**
 * @brief Vẽ một ký tự ASCII kích thước 8x8
 */
void oled_draw_char(int x, int y, char c);

/**
 * @brief Vẽ chuỗi ký tự ASCII
 */
void oled_draw_string(int x, int y, const char *str);

/**
 * @brief Vẽ đường kẻ ngang
 */
void oled_draw_hline(int x, int y, int w);

/**
 * @brief Hiển thị giao diện Dashboard chuyên dụng cho Solar Tracker
 * @param az_angle Góc trục ngang Azimuth
 * @param el_angle Góc trục đứng Elevation
 * @param volt Điện áp tấm pin/pin sạc (V)
 * @param current Dòng điện (mA)
 * @param power Công suất (mW)
 * @param status Chuỗi trạng thái ("TRACKING", "NIGHT", "BALANCED",...)
 */
void oled_show_solar_dashboard(float az_angle, float el_angle, float volt, float current, float power, const char *status);

#endif // OLED_SSD1306_H
