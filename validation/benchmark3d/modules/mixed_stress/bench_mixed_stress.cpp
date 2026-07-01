#include "bench_mixed_stress.h"

namespace tgxbench
{

namespace
{

constexpr int MIX_TEX_W = 40;
constexpr int MIX_TEX_H = 40;
constexpr int MIX_SKY_W = 24;
constexpr int MIX_SKY_H = 24;

struct MixedStressState
    {
    tgx::RGB565* floor_pixels;
    tgx::RGB565* object_pixels;
    tgx::RGB565* accent_pixels;
    tgx::RGB565* sky_pixels[6];
    tgx::Image<tgx::RGB565> floor_texture;
    tgx::Image<tgx::RGB565> object_texture;
    tgx::Image<tgx::RGB565> accent_texture;
    tgx::Image<tgx::RGB565> sky_faces[6];

    int grid_n;
    int vertex_count;
    int triangle_count;
    tgx::fVec3* vertices;
    tgx::fVec3* normals;
    tgx::fVec2* texcoords;
    uint16_t* indices;

    int strip_count;
    tgx::fVec3* strip_vertices;
    tgx::fVec3* strip_normals;
    tgx::fVec2* strip_texcoords;
    uint16_t* strip_indices;

    tgx::Mesh3D<tgx::RGB565> mesh;
    uint16_t* mesh_faces;
    bool ready;
    };

static MixedStressState g_mix = {};

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

int gridNForCaps(const BenchContext& ctx)
{
    if (ctx.caps.power_class >= 8) return 9;
    if (ctx.caps.power_class >= 5) return 8;
    if (ctx.caps.power_class >= 4) return 7;
    return 5;
}

int stripCountForCaps(const BenchContext& ctx)
{
    if (ctx.caps.power_class >= 8) return 24;
    if (ctx.caps.power_class >= 5) return 20;
    if (ctx.caps.power_class >= 4) return 18;
    return 14;
}

int sectorsForCaps(const BenchContext& ctx)
{
    if (ctx.caps.power_class >= 8) return 18;
    if (ctx.caps.power_class >= 5) return 16;
    if (ctx.caps.power_class >= 4) return 14;
    return 10;
}

float surfaceHeight(float x, float y)
{
    return 0.18f * tgx::tgx_fast_sin_deg_clamped(wrapDeg180(x * 180.0f)) *
                   tgx::tgx_fast_cos_deg_clamped(wrapDeg180(y * 210.0f)) +
           0.05f * tgx::tgx_fast_sin_deg_clamped(wrapDeg180((x - y) * 145.0f));
}

tgx::fVec3 surfaceNormal(float x, float y)
{
    const float e = 0.025f;
    const float dzdx = (surfaceHeight(x + e, y) - surfaceHeight(x - e, y)) / (2.0f * e);
    const float dzdy = (surfaceHeight(x, y + e) - surfaceHeight(x, y - e)) / (2.0f * e);
    tgx::fVec3 n{-dzdx, -dzdy, 1.0f};
    n.normalize_fast();
    return n;
}

tgx::RGB565 floorColor(int x, int y)
{
    const bool checker = (((x >> 3) ^ (y >> 3)) & 1) != 0;
    const bool line = ((x & 15) == 0) || ((y & 15) == 0);
    if (line) return rgb888(220, 224, 210);
    return checker ? rgb888(54, 128, 172) : rgb888(184, 190, 178);
}

tgx::RGB565 objectColor(int x, int y)
{
    const bool tile = (((x >> 3) ^ (y >> 4)) & 1) != 0;
    const int stripe = (x * 5 + y * 3) & 31;
    int r = tile ? 218 : 72;
    int g = tile ? 116 : 174;
    int b = tile ? 52 : 132;
    if (stripe < 8)
        {
        r += tile ? 24 : 48;
        g += tile ? 32 : 20;
        b += tile ? 10 : 22;
        }
    return rgb888(r, g, b);
}

tgx::RGB565 accentColor(int x, int y)
{
    const int dx = x - MIX_TEX_W / 2;
    const int dy = y - MIX_TEX_H / 2;
    const bool ring = (((dx * dx + dy * dy) >> 5) & 1) != 0;
    const bool slash = ((x * 2 + y * 5) & 31) < 9;
    if (ring) return slash ? rgb888(246, 216, 80) : rgb888(236, 80, 92);
    return slash ? rgb888(66, 210, 190) : rgb888(28, 86, 152);
}

tgx::RGB565 skyColor(int face, int x, int y)
{
    const int v = (y * 255) / (MIX_SKY_H - 1);
    const int wave = (int)(22.0f * tgx::tgx_fast_sin_deg_clamped(wrapDeg180((float)(x * 11 + y * 7 + face * 29))));
    const bool star = (((x * 43 + y * 89 + face * 31) & 255) > 249) && (face != 3);
    int r = 16 + face * 5 + v / 12;
    int g = 30 + face * 3 + v / 7;
    int b = 78 + face * 9 + wave - v / 12;
    if (face == 2) { r += 28; g += 22; b += 10; }
    if (face == 3) { r = 34 + v / 6; g = 30 + v / 7; b = 36 + v / 8; }
    if (star) { r = 245; g = 238; b = 216; }
    return rgb888(r, g, b);
}

bool buildTextures()
{
    if (g_mix.floor_pixels) return true;

    g_mix.floor_pixels = static_cast<tgx::RGB565*>(
        benchAlloc(BenchMemoryKind::AssetCache, sizeof(tgx::RGB565) * MIX_TEX_W * MIX_TEX_H, 4));
    g_mix.object_pixels = static_cast<tgx::RGB565*>(
        benchAlloc(BenchMemoryKind::AssetCache, sizeof(tgx::RGB565) * MIX_TEX_W * MIX_TEX_H, 4));
    g_mix.accent_pixels = static_cast<tgx::RGB565*>(
        benchAlloc(BenchMemoryKind::AssetCache, sizeof(tgx::RGB565) * MIX_TEX_W * MIX_TEX_H, 4));
    if ((!g_mix.floor_pixels) || (!g_mix.object_pixels) || (!g_mix.accent_pixels)) return false;

    for (int y = 0; y < MIX_TEX_H; ++y)
        {
        for (int x = 0; x < MIX_TEX_W; ++x)
            {
            g_mix.floor_pixels[x + y * MIX_TEX_W] = floorColor(x, y);
            g_mix.object_pixels[x + y * MIX_TEX_W] = objectColor(x, y);
            g_mix.accent_pixels[x + y * MIX_TEX_W] = accentColor(x, y);
            }
        }
    g_mix.floor_texture.set(g_mix.floor_pixels, MIX_TEX_W, MIX_TEX_H);
    g_mix.object_texture.set(g_mix.object_pixels, MIX_TEX_W, MIX_TEX_H);
    g_mix.accent_texture.set(g_mix.accent_pixels, MIX_TEX_W, MIX_TEX_H);

    for (int face = 0; face < 6; ++face)
        {
        g_mix.sky_pixels[face] = static_cast<tgx::RGB565*>(
            benchAlloc(BenchMemoryKind::AssetCache, sizeof(tgx::RGB565) * MIX_SKY_W * MIX_SKY_H, 4));
        if (!g_mix.sky_pixels[face]) return false;
        for (int y = 0; y < MIX_SKY_H; ++y)
            {
            for (int x = 0; x < MIX_SKY_W; ++x)
                {
                g_mix.sky_pixels[face][x + y * MIX_SKY_W] = skyColor(face, x, y);
                }
            }
        g_mix.sky_faces[face].set(g_mix.sky_pixels[face], MIX_SKY_W, MIX_SKY_H);
        }

    return true;
}

bool buildGridGeometry(BenchContext& ctx)
{
    const int grid_n = gridNForCaps(ctx);
    if ((g_mix.vertices) && (g_mix.grid_n == grid_n)) return true;

    g_mix.grid_n = grid_n;
    g_mix.vertex_count = (grid_n + 1) * (grid_n + 1);
    g_mix.triangle_count = grid_n * grid_n * 2;

    g_mix.vertices = static_cast<tgx::fVec3*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(tgx::fVec3) * g_mix.vertex_count, 4));
    g_mix.normals = static_cast<tgx::fVec3*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(tgx::fVec3) * g_mix.vertex_count, 4));
    g_mix.texcoords = static_cast<tgx::fVec2*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(tgx::fVec2) * g_mix.vertex_count, 4));
    g_mix.indices = static_cast<uint16_t*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(uint16_t) * g_mix.triangle_count * 3, 2));
    g_mix.mesh_faces = static_cast<uint16_t*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(uint16_t) * (g_mix.triangle_count * 10 + 1), 2));
    if ((!g_mix.vertices) || (!g_mix.normals) || (!g_mix.texcoords) || (!g_mix.indices) || (!g_mix.mesh_faces))
        {
        return false;
        }

    for (int y = 0; y <= grid_n; ++y)
        {
        const float fy = (float)y / (float)grid_n;
        const float yy = fy * 2.0f - 1.0f;
        for (int x = 0; x <= grid_n; ++x)
            {
            const float fx = (float)x / (float)grid_n;
            const float xx = fx * 2.0f - 1.0f;
            const int k = x + y * (grid_n + 1);
            g_mix.vertices[k] = tgx::fVec3{xx * 2.2f, yy * 1.45f, surfaceHeight(xx, yy)};
            g_mix.normals[k] = surfaceNormal(xx, yy);
            g_mix.texcoords[k] = tgx::fVec2{fx * 3.1f, fy * 2.4f};
            }
        }

    int t = 0;
    int f = 0;
    for (int y = 0; y < grid_n; ++y)
        {
        for (int x = 0; x < grid_n; ++x)
            {
            const uint16_t i00 = (uint16_t)(x + y * (grid_n + 1));
            const uint16_t i10 = (uint16_t)(x + 1 + y * (grid_n + 1));
            const uint16_t i01 = (uint16_t)(x + (y + 1) * (grid_n + 1));
            const uint16_t i11 = (uint16_t)(x + 1 + (y + 1) * (grid_n + 1));
            const uint16_t tris[2][3] = { {i00, i10, i11}, {i00, i11, i01} };
            for (int tri = 0; tri < 2; ++tri)
                {
                g_mix.indices[t++] = tris[tri][0];
                g_mix.indices[t++] = tris[tri][1];
                g_mix.indices[t++] = tris[tri][2];
                g_mix.mesh_faces[f++] = 1;
                for (int k = 0; k < 3; ++k)
                    {
                    const uint16_t i = tris[tri][k];
                    g_mix.mesh_faces[f++] = i;
                    g_mix.mesh_faces[f++] = i;
                    g_mix.mesh_faces[f++] = i;
                    }
                }
            }
        }
    g_mix.mesh_faces[f++] = 0;

    g_mix.mesh.id = 1;
    g_mix.mesh.nb_vertices = (uint16_t)g_mix.vertex_count;
    g_mix.mesh.nb_texcoords = (uint16_t)g_mix.vertex_count;
    g_mix.mesh.nb_normals = (uint16_t)g_mix.vertex_count;
    g_mix.mesh.nb_faces = (uint16_t)g_mix.triangle_count;
    g_mix.mesh.len_face = (uint16_t)f;
    g_mix.mesh.vertice = g_mix.vertices;
    g_mix.mesh.texcoord = g_mix.texcoords;
    g_mix.mesh.normal = g_mix.normals;
    g_mix.mesh.face = g_mix.mesh_faces;
    g_mix.mesh.texture = &g_mix.object_texture;
    g_mix.mesh.color = tgx::RGBf{0.96f, 0.96f, 0.92f};
    g_mix.mesh.ambiant_strength = 0.14f;
    g_mix.mesh.diffuse_strength = 0.88f;
    g_mix.mesh.specular_strength = 0.28f;
    g_mix.mesh.specular_exponent = 24;
    g_mix.mesh.next = nullptr;
    g_mix.mesh.bounding_box = tgx::fBox3(-2.3f, 2.3f, -1.5f, 1.5f, -0.35f, 0.35f);
    g_mix.mesh.name = "mixed_stress_mesh";
    return true;
}

bool buildStripGeometry(BenchContext& ctx)
{
    const int strip_count = stripCountForCaps(ctx);
    if ((g_mix.strip_vertices) && (g_mix.strip_count == strip_count)) return true;

    g_mix.strip_count = strip_count;
    g_mix.strip_vertices = static_cast<tgx::fVec3*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(tgx::fVec3) * strip_count, 4));
    g_mix.strip_normals = static_cast<tgx::fVec3*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(tgx::fVec3) * strip_count, 4));
    g_mix.strip_texcoords = static_cast<tgx::fVec2*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(tgx::fVec2) * strip_count, 4));
    g_mix.strip_indices = static_cast<uint16_t*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(uint16_t) * strip_count, 2));
    if ((!g_mix.strip_vertices) || (!g_mix.strip_normals) || (!g_mix.strip_texcoords) || (!g_mix.strip_indices))
        {
        return false;
        }

    const int pairs = strip_count / 2;
    for (int p = 0; p < pairs; ++p)
        {
        const float u = (float)p / (float)(pairs - 1);
        const float x = -2.4f + 4.8f * u;
        const float z = 0.10f * tgx::tgx_fast_sin_deg_clamped(wrapDeg180(u * 360.0f));
        const float y_center = 0.18f * tgx::tgx_fast_sin_deg_clamped(wrapDeg180(u * 540.0f));
        const int a = 2 * p;
        const int b = a + 1;
        g_mix.strip_vertices[a] = tgx::fVec3{x, y_center - 0.34f, z};
        g_mix.strip_vertices[b] = tgx::fVec3{x, y_center + 0.34f, z};
        g_mix.strip_normals[a] = tgx::fVec3{0.0f, 0.0f, 1.0f};
        g_mix.strip_normals[b] = tgx::fVec3{0.0f, 0.0f, 1.0f};
        g_mix.strip_texcoords[a] = tgx::fVec2{u * 3.0f, 0.0f};
        g_mix.strip_texcoords[b] = tgx::fVec2{u * 3.0f, 1.0f};
        g_mix.strip_indices[a] = (uint16_t)a;
        g_mix.strip_indices[b] = (uint16_t)b;
        }
    return true;
}

bool ensureScene(BenchContext& ctx)
{
    if (g_mix.ready && (g_mix.grid_n == gridNForCaps(ctx)) && (g_mix.strip_count == stripCountForCaps(ctx))) return true;
    if (!buildTextures()) return false;
    if (!buildGridGeometry(ctx)) return false;
    if (!buildStripGeometry(ctx)) return false;
    g_mix.ready = true;
    return true;
}

void setShaderMode(BenchContext& ctx, tgx::Shader shaders)
{
    ctx.renderer.setShaders(shaders);
    ctx.renderer.setTextureQuality((shaders & tgx::SHADER_TEXTURE_BILINEAR) ? tgx::SHADER_TEXTURE_BILINEAR : tgx::SHADER_TEXTURE_NEAREST);
    ctx.renderer.setTextureWrappingMode((shaders & tgx::SHADER_TEXTURE_CLAMP) ? tgx::SHADER_TEXTURE_CLAMP : tgx::SHADER_TEXTURE_WRAP_POW2);
}

void resetCommon(BenchContext& ctx, tgx::Shader shaders)
{
    setShaderMode(ctx, shaders);
    ctx.renderer.setPerspective(50.0f, ((float)ctx.width) / ((float)ctx.height), 1.0f, 100.0f);
    ctx.renderer.setLookAt(tgx::fVec3{0.0f, 0.0f, 0.0f}, tgx::fVec3{0.0f, 0.0f, -1.0f}, tgx::fVec3{0.0f, 1.0f, 0.0f});
    ctx.renderer.setCulling(0);
    ctx.renderer.setDirectionalLightCount(1);
    ctx.renderer.setDirectionalLightAmbiant(tgx::RGBf{0.08f, 0.08f, 0.09f});
    ctx.renderer.setDirectionalLight(0, tgx::fVec3{0.25f, -0.55f, -1.0f},
                                     tgx::RGBf{0.20f, 0.20f, 0.22f},
                                     tgx::RGBf{0.18f, 0.17f, 0.15f});
    ctx.renderer.setSpotLightCount(0);
    ctx.renderer.setMaterial(tgx::RGBf{0.80f, 0.78f, 0.72f}, 0.16f, 0.86f, 0.24f, 24);
}

void setCamera(BenchContext& ctx, uint32_t frame_index, float yaw_offset, float distance)
{
    const float phase = frame_index ? visualPhaseDeg(frame_index) : 0.0f;
    const float yaw = yaw_offset + phase * 0.22f;
    const tgx::fVec3 eye{
        distance * tgx::tgx_fast_sin_deg_clamped(yaw),
        1.2f + 0.18f * tgx::tgx_fast_sin_deg_clamped(phase * 0.7f),
        -0.2f + distance * tgx::tgx_fast_cos_deg_clamped(yaw)
    };
    ctx.renderer.setLookAt(eye, tgx::fVec3{0.0f, -0.05f, -5.5f}, tgx::fVec3{0.0f, 1.0f, 0.0f});
}

void setModel(BenchContext& ctx, const tgx::fVec3& pos, const tgx::fVec3& scale,
              float angle, const tgx::fVec3& axis)
{
    ctx.renderer.setModelPosScaleRot(pos, scale, angle, axis);
}

void drawSkyLayer(BenchContext& ctx, uint32_t frame_index)
{
    setShaderMode(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_NOZBUFFER | tgx::SHADER_UNLIT |
                       tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
    ctx.renderer.drawSkyBox(&g_mix.sky_faces[0], &g_mix.sky_faces[1], &g_mix.sky_faces[2],
                            &g_mix.sky_faces[3], &g_mix.sky_faces[4], &g_mix.sky_faces[5],
                            frame_index ? visualPhaseDeg(frame_index) * 0.16f : 0.0f,
                            0.0f, 900.0f,
                            tgx::SHADER_TEXTURE_NEAREST, tgx::SHADER_TEXTURE_WRAP_POW2);
}

void drawGroundMesh(BenchContext& ctx, uint32_t frame_index, bool use_mesh)
{
    const float phase = visualPhaseDeg(frame_index);
    setShaderMode(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
                       tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
    ctx.renderer.setMaterial(tgx::RGBf{1.0f, 1.0f, 1.0f}, 0.18f, 0.90f, 0.18f, 22);
    setModel(ctx, tgx::fVec3{0.0f, -0.78f, -5.6f}, tgx::fVec3{1.42f, 1.0f, 1.42f},
             66.0f + phase * 0.05f, tgx::fVec3{1.0f, 0.0f, 0.0f});
    if (use_mesh)
        {
        ctx.renderer.drawMesh(&g_mix.mesh, true, true);
        }
    else
        {
        ctx.renderer.drawTriangles(g_mix.triangle_count,
                                   g_mix.indices, g_mix.vertices,
                                   g_mix.indices, g_mix.normals,
                                   g_mix.indices, g_mix.texcoords,
                                   &g_mix.floor_texture);
        }
}

void drawAffineRibbon(BenchContext& ctx, uint32_t frame_index, const tgx::fVec3& pos)
{
    const float phase = visualPhaseDeg(frame_index);
    setShaderMode(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
                       tgx::SHADER_TEXTURE_AFFINE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
    ctx.renderer.setMaterial(tgx::RGBf{1.0f, 1.0f, 1.0f}, 0.12f, 0.92f, 0.12f, 18);
    setModel(ctx, pos, tgx::fVec3{0.58f, 0.58f, 0.58f},
             -12.0f + phase * 0.10f, tgx::fVec3{0.1f, 1.0f, 0.2f});
    ctx.renderer.drawTriangleStrip(g_mix.strip_count,
                                   g_mix.strip_indices, g_mix.strip_vertices,
                                   g_mix.strip_indices, g_mix.strip_normals,
                                   g_mix.strip_indices, g_mix.strip_texcoords,
                                   &g_mix.accent_texture);
}

void drawFlatPrimitives(BenchContext& ctx, uint32_t frame_index)
{
    const float phase = visualPhaseDeg(frame_index);
    setShaderMode(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE);
    ctx.renderer.setMaterial(tgx::RGBf{0.94f, 0.52f, 0.26f}, 0.12f, 0.86f, 0.12f, 18);
    setModel(ctx, tgx::fVec3{-1.25f, -0.10f, -5.2f}, tgx::fVec3{0.48f, 0.68f, 0.48f},
             18.0f + phase * 0.16f, tgx::fVec3{0.2f, 1.0f, 0.3f});
    ctx.renderer.drawCube();

    ctx.renderer.setMaterial(tgx::RGBf{0.42f, 0.84f, 0.62f}, 0.12f, 0.84f, 0.18f, 18);
    setModel(ctx, tgx::fVec3{1.38f, -0.22f, -5.9f}, tgx::fVec3{0.40f, 0.78f, 0.40f},
             -22.0f + phase * 0.12f, tgx::fVec3{0.1f, 1.0f, 0.2f});
    ctx.renderer.drawTruncatedCone(sectorsForCaps(ctx), 1.0f, 0.38f, true, true);
}

void drawGouraudPrimitives(BenchContext& ctx, uint32_t frame_index)
{
    const float phase = visualPhaseDeg(frame_index);
    setShaderMode(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE);
    ctx.renderer.setMaterial(tgx::RGBf{0.38f, 0.66f, 0.96f}, 0.12f, 0.88f, 0.38f, 20);
    setModel(ctx, tgx::fVec3{0.45f, 0.10f, -5.55f}, tgx::fVec3{0.50f, 0.50f, 0.50f}, 0.0f,
             tgx::fVec3{0.0f, 1.0f, 0.0f});
    ctx.renderer.drawSphere(sectorsForCaps(ctx), sectorsForCaps(ctx) / 2 + 1);

    ctx.renderer.setMaterial(tgx::RGBf{0.82f, 0.76f, 0.52f}, 0.12f, 0.86f, 0.22f, 18);
    setModel(ctx, tgx::fVec3{-0.25f, -0.20f, -6.25f}, tgx::fVec3{0.36f, 0.66f, 0.36f},
             14.0f + phase * 0.09f, tgx::fVec3{0.15f, 1.0f, 0.1f});
    ctx.renderer.drawCylinder(sectorsForCaps(ctx), true, true);
}

void drawVertexColorPatch(BenchContext& ctx, uint32_t frame_index)
{
    const float phase = visualPhaseDeg(frame_index);
    const tgx::fVec3 n{0.0f, 0.0f, 1.0f};
    setShaderMode(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE);
    setModel(ctx, tgx::fVec3{0.15f, 0.82f, -5.0f}, tgx::fVec3{0.70f, 0.70f, 0.70f},
             phase * 0.10f, tgx::fVec3{0.1f, 1.0f, 0.0f});
    ctx.renderer.drawTriangleWithVertexColor(tgx::fVec3{-0.85f, -0.45f, 0.0f},
                                             tgx::fVec3{ 0.92f, -0.35f, 0.0f},
                                             tgx::fVec3{ 0.05f,  0.65f, 0.0f},
                                             tgx::RGBf{1.0f, 0.18f, 0.10f},
                                             tgx::RGBf{0.10f, 0.90f, 0.25f},
                                             tgx::RGBf{0.20f, 0.40f, 1.0f},
                                             &n, &n, &n);
}

void drawNoZOverlay(BenchContext& ctx, uint32_t frame_index)
{
    const float phase = visualPhaseDeg(frame_index);
    setShaderMode(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_NOZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE);
    ctx.renderer.setMaterial(tgx::RGBf{0.95f, 0.92f, 0.25f}, 1.0f, 0.0f, 0.0f, 16);
    setModel(ctx, tgx::fVec3{0.0f, 0.0f, -4.25f}, tgx::fVec3{1.0f, 1.0f, 1.0f},
             0.0f, tgx::fVec3{0.0f, 1.0f, 0.0f});
    ctx.renderer.drawWireFrameLine(tgx::fVec3{-1.8f, -1.05f, 0.0f}, tgx::fVec3{1.8f, 1.05f, 0.0f},
                                   1.25f, tgx::RGB565(31, 54, 10), 0.82f);
    ctx.renderer.drawWireFrameLine(tgx::fVec3{-1.8f, 1.05f, 0.0f}, tgx::fVec3{1.8f, -1.05f, 0.0f});
    ctx.renderer.drawDot(tgx::fVec3{0.75f * tgx::tgx_fast_sin_deg_clamped(phase),
                                    0.45f * tgx::tgx_fast_cos_deg_clamped(phase * 1.3f), 0.0f},
                         3, tgx::RGB565(31, 58, 18), 0.9f);
}

bool setupLayeredFullScene(BenchContext& ctx)
{
    if (!ensureScene(ctx)) return false;
    resetCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
                     tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
    ctx.renderer.setSpotLight(0, tgx::fVec3{-1.4f, 0.9f, -3.8f}, 4.0f,
                              tgx::RGBf{1.0f, 0.55f, 0.28f},
                              tgx::RGBf{0.45f, 0.40f, 0.34f});
    ctx.renderer.setSpotLightCount(1);
    return true;
}

void renderLayeredFullScene(BenchContext& ctx, uint32_t frame_index)
{
    const float phase = visualPhaseDeg(frame_index);
    setCamera(ctx, frame_index, -7.0f, 0.95f);
    drawSkyLayer(ctx, frame_index);
    ctx.renderer.setSpotLightPosition(0, tgx::fVec3{-1.2f + 0.75f * tgx::tgx_fast_sin_deg_clamped(phase),
                                                    0.85f + 0.25f * tgx::tgx_fast_cos_deg_clamped(phase * 1.2f),
                                                    -3.7f});
    drawGroundMesh(ctx, frame_index, true);
    drawFlatPrimitives(ctx, frame_index);
    drawGouraudPrimitives(ctx, frame_index);
    drawAffineRibbon(ctx, frame_index, tgx::fVec3{0.0f, 0.60f, -5.45f});
    drawVertexColorPatch(ctx, frame_index);
    drawNoZOverlay(ctx, frame_index);
}

bool setupShaderPingPong(BenchContext& ctx)
{
    if (!ensureScene(ctx)) return false;
    resetCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE);
    return true;
}

void renderShaderPingPong(BenchContext& ctx, uint32_t frame_index)
{
    setCamera(ctx, frame_index, 8.0f, 0.6f);

    setShaderMode(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE);
    ctx.renderer.setMaterial(tgx::RGBf{0.72f, 0.46f, 0.30f}, 0.12f, 0.86f, 0.12f, 18);
    setModel(ctx, tgx::fVec3{-1.1f, -0.35f, -5.3f}, tgx::fVec3{0.55f, 0.55f, 0.55f},
             22.0f, tgx::fVec3{0.2f, 1.0f, 0.3f});
    ctx.renderer.drawCube();

    drawVertexColorPatch(ctx, frame_index);

    setShaderMode(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
                       tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_BILINEAR | tgx::SHADER_TEXTURE_CLAMP);
    ctx.renderer.setMaterial(tgx::RGBf{1.0f, 1.0f, 1.0f}, 0.16f, 0.86f, 0.12f, 20);
    const tgx::fVec3 n{0.0f, 0.0f, 1.0f};
    const tgx::fVec2 t0{0.0f, 0.0f};
    const tgx::fVec2 t1{1.0f, 0.0f};
    const tgx::fVec2 t2{1.0f, 1.0f};
    const tgx::fVec2 t3{0.0f, 1.0f};
    setModel(ctx, tgx::fVec3{1.0f, -0.35f, -5.6f}, tgx::fVec3{0.82f, 0.82f, 0.82f},
             -14.0f, tgx::fVec3{0.0f, 1.0f, 0.0f});
    ctx.renderer.drawQuad(tgx::fVec3{-0.9f, -0.55f, 0.0f}, tgx::fVec3{0.9f, -0.55f, 0.0f},
                          tgx::fVec3{0.9f, 0.55f, 0.0f}, tgx::fVec3{-0.9f, 0.55f, 0.0f},
                          &n, &n, &n, &n, &t0, &t1, &t2, &t3, &g_mix.object_texture);

    drawAffineRibbon(ctx, frame_index, tgx::fVec3{0.0f, 0.35f, -5.2f});
    drawNoZOverlay(ctx, frame_index);
}

bool setupGeometryKitchenSink(BenchContext& ctx)
{
    if (!ensureScene(ctx)) return false;
    resetCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
                     tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
    ctx.renderer.setDirectionalLightAmbiant(tgx::RGBf{0.12f, 0.12f, 0.13f});
    ctx.renderer.setDirectionalLight(0, tgx::fVec3{0.22f, -0.62f, -1.0f},
                                     tgx::RGBf{0.24f, 0.24f, 0.26f},
                                     tgx::RGBf{0.18f, 0.17f, 0.15f});
    return true;
}

void renderGeometryKitchenSink(BenchContext& ctx, uint32_t frame_index)
{
    const float phase = visualPhaseDeg(frame_index);
    setCamera(ctx, frame_index, 18.0f, 1.1f);
    drawGroundMesh(ctx, frame_index, false);

    setShaderMode(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
                       tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
    ctx.renderer.setMaterial(tgx::RGBf{1.0f, 1.0f, 1.0f}, 0.16f, 0.86f, 0.18f, 24);
    setModel(ctx, tgx::fVec3{-1.15f, 0.18f, -5.45f}, tgx::fVec3{0.44f, 0.44f, 0.44f},
             phase * 0.12f, tgx::fVec3{0.2f, 1.0f, 0.2f});
    ctx.renderer.drawCube(&g_mix.object_texture, &g_mix.object_texture, &g_mix.object_texture,
                          &g_mix.object_texture, &g_mix.object_texture, &g_mix.object_texture);

    drawGouraudPrimitives(ctx, frame_index);
    drawFlatPrimitives(ctx, frame_index);
    drawAffineRibbon(ctx, frame_index, tgx::fVec3{0.20f, 0.58f, -5.65f});
    drawVertexColorPatch(ctx, frame_index);

    setShaderMode(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE);
    setModel(ctx, tgx::fVec3{1.45f, 0.24f, -5.95f}, tgx::fVec3{0.42f, 0.34f, 0.42f},
             20.0f + phase * 0.06f, tgx::fVec3{0.1f, 1.0f, 0.2f});
    ctx.renderer.drawWireFrameMesh(&g_mix.mesh, true, 1.10f, tgx::RGB565(31, 34, 8), 0.62f);
}

bool setupLightingStateChurn(BenchContext& ctx)
{
    if (!ensureScene(ctx)) return false;
    resetCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE);
    ctx.renderer.setDirectionalLightCount(2);
    ctx.renderer.setDirectionalLight(0, tgx::fVec3{0.24f, -0.55f, -1.0f},
                                     tgx::RGBf{0.10f, 0.10f, 0.11f},
                                     tgx::RGBf{0.30f, 0.28f, 0.24f});
    ctx.renderer.setDirectionalLight(1, tgx::fVec3{-0.50f, -0.22f, -0.80f},
                                     tgx::RGBf{0.03f, 0.03f, 0.04f},
                                     tgx::RGBf{0.12f, 0.18f, 0.28f});
    ctx.renderer.setSpotLight(0, tgx::fVec3{-1.3f, 0.85f, -3.7f}, 4.0f,
                              tgx::RGBf{1.0f, 0.42f, 0.22f},
                              tgx::RGBf{0.50f, 0.34f, 0.24f});
    ctx.renderer.setSpotLight(1, tgx::fVec3{1.35f, -0.45f, -4.0f}, 3.5f,
                              tgx::RGBf{0.12f, 0.35f, 1.0f},
                              tgx::RGBf{0.20f, 0.30f, 0.50f});
    ctx.renderer.setSpotLightCount(ctx.caps.power_class >= 4 ? 2 : 1);
    return true;
}

void renderLightingStateChurn(BenchContext& ctx, uint32_t frame_index)
{
    const float phase = visualPhaseDeg(frame_index);
    setCamera(ctx, frame_index, -16.0f, 0.9f);
    ctx.renderer.setSpotLightPosition(0, tgx::fVec3{-1.25f + 0.60f * tgx::tgx_fast_sin_deg_clamped(phase),
                                                    0.85f,
                                                    -3.75f + 0.22f * tgx::tgx_fast_cos_deg_clamped(phase)});
    if (ctx.caps.power_class >= 4)
        {
        ctx.renderer.setSpotLightPosition(1, tgx::fVec3{1.20f,
                                                        -0.40f + 0.50f * tgx::tgx_fast_cos_deg_clamped(phase * 1.2f),
                                                        -4.10f});
        }

    drawGroundMesh(ctx, frame_index, true);
    drawGouraudPrimitives(ctx, frame_index);
    drawFlatPrimitives(ctx, frame_index);
    drawAffineRibbon(ctx, frame_index, tgx::fVec3{0.0f, 0.72f, -5.55f});
}

bool setupClippingOverlay(BenchContext& ctx)
{
    if (!ensureScene(ctx)) return false;
    resetCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
                     tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
    return true;
}

void renderClippingOverlay(BenchContext& ctx, uint32_t frame_index)
{
    const float phase = visualPhaseDeg(frame_index);
    ctx.renderer.setLookAt(tgx::fVec3{0.0f, 0.1f, 0.0f},
                           tgx::fVec3{0.0f, -0.2f, -1.0f},
                           tgx::fVec3{0.0f, 1.0f, 0.0f});

    setShaderMode(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
                       tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
    setModel(ctx, tgx::fVec3{-0.15f + 0.10f * tgx::tgx_fast_sin_deg_clamped(phase), -0.20f, -1.35f},
             tgx::fVec3{0.92f, 0.65f, 0.92f}, 18.0f + phase * 0.12f,
             tgx::fVec3{0.2f, 1.0f, 0.3f});
    ctx.renderer.drawMesh(&g_mix.mesh, true, true);

    setShaderMode(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE);
    ctx.renderer.setMaterial(tgx::RGBf{0.90f, 0.45f, 0.26f}, 0.12f, 0.86f, 0.14f, 18);
    setModel(ctx, tgx::fVec3{1.12f, -0.12f, -1.12f}, tgx::fVec3{0.46f, 0.46f, 0.46f},
             15.0f, tgx::fVec3{0.1f, 1.0f, 0.3f});
    ctx.renderer.drawCube();

    drawNoZOverlay(ctx, frame_index);
}

const BenchTest MIXED_STRESS_TESTS[] = {
    {
        "mixed_stress.layered_full_scene",
        "skybox plus textured mesh plus flat and Gouraud primitives plus affine strip plus no-z overlay",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_NOZBUFFER |
            tgx::SHADER_FLAT | tgx::SHADER_GOURAUD | tgx::SHADER_UNLIT |
            tgx::SHADER_NOTEXTURE | tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_AFFINE |
            tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupLayeredFullScene,
        renderLayeredFullScene,
        nullptr
    },
    {
        "mixed_stress.shader_pingpong",
        "explicit shader state switching across flat Gouraud bilinear affine and no-z paths",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_NOZBUFFER |
            tgx::SHADER_FLAT | tgx::SHADER_GOURAUD | tgx::SHADER_UNLIT |
            tgx::SHADER_NOTEXTURE | tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_AFFINE |
            tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_BILINEAR |
            tgx::SHADER_TEXTURE_WRAP_POW2 | tgx::SHADER_TEXTURE_CLAMP,
        0, 0, 0,
        setupShaderPingPong,
        renderShaderPingPong,
        nullptr
    },
    {
        "mixed_stress.geometry_kitchen_sink",
        "mesh plus cube sphere cylinder truncated cone triangle list strip and wire mesh",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER |
            tgx::SHADER_FLAT | tgx::SHADER_GOURAUD | tgx::SHADER_UNLIT |
            tgx::SHADER_NOTEXTURE | tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_AFFINE |
            tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupGeometryKitchenSink,
        renderGeometryKitchenSink,
        nullptr
    },
    {
        "mixed_stress.lighting_state_churn",
        "two directional lights plus point lights plus material changes plus mixed geometry",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER |
            tgx::SHADER_FLAT | tgx::SHADER_GOURAUD |
            tgx::SHADER_NOTEXTURE | tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_AFFINE |
            tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupLightingStateChurn,
        renderLightingStateChurn,
        nullptr
    },
    {
        "mixed_stress.clipping_overlay",
        "near-plane mesh clipping plus partial viewport primitive plus no-z overlay",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_NOZBUFFER |
            tgx::SHADER_FLAT | tgx::SHADER_GOURAUD | tgx::SHADER_UNLIT |
            tgx::SHADER_NOTEXTURE | tgx::SHADER_TEXTURE |
            tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupClippingOverlay,
        renderClippingOverlay,
        nullptr
    }
};

const BenchModule MIXED_STRESS_MODULE = {
    "mixed_stress",
    "Mixed real-life stress scenes",
    MIXED_STRESS_TESTS,
    sizeof(MIXED_STRESS_TESTS) / sizeof(MIXED_STRESS_TESTS[0])
};

} // namespace

const BenchModule& getMixedStressBenchModule()
{
    return MIXED_STRESS_MODULE;
}

} // namespace tgxbench
