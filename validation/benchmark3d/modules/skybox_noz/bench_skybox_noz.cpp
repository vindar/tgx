#include "bench_skybox_noz.h"

namespace tgxbench
{

namespace
{

constexpr int SKY_TEX_FULL = 64;
constexpr int SKY_TEX_SMALL = 48;

struct SkyboxState
    {
    int tex_w;
    int tex_h;
    tgx::RGB565* face_pixels[6];
    tgx::Image<tgx::RGB565> faces[6];
    tgx::RGB565* overlay_pixels;
    tgx::Image<tgx::RGB565> overlay_texture;
    bool ready;
    };

static SkyboxState g_sky = {};

float wrapDeg180(float angle)
{
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

float visualPhaseDeg(uint32_t frame_index)
{
    return wrapDeg180((float)((frame_index % TGX_BENCH_VISUAL_FRAME_COUNT) * 360U) /
                      (float)TGX_BENCH_VISUAL_FRAME_COUNT);
}

uint8_t clampByte(int value)
{
    if (value < 0) return 0;
    if (value > 255) return 255;
    return (uint8_t)value;
}

tgx::RGB565 rgb888(int r, int g, int b)
{
    return tgx::RGB565((uint8_t)(clampByte(r) >> 3),
                       (uint8_t)(clampByte(g) >> 2),
                       (uint8_t)(clampByte(b) >> 3));
}

int skyTextureSizeForCaps(const BenchContext& ctx)
{
    return (ctx.caps.power_class >= 8) ? SKY_TEX_FULL : SKY_TEX_SMALL;
}

tgx::RGB565 skyColor(int face, int x, int y, int tex_h)
{
    const int horizon = 120 + (face == 2 ? 32 : 0) - (face == 3 ? 45 : 0);
    const int vertical = (y * 255) / (tex_h - 1);
    const int wave = (int)(34.0f * benchFastSinDeg(wrapDeg180((float)(x * 7 + y * 3 + face * 41))));
    const bool star = (((x * 53 + y * 97 + face * 29) & 255) > 250) && (face != 3);
    const bool cloud = (((x * 5 + y * 2 + face * 13) & 31) < 8) && (y > tex_h / 3) && (face != 2);
    int r = 12 + face * 9 + vertical / 9;
    int g = 28 + face * 5 + vertical / 5;
    int b = horizon + wave - vertical / 8;

    if (face == 0) { r += 18; g += 8; }
    if (face == 1) { b += 24; }
    if (face == 2) { r += 18; g += 20; b += 18; }
    if (face == 3) { r = 28 + vertical / 5; g = 24 + vertical / 6; b = 30 + vertical / 7; }
    if (face == 4) { g += 16; }
    if (face == 5) { r += 12; b += 10; }
    if (cloud) { r += 38; g += 38; b += 34; }
    if (star) { r = 255; g = 245; b = 210; }
    return rgb888(r, g, b);
}

tgx::RGB565 overlayColor(int x, int y, int tex_w, int tex_h)
{
    const bool checker = (((x >> 3) ^ (y >> 3)) & 1) != 0;
    const int dx = x - tex_w / 2;
    const int dy = y - tex_h / 2;
    const bool ring = (((dx * dx + dy * dy) >> 6) & 1) != 0;
    if (ring) return checker ? tgx::RGB565(31, 40, 10) : tgx::RGB565(6, 44, 29);
    return checker ? tgx::RGB565(30, 20, 7) : tgx::RGB565(5, 24, 28);
}

bool ensureTextures(BenchContext& ctx)
{
    const int tex_size = skyTextureSizeForCaps(ctx);
    if (g_sky.ready && (g_sky.tex_w == tex_size) && (g_sky.tex_h == tex_size)) return true;

    g_sky.tex_w = tex_size;
    g_sky.tex_h = tex_size;

    for (int face = 0; face < 6; ++face)
        {
        g_sky.face_pixels[face] = static_cast<tgx::RGB565*>(
            benchAlloc(BenchMemoryKind::AssetCache, sizeof(tgx::RGB565) * g_sky.tex_w * g_sky.tex_h, 4));
        if (!g_sky.face_pixels[face]) return false;
        for (int y = 0; y < g_sky.tex_h; ++y)
            {
            for (int x = 0; x < g_sky.tex_w; ++x)
                {
                g_sky.face_pixels[face][x + y * g_sky.tex_w] = skyColor(face, x, y, g_sky.tex_h);
                }
            }
        g_sky.faces[face].set(g_sky.face_pixels[face], g_sky.tex_w, g_sky.tex_h);
        }

    g_sky.overlay_pixels = static_cast<tgx::RGB565*>(
        benchAlloc(BenchMemoryKind::AssetCache, sizeof(tgx::RGB565) * g_sky.tex_w * g_sky.tex_h, 4));
    if (!g_sky.overlay_pixels) return false;
    for (int y = 0; y < g_sky.tex_h; ++y)
        {
        for (int x = 0; x < g_sky.tex_w; ++x)
            {
            g_sky.overlay_pixels[x + y * g_sky.tex_w] = overlayColor(x, y, g_sky.tex_w, g_sky.tex_h);
            }
        }
    g_sky.overlay_texture.set(g_sky.overlay_pixels, g_sky.tex_w, g_sky.tex_h);
    g_sky.ready = true;
    return true;
}

void setCamera(BenchContext& ctx, uint32_t frame_index, float yaw_offset, float height)
{
    const float phase = frame_index ? visualPhaseDeg(frame_index) : 0.0f;
    const float yaw = yaw_offset + phase * 0.65f;
    const float pitch = 0.10f * benchFastSinDeg(phase * 0.75f);
    tgx::fVec3 dir{
        benchFastSinDeg(yaw),
        pitch,
        -benchFastCosDeg(yaw)
    };
    dir.normalize_fast();
    const tgx::fVec3 eye{0.0f, height, 0.0f};
    ctx.renderer.setLookAt(eye, eye + dir, tgx::fVec3{0.0f, 1.0f, 0.0f});
}

bool setupCommon(BenchContext& ctx, tgx::Shader shaders)
{
    if (!ensureTextures(ctx)) return false;
    ctx.renderer.setShaders(shaders);
    ctx.renderer.setPerspective(54.0f, ((float)ctx.width) / ((float)ctx.height), 1.0f, 120.0f);
    ctx.renderer.setLookAt(tgx::fVec3{0.0f, 0.0f, 0.0f}, tgx::fVec3{0.0f, 0.0f, -1.0f}, tgx::fVec3{0.0f, 1.0f, 0.0f});
    ctx.renderer.setCulling(0);
    ctx.renderer.setLight(tgx::fVec3{0.35f, -0.45f, -1.0f},
                          tgx::RGBf{0.12f, 0.12f, 0.13f},
                          tgx::RGBf{0.86f, 0.82f, 0.72f},
                          tgx::RGBf{0.22f, 0.22f, 0.20f});
    ctx.renderer.setMaterial(tgx::RGBf{0.78f, 0.70f, 0.52f}, 0.18f, 0.82f, 0.22f, 24);
    return true;
}

void drawSky(BenchContext& ctx, float rot_y, float ref_height, float radius,
             tgx::Shader texture_quality, tgx::Shader texture_mode)
{
    ctx.renderer.drawSkyBox(&g_sky.faces[0], &g_sky.faces[1], &g_sky.faces[2],
                            &g_sky.faces[3], &g_sky.faces[4], &g_sky.faces[5],
                            rot_y, ref_height, radius, texture_quality, texture_mode);
}

bool setupNearestWrap(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_NOZBUFFER |
                            tgx::SHADER_UNLIT | tgx::SHADER_TEXTURE |
                            tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
}

void renderNearestWrap(BenchContext& ctx, uint32_t frame_index)
{
    setCamera(ctx, frame_index, 0.0f, 0.0f);
    drawSky(ctx, frame_index ? visualPhaseDeg(frame_index) * 0.25f : 0.0f,
            0.0f, 900.0f, tgx::SHADER_TEXTURE_NEAREST, tgx::SHADER_TEXTURE_WRAP_POW2);
}

bool setupBilinearClamp(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_NOZBUFFER |
                            tgx::SHADER_UNLIT | tgx::SHADER_TEXTURE |
                            tgx::SHADER_TEXTURE_BILINEAR | tgx::SHADER_TEXTURE_CLAMP);
}

void renderBilinearClamp(BenchContext& ctx, uint32_t frame_index)
{
    setCamera(ctx, frame_index, 35.0f, 0.0f);
    drawSky(ctx, -18.0f, 0.0f, 900.0f, tgx::SHADER_TEXTURE_BILINEAR, tgx::SHADER_TEXTURE_CLAMP);
}

void renderPartialFaces(BenchContext& ctx, uint32_t frame_index)
{
    setCamera(ctx, frame_index, -25.0f, 0.0f);
    ctx.renderer.drawSkyBox(&g_sky.faces[0], nullptr, &g_sky.faces[2],
                            nullptr, &g_sky.faces[4], &g_sky.faces[5],
                            12.0f, 0.0f, 850.0f,
                            tgx::SHADER_TEXTURE_NEAREST, tgx::SHADER_TEXTURE_WRAP_POW2);
}

void renderReferenceHeight(BenchContext& ctx, uint32_t frame_index)
{
    const float phase = frame_index ? visualPhaseDeg(frame_index) : 0.0f;
    const float height = 1.1f + 0.6f * benchFastSinDeg(phase);
    setCamera(ctx, frame_index, 10.0f, height);
    drawSky(ctx, 0.0f, -0.35f, 650.0f, tgx::SHADER_TEXTURE_NEAREST, tgx::SHADER_TEXTURE_WRAP_POW2);
}

bool setupSkyObject(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER |
                            tgx::SHADER_GOURAUD | tgx::SHADER_TEXTURE |
                            tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
}

void renderSkyObject(BenchContext& ctx, uint32_t frame_index)
{
    const float phase = frame_index ? visualPhaseDeg(frame_index) : 0.0f;
    setCamera(ctx, frame_index, 20.0f, 0.0f);
    drawSky(ctx, 8.0f, 0.0f, 900.0f, tgx::SHADER_TEXTURE_NEAREST, tgx::SHADER_TEXTURE_WRAP_POW2);
    ctx.renderer.setTextureQuality(tgx::SHADER_TEXTURE_NEAREST);
    ctx.renderer.setTextureWrappingMode(tgx::SHADER_TEXTURE_WRAP_POW2);
    ctx.renderer.setModelPosScaleRot(tgx::fVec3{0.0f, -0.1f, -5.2f}, tgx::fVec3{1.0f, 1.0f, 1.0f},
                                     22.0f + phase * 0.30f, tgx::fVec3{0.20f, 0.92f, 0.28f});
    ctx.renderer.drawCube(&g_sky.overlay_texture, &g_sky.overlay_texture, &g_sky.overlay_texture,
                          &g_sky.overlay_texture, &g_sky.overlay_texture, &g_sky.overlay_texture);
}

void renderSkyNoZOverlay(BenchContext& ctx, uint32_t frame_index)
{
    setCamera(ctx, frame_index, -45.0f, 0.0f);
    drawSky(ctx, 0.0f, 0.0f, 900.0f, tgx::SHADER_TEXTURE_NEAREST, tgx::SHADER_TEXTURE_WRAP_POW2);
    ctx.renderer.setShaders(tgx::SHADER_PERSPECTIVE | tgx::SHADER_NOZBUFFER |
                            tgx::SHADER_UNLIT | tgx::SHADER_TEXTURE |
                            tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
    ctx.renderer.setTextureQuality(tgx::SHADER_TEXTURE_NEAREST);
    ctx.renderer.setTextureWrappingMode(tgx::SHADER_TEXTURE_WRAP_POW2);
    ctx.renderer.setModelPosScaleRot(tgx::fVec3{0.0f, 0.0f, -4.0f}, tgx::fVec3{1.25f, 0.82f, 1.0f}, 0.0f);
    const tgx::fVec3 n{0.0f, 0.0f, 1.0f};
    const tgx::fVec2 t0{0.0f, 0.0f};
    const tgx::fVec2 t1{1.0f, 0.0f};
    const tgx::fVec2 t2{1.0f, 1.0f};
    const tgx::fVec2 t3{0.0f, 1.0f};
    ctx.renderer.drawQuad(tgx::fVec3{-1.0f, -0.65f, 0.0f},
                          tgx::fVec3{ 1.0f, -0.65f, 0.0f},
                          tgx::fVec3{ 1.0f,  0.65f, 0.0f},
                          tgx::fVec3{-1.0f,  0.65f, 0.0f},
                          &n, &n, &n, &n, &t0, &t1, &t2, &t3, &g_sky.overlay_texture);
}

const BenchTest SKYBOX_NOZ_TESTS[] = {
    {
        "skybox_noz.nearest.wrap",
        "skybox only nearest wrap",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_NOZBUFFER | tgx::SHADER_UNLIT |
            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupNearestWrap,
        renderNearestWrap,
        nullptr
    },
    {
        "skybox_noz.bilinear.clamp",
        "skybox only bilinear clamp",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_NOZBUFFER | tgx::SHADER_UNLIT |
            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_BILINEAR | tgx::SHADER_TEXTURE_CLAMP,
        0, 0, 0,
        setupBilinearClamp,
        renderBilinearClamp,
        nullptr
    },
    {
        "skybox_noz.partial_faces",
        "skybox with skipped null texture faces",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_NOZBUFFER | tgx::SHADER_UNLIT |
            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupNearestWrap,
        renderPartialFaces,
        nullptr
    },
    {
        "skybox_noz.reference_height",
        "skybox camera height and reference height path",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_NOZBUFFER | tgx::SHADER_UNLIT |
            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupNearestWrap,
        renderReferenceHeight,
        nullptr
    },
    {
        "skybox_noz.object_front.z",
        "skybox background then zbuffered textured cube",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupSkyObject,
        renderSkyObject,
        nullptr
    },
    {
        "skybox_noz.overlay.noz",
        "skybox background then no-zbuffer unlit textured overlay",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_NOZBUFFER | tgx::SHADER_UNLIT |
            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupNearestWrap,
        renderSkyNoZOverlay,
        nullptr
    }
};

const BenchModule SKYBOX_NOZ_MODULE = {
    "skybox_noz",
    "Skybox and no-zbuffer backgrounds",
    SKYBOX_NOZ_TESTS,
    sizeof(SKYBOX_NOZ_TESTS) / sizeof(SKYBOX_NOZ_TESTS[0])
};

} // namespace

const BenchModule& getSkyboxNoZBenchModule()
{
    return SKYBOX_NOZ_MODULE;
}

} // namespace tgxbench
