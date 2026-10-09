# XiaoZhi Lite — Trợ lý giọng nói AI cho ESP32 (bản tinh gọn, tài liệu tiếng Việt)

> Fork tinh gọn từ dự án mã nguồn mở [78/xiaozhi-esp32](https://github.com/78/xiaozhi-esp32),
> chỉ giữ lại **3 dòng chip chính: ESP32, ESP32-S3, ESP32-C3**, kèm toàn bộ hướng dẫn
> và ghi chú bằng tiếng Việt. Lõi firmware giữ nguyên để tiếp tục tương thích server
> Xiaozhi chính thức (xiaozhi.me) và các server tự dựng theo đúng giao thức.

XiaoZhi là firmware trợ lý giọng nói: thiết bị thu âm, nhận diện từ khóa đánh thức
(wake word) ngay trên chip, mã hóa Opus, gửi lên server AI (Qwen / DeepSeek...),
nhận lại giọng đọc TTS phát ra loa, và điều khiển thiết bị qua giao thức MCP.

<img src="docs/mcp-based-graph.jpg" alt="Điều khiển mọi thứ qua MCP" width="320">

## Tính năng

- Wake word ngoại tuyến bằng ESP-SR (WakeNet / MultiNet), có thể tùy chỉnh
- Hai kênh truyền: WebSocket hoặc MQTT + UDP (âm thanh mã hóa AES-CTR)
- Âm thanh Opus 16 kHz/60 ms lên — 24 kHz xuống; AEC phía thiết bị trên ESP32-S3-BOX-3
- MCP phía thiết bị: AI có thể gọi tool điều khiển âm lượng, đèn, GPIO...
- Cấu hình WiFi qua hotspot hoặc BluFi (Bluetooth LE)
- Màn hình OLED / LCD với emoji, biểu cảm; đa ngôn ngữ (mặc định tiếng Việt)
- OTA firmware + gói assets (font, emoji, model wake word) từ server
- 39 ngôn ngữ giao diện, giọng nhắc tiếng Việt khi có

## Board được hỗ trợ

| Chip | Board | Biến thể build | Phù hợp với |
|---|---|---|---|
| ESP32 | `bread-compact-esp32` | `bread-compact-esp32`, `bread-compact-esp32-128x32` | Tự đấu breadboard, chi phí thấp nhất |
| ESP32-S3 | `bread-compact-wifi` | `bread-compact-wifi`, `bread-compact-wifi-128x64` | Tự đấu breadboard, có PSRAM để chạy AFE + wake word đầy đủ |
| ESP32-S3 | `espressif/esp32-s3-box-3` | `esp-box-3` | Board thương mại: loa + 2 mic + LCD + AEC |
| ESP32-C3 | `xmini/c3` | `xmini-c3` | Board nhỏ gọn bán sẵn (XiaGe Mini C3) |

Muốn thêm board khác? Xem [docs/tao-board-moi.md](docs/tao-board-moi.md) —
bạn cũng có thể copy board tương ứng từ repo upstream vì cấu trúc hai bên giống hệt nhau.

## Bắt đầu nhanh

```sh
# 1) Cài ESP-IDF v6.1 (tối thiểu 6.0.1) — xem docs/huong-dan-cai-dat.md

# 2) Liệt kê board/biến thể có thể build
python scripts/build.py --list-boards

# 3) Build một biến thể (ví dụ board DIY ESP32-S3)
python scripts/build.py bread-compact-wifi --name bread-compact-wifi

# 4) Nạp firmware và xem log
idf.py -p COM5 flash monitor
```

Lần đầu khởi động, thiết bị phát hotspot để bạn nhập WiFi — chi tiết ở
[docs/huong-dan-build-va-flash.md](docs/huong-dan-build-va-flash.md).
Đấu dây breadboard xem [docs/day-day-day-cam.md](docs/day-day-day-cam.md).

## Tài liệu

| Tài liệu | Nội dung |
|---|---|
| [docs/huong-dan-cai-dat.md](docs/huong-dan-cai-dat.md) | Cài ESP-IDF 6.x trên Windows/Linux |
| [docs/huong-dan-build-va-flash.md](docs/huong-dan-build-va-flash.md) | Build, nạp, cấu hình WiFi, đổi ngôn ngữ/wake word |
| [docs/day-day-day-cam.md](docs/day-day-day-cam.md) | Sơ đồ chân và đấu dây 3 board |
| [docs/kien-truc.md](docs/kien-truc.md) | Kiến trúc firmware: task, luồng âm thanh, máy trạng thái |
| [docs/tao-board-moi.md](docs/tao-board-moi.md) | Thêm board mới của riêng bạn |
| [docs/giai-quyet-su-co.md](docs/giai-quyet-su-co.md) | Sự cố thường gặp và cách xử lý |
| [docs/websocket.md](docs/websocket.md) *(EN)* | Đặc tả giao thức WebSocket với server |
| [docs/mqtt-udp.md](docs/mqtt-udp.md) *(EN)* | Đặc tả giao thức MQTT + UDP |
| [docs/mcp-protocol.md](docs/mcp-protocol.md) *(EN)* | Giao thức MCP phía thiết bị |
| [docs/code_style.md](docs/code_style.md) | Quy tắc code (Google C++ style) |

## Cấu trúc thư mục

```
main/
├── application.cc/.h      # Vòng lặp sự kiện chính, điều phối toàn bộ
├── device_state_machine.* # Máy trạng thái thiết bị
├── ota.cc/.h              # Kiểm tra và nâng cấp firmware qua mạng
├── mcp_server.cc/.h       # MCP server: tool cho AI gọi
├── audio/                 # Codec, engine AFE/lite, wake word, Opus, demuxer
├── protocols/             # WebSocket + MQTT/UDP
├── display/               # OLED, LCD LVGL, emote, font
├── led/                   # LED đơn / WS2812
├── boards/common/         # Lớp nền Board, WiFi/4G/RNDIS, nút, pin, PMIC
└── boards/<board>/        # 4 board: config.h (chân) + config.json (biến thể)
scripts/build.py           # Lệnh build chuẩn cho mọi board
partitions/v2/             # Bảng partition (4m / 16m / 16m_c3)
```

Chi tiết thiết kế hệ thống âm thanh: `main/audio/README.md`.

## Server

Firmware mặc định nối server chính thức [xiaozhi.me](https://xiaozhi.me)
(dùng miễn phí mô hình Qwen realtime). Muốn tự dựng server, xem các dự án:
[xinnan-tech/xiaozhi-esp32-server](https://github.com/xinnan-tech/xiaozhi-esp32-server) (Python),
[joey-zhou/xiaozhi-esp32-server-java](https://github.com/joey-zhou/xiaozhi-esp32-server-java) (Java).

## Đóng góp và bản quyền

- Code theo Google C++ style, format bằng `.clang-format` của repo.
- Chạy `python -m unittest discover -s scripts/tests` trước khi gửi PR.
- Giấy phép MIT (kế thừa từ upstream). Cảm ơn [78/xiaozhi-esp32](https://github.com/78/xiaozhi-esp32)
  và cộng đồng Espressif.
