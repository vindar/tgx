#include "bench_texture_raster.h"

namespace tgxbench
{

namespace
{

struct TextureState
    {
    tgx::RGB565* wrap_pixels;
    tgx::RGB565* clamp_pixels;
    tgx::Image<tgx::RGB565> wrap_texture;
    tgx::Image<tgx::RGB565> clamp_texture;
    };

struct TextureGridState
    {
    int grid_n;
    int vertex_count;
    int triangle_count;
    tgx::fVec3* vertices;
    tgx::fVec3* normals;
    tgx::fVec2* texcoords;
    uint16_t* indices;
    };

struct TextureStripState
    {
    int cols;
    int rows;
    int vertex_count;
    int index_count;
    tgx::fVec3* vertices;
    tgx::fVec3* normals;
    tgx::fVec2* texcoords;
    uint16_t* indices;
    };

static TextureState g_texture = { nullptr, nullptr, tgx::Image<tgx::RGB565>(), tgx::Image<tgx::RGB565>() };
static TextureGridState g_grid = { 0, 0, 0, nullptr, nullptr, nullptr, nullptr };
static TextureStripState g_strip = { 0, 0, 0, 0, nullptr, nullptr, nullptr, nullptr };

constexpr int WRAP_TEX_W = 64;
constexpr int WRAP_TEX_H = 64;
constexpr int CLAMP_TEX_W = 50;
constexpr int CLAMP_TEX_H = 37;

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

tgx::RGB565 textureColor(int x, int y, int w, int h, bool clamp_texture)
{
    const int tile = clamp_texture ? 7 : 8;
    const bool checker = (((x / tile) ^ (y / tile)) & 1) != 0;
    const int stripe = ((x * 5 + y * 3) & 31);
    const int edge = ((x < 3) || (y < 3) || (x >= w - 3) || (y >= h - 3)) ? 1 : 0;

    int r = checker ? 27 : 5;
    int g = checker ? 18 : 50;
    int b = checker ? 7 : 25;

    if (stripe < 8)
        {
        r = checker ? 31 : 9;
        g = checker ? 45 : 60;
        b = checker ? 10 : 28;
        }
    if (edge)
        {
        r = clamp_texture ? 31 : 4;
        g = clamp_texture ? 8 : 62;
        b = clamp_texture ? 4 : 30;
        }
    return tgx::RGB565(r, g, b);
}

bool ensureTextures(BenchContext& ctx)
{
    if (g_texture.wrap_pixels && g_texture.clamp_pixels) return true;

    g_texture.wrap_pixels = static_cast<tgx::RGB565*>(
        benchAlloc(BenchMemoryKind::AssetCache, sizeof(tgx::RGB565) * WRAP_TEX_W * WRAP_TEX_H, 4));
    g_texture.clamp_pixels = static_cast<tgx::RGB565*>(
        benchAlloc(BenchMemoryKind::AssetCache, sizeof(tgx::RGB565) * CLAMP_TEX_W * CLAMP_TEX_H, 4));
    if ((!g_texture.wrap_pixels) || (!g_texture.clamp_pixels))
        {
        return false;
        }

    for (int y = 0; y < WRAP_TEX_H; ++y)
        {
        for (int x = 0; x < WRAP_TEX_W; ++x)
            {
            g_texture.wrap_pixels[x + y * WRAP_TEX_W] = textureColor(x, y, WRAP_TEX_W, WRAP_TEX_H, false);
            }
        }
    for (int y = 0; y < CLAMP_TEX_H; ++y)
        {
        for (int x = 0; x < CLAMP_TEX_W; ++x)
            {
            g_texture.clamp_pixels[x + y * CLAMP_TEX_W] = textureColor(x, y, CLAMP_TEX_W, CLAMP_TEX_H, true);
            }
        }

    g_texture.wrap_texture.set(g_texture.wrap_pixels, WRAP_TEX_W, WRAP_TEX_H);
    g_texture.clamp_texture.set(g_texture.clamp_pixels, CLAMP_TEX_W, CLAMP_TEX_H);
    (void)ctx;
    return true;
}

int chooseGridN(const BenchContext& ctx)
{
    if (ctx.caps.power_class >= 7) return 16;
    if (ctx.caps.power_class >= 4) return 12;
    return 8;
}

float gridWave(float fx, float fy)
{
    return 0.22f * tgx::tgx_fast_sin_deg_clamped(wrapDeg180(fx * 360.0f)) *
                   tgx::tgx_fast_cos_deg_clamped(wrapDeg180(fy * 270.0f)) +
           0.08f * tgx::tgx_fast_sin_deg_clamped(wrapDeg180((fx + fy) * 250.0f));
}

bool ensureGrid(BenchContext& ctx)
{
    const int grid_n = chooseGridN(ctx);
    if ((g_grid.vertices) && (g_grid.grid_n == grid_n)) return true;

    g_grid.grid_n = grid_n;
    g_grid.vertex_count = (grid_n + 1) * (grid_n + 1);
    g_grid.triangle_count = grid_n * grid_n * 2;
    g_grid.vertices = static_cast<tgx::fVec3*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(tgx::fVec3) * g_grid.vertex_count, 4));
    g_grid.normals = static_cast<tgx::fVec3*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(tgx::fVec3) * g_grid.vertex_count, 4));
    g_grid.texcoords = static_cast<tgx::fVec2*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(tgx::fVec2) * g_grid.vertex_count, 4));
    g_grid.indices = static_cast<uint16_t*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(uint16_t) * g_grid.triangle_count * 3, 2));
    if ((!g_grid.vertices) || (!g_grid.normals) || (!g_grid.texcoords) || (!g_grid.indices))
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
            const float wave = gridWave(fx, fy);
            const float dfx = 1.0f / (float)grid_n;
            const float dfy = 1.0f / (float)grid_n;
            const float dzdx = (gridWave(fx + dfx, fy) - gridWave(fx - dfx, fy)) / (2.0f * dfx * width);
            const float dzdy = (gridWave(fx, fy + dfy) - gridWave(fx, fy - dfy)) / (2.0f * dfy * height);

            g_grid.vertices[k] = tgx::fVec3{fx * width, fy * height, z + wave};
            g_grid.normals[k] = tgx::fVec3{-dzdx, -dzdy, 1.0f};
            g_grid.normals[k].normalize_fast();
            g_grid.texcoords[k] = tgx::fVec2{fx01 * 4.0f, fy01 * 3.0f};
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

bool ensureStrip(BenchContext& ctx)
{
    const int cols = (ctx.caps.power_class >= 7) ? 20 : ((ctx.caps.power_class >= 4) ? 16 : 10);
    const int rows = (ctx.caps.power_class >= 7) ? 8 : ((ctx.caps.power_class >= 4) ? 6 : 4);
    if ((g_strip.vertices) && (g_strip.cols == cols) && (g_strip.rows == rows)) return true;

    g_strip.cols = cols;
    g_strip.rows = rows;
    g_strip.vertex_count = (cols + 1) * (rows + 1);
    g_strip.index_count = rows * (2 * (cols + 1)) + (rows - 1) * 2;
    g_strip.vertices = static_cast<tgx::fVec3*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(tgx::fVec3) * g_strip.vertex_count, 4));
    g_strip.normals = static_cast<tgx::fVec3*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(tgx::fVec3) * g_strip.vertex_count, 4));
    g_strip.texcoords = static_cast<tgx::fVec2*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(tgx::fVec2) * g_strip.vertex_count, 4));
    g_strip.indices = static_cast<uint16_t*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(uint16_t) * g_strip.index_count, 2));
    if ((!g_strip.vertices) || (!g_strip.normals) || (!g_strip.texcoords) || (!g_strip.indices))
        {
        return false;
        }

    const float width = 4.8f;
    const float height = 3.1f;
    for (int y = 0; y <= rows; ++y)
        {
        const float fy01 = (float)y / (float)rows;
        const float fy = fy01 - 0.5f;
        for (int x = 0; x <= cols; ++x)
            {
            const float fx01 = (float)x / (float)cols;
            const float fx = fx01 - 0.5f;
            const int k = y * (cols + 1) + x;
            const float z = -6.0f + 0.20f * tgx::tgx_fast_sin_deg_clamped(wrapDeg180((fx - fy) * 270.0f));
            g_strip.vertices[k] = tgx::fVec3{fx * width, fy * height, z};
            g_strip.normals[k] = tgx::fVec3{0.0f, 0.0f, 1.0f};
            g_strip.texcoords[k] = tgx::fVec2{fx01 * 5.0f, fy01 * 3.0f};
            }
        }

    int k = 0;
    for (int y = 0; y < rows; ++y)
        {
        if (y > 0)
            {
            g_strip.indices[k++] = (uint16_t)(y * (cols + 1));
            g_strip.indices[k++] = (uint16_t)(y * (cols + 1));
            }
        for (int x = 0; x <= cols; ++x)
            {
            g_strip.indices[k++] = (uint16_t)(y * (cols + 1) + x);
            g_strip.indices[k++] = (uint16_t)((y + 1) * (cols + 1) + x);
            }
        }
    return true;
}

void applyTextureMode(BenchContext& ctx, tgx::Shader shaders)
{
    ctx.renderer.setShaders(shaders);
    if ((int)(shaders & tgx::SHADER_TEXTURE_BILINEAR))
        {
        ctx.renderer.setTextureQuality(tgx::SHADER_TEXTURE_BILINEAR);
        }
    else
        {
        ctx.renderer.setTextureQuality(tgx::SHADER_TEXTURE_NEAREST);
        }

    if ((int)(shaders & tgx::SHADER_TEXTURE_CLAMP))
        {
        ctx.renderer.setTextureWrappingMode(tgx::SHADER_TEXTURE_CLAMP);
        }
    else
        {
        ctx.renderer.setTextureWrappingMode(tgx::SHADER_TEXTURE_WRAP_POW2);
        }
}

bool setupCommon(BenchContext& ctx, tgx::Shader shaders)
{
    if (!ensureTextures(ctx)) return false;
    applyTextureMode(ctx, shaders);
    ctx.renderer.setPerspective(45.0f, ((float)ctx.width) / ((float)ctx.height), 1.0f, 80.0f);
    ctx.renderer.setLookAt(tgx::fVec3{0.0f, 0.0f, 0.0f}, tgx::fVec3{0.0f, 0.0f, -1.0f}, tgx::fVec3{0.0f, 1.0f, 0.0f});
    ctx.renderer.setModelPosScaleRot(tgx::fVec3{0.0f, 0.0f, 0.0f}, tgx::fVec3{1.0f, 1.0f, 1.0f}, 0.0f);
    ctx.renderer.setCulling(0);
    ctx.renderer.setLight(tgx::fVec3{0.30f, -0.42f, -1.0f},
                          tgx::RGBf{0.20f, 0.20f, 0.20f},
                          tgx::RGBf{0.95f, 0.91f, 0.82f},
                          tgx::RGBf{0.0f, 0.0f, 0.0f});
    ctx.renderer.setMaterial(tgx::RGBf{0.95f, 0.95f, 0.95f}, 0.18f, 0.88f, 0.0f, 32);
    return true;
}

void applyVisualMotion(BenchContext& ctx, uint32_t frame_index, float angle_scale)
{
    if (frame_index == 0)
        {
        ctx.renderer.setModelPosScaleRot(tgx::fVec3{0.0f, 0.0f, 0.0f}, tgx::fVec3{1.0f, 1.0f, 1.0f}, 0.0f);
        return;
        }
    const float angle = visualPhaseDeg(frame_index);
    const float x = 0.20f * tgx::tgx_fast_sin_deg_clamped(angle);
    const float y = 0.12f * tgx::tgx_fast_cos_deg_clamped(angle * 0.75f);
    ctx.renderer.setModelPosScaleRot(tgx::fVec3{x, y, 0.0f},
                                     tgx::fVec3{1.0f, 1.0f, 1.0f},
                                     angle * angle_scale,
                                     tgx::fVec3{0.25f, 0.55f, 1.0f});
}

void drawTexturedBigTriangles(BenchContext& ctx, const tgx::Image<tgx::RGB565>* texture)
{
    const tgx::fVec3 n0{0.0f, 0.0f, 1.0f};
    const tgx::fVec3 n1{-0.20f, 0.12f, 0.97f};
    const tgx::fVec3 n2{0.18f, -0.25f, 0.95f};

    const tgx::fVec2 t00{-0.35f, 0.08f};
    const tgx::fVec2 t10{ 2.65f, 0.12f};
    const tgx::fVec2 t01{ 0.15f, 2.25f};
    const tgx::fVec2 t11{ 2.35f, 2.45f};

    ctx.renderer.drawTriangle(tgx::fVec3{-3.0f, -1.9f, -5.7f},
                              tgx::fVec3{ 3.0f, -1.6f, -6.4f},
                              tgx::fVec3{-2.4f,  2.0f, -6.1f},
                              &n0, &n1, &n2,
                              &t00, &t10, &t01,
                              texture);
    ctx.renderer.drawTriangle(tgx::fVec3{ 3.0f, -1.6f, -5.4f},
                              tgx::fVec3{ 2.5f,  2.1f, -6.0f},
                              tgx::fVec3{-2.4f,  2.0f, -5.6f},
                              &n1, &n2, &n0,
                              &t10, &t11, &t01,
                              texture);
}

bool setupBigPerspectiveNearestWrap(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
                            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
}

void renderBigPerspectiveNearestWrap(BenchContext& ctx, uint32_t frame_index)
{
    applyVisualMotion(ctx, frame_index, 0.45f);
    drawTexturedBigTriangles(ctx, &g_texture.wrap_texture);
}

bool setupBigAffineNearestWrap(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
                            tgx::SHADER_TEXTURE_AFFINE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
}

void renderBigAffineNearestWrap(BenchContext& ctx, uint32_t frame_index)
{
    applyVisualMotion(ctx, frame_index, 0.45f);
    drawTexturedBigTriangles(ctx, &g_texture.wrap_texture);
}

bool setupBigPerspectiveBilinearClamp(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_UNLIT |
                            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_BILINEAR | tgx::SHADER_TEXTURE_CLAMP);
}

void renderBigPerspectiveBilinearClamp(BenchContext& ctx, uint32_t frame_index)
{
    applyVisualMotion(ctx, frame_index, 0.30f);
    drawTexturedBigTriangles(ctx, &g_texture.clamp_texture);
}

bool setupSmallGridPerspectiveWrap(BenchContext& ctx)
{
    if (!ensureGrid(ctx)) return false;
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
                            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
}

void renderSmallGridPerspectiveWrap(BenchContext& ctx, uint32_t frame_index)
{
    applyVisualMotion(ctx, frame_index, 0.25f);
    ctx.renderer.drawTriangles(g_grid.triangle_count,
                               g_grid.indices, g_grid.vertices,
                               g_grid.indices, g_grid.normals,
                               g_grid.indices, g_grid.texcoords,
                               &g_texture.wrap_texture);
}

bool setupSmallGridAffineWrap(BenchContext& ctx)
{
    if (!ensureGrid(ctx)) return false;
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
                            tgx::SHADER_TEXTURE_AFFINE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
}

void renderSmallGridAffineWrap(BenchContext& ctx, uint32_t frame_index)
{
    applyVisualMotion(ctx, frame_index, 0.25f);
    ctx.renderer.drawTriangles(g_grid.triangle_count,
                               g_grid.indices, g_grid.vertices,
                               g_grid.indices, g_grid.normals,
                               g_grid.indices, g_grid.texcoords,
                               &g_texture.wrap_texture);
}

bool setupStripPerspectiveWrap(BenchContext& ctx)
{
    if (!ensureStrip(ctx)) return false;
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
                            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
}

void renderStripPerspectiveWrap(BenchContext& ctx, uint32_t frame_index)
{
    applyVisualMotion(ctx, frame_index, 0.35f);
    ctx.renderer.drawTriangleStrip(g_strip.index_count,
                                   g_strip.indices, g_strip.vertices,
                                   g_strip.indices, g_strip.normals,
                                   g_strip.indices, g_strip.texcoords,
                                   &g_texture.wrap_texture);
}

bool setupQuadsBilinearClampNoZ(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_NOZBUFFER | tgx::SHADER_UNLIT |
                            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_BILINEAR | tgx::SHADER_TEXTURE_CLAMP);
}

void renderQuadsBilinearClampNoZ(BenchContext& ctx, uint32_t frame_index)
{
    applyVisualMotion(ctx, frame_index, 0.20f);

    const tgx::fVec3 n{0.0f, 0.0f, 1.0f};
    const tgx::fVec2 t0{-0.25f, -0.15f};
    const tgx::fVec2 t1{ 1.25f, -0.15f};
    const tgx::fVec2 t2{ 1.25f,  1.20f};
    const tgx::fVec2 t3{-0.25f,  1.20f};

    ctx.renderer.drawQuad(tgx::fVec3{-2.9f, -1.8f, -6.0f},
                          tgx::fVec3{ 0.0f, -1.7f, -5.7f},
                          tgx::fVec3{ 0.2f,  1.5f, -6.1f},
                          tgx::fVec3{-2.7f,  1.7f, -6.3f},
                          &n, &n, &n, &n, &t0, &t1, &t2, &t3, &g_texture.clamp_texture);

    ctx.renderer.drawQuad(tgx::fVec3{ 0.1f, -1.6f, -5.4f},
                          tgx::fVec3{ 2.9f, -1.7f, -6.1f},
                          tgx::fVec3{ 2.7f,  1.8f, -5.8f},
                          tgx::fVec3{ 0.0f,  1.5f, -6.2f},
                          &n, &n, &n, &n, &t1, &t0, &t3, &t2, &g_texture.clamp_texture);
}

bool setupCubeNearestWrap(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
                            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
}

void renderCubeNearestWrap(BenchContext& ctx, uint32_t frame_index)
{
    const float angle = frame_index ? visualPhaseDeg(frame_index) : 0.0f;
    ctx.renderer.setMaterial(tgx::RGBf{1.0f, 1.0f, 1.0f}, 0.18f, 0.86f, 0.0f, 32);
    ctx.renderer.setModelPosScaleRot(tgx::fVec3{0.0f, 0.0f, -5.8f},
                                     tgx::fVec3{1.15f, 1.15f, 1.15f},
                                     22.0f + angle * 0.45f,
                                     tgx::fVec3{0.20f, 0.90f, 0.32f});
    ctx.renderer.drawCube(&g_texture.wrap_texture,
                          &g_texture.clamp_texture,
                          &g_texture.wrap_texture,
                          &g_texture.clamp_texture,
                          &g_texture.wrap_texture,
                          &g_texture.clamp_texture);
}

const BenchTest TEXTURE_RASTER_TESTS[] = {
    {
        "texture_raster.big_tri.perspective.nearest.wrap",
        "large perspective-correct textured triangles nearest wrap",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupBigPerspectiveNearestWrap,
        renderBigPerspectiveNearestWrap,
        nullptr
    },
    {
        "texture_raster.big_tri.affine.nearest.wrap",
        "large affine textured triangles nearest wrap",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
            tgx::SHADER_TEXTURE_AFFINE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupBigAffineNearestWrap,
        renderBigAffineNearestWrap,
        nullptr
    },
    {
        "texture_raster.big_tri.perspective.bilinear.clamp",
        "large perspective-correct textured triangles bilinear clamp",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_UNLIT |
            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_BILINEAR | tgx::SHADER_TEXTURE_CLAMP,
        0, 0, 0,
        setupBigPerspectiveBilinearClamp,
        renderBigPerspectiveBilinearClamp,
        nullptr
    },
    {
        "texture_raster.small_grid.perspective.nearest.wrap",
        "small triangle grid perspective texture nearest wrap",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupSmallGridPerspectiveWrap,
        renderSmallGridPerspectiveWrap,
        nullptr
    },
    {
        "texture_raster.small_grid.affine.nearest.wrap",
        "small triangle grid affine texture nearest wrap",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
            tgx::SHADER_TEXTURE_AFFINE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupSmallGridAffineWrap,
        renderSmallGridAffineWrap,
        nullptr
    },
    {
        "texture_raster.triangle_strip.perspective.nearest.wrap",
        "textured triangle strip perspective nearest wrap",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupStripPerspectiveWrap,
        renderStripPerspectiveWrap,
        nullptr
    },
    {
        "texture_raster.quads.perspective.bilinear.clamp.noz",
        "textured quads bilinear clamp without zbuffer",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_NOZBUFFER | tgx::SHADER_UNLIT |
            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_BILINEAR | tgx::SHADER_TEXTURE_CLAMP,
        0, 0, 0,
        setupQuadsBilinearClampNoZ,
        renderQuadsBilinearClampNoZ,
        nullptr
    },
    {
        "texture_raster.cube.faces.nearest.wrap",
        "textured cube with per-face textures nearest wrap",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupCubeNearestWrap,
        renderCubeNearestWrap,
        nullptr
    }
};

const BenchModule TEXTURE_RASTER_MODULE = {
    "texture_raster",
    "Texture raster pipeline",
    TEXTURE_RASTER_TESTS,
    sizeof(TEXTURE_RASTER_TESTS) / sizeof(TEXTURE_RASTER_TESTS[0])
};

} // namespace

const BenchModule& getTextureRasterBenchModule()
{
    return TEXTURE_RASTER_MODULE;
}

} // namespace tgxbench
