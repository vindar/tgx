#pragma once

#include <stdint.h>
#include "esp_err.h"

namespace tgx_p4
{

static constexpr int LCD_WIDTH = 720;
static constexpr int LCD_HEIGHT = 720;

class Display
    {
    public:
        Display() = default;

        esp_err_t begin();

        int width() const { return LCD_WIDTH; }
        int height() const { return LCD_HEIGHT; }

        void clear(uint16_t color);
        void presentCenteredRGB565(const uint16_t* src, int src_w, int src_h, uint16_t border_color);

        void beginFrame();
        void beginFrame(uint16_t color);
        uint16_t* backBuffer() const { return _fb[_back]; }
        int framebufferStride() const { return LCD_WIDTH; }
        void copyTileRGB565(const uint16_t* src, int src_w, int src_h, int dst_x, int dst_y);
        void startCopyTileRGB565Async(const uint16_t* src, int src_w, int src_h, int dst_x, int dst_y);
        void waitCopyDone();
        void presentFrame();

    private:
        void fillBackBuffer(uint16_t color);
        void invalidateCenteredCache();
        void drainRefreshSemaphore();
        bool waitForRefreshDone(const char* context);
        void ensureBackBufferReady();
        void presentBackBuffer(const char* context);

        void* _panel = nullptr;
        void* _dsi_bus = nullptr;
        void* _dbi_io = nullptr;
        void* _ldo = nullptr;
        void* _refresh_sem = nullptr;
        void* _copy_sem = nullptr;
        void* _ppa_srm_client = nullptr;
        uint16_t* _fb[2] = { nullptr, nullptr };
        int _back = 0;
        int _centered_w = 0;
        int _centered_h = 0;
        uint16_t _centered_border_color = 0;
        bool _centered_buffer_valid[2] = { false, false };
        bool _centered_cache_started = false;
        bool _back_needs_refresh_wait = false;
        bool _copy_in_flight = false;
        bool _started = false;
    };

uint64_t millis64();

} // namespace tgx_p4
