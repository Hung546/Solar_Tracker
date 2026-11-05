/**
 * @file oled_ssd1306.c
 * @brief Hiện thực driver màn hình OLED SSD1306 giao tiếp I2C ESP-IDF v5.5
 */

#include "oled_ssd1306.h"
#include "font8x8.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "OLED_SSD1306";

#define I2C_TIMEOUT_MS  100

static i2c_master_dev_handle_t s_oled_dev = NULL;
static uint8_t s_framebuffer[OLED_WIDTH * OLED_HEIGHT / 8]; // 1024 bytes
static bool s_is_initialized = false;

static esp_err_t send_cmd(uint8_t cmd) {
    if (s_oled_dev == NULL) return ESP_ERR_INVALID_STATE;
    uint8_t buf[2] = {0x00, cmd}; // 0x00: Co = 0, D/C# = 0 (Command)
    return i2c_master_transmit(s_oled_dev, buf, sizeof(buf), I2C_TIMEOUT_MS);
}

static esp_err_t send_cmd_list(const uint8_t *cmds, size_t len) {
    for (size_t i = 0; i < len; i++) {
        esp_err_t ret = send_cmd(cmds[i]);
        if (ret != ESP_OK) return ret;
    }
    return ESP_OK;
}

esp_err_t oled_ssd1306_init(i2c_master_bus_handle_t bus_handle, uint8_t i2c_addr) {
    if (bus_handle == NULL) return ESP_ERR_INVALID_ARG;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = i2c_addr,
        .scl_speed_hz    = 400000,
    };

    esp_err_t ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &s_oled_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Them OLED vao I2C bus that bai: %s", esp_err_to_name(ret));
        return ret;
    }

    // Trình tự khởi động SSD1306 chuẩn
    const uint8_t init_cmds[] = {
        0xAE,       // Display OFF
        0xD5, 0x80, // Set Display Clock Divide Ratio/Oscillator Frequency
        0xA8, 0x3F, // Set Multiplex Ratio (64 lines)
        0xD3, 0x00, // Set Display Offset (0)
        0x40,       // Set Display Start Line (0)
        0x8D, 0x14, // Charge Pump Setting (Enable charge pump 7.5V)
        0x20, 0x00, // Memory Addressing Mode: Horizontal Addressing Mode
        0xA1,       // Set Segment Re-map (A0/A1 flip horizontal)
        0xC8,       // Set COM Output Scan Direction (C0/C8 flip vertical)
        0xDA, 0x12, // Set COM Pins Hardware Configuration
        0x81, 0xCF, // Set Contrast Control
        0xD9, 0xF1, // Set Pre-charge Period
        0xDB, 0x40, // Set VCOMH Deselect Level
        0xA4,       // Entire Display ON (Resume to RAM content)
        0xA6,       // Set Normal Display (0: tắt, 1: sáng)
        0xAF        // Display ON
    };

    ret = send_cmd_list(init_cmds, sizeof(init_cmds));
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Khong the gui lenh khoi tao OLED (kiem tra ket noi phan cung 0x%02X)", i2c_addr);
        return ret;
    }

    s_is_initialized = true;
    oled_clear();
    oled_update();
    ESP_LOGI(TAG, "Khoi tao man hinh OLED SSD1306 (0x%02X) thanh cong!", i2c_addr);
    return ESP_OK;
}

void oled_clear(void) {
    memset(s_framebuffer, 0, sizeof(s_framebuffer));
}

esp_err_t oled_update(void) {
    if (!s_is_initialized || s_oled_dev == NULL) return ESP_ERR_INVALID_STATE;

    // Thiết lập vùng vẽ toàn màn hình (Cột 0-127, Page 0-7)
    const uint8_t page_cmds[] = {
        0x21, 0, 127, // Column Address: 0 - 127
        0x22, 0, 7    // Page Address: 0 - 7
    };
    esp_err_t ret = send_cmd_list(page_cmds, sizeof(page_cmds));
    if (ret != ESP_OK) return ret;

    // Gửi từng Page (128 bytes) kèm byte tiền tố 0x40 (Data prefix)
    uint8_t page_buf[129];
    page_buf[0] = 0x40; // Control byte cho dữ liệu RAM hiển thị

    for (int p = 0; p < 8; p++) {
        memcpy(&page_buf[1], &s_framebuffer[p * OLED_WIDTH], OLED_WIDTH);
        ret = i2c_master_transmit(s_oled_dev, page_buf, sizeof(page_buf), I2C_TIMEOUT_MS);
        if (ret != ESP_OK) return ret;
    }

    return ESP_OK;
}

void oled_draw_pixel(int x, int y, bool on) {
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;

    int page = y / 8;
    int bit = y % 8;
    int idx = page * OLED_WIDTH + x;

    if (on) {
        s_framebuffer[idx] |= (1 << bit);
    } else {
        s_framebuffer[idx] &= ~(1 << bit);
    }
}

void oled_draw_char(int x, int y, char c) {
    if (c < 0x20 || c > 0x7E) c = ' ';
    int font_idx = c - 0x20;

    for (int col = 0; col < 8; col++) {
        uint8_t line = font8x8_basic[font_idx][col];
        for (int row = 0; row < 8; row++) {
            bool on = (line >> row) & 0x01;
            oled_draw_pixel(x + col, y + row, on);
        }
    }
}

void oled_draw_string(int x, int y, const char *str) {
    if (str == NULL) return;
    int cur_x = x;
    while (*str) {
        if (cur_x + 8 > OLED_WIDTH) break;
        oled_draw_char(cur_x, y, *str);
        cur_x += 8;
        str++;
    }
}

void oled_draw_hline(int x, int y, int w) {
    for (int i = 0; i < w; i++) {
        oled_draw_pixel(x + i, y, true);
    }
}

void oled_show_solar_dashboard(float az_angle, float el_angle, float volt, float current, float power, const char *status) {
    if (!s_is_initialized) return;

    oled_clear();

    // Hàng 1 (y = 0): Tiêu đề
    oled_draw_string(4, 0, "SOLAR TRACKER");
    oled_draw_hline(0, 10, OLED_WIDTH);

    // Hàng 2 (y = 14): Góc xoay 2 trục
    char buf[24];
    snprintf(buf, sizeof(buf), "AZ:%3.0f EL:%3.0f", az_angle, el_angle);
    oled_draw_string(0, 14, buf);

    // Hàng 3 (y = 26): Điện áp & Dòng điện đo từ tấm pin qua INA219
    snprintf(buf, sizeof(buf), "V:%4.2fV I:%3.0fmA", volt, current);
    oled_draw_string(0, 26, buf);

    // Hàng 4 (y = 38): Công suất phát ra (mW)
    snprintf(buf, sizeof(buf), "P:%5.1fmW", power);
    oled_draw_string(0, 38, buf);

    oled_draw_hline(0, 48, OLED_WIDTH);

    // Hàng 5 (y = 52): Trạng thái hoạt động
    snprintf(buf, sizeof(buf), "[%s]", status ? status : "RUN");
    oled_draw_string(0, 52, buf);

    oled_update();
}
