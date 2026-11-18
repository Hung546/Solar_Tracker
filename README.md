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
| **09** | **Mạch tăng áp Boost MT3608** | Kích áp từ pin (3.7V) lên 5.16V ổn định cấp cho Servo và ESP32 | Chỉnh áp trước khi cắm vào J9 |
| **10** | **Công tắc bập bênh KCD1 (2 chân)** | Bật/tắt nguồn toàn hệ thống không cần rút dây | Nối tiếp đường OUT+ từ TP4056 vào MT3608 |
| **11** | **Màn hình OLED SSD1306 (128x64)** | Hiển thị trực tiếp góc quay, điện áp, dòng, công suất | I2C (SDA: 21, SCL: 22, Addr: `0x3C`) |
| **12** | **Khung cơ khí 2 trục** | Khung xoay mica/in 3D lắp ráp 2 servo và tấm pin | — |

---

## 2. Sơ đồ nối dây chi tiết (Hardware Wiring & Pinout Map)

### 2.1. Bảng tra cứu chân cắm Vi điều khiển ESP32 DevKit V4 (38 chân)

```text
                                  +-------------------+
                                  |   ESP-WROOM-32    |
                                  |   (ANTENNA NẰM TRÊN)
                                  +-------------------+
             (Nguồn ADC/I2C 3.3V) 3V3 [ 1]       [20] GND (Mass chung toàn hệ thống)
                                   EN [ 2]       [21] 23
                                   36 [ 3]       [22] 22 <--- SCL (Chung OLED & INA219)
                                   39 [ 4]       [23] TX
             LDR_TL (J6 Trên-Trái) 34 [ 5]       [24] RX
            LDR_TR (J5 Trên-Phải) 35 [ 6]       [25] 21 <--- SDA (Chung OLED & INA219)
             LDR_BL (J4 Dưới-Trái) 32 [ 7]       [26] GND
       LDR_BR (LDR-BR Dưới-Phải) 33 [ 8]       [27] 19 <--- Servo Elevation (J1 Trục đứng)
                                   25 [ 9]       [28] 18 <--- Servo Azimuth (J3 Trục ngang)
                                   26 [10]       [29] 5
                                   27 [11]       [30] 17
                                   14 [12]       [31] 16
                                   12 [13]       [32] 4
                                  GND [14]       [33] 0
                                   13 [15]       [34] 2
                                   D2 [16]       [35] 15
                                   D3 [17]       [36] D1
                                  CMD [18]       [37] D0
    (Nguồn 5.16V từ MT3608 qua J9) 5V [19]       [38] CLK
```

---

### 2.2. Bảng đối chiếu chi tiết từng cổng kết nối (Headers & Modules)

#### CỤM 1: 4 Cảm biến quang trở LDR (Cầu phân áp 10k nối 3.3V)
*Quang trở không phân cực, mỗi con nối 2 dây vào 2 lỗ của connector tương ứng trên mạch:*

| Cổng trên Bo mạch | Vị trí trên Tấm pin | Chân ESP32 | Chức năng trong code |
| :--- | :--- | :---: | :--- |
| **`J6` (`LDR-TL`)** | Góc **Trên - Trái** (Top-Left) | **GPIO 34** | Đọc ADC1_CH6 (12-bit) |
| **`J5` (`LDR-TR`)** | Góc **Trên - Phải** (Top-Right) | **GPIO 35** | Đọc ADC1_CH7 (12-bit) |
| **`J4` (`LDR-BL`)** | Góc **Dưới - Trái** (Bottom-Left) | **GPIO 32** | Đọc ADC1_CH4 (12-bit) |
| **`LDR-BR`** (chân gần U4) | Góc **Dưới - Phải** (Bottom-Right) | **GPIO 33** | Đọc ADC1_CH5 (12-bit) |

*(Lưu ý: Nếu giá trị ADC của một con bất kỳ bị đọc ra bằng 0 tròn trĩnh, hãy kiểm tra lại mối hàn/giắc cắm vì điện trở 10k đang kéo thẳng chân đó về GND).*

#### CỤM 2: 2 Động cơ Servo MG90S
*Header 3 chân trên mạch có thứ tự: Chân 1 (Signal) - Chân 2 (GND) - Chân 3 (5V):*

| Cổng trên Bo mạch | Động cơ điều khiển | Thứ tự chân cắm (Màu dây Servo) | Chân ESP32 |
| :--- | :--- | :--- | :---: |
| **`J3` (`SERVO-NGANG`)** | **Servo Trục Dưới** (Quay ngang Azimuth) | • Chân 1: Dây **Cam / Vàng** (PWM)<br>• Chân 2: Dây **Nâu / Đen** (GND)<br>• Chân 3: Dây **Đỏ** (5V) | **GPIO 18** |
| **`J1` (`SERVO-DOC`)** | **Servo Trục Trên** (Ngẩng / Hạ Elevation) | • Chân 1: Dây **Cam / Vàng** (PWM)<br>• Chân 2: Dây **Nâu / Đen** (GND)<br>• Chân 3: Dây **Đỏ** (5V) | **GPIO 19** |

#### CỤM 3: Chuỗi nguồn Năng lượng mặt trời & Pin 18650
* **`J7` (`SOLAR-IN`):** Nhận nguồn từ Tấm pin mặt trời:
  * Chân 1 (`SL-POS`): Nối dây **Đỏ (+)** của Tấm pin mặt trời.
  * Chân 2 (`GND`): Nối dây **Đen (-)** của Tấm pin mặt trời.
* **`J8` (`TO-TP4056`):** Đưa điện từ tấm pin (sau khi qua cảm biến INA219) sang sạc cho pin:
  * Chân 1 (`SL-TP`): Nối vào chân **`IN+`** của module TP4056.
  * Chân 2 (`GND`): Nối vào chân **`IN-`** của module TP4056.
* **Module TP4056 & Pin 18650:**
  * Chân **`B+`** $\to$ Cực Dương (+) Pin 18650.
  * Chân **`B-`** $\to$ Cực Âm (-) Pin 18650.
* **Công tắc KCD1 & Module Tăng áp MT3608:**
  * Chân **`OUT-`** của TP4056 $\to$ Nối thẳng vào chân **`VIN-`** của MT3608.
  * Chân **`OUT+`** của TP4056 $\to$ Nối vào 1 chân của Công tắc bập bênh KCD1.
  * Chân còn lại của Công tắc KCD1 $\to$ Nối vào chân **`VIN+`** của MT3608.
  * *(Bật công tắc, dùng đồng hồ VOM đo ở VOUT và xoay chiết áp MT3608 để ra đúng **5.16V**).*
* **`J9` (`J-5V-IN`):** Cấp nguồn 5.16V nuôi toàn bộ bo mạch:
  * Chân 1 (`5V`): Nối từ chân **`VOUT+`** của MT3608 vào.
  * Chân 2 (`GND`): Nối từ chân **`VOUT-`** của MT3608 vào.

#### CỤM 4: Bus I2C dùng chung (Màn hình OLED & Module INA219)
* **Màn hình OLED SSD1306 (0.96"):**
  * `VCC` $\to$ Nối nguồn **`3V3`**
  * `GND` $\to$ Nối **`GND`**
  * `SCL` $\to$ **GPIO 22** (Chân 22 của ESP32)
  * `SDA` $\to$ **GPIO 21** (Chân 25 của ESP32)
* **Module INA219 (Đo dòng/áp tấm pin):**
  * `VIN+` $\to$ Nối `SL-POS` (Chân 1 của J7)
  * `VIN-` $\to$ Nối `SL-TP` (Chân 1 của J8)
  * `SDA` $\to$ **GPIO 21**
  * `SCL` $\to$ **GPIO 22**
  * `GND` $\to$ **GND**
  * `VCC` $\to$ **3V3**

---

### 2.3. Giới hạn góc quay & Lưu ý cơ khí chống gãy vỡ

1. **Quy tắc lấy tâm servo ($90^\circ$):** Trước khi vặn ốc cố định khung mica vào cánh tay servo, luôn tháo đầu quay ra trước, cấp điện cho ESP32 để 2 servo tự động quay về vị trí tâm $90.0^\circ$, sau đó mới gắn khung mica ở trạng thái thăng bằng và siết ốc.
2. **Dải góc an toàn đã cấu hình trong code:**
   * **Trục ngang (Azimuth):** $0.0^\circ \to 180.0^\circ$ (Quay Đông - Tây).
   * **Trục đứng (Elevation):** $45.0^\circ \to 135.0^\circ$ (Vị trí nằm ngang ở giữa là $90^\circ$. Khi chiếu trên ngẩng lên $45^\circ$, khi chiếu dưới gập xuống $135^\circ$, hoàn toàn không đụng vào 4 trụ nylon trắng).
3. **Độ nhạy bám sáng:** Ngưỡng chết (Deadband) đặt ở mức $50$ cho phép hệ thống cực kỳ nhạy khi rọi đèn flash điện thoại xoay vòng quanh tấm pin. Khi che kín cả 4 quang trở ($ADC < 80$), hệ thống tự động quay về hướng Đông đón bình minh (Night Mode).

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
