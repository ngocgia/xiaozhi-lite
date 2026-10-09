# Board bread-compact-esp32 (ESP32 DevKit đấu breadboard)

Board DIY tối giản cho chip ESP32 gốc: mic INMP441 + loa MAX98357A (I2S không
cần codec) + OLED SSD1306/SH1106 I2C. Sơ đồ chân chi tiết xem
[docs/day-day-day-cam.md](../../../docs/day-day-day-cam.md).

## Build

Khuyến nghị dùng script build chuẩn của repo (tự chọn board + kiểm chứng cấu hình):

```sh
python scripts/build.py bread-compact-esp32 --name bread-compact-esp32          # OLED 128x64
python scripts/build.py bread-compact-esp32 --name bread-compact-esp32-128x32   # OLED 128x32
```

Nếu muốn dùng idf.py trực tiếp:

```sh
idf.py set-target esp32
idf.py menuconfig   # Xiaozhi Assistant -> Target Board -> Bread Compact ESP32 DevKit
idf.py build
```

## Lưu ý

- ESP32 **không có PSRAM** sẽ không chạy được wake word — khi đó dùng nút BOOT
  để bắt đầu hội thoại. Board DevKit có PSRAM (ví dụ WROVER) sẽ bật được
  WakeNet đơn giản.
- Wake word mặc định cho ESP32 (khi có PSRAM): WakeNet `wn9_nihaoxiaozhi_tts`
  ("你好小智").
