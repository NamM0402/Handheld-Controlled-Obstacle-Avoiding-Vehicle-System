# Hệ thống Xe Điều Khiển Tay Cầm và Tránh Vật Cản

Chào mừng bạn đến với dự án **Hệ thống Xe Điều Khiển Tay Cầm và Tránh Vật Cản**, được phát triển bởi Nam (NamM0402). Dự án này là một hệ thống nhúng IoT tích hợp điều khiển thủ công qua tay cầm, tự động tránh vật cản, và giám sát quãng đường/tốc độ. Hệ thống sử dụng ESP32 làm bộ vi điều khiển chính, STM32 để điều khiển động cơ, và kết nối không dây qua Wi-Fi.

## Thành phần phần cứng
- **ESP32 (code_xe.ino)**:
  - Cảm biến siêu âm (TRIG 26, ECHO 27) để phát hiện vật cản.
  - Servo động cơ qua GPIO 25 để quét vật cản.
  - LED chỉ báo trạng thái qua GPIO 2.
  - Encoder động cơ trái (GPIO 34, 35) và phải (GPIO 22, 23) để đo tốc độ và quãng đường.
  - Giao tiếp UART2 (RX 16, TX 17) để truyền lệnh đến STM32.
- **STM32 (main.c, stm32.ino)**:
  - Bộ vi điều khiển STM32 với GPIO, Timer (TIM1), và UART (USART6) để điều khiển động cơ.
  - Động cơ trái và phải với điều khiển hướng qua GPIO.
  - LED chỉ báo trạng thái.
- **Tay cầm (taycam.ino)**:
  - Cảm biến ADC (ADS1115) qua I2C (địa chỉ 0x48) để đọc tín hiệu tay cầm.
  - Màn hình LCD I2C (0x27, 16x2) để hiển thị trạng thái.
  - Nút nhấn chế độ (GPIO 19) để chuyển đổi giữa thủ công và tự động.
  - LED chỉ báo qua GPIO 32.

## Chức năng chính
- **ESP32 (Xe)**:
  - **Điều khiển tự động**: Sử dụng cảm biến siêu âm và servo để quét vật cản, di chuyển nếu khoảng cách > 20cm; rẽ trái/phải nếu có vật cản, lùi xe nếu cả hai hướng bị chặn.
  - **Điều khiển thủ công**: Nhận lệnh từ tay cầm qua Wi-Fi, điều chỉnh tốc độ và hướng dựa trên tín hiệu ADC.
  - **Giám sát**: Đo tốc độ trung bình (km/h) và tổng quãng đường (m) dựa trên encoder, gửi dữ liệu qua Wi-Fi.
  - **Kết nối mạng**: Tạo điểm truy cập Wi-Fi (SSID: "ESP32_SERVER", mật khẩu: "12345678") để kết nối với tay cầm.
- **STM32**:
  - Nhận lệnh từ ESP32 qua UART, điều chỉnh hướng (tiến/lùi) và tốc độ động cơ dựa trên duty cycle PWM từ Timer 1.
  - Nhấp nháy LED mỗi 300ms để báo hiệu hệ thống hoạt động.
- **Tay cầm**:
  - Gửi lệnh điều khiển tốc độ và hướng qua Wi-Fi dựa trên tín hiệu ADC từ tay cầm.
  - Chuyển đổi chế độ (thủ công/tự động) bằng nút nhấn, hiển thị trạng thái trên LCD (tốc độ, quãng đường, hướng: Tiên, Lùi, Rẽ Trái, Rẽ Phải, Dừng Yên).
  - Kết nối với server ESP32 qua IP 192.168.4.1, cổng 1234.

## Công nghệ sử dụng
- **Phần cứng**: ESP32, STM32, cảm biến siêu âm, servo, encoder, ADS1115, LCD I2C.
- **Thư viện**: WiFi, Ticker, ESP32Servo, ADS1115_WE, LiquidCrystal_I2C, STM32Cube HAL, thư viện tùy chỉnh (motor.h, uartHandle.h).
- **Giao thức**: Wi-Fi, UART, I2C.
- **Quản lý thời gian**: Ticker (ESP32), HAL_GetTick() (STM32).

## Lưu ý cài đặt
- **Cấu hình Wi-Fi**: Tay cầm kết nối với SSID "ESP32_SERVER" và mật khẩu "12345678".
- **Điều chỉnh ngưỡng**: Thay đổi ngưỡng khoảng cách (20cm) trong `autoDriveLogic()` của `code_xe.ino` nếu cần.
- **Cấu hình STM32**: Đảm bảo thư viện tùy chỉnh (`myLib/motor.h`, `myLib/uartHandle.h`) được liên kết, kiểm tra tốc độ baud UART6 (thường 115200) để đồng bộ với ESP32.

## Hướng dẫn cài đặt
1. **Yêu cầu phần cứng**:
   - ESP32 và STM32 với các cảm biến/moduie theo danh sách.
   - Kết nối Wi-Fi giữa tay cầm và xe.
2. **Cài đặt phần mềm**:
   - Cài đặt Arduino IDE hoặc PlatformIO cho ESP32.
   - Cài đặt STM32CubeIDE hoặc GCC ARM Embedded Toolchain cho STM32.
   - Thêm các thư viện cần thiết qua Library Manager trong Arduino IDE.
3. **Chạy dự án**:
   - Mở `code_xe.ino` và `taycam.ino` trong Arduino IDE, nạp mã vào ESP32.
   - Mở `main.c` trong STM32CubeIDE, cấu hình và nạp mã vào STM32.
   - Kết nối tay cầm với xe qua Wi-Fi, kiểm tra điều khiển và tránh vật cản.

## Tác giả
-  Nam (NamM0402)
- GitHub: [https://github.com/NamM0402](https://github.com/NamM0402)
- Link video : https://www.facebook.com/stories/1672159190135797/?source=profile_highlight s
