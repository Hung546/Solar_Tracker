/**
 * @file servo_control.c
 * @brief Hiện thực điều khiển xung PWM LEDC cho Servo MG90S
 */

#include "servo_control.h"
#include "esp_log.h"
#include "driver/ledc.h"

static const char *TAG = "SERVO_CONTROL";

#define SERVO_LEDC_SPEED_MODE   LEDC_LOW_SPEED_MODE
#define SERVO_LEDC_TIMER        LEDC_TIMER_0
#define SERVO_LEDC_DUTY_RES     LEDC_TIMER_14_BIT   // Độ phân giải 14-bit (0 ~ 16383)
#define SERVO_LEDC_FREQ_HZ      50                  // 50Hz (chu kỳ 20ms = 20000us)

// Độ rộng xung MG90S chuẩn (us)
#define SERVO_MIN_PULSEWIDTH_US 500     // Tương ứng 0 độ
#define SERVO_MAX_PULSEWIDTH_US 2500    // Tương ứng 180 độ

#define SERVO_AZ_CHANNEL        LEDC_CHANNEL_0
#define SERVO_EL_CHANNEL        LEDC_CHANNEL_1

static float s_azimuth_angle = SERVO_AZ_DEFAULT_ANGLE;
static float s_elevation_angle = SERVO_EL_DEFAULT_ANGLE;

// Chuyển đổi từ góc (0 - 180 độ) sang Duty Cycle 14-bit
static uint32_t angle_to_duty(float angle) {
    if (angle < 0.0f) angle = 0.0f;
    if (angle > 180.0f) angle = 180.0f;

    float pulse_us = SERVO_MIN_PULSEWIDTH_US +
        (angle / 180.0f) * (SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US);

    // Chu kỳ T = 20ms = 20,000us. Duty = (pulse_us / 20000) * (2^14 - 1)
    uint32_t max_duty = (1 << SERVO_LEDC_DUTY_RES) - 1;
    return (uint32_t)((pulse_us * max_duty) / 20000.0f);
}

esp_err_t servo_control_init(void) {
    ledc_timer_config_t timer_conf = {
        .speed_mode       = SERVO_LEDC_SPEED_MODE,
        .duty_resolution  = SERVO_LEDC_DUTY_RES,
        .timer_num        = SERVO_LEDC_TIMER,
        .freq_hz          = SERVO_LEDC_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    esp_err_t ret = ledc_timer_config(&timer_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Loi cau hinh LEDC Timer cho Servo: %s", esp_err_to_name(ret));
        return ret;
    }

    // Kênh Azimuth (GPIO 18)
    ledc_channel_config_t az_ch_conf = {
        .gpio_num   = SERVO_AZIMUTH_GPIO,
        .speed_mode = SERVO_LEDC_SPEED_MODE,
        .channel    = SERVO_AZ_CHANNEL,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = SERVO_LEDC_TIMER,
        .duty       = angle_to_duty(s_azimuth_angle),
        .hpoint     = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&az_ch_conf));

    // Kênh Elevation (GPIO 19)
    ledc_channel_config_t el_ch_conf = {
        .gpio_num   = SERVO_ELEVATION_GPIO,
        .speed_mode = SERVO_LEDC_SPEED_MODE,
        .channel    = SERVO_EL_CHANNEL,
        .intr_type  = LEDC_INTR_DISABLE,
        .timer_sel  = SERVO_LEDC_TIMER,
        .duty       = angle_to_duty(s_elevation_angle),
        .hpoint     = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&el_ch_conf));

    ESP_LOGI(TAG, "Khoi tao 2 Servo MG90S thanh cong (AZ: GPIO %d, EL: GPIO %d)",
             SERVO_AZIMUTH_GPIO, SERVO_ELEVATION_GPIO);

    // Đưa servo về góc mặc định ban đầu
    servo_set_angle(SERVO_ID_AZIMUTH, SERVO_AZ_DEFAULT_ANGLE);
    servo_set_angle(SERVO_ID_ELEVATION, SERVO_EL_DEFAULT_ANGLE);

    return ESP_OK;
}

esp_err_t servo_set_angle(servo_id_t id, float angle) {
    ledc_channel_t ch;
    float clamped_angle = angle;

    if (id == SERVO_ID_AZIMUTH) {
        if (clamped_angle < SERVO_AZ_MIN_ANGLE) clamped_angle = SERVO_AZ_MIN_ANGLE;
        if (clamped_angle > SERVO_AZ_MAX_ANGLE) clamped_angle = SERVO_AZ_MAX_ANGLE;
        s_azimuth_angle = clamped_angle;
        ch = SERVO_AZ_CHANNEL;
    } else if (id == SERVO_ID_ELEVATION) {
        if (clamped_angle < SERVO_EL_MIN_ANGLE) clamped_angle = SERVO_EL_MIN_ANGLE;
        if (clamped_angle > SERVO_EL_MAX_ANGLE) clamped_angle = SERVO_EL_MAX_ANGLE;
        s_elevation_angle = clamped_angle;
        ch = SERVO_EL_CHANNEL;
    } else {
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t duty = angle_to_duty(clamped_angle);
    ledc_set_duty(SERVO_LEDC_SPEED_MODE, ch, duty);
    ledc_update_duty(SERVO_LEDC_SPEED_MODE, ch);

    return ESP_OK;
}

float servo_get_angle(servo_id_t id) {
    if (id == SERVO_ID_AZIMUTH) {
        return s_azimuth_angle;
    } else if (id == SERVO_ID_ELEVATION) {
        return s_elevation_angle;
    }
    return 0.0f;
}

float servo_step(servo_id_t id, float step_delta) {
    float cur = servo_get_angle(id);
    float target = cur + step_delta;
    servo_set_angle(id, target);
    return servo_get_angle(id);
}
