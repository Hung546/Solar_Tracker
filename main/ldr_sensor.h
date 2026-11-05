/**
 * @file ldr_sensor.h
 * @brief Thư viện đọc và xử lý tín hiệu 4 quang trở (LDR) theo 4 góc phần tư
 */

#ifndef LDR_SENSOR_H
#define LDR_SENSOR_H

#include <stdbool.h>
#include "esp_err.h"
#include "hal/gpio_types.h"

// ==================== CẤU HÌNH CHÂN QUANG TRỞ (LDR) ====================
#define LDR_TL_GPIO         GPIO_NUM_34 // Trên - Trái (Top-Left)
#define LDR_TR_GPIO         GPIO_NUM_35 // Trên - Phải (Top-Right)
#define LDR_BL_GPIO         GPIO_NUM_32 // Dưới - Trái (Bottom-Left)
#define LDR_BR_GPIO         GPIO_NUM_33 // Dưới - Phải (Bottom-Right)

#define LDR_DEFAULT_DEADBAND    100     // Ngưỡng sai số cân bằng ánh sáng
#define LDR_NIGHT_THRESHOLD     300     // Ngưỡng trời tối (giá trị ADC trung bình < 300)

typedef enum {
    LDR_V_BALANCED = 0,
    LDR_V_TOP      = 1,
    LDR_V_BOTTOM   = 2
} ldr_dir_v_t;

typedef enum {
    LDR_H_BALANCED = 0,
    LDR_H_LEFT     = 1,
    LDR_H_RIGHT    = 2
} ldr_dir_h_t;

typedef struct {
    int tl_raw;     // Top-Left (Trên-Trái)
    int tr_raw;     // Top-Right (Trên-Phải)
    int bl_raw;     // Bottom-Left (Dưới-Trái)
    int br_raw;     // Bottom-Right (Dưới-Phải)

    int tl_mv;      // Điện áp mV
    int tr_mv;
    int bl_mv;
    int br_mv;

    int avg_top;    // Trung bình nửa trên
    int avg_bottom; // Trung bình nửa dưới
    int avg_left;   // Trung bình nửa trái
    int avg_right;  // Trung bình nửa phải

    int diff_v;     // Sai lệch trục dọc: Top - Bottom
    int diff_h;     // Sai lệch trục ngang: Left - Right
    int avg_total;  // Cường độ sáng trung bình toàn hệ thống

    ldr_dir_v_t dir_v; // Hướng lệch trục đứng
    ldr_dir_h_t dir_h; // Hướng lệch trục ngang
    bool is_night;     // Cờ phát hiện ban đêm / thiếu sáng
} ldr_readings_t;

/**
 * @brief Khởi tạo hệ thống đọc ADC cho 4 quang trở
 * @return ESP_OK nếu thành công
 */
esp_err_t ldr_sensor_init(void);

/**
 * @brief Đọc giá trị từ 4 quang trở, lọc nhiễu và phân tích hướng lệch
 * @param[out] out Con trỏ lưu kết quả đo
 * @return ESP_OK nếu đọc thành công
 */
esp_err_t ldr_sensor_read(ldr_readings_t *out);

#endif // LDR_SENSOR_H
