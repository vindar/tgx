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

        void beginFrame(uint16_t color);
        void copyTileRGB565(const uint16_t* src, int src_w, int src_h, int dst_x, int dst_y);
        void presentFrame();

    private:
        void fillBackBuffer(uint16_t color);

        void* _panel = nullptr;
        void* _dsi_bus = nullptr;
        void* _dbi_io = nullptr;
        void* _ldo = nullptr;
        void* _refresh_sem = nullptr;
        uint16_t* _fb[2] = { nullptr, nullptr };
        int _back = 0;
        bool _started = false;
    };

uint64_t millis64();

} // namespace tgx_p4
