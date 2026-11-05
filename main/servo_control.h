/**
 * @file servo_control.h
 * @brief Điều khiển 2 động cơ servo MG90S (Trục ngang Azimuth & Trục đứng Elevation) dùng LEDC PWM
 */

#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H

#include <stdbool.h>
#include "esp_err.h"
#include "hal/gpio_types.h"

// Chân điều khiển PWM xuất xung tới chân Signal (dây cam/vàng) của servo MG90S
#define SERVO_AZIMUTH_GPIO      GPIO_NUM_18 // Trục ngang (Horizontal / Azimuth)
#define SERVO_ELEVATION_GPIO    GPIO_NUM_19 // Trục đứng (Vertical / Elevation)

// Giới hạn góc an toàn (độ) để bảo vệ khung cơ khí và dây dẫn
#define SERVO_AZ_MIN_ANGLE      0.0f
#define SERVO_AZ_MAX_ANGLE      180.0f
#define SERVO_AZ_DEFAULT_ANGLE  90.0f

#define SERVO_EL_MIN_ANGLE      15.0f   // Tránh để góc quá thấp đập vào chân đế
#define SERVO_EL_MAX_ANGLE      165.0f  // Tránh ngửa quá mức kéo căng dây
#define SERVO_EL_DEFAULT_ANGLE  90.0f

typedef enum {
    SERVO_ID_AZIMUTH   = 0, // Trục ngang
    SERVO_ID_ELEVATION = 1  // Trục đứng
} servo_id_t;

/**
 * @brief Khởi tạo LEDC PWM 50Hz cho 2 Servo MG90S
 * @return ESP_OK nếu thành công
 */
esp_err_t servo_control_init(void);

/**
 * @brief Đặt góc cho servo
 * @param id Định danh servo (SERVO_ID_AZIMUTH hoặc SERVO_ID_ELEVATION)
 * @param angle Góc mong muốn (độ, từ 0.0 đến 180.0)
 * @return ESP_OK nếu thành công
 */
esp_err_t servo_set_angle(servo_id_t id, float angle);

/**
 * @brief Lấy góc hiện tại của servo
 * @param id Định danh servo
 * @return Góc hiện tại (độ)
 */
float servo_get_angle(servo_id_t id);

/**
 * @brief Dịch chuyển góc servo thêm một khoảng bước (step)
 * @param id Định danh servo
 * @param step_delta Độ dời góc (dương: tăng góc, âm: giảm góc)
 * @return Góc mới sau khi dịch chuyển
 */
float servo_step(servo_id_t id, float step_delta);

#endif // SERVO_CONTROL_H
