#include "bench_mesh.h"

#include <stddef.h>
#include <stdint.h>

namespace tgxbench
{

namespace
{

constexpr int MESH_TEX_W = 64;
constexpr int MESH_TEX_H = 64;
constexpr int LEGACY_GRID_CELLS = 7;

struct MeshTextureState
    {
    tgx::RGB565* warm_pixels;
    tgx::RGB565* cool_pixels;
    tgx::Image<tgx::RGB565> warm_texture;
    tgx::Image<tgx::RGB565> cool_texture;
    };

struct MeshBenchState
    {
    MeshTextureState textures;

    tgx::MeshMaterial3Dv2<tgx::RGB565> v2_materials[2];

    tgx::Meshlet3Dv2 low_meshlets[2];
    uint32_t* low_payload;
    tgx::Mesh3Dv2<tgx::RGB565> low_mesh;

    tgx::Meshlet3Dv2 grid_meshlets[1];
    uint32_t* grid_payload;
    int grid_cells;
    tgx::Mesh3Dv2<tgx::RGB565> grid_mesh;

    tgx::fVec3* legacy_vertices;
    tgx::fVec3* legacy_normals;
    tgx::fVec2* legacy_texcoords;
    uint16_t* legacy_faces;
    tgx::Mesh3D<tgx::RGB565> legacy_mesh;

    bool ready;
    };

static MeshBenchState g_mesh = {};

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

int gridCellsForCaps(const BenchContext& ctx)
{
    if (ctx.caps.power_class >= 7) return 10;
    if (ctx.caps.power_class >= 4) return 8;
    return 6;
}

int16_t qSnorm(float value)
{
    if (value > 1.0f) value = 1.0f;
    if (value < -1.0f) value = -1.0f;
    const float scaled = value * 32767.0f;
    const int v = (int)(scaled >= 0.0f ? scaled + 0.5f : scaled - 0.5f);
    if (v > 32767) return 32767;
    if (v < -32767) return -32767;
    return (int16_t)v;
}

int16_t qTex(float value)
{
    if (value > 3.95f) value = 3.95f;
    if (value < -3.95f) value = -3.95f;
    const float scaled = value * (32767.0f / 4.0f);
    const int v = (int)(scaled >= 0.0f ? scaled + 0.5f : scaled - 0.5f);
    if (v > 32767) return 32767;
    if (v < -32767) return -32767;
    return (int16_t)v;
}

float surfaceHeight(float x, float y)
{
    return 0.18f * benchFastSinDeg(wrapDeg180(x * 210.0f)) *
                   benchFastCosDeg(wrapDeg180(y * 170.0f)) +
           0.07f * benchFastSinDeg(wrapDeg180((x - y) * 160.0f));
}

tgx::fVec3 surfaceNormal(float x, float y)
{
    const float e = 0.02f;
    const float dzdx = (surfaceHeight(x + e, y) - surfaceHeight(x - e, y)) / (2.0f * e);
    const float dzdy = (surfaceHeight(x, y + e) - surfaceHeight(x, y - e)) / (2.0f * e);
    tgx::fVec3 n{-dzdx, -dzdy, 1.0f};
    n.normalize_fast();
    return n;
}

tgx::RGB565 warmTextureColor(int x, int y)
{
    const bool checker = (((x >> 3) ^ (y >> 3)) & 1) != 0;
    const int stripe = (x * 3 + y * 5) & 31;
    int r = checker ? 30 : 12;
    int g = checker ? 34 : 18;
    int b = checker ? 8 : 4;
    if (stripe < 7)
        {
        r = checker ? 31 : 18;
        g = checker ? 52 : 28;
        b = checker ? 16 : 8;
        }
    return tgx::RGB565(r, g, b);
}

tgx::RGB565 coolTextureColor(int x, int y)
{
    const int dx = x - MESH_TEX_W / 2;
    const int dy = y - MESH_TEX_H / 2;
    const bool ring = (((dx * dx + dy * dy) >> 6) & 1) != 0;
    const bool stripe = ((x + y * 2) & 15) < 5;
    if (ring)
        {
        return stripe ? tgx::RGB565(6, 54, 31) : tgx::RGB565(4, 34, 24);
        }
    return stripe ? tgx::RGB565(12, 42, 30) : tgx::RGB565(3, 22, 18);
}

bool ensureTextures()
{
    if (g_mesh.textures.warm_pixels && g_mesh.textures.cool_pixels) return true;

    g_mesh.textures.warm_pixels = static_cast<tgx::RGB565*>(
        benchAlloc(BenchMemoryKind::AssetCache, sizeof(tgx::RGB565) * MESH_TEX_W * MESH_TEX_H, 4));
    g_mesh.textures.cool_pixels = static_cast<tgx::RGB565*>(
        benchAlloc(BenchMemoryKind::AssetCache, sizeof(tgx::RGB565) * MESH_TEX_W * MESH_TEX_H, 4));
    if ((!g_mesh.textures.warm_pixels) || (!g_mesh.textures.cool_pixels)) return false;

    for (int y = 0; y < MESH_TEX_H; ++y)
        {
        for (int x = 0; x < MESH_TEX_W; ++x)
            {
            g_mesh.textures.warm_pixels[x + y * MESH_TEX_W] = warmTextureColor(x, y);
            g_mesh.textures.cool_pixels[x + y * MESH_TEX_W] = coolTextureColor(x, y);
            }
        }

    g_mesh.textures.warm_texture.set(g_mesh.textures.warm_pixels, MESH_TEX_W, MESH_TEX_H);
    g_mesh.textures.cool_texture.set(g_mesh.textures.cool_pixels, MESH_TEX_W, MESH_TEX_H);
    return true;
}

void setupMaterials()
{
    g_mesh.v2_materials[0].texture = &g_mesh.textures.warm_texture;
    g_mesh.v2_materials[0].color = tgx::RGBf{0.82f, 0.50f, 0.26f};
    g_mesh.v2_materials[0].ambiant_strength = 0.18f;
    g_mesh.v2_materials[0].diffuse_strength = 0.86f;
    g_mesh.v2_materials[0].specular_strength = 0.24f;
    g_mesh.v2_materials[0].specular_exponent = 24;

    g_mesh.v2_materials[1].texture = &g_mesh.textures.cool_texture;
    g_mesh.v2_materials[1].color = tgx::RGBf{0.24f, 0.62f, 0.90f};
    g_mesh.v2_materials[1].ambiant_strength = 0.16f;
    g_mesh.v2_materials[1].diffuse_strength = 0.88f;
    g_mesh.v2_materials[1].specular_strength = 0.42f;
    g_mesh.v2_materials[1].specular_exponent = 18;
}

void initMeshletHeader(tgx::Meshlet3Dv2& meshlet, uint32_t payload_offset32, uint8_t nb_vertices,
                       uint8_t nb_normals, uint8_t nb_texcoords, uint8_t material_index)
{
    meshlet.sphere_center[0] = 0;
    meshlet.sphere_center[1] = 0;
    meshlet.sphere_center[2] = 0;
    meshlet.cone_dir[0] = 0;
    meshlet.cone_dir[1] = 0;
    meshlet.cone_dir[2] = 0;
    meshlet.sphere_radius = 32767;
    meshlet.cone_cos = -32767;
    meshlet.payload_offset32 = payload_offset32;
    meshlet.nb_vertices = nb_vertices;
    meshlet.nb_normals = nb_normals;
    meshlet.nb_texcoords = nb_texcoords;
    meshlet.material_index = material_index;
}

uint8_t* align4(uint8_t* p, const uint8_t* base)
{
    while ((((uintptr_t)(p - base)) & 3U) != 0U) *(p++) = 0;
    return p;
}

uint32_t writeLowMeshlet(uint32_t* payload, const tgx::fVec3 vertices[4],
                         const tgx::fVec3 normals[4], uint8_t material_index)
{
    uint8_t* const base = reinterpret_cast<uint8_t*>(payload);
    uint8_t* p = base;

    int16_t* qv = reinterpret_cast<int16_t*>(p);
    p += 4 * 3 * sizeof(int16_t);
    int16_t* qn = reinterpret_cast<int16_t*>(p);
    p += 4 * 3 * sizeof(int16_t);
    int16_t* qt = reinterpret_cast<int16_t*>(p);
    p += 4 * 2 * sizeof(int16_t);

    const tgx::fVec2 tex[4] = {
        tgx::fVec2{0.0f, 0.0f},
        tgx::fVec2{1.4f, 0.1f},
        tgx::fVec2{1.2f, 1.2f},
        tgx::fVec2{0.1f, 1.1f}
    };

    for (int i = 0; i < 4; ++i)
        {
        qv[i * 3 + 0] = qSnorm(vertices[i].x);
        qv[i * 3 + 1] = qSnorm(vertices[i].y);
        qv[i * 3 + 2] = qSnorm(vertices[i].z);
        qn[i * 3 + 0] = qSnorm(normals[i].x);
        qn[i * 3 + 1] = qSnorm(normals[i].y);
        qn[i * 3 + 2] = qSnorm(normals[i].z);
        qt[i * 2 + 0] = qTex(tex[i].x);
        qt[i * 2 + 1] = qTex(tex[i].y);
        }

    const uint8_t tri[2][3] = { {0, 1, 2}, {0, 2, 3} };
    for (int t = 0; t < 2; ++t)
        {
        *(p++) = 1;
        for (int k = 0; k < 3; ++k)
            {
            const uint8_t i = tri[t][k];
            *(p++) = i;
            *(p++) = i;
            *(p++) = i;
            }
        }
    *(p++) = 0;
    p = align4(p, base);
    (void)material_index;
    return (uint32_t)((p - base) / 4);
}

bool buildLowMesh()
{
    if (g_mesh.low_payload) return true;

    g_mesh.low_payload = static_cast<uint32_t*>(benchAlloc(BenchMemoryKind::AssetCache, 256 * sizeof(uint32_t), 4));
    if (!g_mesh.low_payload) return false;

    const tgx::fVec3 left_v[4] = {
        tgx::fVec3{-0.96f, -0.82f, -0.08f},
        tgx::fVec3{ 0.04f, -0.90f,  0.10f},
        tgx::fVec3{ 0.10f,  0.84f, -0.12f},
        tgx::fVec3{-0.90f,  0.92f,  0.06f}
    };
    const tgx::fVec3 right_v[4] = {
        tgx::fVec3{-0.05f, -0.86f,  0.05f},
        tgx::fVec3{ 0.95f, -0.76f, -0.12f},
        tgx::fVec3{ 0.86f,  0.90f,  0.10f},
        tgx::fVec3{ 0.02f,  0.80f, -0.04f}
    };
    const tgx::fVec3 normals[4] = {
        tgx::fVec3{-0.20f, -0.10f, 0.97f},
        tgx::fVec3{ 0.10f, -0.18f, 0.98f},
        tgx::fVec3{ 0.18f,  0.14f, 0.97f},
        tgx::fVec3{-0.14f,  0.16f, 0.98f}
    };

    uint32_t offset = 0;
    initMeshletHeader(g_mesh.low_meshlets[0], offset, 4, 4, 4, 0);
    offset += writeLowMeshlet(g_mesh.low_payload + offset, left_v, normals, 0);
    initMeshletHeader(g_mesh.low_meshlets[1], offset, 4, 4, 4, 1);
    offset += writeLowMeshlet(g_mesh.low_payload + offset, right_v, normals, 1);

    g_mesh.low_mesh.id = 2162;
    g_mesh.low_mesh.nb_meshlets = 2;
    g_mesh.low_mesh.nb_materials = 2;
    g_mesh.low_mesh.materials = g_mesh.v2_materials;
    g_mesh.low_mesh.meshlets = g_mesh.low_meshlets;
    g_mesh.low_mesh.payload = g_mesh.low_payload;
    g_mesh.low_mesh.bounding_box = tgx::fBox3(-1.0f, 1.0f, -1.0f, 1.0f, -0.25f, 0.25f);
    g_mesh.low_mesh.name = "synthetic_low_mesh_v2";
    g_mesh.low_mesh.material_extras = nullptr;
    return true;
}

uint32_t writeGridMeshlet(uint32_t* payload, int cells)
{
    const int side = cells + 1;
    const int vertices = side * side;
    const int triangles = cells * cells * 2;

    uint8_t* const base = reinterpret_cast<uint8_t*>(payload);
    uint8_t* p = base;

    int16_t* qv = reinterpret_cast<int16_t*>(p);
    p += vertices * 3 * sizeof(int16_t);
    int16_t* qn = reinterpret_cast<int16_t*>(p);
    p += vertices * 3 * sizeof(int16_t);
    int16_t* qt = reinterpret_cast<int16_t*>(p);
    p += vertices * 2 * sizeof(int16_t);

    for (int y = 0; y <= cells; ++y)
        {
        const float fy01 = (float)y / (float)cells;
        const float yy = fy01 * 2.0f - 1.0f;
        for (int x = 0; x <= cells; ++x)
            {
            const float fx01 = (float)x / (float)cells;
            const float xx = fx01 * 2.0f - 1.0f;
            const int k = x + y * side;
            const float zz = surfaceHeight(xx, yy);
            const tgx::fVec3 n = surfaceNormal(xx, yy);
            qv[k * 3 + 0] = qSnorm(xx);
            qv[k * 3 + 1] = qSnorm(yy);
            qv[k * 3 + 2] = qSnorm(zz);
            qn[k * 3 + 0] = qSnorm(n.x);
            qn[k * 3 + 1] = qSnorm(n.y);
            qn[k * 3 + 2] = qSnorm(n.z);
            qt[k * 2 + 0] = qTex(fx01 * 2.2f);
            qt[k * 2 + 1] = qTex(fy01 * 1.7f);
            }
        }

    for (int y = 0; y < cells; ++y)
        {
        for (int x = 0; x < cells; ++x)
            {
            const uint8_t i00 = (uint8_t)(x + y * side);
            const uint8_t i10 = (uint8_t)(x + 1 + y * side);
            const uint8_t i01 = (uint8_t)(x + (y + 1) * side);
            const uint8_t i11 = (uint8_t)(x + 1 + (y + 1) * side);
            const uint8_t tris[2][3] = { {i00, i10, i11}, {i00, i11, i01} };
            for (int t = 0; t < 2; ++t)
                {
                *(p++) = 1;
                for (int k = 0; k < 3; ++k)
                    {
                    const uint8_t i = tris[t][k];
                    *(p++) = i;
                    *(p++) = i;
                    *(p++) = i;
                    }
                }
            }
        }
    *(p++) = 0;
    p = align4(p, base);
    (void)triangles;
    return (uint32_t)((p - base) / 4);
}

bool buildGridMesh(const BenchContext& ctx)
{
    const int cells = gridCellsForCaps(ctx);
    if (g_mesh.grid_payload && (g_mesh.grid_cells == cells)) return true;

    const int side = cells + 1;
    const int vertices = side * side;
    const int triangles = cells * cells * 2;
    const size_t bytes = (size_t)vertices * (3 + 3 + 2) * sizeof(int16_t) +
                         (size_t)triangles * 10 + 8;
    g_mesh.grid_payload = static_cast<uint32_t*>(benchAlloc(BenchMemoryKind::AssetCache, bytes, 4));
    if (!g_mesh.grid_payload) return false;

    initMeshletHeader(g_mesh.grid_meshlets[0], 0, (uint8_t)vertices, (uint8_t)vertices, (uint8_t)vertices, 0);
    writeGridMeshlet(g_mesh.grid_payload, cells);

    g_mesh.grid_mesh.id = 2162;
    g_mesh.grid_mesh.nb_meshlets = 1;
    g_mesh.grid_mesh.nb_materials = 2;
    g_mesh.grid_mesh.materials = g_mesh.v2_materials;
    g_mesh.grid_mesh.meshlets = g_mesh.grid_meshlets;
    g_mesh.grid_mesh.payload = g_mesh.grid_payload;
    g_mesh.grid_mesh.bounding_box = tgx::fBox3(-1.0f, 1.0f, -1.0f, 1.0f, -0.35f, 0.35f);
    g_mesh.grid_mesh.name = "synthetic_grid_mesh_v2";
    g_mesh.grid_mesh.material_extras = nullptr;
    g_mesh.grid_cells = cells;
    return true;
}

bool buildLegacyMesh()
{
    if (g_mesh.legacy_vertices) return true;

    const int cells = LEGACY_GRID_CELLS;
    const int side = cells + 1;
    const int vertices = side * side;
    const int triangles = cells * cells * 2;
    const int face_words = triangles * 10 + 1;

    g_mesh.legacy_vertices = static_cast<tgx::fVec3*>(benchAlloc(BenchMemoryKind::Bulk, sizeof(tgx::fVec3) * vertices, 4));
    g_mesh.legacy_normals = static_cast<tgx::fVec3*>(benchAlloc(BenchMemoryKind::Bulk, sizeof(tgx::fVec3) * vertices, 4));
    g_mesh.legacy_texcoords = static_cast<tgx::fVec2*>(benchAlloc(BenchMemoryKind::Bulk, sizeof(tgx::fVec2) * vertices, 4));
    g_mesh.legacy_faces = static_cast<uint16_t*>(benchAlloc(BenchMemoryKind::Bulk, sizeof(uint16_t) * face_words, 2));
    if ((!g_mesh.legacy_vertices) || (!g_mesh.legacy_normals) || (!g_mesh.legacy_texcoords) || (!g_mesh.legacy_faces))
        {
        return false;
        }

    for (int y = 0; y <= cells; ++y)
        {
        const float fy01 = (float)y / (float)cells;
        const float yy = fy01 * 2.0f - 1.0f;
        for (int x = 0; x <= cells; ++x)
            {
            const float fx01 = (float)x / (float)cells;
            const float xx = fx01 * 2.0f - 1.0f;
            const int k = x + y * side;
            g_mesh.legacy_vertices[k] = tgx::fVec3{xx, yy, surfaceHeight(xx, yy)};
            g_mesh.legacy_normals[k] = surfaceNormal(xx, yy);
            g_mesh.legacy_texcoords[k] = tgx::fVec2{fx01 * 2.0f, fy01 * 1.6f};
            }
        }

    int f = 0;
    for (int y = 0; y < cells; ++y)
        {
        for (int x = 0; x < cells; ++x)
            {
            const uint16_t i00 = (uint16_t)(x + y * side);
            const uint16_t i10 = (uint16_t)(x + 1 + y * side);
            const uint16_t i01 = (uint16_t)(x + (y + 1) * side);
            const uint16_t i11 = (uint16_t)(x + 1 + (y + 1) * side);
            const uint16_t tris[2][3] = { {i00, i10, i11}, {i00, i11, i01} };
            for (int t = 0; t < 2; ++t)
                {
                g_mesh.legacy_faces[f++] = 1;
                for (int k = 0; k < 3; ++k)
                    {
                    const uint16_t i = tris[t][k];
                    g_mesh.legacy_faces[f++] = i;
                    g_mesh.legacy_faces[f++] = i;
                    g_mesh.legacy_faces[f++] = i;
                    }
                }
            }
        }
    g_mesh.legacy_faces[f++] = 0;

    g_mesh.legacy_mesh.id = 1;
    g_mesh.legacy_mesh.nb_vertices = (uint16_t)vertices;
    g_mesh.legacy_mesh.nb_texcoords = (uint16_t)vertices;
    g_mesh.legacy_mesh.nb_normals = (uint16_t)vertices;
    g_mesh.legacy_mesh.nb_faces = (uint16_t)triangles;
    g_mesh.legacy_mesh.len_face = (uint16_t)f;
    g_mesh.legacy_mesh.vertice = g_mesh.legacy_vertices;
    g_mesh.legacy_mesh.texcoord = g_mesh.legacy_texcoords;
    g_mesh.legacy_mesh.normal = g_mesh.legacy_normals;
    g_mesh.legacy_mesh.face = g_mesh.legacy_faces;
    g_mesh.legacy_mesh.texture = &g_mesh.textures.cool_texture;
    g_mesh.legacy_mesh.color = tgx::RGBf{0.68f, 0.72f, 0.78f};
    g_mesh.legacy_mesh.ambiant_strength = 0.18f;
    g_mesh.legacy_mesh.diffuse_strength = 0.86f;
    g_mesh.legacy_mesh.specular_strength = 0.28f;
    g_mesh.legacy_mesh.specular_exponent = 24;
    g_mesh.legacy_mesh.next = nullptr;
    g_mesh.legacy_mesh.bounding_box = tgx::fBox3(-1.0f, 1.0f, -1.0f, 1.0f, -0.35f, 0.35f);
    g_mesh.legacy_mesh.name = "synthetic_legacy_mesh";
    return true;
}

bool ensureMeshes(BenchContext& ctx)
{
    if (g_mesh.ready) return true;
    if (!ensureTextures()) return false;
    setupMaterials();
    if (!buildLowMesh()) return false;
    if (!buildGridMesh(ctx)) return false;
    if (!buildLegacyMesh()) return false;
    g_mesh.ready = true;
    return true;
}

void setBaseScene(BenchContext& ctx, tgx::Shader shaders)
{
    ctx.renderer.setShaders(shaders);
    ctx.renderer.setTextureQuality((shaders & tgx::SHADER_TEXTURE_BILINEAR) ? tgx::SHADER_TEXTURE_BILINEAR : tgx::SHADER_TEXTURE_NEAREST);
    ctx.renderer.setTextureWrappingMode((shaders & tgx::SHADER_TEXTURE_CLAMP) ? tgx::SHADER_TEXTURE_CLAMP : tgx::SHADER_TEXTURE_WRAP_POW2);
    ctx.renderer.setPerspective(46.0f, ((float)ctx.width) / ((float)ctx.height), 1.0f, 90.0f);
    ctx.renderer.setLookAt(tgx::fVec3{0.0f, 0.0f, 0.0f}, tgx::fVec3{0.0f, 0.0f, -1.0f}, tgx::fVec3{0.0f, 1.0f, 0.0f});
    ctx.renderer.setCulling(0);
    ctx.renderer.setLight(tgx::fVec3{0.34f, -0.50f, -1.0f},
                          tgx::RGBf{0.12f, 0.12f, 0.13f},
                          tgx::RGBf{0.86f, 0.82f, 0.72f},
                          tgx::RGBf{0.35f, 0.34f, 0.30f});
    ctx.renderer.setMaterial(tgx::RGBf{0.72f, 0.74f, 0.78f}, 0.18f, 0.86f, 0.30f, 24);
}

bool setupCommon(BenchContext& ctx, tgx::Shader shaders)
{
    if (!ensureMeshes(ctx)) return false;
    setBaseScene(ctx, shaders);
    return true;
}

void setModel(BenchContext& ctx, uint32_t frame_index, const tgx::fVec3& pos,
              const tgx::fVec3& scale, float angle)
{
    const float phase = frame_index ? visualPhaseDeg(frame_index) : 0.0f;
    ctx.renderer.setModelPosScaleRot(pos, scale, angle + phase * 0.35f,
                                     tgx::fVec3{0.25f, 0.90f, 0.35f});
}

bool setupV2FlatLow(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER |
                            tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE);
}

void renderV2FlatLow(BenchContext& ctx, uint32_t frame_index)
{
    setModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.7f}, tgx::fVec3{1.55f, 1.28f, 1.55f}, -12.0f);
    ctx.renderer.drawMesh(&g_mesh.low_mesh, true);
}

bool setupV2GouraudOverride(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER |
                            tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE);
}

void renderV2GouraudOverride(BenchContext& ctx, uint32_t frame_index)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.66f, 0.70f, 0.84f}, 0.16f, 0.86f, 0.48f, 20);
    setModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.7f}, tgx::fVec3{1.55f, 1.28f, 1.55f}, 18.0f);
    ctx.renderer.drawMesh(&g_mesh.low_mesh, false);
}

bool setupV2GridGouraud(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER |
                            tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE);
}

void renderV2GridGouraud(BenchContext& ctx, uint32_t frame_index)
{
    setModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.6f}, tgx::fVec3{1.65f, 1.18f, 1.65f}, 16.0f);
    ctx.renderer.drawMesh(&g_mesh.grid_mesh, true);
}

bool setupV2GridFlatNoZ(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_NOZBUFFER |
                            tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE);
}

void renderV2GridFlatNoZ(BenchContext& ctx, uint32_t frame_index)
{
    setModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.7f}, tgx::fVec3{1.70f, 1.20f, 1.70f}, -20.0f);
    ctx.renderer.drawMesh(&g_mesh.grid_mesh, true);
}

bool setupV2TextureNearest(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER |
                            tgx::SHADER_GOURAUD | tgx::SHADER_TEXTURE |
                            tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
}

void renderV2TextureGrid(BenchContext& ctx, uint32_t frame_index)
{
    setModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.7f}, tgx::fVec3{1.55f, 1.12f, 1.55f}, 20.0f);
    ctx.renderer.drawMesh(&g_mesh.grid_mesh, true);
}

bool setupV2TextureBilinear(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER |
                            tgx::SHADER_GOURAUD | tgx::SHADER_TEXTURE |
                            tgx::SHADER_TEXTURE_BILINEAR | tgx::SHADER_TEXTURE_CLAMP);
}

bool setupV2TextureAffine(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER |
                            tgx::SHADER_GOURAUD | tgx::SHADER_TEXTURE_AFFINE |
                            tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
}

bool setupV2MultiMaterial(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER |
                            tgx::SHADER_GOURAUD | tgx::SHADER_TEXTURE |
                            tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
}

void renderV2MultiMaterial(BenchContext& ctx, uint32_t frame_index)
{
    setModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.6f}, tgx::fVec3{1.70f, 1.30f, 1.70f}, -10.0f);
    ctx.renderer.drawMesh(&g_mesh.low_mesh, true);
}

bool setupV2NearClip(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER |
                            tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE);
}

void renderV2NearClip(BenchContext& ctx, uint32_t frame_index)
{
    const float phase = frame_index ? visualPhaseDeg(frame_index) : 0.0f;
    const float z = -1.22f + (frame_index ? 0.08f * benchFastSinDeg(phase) : 0.0f);
    setModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, z}, tgx::fVec3{1.55f, 1.20f, 1.55f}, 12.0f);
    ctx.renderer.drawMesh(&g_mesh.grid_mesh, true);
}

bool setupV2PartialViewport(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER |
                            tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE);
}

void renderV2PartialViewport(BenchContext& ctx, uint32_t frame_index)
{
    const float phase = frame_index ? visualPhaseDeg(frame_index) : 0.0f;
    const float x = 2.45f + (frame_index ? 0.20f * benchFastSinDeg(phase) : 0.0f);
    setModel(ctx, frame_index, tgx::fVec3{x, 0.0f, -5.4f}, tgx::fVec3{1.80f, 1.25f, 1.80f}, 18.0f);
    ctx.renderer.drawMesh(&g_mesh.grid_mesh, true);
}

bool setupV2FarGouraud(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER |
                            tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE);
}

void renderV2FarGouraud(BenchContext& ctx, uint32_t frame_index)
{
    const float phase = frame_index ? visualPhaseDeg(frame_index) : 0.0f;
    const float z = -74.0f + (frame_index ? 3.0f * benchFastSinDeg(phase) : 0.0f);
    setModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, z}, tgx::fVec3{16.0f, 11.0f, 16.0f}, -12.0f);
    ctx.renderer.drawMesh(&g_mesh.low_mesh, true);
}

bool setupV2Discard(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER |
                            tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE);
}

void renderV2Discard(BenchContext& ctx, uint32_t)
{
    ctx.renderer.setModelPosScaleRot(tgx::fVec3{8.0f, 0.0f, -5.0f}, tgx::fVec3{1.5f, 1.5f, 1.5f}, 0.0f);
    ctx.renderer.drawMesh(&g_mesh.low_mesh, true);
}

bool setupV2Culling(BenchContext& ctx)
{
    if (!setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER |
                          tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE)) return false;
    ctx.renderer.setCulling(1);
    return true;
}

void renderV2Culling(BenchContext& ctx, uint32_t frame_index)
{
    setModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.6f}, tgx::fVec3{1.65f, 1.22f, 1.65f}, 22.0f);
    ctx.renderer.drawMesh(&g_mesh.grid_mesh, true);
}

bool setupLegacyGouraud(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER |
                            tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE);
}

void renderLegacyGouraud(BenchContext& ctx, uint32_t frame_index)
{
    setModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.7f}, tgx::fVec3{1.55f, 1.15f, 1.55f}, -18.0f);
    ctx.renderer.drawMesh(&g_mesh.legacy_mesh, true, true);
}

bool setupLegacyTextured(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER |
                            tgx::SHADER_GOURAUD | tgx::SHADER_TEXTURE |
                            tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
}

void renderLegacyTextured(BenchContext& ctx, uint32_t frame_index)
{
    setModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.7f}, tgx::fVec3{1.55f, 1.15f, 1.55f}, 16.0f);
    ctx.renderer.drawMesh(&g_mesh.legacy_mesh, true, true);
}

const BenchTest MESH_TESTS[] = {
    {
        "mesh.v2.lowpoly.flat.material",
        "Mesh3Dv2 low-poly flat shading with mesh materials",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupV2FlatLow,
        renderV2FlatLow,
        nullptr
    },
    {
        "mesh.v2.lowpoly.gouraud.override",
        "Mesh3Dv2 low-poly Gouraud using renderer material",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupV2GouraudOverride,
        renderV2GouraudOverride,
        nullptr
    },
    {
        "mesh.v2.grid.gouraud.small_tri",
        "Mesh3Dv2 small-triangle grid Gouraud",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupV2GridGouraud,
        renderV2GridGouraud,
        nullptr
    },
    {
        "mesh.v2.grid.flat.noz",
        "Mesh3Dv2 small-triangle grid flat without zbuffer",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_NOZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupV2GridFlatNoZ,
        renderV2GridFlatNoZ,
        nullptr
    },
    {
        "mesh.v2.texture.nearest.wrap",
        "Mesh3Dv2 textured nearest wrap",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupV2TextureNearest,
        renderV2TextureGrid,
        nullptr
    },
    {
        "mesh.v2.texture.bilinear.clamp",
        "Mesh3Dv2 textured bilinear clamp",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_BILINEAR | tgx::SHADER_TEXTURE_CLAMP,
        0, 0, 0,
        setupV2TextureBilinear,
        renderV2TextureGrid,
        nullptr
    },
    {
        "mesh.v2.texture.affine",
        "Mesh3Dv2 affine textured small-triangle grid",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
            tgx::SHADER_TEXTURE_AFFINE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupV2TextureAffine,
        renderV2TextureGrid,
        nullptr
    },
    {
        "mesh.v2.multi_material.textured",
        "Mesh3Dv2 two meshlets with two textured materials",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupV2MultiMaterial,
        renderV2MultiMaterial,
        nullptr
    },
    {
        "mesh.v2.near_clip.gouraud",
        "Mesh3Dv2 Gouraud mesh crossing near plane",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupV2NearClip,
        renderV2NearClip,
        nullptr
    },
    {
        "mesh.v2.partial_viewport.gouraud",
        "Mesh3Dv2 partially outside the viewport",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupV2PartialViewport,
        renderV2PartialViewport,
        nullptr
    },
    {
        "mesh.v2.far.gouraud",
        "Mesh3Dv2 Gouraud mesh close to the far plane",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupV2FarGouraud,
        renderV2FarGouraud,
        nullptr
    },
    {
        "mesh.v2.offscreen.discard",
        "Mesh3Dv2 whole-mesh bbox discard",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_FLAT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupV2Discard,
        renderV2Discard,
        nullptr
    },
    {
        "mesh.v2.culling.gouraud",
        "Mesh3Dv2 Gouraud with face culling enabled",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupV2Culling,
        renderV2Culling,
        nullptr
    },
    {
        "mesh.legacy.gouraud",
        "legacy Mesh3D Gouraud small-triangle grid",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupLegacyGouraud,
        renderLegacyGouraud,
        nullptr
    },
    {
        "mesh.legacy.textured",
        "legacy Mesh3D textured Gouraud grid",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupLegacyTextured,
        renderLegacyTextured,
        nullptr
    }
};

const BenchModule MESH_MODULE = {
    "mesh",
    "Mesh3D and Mesh3Dv2 drawing",
    MESH_TESTS,
    sizeof(MESH_TESTS) / sizeof(MESH_TESTS[0])
};

} // namespace

const BenchModule& getMeshBenchModule()
{
    return MESH_MODULE;
}

} // namespace tgxbench
