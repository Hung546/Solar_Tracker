# CE224 - Đồ Án Hệ Thống Pin Mặt Trời Bám Nhật 2 Trục (2-Axis Solar Tracker)

Dự án đồ án môn học **Thiết kế Hệ thống Nhúng (CE224)** - Trường Đại học Công nghệ Thông tin (ĐHQG-HCM).  
Chương trình được phát triển trên vi điều khiển **ESP32 DevKit V4 (38 chân)** sử dụng nền tảng **ESP-IDF v5.5.1**.

---

## 1. Danh sách linh kiện phần cứng

| STT | Linh kiện phần cứng | Vai trò trong hệ thống | Giao tiếp / Chân ESP32 |
| :-: | :--- | :--- | :--- |
| **01** | **ESP32 DevKit V4** | Bộ xử lý trung tâm, đọc ADC, điều khiển PWM, I2C | — |
| **02** | **Tấm pin mặt trời 5V 200mA** | Chuyển đổi quang năng thành điện năng, sạc pin | Nối vào INA219 & TP4056 |
| **03** | **4 Quang trở LDR** (kèm trở 10k) | Cảm nhận cường độ ánh sáng 4 góc phần tư | ADC1 (GPIO 34, 35, 32, 33) |
| **04** | **Tấm chắn chữ thập (+)** | Tạo bóng đổ chênh lệch ánh sáng lên 4 LDR | Đặt giữa 4 LDR |
| **05** | **2 Động cơ Servo MG90S** | Xoay cơ cấu tấm pin theo 2 trục Azimuth & Elevation | PWM LEDC (GPIO 18, GPIO 19) |
| **06** | **Cảm biến dòng/áp INA219** | Đo điện áp, dòng điện và công suất pin mặt trời | I2C (SDA: 21, SCL: 22, Addr: `0x40`) |
| **07** | **Mạch sạc TP4056 (có bảo vệ)** | Sạc pin Li-ion từ tấm pin mặt trời, ngắt khi đầy/xả sâu | Nối giữa Solar Panel và Pin 18650 |
| **08** | **Pin Li-ion 18650** | Lưu trữ năng lượng vận hành hệ thống độc lập | Nối vào cổng B+/B- của TP4056 |
| **09** | **Mạch hạ áp Buck LM2596** | Hạ áp từ pin tạo nguồn 5V ổn định cấp cho Servo và ESP32 | Nối vào OUT của TP4056 |
| **10** | **Màn hình OLED SSD1306 (128x64)** | Hiển thị trực tiếp góc quay, điện áp, dòng, công suất | I2C (SDA: 21, SCL: 22, Addr: `0x3C`) |
| **11** | **Khung cơ khí 2 trục** | Khung xoay mica/in 3D lắp ráp 2 servo và tấm pin | — |

---

## 2. Sơ đồ nối dây chi tiết (Pinout Map)

### 2.1. Phân bổ chân trên ESP32 DevKit V4

```text
                                  +-------------------+
                                  |   ESP-WROOM-32    |
                                  |   (ANTENNA NẰM TRÊN)
                                  +-------------------+
                   (Nguồn ADC 3.3V) 3V3 [ 1]       [ 1] GND (Mass chung toàn hệ thống)
                                     EN [ 2]       [ 2] 23
                   LDR_TL (Trên-Trái) 36 [ 3]       [ 3] 22 <--- SCL (Chung OLED & INA219)
                  LDR_TR (Trên-Phải) 39 [ 4]       [ 4]  1
                   LDR_BL (Dưới-Trái) 34 [ 5]       [ 5]  3
                  LDR_BR (Dưới-Phải) 35 [ 6]       [ 6] 21 <--- SDA (Chung OLED & INA219)
                                     32 [ 7]       [ 7] 19 <--- Servo Elevation (Trục đứng)
                                     33 [ 8]       [ 8] 18 <--- Servo Azimuth (Trục ngang)
                                     ...               ...
```

*Cấu hình chân chính xác trong mã nguồn:*
- **LDR Trên - Trái (Top-Left - TL):** `GPIO 34` (ADC1_CH6)
- **LDR Trên - Phải (Top-Right - TR):** `GPIO 35` (ADC1_CH7)
- **LDR Dưới - Trái (Bottom-Left - BL):** `GPIO 32` (ADC1_CH4)
- **LDR Dưới - Phải (Bottom-Right - BR):** `GPIO 33` (ADC1_CH5)
- **Servo Trục Ngang (Azimuth - Đông sang Tây):** `GPIO 18` (PWM 50Hz)
- **Servo Trục Đứng (Elevation - Ngẩng lên/xuống):** `GPIO 19` (PWM 50Hz)
- **I2C Bus (OLED + INA219):** `SDA = GPIO 21`, `SCL = GPIO 22`

> [!CAUTION]
> **Đặc biệt lưu ý an toàn nguồn điện:**
> 1. **Mạch phân áp LDR:** Bắt buộc cấp nguồn **3.3V (chân 3V3)** của ESP32. Tuyệt đối không cấp 5V vào mạch phân áp để tránh vượt ngưỡng áp chân ADC ESP32.
> 2. **Nguồn cho 2 Servo MG90S:** Servo tiêu thụ dòng tức thời cao (lên tới 500mA - 1A khi khởi động). **Bắt buộc dùng nguồn 5V riêng từ mạch LM2596**, không lấy từ chân 5V của ESP32 qua cổng USB máy tính để tránh sụt áp gây reset ESP32 (Brownout reset).
> 3. **Mass chung (Common GND):** GND của ESP32, LM2596, Servo, TP4056 và các cảm biến phải nối chung với nhau.

---

## 3. Kiến trúc mã nguồn & Các module thư viện

Toàn bộ hệ thống được thiết kế theo mô hình kiến trúc module độc lập, sạch sẽ và dễ mở rộng:

```
CE224_SolarTracker/
├── CMakeLists.txt              # CMake tổng của project
├── sdkconfig                   # Cấu hình SDK ESP-IDF
├── README.md                   # Hướng dẫn chi tiết hệ thống
├── PROGRESS.md                 # Báo cáo tiến độ bàn giao cho nhóm
└── main/
    ├── CMakeLists.txt          # Khai báo file nguồn và dependencies
    ├── CE224_SolarTracker.c    # Chương trình chính (FreeRTOS Tasks)
    │
    ├── ldr_sensor.h / .c       # Module đọc 4 quang trở ADC1, lọc nhiễu, calib mV
    ├── servo_control.h / .c    # Module điều khiển 2 Servo MG90S qua LEDC PWM
    ├── ina219.h / .c           # Module giao tiếp cảm biến dòng/áp/công suất I2C
    ├── oled_ssd1306.h / .c     # Module hiển thị màn hình OLED SSD1306 I2C
    ├── font8x8.h               # Bảng font chữ ASCII cho màn hình OLED
    ├── i2c_bus.h / .c          # Quản lý I2C Master Bus dùng chung an toàn
    └── solar_tracker.h / .c    # Bộ điều khiển thuật toán bám nhật & máy trạng thái
```

### Các hàm API quan trọng:
* **`ldr_sensor_read(&ldr_data)`**: Lấy giá trị thô, mV, trung bình các phía, sai lệch $\text{diff\_v}$, $\text{diff\_h}$ và cờ phát hiện ban đêm.
* **`servo_set_angle(id, angle)` / `servo_step(id, delta)`**: Điều chỉnh góc servo có clamp giới hạn cơ khí an toàn.
* **`ina219_read_all(&ina_data)`**: Đọc điện áp (V), dòng điện (mA), công suất (mW).
* **`oled_show_solar_dashboard(...)`**: Cập nhật toàn bộ dashboard lên màn hình OLED.
* **`solar_tracker_process(&ldr_data, &state)`**: Thuật toán bám nhật tự động + cơ chế tự về vị trí đón bình minh ban đêm.

---

## 4. Hướng dẫn Biên dịch, Nạp và Theo dõi

### Bước 1: Kích hoạt môi trường ESP-IDF v5.5.1
```bash
source ~/.espressif/v5.5.1/esp-idf/export.sh
# Hoặc gõ alias nếu đã cấu hình zsh: idf
```

### Bước 2: Biên dịch (Build)
```bash
cd ~/UIT/CE224/CE224_SolarTracker
idf.py build
```

### Bước 3: Nạp chương trình & Mở Monitor
```bash
idf.py flash monitor
# Hoặc chỉ định cổng cụ thể nếu máy nhận nhiều cổng USB:
idf.py -p /dev/ttyUSB0 flash monitor
```
*(Để thoát Terminal monitor, nhấn tổ hợp phím `Ctrl + ]`)*

---

## 5. Cơ chế đa nhiệm (FreeRTOS Architecture)

Chương trình vận hành với 2 FreeRTOS tasks chạy song song:
1. **`solar_tracking_task` (Core 1, Ưu tiên 5, Chu kỳ 100ms):**
   - Đọc mẫu ADC từ 4 quang trở.
   - So sánh chênh lệch ánh sáng với ngưỡng dung sai (`deadband`).
   - Tự động xuất xung xoay 2 Servo theo góc bù sáng tức thời.
2. **`monitoring_task` (Core 0, Ưu tiên 3, Chu kỳ 500ms):**
   - Đọc dữ liệu công suất từ cảm biến INA219.
   - Làm mới giao diện màn hình OLED SSD1306.
   - In bảng Dashboard chi tiết ra UART Serial Terminal.
