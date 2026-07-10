#include "tgx_p4_display.h"

#include <string.h>

#include "driver/gpio.h"
#include "driver/ppa.h"
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
#include "esp_lcd_jd9365.h"

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
static constexpr int MIPI_DSI_LANE_NUM = 2;
static constexpr int LCD_REFRESH_WAIT_MS = 100;

// Vendor-specific initialization sequence validated on the JD9365-based
// ESP32-P4-WIFI6-Touch-LCD-XC panel (720x720, 2-lane MIPI-DSI, 80MHz DPI clock).
// Kept byte-for-byte identical to the working display bring-up test.
// Note: the 0x3A entries below live inside page 1/page 2 blocks (after an
// 0xE0 page-select command other than 0x00), so the driver does not treat
// them as COLMOD overrides -- the real page-0 COLMOD register is set from
// esp_lcd_panel_dev_config_t::bits_per_pixel and is unaffected by this table.
static const jd9365_lcd_init_cmd_t lcd_init_cmds[] = {
    {0xE0, (uint8_t[]){0x00}, 1, 0},

    {0xE1, (uint8_t[]){0x93}, 1, 0},
    {0xE2, (uint8_t[]){0x65}, 1, 0},
    {0xE3, (uint8_t[]){0xF8}, 1, 0},
    {0x80, (uint8_t[]){0x01}, 1, 0},

    {0xE0, (uint8_t[]){0x01}, 1, 0},

    {0x00, (uint8_t[]){0x00}, 1, 0},
    {0x01, (uint8_t[]){0x41}, 1, 0},
    {0x03, (uint8_t[]){0x10}, 1, 0},
    {0x04, (uint8_t[]){0x44}, 1, 0},

    {0x17, (uint8_t[]){0x00}, 1, 0},
    {0x18, (uint8_t[]){0xD0}, 1, 0},
    {0x19, (uint8_t[]){0x00}, 1, 0},
    {0x1A, (uint8_t[]){0x00}, 1, 0},
    {0x1B, (uint8_t[]){0xD0}, 1, 0},
    {0x1C, (uint8_t[]){0x00}, 1, 0},

    {0x24, (uint8_t[]){0xFE}, 1, 0},
    {0x35, (uint8_t[]){0x26}, 1, 0},

    {0x37, (uint8_t[]){0x09}, 1, 0},

    {0x38, (uint8_t[]){0x04}, 1, 0},
    {0x39, (uint8_t[]){0x08}, 1, 0},
    {0x3A, (uint8_t[]){0x0A}, 1, 0},
    {0x3C, (uint8_t[]){0x78}, 1, 0},
    {0x3D, (uint8_t[]){0xFF}, 1, 0},
    {0x3E, (uint8_t[]){0xFF}, 1, 0},
    {0x3F, (uint8_t[]){0xFF}, 1, 0},

    {0x40, (uint8_t[]){0x04}, 1, 0},
    {0x41, (uint8_t[]){0x64}, 1, 0},
    {0x42, (uint8_t[]){0xC7}, 1, 0},
    {0x43, (uint8_t[]){0x18}, 1, 0},
    {0x44, (uint8_t[]){0x0B}, 1, 0},
    {0x45, (uint8_t[]){0x14}, 1, 0},

    {0x55, (uint8_t[]){0x02}, 1, 0},
    {0x57, (uint8_t[]){0x49}, 1, 0},
    {0x59, (uint8_t[]){0x0A}, 1, 0},
    {0x5A, (uint8_t[]){0x1B}, 1, 0},
    {0x5B, (uint8_t[]){0x19}, 1, 0},

    {0x5D, (uint8_t[]){0x7F}, 1, 0},
    {0x5E, (uint8_t[]){0x56}, 1, 0},
    {0x5F, (uint8_t[]){0x43}, 1, 0},
    {0x60, (uint8_t[]){0x37}, 1, 0},
    {0x61, (uint8_t[]){0x33}, 1, 0},
    {0x62, (uint8_t[]){0x25}, 1, 0},
    {0x63, (uint8_t[]){0x2A}, 1, 0},
    {0x64, (uint8_t[]){0x16}, 1, 0},
    {0x65, (uint8_t[]){0x30}, 1, 0},
    {0x66, (uint8_t[]){0x2F}, 1, 0},
    {0x67, (uint8_t[]){0x32}, 1, 0},
    {0x68, (uint8_t[]){0x53}, 1, 0},
    {0x69, (uint8_t[]){0x43}, 1, 0},
    {0x6A, (uint8_t[]){0x4C}, 1, 0},
    {0x6B, (uint8_t[]){0x40}, 1, 0},
    {0x6C, (uint8_t[]){0x3D}, 1, 0},
    {0x6D, (uint8_t[]){0x31}, 1, 0},
    {0x6E, (uint8_t[]){0x20}, 1, 0},
    {0x6F, (uint8_t[]){0x0F}, 1, 0},

    {0x70, (uint8_t[]){0x7F}, 1, 0},
    {0x71, (uint8_t[]){0x56}, 1, 0},
    {0x72, (uint8_t[]){0x43}, 1, 0},
    {0x73, (uint8_t[]){0x37}, 1, 0},
    {0x74, (uint8_t[]){0x33}, 1, 0},
    {0x75, (uint8_t[]){0x25}, 1, 0},
    {0x76, (uint8_t[]){0x2A}, 1, 0},
    {0x77, (uint8_t[]){0x16}, 1, 0},
    {0x78, (uint8_t[]){0x30}, 1, 0},
    {0x79, (uint8_t[]){0x2F}, 1, 0},
    {0x7A, (uint8_t[]){0x32}, 1, 0},
    {0x7B, (uint8_t[]){0x53}, 1, 0},
    {0x7C, (uint8_t[]){0x43}, 1, 0},
    {0x7D, (uint8_t[]){0x4C}, 1, 0},
    {0x7E, (uint8_t[]){0x40}, 1, 0},
    {0x7F, (uint8_t[]){0x3D}, 1, 0},
    {0x80, (uint8_t[]){0x31}, 1, 0},
    {0x81, (uint8_t[]){0x20}, 1, 0},
    {0x82, (uint8_t[]){0x0F}, 1, 0},

    {0xE0, (uint8_t[]){0x02}, 1, 0},
    {0x00, (uint8_t[]){0x5F}, 1, 0},
    {0x01, (uint8_t[]){0x5F}, 1, 0},
    {0x02, (uint8_t[]){0x5E}, 1, 0},
    {0x03, (uint8_t[]){0x5E}, 1, 0},
    {0x04, (uint8_t[]){0x50}, 1, 0},
    {0x05, (uint8_t[]){0x48}, 1, 0},
    {0x06, (uint8_t[]){0x48}, 1, 0},
    {0x07, (uint8_t[]){0x4A}, 1, 0},
    {0x08, (uint8_t[]){0x4A}, 1, 0},
    {0x09, (uint8_t[]){0x44}, 1, 0},
    {0x0A, (uint8_t[]){0x44}, 1, 0},
    {0x0B, (uint8_t[]){0x46}, 1, 0},
    {0x0C, (uint8_t[]){0x46}, 1, 0},
    {0x0D, (uint8_t[]){0x5F}, 1, 0},
    {0x0E, (uint8_t[]){0x5F}, 1, 0},
    {0x0F, (uint8_t[]){0x57}, 1, 0},
    {0x10, (uint8_t[]){0x57}, 1, 0},
    {0x11, (uint8_t[]){0x77}, 1, 0},
    {0x12, (uint8_t[]){0x77}, 1, 0},
    {0x13, (uint8_t[]){0x40}, 1, 0},
    {0x14, (uint8_t[]){0x42}, 1, 0},
    {0x15, (uint8_t[]){0x5F}, 1, 0},

    {0x16, (uint8_t[]){0x5F}, 1, 0},
    {0x17, (uint8_t[]){0x5F}, 1, 0},
    {0x18, (uint8_t[]){0x5E}, 1, 0},
    {0x19, (uint8_t[]){0x5E}, 1, 0},
    {0x1A, (uint8_t[]){0x50}, 1, 0},
    {0x1B, (uint8_t[]){0x49}, 1, 0},
    {0x1C, (uint8_t[]){0x49}, 1, 0},
    {0x1D, (uint8_t[]){0x4B}, 1, 0},
    {0x1E, (uint8_t[]){0x4B}, 1, 0},
    {0x1F, (uint8_t[]){0x45}, 1, 0},
    {0x20, (uint8_t[]){0x45}, 1, 0},
    {0x21, (uint8_t[]){0x47}, 1, 0},
    {0x22, (uint8_t[]){0x47}, 1, 0},
    {0x23, (uint8_t[]){0x5F}, 1, 0},
    {0x24, (uint8_t[]){0x5F}, 1, 0},
    {0x25, (uint8_t[]){0x57}, 1, 0},
    {0x26, (uint8_t[]){0x57}, 1, 0},
    {0x27, (uint8_t[]){0x77}, 1, 0},
    {0x28, (uint8_t[]){0x77}, 1, 0},
    {0x29, (uint8_t[]){0x41}, 1, 0},
    {0x2A, (uint8_t[]){0x43}, 1, 0},
    {0x2B, (uint8_t[]){0x5F}, 1, 0},

    {0x2C, (uint8_t[]){0x1E}, 1, 0},
    {0x2D, (uint8_t[]){0x1E}, 1, 0},
    {0x2E, (uint8_t[]){0x1F}, 1, 0},
    {0x2F, (uint8_t[]){0x1F}, 1, 0},
    {0x30, (uint8_t[]){0x10}, 1, 0},
    {0x31, (uint8_t[]){0x07}, 1, 0},
    {0x32, (uint8_t[]){0x07}, 1, 0},
    {0x33, (uint8_t[]){0x05}, 1, 0},
    {0x34, (uint8_t[]){0x05}, 1, 0},
    {0x35, (uint8_t[]){0x0B}, 1, 0},
    {0x36, (uint8_t[]){0x0B}, 1, 0},
    {0x37, (uint8_t[]){0x09}, 1, 0},
    {0x38, (uint8_t[]){0x09}, 1, 0},
    {0x39, (uint8_t[]){0x1F}, 1, 0},
    {0x3A, (uint8_t[]){0x1F}, 1, 0},
    {0x3B, (uint8_t[]){0x17}, 1, 0},
    {0x3C, (uint8_t[]){0x17}, 1, 0},
    {0x3D, (uint8_t[]){0x17}, 1, 0},
    {0x3E, (uint8_t[]){0x17}, 1, 0},
    {0x3F, (uint8_t[]){0x03}, 1, 0},
    {0x40, (uint8_t[]){0x01}, 1, 0},
    {0x41, (uint8_t[]){0x1F}, 1, 0},

    {0x42, (uint8_t[]){0x1E}, 1, 0},
    {0x43, (uint8_t[]){0x1E}, 1, 0},
    {0x44, (uint8_t[]){0x1F}, 1, 0},
    {0x45, (uint8_t[]){0x1F}, 1, 0},
    {0x46, (uint8_t[]){0x10}, 1, 0},
    {0x47, (uint8_t[]){0x06}, 1, 0},
    {0x48, (uint8_t[]){0x06}, 1, 0},
    {0x49, (uint8_t[]){0x04}, 1, 0},
    {0x4A, (uint8_t[]){0x04}, 1, 0},
    {0x4B, (uint8_t[]){0x0A}, 1, 0},
    {0x4C, (uint8_t[]){0x0A}, 1, 0},
    {0x4D, (uint8_t[]){0x08}, 1, 0},
    {0x4E, (uint8_t[]){0x08}, 1, 0},
    {0x4F, (uint8_t[]){0x1F}, 1, 0},
    {0x50, (uint8_t[]){0x1F}, 1, 0},
    {0x51, (uint8_t[]){0x17}, 1, 0},
    {0x52, (uint8_t[]){0x17}, 1, 0},
    {0x53, (uint8_t[]){0x17}, 1, 0},
    {0x54, (uint8_t[]){0x17}, 1, 0},
    {0x55, (uint8_t[]){0x02}, 1, 0},
    {0x56, (uint8_t[]){0x00}, 1, 0},
    {0x57, (uint8_t[]){0x1F}, 1, 0},

    {0xE0, (uint8_t[]){0x02}, 1, 0},
    {0x58, (uint8_t[]){0x40}, 1, 0},
    {0x59, (uint8_t[]){0x00}, 1, 0},
    {0x5A, (uint8_t[]){0x00}, 1, 0},
    {0x5B, (uint8_t[]){0x30}, 1, 0},
    {0x5C, (uint8_t[]){0x01}, 1, 0},
    {0x5D, (uint8_t[]){0x30}, 1, 0},
    {0x5E, (uint8_t[]){0x01}, 1, 0},
    {0x5F, (uint8_t[]){0x02}, 1, 0},
    {0x60, (uint8_t[]){0x30}, 1, 0},
    {0x61, (uint8_t[]){0x03}, 1, 0},
    {0x62, (uint8_t[]){0x04}, 1, 0},
    {0x63, (uint8_t[]){0x04}, 1, 0},
    {0x64, (uint8_t[]){0xA6}, 1, 0},
    {0x65, (uint8_t[]){0x43}, 1, 0},
    {0x66, (uint8_t[]){0x30}, 1, 0},
    {0x67, (uint8_t[]){0x73}, 1, 0},
    {0x68, (uint8_t[]){0x05}, 1, 0},
    {0x69, (uint8_t[]){0x04}, 1, 0},
    {0x6A, (uint8_t[]){0x7F}, 1, 0},
    {0x6B, (uint8_t[]){0x08}, 1, 0},
    {0x6C, (uint8_t[]){0x00}, 1, 0},
    {0x6D, (uint8_t[]){0x04}, 1, 0},
    {0x6E, (uint8_t[]){0x04}, 1, 0},
    {0x6F, (uint8_t[]){0x88}, 1, 0},

    {0x75, (uint8_t[]){0xD9}, 1, 0},
    {0x76, (uint8_t[]){0x00}, 1, 0},
    {0x77, (uint8_t[]){0x33}, 1, 0},
    {0x78, (uint8_t[]){0x43}, 1, 0},

    {0xE0, (uint8_t[]){0x00}, 1, 0},
    {0x11, (uint8_t[]){0x00}, 1, 120},

    {0x29, (uint8_t[]){0x00}, 1, 20},
    {0x35, (uint8_t[]){0x00}, 1, 0},
};


IRAM_ATTR bool refreshDoneCallback(esp_lcd_panel_handle_t, esp_lcd_dpi_panel_event_data_t*, void* user_ctx)
    {
    SemaphoreHandle_t sem = static_cast<SemaphoreHandle_t>(user_ctx);
    BaseType_t need_yield = pdFALSE;
    xSemaphoreGiveFromISR(sem, &need_yield);
    return need_yield == pdTRUE;
    }


IRAM_ATTR bool ppaCopyDoneCallback(ppa_client_handle_t, ppa_event_data_t*, void* user_ctx)
    {
    SemaphoreHandle_t sem = static_cast<SemaphoreHandle_t>(user_ctx);
    BaseType_t need_yield = pdFALSE;
    xSemaphoreGiveFromISR(sem, &need_yield);
    return need_yield == pdTRUE;
    }


void fillRGB565Buffer(uint16_t* dst, int count, uint16_t color)
    {
    if (!dst || count <= 0) return;

    while ((count > 0) && ((reinterpret_cast<uintptr_t>(dst) & 3U) != 0))
        {
        *dst++ = color;
        count--;
        }

    const uint32_t packed = static_cast<uint32_t>(color) | (static_cast<uint32_t>(color) << 16);
    uint32_t* dst32 = reinterpret_cast<uint32_t*>(dst);
    const int count32 = count / 2;
    for (int i = 0; i < count32; i++) dst32[i] = packed;

    if (count & 1) dst[count - 1] = color;
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

    esp_lcd_dsi_bus_config_t bus_config = JD9365_PANEL_BUS_DSI_2CH_CONFIG();
    esp_lcd_dsi_bus_handle_t dsi_bus = nullptr;
    ESP_RETURN_ON_ERROR(esp_lcd_new_dsi_bus(&bus_config, &dsi_bus), TAG, "create MIPI DSI bus failed");
    _dsi_bus = dsi_bus;

    esp_lcd_dbi_io_config_t dbi_config = JD9365_PANEL_IO_DBI_CONFIG();
    esp_lcd_panel_io_handle_t dbi_io = nullptr;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_dbi(dsi_bus, &dbi_config, &dbi_io), TAG, "create DBI IO failed");
    _dbi_io = dbi_io;

    // RGB565 keeps the continuous MIPI-DSI video-mode fetch from PSRAM
    // within bandwidth: RGB888 at the same 80MHz DPI clock caused DMA
    // underruns on this panel.
    esp_lcd_dpi_panel_config_t dpi_config = {};
    dpi_config.dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT;
    dpi_config.dpi_clock_freq_mhz = 80;
    dpi_config.virtual_channel = 0;
    dpi_config.pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB565;
    dpi_config.in_color_format = LCD_COLOR_FMT_RGB565;
    dpi_config.out_color_format = LCD_COLOR_FMT_RGB565;
    dpi_config.num_fbs = 2;
    dpi_config.video_timing.h_size = LCD_WIDTH;
    dpi_config.video_timing.v_size = LCD_HEIGHT;
    dpi_config.video_timing.hsync_back_porch = 20;
    dpi_config.video_timing.hsync_pulse_width = 20;
    dpi_config.video_timing.hsync_front_porch = 40;
    dpi_config.video_timing.vsync_back_porch = 12;
    dpi_config.video_timing.vsync_pulse_width = 4;
    dpi_config.video_timing.vsync_front_porch = 24;
    dpi_config.flags.use_dma2d = true;

    jd9365_vendor_config_t vendor_config = {};
    vendor_config.init_cmds = lcd_init_cmds;
    vendor_config.init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]);
    vendor_config.mipi_config.dsi_bus = dsi_bus;
    vendor_config.mipi_config.dpi_config = &dpi_config;
    vendor_config.mipi_config.lane_num = MIPI_DSI_LANE_NUM;

    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.reset_gpio_num = LCD_RST_GPIO;
    panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
    panel_config.bits_per_pixel = 16;
    panel_config.vendor_config = &vendor_config;

    esp_lcd_panel_handle_t panel = nullptr;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_jd9365(dbi_io, &panel_config, &panel), TAG, "create JD9365 panel failed");
    _panel = panel;

    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(panel), TAG, "panel reset failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(panel), TAG, "panel init failed");
    // The panel is mounted upside down relative to the framebuffer's
    // native row/column scan order; flip both axes to compensate.
    ESP_RETURN_ON_ERROR(esp_lcd_panel_mirror(panel, true, true), TAG, "panel mirror failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(panel, true), TAG, "panel display on failed");

    SemaphoreHandle_t sem = xSemaphoreCreateBinary();
    ESP_RETURN_ON_FALSE(sem != nullptr, ESP_ERR_NO_MEM, TAG, "create refresh semaphore failed");
    _refresh_sem = sem;

    esp_lcd_dpi_panel_event_callbacks_t cbs = {};
    cbs.on_refresh_done = refreshDoneCallback;
    ESP_RETURN_ON_ERROR(esp_lcd_dpi_panel_register_event_callbacks(panel, &cbs, sem), TAG, "register LCD callbacks failed");

    SemaphoreHandle_t copy_sem = xSemaphoreCreateBinary();
    ESP_RETURN_ON_FALSE(copy_sem != nullptr, ESP_ERR_NO_MEM, TAG, "create PPA copy semaphore failed");
    _copy_sem = copy_sem;

    ppa_client_config_t ppa_config = {};
    ppa_config.oper_type = PPA_OPERATION_SRM;
    ppa_config.max_pending_trans_num = 1;
    ppa_config.data_burst_length = PPA_DATA_BURST_LENGTH_128;
    ppa_client_handle_t ppa_client = nullptr;
    ESP_RETURN_ON_ERROR(ppa_register_client(&ppa_config, &ppa_client), TAG, "register PPA SRM client failed");
    ppa_event_callbacks_t ppa_cbs = {};
    ppa_cbs.on_trans_done = ppaCopyDoneCallback;
    ESP_RETURN_ON_ERROR(ppa_client_register_event_callbacks(ppa_client, &ppa_cbs), TAG, "register PPA callbacks failed");
    _ppa_srm_client = ppa_client;

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
    waitCopyDone();
    ensureBackBufferReady();
    uint16_t* dst = _fb[_back];
    fillRGB565Buffer(dst, LCD_WIDTH * LCD_HEIGHT, color);
    _centered_buffer_valid[_back] = false;
    }


void Display::invalidateCenteredCache()
    {
    _centered_cache_started = false;
    _centered_buffer_valid[0] = false;
    _centered_buffer_valid[1] = false;
    _centered_w = 0;
    _centered_h = 0;
    }


void Display::drainRefreshSemaphore()
    {
    SemaphoreHandle_t sem = static_cast<SemaphoreHandle_t>(_refresh_sem);
    if (!sem) return;
    while (xSemaphoreTake(sem, 0) == pdTRUE) {}
    }


bool Display::waitForRefreshDone(const char* context)
    {
    SemaphoreHandle_t sem = static_cast<SemaphoreHandle_t>(_refresh_sem);
    if (!sem) return true;

    if (xSemaphoreTake(sem, pdMS_TO_TICKS(LCD_REFRESH_WAIT_MS)) == pdTRUE)
        {
        return true;
        }

    ESP_LOGW(TAG, "%s: LCD refresh wait timed out", context ? context : "present");
    return false;
    }


void Display::ensureBackBufferReady()
    {
    if (!_back_needs_refresh_wait) return;
    waitForRefreshDone("reuse back buffer");
    _back_needs_refresh_wait = false;
    }


void Display::presentBackBuffer(const char* context)
    {
    waitCopyDone();
    esp_lcd_panel_handle_t panel = static_cast<esp_lcd_panel_handle_t>(_panel);
    esp_err_t err = esp_lcd_panel_draw_bitmap(panel, 0, 0, LCD_WIDTH, LCD_HEIGHT, _fb[_back]);
    if (err != ESP_OK)
        {
        ESP_LOGE(TAG, "%s: draw_bitmap failed: %s", context ? context : "present", esp_err_to_name(err));
        return;
        }

    _back = 1 - _back;
    _back_needs_refresh_wait = true;
    }


void Display::clear(uint16_t color)
    {
    waitCopyDone();
    invalidateCenteredCache();
    if (!_fb[0] || !_fb[1]) return;
    ensureBackBufferReady();
    fillRGB565Buffer(_fb[0], LCD_WIDTH * LCD_HEIGHT, color);
    fillRGB565Buffer(_fb[1], LCD_WIDTH * LCD_HEIGHT, color);
    esp_lcd_panel_handle_t panel = static_cast<esp_lcd_panel_handle_t>(_panel);
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_lcd_panel_draw_bitmap(panel, 0, 0, LCD_WIDTH, LCD_HEIGHT, _fb[0]));
    drainRefreshSemaphore();
    waitForRefreshDone("clear");
    _back = 1;
    _back_needs_refresh_wait = false;
    }


void Display::presentCenteredRGB565(const uint16_t* src, int src_w, int src_h, uint16_t border_color)
    {
    if (!_started || !src || src_w <= 0 || src_h <= 0) return;
    if (src_w > LCD_WIDTH) src_w = LCD_WIDTH;
    if (src_h > LCD_HEIGHT) src_h = LCD_HEIGHT;

    waitCopyDone();
    ensureBackBufferReady();

    if (!_centered_cache_started ||
        (_centered_border_color != border_color) ||
        (_centered_w != src_w) ||
        (_centered_h != src_h))
        {
        _centered_cache_started = true;
        _centered_border_color = border_color;
        _centered_w = src_w;
        _centered_h = src_h;
        _centered_buffer_valid[0] = false;
        _centered_buffer_valid[1] = false;
        }

    if (!_centered_buffer_valid[_back])
        {
        fillRGB565Buffer(_fb[_back], LCD_WIDTH * LCD_HEIGHT, border_color);
        _centered_buffer_valid[_back] = true;
        }

    const int ox = (LCD_WIDTH - src_w) / 2;
    const int oy = (LCD_HEIGHT - src_h) / 2;
    uint16_t* dst = _fb[_back] + oy * LCD_WIDTH + ox;
    for (int y = 0; y < src_h; y++)
        {
        memcpy(dst + y * LCD_WIDTH, src + y * src_w, (size_t)src_w * sizeof(uint16_t));
        }

    presentBackBuffer("presentCenteredRGB565");
    }


void Display::beginFrame(uint16_t color)
    {
    if (!_started) return;
    fillBackBuffer(color);
    }


void Display::beginFrame()
    {
    if (!_started) return;
    ensureBackBufferReady();
    _centered_buffer_valid[_back] = false;
    }


void Display::copyTileRGB565(const uint16_t* src, int src_w, int src_h, int dst_x, int dst_y)
    {
    if (!_started || !src || src_w <= 0 || src_h <= 0) return;
    if (dst_x >= LCD_WIDTH || dst_y >= LCD_HEIGHT) return;
    _centered_buffer_valid[_back] = false;

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


void Display::startCopyTileRGB565Async(const uint16_t* src, int src_w, int src_h, int dst_x, int dst_y)
    {
    if (!_started || !src || src_w <= 0 || src_h <= 0) return;
    if (dst_x >= LCD_WIDTH || dst_y >= LCD_HEIGHT) return;

    waitCopyDone();
    _centered_buffer_valid[_back] = false;

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

    ppa_client_handle_t ppa_client = static_cast<ppa_client_handle_t>(_ppa_srm_client);
    if (!ppa_client)
        {
        copyTileRGB565(src, src_w, src_h, dst_x, dst_y);
        return;
        }

    SemaphoreHandle_t copy_sem = static_cast<SemaphoreHandle_t>(_copy_sem);
    if (copy_sem) while (xSemaphoreTake(copy_sem, 0) == pdTRUE) {}

    ppa_srm_oper_config_t copy_config = {};
    copy_config.in.buffer = src;
    copy_config.in.pic_w = src_w;
    copy_config.in.pic_h = src_h;
    copy_config.in.block_w = copy_w;
    copy_config.in.block_h = copy_h;
    copy_config.in.block_offset_x = src_x;
    copy_config.in.block_offset_y = src_y;
    copy_config.in.srm_cm = PPA_SRM_COLOR_MODE_RGB565;
    copy_config.out.buffer = _fb[_back];
    copy_config.out.buffer_size = LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t);
    copy_config.out.pic_w = LCD_WIDTH;
    copy_config.out.pic_h = LCD_HEIGHT;
    copy_config.out.block_offset_x = dst_x;
    copy_config.out.block_offset_y = dst_y;
    copy_config.out.srm_cm = PPA_SRM_COLOR_MODE_RGB565;
    copy_config.rotation_angle = PPA_SRM_ROTATION_ANGLE_0;
    copy_config.scale_x = 1.0f;
    copy_config.scale_y = 1.0f;
    copy_config.mode = PPA_TRANS_MODE_NON_BLOCKING;
    copy_config.user_data = _copy_sem;

    esp_err_t err = ppa_do_scale_rotate_mirror(ppa_client, &copy_config);
    if (err == ESP_OK)
        {
        _copy_in_flight = true;
        return;
        }

    ESP_LOGW(TAG, "PPA async copy failed (%s), falling back to CPU copy", esp_err_to_name(err));
    copyTileRGB565(src, src_w, src_h, dst_x, dst_y);
    }


void Display::waitCopyDone()
    {
    if (!_copy_in_flight) return;

    SemaphoreHandle_t copy_sem = static_cast<SemaphoreHandle_t>(_copy_sem);
    if (!copy_sem)
        {
        _copy_in_flight = false;
        return;
        }

    if (xSemaphoreTake(copy_sem, pdMS_TO_TICKS(LCD_REFRESH_WAIT_MS)) != pdTRUE)
        {
        ESP_LOGW(TAG, "PPA async copy wait timed out");
        }
    _copy_in_flight = false;
    }


void Display::presentFrame()
    {
    if (!_started) return;
    presentBackBuffer("presentFrame");
    }

} // namespace tgx_p4
