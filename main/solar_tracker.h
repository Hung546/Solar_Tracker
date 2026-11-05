/**
 * @file solar_tracker.h
 * @brief Bộ điều khiển thuật toán bám nhật 2 trục (Solar Tracking Controller)
 */

#ifndef SOLAR_TRACKER_H
#define SOLAR_TRACKER_H

#include <stdbool.h>
#include "ldr_sensor.h"
#include "servo_control.h"

typedef enum {
    TRACKER_MODE_AUTO = 0,      // Tự động bám theo ánh sáng mặt trời
    TRACKER_MODE_NIGHT = 1,     // Chế độ ban đêm: quay về hướng Đông chờ sáng
    TRACKER_MODE_MANUAL = 2     // Chế độ điều khiển góc thủ công
} tracker_mode_t;

typedef struct {
    int deadband;               // Ngưỡng dung sai ánh sáng tránh rung servo
    float step_size;            // Bước dời góc mỗi chu kỳ (độ)
    int night_threshold;        // Ngưỡng phát hiện trời tối (ADC TB < threshold)
    float home_az_angle;        // Góc đón bình minh trục ngang (Đông)
    float home_el_angle;        // Góc đón bình minh trục đứng
} solar_tracker_config_t;

typedef struct {
    tracker_mode_t mode;
    float current_az;           // Góc hiện tại trục ngang Azimuth
    float current_el;           // Góc hiện tại trục đứng Elevation
    char status_str[16];        // Chuỗi mô tả trạng thái ("TRACKING", "BALANCED", "NIGHT")
} solar_tracker_state_t;

/**
 * @brief Khởi tạo bộ điều khiển Solar Tracker
 * @param cfg Cấu hình tham số (truyền NULL để dùng mặc định)
 * @return ESP_OK nếu thành công
 */
esp_err_t solar_tracker_init(const solar_tracker_config_t *cfg);

/**
 * @brief Thực thi một chu kỳ tính toán và điều khiển bám nhật
 * @param ldr Dữ liệu đọc được từ 4 quang trở
 * @param[out] out_state Con trỏ nhận trạng thái mới sau khi cập nhật
 * @return ESP_OK nếu xử lý thành công
 */
esp_err_t solar_tracker_process(const ldr_readings_t *ldr, solar_tracker_state_t *out_state);

/**
 * @brief Lấy trạng thái hiện tại của bộ bám nhật
 */
void solar_tracker_get_state(solar_tracker_state_t *out_state);

#endif // SOLAR_TRACKER_H
