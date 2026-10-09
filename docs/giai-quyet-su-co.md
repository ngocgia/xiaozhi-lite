# Giải quyết sự cố thường gặp

## Build

### `idf.py` in cảnh báo "MSys/Mingw is no longer supported" rồi đứng im / thoát

Bạn đang chạy trong môi trường có biến `MSYSTEM` (thường là gọi từ Git Bash).
Gỡ biến trước khi build:

```powershell
Remove-Item Env:MSYSTEM      # PowerShell
```

Với Git Bash, hãy mở "ESP-IDF 6.1 PowerShell/ CMD" riêng từ Start Menu thay vì
gọi qua bash.

### `collect2.exe: fatal error: CreateProcess` hoặc linker báo "An Application Control policy has blocked this file"

Windows **Smart App Control** đang bật và chặn launcher `ld` của toolchain
(đặc trưng trên Windows 11 mới setup). Kiểm tra:

```powershell
Get-MpComputerStatus | Select-Object SmartAppControlState   # "On" = đang chặn
```

Cách xử lý (chọn một):

1. **Tắt Smart App Control**: Settings → Privacy & security → Windows Security →
   App & browser control → Smart App Control settings → **Off**.
   ⚠️ Tắt là một chiều — không bật lại được trừ khi cài lại Windows.
2. Build bằng **Docker** (`docker/firmware-builder/`) hoặc **GitHub Actions**
   (workflow `.github/workflows/build.yml` build trong container `espressif/idf:v6.1`).
3. Build trên máy Linux.

### `No module named 'rich_click'` khi chạy idf.py

`python` đang trỏ tới Python hệ thống thay vì venv của ESP-IDF. Chỉnh sửa:

- Dùng shortcut "ESP-IDF 6.1 PowerShell" do trình cài đặt tạo, hoặc
- Gọi thẳng venv: `C:\Espressif\tools\python\v6.1\venv\Scripts\python.exe`

### Component Manager không tải được component (lỗi mạng)

Xóa cache và thử lại: `idf.py reconfigure`. Nếu tường lửa chặn
`components.espressif.com` / `github.com`, cấu hình proxy qua biến môi trường
`HTTP_PROXY`/`HTTPS_PROXY`.

### `sdkconfig` "không sinh ra" sau khi đổi board

`build.py` validate mọi symbol sau reconfigure; nếu bạn sửa tay Kconfig/CMake
mà quên đăng ký board (xem `docs/tao-board-moi.md`), validation sẽ fail. Chạy
reconfigure thủ công để xem lỗi thật: `idf.py reconfigure`.

## Vận hành thiết bị

### Thiết bị không vào được WiFi

- Giữ nút BOOT khi khởi động để vào lại chế độ cấu hình WiFi (hotspot).
- Đã cấu hình BluFi nhưng muốn dùng hotspot: build lại với
  `--build-options-json '{"wifi_provisioning":"hotspot"}'`.

### Không nghe thấy âm thanh / mic không hoạt động

1. Kiểm tra đấu dây I2S (bảng chân: `docs/day-day-day-cam.md`), đặc biệt GND chung.
2. Xem log monitor: khi nói phải thấy VAD/wake word; khi server nói phải thấy
   packet audio xuống.
3. bật `CONFIG_USE_AUDIO_DEBUGGER` (menuconfig) rồi mở công cụ nghe UDP để
   kiểm tra PCM thật của mic.

### Wake word không nhận

- ESP32 không PSRAM và ESP32-C3 mặc định có thể tắt wake word — build với
  `--wake-word nihaoxiaozhi` (C3/ESP32 có PSRAM) hoặc dùng biến thể S3.
- Thử giảm ngưỡng với wake word tùy chỉnh (`CUSTOM_WAKE_WORD_THRESHOLD`).

### OTA thất bại / thiết bị lặp lại khởi động

- Bootloader có rollback: nếu firmware mới không "mark valid" được, nó tự quay
  về partition cũ — thử nạp lại firmware ổn định bằng cáp.
- Kiểm tra thiết bị còn ít nhất 2 partition OTA trong bảng partition (mặc định
  của repo là có).

### Màn hình OLED trắng / ngược

Chọn đúng loại OLED khi build: biến thể `...-128x64`/`...-128x32`, hoặc đổi
`DISPLAY_OLED_TYPE` qua `--build-options-json '{"display_model":"OLED_SH1106_128X64"}'`.

## Chạy unit test phía host

```sh
python -m unittest discover -s scripts/tests -v
```

Không cần chip hay ESP-IDF. Một test cần trình biên dịch C++ (ogg demuxer) sẽ
tự skip nếu máy không có `c++`/`g++`.
