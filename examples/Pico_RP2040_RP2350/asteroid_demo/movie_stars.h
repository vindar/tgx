#pragma once

#include "movie_world.h"


struct Star
    {
    fVec3 dir;
    uint8_t brightness;
    uint8_t size;
    };


static const int NB_STARS = 820;
Star stars[NB_STARS];


void initStars()
    {
    for (int i = 0; i < NB_STARS; i++)
        {
        const uint32_t h1 = hash32(0x4100u + i * 1193u);
        const uint32_t h2 = hash32(0x9231u + i * 1741u);
        const float z = ((float)(h1 & 65535u) / 32767.5f) - 1.0f;
        const float a = ((float)(h2 & 65535u) / 65535.0f) * 2.0f * MOVIE_PI;
        const float r = sqrtf((1.0f - z * z > 0.0f) ? (1.0f - z * z) : 0.0f);
        stars[i].dir = { r * cosf(a), z, r * sinf(a) };
        stars[i].brightness = (uint8_t)(72 + ((h1 >> 18) & 127u));
        stars[i].size = (uint8_t)(1 + ((h2 >> 27) & 1u));
        }
    }


template<typename ImageT, typename ColorT>
void drawCameraStars(ImageT& im, const CameraState& cam, int lx, int ly, ColorT (*rgb)(int, int, int))
    {
    const CameraBasis b = makeCameraBasis(cam.eye, cam.target);
    const float focal = 0.5f * (float)lx / tanf(cam.fov * 0.5f / MOVIE_RAD_TO_DEG);
    const int cx = lx / 2;
    const int cy = ly / 2;

    for (int i = 0; i < NB_STARS; i++)
        {
        const fVec3 d = stars[i].dir;
        const float z = dotv(d, b.forward);
        if (z <= 0.10f) continue;
        const float x = dotv(d, b.right);
        const float y = dotv(d, b.up);
        const int sx = cx + (int)(focal * x / z);
        const int sy = cy - (int)(focal * y / z);
        if (sx < 0 || sx >= lx || sy < 0 || sy >= ly) continue;

        const int v = stars[i].brightness;
        const ColorT c = rgb(v, v, v + 10);
        im.drawPixel({ sx, sy }, c);
        if (stars[i].size > 1 && sx + 1 < lx) im.drawPixel({ sx + 1, sy }, c, 0.32f);
        }
    }
