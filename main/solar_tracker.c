/**
 * @file solar_tracker.c
 * @brief Hiện thực thuật toán bám nhật 2 trục và máy trạng thái (State Machine)
 */

#include "solar_tracker.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "SOLAR_TRACKER";

static solar_tracker_config_t s_config = {
    .deadband = 50,
    .step_size = 1.0f,
    .night_threshold = 80,
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

    // Kiểm tra có chênh lệch ánh sáng giữa các hướng hay không
    bool has_diff = (ldr->diff_v > s_config.deadband || ldr->diff_v < -s_config.deadband ||
                     ldr->diff_h > s_config.deadband || ldr->diff_h < -s_config.deadband);

    // 1. ƯU TIÊN HÀNG ĐẦU CHO DEMO: Miễn là có nguồn sáng lệch hướng -> Bám theo ngay!
    // Bất kể phòng tối hay sáng, chỉ cần rọi đèn tạo chênh lệch là Servo quay bám theo
    if (has_diff) {
        s_state.mode = TRACKER_MODE_AUTO;

        // --- Điều khiển trục đứng (Elevation / Vertical) ---
        // Chiều cơ khí thực tế: -step là Ngẩng lên (Top), +step là Gập xuống (Bottom)
        if (ldr->diff_v > s_config.deadband) {
            servo_step(SERVO_ID_ELEVATION, -s_config.step_size); // Chiếu trên -> Ngẩng lên
        } else if (ldr->diff_v < -s_config.deadband) {
            servo_step(SERVO_ID_ELEVATION, s_config.step_size);  // Chiếu dưới -> Gập xuống
        }

        // --- Điều khiển trục ngang (Azimuth / Horizontal) ---
        // Left > Right -> Quay trái; Right > Left -> Quay phải
        if (ldr->diff_h > s_config.deadband) {
            servo_step(SERVO_ID_AZIMUTH, -s_config.step_size);
        } else if (ldr->diff_h < -s_config.deadband) {
            servo_step(SERVO_ID_AZIMUTH, s_config.step_size);
        }

        s_state.current_az = servo_get_angle(SERVO_ID_AZIMUTH);
        s_state.current_el = servo_get_angle(SERVO_ID_ELEVATION);
        strncpy(s_state.status_str, "TRACKING", sizeof(s_state.status_str));

        if (out_state) *out_state = s_state;
        return ESP_OK;
    }

    // 2. Không có chênh lệch ánh sáng: Kiểm tra điều kiện tối hoàn toàn (Night Mode)
    // Chỉ kích hoạt khi thực sự không có đèn (hoặc lấy tay/hộp đen trùm kín cả 4 quang trở)
    if (ldr->avg_total < s_config.night_threshold) {
        s_state.mode = TRACKER_MODE_NIGHT;
        strncpy(s_state.status_str, "NIGHT", sizeof(s_state.status_str));

        // Quay từ từ về vị trí đón bình minh (Home position: Đông 45°, Ngẩng 60°)
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

    // 3. Đã cân bằng hướng sáng (BALANCED - Servo đứng yên khóa vị trí)
    s_state.mode = TRACKER_MODE_AUTO;
    s_state.current_az = servo_get_angle(SERVO_ID_AZIMUTH);
    s_state.current_el = servo_get_angle(SERVO_ID_ELEVATION);
    strncpy(s_state.status_str, "BALANCED", sizeof(s_state.status_str));

    if (out_state) *out_state = s_state;
    return ESP_OK;
}

void solar_tracker_get_state(solar_tracker_state_t *out_state) {
    if (out_state) {
        *out_state = s_state;
    }
}
