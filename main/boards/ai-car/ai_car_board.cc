/*
 * Board AI-CAR (ESP32-S3) — xe điều khiển bằng giọng nói qua XiaoZhi
 *
 * Thiết bị là một trợ lý XiaoZhi đầy đủ (wake word, hội thoại TTS, OTA...)
 * cộng thêm phần cứng riêng của xe:
 *   - 2 động cơ DC qua driver L298N        -> AI điều khiển bằng MCP tool
 *   - cảm biến siêu âm HC-SR04 phía trước  -> task chống va chạm chạy nền
 *   - màn hình TFT ST7789 240x240          -> hiển thị giao diện XiaoZhi
 *
 * AI (mô hình ngôn ngữ trên server) lái xe bằng cách gọi các tool:
 *   self.car.move(direction, duration_ms)  — tiến/lùi/trái/phải, tự dừng sau duration_ms
 *   self.car.stop()                        — dừng ngay lập tức
 *   self.car.get_distance()                — đọc khoảng cách vật cản phía trước (cm)
 */

#include "wifi_board.h"
#include "codecs/no_audio_codec.h"
#include "display/lcd_display.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "mcp_server.h"
#include "led/single_led.h"

#include <esp_log.h>
#include <esp_rom_sys.h>
#include <esp_timer.h>
#include <driver/spi_common.h>
#include <driver/gpio.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <atomic>
#include <mutex>
#include <string>

#define TAG "AiCarBoard"

/*
 * MotorController — điều khiển 2 động cơ DC qua L298N.
 *
 * L298N có 2 cầu H, mỗi cầu nhận 2 tín hiệu GPIO (INx):
 *   (1,0) = quay thuận, (0,1) = quay ngược, (0,0) = dừng.
 * Hướng của xe = tổ hợp quay của 2 bánh:
 *   forward  : hai bánh cùng quay thuận
 *   backward : hai bánh cùng quay ngược
 *   left     : bánh trái quay ngược, bánh phải quay thuận (xoay tại chỗ)
 *   right    : bánh trái quay thuận, bánh phải quay ngược
 */
class MotorController {
public:
    /**
     * Khởi tạo 4 GPIO điều khiển L298N ở chế độ output, mức thấp (xe đứng yên).
     */
    explicit MotorController(gpio_num_t a1, gpio_num_t a2, gpio_num_t b1, gpio_num_t b2)
        : a1_(a1), a2_(a2), b1_(b1), b2_(b2) {
        gpio_config_t config = {
            .pin_bit_mask = (1ULL << a1) | (1ULL << a2) | (1ULL << b1) | (1ULL << b2),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_ERROR_CHECK(gpio_config(&config));
        ApplyPins(0, 0, 0, 0);
    }

    /**
     * Đặt hướng chạy của xe.
     * @param direction một trong: "forward" | "backward" | "left" | "right" | "stop"
     * @return true nếu hướng hợp lệ và đã được áp dụng.
     */
    bool SetDirection(const std::string& direction) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (direction == "forward") {
            ApplyPins(1, 0, 1, 0);
        } else if (direction == "backward") {
            ApplyPins(0, 1, 0, 1);
        } else if (direction == "left") {
            ApplyPins(0, 1, 1, 0);
        } else if (direction == "right") {
            ApplyPins(1, 0, 0, 1);
        } else if (direction == "stop") {
            ApplyPins(0, 0, 0, 0);
        } else {
            return false;
        }
        direction_ = direction;
        return true;
    }

    /** Hướng hiện tại của xe ("stop" khi đứng yên). */
    std::string direction() {
        std::lock_guard<std::mutex> lock(mutex_);
        return direction_;
    }

    /** Dừng hai động cơ bất kể hướng hiện tại (dùng cho khẩn cấp/tự dừng). */
    void Stop() {
        SetDirection("stop");
    }

private:
    /**
     * Xuất mức logic ra 4 chân IN của L298N (chỉ gọi khi đã giữ mutex).
     */
    void ApplyPins(int a1, int a2, int b1, int b2) {
        gpio_set_level(a1_, a1);
        gpio_set_level(a2_, a2);
        gpio_set_level(b1_, b1);
        gpio_set_level(b2_, b2);
    }

    gpio_num_t a1_, a2_, b1_, b2_;
    std::mutex mutex_;
    std::string direction_ = "stop";
};

/*
 * UltrasonicSensor — đọc khoảng cách từ HC-SR04.
 *
 * Nguyên lý: phát một xung 10us vào TRIG, cảm biến trả về xung ở ECHO có
 * độ rộng tỷ lệ với khoảng cách: khoảng cách(cm) = thời gian(us) * 0.0343 / 2.
 *
 * Lưu ý phần cứng: ECHO của HC-SR04 là mức 5V — PHẢI qua cầu phân áp
 * (ví dụ 1kΩ/2kΩ) về ~3.3V trước khi nối vào GPIO.
 */
class UltrasonicSensor {
public:
    UltrasonicSensor(gpio_num_t trig, gpio_num_t echo) : trig_(trig), echo_(echo) {
        gpio_config_t trig_config = {
            .pin_bit_mask = (1ULL << trig),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_ERROR_CHECK(gpio_config(&trig_config));

        gpio_config_t echo_config = {
            .pin_bit_mask = (1ULL << echo),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_ENABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_ERROR_CHECK(gpio_config(&echo_config));
    }

    /**
     * Đo một lần và trả về khoảng cách tính bằng cm.
     * @return khoảng cách cm; trả về 999 khi hết thời gian chờ (không có vật cản / lỗi dây).
     */
    uint32_t MeasureDistanceCm() {
        // Phát xung 10us vào TRIG để cảm biến gửi loạt sóng siêu âm
        gpio_set_level(trig_, 0);
        esp_rom_delay_us(2);
        gpio_set_level(trig_, 1);
        esp_rom_delay_us(10);
        gpio_set_level(trig_, 0);

        // Chờ ECHO lên cao (tối đa 30ms — quá thời gian là mất tín hiệu)
        int64_t start = esp_timer_get_time();
        while (gpio_get_level(echo_) == 0) {
            if (esp_timer_get_time() - start > 30000) {
                return 999;
            }
        }
        // Đo độ rộng xung ECHO (tối đa 30ms ~ vật cản xa hơn ~5m)
        start = esp_timer_get_time();
        while (gpio_get_level(echo_) == 1) {
            if (esp_timer_get_time() - start > 30000) {
                return 999;
            }
        }
        int64_t duration_us = esp_timer_get_time() - start;
        return static_cast<uint32_t>(duration_us * 0.0343 / 2);
    }

private:
    gpio_num_t trig_;
    gpio_num_t echo_;
};

/*
 * Board chính của xe AI.
 */
class AiCarBoard : public WifiBoard {
private:
    Display* display_ = nullptr;
    Button boot_button_;
    MotorController motors_{MOTOR_A_IN1, MOTOR_A_IN2, MOTOR_B_IN3, MOTOR_B_IN4};
    UltrasonicSensor ultrasonic_{TRIG_PIN, ECHO_PIN};
    esp_timer_handle_t auto_stop_timer_ = nullptr;
    std::atomic<int> last_distance_cm_{999};

    /**
     * Khởi tạo bus SPI phần cứng (SPI3_HOST) cho màn hình TFT.
     */
    void InitializeSpi() {
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = DISPLAY_MOSI_PIN;
        buscfg.miso_io_num = GPIO_NUM_NC;          // Màn hình chỉ nhận, không có MISO
        buscfg.sclk_io_num = DISPLAY_CLK_PIN;
        buscfg.quadwp_io_num = GPIO_NUM_NC;
        buscfg.quadhd_io_num = GPIO_NUM_NC;
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    /**
     * Cài driver panel ST7789 và dựng đối tượng hiển thị XiaoZhi (LVGL).
     * Nếu màn hình không khởi động được, dùng NoDisplay để firmware vẫn chạy.
     */
    void InitializeLcdDisplay() {
        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel = nullptr;

        // Lớp IO SPI: clock 40MHz, CS không dùng (nối GND cứng)
        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = DISPLAY_CS_PIN;
        io_config.dc_gpio_num = DISPLAY_DC_PIN;
        io_config.spi_mode = DISPLAY_SPI_MODE;
        io_config.pclk_hz = 40 * 1000 * 1000;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI3_HOST, &io_config, &panel_io));

        // Driver ST7789 có sẵn trong ESP-IDF
        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = DISPLAY_RST_PIN;
        panel_config.rgb_ele_order = DISPLAY_RGB_ORDER;
        panel_config.bits_per_pixel = 16;
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel));

        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
        ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel, DISPLAY_INVERT_COLOR));
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel, DISPLAY_SWAP_XY));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y));

        display_ = new SpiLcdDisplay(panel_io, panel, DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                     DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y,
                                     DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
    }

    /**
     * Gán chức năng cho nút bấm:
     * - BOOT: khi đang khởi động -> vào chế độ cấu hình WiFi;
     *         khi chạy bình thường -> bắt đầu/kết thúc một lượt hội thoại.
     */
    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }
            app.ToggleChatState();
        });
    }

    /**
     * Tạo bộ đếm hẹn giờ "tự dừng": mỗi lệnh move(duration_ms) sẽ kích hoạt timer,
     * hết giờ xe tự dừng — tránh xe chạy mất khi AI quên gửi lệnh stop.
     */
    void InitializeAutoStopTimer() {
        esp_timer_create_args_t args = {
            .callback = [](void* arg) {
                // Timer chạy trong ngữ cảnh esp_timer — chỉ dừng động cơ (an toàn),
                // phần hiển thị không đụng ở đây.
                static_cast<AiCarBoard*>(arg)->motors_.Stop();
            },
            .arg = this,
            .name = "car_auto_stop",
        };
        ESP_ERROR_CHECK(esp_timer_create(&args, &auto_stop_timer_));
    }

    /**
     * Đăng ký MCP tools cho AI lái xe. Các tool được server gọi qua hội thoại,
     * callback chạy trên main loop của ứng dụng (an toàn khi đụng GPIO động cơ).
     */
    void InitializeTools() {
        auto& mcp_server = McpServer::GetInstance();

        // Tool: cho xe chạy theo hướng, tự dừng sau duration_ms (0 = chạy tới khi có lệnh khác)
        mcp_server.AddTool(
            "self.car.move",
            "Drive the car. Direction must be one of: forward, backward, left, right, stop. "
            "duration_ms is how long to drive before auto-stopping (0 = keep driving until the next command). "
            "Forward is rejected when an obstacle is closer than the safe distance.",
            PropertyList(std::vector<Property>{
                Property("direction", kPropertyTypeString),
                Property("duration_ms", kPropertyTypeInteger, 1500, 0, 10000),
            }),
            [this](const PropertyList& properties) -> ToolResult {
                std::string direction = properties["direction"].value<std::string>();
                int duration_ms = properties["duration_ms"].value<int>();

                // Chỉ chặn hướng tiến khi vật cản quá gần; lùi/trái/phải vẫn cho phép để thoát chỗ
                if (direction == "forward") {
                    uint32_t distance = ultrasonic_.MeasureDistanceCm();
                    last_distance_cm_ = static_cast<int>(distance);
                    if (distance < SAFE_DISTANCE_CM) {
                        return std::unexpected("Obstacle ahead at " + std::to_string(distance) +
                                               "cm, closer than the safe distance of " +
                                               std::to_string(SAFE_DISTANCE_CM) + "cm");
                    }
                }

                if (!motors_.SetDirection(direction)) {
                    return std::unexpected(
                        "Invalid direction '" + direction +
                        "'. Use forward, backward, left, right or stop.");
                }

                // Kích hoạt (hoặc đặt lại) bộ đếm tự dừng
                esp_timer_stop(auto_stop_timer_);
                if (duration_ms > 0) {
                    esp_timer_start_once(auto_stop_timer_,
                                         static_cast<uint64_t>(duration_ms) * 1000);
                }
                return "{\"direction\": \"" + direction + "\", \"duration_ms\": " +
                       std::to_string(duration_ms) + "}";
            });

        // Tool: dừng xe ngay lập tức
        mcp_server.AddTool(
            "self.car.stop",
            "Stop the car immediately.",
            PropertyList(),
            [this](const PropertyList& properties) -> ToolResult {
                esp_timer_stop(auto_stop_timer_);
                motors_.Stop();
                return true;
            });

        // Tool: đọc khoảng cách vật cản phía trước (cm), 999 = ngoài tầm đo
        mcp_server.AddTool(
            "self.car.get_distance",
            "Get the distance to the obstacle in front of the car, in centimeters. "
            "999 means no obstacle within range.",
            PropertyList(),
            [this](const PropertyList& properties) -> ToolResult {
                uint32_t distance = ultrasonic_.MeasureDistanceCm();
                last_distance_cm_ = static_cast<int>(distance);
                return "{\"distance_cm\": " + std::to_string(distance) + "}";
            });
    }

    /**
     * Task chống va chạm chạy nền trên core 0 (thay obstacleTask của bản Arduino).
     * Cứ 80ms đo khoảng cách một lần; nếu xe đang tiến mà vật cản vào trong
     * ngưỡng an toàn thì: dừng -> lùi nhẹ 400ms -> dừng hẳn, đồng thời hiện
     * cảnh báo lên màn hình (qua Schedule vì task này không phải main loop).
     */
    static void ObstacleTask(void* arg) {
        auto* self = static_cast<AiCarBoard*>(arg);
        while (true) {
            uint32_t distance = self->ultrasonic_.MeasureDistanceCm();
            self->last_distance_cm_ = static_cast<int>(distance);

            if (distance < SAFE_DISTANCE_CM &&
                self->motors_.direction() == "forward") {
                self->motors_.Stop();
                esp_timer_stop(self->auto_stop_timer_);

                // Hiện cảnh báo trên màn hình — phải qua Schedule để chạy trên main loop
                Application::GetInstance().Schedule([self]() {
                    if (self->display_) {
                        self->display_->ShowNotification("⚠ Vật cản phía trước!");
                    }
                });

                self->motors_.SetDirection("backward");
                vTaskDelay(pdMS_TO_TICKS(400));
                self->motors_.Stop();
            }
            vTaskDelay(pdMS_TO_TICKS(80));
        }
    }

public:
    AiCarBoard() : boot_button_(BOOT_BUTTON_GPIO) {
        InitializeSpi();
        InitializeLcdDisplay();
        InitializeButtons();
        InitializeAutoStopTimer();
        InitializeTools();
        // Task chống va chạm: core 0 (core 1 dành cho WiFi/âm thanh), stack 2048 byte
        xTaskCreatePinnedToCore(ObstacleTask, "car_obstacle", 2048, this, 2, nullptr, 0);
    }

    /** LED trạng thái WS2812 trên DevKit (đổi BUILTIN_LED_GPIO trong config.h nếu khác). */
    virtual Led* GetLed() override {
        static SingleLed led(BUILTIN_LED_GPIO);
        return &led;
    }

    /** Codec âm thanh "mềm": mic INMP441 + loa MAX98357A nối thẳng I2S, không chip codec. */
    virtual AudioCodec* GetAudioCodec() override {
        static NoAudioCodecSimplex audio_codec(
            AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_SPK_GPIO_BCLK, AUDIO_I2S_SPK_GPIO_LRCK, AUDIO_I2S_SPK_GPIO_DOUT,
            AUDIO_I2S_MIC_GPIO_SCK, AUDIO_I2S_MIC_GPIO_WS, AUDIO_I2S_MIC_GPIO_DIN);
        return &audio_codec;
    }

    /** Màn hình ST7789 (nếu khởi tạo lỗi thì là NoDisplay — firmware vẫn hoạt động). */
    virtual Display* GetDisplay() override {
        return display_;
    }
};

DECLARE_BOARD(AiCarBoard);
