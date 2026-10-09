# Hướng dẫn xe AI (board `ai-car`) — xe điều khiển bằng giọng nói

Board này tích hợp ý tưởng của bạn vào firmware XiaoZhi: xe vẫn là một trợ lý
giọng nói đầy đủ (wake word, hội thoại, OTA, màn hình), **đồng thời** AI trên
server lái xe được thông qua các MCP tool. Nói *"tiến lên 2 giây"*, *"quẹo trái"*,
*"cách vật cản bao xa?"* — mô hình ngôn ngữ sẽ gọi tool tương ứng.

## Linh kiện

| Linh kiện | Ghi chú |
|---|---|
| ESP32-S3 DevKitC (N16R8 khuyến nghị) | 16MB flash + PSRAM để chạy AFE + wake word |
| Driver động cơ L298N | 2 kênh cầu H cho 2 động cơ DC |
| 2 động cơ DC + bánh xe | Cùng loại, cùng điện áp |
| Cảm biến siêu âm HC-SR04 | Đặt hướng phía trước xe |
| Màn hình TFT ST7789 240x240 (SPI, IPS) | Bản 1.3"/2.0" phổ biến |
| Micro INMP441 | I2S |
| Loa + khuếch đại MAX98357A | I2S |
| Nguồn 5V riêng cho động cơ | Pin 18650/2S + cầu chì, KHÔNG lấy từ USB DevKit |

## Đấu dây (nguồn chân lý: `main/boards/ai-car/config.h`)

### Âm thanh

| Chức năng | GPIO ESP32-S3 | Chân linh kiện |
|---|---|---|
| INMP441 — SCK | GPIO 6 | SCK |
| INMP441 — WS | GPIO 5 | WS / LRCL |
| INMP441 — SD | GPIO 4 | SD (chân L/R nối GND) |
| MAX98357A — BCLK | GPIO 15 | BCLK |
| MAX98357A — LRC | GPIO 7 | LRC |
| MAX98357A — DIN | GPIO 16 | DIN |

### Màn hình ST7789

| Chức năng | GPIO ESP32-S3 | Chân màn hình |
|---|---|---|
| SPI MOSI | GPIO 41 | SDA / DIN |
| SPI SCLK | GPIO 42 | SCL / CLK |
| Data/Command | GPIO 43 | DC |
| Reset | GPIO 44 | RES |
| Chip Select | — (nối GND) | CS |
| Backlight | — (nối 3.3V) | BL |

> GPIO 43/44 vốn là UART0; board này bật console qua USB (`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG`)
> để nhường 2 chân đó cho màn hình — xem log qua cổng USB Serial/JTAG.

### Động cơ (L298N)

| Chức năng | GPIO ESP32-S3 | Chân L298N |
|---|---|---|
| Động cơ trái thuận/ngược | GPIO 12 / GPIO 13 | IN1 / IN2 |
| Động cơ phải thuận/ngược | GPIO 14 / GPIO 21 | IN3 / IN4 |
| Nguồn động cơ | — | +12V (tối đa) và GND |
| Cấp logic | — | +5V từ L298N ra có thể nuôi DevKit (đủ dòng thì dùng) |

Cả hai chân ENA/ENB của L298N để jumper (mức cao = chạy hết tốc độ).

### Cảm biến HC-SR04

| Chức năng | GPIO ESP32-S3 | Chân HC-SR04 |
|---|---|---|
| Phát xung | GPIO 10 | TRIG |
| Đo phản hồi | GPIO 11 | ECHO — **qua cầu phân áp 1kΩ/2kΩ** (5V → ~3.3V) |

> ⚠️ ECHO của HC-SR04 là mức 5V. Nối thẳng vào GPIO có thể hỏng chân.
> **Chắc chắn nối GND chung** giữa nguồn động cơ, L298N, DevKit và các module.

### Nút và LED

| Chức năng | GPIO |
|---|---|
| Nút BOOT (bấm để nói) | GPIO 0 |
| LED WS2812 trên DevKitC | GPIO 48 (đổi trong `config.h` nếu board khác) |

## Build và nạp

```sh
python scripts/build.py ai-car --name ai-car
idf.py -p COM5 flash monitor
```

Sau khi nạp: nối hotspot để nhập WiFi (xem
[docs/huong-dan-build-va-flash.md](huong-dan-build-va-flash.md)), kích hoạt thiết bị
tại [xiaozhi.me](https://xiaozhi.me), chọn mô hình có hỗ trợ tool (function calling).

## AI lái xe bằng cách nào?

Firmware đăng ký 3 MCP tool (xem `InitializeTools()` trong
`main/boards/ai-car/ai_car_board.cc`):

| Tool | Tham số | Tác dụng |
|---|---|---|
| `self.car.move` | `direction` (forward/backward/left/right/stop), `duration_ms` (0-10000, mặc định 1500) | Cho xe chạy hướng đó, **tự dừng** khi hết `duration_ms` |
| `self.car.stop` | — | Dừng ngay |
| `self.car.get_distance` | — | Trả về khoảng cách vật cản trước mặt (cm, 999 = ngoài tầm) |

Ví dụ hội thoại:

- **Bạn**: "Cho xe tiến lên 2 giây" → AI gọi `self.car.move(forward, 2000)`.
- **Bạn**: "Còn bao xa nữa thì đụng tường?" → AI gọi `self.car.get_distance` rồi đọc số liệu.
- **Bạn**: "Quẹo trái đi" → AI gọi `self.car.move(left, 800)` (quay tại chỗ).

Hành vi an toàn cài sẵn trong firmware:

1. **Chống va chạm** (task nền, core 0): đang tiến mà vật cản < 20 cm
   (`SAFE_DISTANCE_CM`) → tự dừng → lùi 400 ms → dừng hẳn, hiện "⚠ Vật cản phía trước!".
2. **Từ chối lệnh tiến** khi vật cản đã trong ngưỡng an toàn (tool trả lỗi, AI sẽ báo lời).
3. **Tự dừng theo thời gian**: mỗi lệnh `move` có hẹn giờ (esp_timer) — AI quên gửi
   `stop` thì xe cũng dừng.

## So với bản Arduino ban đầu của bạn

Bản sketch Arduino cũ có vài điểm đã được thay/thay thế trong firmware này:

- **Mic không bao giờ được đọc** (chỉ khởi tạo loa) → giờ có toàn bộ pipeline
  thu âm → wake word → Opus → server.
- **WebSocket tự viết tới `ai-box.vn/v1/ws/talk`** → firmware XiaoZhi dùng giao thức
  XiaoZhi chuẩn tới server xiaozhi.me (hoặc server tự dựng — xem README). Không thể
  nói chuyện trực tiếp với `ai-box.vn` vì đó là hai giao thức khác nhau.
- **Hello JSON bị lỗi ký tự** (`"type"😕...`) → handshake chuẩn do firmware lo.
- **`vTaskDelay(1500)` chặn callback WebSocket** → tool MCP trả về ngay, bộ đếm
  esp_timer lo việc tự dừng (không block main loop).
- **Mặt cảm xúc tự vẽ** → màn hình hiển thị giao diện XiaoZhi đầy đủ (emoji,
  phụ đề hội thoại) + thông báo khi có vật cản.

## Tùy biến

- Đổi ngưỡng tránh va chạm: `SAFE_DISTANCE_CM` trong `config.h`.
- Đổi chân bất kỳ: sửa `config.h` (chỉ board của bạn — đừng đụng board chung).
- Đổi hướng quay trái/phải nếu ngược bánh: hoán đổi IN1/IN2 hoặc IN3/IN4 trong
  `MotorController::SetDirection()`.
- Muốn điều tốc (PWM) thay vì on/off: thay `gpio_set_level` bằng LEDC PWM cho
  các chân ENA/ENB của L298N.
- Thêm biến thể build (ví dụ không màn hình): thêm mục vào `builds[]` trong
  `config.json` — xem [docs/tao-board-moi.md](tao-board-moi.md).
