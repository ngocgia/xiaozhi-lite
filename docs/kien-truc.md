# Kiến trúc firmware XiaoZhi Lite

Tài liệu này tóm tắt cách firmware được tổ chức để bạn đọc code nhanh chóng.
Mọi chi tiết cập nhật nhất vẫn nằm trong code và `main/audio/README.md`.

## Tổng quan một vòng đời hội thoại

```
 [Mic] -> AFE/WakeNet (S3) hoặc WakeNet (C3/ESP32)
       -> Opus encode (16 kHz, 60 ms/frame)
       -> WebSocket hoặc MQTT+UDP ----------------------> Server AI (ASR+LLM+TTS)
 [Loa] <- Opus decode + resample (24 kHz -> tốc độ codec)
       <- ------------------------------------------------
```

Ngoài hội thoại, server có thể: đẩy thông báo giọng nói (`notify`), gọi tool
MCP trên thiết bị (âm lượng, đèn, chụp màn hình...), và yêu cầu OTA.

## Vòng lặp chính và task

Firmware chạy trên FreeRTOS với các task chính:

| Task | Ưu tiên | Vai trò |
|---|---|---|
| main (event loop) | 10 | Xử lý sự kiện, UI, state machine, MCP — `Application::Run()` |
| audio_input | 8 | Đọc mic, nạp vào engine âm thanh (ghim core 0 trên S3) |
| audio_output | 4 | Phát PCM ra loa |
| opus_codec | 2 | Encode/decode Opus dùng chung một task |
| audio_afe (S3) | 3 | AFE fetch: AEC, VAD, WakeNet |
| activation | 2 | Kiểm tra OTA + khởi tạo protocol sau khi có mạng |

Các task không nói chuyện trực tiếp với nhau: audio/protocol callback chỉ
**set event bit** (`MAIN_EVENT_SEND_AUDIO`, `MAIN_EVENT_WAKE_WORD_DETECTED`...),
main loop xử lý lần lượt. Muốn chạy gì đó trên main task từ callback khác,
dùng `Application::Schedule(callback)`.

## Máy trạng thái thiết bị

`main/device_state_machine.{h,cc}` định nghĩa các trạng thái hợp lệ:

```
Unknown -> Starting -> (WifiConfiguring | Activating)
Activating -> Idle | Upgrading
Idle -> Connecting -> Listening <-> Speaking
Idle -> Notifying (thông báo giọng nói)
Bất kỳ -> Upgrading -> reboot
```

Chuyển trạng thái chỉ qua `Application::SetDeviceState()`; chuyển không hợp lệ
bị từ chối và ghi log. UI (màn hình, LED) cập nhật theo trạng thái trong
`HandleStateChangedEvent()`.

## Luồng âm thanh chi tiết

- **Uplink**: mic → chunk 10 ms → engine (S3: `AfeAudioEngine` với AEC+VAD+WakeNet;
  ESP32/C3: `LiteAudioEngine` chỉ WakeNet) → queue encode (đầy thì bỏ frame cũ)
  → Opus (complexity 0, DTX) → main loop gửi qua protocol.
- **Downlink**: packet Opus → decode queue (đệm ~1.2s) → decode + resample theo
  sample rate server yêu cầu → playback queue → loa. Timestamp các frame đã phát
  được gửi ngược lên để hỗ trợ AEC phía server.
- **Wake word**: 2 giây âm thanh *trước khi* phát hiện wake word được giữ trong
  ring buffer PSRAM và upload lên server như tin nhắn đầu tiên (tùy chọn).
- Chi tiết: `main/audio/README.md`.

## Lớp giao thức

`Protocol` (base) → `WebsocketProtocol` | `MqttProtocol`:

- WebSocket: text frame = JSON điều khiển, binary frame = âm thanh Opus.
- MQTT (TLS) chở JSON điều khiển; âm thanh chạy **UDP riêng, mã hóa AES-CTR**
  với key server cấp trong handshake.
- Hello handshake: thiết bị báo audio params (opus/16kHz/60ms), tính năng
  (mcp, aec); server trả `session_id` và sample rate phát (mặc định 24 kHz).
- MCP chạy gói trong JSON `{"type":"mcp","payload":{JSON-RPC 2.0}}`.

Xem đặc tả đầy đủ: `docs/websocket.md`, `docs/mqtt-udp.md`, `docs/mcp-protocol.md`.

## MCP phía thiết bị

`McpServer` đăng ký tool cho AI gọi:

- Tool chung: `self.get_device_status`, `self.audio_speaker.set_volume`,
  `self.screen.set_brightness`, `self.screen.set_theme` (tùy hardware).
- Tool chỉ người dùng: `self.reboot`, `self.upgrade_firmware`, `self.screen.snapshot`...
- Board có thể thêm tool riêng trong `InitializeTools()` của lớp board.

Mọi tool call được schedule về main loop để chạy an toàn.

## OTA và assets

- `ota.cc` kiểm tra version → download firmware → ghi partition OTA kế tiếp →
  rollback tự động nếu boot lỗi (`esp_ota_mark_app_valid`).
- Gói **assets** (font, emoji, model wake word) nằm ở partition `assets` riêng,
  có thể cập nhật độc lập qua HTTP (`assets/download_url` đặt bằng MCP tool).
- Chống clone phần cứng: serial eFuse + challenge-response HMAC.

## Lớp board

`Board` là interface duy nhất mà code lõi biết đến:

- `GetAudioCodec()`, `GetDisplay()`, `GetLed()`, `GetCamera()` (tùy chọn, trả null được)
- `GetNetwork()` + `StartNetwork()` (bất đồng bộ) — WiFi / 4G modem / RNDIS
- `SetPowerSaveLevel()` — điều chỉnh CPU/light-sleep

Các lớp nền theo kiểu mạng: `WifiBoard`, `Ml307Board`, `Nt26Board`,
`DualNetworkBoard`, `RndisBoard` (trong `main/boards/common/`). Board cụ thể kế
thừa lớp nền phù hợp và khai báo `DECLARE_BOARD(...)` đúng một lần.

## Cấu hình build

Một board được chọn qua chuỗi (xem `docs/tao-board-moi.md`):

```
config.json -> scripts/build.py -> main/Kconfig.projbuild -> main/CMakeLists.txt -> boards/<board>/
```

Partition: bảng v2 có partition `assets` — 4MB (esp32), 16MB (s3), 16m_c3 (c3).
