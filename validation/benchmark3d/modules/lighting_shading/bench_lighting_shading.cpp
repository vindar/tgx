#include "bench_lighting_shading.h"

namespace tgxbench
{

namespace
{

struct SurfaceState
    {
    int grid_n;
    int vertex_count;
    int triangle_count;
    tgx::fVec3* vertices;
    tgx::fVec3* normals;
    tgx::fVec2* texcoords;
    uint16_t* indices;
    tgx::RGB565* texture_pixels;
    tgx::Image<tgx::RGB565> texture;
    };

static SurfaceState g_surface = { 0, 0, 0, nullptr, nullptr, nullptr, nullptr, nullptr, tgx::Image<tgx::RGB565>() };

constexpr int LIGHTING_TEX_W = 64;
constexpr int LIGHTING_TEX_H = 64;

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

int chooseGridN(const BenchContext& ctx)
{
    if (ctx.caps.power_class >= 8) return 16;
    if (ctx.caps.power_class >= 5) return 13;
    if (ctx.caps.power_class >= 4) return 11;
    return 8;
}

float surfaceWave(float fx, float fy)
{
    return 0.26f * benchFastSinDeg(wrapDeg180(fx * 360.0f)) *
                   benchFastCosDeg(wrapDeg180(fy * 300.0f)) +
           0.10f * benchFastSinDeg(wrapDeg180((fx + 0.65f * fy) * 230.0f));
}

tgx::RGB565 lightingTextureColor(int x, int y)
{
    const bool checker = (((x >> 3) ^ (y >> 3)) & 1) != 0;
    const int diag = (x * 3 + y * 5) & 31;
    int r = checker ? 27 : 7;
    int g = checker ? 28 : 45;
    int b = checker ? 6 : 24;
    if (diag < 9)
        {
        r = checker ? 31 : 10;
        g = checker ? 50 : 62;
        b = checker ? 12 : 28;
        }
    return tgx::RGB565(r, g, b);
}

bool ensureSurface(BenchContext& ctx)
{
    const int grid_n = chooseGridN(ctx);
    const bool has_geometry = (g_surface.vertices != nullptr) && (g_surface.grid_n == grid_n);
    const bool has_texture = (g_surface.texture_pixels != nullptr);
    if (has_geometry && has_texture) return true;

    if (!has_texture)
        {
        g_surface.texture_pixels = static_cast<tgx::RGB565*>(
            benchAlloc(BenchMemoryKind::AssetCache, sizeof(tgx::RGB565) * LIGHTING_TEX_W * LIGHTING_TEX_H, 4));
        if (!g_surface.texture_pixels) return false;
        for (int y = 0; y < LIGHTING_TEX_H; ++y)
            {
            for (int x = 0; x < LIGHTING_TEX_W; ++x)
                {
                g_surface.texture_pixels[x + y * LIGHTING_TEX_W] = lightingTextureColor(x, y);
                }
            }
        g_surface.texture.set(g_surface.texture_pixels, LIGHTING_TEX_W, LIGHTING_TEX_H);
        }

    if (has_geometry) return true;

    g_surface.grid_n = grid_n;
    g_surface.vertex_count = (grid_n + 1) * (grid_n + 1);
    g_surface.triangle_count = grid_n * grid_n * 2;
    g_surface.vertices = static_cast<tgx::fVec3*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(tgx::fVec3) * g_surface.vertex_count, 4));
    g_surface.normals = static_cast<tgx::fVec3*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(tgx::fVec3) * g_surface.vertex_count, 4));
    g_surface.texcoords = static_cast<tgx::fVec2*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(tgx::fVec2) * g_surface.vertex_count, 4));
    g_surface.indices = static_cast<uint16_t*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(uint16_t) * g_surface.triangle_count * 3, 2));

    if ((!g_surface.vertices) || (!g_surface.normals) || (!g_surface.texcoords) || (!g_surface.indices))
        {
        return false;
        }

    const float width = 4.4f;
    const float height = 3.0f;
    const float z = -6.0f;
    for (int y = 0; y <= grid_n; ++y)
        {
        const float fy01 = (float)y / (float)grid_n;
        const float fy = fy01 - 0.5f;
        for (int x = 0; x <= grid_n; ++x)
            {
            const float fx01 = (float)x / (float)grid_n;
            const float fx = fx01 - 0.5f;
            const int k = y * (grid_n + 1) + x;
            const float wave = surfaceWave(fx, fy);
            const float dfx = 1.0f / (float)grid_n;
            const float dfy = 1.0f / (float)grid_n;
            const float dzdx = (surfaceWave(fx + dfx, fy) - surfaceWave(fx - dfx, fy)) / (2.0f * dfx * width);
            const float dzdy = (surfaceWave(fx, fy + dfy) - surfaceWave(fx, fy - dfy)) / (2.0f * dfy * height);

            g_surface.vertices[k] = tgx::fVec3{fx * width, fy * height, z + wave + 0.12f * fx};
            g_surface.normals[k] = tgx::fVec3{-dzdx, -dzdy, 1.0f};
            g_surface.normals[k].normalize_fast();
            g_surface.texcoords[k] = tgx::fVec2{fx01 * 3.0f, fy01 * 2.0f};
            }
        }

    int t = 0;
    for (int y = 0; y < grid_n; ++y)
        {
        for (int x = 0; x < grid_n; ++x)
            {
            const uint16_t i00 = (uint16_t)(y * (grid_n + 1) + x);
            const uint16_t i10 = (uint16_t)(y * (grid_n + 1) + x + 1);
            const uint16_t i01 = (uint16_t)((y + 1) * (grid_n + 1) + x);
            const uint16_t i11 = (uint16_t)((y + 1) * (grid_n + 1) + x + 1);
            g_surface.indices[t++] = i00;
            g_surface.indices[t++] = i10;
            g_surface.indices[t++] = i11;
            g_surface.indices[t++] = i00;
            g_surface.indices[t++] = i11;
            g_surface.indices[t++] = i01;
            }
        }

    return true;
}

void applyVisualMotion(BenchContext& ctx, uint32_t frame_index)
{
    if (frame_index == 0)
        {
        ctx.renderer.setModelPosScaleRot(tgx::fVec3{0.0f, 0.0f, 0.0f}, tgx::fVec3{1.0f, 1.0f, 1.0f}, 0.0f);
        return;
        }

    const float angle = visualPhaseDeg(frame_index);
    const float x = 0.18f * benchFastSinDeg(angle);
    const float y = 0.10f * benchFastCosDeg(angle * 0.8f);
    ctx.renderer.setModelPosScaleRot(tgx::fVec3{x, y, 0.0f},
                                     tgx::fVec3{1.0f, 1.0f, 1.0f},
                                     angle * 0.20f,
                                     tgx::fVec3{0.15f, 0.35f, 1.0f});
}

void setBaseCamera(BenchContext& ctx)
{
    ctx.renderer.setPerspective(46.0f, ((float)ctx.width) / ((float)ctx.height), 1.0f, 80.0f);
    ctx.renderer.setLookAt(tgx::fVec3{0.0f, 0.0f, 0.0f}, tgx::fVec3{0.0f, 0.0f, -1.0f}, tgx::fVec3{0.0f, 1.0f, 0.0f});
    ctx.renderer.setCulling(0);
}

void resetLights(BenchContext& ctx)
{
    ctx.renderer.setDirectionalLightCount(1);
    ctx.renderer.setDirectionalLightAmbiant(tgx::RGBf{0.08f, 0.08f, 0.09f});
    ctx.renderer.setDirectionalLight(0,
                                     tgx::fVec3{0.25f, -0.45f, -1.0f},
                                     tgx::RGBf{0.58f, 0.56f, 0.52f},
                                     tgx::RGBf{0.0f, 0.0f, 0.0f});
    ctx.renderer.setDirectionalLight(1,
                                     tgx::fVec3{-0.55f, 0.15f, -1.0f},
                                     tgx::RGBf{0.0f, 0.0f, 0.0f},
                                     tgx::RGBf{0.0f, 0.0f, 0.0f});
    ctx.renderer.setSpotLight(0, tgx::fVec3{-1.0f, 1.0f, -3.8f}, 1.0f,
                              tgx::RGBf{0.0f, 0.0f, 0.0f},
                              tgx::RGBf{0.0f, 0.0f, 0.0f});
    ctx.renderer.setSpotLight(1, tgx::fVec3{1.0f, -0.8f, -4.2f}, 1.0f,
                              tgx::RGBf{0.0f, 0.0f, 0.0f},
                              tgx::RGBf{0.0f, 0.0f, 0.0f});
    ctx.renderer.setSpotLightCount(0);
}

bool setupCommon(BenchContext& ctx, tgx::Shader shaders)
{
    if (!ensureSurface(ctx)) return false;
    ctx.renderer.setShaders(shaders);
    ctx.renderer.setTextureQuality(tgx::SHADER_TEXTURE_NEAREST);
    ctx.renderer.setTextureWrappingMode(tgx::SHADER_TEXTURE_WRAP_POW2);
    setBaseCamera(ctx);
    resetLights(ctx);
    ctx.renderer.setMaterial(tgx::RGBf{0.62f, 0.72f, 0.86f}, 0.20f, 0.90f, 0.0f, 32);
    return true;
}

void drawSurface(BenchContext& ctx, bool gouraud, bool textured)
{
    ctx.renderer.drawTriangles(g_surface.triangle_count,
                               g_surface.indices, g_surface.vertices,
                               gouraud ? g_surface.indices : nullptr,
                               gouraud ? g_surface.normals : nullptr,
                               textured ? g_surface.indices : nullptr,
                               textured ? g_surface.texcoords : nullptr,
                               textured ? &g_surface.texture : nullptr);
}

bool setupDirectionalFlat(BenchContext& ctx)
{
    if (!setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE)) return false;
    ctx.renderer.setDirectionalLightAmbiant(tgx::RGBf{0.13f, 0.13f, 0.14f});
    ctx.renderer.setDirectionalLight(0, tgx::fVec3{0.35f, -0.40f, -1.0f}, tgx::RGBf{0.90f, 0.82f, 0.62f}, tgx::RGBf{0.0f, 0.0f, 0.0f});
    return true;
}

bool setupAmbientOnlyGouraud(BenchContext& ctx)
{
    if (!setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE)) return false;
    ctx.renderer.setDirectionalLightCount(0);
    ctx.renderer.setDirectionalLightAmbiant(tgx::RGBf{0.28f, 0.24f, 0.18f});
    ctx.renderer.setMaterial(tgx::RGBf{0.78f, 0.72f, 0.62f}, 0.70f, 0.0f, 0.0f, 16);
    return true;
}

void renderDirectionalFlat(BenchContext& ctx, uint32_t frame_index)
{
    applyVisualMotion(ctx, frame_index);
    ctx.renderer.setMaterial(tgx::RGBf{0.88f, 0.55f, 0.28f}, 0.22f, 0.88f, 0.0f, 32);
    drawSurface(ctx, false, false);
}

bool setupDirectionalGouraudSpecular(BenchContext& ctx)
{
    if (!setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE)) return false;
    ctx.renderer.setDirectionalLightAmbiant(tgx::RGBf{0.10f, 0.10f, 0.12f});
    ctx.renderer.setDirectionalLight(0, tgx::fVec3{0.25f, -0.55f, -1.0f}, tgx::RGBf{0.55f, 0.62f, 0.82f}, tgx::RGBf{0.70f, 0.68f, 0.62f});
    ctx.renderer.setMaterial(tgx::RGBf{0.50f, 0.64f, 0.95f}, 0.18f, 0.80f, 0.65f, 24);
    return true;
}

void renderGouraudSurface(BenchContext& ctx, uint32_t frame_index)
{
    applyVisualMotion(ctx, frame_index);
    drawSurface(ctx, true, false);
}

bool setupTwoDirectionalGouraud(BenchContext& ctx)
{
    if (!setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE)) return false;
    ctx.renderer.setDirectionalLightCount(2);
    ctx.renderer.setDirectionalLightAmbiant(tgx::RGBf{0.05f, 0.05f, 0.06f});
    ctx.renderer.setDirectionalLight(0, tgx::fVec3{0.42f, -0.52f, -1.0f}, tgx::RGBf{0.70f, 0.50f, 0.36f}, tgx::RGBf{0.20f, 0.18f, 0.15f});
    ctx.renderer.setDirectionalLight(1, tgx::fVec3{-0.62f, 0.16f, -1.0f}, tgx::RGBf{0.26f, 0.44f, 0.86f}, tgx::RGBf{0.0f, 0.0f, 0.0f});
    ctx.renderer.setMaterial(tgx::RGBf{0.68f, 0.72f, 0.78f}, 0.18f, 0.88f, 0.25f, 32);
    return true;
}

bool setupMaterialSettersGouraud(BenchContext& ctx)
{
    if (!setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE)) return false;
    ctx.renderer.setDirectionalLightAmbiant(tgx::RGBf{0.09f, 0.09f, 0.10f});
    ctx.renderer.setDirectionalLight(0, tgx::fVec3{0.32f, -0.58f, -1.0f},
                                     tgx::RGBf{0.55f, 0.56f, 0.62f},
                                     tgx::RGBf{0.75f, 0.70f, 0.55f});
    ctx.renderer.setMaterialColor(tgx::RGBf{0.70f, 0.58f, 0.90f});
    ctx.renderer.setMaterialAmbiantStrength(0.16f);
    ctx.renderer.setMaterialDiffuseStrength(0.84f);
    ctx.renderer.setMaterialSpecularStrength(0.62f);
    ctx.renderer.setMaterialSpecularExponent(18);
    return true;
}

bool setupSpotDeclaredInactive(BenchContext& ctx)
{
    if (!setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE)) return false;
    ctx.renderer.setSpotLight(0, tgx::fVec3{-1.4f, 1.0f, -3.4f}, 4.2f, tgx::RGBf{1.0f, 0.35f, 0.22f});
    ctx.renderer.setSpotLight(1, tgx::fVec3{1.3f, -0.9f, -3.8f}, 3.5f, tgx::RGBf{0.18f, 0.35f, 1.0f});
    ctx.renderer.setSpotLightCount(0);
    ctx.renderer.setMaterial(tgx::RGBf{0.70f, 0.72f, 0.76f}, 0.18f, 0.85f, 0.0f, 32);
    return true;
}

bool setupPointGouraudDiffuse(BenchContext& ctx)
{
    if (!setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE)) return false;
    ctx.renderer.setDirectionalLightAmbiant(tgx::RGBf{0.04f, 0.04f, 0.05f});
    ctx.renderer.setDirectionalLight(0, tgx::fVec3{0.25f, -0.45f, -1.0f}, tgx::RGBf{0.18f, 0.18f, 0.20f}, tgx::RGBf{0.0f, 0.0f, 0.0f});
    ctx.renderer.setSpotLight(0, tgx::fVec3{-1.45f, 0.90f, -3.55f}, 4.0f, tgx::RGBf{1.00f, 0.55f, 0.24f});
    ctx.renderer.setSpotLightCount(1);
    ctx.renderer.setMaterial(tgx::RGBf{0.64f, 0.68f, 0.72f}, 0.15f, 0.92f, 0.0f, 32);
    return true;
}

bool setupPointFlatDiffuse(BenchContext& ctx)
{
    if (!setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE)) return false;
    ctx.renderer.setDirectionalLightAmbiant(tgx::RGBf{0.04f, 0.04f, 0.05f});
    ctx.renderer.setDirectionalLight(0, tgx::fVec3{0.25f, -0.45f, -1.0f}, tgx::RGBf{0.16f, 0.16f, 0.18f}, tgx::RGBf{0.0f, 0.0f, 0.0f});
    ctx.renderer.setSpotLight(0, tgx::fVec3{-1.55f, 1.05f, -3.65f}, 3.8f, tgx::RGBf{1.00f, 0.62f, 0.26f});
    ctx.renderer.setSpotLightCount(1);
    ctx.renderer.setMaterial(tgx::RGBf{0.70f, 0.70f, 0.68f}, 0.14f, 0.92f, 0.0f, 32);
    return true;
}

void renderFlatSurface(BenchContext& ctx, uint32_t frame_index)
{
    applyVisualMotion(ctx, frame_index);
    drawSurface(ctx, false, false);
}

bool setupPointGouraudSpecular(BenchContext& ctx)
{
    if (!setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE)) return false;
    ctx.renderer.setDirectionalLightAmbiant(tgx::RGBf{0.03f, 0.03f, 0.04f});
    ctx.renderer.setDirectionalLight(0, tgx::fVec3{0.25f, -0.45f, -1.0f}, tgx::RGBf{0.12f, 0.12f, 0.14f}, tgx::RGBf{0.0f, 0.0f, 0.0f});
    ctx.renderer.setSpotLight(0, tgx::fVec3{-1.2f, 0.95f, -3.50f}, 4.1f,
                              tgx::RGBf{0.85f, 0.52f, 0.28f},
                              tgx::RGBf{0.80f, 0.78f, 0.70f});
    ctx.renderer.setSpotLightCount(1);
    ctx.renderer.setMaterial(tgx::RGBf{0.55f, 0.64f, 0.78f}, 0.12f, 0.85f, 0.75f, 24);
    return true;
}

bool setupTwoPointColored(BenchContext& ctx)
{
    if (!setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE)) return false;
    ctx.renderer.setDirectionalLightAmbiant(tgx::RGBf{0.03f, 0.03f, 0.04f});
    ctx.renderer.setDirectionalLight(0, tgx::fVec3{0.25f, -0.45f, -1.0f}, tgx::RGBf{0.10f, 0.10f, 0.12f}, tgx::RGBf{0.0f, 0.0f, 0.0f});
    ctx.renderer.setSpotLight(0, tgx::fVec3{-1.75f, 0.85f, -3.65f}, 3.7f, tgx::RGBf{1.00f, 0.22f, 0.16f});
    ctx.renderer.setSpotLight(1, tgx::fVec3{1.65f, -0.85f, -3.75f}, 3.7f, tgx::RGBf{0.16f, 0.32f, 1.00f});
    ctx.renderer.setSpotLightCount(2);
    ctx.renderer.setMaterial(tgx::RGBf{0.68f, 0.68f, 0.70f}, 0.10f, 0.92f, 0.0f, 32);
    return true;
}

bool setupSpotSoftGouraud(BenchContext& ctx)
{
    if (!setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE)) return false;
    ctx.renderer.setDirectionalLightAmbiant(tgx::RGBf{0.04f, 0.04f, 0.05f});
    ctx.renderer.setDirectionalLight(0, tgx::fVec3{0.25f, -0.45f, -1.0f}, tgx::RGBf{0.14f, 0.14f, 0.16f}, tgx::RGBf{0.0f, 0.0f, 0.0f});
    ctx.renderer.setSpotLight(0, tgx::fVec3{-1.2f, 1.2f, -3.10f},
                              tgx::fVec3{0.30f, -0.25f, -1.0f},
                              5.0f, 28.0f, 15.0f,
                              tgx::RGBf{0.90f, 0.72f, 0.34f},
                              tgx::RGBf{0.35f, 0.30f, 0.22f});
    ctx.renderer.setSpotLightCount(1);
    ctx.renderer.setMaterial(tgx::RGBf{0.62f, 0.66f, 0.74f}, 0.12f, 0.88f, 0.45f, 28);
    return true;
}

bool setupSpotHardFlat(BenchContext& ctx)
{
    if (!setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE)) return false;
    ctx.renderer.setDirectionalLightAmbiant(tgx::RGBf{0.03f, 0.03f, 0.04f});
    ctx.renderer.setDirectionalLight(0, tgx::fVec3{0.25f, -0.45f, -1.0f}, tgx::RGBf{0.12f, 0.12f, 0.14f}, tgx::RGBf{0.0f, 0.0f, 0.0f});
    ctx.renderer.setSpotLight(0, tgx::fVec3{1.35f, 1.0f, -3.25f},
                              tgx::fVec3{-0.28f, -0.18f, -1.0f},
                              5.0f, 22.0f,
                              tgx::RGBf{0.45f, 0.75f, 1.00f});
    ctx.renderer.setSpotLightCount(1);
    ctx.renderer.setMaterial(tgx::RGBf{0.66f, 0.68f, 0.70f}, 0.12f, 0.90f, 0.0f, 32);
    return true;
}

bool setupPointTextured(BenchContext& ctx)
{
    if (!setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
                          tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2)) return false;
    ctx.renderer.setDirectionalLightAmbiant(tgx::RGBf{0.05f, 0.05f, 0.05f});
    ctx.renderer.setDirectionalLight(0, tgx::fVec3{0.25f, -0.45f, -1.0f}, tgx::RGBf{0.12f, 0.12f, 0.12f}, tgx::RGBf{0.0f, 0.0f, 0.0f});
    ctx.renderer.setSpotLight(0, tgx::fVec3{-1.25f, 0.95f, -3.60f}, 4.0f, tgx::RGBf{0.95f, 0.70f, 0.38f});
    ctx.renderer.setSpotLightCount(1);
    ctx.renderer.setMaterial(tgx::RGBf{1.0f, 1.0f, 1.0f}, 0.15f, 0.88f, 0.0f, 32);
    return true;
}

void renderTexturedSurface(BenchContext& ctx, uint32_t frame_index)
{
    applyVisualMotion(ctx, frame_index);
    drawSurface(ctx, true, true);
}

bool setupPointMovingUpdate(BenchContext& ctx)
{
    if (!setupPointGouraudDiffuse(ctx)) return false;
    ctx.renderer.setMaterial(tgx::RGBf{0.62f, 0.68f, 0.74f}, 0.14f, 0.88f, 0.0f, 32);
    return true;
}

void renderPointMovingUpdate(BenchContext& ctx, uint32_t frame_index)
{
    const float angle = visualPhaseDeg(frame_index);
    const float x = 1.45f * benchFastSinDeg(angle);
    const float y = 0.95f * benchFastCosDeg(angle * 0.75f);
    ctx.renderer.setSpotLightPosition(0, tgx::fVec3{x, y, -3.55f});
    applyVisualMotion(ctx, frame_index);
    drawSurface(ctx, true, false);
}

bool setupUnlitSpotIgnored(BenchContext& ctx)
{
    if (!setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE)) return false;
    ctx.renderer.setSpotLight(0, tgx::fVec3{-1.3f, 1.0f, -3.6f}, 4.0f, tgx::RGBf{1.0f, 0.25f, 0.15f}, tgx::RGBf{1.0f, 1.0f, 0.8f});
    ctx.renderer.setSpotLightCount(1);
    ctx.renderer.setMaterial(tgx::RGBf{0.24f, 0.68f, 0.95f}, 1.0f, 0.0f, 0.0f, 32);
    return true;
}

void renderUnlitSurface(BenchContext& ctx, uint32_t frame_index)
{
    applyVisualMotion(ctx, frame_index);
    drawSurface(ctx, false, false);
}

const BenchTest LIGHTING_SHADING_TESTS[] = {
    {
        "lighting_shading.directional.flat",
        "directional light flat shading",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupDirectionalFlat,
        renderDirectionalFlat,
        nullptr
    },
    {
        "lighting_shading.directional.ambient_only.gouraud",
        "ambient-only Gouraud shading with zero directional lights",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupAmbientOnlyGouraud,
        renderGouraudSurface,
        nullptr
    },
    {
        "lighting_shading.directional.gouraud.specular",
        "directional light Gouraud shading with specular",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupDirectionalGouraudSpecular,
        renderGouraudSurface,
        nullptr
    },
    {
        "lighting_shading.directional.two.gouraud",
        "two directional lights with Gouraud shading",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupTwoDirectionalGouraud,
        renderGouraudSurface,
        nullptr
    },
    {
        "lighting_shading.material_setters.gouraud.specular",
        "material configured through individual setters",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupMaterialSettersGouraud,
        renderGouraudSurface,
        nullptr
    },
    {
        "lighting_shading.spot.declared_inactive.gouraud",
        "spot-light capacity compiled but active count is zero",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupSpotDeclaredInactive,
        renderGouraudSurface,
        nullptr
    },
    {
        "lighting_shading.point.gouraud.diffuse",
        "one point light diffuse Gouraud shading",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupPointGouraudDiffuse,
        renderGouraudSurface,
        nullptr
    },
    {
        "lighting_shading.point.flat.diffuse",
        "one point light diffuse flat shading",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupPointFlatDiffuse,
        renderFlatSurface,
        nullptr
    },
    {
        "lighting_shading.point.gouraud.specular",
        "one point light with local specular Gouraud shading",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupPointGouraudSpecular,
        renderGouraudSurface,
        nullptr
    },
    {
        "lighting_shading.point.two_colored.gouraud",
        "two colored point lights Gouraud shading",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupTwoPointColored,
        renderGouraudSurface,
        nullptr
    },
    {
        "lighting_shading.spot.soft.gouraud",
        "soft cone spotlight Gouraud shading",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupSpotSoftGouraud,
        renderGouraudSurface,
        nullptr
    },
    {
        "lighting_shading.spot.hard.flat",
        "hard cone spotlight flat shading",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupSpotHardFlat,
        renderFlatSurface,
        nullptr
    },
    {
        "lighting_shading.point.textured.gouraud",
        "point light on perspective textured Gouraud geometry",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupPointTextured,
        renderTexturedSurface,
        nullptr
    },
    {
        "lighting_shading.point.moving_update.gouraud",
        "point-light position updated once per rendered frame",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupPointMovingUpdate,
        renderPointMovingUpdate,
        nullptr
    },
    {
        "lighting_shading.unlit.spot_ignored",
        "active spot light ignored by unlit shading",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupUnlitSpotIgnored,
        renderUnlitSurface,
        nullptr
    }
};

const BenchModule LIGHTING_SHADING_MODULE = {
    "lighting_shading",
    "Lighting and shading pipeline",
    LIGHTING_SHADING_TESTS,
    sizeof(LIGHTING_SHADING_TESTS) / sizeof(LIGHTING_SHADING_TESTS[0])
};

} // namespace

const BenchModule& getLightingShadingBenchModule()
{
    return LIGHTING_SHADING_MODULE;
}

} // namespace tgxbench
