# BÁO CÁO TIẾN ĐỘ & TÀI LIỆU BÀN GIAO DỰ ÁN
## Đồ án: Hệ thống Pin Mặt Trời Bám Nhật 2 Trục (CE224 - UIT)

> **Người thực hiện đợt 1:** Nhóm đồ án CE224  
> **Nền tảng:** ESP32 DevKit V4 | ESP-IDF v5.5.1  
> **Trạng thái biên dịch:** `BUILD SUCCESS (100% PASS)`

---

## 1. Tổng quan các hạng mục đã hoàn thành (Completed Milestones)

Dự án đã được tổ chức cấu trúc hoàn chỉnh dưới dạng các module thư viện độc lập, dễ hiểu, sẵn sàng cho việc clone từ GitHub và phát triển tiếp:

| STT | Hạng mục / Module | File mã nguồn | Trạng thái | Chi tiết kỹ thuật |
| :-: | :--- | :--- | :-: | :--- |
| **01** | **Khởi tạo Project & CMake** | `CMakeLists.txt`, `sdkconfig`, `.gitignore` | **Hoàn thành** | Đã cấu hình target `esp32`, cấu hình `.gitignore` chuẩn tránh commit file build. |
| **02** | **Quang trở LDR (ADC1)** | `ldr_sensor.h`, `ldr_sensor.c` | **Hoàn thành** | Đọc 4 kênh ADC1 (GPIO 34, 35, 32, 33), lọc nhiễu 32 mẫu, hiệu chuẩn mV, tính toán sai lệch hướng và nhận diện ban đêm. |
| **03** | **Động cơ Servo MG90S** | `servo_control.h`, `servo_control.c` | **Hoàn thành** | Điều khiển PWM LEDC 50Hz (GPIO 18 Azimuth, GPIO 19 Elevation), độ phân giải 14-bit, có hàm chống kẹt cơ khí (clamp góc an toàn). |
| **04** | **Cảm biến dòng/áp INA219** | `ina219.h`, `ina219.c` | **Hoàn thành** | Giao tiếp I2C đọc Bus Voltage (V), Shunt Voltage (mV), Dòng nạp/xả (mA), Công suất (mW). Tự động calib với Shunt 0.1Ω. |
| **05** | **Màn hình OLED SSD1306** | `oled_ssd1306.h`, `oled_ssd1306.c`, `font8x8.h` | **Hoàn thành** | Khởi tạo I2C 128x64, bộ đệm khung hình 1024 bytes, font 8x8 ASCII, hàm vẽ giao diện Dashboard thời gian thực cho Solar Tracker. |
| **06** | **Bus I2C Master dùng chung** | `i2c_bus.h`, `i2c_bus.c` | **Hoàn thành** | Khởi tạo I2C Master trên GPIO 21 (SDA), GPIO 22 (SCL), cho phép OLED (`0x3C`) và INA219 (`0x40`) dùng chung đường truyền mà không xung đột. |
| **07** | **Bộ điều khiển bám nhật** | `solar_tracker.h`, `solar_tracker.c` | **Hoàn thành** | Thuật toán so sánh chênh lệch ánh sáng với ngưỡng dung sai (`deadband`), tính bước dịch góc (`step_size`), tự động quay về vị trí chờ đón bình minh khi trời tối. |
| **08** | **Tích hợp Đa nhiệm FreeRTOS** | `CE224_SolarTracker.c` | **Hoàn thành** | Tách 2 task: `solar_tracking_task` (100ms) điều khiển góc và `monitoring_task` (500ms) đọc năng lượng, vẽ OLED và in bảng log Serial. |
| **09** | **Cơ chế chịu lỗi (Fault-Tolerance)** | `CE224_SolarTracker.c` | **Hoàn thành** | Nếu chưa gắn OLED hoặc INA219, chương trình **không bị sập/crash** mà vẫn tiếp tục đọc LDR và điều khiển Servo bình thường. |

---

## 2. Checklist công việc dành cho bạn cùng nhóm tiếp nhận (Next Steps)

Khi bạn cùng nhóm clone repo này về từ GitHub, dưới đây là các bước và hạng mục cần thực hiện:

### Checklist 1: Cài đặt và Kiểm tra môi trường (Máy tính cá nhân)
- [ ] Cài đặt ESP-IDF phiên bản **v5.5.1** (hoặc v5.x tương đương).
- [ ] Chạy lệnh kiểm tra môi trường:
  ```bash
  source ~/.espressif/v5.5.1/esp-idf/export.sh
  idf.py --version
  ```
- [ ] Biên dịch thử nghiệm để đảm bảo không thiếu thư viện:
  ```bash
  cd CE224_SolarTracker
  idf.py build
  ```

### Checklist 2: Lắp ráp cơ khí & Đấu nối dây thực tế
- [ ] **Lắp ráp 4 LDR quanh tấm chắn chữ thập:**
  - TL (Trên - Trái): Nối vào GPIO 34 qua mạch phân áp trở 10k và 3.3V.
  - TR (Trên - Phải): Nối vào GPIO 35 qua mạch phân áp trở 10k và 3.3V.
  - BL (Dưới - Trái): Nối vào GPIO 32 qua mạch phân áp trở 10k và 3.3V.
  - BR (Dưới - Phải): Nối vào GPIO 33 qua mạch phân áp trở 10k và 3.3V.
- [ ] **Lắp 2 Động cơ Servo MG90S vào khung xoay 2 trục:**
  - Dây cam (Signal) của Servo Azimuth nối vào **GPIO 18**.
  - Dây cam (Signal) của Servo Elevation nối vào **GPIO 19**.
  - **Lưu ý:** Nguồn 5V và GND cho 2 servo phải lấy từ **mạch hạ áp LM2596**, nối chung mass GND với ESP32.
- [ ] **Cắm màn hình OLED SSD1306 và cảm biến INA219 vào I2C Bus:**
  - Chân VCC $\rightarrow$ Nối 3.3V (hoặc 5V tùy loại module).
  - Chân GND $\rightarrow$ GND chung.
  - Chân SDA $\rightarrow$ **GPIO 21**.
  - Chân SCL $\rightarrow$ **GPIO 22**.

### Checklist 3: Tinh chỉnh tham số thuật toán ngoài trời
- [ ] **Kiểm tra chiều quay của Servo:**
  - Bật đèn flash rọi vào bên Trái. Nếu servo quay ngược chiều (quay sang Phải), vào file [`solar_tracker.c`](file:///home/hung/UIT/CE224/CE224_SolarTracker/main/solar_tracker.c) đổi dấu tại dòng điều khiển:
    ```c
    // Đổi từ +s_config.step_size thành -s_config.step_size hoặc ngược lại
    servo_step(SERVO_ID_AZIMUTH, s_config.step_size);
    ```
- [ ] **Cân chỉnh ngưỡng Deadband:**
  - Mặc định là `100`. Nếu ra ngoài trời nắng gắt quang trở nhạy làm servo bị giật nhẹ, có thể tăng lên `150` hoặc `200` trong `s_config.deadband`.
- [ ] **Kiểm tra đo năng lượng từ Tấm pin mặt trời:**
  - Nối dương tấm pin qua chân $VIN+$ của INA219, chân $VIN-$ đi vào cực dương sạc của TP4056.
  - Quan sát dòng sạc và công suất hiển thị trên OLED và Terminal.

---

## 3. Hướng dẫn các lệnh Git để đẩy lên GitHub

Để đưa toàn bộ mã nguồn lên GitHub của nhóm:

```bash
cd ~/UIT/CE224/CE224_SolarTracker

# 1. Thêm tất cả file mã nguồn vào Git staging
git add .

# 2. Tạo commit đầu tiên
git commit -m "feat: complete modular libraries for solar tracker (ldr, servo, ina219, oled, fsm)"

# 3. Đổi tên nhánh chính thành main (nếu chưa)
git branch -M main

# 4. Thêm remote repository GitHub của bạn (thay bằng link repo thật của bạn)
# git remote add origin https://github.com/<tai_khoan_cua_ban>/CE224_SolarTracker.git

# 5. Đẩy mã nguồn lên GitHub
# git push -u origin main
```
