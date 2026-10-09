# Hướng dẫn build, nạp firmware và khởi động lần đầu

Tất cả lệnh dưới đây chạy trong môi trường ESP-IDF đã kích hoạt
(Xem [docs/huong-dan-cai-dat.md](huong-dan-cai-dat.md)).

## 1. Chọn board/biến thể

Liệt kê mọi board có thể build:

```sh
python scripts/build.py --list-boards
```

Kết quả hiện tại của repo:

```
bread-compact-esp32            (ESP32)    -> bread-compact-esp32, bread-compact-esp32-128x32
bread-compact-wifi             (ESP32-S3) -> bread-compact-wifi, bread-compact-wifi-128x64
espressif/esp32-s3-box-3       (ESP32-S3) -> esp-box-3
xmini/c3                       (ESP32-C3) -> xmini-c3
```

## 2. Build

```sh
# Cú pháp chung
python scripts/build.py <thư-mục-board> --name <tên-biến-thể>

# Ví dụ: board DIY ESP32-S3 với OLED 128x64
python scripts/build.py bread-compact-wifi --name bread-compact-wifi-128x64
```

Script sẽ tự động: chọn đúng Kconfig board → tạo `sdkconfig` → tải component
(Component Manager) → build → gộp firmware thành `build/merged-binary.bin`
và nén vào `releases/` (nếu có).

Các tùy chọn hữu ích:

```sh
# Đổi ngôn ngữ giao diện (mặc định tiếng Việt - vi-VN)
python scripts/build.py xmini/c3 --name xmini-c3 --language en-US

# Đổi wake word (nihaoxiaozhi = "你好小智", hoặc disabled)
python scripts/build.py bread-compact-wifi --name bread-compact-wifi --wake-word nihaoxiaozhi

# Liệt kê wake word khả dụng theo chip
python scripts/build.py --list-wake-words

# Tùy chọn ngữ nghĩa cho board hỗ trợ (ví dụ OLED: display_model, wifi_provisioning)
python scripts/build.py bread-compact-wifi --name bread-compact-wifi \
    --build-options-json '{"display_model":"OLED_SSD1306_128X64"}'
```

Lưu ý quan trọng:

- `build.py` **ghi đè `sdkconfig` cục bộ** — thư mục `build/` sau mỗi lần chạy
  đại diện cho biến thể vừa build, không phải biến thể trước đó.
- Muốn chỉnh tay cấu hình (menuconfig): `idf.py menuconfig` — nhưng các lựa chọn
  board/OLED nên đi qua `build.py` để được kiểm chứng tự động.

## 3. Nạp firmware (flash)

Cắm board qua USB, xác định cổng COM (Windows: Device Manager; Linux: `/dev/ttyUSB*`
hoặc `/dev/ttyACM*`):

```sh
idf.py -p COM5 flash          # Windows
idf.py -p /dev/ttyUSB0 flash  # Linux
```

File nạp đã gộp sẵn (bootloader + partition + firmware + assets):
`build/merged-binary.bin` — có thể nạp bằng esptool nếu cần:

```sh
esptool.py --chip esp32s3 -p COM5 --baud 921600 write_flash 0x0 build/merged-binary.bin
```

## 4. Xem log

```sh
idf.py -p COM5 monitor
# Thoát: Ctrl+]
```

## 5. Cấu hình WiFi lần đầu

1. Sau khi khởi động, thiết bị không có WiFi sẽ **phát một hotspot** tên
   `Xiaozhi-XXXX` (mật khẩu mặc định trong màn hình/serial log).
2. Điện thoại nối vào hotspot đó → tự mở trang cấu hình → chọn WiFi nhà và nhập mật khẩu.
3. Thiết bị khởi động lại và kết nối. Đèn LED nháy theo trạng thái kết nối.

Ngoài hotspot, có thể dùng **BluFi** (cấu hình qua Bluetooth LE) bằng app
XiaoZhi (Android/iOS) — chọn khi build:

```sh
python scripts/build.py bread-compact-wifi --name bread-compact-wifi \
    --build-options-json '{"wifi_provisioning":"blufi"}'
```

## 6. Kích hoạt thiết bị

Lần đầu nối tới server chính thức xiaozhi.me, thiết bị cần kích hoạt:
mã kích hoạt được **đọc to từng chữ số** (và hiển thị trên màn hình nếu có).
Vào <https://xiaozhi.me> → đăng nhập → nhập mã để ghép thiết bị với tài khoản.
Sau đó bạn chọn mô hình (miễn phí Qwen realtime) trong console.

## 7. Sửa đổi firmware và nạp lại

Chỉnh code trong `main/` → build lại biến thể đang dùng → flash lại như bước 2-3.
Nếu chỉ đổi mã nguồn (không đổi cấu hình), có thể build nhanh trực tiếp:

```sh
idf.py build && idf.py -p COM5 flash monitor
```
