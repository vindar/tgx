#include "bench_core_raster.h"

namespace tgxbench
{

namespace
{

struct GridState
    {
    int grid_n;
    int vertex_count;
    int triangle_count;
    tgx::fVec3* vertices;
    tgx::fVec3* normals;
    uint16_t* indices;
    };

static GridState g_grid = { 0, 0, 0, nullptr, nullptr, nullptr };

int chooseGridN(const BenchContext& ctx)
{
    if (ctx.caps.power_class >= 7) return 18;
    if (ctx.caps.power_class >= 4) return 14;
    return 10;
}

float wrapDeg180(float angle)
{
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

float gridWave(float fx, float fy)
{
    return 0.30f * tgx::tgx_fast_sin_deg_clamped(wrapDeg180(fx * 360.0f)) *
                   tgx::tgx_fast_cos_deg_clamped(wrapDeg180(fy * 270.0f)) +
           0.11f * tgx::tgx_fast_sin_deg_clamped(wrapDeg180((fx - fy) * 220.0f));
}

float visualPhaseDeg(uint32_t frame_index)
{
    return wrapDeg180((float)((frame_index % TGX_BENCH_VISUAL_FRAME_COUNT) * 360U) /
                      (float)TGX_BENCH_VISUAL_FRAME_COUNT);
}

bool setupCommon(BenchContext& ctx, tgx::Shader shaders)
{
    ctx.renderer.setShaders(shaders);
    ctx.renderer.setPerspective(45.0f, ((float)ctx.width) / ((float)ctx.height), 1.0f, 80.0f);
    ctx.renderer.setLookAt(tgx::fVec3{0.0f, 0.0f, 0.0f}, tgx::fVec3{0.0f, 0.0f, -1.0f}, tgx::fVec3{0.0f, 1.0f, 0.0f});
    ctx.renderer.setModelPosScaleRot(tgx::fVec3{0.0f, 0.0f, 0.0f}, tgx::fVec3{1.0f, 1.0f, 1.0f}, 0.0f);
    ctx.renderer.setCulling(0);
    ctx.renderer.setMaterial(tgx::RGBf{0.92f, 0.55f, 0.28f}, 0.25f, 0.85f, 0.0f, 32);
    return true;
}

void applyVisualMotion(BenchContext& ctx, uint32_t frame_index, float angle_scale, const tgx::fVec3& axis)
{
    if (frame_index == 0)
        {
        ctx.renderer.setModelPosScaleRot(tgx::fVec3{0.0f, 0.0f, 0.0f}, tgx::fVec3{1.0f, 1.0f, 1.0f}, 0.0f);
        return;
        }
    const float angle = visualPhaseDeg(frame_index) * angle_scale;
    const float x = 0.16f * tgx::tgx_fast_sin_deg_clamped(angle);
    const float y = 0.10f * tgx::tgx_fast_cos_deg_clamped(angle);
    ctx.renderer.setModelPosScaleRot(tgx::fVec3{x, y, 0.0f}, tgx::fVec3{1.0f, 1.0f, 1.0f}, angle, axis);
}

void applyVisualDrift(BenchContext& ctx, uint32_t frame_index, float amplitude)
{
    if (frame_index == 0)
        {
        ctx.renderer.setModelPosScaleRot(tgx::fVec3{0.0f, 0.0f, 0.0f}, tgx::fVec3{1.0f, 1.0f, 1.0f}, 0.0f);
        return;
        }
    const float angle = visualPhaseDeg(frame_index);
    const float x = amplitude * tgx::tgx_fast_sin_deg_clamped(angle);
    const float y = 0.55f * amplitude * tgx::tgx_fast_cos_deg_clamped(angle);
    ctx.renderer.setModelPosScaleRot(tgx::fVec3{x, y, 0.0f}, tgx::fVec3{1.0f, 1.0f, 1.0f}, 0.0f);
}

bool ensureGrid(BenchContext& ctx)
{
    const int grid_n = chooseGridN(ctx);
    if ((g_grid.vertices) && (g_grid.grid_n == grid_n)) return true;

    g_grid.grid_n = grid_n;
    g_grid.vertex_count = (grid_n + 1) * (grid_n + 1);
    g_grid.triangle_count = grid_n * grid_n * 2;
    g_grid.vertices = static_cast<tgx::fVec3*>(benchAlloc(BenchMemoryKind::Bulk, sizeof(tgx::fVec3) * g_grid.vertex_count, 4));
    g_grid.normals = static_cast<tgx::fVec3*>(benchAlloc(BenchMemoryKind::Bulk, sizeof(tgx::fVec3) * g_grid.vertex_count, 4));
    g_grid.indices = static_cast<uint16_t*>(benchAlloc(BenchMemoryKind::Bulk, sizeof(uint16_t) * g_grid.triangle_count * 3, 2));

    if ((!g_grid.vertices) || (!g_grid.normals) || (!g_grid.indices))
        {
        return false;
        }

    const float width = 4.4f;
    const float height = 3.1f;
    const float z = -6.2f;
    for (int y = 0; y <= grid_n; ++y)
        {
        const float fy = ((float)y / (float)grid_n) - 0.5f;
        for (int x = 0; x <= grid_n; ++x)
            {
            const float fx = ((float)x / (float)grid_n) - 0.5f;
            const int k = y * (grid_n + 1) + x;
            const float wave = gridWave(fx, fy);
            const float dfx = 1.0f / (float)grid_n;
            const float dfy = 1.0f / (float)grid_n;
            const float dzdx = (gridWave(fx + dfx, fy) - gridWave(fx - dfx, fy)) / (2.0f * dfx * width);
            const float dzdy = (gridWave(fx, fy + dfy) - gridWave(fx, fy - dfy)) / (2.0f * dfy * height);

            g_grid.vertices[k] = tgx::fVec3{fx * width, fy * height, z + wave + 0.08f * fx - 0.06f * fy};
            g_grid.normals[k] = tgx::fVec3{-dzdx, -dzdy, 1.0f};
            g_grid.normals[k].normalize_fast();
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
            g_grid.indices[t++] = i00;
            g_grid.indices[t++] = i10;
            g_grid.indices[t++] = i11;
            g_grid.indices[t++] = i00;
            g_grid.indices[t++] = i11;
            g_grid.indices[t++] = i01;
            }
        }
    return true;
}

void drawBigTriangles(BenchContext& ctx)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.95f, 0.42f, 0.25f}, 0.25f, 0.75f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{-3.1f, -2.0f, -6.0f},
                              tgx::fVec3{ 3.0f, -1.7f, -6.4f},
                              tgx::fVec3{-2.5f,  2.1f, -6.2f});
    ctx.renderer.setMaterial(tgx::RGBf{0.22f, 0.58f, 0.95f}, 0.25f, 0.75f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{ 3.0f, -1.7f, -5.6f},
                              tgx::fVec3{ 2.7f,  2.0f, -5.8f},
                              tgx::fVec3{-2.5f,  2.1f, -5.5f});
    ctx.renderer.setMaterial(tgx::RGBf{0.30f, 0.82f, 0.45f}, 0.25f, 0.75f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{-0.4f, -2.25f, -4.4f},
                              tgx::fVec3{ 2.5f,  1.6f, -4.8f},
                              tgx::fVec3{-2.7f,  1.4f, -4.6f});
}

bool setupBigFlat(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE);
}

void renderBigFlat(BenchContext& ctx, uint32_t frame_index)
{
    applyVisualMotion(ctx, frame_index, 0.12f, tgx::fVec3{0.0f, 0.0f, 1.0f});
    drawBigTriangles(ctx);
}

bool setupBigGouraud(BenchContext& ctx)
{
    if (!setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE)) return false;
    ctx.renderer.setLight(tgx::fVec3{0.28f, -0.50f, -1.0f},
                          tgx::RGBf{0.16f, 0.16f, 0.16f},
                          tgx::RGBf{0.90f, 0.86f, 0.78f},
                          tgx::RGBf{0.0f, 0.0f, 0.0f});
    return true;
}

void renderBigGouraud(BenchContext& ctx, uint32_t frame_index)
{
    applyVisualMotion(ctx, frame_index, 0.10f, tgx::fVec3{0.25f, 0.45f, 1.0f});

    tgx::fVec3 n1{-0.55f, -0.20f, 1.0f};
    tgx::fVec3 n2{ 0.65f, -0.10f, 1.0f};
    tgx::fVec3 n3{-0.10f,  0.75f, 1.0f};
    n1.normalize_fast();
    n2.normalize_fast();
    n3.normalize_fast();

    ctx.renderer.setMaterial(tgx::RGBf{0.88f, 0.52f, 0.30f}, 0.20f, 0.82f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{-3.1f, -2.0f, -6.0f},
                              tgx::fVec3{ 3.0f, -1.7f, -6.4f},
                              tgx::fVec3{-2.5f,  2.1f, -6.2f},
                              &n1, &n2, &n3);

    ctx.renderer.setMaterial(tgx::RGBf{0.30f, 0.64f, 0.98f}, 0.20f, 0.82f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{ 3.0f, -1.7f, -5.6f},
                              tgx::fVec3{ 2.7f,  2.0f, -5.8f},
                              tgx::fVec3{-2.5f,  2.1f, -5.5f},
                              &n2, &n3, &n1);
}

bool setupBigFlatNoZ(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_NOZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE);
}

void renderBigFlatNoZ(BenchContext& ctx, uint32_t frame_index)
{
    applyVisualMotion(ctx, frame_index, 0.12f, tgx::fVec3{0.0f, 0.0f, 1.0f});
    ctx.renderer.setMaterial(tgx::RGBf{0.30f, 0.82f, 0.45f}, 0.25f, 0.75f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{-0.4f, -2.25f, -4.4f},
                              tgx::fVec3{ 2.5f,  1.6f, -4.8f},
                              tgx::fVec3{-2.7f,  1.4f, -4.6f});
    ctx.renderer.setMaterial(tgx::RGBf{0.95f, 0.42f, 0.25f}, 0.25f, 0.75f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{-3.1f, -2.0f, -6.0f},
                              tgx::fVec3{ 3.0f, -1.7f, -6.4f},
                              tgx::fVec3{-2.5f,  2.1f, -6.2f});
    ctx.renderer.setMaterial(tgx::RGBf{0.22f, 0.58f, 0.95f}, 0.25f, 0.75f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{ 3.0f, -1.7f, -5.6f},
                              tgx::fVec3{ 2.7f,  2.0f, -5.8f},
                              tgx::fVec3{-2.5f,  2.1f, -5.5f});
}

bool setupSmallFlat(BenchContext& ctx)
{
    if (!ensureGrid(ctx)) return false;
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE);
}

void renderSmallFlat(BenchContext& ctx, uint32_t frame_index)
{
    applyVisualDrift(ctx, frame_index, 0.45f);
    ctx.renderer.setMaterial(tgx::RGBf{0.92f, 0.58f, 0.28f}, 0.22f, 0.80f, 0.0f, 32);
    ctx.renderer.drawTriangles(g_grid.triangle_count,
                               g_grid.indices, g_grid.vertices,
                               nullptr, nullptr,
                               nullptr, nullptr,
                               nullptr);
}

bool setupSmallFlatNoZ(BenchContext& ctx)
{
    if (!ensureGrid(ctx)) return false;
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_NOZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE);
}

void renderSmallFlatNoZ(BenchContext& ctx, uint32_t frame_index)
{
    applyVisualDrift(ctx, frame_index, 0.45f);
    ctx.renderer.setMaterial(tgx::RGBf{0.95f, 0.65f, 0.30f}, 0.22f, 0.80f, 0.0f, 32);
    ctx.renderer.drawTriangles(g_grid.triangle_count,
                               g_grid.indices, g_grid.vertices,
                               nullptr, nullptr,
                               nullptr, nullptr,
                               nullptr);
}

bool setupSmallGouraud(BenchContext& ctx)
{
    if (!ensureGrid(ctx)) return false;
    if (!setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE)) return false;
    ctx.renderer.setLight(tgx::fVec3{0.28f, -0.50f, -1.0f},
                          tgx::RGBf{0.16f, 0.16f, 0.16f},
                          tgx::RGBf{0.90f, 0.86f, 0.78f},
                          tgx::RGBf{0.0f, 0.0f, 0.0f});
    return true;
}

void renderSmallGouraud(BenchContext& ctx, uint32_t frame_index)
{
    applyVisualDrift(ctx, frame_index, 0.45f);
    ctx.renderer.setMaterial(tgx::RGBf{0.45f, 0.72f, 0.95f}, 0.22f, 0.86f, 0.0f, 32);
    ctx.renderer.drawTriangles(g_grid.triangle_count,
                               g_grid.indices, g_grid.vertices,
                               g_grid.indices, g_grid.normals,
                               nullptr, nullptr,
                               nullptr);
}

bool setupNearClipFlat(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE);
}

void renderNearClipFlat(BenchContext& ctx, uint32_t frame_index)
{
    applyVisualMotion(ctx, frame_index, 0.08f, tgx::fVec3{0.0f, 1.0f, 0.0f});
    ctx.renderer.setMaterial(tgx::RGBf{0.95f, 0.75f, 0.22f}, 0.18f, 0.84f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{-2.2f, -1.7f, -2.6f},
                              tgx::fVec3{ 2.4f, -1.4f, -0.55f},
                              tgx::fVec3{-1.6f,  2.0f, -2.8f});
    ctx.renderer.setMaterial(tgx::RGBf{0.28f, 0.75f, 0.95f}, 0.18f, 0.84f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{ 2.0f, -1.8f, -2.5f},
                              tgx::fVec3{ 2.3f,  1.8f, -2.6f},
                              tgx::fVec3{-2.4f,  1.7f, -0.70f});
    ctx.renderer.setMaterial(tgx::RGBf{0.95f, 0.36f, 0.26f}, 0.18f, 0.84f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{-0.9f, -1.5f, -0.58f},
                              tgx::fVec3{ 1.2f, -1.2f, -0.62f},
                              tgx::fVec3{ 0.2f,  1.4f, -2.3f});
}

bool setupRasterizerMarginFlat(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE);
}

void renderRasterizerMarginFlat(BenchContext& ctx, uint32_t frame_index)
{
    applyVisualMotion(ctx, frame_index, 0.06f, tgx::fVec3{0.0f, 0.0f, 1.0f});
    ctx.renderer.setMaterial(tgx::RGBf{0.85f, 0.30f, 0.85f}, 0.20f, 0.82f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{-7.8f, -2.1f, -5.7f},
                              tgx::fVec3{ 0.8f, -1.8f, -5.5f},
                              tgx::fVec3{-6.2f,  2.4f, -5.8f});
    ctx.renderer.setMaterial(tgx::RGBf{0.20f, 0.88f, 0.72f}, 0.20f, 0.82f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{ 7.7f, -2.2f, -5.3f},
                              tgx::fVec3{ 6.3f,  2.4f, -5.5f},
                              tgx::fVec3{-0.9f,  1.7f, -5.6f});
    ctx.renderer.setMaterial(tgx::RGBf{0.95f, 0.80f, 0.25f}, 0.20f, 0.82f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{-3.6f, -4.5f, -4.6f},
                              tgx::fVec3{ 3.9f, -4.4f, -4.8f},
                              tgx::fVec3{ 0.1f,  0.7f, -4.7f});
}

bool setupScreenClipFlat(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE);
}

void renderScreenClipOnePlane1InFlat(BenchContext& ctx, uint32_t)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.95f, 0.72f, 0.22f}, 0.18f, 0.84f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{ 0.4f, -1.4f, -5.2f},
                              tgx::fVec3{-34.0f, -8.0f, -5.2f},
                              tgx::fVec3{-36.0f,  8.0f, -5.2f});
    ctx.renderer.setMaterial(tgx::RGBf{0.20f, 0.72f, 0.95f}, 0.18f, 0.84f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{ 0.6f,  1.5f, -5.4f},
                              tgx::fVec3{-35.0f, -5.0f, -5.4f},
                              tgx::fVec3{-33.5f,  9.0f, -5.4f});
}

void renderScreenClipOnePlane2InFlat(BenchContext& ctx, uint32_t)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.92f, 0.30f, 0.68f}, 0.18f, 0.84f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{-1.7f, -1.6f, -5.0f},
                              tgx::fVec3{-0.2f,  1.8f, -5.0f},
                              tgx::fVec3{36.0f,  0.2f, -5.0f});
    ctx.renderer.setMaterial(tgx::RGBf{0.25f, 0.88f, 0.54f}, 0.18f, 0.84f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{-2.0f,  1.2f, -5.5f},
                              tgx::fVec3{ 0.8f, -1.7f, -5.5f},
                              tgx::fVec3{34.0f,  6.0f, -5.5f});
}

void renderScreenClipTwoPlanesFlat(BenchContext& ctx, uint32_t)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.95f, 0.45f, 0.22f}, 0.18f, 0.84f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{-1.7f, -1.6f, -5.3f},
                              tgx::fVec3{34.0f,  5.0f, -5.3f},
                              tgx::fVec3{ 2.0f, 32.0f, -5.3f});
    ctx.renderer.setMaterial(tgx::RGBf{0.25f, 0.60f, 0.95f}, 0.18f, 0.84f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{ 1.6f, -1.5f, -5.6f},
                              tgx::fVec3{-34.0f,  4.0f, -5.6f},
                              tgx::fVec3{-2.0f, 30.0f, -5.6f});
}

void renderScreenClipMultiPlaneFlat(BenchContext& ctx, uint32_t)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.90f, 0.82f, 0.22f}, 0.18f, 0.84f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{-38.0f, -24.0f, -5.6f},
                              tgx::fVec3{ 38.0f, -22.0f, -5.6f},
                              tgx::fVec3{  0.0f,  34.0f, -5.6f});
    ctx.renderer.setMaterial(tgx::RGBf{0.28f, 0.92f, 0.86f}, 0.18f, 0.84f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{-36.0f,  20.0f, -6.0f},
                              tgx::fVec3{ 34.0f,  22.0f, -6.0f},
                              tgx::fVec3{  0.0f, -34.0f, -6.0f});
}

void renderScreenClipDiscardFlat(BenchContext& ctx, uint32_t)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.25f, 0.50f, 0.95f}, 0.18f, 0.84f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{-1.2f, -0.9f, -5.2f},
                              tgx::fVec3{ 1.2f, -0.9f, -5.2f},
                              tgx::fVec3{ 0.0f,  1.2f, -5.2f});
    ctx.renderer.setMaterial(tgx::RGBf{0.95f, 0.25f, 0.18f}, 0.18f, 0.84f, 0.0f, 32);
    for (int i = 0; i < 8; ++i)
        {
        const float y = -7.0f + (float)i * 2.0f;
        ctx.renderer.drawTriangle(tgx::fVec3{34.0f, y, -5.0f},
                                  tgx::fVec3{42.0f, y + 0.7f, -5.0f},
                                  tgx::fVec3{38.0f, y + 1.8f, -5.0f});
        }
}

bool setupFarClipFlat(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE);
}

void renderFarClipFlat(BenchContext& ctx, uint32_t)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.72f, 0.38f, 0.95f}, 0.18f, 0.84f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{-24.0f, -16.0f, -70.0f},
                              tgx::fVec3{ 24.0f, -16.0f, -70.0f},
                              tgx::fVec3{  0.0f,  25.0f, -92.0f});
    ctx.renderer.setMaterial(tgx::RGBf{0.25f, 0.82f, 0.95f}, 0.18f, 0.84f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{-18.0f,  11.0f, -88.0f},
                              tgx::fVec3{ 18.0f,  13.0f, -88.0f},
                              tgx::fVec3{  0.0f, -17.0f, -72.0f});
}

bool setupCullingFlat(BenchContext& ctx)
{
    if (!setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE)) return false;
    ctx.renderer.setCulling(1);
    return true;
}

void renderCullingFlat(BenchContext& ctx, uint32_t frame_index)
{
    applyVisualMotion(ctx, frame_index, 0.08f, tgx::fVec3{0.0f, 1.0f, 0.0f});
    ctx.renderer.setMaterial(tgx::RGBf{0.18f, 0.55f, 0.95f}, 0.20f, 0.82f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{-2.4f, -1.6f, -5.8f},
                              tgx::fVec3{ 2.4f, -1.6f, -5.8f},
                              tgx::fVec3{ 0.0f,  2.0f, -5.8f});
    ctx.renderer.setMaterial(tgx::RGBf{0.95f, 0.25f, 0.18f}, 0.20f, 0.82f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{-1.8f, -1.0f, -5.2f},
                              tgx::fVec3{ 0.0f,  1.4f, -5.2f},
                              tgx::fVec3{ 1.8f, -1.0f, -5.2f});
}

bool setupVertexColorGouraud(BenchContext& ctx)
{
    if (!setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE)) return false;
    ctx.renderer.setLight(tgx::fVec3{0.20f, -0.45f, -1.0f},
                          tgx::RGBf{0.14f, 0.14f, 0.15f},
                          tgx::RGBf{0.86f, 0.82f, 0.74f},
                          tgx::RGBf{0.0f, 0.0f, 0.0f});
    return true;
}

void renderVertexColorGouraud(BenchContext& ctx, uint32_t frame_index)
{
    applyVisualMotion(ctx, frame_index, 0.10f, tgx::fVec3{0.20f, 0.50f, 1.0f});

    tgx::fVec3 n0{-0.20f, -0.10f, 0.97f};
    tgx::fVec3 n1{ 0.25f, -0.20f, 0.95f};
    tgx::fVec3 n2{ 0.05f,  0.32f, 0.94f};
    tgx::fVec3 n3{-0.28f,  0.18f, 0.95f};
    n0.normalize_fast();
    n1.normalize_fast();
    n2.normalize_fast();
    n3.normalize_fast();

    ctx.renderer.drawTriangleWithVertexColor(tgx::fVec3{-2.8f, -1.7f, -5.9f},
                                             tgx::fVec3{ 2.8f, -1.5f, -6.2f},
                                             tgx::fVec3{ 2.2f,  1.8f, -5.7f},
                                             tgx::RGBf{0.95f, 0.20f, 0.15f},
                                             tgx::RGBf{0.20f, 0.88f, 0.30f},
                                             tgx::RGBf{0.22f, 0.45f, 0.98f},
                                             &n0, &n1, &n2);
    ctx.renderer.drawTriangleWithVertexColor(tgx::fVec3{-2.8f, -1.7f, -5.9f},
                                             tgx::fVec3{ 2.2f,  1.8f, -5.7f},
                                             tgx::fVec3{-2.5f,  1.9f, -6.4f},
                                             tgx::RGBf{0.95f, 0.20f, 0.15f},
                                             tgx::RGBf{0.22f, 0.45f, 0.98f},
                                             tgx::RGBf{0.95f, 0.86f, 0.20f},
                                             &n0, &n2, &n3);
}

bool setupOrthoVertexColorQuad(BenchContext& ctx)
{
    if (!setupCommon(ctx, tgx::SHADER_ORTHO | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE)) return false;
    const float ar = ((float)ctx.width) / ((float)ctx.height);
    ctx.renderer.setOrtho(-3.2f * ar, 3.2f * ar, -2.2f, 2.2f, 1.0f, 40.0f);
    ctx.renderer.setLight(tgx::fVec3{0.25f, -0.35f, -1.0f},
                          tgx::RGBf{0.16f, 0.16f, 0.17f},
                          tgx::RGBf{0.85f, 0.82f, 0.74f},
                          tgx::RGBf{0.0f, 0.0f, 0.0f});
    return true;
}

void renderOrthoVertexColorQuad(BenchContext& ctx, uint32_t frame_index)
{
    applyVisualMotion(ctx, frame_index, 0.18f, tgx::fVec3{0.0f, 0.0f, 1.0f});
    const tgx::fVec3 n{0.0f, 0.0f, 1.0f};
    ctx.renderer.drawQuadWithVertexColor(tgx::fVec3{-2.2f, -1.45f, -9.0f},
                                         tgx::fVec3{ 2.1f, -1.25f, -9.0f},
                                         tgx::fVec3{ 2.0f,  1.40f, -9.0f},
                                         tgx::fVec3{-2.4f,  1.25f, -9.0f},
                                         tgx::RGBf{0.95f, 0.26f, 0.18f},
                                         tgx::RGBf{0.22f, 0.88f, 0.34f},
                                         tgx::RGBf{0.24f, 0.45f, 0.98f},
                                         tgx::RGBf{0.95f, 0.82f, 0.22f},
                                         &n, &n, &n, &n);
}

bool setupFrustumFlat(BenchContext& ctx)
{
    if (!setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE)) return false;
    const float ar = ((float)ctx.width) / ((float)ctx.height);
    ctx.renderer.setFrustum(-0.40f * ar, 0.40f * ar, -0.40f, 0.40f, 1.0f, 70.0f);
    return true;
}

void renderFrustumFlat(BenchContext& ctx, uint32_t frame_index)
{
    applyVisualMotion(ctx, frame_index, 0.08f, tgx::fVec3{0.0f, 1.0f, 0.2f});
    drawBigTriangles(ctx);
}

bool setupCustomOrthoProjection(BenchContext& ctx)
{
    if (!setupCommon(ctx, tgx::SHADER_ORTHO | tgx::SHADER_NOZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE)) return false;
    const float ar = ((float)ctx.width) / ((float)ctx.height);
    tgx::fMat4 M;
    M.setOrtho(-3.4f * ar, 3.4f * ar, -2.4f, 2.4f, 1.0f, 50.0f);
    ctx.renderer.setProjectionMatrix(M);
    ctx.renderer.useOrthographicProjection();
    return true;
}

void renderCustomOrthoProjection(BenchContext& ctx, uint32_t frame_index)
{
    applyVisualMotion(ctx, frame_index, 0.16f, tgx::fVec3{0.0f, 0.0f, 1.0f});
    drawBigTriangles(ctx);
}

bool setupDegenerateFlat(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE);
}

void renderDegenerateFlat(BenchContext& ctx, uint32_t frame_index)
{
    applyVisualMotion(ctx, frame_index, 0.05f, tgx::fVec3{0.0f, 0.0f, 1.0f});
    ctx.renderer.setMaterial(tgx::RGBf{0.95f, 0.78f, 0.22f}, 0.20f, 0.82f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{-1.8f, -1.2f, -5.2f},
                              tgx::fVec3{ 1.8f, -1.2f, -5.2f},
                              tgx::fVec3{ 0.0f,  1.4f, -5.2f});
    ctx.renderer.setMaterial(tgx::RGBf{0.95f, 0.20f, 0.18f}, 0.20f, 0.82f, 0.0f, 32);
    ctx.renderer.drawTriangle(tgx::fVec3{-2.0f,  1.6f, -5.1f},
                              tgx::fVec3{-2.0f,  1.6f, -5.1f},
                              tgx::fVec3{ 2.0f,  1.6f, -5.1f});
    ctx.renderer.drawTriangle(tgx::fVec3{-2.2f, -1.6f, -5.0f},
                              tgx::fVec3{ 2.2f, -1.6f, -5.0f},
                              tgx::fVec3{ 2.2f, -1.599f, -5.0f});
}

const BenchTest CORE_RASTER_TESTS[] = {
    {
        "core_raster.big_tri.flat.z",
        "large flat triangles with zbuffer",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupBigFlat,
        renderBigFlat,
        nullptr
    },
    {
        "core_raster.big_tri.gouraud.z",
        "large triangles with Gouraud shading",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupBigGouraud,
        renderBigGouraud,
        nullptr
    },
    {
        "core_raster.big_tri.flat.noz",
        "large flat triangles without zbuffer",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_NOZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupBigFlatNoZ,
        renderBigFlatNoZ,
        nullptr
    },
    {
        "core_raster.small_tri_grid.flat.z",
        "small triangle grid flat shading",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupSmallFlat,
        renderSmallFlat,
        nullptr
    },
    {
        "core_raster.small_tri_grid.flat.noz",
        "small triangle grid flat shading without zbuffer",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_NOZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupSmallFlatNoZ,
        renderSmallFlatNoZ,
        nullptr
    },
    {
        "core_raster.small_tri_grid.gouraud.z",
        "small triangle grid Gouraud shading",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupSmallGouraud,
        renderSmallGouraud,
        nullptr
    },
    {
        "core_raster.near_clip.flat.z",
        "large triangles crossing the near plane",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupNearClipFlat,
        renderNearClipFlat,
        nullptr
    },
    {
        "core_raster.rasterizer_margin.flat.z",
        "large triangles outside the visible viewport but inside renderer clip margin",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupRasterizerMarginFlat,
        renderRasterizerMarginFlat,
        nullptr
    },
    {
        "core_raster.screen_clip.one_plane_1in.flat.z",
        "screen clipping one inside vertex on one plane",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupScreenClipFlat,
        renderScreenClipOnePlane1InFlat,
        nullptr
    },
    {
        "core_raster.screen_clip.one_plane_2in.flat.z",
        "screen clipping two inside vertices on one plane",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupScreenClipFlat,
        renderScreenClipOnePlane2InFlat,
        nullptr
    },
    {
        "core_raster.screen_clip.two_planes.flat.z",
        "screen clipping against two viewport planes",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupScreenClipFlat,
        renderScreenClipTwoPlanesFlat,
        nullptr
    },
    {
        "core_raster.screen_clip.multi_plane.flat.z",
        "screen clipping against several viewport planes",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupScreenClipFlat,
        renderScreenClipMultiPlaneFlat,
        nullptr
    },
    {
        "core_raster.screen_clip.discard.flat.z",
        "fully offscreen triangles discarded by clip test",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupScreenClipFlat,
        renderScreenClipDiscardFlat,
        nullptr
    },
    {
        "core_raster.far_clip.flat.z",
        "large triangles crossing the far plane",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupFarClipFlat,
        renderFarClipFlat,
        nullptr
    },
    {
        "core_raster.culling.flat.z",
        "front and back triangles with culling enabled",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupCullingFlat,
        renderCullingFlat,
        nullptr
    },
    {
        "core_raster.vertex_color.gouraud.z",
        "direct triangles with per-vertex colors and Gouraud normals",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupVertexColorGouraud,
        renderVertexColorGouraud,
        nullptr
    },
    {
        "core_raster.vertex_color_quad.ortho.z",
        "orthographic quad with per-vertex colors",
        tgx::SHADER_ORTHO | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupOrthoVertexColorQuad,
        renderOrthoVertexColorQuad,
        nullptr
    },
    {
        "core_raster.frustum.flat.z",
        "explicit frustum projection with large flat triangles",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupFrustumFlat,
        renderFrustumFlat,
        nullptr
    },
    {
        "core_raster.custom_projection.ortho.noz",
        "setProjectionMatrix plus explicit orthographic mode",
        tgx::SHADER_ORTHO | tgx::SHADER_NOZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupCustomOrthoProjection,
        renderCustomOrthoProjection,
        nullptr
    },
    {
        "core_raster.degenerate.flat.z",
        "normal plus degenerate and nearly degenerate flat triangles",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupDegenerateFlat,
        renderDegenerateFlat,
        nullptr
    }
};

const BenchModule CORE_RASTER_MODULE = {
    "core_raster",
    "Core raster pipeline",
    CORE_RASTER_TESTS,
    sizeof(CORE_RASTER_TESTS) / sizeof(CORE_RASTER_TESTS[0])
};

} // namespace

const BenchModule& getCoreRasterBenchModule()
{
    return CORE_RASTER_MODULE;
}

} // namespace tgxbench
