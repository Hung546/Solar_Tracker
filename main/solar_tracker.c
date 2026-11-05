/**
 * @file solar_tracker.c
 * @brief Hiện thực thuật toán bám nhật 2 trục và máy trạng thái (State Machine)
 */

#include "solar_tracker.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "SOLAR_TRACKER";

static solar_tracker_config_t s_config = {
    .deadband = 100,
    .step_size = 1.0f,
    .night_threshold = 300,
    .home_az_angle = 45.0f, // Góc hướng Đông
    .home_el_angle = 60.0f  // Góc ngẩng đón mặt trời mọc
};

static solar_tracker_state_t s_state = {
    .mode = TRACKER_MODE_AUTO,
    .current_az = SERVO_AZ_DEFAULT_ANGLE,
    .current_el = SERVO_EL_DEFAULT_ANGLE,
    .status_str = "INIT"
};

esp_err_t solar_tracker_init(const solar_tracker_config_t *cfg) {
    if (cfg != NULL) {
        s_config = *cfg;
    }
    s_state.current_az = servo_get_angle(SERVO_ID_AZIMUTH);
    s_state.current_el = servo_get_angle(SERVO_ID_ELEVATION);
    strncpy(s_state.status_str, "READY", sizeof(s_state.status_str));

    ESP_LOGI(TAG, "Khoi tao Solar Tracker Controller thanh cong (Deadband: %d, Step: %.1f deg)",
             s_config.deadband, s_config.step_size);
    return ESP_OK;
}

esp_err_t solar_tracker_process(const ldr_readings_t *ldr, solar_tracker_state_t *out_state) {
    if (ldr == NULL) return ESP_ERR_INVALID_ARG;

    // 1. Kiểm tra điều kiện ban đêm / thiếu sáng
    if (ldr->avg_total < s_config.night_threshold) {
        s_state.mode = TRACKER_MODE_NIGHT;
        strncpy(s_state.status_str, "NIGHT", sizeof(s_state.status_str));

        // Quay từ từ về vị trí đón bình minh (Home position)
        float cur_az = servo_get_angle(SERVO_ID_AZIMUTH);
        float cur_el = servo_get_angle(SERVO_ID_ELEVATION);

        if (cur_az < s_config.home_az_angle) cur_az += s_config.step_size;
        else if (cur_az > s_config.home_az_angle) cur_az -= s_config.step_size;

        if (cur_el < s_config.home_el_angle) cur_el += s_config.step_size;
        else if (cur_el > s_config.home_el_angle) cur_el -= s_config.step_size;

        servo_set_angle(SERVO_ID_AZIMUTH, cur_az);
        servo_set_angle(SERVO_ID_ELEVATION, cur_el);

        s_state.current_az = cur_az;
        s_state.current_el = cur_el;

        if (out_state) *out_state = s_state;
        return ESP_OK;
    }

    // 2. Chế độ bám nhật tự động ban ngày (AUTO TRACKING)
    s_state.mode = TRACKER_MODE_AUTO;
    bool is_moving = false;

    // --- Điều khiển trục đứng (Elevation / Vertical) ---
    // diff_v = Top - Bottom:
    // Nếu Top > Bottom (diff_v > deadband) -> Cần ngẩng servo lên (tăng góc)
    // Nếu Bottom > Top (diff_v < -deadband) -> Cần hạ servo xuống (giảm góc)
    if (ldr->diff_v > s_config.deadband) {
        servo_step(SERVO_ID_ELEVATION, s_config.step_size);
        is_moving = true;
    } else if (ldr->diff_v < -s_config.deadband) {
        servo_step(SERVO_ID_ELEVATION, -s_config.step_size);
        is_moving = true;
    }

    // --- Điều khiển trục ngang (Azimuth / Horizontal) ---
    // diff_h = Left - Right:
    // Nếu Left > Right (diff_h > deadband) -> Cần quay servo sang trái (giảm/tăng tùy chiều lắp)
    // Nếu Right > Left (diff_h < -deadband) -> Cần quay servo sang phải
    if (ldr->diff_h > s_config.deadband) {
        servo_step(SERVO_ID_AZIMUTH, -s_config.step_size);
        is_moving = true;
    } else if (ldr->diff_h < -s_config.deadband) {
        servo_step(SERVO_ID_AZIMUTH, s_config.step_size);
        is_moving = true;
    }

    s_state.current_az = servo_get_angle(SERVO_ID_AZIMUTH);
    s_state.current_el = servo_get_angle(SERVO_ID_ELEVATION);

    if (is_moving) {
        strncpy(s_state.status_str, "TRACKING", sizeof(s_state.status_str));
    } else {
        strncpy(s_state.status_str, "BALANCED", sizeof(s_state.status_str));
    }

    if (out_state) *out_state = s_state;
    return ESP_OK;
}

void solar_tracker_get_state(solar_tracker_state_t *out_state) {
    if (out_state) {
        *out_state = s_state;
    }
}
