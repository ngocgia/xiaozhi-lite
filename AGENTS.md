# AGENTS.md — Hướng dẫn cho agent/contributor

## Dự án

XiaoZhi Lite là firmware trợ lý giọng nói ESP-IDF (C/C++), **bản tinh gọn chỉ hỗ trợ
3 dòng chip: ESP32, ESP32-S3 (ESP32-S3) và ESP32-C3**. Mỗi bản build chọn đúng
một board.

Dùng ESP-IDF v6.1 khi có thể; tối thiểu là ESP-IDF v6.0.1. Không hỗ trợ IDF 5.x.

## Kiến trúc

- `main/application.*`: vòng lặp sự kiện chính, vòng đời protocol, behavior cấp cao.
- `main/device_state_machine.*`: các chuyển trạng thái runtime hợp lệ.
- `main/boards/common/`: interface `Board` và các helper phần cứng/mạng dùng chung.
- `main/boards/**/`: 4 board hiện có (chân, khởi tạo, biến thể build).
- `main/audio/`: codec, task âm thanh, engine AFE/lite, wake word, Opus — đọc
  `main/audio/README.md` trước khi sửa.
- `main/protocols/`: API trung lập transport + WebSocket + MQTT/UDP.
- `main/display/` và `main/led/`: giao diện dùng chung.
- `main/mcp_server.*`: tool MCP phía thiết bị và dispatch.
- `main/Kconfig.projbuild`: cấu hình board và tính năng.
- `main/CMakeLists.txt`: chọn source, board, locale, font, assets.
- `scripts/build.py`: lệnh build chuẩn cho mọi board/biến thể.

Trước khi thêm code mới, đọc kỹ phần triển khai gần nhất đã có. Ưu tiên lớp sở hữu
hẹp nhất; không đặt hành vi đặc thù board vào module lõi.

## Quy tắc bắt buộc

- Giữ nguyên các thay đổi ngoài phạm vi; patch tập trung.
- Một bản build phải export đúng một board factory qua `DECLARE_BOARD(...)`.
- **Không bao giờ đổi pin của board có sẵn** để chạy phần cứng khác — hãy thêm
  board/variant mới với tên riêng; danh tính board ảnh hưởng tương thích OTA.
- Code lõi chỉ phụ thuộc interface `Board`, tuyệt đối không biết board cụ thể
  hay `config.h` của board.
- Camera, backlight, display, LED, pin... là tính năng tùy chọn — luôn kiểm tra null.
- Đổi trạng thái runtime qua `Application::SetDeviceState()` và state machine.
- Callback có thể chạy ngoài main task: dùng `Application::Schedule()` hoặc event bit
  khi đụng state/UI.
- Không block main event loop hay audio task. Tránh queue không giới hạn và
  cấp phát lớn lặp lại trong đường âm thanh.
- Giữ ngữ nghĩa message dùng chung trong `Protocol`; đổi contract phải xác minh
  cả WebSocket lẫn MQTT/UDP.
- Validate input mạng, giữ ownership `cJSON`. Khóa NVS là API bền vững — đổi khóa
  phải kèm migration.
- Tính năng theo chip phải guard bằng Kconfig/điều kiện biên dịch; không mặc định
  mọi target có PSRAM.
- Không sửa tay output sinh ra: `build/`, `releases/`, `managed_components/`,
  `components/`, `sdkconfig*`, `main/assets/lang_config.h`, các header mmap sinh ra.
- Chỉ format những file C/C++ đã đụng đến bằng `.clang-format`; không format hàng loạt.

## Board và cấu hình

Chuỗi chọn board là một dây chuyền ràng buộc:

`config.json (của board)` → `scripts/build.py` → `main/Kconfig.projbuild` →
`main/CMakeLists.txt` → source board và `config.h`.

Khi thêm board/variant, cập nhật **mọi mắt xích** của chuỗi: danh tính board duy
nhất, chip target đúng, flash/partition, đúng một `DECLARE_BOARD`, và tài liệu
board. Xem `docs/tao-board-moi.md`.

## Lệnh thường dùng

Nạp môi trường ESP-IDF trước (PowerShell Windows hoặc `export.sh` Linux):

```sh
idf.py --version

# Liệt kê board và biến thể
python scripts/build.py --list-boards

# Build chuẩn một biến thể
python scripts/build.py <thư-mục-board> --name <tên-biến-thể>

# Unit test phía host (không cần chip)
python -m unittest discover -s scripts/tests -v

# Format/check file đã sửa
clang-format -i <files>
clang-format --dry-run -Werror <files>
```

Lưu ý: `build.py` thay đổi `sdkconfig` cục bộ và trạng thái build — thư mục
`build/` không còn đại diện cho target trước đó.

## Kiểm chứng (Validation)

- Chỉ đổi board: build các biến thể bị ảnh hưởng + smoke test phần cứng.
- Đổi lõi / boards/common / audio / protocol / display / Kconfig / CMake:
  chạy unit test host + build các đường chip/mạng liên quan.
- Đổi protocol: kiểm cả WebSocket và MQTT/UDP.
- Đổi audio: kiểm thu âm, phát, wake/VAD, ngắt lời, reconnect, các chế độ AEC.
- Đổi UI/assets: kiểm đường OLED/LVGL tương ứng và kích thước partition.
- Luôn báo cáo đã test gì và cái gì còn cần phần cứng thật. Build thành công
  không đồng nghĩa đã kiểm chứng phần cứng.

## Tài liệu chuẩn

- Tổng quan & chính sách SDK: `README.md`
- Hướng dẫn board: `docs/tao-board-moi.md`
- Thiết kế âm thanh: `main/audio/README.md`
- Quy tắc code: `docs/code_style.md`
- Protocol: `docs/websocket.md`, `docs/mqtt-udp.md`, `docs/mcp-protocol.md`
- Ma trận CI: `.github/workflows/build.yml`
