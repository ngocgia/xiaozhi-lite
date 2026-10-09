# Hướng dẫn thêm board mới

Repo này giữ nguyên chuỗi chọn board của upstream nên **board từ
[78/xiaozhi-esp32](https://github.com/78/xiaozhi-esp32) có thể copy nguyên
thư mục sang** (nếu board đó dùng chip ESP32 / S3 / C3) rồi thực hiện các bước
2-6 dưới đây. Nếu board upstream dùng chip khác (P4, C6, S31...), bạn phải tự
chuyển đổi phần cứng.

> ⚠️ Danh tính board (`type` trong config.json) ảnh hưởng đến OTA: đừng dùng lại
> `type`/`name` của board có sẵn. Nếu server thấy trùng tên, firmware có thể bị
> OTA ghi đè sai.

## Chuỗi chọn board

Một board được "nhận diện" qua 5 mắt xích — thêm board mới phải cập nhật đủ:

```
config.json của board
   └─> scripts/build.py        (tự quét main/boards/**/config.json)
        └─> main/Kconfig.projbuild  (symbol CONFIG_BOARD_TYPE_*)
             └─> main/CMakeLists.txt (map symbol -> BOARD_DIR, font, emoji)
                  └─> main/boards/<board>/  (config.h + *.cc + DECLARE_BOARD)
```

## Ví dụ thực tế: thêm board `my-board-v1` (ESP32-S3)

### 1) Tạo thư mục board

```
main/boards/my-board-v1/
├── config.json
├── config.h
└── my_board_v1.cc
```

`config.json` — khai báo danh tính và biến thể build:

```json
{
    "type": "my-board-v1",
    "target": "esp32s3",
    "builds": [
        { "name": "my-board-v1" }
    ]
}
```

`config.h` — định nghĩa chân phần cứng (xem ví dụ đầy đủ trong
`main/boards/bread-compact-wifi/config.h`): chân I2S mic/loa, I2C màn hình,
nút bấm, LED...

`my_board_v1.cc` — lớp board kế thừa `WifiBoard` (hoặc `Ml307Board` cho 4G...),
kết thúc bằng **đúng một** `DECLARE_BOARD(MyBoardV1);`.

### 2) Đăng ký trong Kconfig

Thêm vào `choice BOARD_TYPE` trong `main/Kconfig.projbuild`:

```
config BOARD_TYPE_MY_BOARD_V1
    bool "My Board V1 (mô tả tiếng Việt)"
    depends on IDF_TARGET_ESP32S3
```

### 3) Đăng ký trong CMake

Thêm một nhánh vào chuỗi `if/elseif` trong `main/CMakeLists.txt`:

```cmake
elseif(CONFIG_BOARD_TYPE_MY_BOARD_V1)
    set(BOARD_DIR "my-board-v1")
    set(BUILTIN_TEXT_FONT font_noto_sans_basic_14_1)
    set(BUILTIN_ICON_FONT font_material_symbols_14_1)
```

(Chỉ board có màn màu mới cần `DEFAULT_EMOJI_COLLECTION`.)

### 4) Build và kiểm tra

```sh
python scripts/build.py --list-boards          # board mới phải xuất hiện
python scripts/build.py my-board-v1 --name my-board-v1
```

`build.py` tự kiểm tra: symbol Kconfig hợp lệ với chip target, không trùng
danh tính OTA, mọi `CONFIG_*` trong `sdkconfig_append` đều được Kconfig chấp nhận.

### 5) Nhiều biến thể cho một board

Thêm phần tử vào `builds[]` — khác nhau qua `sdkconfig_append`:

```json
"builds": [
    { "name": "my-board-v1",        "sdkconfig_append": ["CONFIG_OLED_SSD1306_128X32=y"] },
    { "name": "my-board-v1-128x64", "sdkconfig_append": ["CONFIG_OLED_SSD1306_128X64=y"] }
]
```

Nhiều biến thể dùng chung một `BOARD_DIR` và một `config.h` (rẽ nhánh bằng
`#if CONFIG_...`).

### 6) Cập nhật CI (tùy chọn)

Thêm biến thể vào `scripts/ci/representative-variants.json` nếu muốn CI build
thường xuyên board của bạn.

## Checklist phần cứng khi thiết kế board

1. **Codec âm thanh**: dùng codec chuẩn có sẵn (`ES8311`, `ES8388`, `Box`...)
   hoặc I2S trần (`NoAudioCodec`) như bread-compact. Thêm codec mới trong
   `main/audio/codecs/`.
2. **Wake word**: cần PSRAM. ESP32-S3 dùng AFE (AEC/VAD); ESP32/C3 chỉ WakeNet
   đơn giản. Khai báo qua `WAKE_WORD_TYPE` trong Kconfig.
3. **AEC thiết bị**: cần đường tín hiệu tham chiếu loa về codec (xem
   `AUDIO_INPUT_REFERENCE` của esp-box-3) và thêm board vào điều kiện
   `USE_DEVICE_AEC` trong Kconfig.
4. **Màn hình**: OLED SSD1306/SH1106 dùng chung driver có sẵn; LCD cần driver
   panel phù hợp trong `main/display/`.
5. **Partition**: flash 4MB dùng `partitions/v2/4m.csv`, 16MB dùng `16m.csv`,
   C3 dùng `16m_c3.csv` — xem `sdkconfig.defaults.<target>`.

## Nguyên tắc bắt buộc

- Không đặt hành vi đặc thù board vào code lõi (`application.cc`, `audio/`...).
  Code lõi chỉ biết interface `Board`.
- Tính năng phần cứng là tùy chọn: màn hình, camera, pin... phải hoạt động cả
  khi board không có (trả `nullptr` từ getter).
- Chạy `python -m unittest discover -s scripts/tests -v` sau khi sửa Kconfig/
  CMake — có test kiểm tra tính nhất quán của chuỗi chọn board.
