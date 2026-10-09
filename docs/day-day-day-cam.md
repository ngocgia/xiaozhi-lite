# Sơ đồ chân và đấu dây các board

Boards trong repo chia làm hai loại: **DIY đấu breadboard** (tự mua linh kiện)
và **board nguyên bản bán sẵn**. Dưới đây là sơ đồ chân tham chiếu lấy trực tiếp
từ `config.h` của từng board — đây là "nguồn chân lý" khi bạn muốn đổi chân
(không khuyến nghị đổi vì ảnh hưởng tương thích OTA).

## 1. bread-compact-wifi (ESP32-S3) — DIY breadboard

Board tối giản nhất cho ESP32-S3: mic + loa + OLED I2C + nút bấm, không cần
chip codec âm thanh. Linh kiện đề xuất:

- ESP32-S3 DevKitC (N16R8: 16MB flash, 8MB PSRAM — khuyến nghị)
- Microphone INMP441 (I2S)
- Loa khuếch đại MAX98357A (I2S)
- OLED SSD1306 0.96" I2C (128x32 hoặc 128x64)

| Chức năng | GPIO ESP32-S3 | Chân linh kiện |
|---|---|---|
| Mic INMP441 — WS (LRCLK) | GPIO 4 | WS |
| Mic INMP441 — SCK (BCLK) | GPIO 5 | SCK |
| Mic INMP441 — SD (dữ liệu) | GPIO 6 | SD |
| MAX98357A — DIN | GPIO 7 | DIN |
| MAX98357A — BCLK | GPIO 15 | BCLK |
| MAX98357A — LRC | GPIO 16 | LRC |
| OLED — SDA | GPIO 41 | SDA |
| OLED — SCL | GPIO 42 | SCL |
| Nút BOOT (bấm để nói) | GPIO 0 | — |
| Nút cảm ứng (touch) | GPIO 47 | — |
| Nút volume + | GPIO 40 | — |
| Nút volume − | GPIO 39 | — |
| LED WS2812 onboard | GPIO 48 | — |
| Đèn MCP demo (tùy chọn) | GPIO 18 | relay/LED ngoài |

Nguồn cấp: 5V qua USB của DevKit. INMP441 và MAX98357A lấy 3.3V từ chân
`3V3` của DevKit. GND chung.

## 2. bread-compact-esp32 (ESP32 gốc) — DIY breadboard

Tương tự board trên nhưng cho ESP32 thường (DevKit V1 30/38 chân, 4MB flash).
Cần bản **có PSRAM** nếu muốn bật wake word (ESP32 không PSRAM chỉ dùng nút bấm):

| Chức năng | GPIO ESP32 | Chân linh kiện |
|---|---|---|
| Mic INMP441 — WS | GPIO 25 | WS |
| Mic INMP441 — SCK | GPIO 26 | SCK |
| Mic INMP441 — SD | GPIO 32 | SD |
| MAX98357A — DIN | GPIO 33 | DIN |
| MAX98357A — BCLK | GPIO 14 | BCLK |
| MAX98357A — LRC | GPIO 27 | LRC |
| OLED — SDA | GPIO 4 | SDA |
| OLED — SCL | GPIO 15 | SCL |
| Nút BOOT (bấm để nói) | GPIO 0 | — |
| Nút touch | GPIO 5 | — |
| Nút ASR | GPIO 19 | — |
| LED onboard | GPIO 2 | — |
| Modem ML307 RX (tùy chọn 4G) | GPIO 16 | TX modem |
| Modem ML307 TX (tùy chọn 4G) | GPIO 17 | RX modem |

## 3. xmini/c3 (XiaGe Mini C3) — board bán sẵn

Board nhỏ gọn của "虾哥" (XiaGe), đã hàn sẵn codec ES8311 + loa + mic + nút,
không cần đấu dây gì thêm. Sơ đồ chân chính (từ `config.h`, phục vụ tham khảo/sửa):

| Chức năng | GPIO |
|---|---|
| I2S MCLK | GPIO 10 |
| I2S WS / BCLK / DIN / DOUT | GPIO 6 / 8 / 7 / 5 |
| Khuếch đại PA bật/tắt | GPIO 11 |
| Codec I2C SDA / SCL | GPIO 3 / 4 |
| LED | GPIO 2 |
| Nút BOOT | GPIO 9 |

Chỉ cần cắm USB (Cổng USB-C, console ra qua USB Serial JTAG) và nạp firmware.

## 4. espressif/esp32-s3-box-3 — board thương mại

Board chính hãng Espressif với 2 micro mảng, loa, màn hình LCD 320x240 cảm ứng,
có AEC phần cứng (wake được cả khi đang phát âm thanh). Không cần đấu dây.
Điểm đáng chú ý trong cấu hình:

- Codec: ES8311 (loa) + ES7210 (mic, TDM)
- Màn hình LCD ILI9341, backlight GPIO 47
- AEC phía thiết bị bật sẵn: `CONFIG_USE_DEVICE_AEC=y`

## Mẹo chung khi tự đấu breadboard

- Dây I2S nên ngắn (< 10cm) để tránh nhiễu làm giọng nói rèm.
- Chân `L/R` của INMP441 nối GND (chọn kênh trái).
- Nếu nghe loá loa rè: kiểm tra GND chung và nguồn 5V đủ dòng.
- Đổi biến thể OLED 128x32/128x64 lúc build bằng `--name` (xem
  [docs/huong-dan-build-va-flash.md](huong-dan-build-va-flash.md)).
