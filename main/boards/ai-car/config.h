#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

/*
 * Cấu hình chân board AI-CAR (ESP32-S3)
 * Xe điều khiển bằng giọng nói: mic + loa I2S, màn hình ST7789,
 * driver động cơ L298N và cảm biến siêu âm HC-SR04.
 * Sơ đồ đấu dây chi tiết: docs/huong-dan-xe-ai.md
 */

// ---------------- Âm thanh I2S ----------------
// Tần số lấy mẫu mic (thiết bị gửi lên server ở 16 kHz)
#define AUDIO_INPUT_SAMPLE_RATE  16000
// Tần số phát loa (server trả về 24 kHz)
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

// Micro INMP441 (đơn công, riêng một bus I2S)
#define AUDIO_I2S_MIC_GPIO_SCK  GPIO_NUM_6   // SCK  (clock)  <- chân SCK của INMP441
#define AUDIO_I2S_MIC_GPIO_WS   GPIO_NUM_5   // WS   (chọn kênh) <- chân WS/LRCL của INMP441
#define AUDIO_I2S_MIC_GPIO_DIN  GPIO_NUM_4   // DIN  (dữ liệu) <- chân SD của INMP441

// Loa MAX98357A (đơn công, riêng một bus I2S)
#define AUDIO_I2S_SPK_GPIO_BCLK GPIO_NUM_15  // BCLK <- chân BCLK của MAX98357A
#define AUDIO_I2S_SPK_GPIO_LRCK GPIO_NUM_7   // LRC  <- chân LRC của MAX98357A
#define AUDIO_I2S_SPK_GPIO_DOUT GPIO_NUM_16  // DOUT <- chân DIN của MAX98357A

// ---------------- Màn hình TFT ST7789 240x240 (SPI) ----------------
#define DISPLAY_MOSI_PIN        GPIO_NUM_41  // SDA/MOSI <- chân DIN của màn hình
#define DISPLAY_CLK_PIN         GPIO_NUM_42  // SCL/SCLK <- chân CLK của màn hình
#define DISPLAY_DC_PIN          GPIO_NUM_43  // DC (data/command)
#define DISPLAY_RST_PIN         GPIO_NUM_44  // RES (reset)
#define DISPLAY_CS_PIN          GPIO_NUM_NC  // CS nối GND nên không dùng GPIO
#define DISPLAY_BACKLIGHT_PIN   GPIO_NUM_NC  // BL nối thẳng 3.3V (nếu có chân BL, khai báo ở đây)

// Tham số panel ST7789 240x240 (bản IPS 1.3"/2.0" phổ biến)
#define LCD_TYPE_ST7789_SERIAL
#define DISPLAY_WIDTH           240
#define DISPLAY_HEIGHT          240
#define DISPLAY_MIRROR_X        false
#define DISPLAY_MIRROR_Y        false
#define DISPLAY_SWAP_XY         false
#define DISPLAY_OFFSET_X        0
#define DISPLAY_OFFSET_Y        0
#define DISPLAY_INVERT_COLOR    true         // Màn IPS cần đảo màu để đúng sắc
#define DISPLAY_RGB_ORDER       LCD_RGB_ELEMENT_ORDER_RGB
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
#define DISPLAY_SPI_MODE        0            // Mode SPI của panel (0 cho đa số ST7789)

// ---------------- Driver động cơ L298N ----------------
#define MOTOR_A_IN1             GPIO_NUM_12  // IN1 của L298N (động cơ trái)
#define MOTOR_A_IN2             GPIO_NUM_13  // IN2 của L298N (động cơ trái)
#define MOTOR_B_IN3             GPIO_NUM_14  // IN3 của L298N (động cơ phải)
#define MOTOR_B_IN4             GPIO_NUM_21  // IN4 của L298N (động cơ phải)

// ---------------- Cảm biến siêu âm HC-SR04 ----------------
#define TRIG_PIN                GPIO_NUM_10  // TRIG (phát xung)
#define ECHO_PIN                GPIO_NUM_11  // ECHO (đo thời gian phản hồi) - QUA PHÂN ÁP!
#define SAFE_DISTANCE_CM        20           // Ngưỡng khoảng cách an toàn (cm)

// ---------------- Nút bấm và LED ----------------
#define BOOT_BUTTON_GPIO        GPIO_NUM_0   // Nút BOOT: bấm để bắt đầu/kết thúc hội thoại
#define BUILTIN_LED_GPIO        GPIO_NUM_48  // LED WS2812 trên DevKitC (đổi nếu board khác)

#endif // _BOARD_CONFIG_H_
