# Hướng dẫn cài đặt môi trường phát triển

Firmware cần **ESP-IDF v6.0.1 trở lên** (khuyến nghị **v6.1**). ESP-IDF 5.x
không được hỗ trợ. Dưới đây là hai cách cài phổ biến.

## Windows (khuyến nghị dùng EIM)

Cách dễ nhất là dùng **EIM (Espressif IDF Manager)** — tải tại
<https://github.com/espressif/idf-im/releases> (bản GUI) hoặc chạy:

```powershell
# Cài đặt dòng lệnh (PowerShell)
Invoke-WebRequest -Uri "https://github.com/espressif/idf-im/releases/latest/download/eim-windows-x86_64.exe" -OutFile eim.exe
.\eim.exe install --idf-version 6.1 --targets esp32,esp32s3,esp32c3
```

Hoặc cài thủ công theo hướng dẫn chính thức:
<https://docs.espressif.com/projects/esp-idf/v6.1/esp32/get-started/windows-setup.html>

Sau khi cài, mở **"ESP-IDF 6.1 PowerShell"** từ Start Menu (hoặc chạy script
kích hoạt mà trình cài đặt tạo, ví dụ `C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1`).

Kiểm tra:

```powershell
idf.py --version     # phải in ESP-IDF v6.1.x
```

### Lưu ý riêng khi gọi từ Git Bash / MSYS2

Nếu bạn kích hoạt IDF từ một shell mà biến môi trường `MSYSTEM` còn tồn tại
(ví dụ gọi PowerShell từ Git Bash), `idf.py` sẽ **chết im sau cảnh báo
"MSys/Mingw is no longer supported"**. Hãy gỡ biến này trước khi build:

```powershell
Remove-Item Env:MSYSTEM
```

## Linux / macOS

```sh
# 1) Cài dependency (Ubuntu)
sudo apt-get install git wget flex bison gperf python3 python3-pip python3-venv \
     cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

# 2) Tải ESP-IDF v6.1
mkdir -p ~/esp && cd ~/esp
git clone -b v6.1 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf && ./install.sh esp32,esp32s3,esp32c3

# 3) Mỗi phiên làm việc, nạp môi trường
source ~/esp/esp-idf/export.sh
idf.py --version
```

## Docker (không muốn cài lên máy)

Repo có sẵn `docker/firmware-builder/` — xem README trong đó. Ảnh cơ bản là
`espressif/idf:release-v6.1`:

```sh
docker run --rm -v "$PWD":/project -w /project espressif/idf:release-v6.1 \
    python scripts/build.py bread-compact-wifi --name bread-compact-wifi
```

## Sau khi cài xong

Quay lại [docs/huong-dan-build-va-flash.md](huong-dan-build-va-flash.md)
để build firmware đầu tiên.
