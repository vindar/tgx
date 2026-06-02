#include "tgx_p4_display.h"

#include <string.h>

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_ldo_regulator.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_lcd_st7703.h"

namespace tgx_p4
{

namespace
{

static const char* TAG = "tgx_p4_display";

static constexpr int LCD_RST_GPIO = 27;
static constexpr int LCD_BACKLIGHT_GPIO = 26;
static constexpr int LCD_BACKLIGHT_ON = 0;
static constexpr int MIPI_DSI_PHY_PWR_LDO_CHAN = 3;
static constexpr int MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV = 2500;

IRAM_ATTR bool refreshReadyCallback(esp_lcd_panel_handle_t, esp_lcd_dpi_panel_event_data_t*, void* user_ctx)
    {
    SemaphoreHandle_t sem = static_cast<SemaphoreHandle_t>(user_ctx);
    BaseType_t need_yield = pdFALSE;
    xSemaphoreGiveFromISR(sem, &need_yield);
    return need_yield == pdTRUE;
    }

} // namespace


uint64_t millis64()
    {
    return (uint64_t)(esp_timer_get_time() / 1000ULL);
    }


esp_err_t Display::begin()
    {
    if (_started) return ESP_OK;

    gpio_config_t bk_gpio_config = {};
    bk_gpio_config.mode = GPIO_MODE_OUTPUT;
    bk_gpio_config.pin_bit_mask = 1ULL << LCD_BACKLIGHT_GPIO;
    ESP_RETURN_ON_ERROR(gpio_config(&bk_gpio_config), TAG, "configure backlight GPIO failed");
    ESP_RETURN_ON_ERROR(gpio_set_level((gpio_num_t)LCD_BACKLIGHT_GPIO, LCD_BACKLIGHT_ON), TAG, "turn backlight on failed");

    esp_ldo_channel_config_t ldo_config = {};
    ldo_config.chan_id = MIPI_DSI_PHY_PWR_LDO_CHAN;
    ldo_config.voltage_mv = MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV;
    esp_ldo_channel_handle_t ldo = nullptr;
    ESP_RETURN_ON_ERROR(esp_ldo_acquire_channel(&ldo_config, &ldo), TAG, "enable MIPI DSI PHY LDO failed");
    _ldo = ldo;

    esp_lcd_dsi_bus_config_t bus_config = {};
    bus_config.bus_id = 0;
    bus_config.num_data_lanes = 2;
    bus_config.phy_clk_src = static_cast<mipi_dsi_phy_pllref_clock_source_t>(0);
    bus_config.lane_bit_rate_mbps = 480;
    esp_lcd_dsi_bus_handle_t dsi_bus = nullptr;
    ESP_RETURN_ON_ERROR(esp_lcd_new_dsi_bus(&bus_config, &dsi_bus), TAG, "create MIPI DSI bus failed");
    _dsi_bus = dsi_bus;

    esp_lcd_dbi_io_config_t dbi_config = {};
    dbi_config.virtual_channel = 0;
    dbi_config.lcd_cmd_bits = 8;
    dbi_config.lcd_param_bits = 8;
    esp_lcd_panel_io_handle_t dbi_io = nullptr;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_dbi(dsi_bus, &dbi_config, &dbi_io), TAG, "create DBI IO failed");
    _dbi_io = dbi_io;

    esp_lcd_dpi_panel_config_t dpi_config = {};
    dpi_config.dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT;
    dpi_config.dpi_clock_freq_mhz = 38;
    dpi_config.virtual_channel = 0;
    dpi_config.pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB565;
    dpi_config.in_color_format = LCD_COLOR_FMT_RGB565;
    dpi_config.out_color_format = LCD_COLOR_FMT_RGB565;
    dpi_config.num_fbs = 2;
    dpi_config.video_timing.h_size = LCD_WIDTH;
    dpi_config.video_timing.v_size = LCD_HEIGHT;
    dpi_config.video_timing.hsync_back_porch = 50;
    dpi_config.video_timing.hsync_pulse_width = 20;
    dpi_config.video_timing.hsync_front_porch = 50;
    dpi_config.video_timing.vsync_back_porch = 20;
    dpi_config.video_timing.vsync_pulse_width = 4;
    dpi_config.video_timing.vsync_front_porch = 20;
    dpi_config.flags.use_dma2d = true;

    st7703_vendor_config_t vendor_config = {};
    vendor_config.flags.use_mipi_interface = 1;
    vendor_config.mipi_config.dsi_bus = dsi_bus;
    vendor_config.mipi_config.dpi_config = &dpi_config;

    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.reset_gpio_num = LCD_RST_GPIO;
    panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
    panel_config.bits_per_pixel = 16;
    panel_config.vendor_config = &vendor_config;

    esp_lcd_panel_handle_t panel = nullptr;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7703(dbi_io, &panel_config, &panel), TAG, "create ST7703 panel failed");
    _panel = panel;

    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(panel), TAG, "panel reset failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(panel), TAG, "panel init failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(panel, true), TAG, "panel display on failed");

    SemaphoreHandle_t sem = xSemaphoreCreateBinary();
    ESP_RETURN_ON_FALSE(sem != nullptr, ESP_ERR_NO_MEM, TAG, "create refresh semaphore failed");
    _refresh_sem = sem;

    esp_lcd_dpi_panel_event_callbacks_t cbs = {};
    cbs.on_color_trans_done = refreshReadyCallback;
    ESP_RETURN_ON_ERROR(esp_lcd_dpi_panel_register_event_callbacks(panel, &cbs, sem), TAG, "register LCD callbacks failed");

    void* fb0 = nullptr;
    void* fb1 = nullptr;
    ESP_RETURN_ON_ERROR(esp_lcd_dpi_panel_get_frame_buffer(panel, 2, &fb0, &fb1), TAG, "get LCD framebuffers failed");
    _fb[0] = static_cast<uint16_t*>(fb0);
    _fb[1] = static_cast<uint16_t*>(fb1);
    ESP_RETURN_ON_FALSE(_fb[0] && _fb[1], ESP_ERR_NO_MEM, TAG, "LCD framebuffers are null");

    ESP_LOGI(TAG, "LCD framebuffers: fb0=%p fb1=%p, size=%d x %d RGB565", _fb[0], _fb[1], LCD_WIDTH, LCD_HEIGHT);
    clear(0x0000);
    _started = true;
    return ESP_OK;
    }


void Display::fillBackBuffer(uint16_t color)
    {
    uint16_t* dst = _fb[_back];
    const int count = LCD_WIDTH * LCD_HEIGHT;
    for (int i = 0; i < count; i++) dst[i] = color;
    }


void Display::clear(uint16_t color)
    {
    if (!_fb[0] || !_fb[1]) return;
    for (int b = 0; b < 2; b++)
        {
        uint16_t* dst = _fb[b];
        const int count = LCD_WIDTH * LCD_HEIGHT;
        for (int i = 0; i < count; i++) dst[i] = color;
        }
    esp_lcd_panel_handle_t panel = static_cast<esp_lcd_panel_handle_t>(_panel);
    esp_lcd_panel_draw_bitmap(panel, 0, 0, LCD_WIDTH, LCD_HEIGHT, _fb[0]);
    xSemaphoreTake(static_cast<SemaphoreHandle_t>(_refresh_sem), pdMS_TO_TICKS(100));
    _back = 1;
    }


void Display::presentCenteredRGB565(const uint16_t* src, int src_w, int src_h, uint16_t border_color)
    {
    if (!_started || !src || src_w <= 0 || src_h <= 0) return;
    if (src_w > LCD_WIDTH) src_w = LCD_WIDTH;
    if (src_h > LCD_HEIGHT) src_h = LCD_HEIGHT;

    fillBackBuffer(border_color);

    const int ox = (LCD_WIDTH - src_w) / 2;
    const int oy = (LCD_HEIGHT - src_h) / 2;
    uint16_t* dst = _fb[_back] + oy * LCD_WIDTH + ox;
    for (int y = 0; y < src_h; y++)
        {
        memcpy(dst + y * LCD_WIDTH, src + y * src_w, (size_t)src_w * sizeof(uint16_t));
        }

    esp_lcd_panel_handle_t panel = static_cast<esp_lcd_panel_handle_t>(_panel);
    esp_lcd_panel_draw_bitmap(panel, 0, 0, LCD_WIDTH, LCD_HEIGHT, _fb[_back]);
    xSemaphoreTake(static_cast<SemaphoreHandle_t>(_refresh_sem), pdMS_TO_TICKS(100));
    _back = 1 - _back;
    }


void Display::beginFrame(uint16_t color)
    {
    if (!_started) return;
    fillBackBuffer(color);
    }


void Display::copyTileRGB565(const uint16_t* src, int src_w, int src_h, int dst_x, int dst_y)
    {
    if (!_started || !src || src_w <= 0 || src_h <= 0) return;
    if (dst_x >= LCD_WIDTH || dst_y >= LCD_HEIGHT) return;

    int copy_w = src_w;
    int copy_h = src_h;
    int src_x = 0;
    int src_y = 0;

    if (dst_x < 0)
        {
        src_x = -dst_x;
        copy_w -= src_x;
        dst_x = 0;
        }
    if (dst_y < 0)
        {
        src_y = -dst_y;
        copy_h -= src_y;
        dst_y = 0;
        }
    if (dst_x + copy_w > LCD_WIDTH) copy_w = LCD_WIDTH - dst_x;
    if (dst_y + copy_h > LCD_HEIGHT) copy_h = LCD_HEIGHT - dst_y;
    if (copy_w <= 0 || copy_h <= 0) return;

    const uint16_t* src_row = src + src_y * src_w + src_x;
    uint16_t* dst_row = _fb[_back] + dst_y * LCD_WIDTH + dst_x;
    for (int y = 0; y < copy_h; y++)
        {
        memcpy(dst_row + y * LCD_WIDTH, src_row + y * src_w, (size_t)copy_w * sizeof(uint16_t));
        }
    }


void Display::presentFrame()
    {
    if (!_started) return;
    esp_lcd_panel_handle_t panel = static_cast<esp_lcd_panel_handle_t>(_panel);
    esp_lcd_panel_draw_bitmap(panel, 0, 0, LCD_WIDTH, LCD_HEIGHT, _fb[_back]);
    xSemaphoreTake(static_cast<SemaphoreHandle_t>(_refresh_sem), pdMS_TO_TICKS(100));
    _back = 1 - _back;
    }

} // namespace tgx_p4
