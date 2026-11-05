/**
 * @file CE224_SolarTracker.c
 * @brief Chương trình chính Điều khiển Hệ thống Pin Mặt Trời 2 Trục (2-Axis Solar Tracker)
 * @details Môn học: Thiết kế Hệ thống Nhúng (CE224) - Trường ĐH Công nghệ Thông tin (UIT)
 *          Phần cứng: ESP32 DevKit V4 (38 chân)
 *          Framework: ESP-IDF v5.5.1
 *
 * Sơ đồ khối chức năng:
 * - 4 Cảm biến quang trở LDR (ADC1): GPIO 34 (TL), GPIO 35 (TR), GPIO 32 (BL), GPIO 33 (BR)
 * - 2 Động cơ Servo MG90S (PWM LEDC 50Hz): GPIO 18 (Azimuth - Trục ngang), GPIO 19 (Elevation - Trục đứng)
 * - Cảm biến dòng/áp INA219 (I2C): GPIO 21 (SDA), GPIO 22 (SCL), Địa chỉ 0x40
 * - Màn hình OLED SSD1306 128x64 (I2C): GPIO 21 (SDA), GPIO 22 (SCL), Địa chỉ 0x3C
 * - Tấm pin mặt trời 5V 200mA qua mạch sạc TP4056 nạp pin Li-ion 18650
 * - Mạch nguồn hạ áp Buck LM2596 tạo nguồn 5V nuôi Servo và ESP32
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

// Thư viện các module chức năng
#include "i2c_bus.h"
#include "ldr_sensor.h"
#include "servo_control.h"
#include "ina219.h"
#include "oled_ssd1306.h"
#include "solar_tracker.h"

static const char *TAG = "MAIN_APP";

// Biến trạng thái toàn cục dùng chung giữa các task
static ldr_readings_t       s_ldr_data;
static ina219_data_t        s_ina_data;
static solar_tracker_state_t s_tracker_state;
static bool                 s_ina219_ready = false;
static bool                 s_oled_ready   = false;

/**
 * @brief Task 1: Xử lý bám nhật tự động (Tracking Task)
 * Chu kỳ thực thi: 100ms
 * Nhiệm vụ: Đọc LDR, tính toán sai lệch và xuất xung điều khiển 2 Servo
 */
static void solar_tracking_task(void *pvParameters) {
    ESP_LOGI(TAG, "Task Tracking bat dau hoat dong...");

    while (1) {
        // 1. Đọc dữ liệu từ 4 quang trở
        if (ldr_sensor_read(&s_ldr_data) == ESP_OK) {
            // 2. Thuật toán bám nhật xử lý và xuất tín hiệu điều khiển Servo
            solar_tracker_process(&s_ldr_data, &s_tracker_state);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @brief Task 2: Giám sát năng lượng và hiển thị (Monitoring & UI Task)
 * Chu kỳ thực thi: 500ms
 * Nhiệm vụ: Đọc INA219, cập nhật OLED và in bảng log định kỳ ra Serial
 */
static void monitoring_task(void *pvParameters) {
    ESP_LOGI(TAG, "Task Monitoring bat dau hoat dong...");

    while (1) {
        // 1. Đọc dữ liệu năng lượng từ INA219 (nếu có module gắn vào mạch)
        if (s_ina219_ready) {
            if (ina219_read_all(&s_ina_data) != ESP_OK) {
                // Xử lý khi mất kết nối tạm thời
                s_ina_data.bus_voltage_v = 0.0f;
                s_ina_data.current_ma = 0.0f;
                s_ina_data.power_mw = 0.0f;
            }
        }

        // 2. Cập nhật giao diện màn hình OLED SSD1306 (nếu có gắn màn hình)
        if (s_oled_ready) {
            oled_show_solar_dashboard(
                s_tracker_state.current_az,
                s_tracker_state.current_el,
                s_ina_data.bus_voltage_v,
                s_ina_data.current_ma,
                s_ina_data.power_mw,
                s_tracker_state.status_str
            );
        }

        // 3. In log giám sát định kỳ ra UART Terminal
        printf("\n==================== [ CE224 SOLAR TRACKER DASHBOARD ] ====================\n");
        printf(" [QUANG TRO ADC]   TL: %4d (%4dmV) | TR: %4d (%4dmV)\n",
               s_ldr_data.tl_raw, s_ldr_data.tl_mv, s_ldr_data.tr_raw, s_ldr_data.tr_mv);
        printf("                   BL: %4d (%4dmV) | BR: %4d (%4dmV)\n",
               s_ldr_data.bl_raw, s_ldr_data.bl_mv, s_ldr_data.br_raw, s_ldr_data.br_mv);
        printf(" [CHENH LECH]      Trục dọc (El): %5d | Trục ngang (Az): %5d | Sáng TB: %4d\n",
               s_ldr_data.diff_v, s_ldr_data.diff_h, s_ldr_data.avg_total);
        printf(" [SERVO GOC QUAY]  Azimuth (Ngang): %5.1f° | Elevation (Đứng): %5.1f°\n",
               s_tracker_state.current_az, s_tracker_state.current_el);

        if (s_ina219_ready) {
            printf(" [INA219 PIN MT]   V_Bus: %5.2f V | Dòng: %5.1f mA | Công suất: %6.1f mW\n",
                   s_ina_data.bus_voltage_v, s_ina_data.current_ma, s_ina_data.power_mw);
        } else {
            printf(" [INA219 PIN MT]   (Chưa phát hiện module INA219 trên I2C Bus 0x40)\n");
        }

        printf(" [TRANG THAI HE]   Chế độ: %s | OLED: %s\n",
               s_tracker_state.status_str, s_oled_ready ? "HOAT DONG" : "CHUA KET NOI");
        printf("===========================================================================\n");

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "==========================================================");
    ESP_LOGI(TAG, "  DO AN SOLAR TRACKER 2 TRUC - CE224 UIT                  ");
    ESP_LOGI(TAG, "  Khoi dong vi dieu khien ESP32 DevKit V4 (IDF v5.5.1)   ");
    ESP_LOGI(TAG, "==========================================================");

    // 1. Khởi tạo ngoại vi Quang trở LDR (Bắt buộc)
    ESP_ERROR_CHECK(ldr_sensor_init());

    // 2. Khởi tạo điều khiển 2 Động cơ Servo MG90S
    ESP_ERROR_CHECK(servo_control_init());

    // 3. Khởi tạo bộ điều khiển Solar Tracking State Machine
    ESP_ERROR_CHECK(solar_tracker_init(NULL));

    // 4. Khởi tạo Bus I2C Master (Dùng chung cho INA219 và OLED)
    i2c_master_bus_handle_t i2c_bus = NULL;
    esp_err_t i2c_ret = i2c_bus_init(&i2c_bus);

    if (i2c_ret == ESP_OK && i2c_bus != NULL) {
        // Khởi tạo cảm biến INA219
        if (ina219_init(i2c_bus, INA219_I2C_ADDR_DEFAULT) == ESP_OK) {
            s_ina219_ready = true;
            ESP_LOGI(TAG, "INA219 san sang hoat dong!");
        } else {
            ESP_LOGW(TAG, "Module INA219 chua duoc cam hoac sai dia chi I2C 0x40");
        }

        // Khởi tạo màn hình OLED SSD1306
        if (oled_ssd1306_init(i2c_bus, OLED_I2C_ADDR_DEFAULT) == ESP_OK) {
            s_oled_ready = true;
            ESP_LOGI(TAG, "OLED SSD1306 san sang hoat dong!");
        } else {
            ESP_LOGW(TAG, "Man hinh OLED chua duoc cam hoac sai dia chi I2C 0x3C");
        }
    } else {
        ESP_LOGE(TAG, "Khoi tao I2C Bus that bai!");
    }

    ESP_LOGI(TAG, "Khoi tao he thong hoan tat! Tao cac FreeRTOS tasks...");

    // Tạo Task bám nhật (Ưu tiên cao hơn để phản ứng chuyển động kịp thời)
    xTaskCreatePinnedToCore(
        solar_tracking_task,
        "solar_track_task",
        4096,
        NULL,
        5,
        NULL,
        1
    );

    // Tạo Task giám sát & hiển thị UI
    xTaskCreatePinnedToCore(
        monitoring_task,
        "monitoring_task",
        4096,
        NULL,
        3,
        NULL,
        0
    );
}
