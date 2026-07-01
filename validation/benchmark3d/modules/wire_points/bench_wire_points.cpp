#include "bench_wire_points.h"

#include <stddef.h>
#include <stdint.h>

namespace tgxbench
{

namespace
{

constexpr int WIREFRAME_MESH_FACE_COUNT = 12;

struct WirePointsState
    {
    tgx::fVec3* line_vertices;
    uint16_t* line_indices;
    int line_count;

    tgx::fVec3* surface_vertices;
    uint16_t* triangle_indices;
    uint16_t* quad_indices;
    int surface_side;
    int triangle_count;
    int quad_count;

    tgx::fVec3* strip_vertices;
    uint16_t* strip_indices;
    int strip_index_count;

    tgx::MeshMaterial3Dv2<tgx::RGB565> v2_material;
    tgx::Meshlet3Dv2 v2_meshlet;
    uint32_t* v2_payload;
    tgx::Mesh3Dv2<tgx::RGB565> v2_mesh;

    tgx::fVec3* legacy_vertices;
    uint16_t* legacy_faces;
    tgx::Mesh3D<tgx::RGB565> legacy_mesh;

    tgx::fVec3* point_positions;
    int* color_indices;
    int* opacity_indices;
    int* radius_indices;
    int point_count;
    tgx::RGB565 point_colors[6];
    float point_opacities[4];
    int point_radii[4];

    int built_power_class;
    bool ready;
    };

static WirePointsState g_wire = {};

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

int lineCountForCaps(const BenchContext& ctx)
{
    if (ctx.caps.power_class >= 7) return 72;
    if (ctx.caps.power_class >= 5) return 56;
    if (ctx.caps.power_class >= 4) return 44;
    return 28;
}

int surfaceSideForCaps(const BenchContext& ctx)
{
    if (ctx.caps.power_class >= 7) return 11;
    if (ctx.caps.power_class >= 5) return 10;
    if (ctx.caps.power_class >= 4) return 9;
    return 7;
}

int stripCountForCaps(const BenchContext& ctx)
{
    if (ctx.caps.power_class >= 7) return 42;
    if (ctx.caps.power_class >= 5) return 34;
    if (ctx.caps.power_class >= 4) return 28;
    return 20;
}

int pointCountForCaps(const BenchContext& ctx)
{
    if (ctx.caps.power_class >= 7) return 260;
    if (ctx.caps.power_class >= 5) return 190;
    if (ctx.caps.power_class >= 4) return 150;
    return 90;
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

uint8_t* align4(uint8_t* p, const uint8_t* base)
{
    while ((((uintptr_t)(p - base)) & 3U) != 0U) *(p++) = 0;
    return p;
}

float waveHeight(float x, float y)
{
    return 0.18f * tgx::tgx_fast_sin_deg_clamped(wrapDeg180(220.0f * x + 18.0f)) *
                   tgx::tgx_fast_cos_deg_clamped(wrapDeg180(150.0f * y - 12.0f));
}

void setBaseScene(BenchContext& ctx, tgx::Shader shaders)
{
    ctx.renderer.setShaders(shaders);
    ctx.renderer.setPerspective(50.0f, ((float)ctx.width) / ((float)ctx.height), 1.0f, 80.0f);
    ctx.renderer.setLookAt(tgx::fVec3{0.0f, 0.0f, 0.0f}, tgx::fVec3{0.0f, 0.0f, -1.0f}, tgx::fVec3{0.0f, 1.0f, 0.0f});
    ctx.renderer.setCulling(0);
    ctx.renderer.setMaterial(tgx::RGBf{0.92f, 0.86f, 0.35f}, 1.0f, 0.0f, 0.0f, 16);
}

void setAnimatedModel(BenchContext& ctx, uint32_t frame_index, const tgx::fVec3& pos,
                      const tgx::fVec3& scale, float angle_offset)
{
    const float phase = frame_index ? visualPhaseDeg(frame_index) : 0.0f;
    ctx.renderer.setModelPosScaleRot(pos, scale, angle_offset + phase * 0.42f,
                                     tgx::fVec3{0.30f, 0.92f, 0.22f});
}

bool buildLines(const BenchContext& ctx)
{
    const int count = lineCountForCaps(ctx);
    g_wire.line_vertices = static_cast<tgx::fVec3*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(tgx::fVec3) * count * 2, 4));
    g_wire.line_indices = static_cast<uint16_t*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(uint16_t) * count * 2, 2));
    if ((!g_wire.line_vertices) || (!g_wire.line_indices)) return false;

    for (int i = 0; i < count; ++i)
        {
        const float a = wrapDeg180((float)i * 137.5f);
        const float b = wrapDeg180((float)i * 61.0f + 35.0f);
        const float r0 = 0.35f + 0.010f * (float)(i % 29);
        const float r1 = 1.00f + 0.005f * (float)(i % 17);
        const float x0 = r0 * tgx::tgx_fast_cos_deg_clamped(a);
        const float y0 = r0 * tgx::tgx_fast_sin_deg_clamped(b);
        const float z0 = -0.55f + 1.10f * (float)(i % 11) / 10.0f;
        const float x1 = r1 * tgx::tgx_fast_cos_deg_clamped(a + 72.0f);
        const float y1 = r1 * tgx::tgx_fast_sin_deg_clamped(b - 48.0f);
        const float z1 = -0.55f + 1.10f * (float)((i * 7) % 11) / 10.0f;
        g_wire.line_vertices[i * 2 + 0] = tgx::fVec3{x0, y0, z0};
        g_wire.line_vertices[i * 2 + 1] = tgx::fVec3{x1, y1, z1};
        g_wire.line_indices[i * 2 + 0] = (uint16_t)(i * 2 + 0);
        g_wire.line_indices[i * 2 + 1] = (uint16_t)(i * 2 + 1);
        }

    g_wire.line_count = count;
    return true;
}

bool buildSurface(const BenchContext& ctx)
{
    const int side = surfaceSideForCaps(ctx);
    const int vertices = side * side;
    const int cells = side - 1;
    const int quads = cells * cells;
    const int triangles = quads * 2;

    g_wire.surface_vertices = static_cast<tgx::fVec3*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(tgx::fVec3) * vertices, 4));
    g_wire.triangle_indices = static_cast<uint16_t*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(uint16_t) * triangles * 3, 2));
    g_wire.quad_indices = static_cast<uint16_t*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(uint16_t) * quads * 4, 2));
    if ((!g_wire.surface_vertices) || (!g_wire.triangle_indices) || (!g_wire.quad_indices)) return false;

    for (int y = 0; y < side; ++y)
        {
        const float fy = (float)y / (float)(side - 1);
        const float yy = fy * 2.0f - 1.0f;
        for (int x = 0; x < side; ++x)
            {
            const float fx = (float)x / (float)(side - 1);
            const float xx = fx * 2.0f - 1.0f;
            g_wire.surface_vertices[x + y * side] = tgx::fVec3{xx, yy, waveHeight(xx, yy)};
            }
        }

    int ti = 0;
    int qi = 0;
    for (int y = 0; y < cells; ++y)
        {
        for (int x = 0; x < cells; ++x)
            {
            const uint16_t i00 = (uint16_t)(x + y * side);
            const uint16_t i10 = (uint16_t)(x + 1 + y * side);
            const uint16_t i01 = (uint16_t)(x + (y + 1) * side);
            const uint16_t i11 = (uint16_t)(x + 1 + (y + 1) * side);

            g_wire.triangle_indices[ti++] = i00;
            g_wire.triangle_indices[ti++] = i10;
            g_wire.triangle_indices[ti++] = i11;
            g_wire.triangle_indices[ti++] = i00;
            g_wire.triangle_indices[ti++] = i11;
            g_wire.triangle_indices[ti++] = i01;

            g_wire.quad_indices[qi++] = i00;
            g_wire.quad_indices[qi++] = i10;
            g_wire.quad_indices[qi++] = i11;
            g_wire.quad_indices[qi++] = i01;
            }
        }

    g_wire.surface_side = side;
    g_wire.triangle_count = triangles;
    g_wire.quad_count = quads;
    return true;
}

bool buildStrip(const BenchContext& ctx)
{
    const int count = stripCountForCaps(ctx);
    g_wire.strip_vertices = static_cast<tgx::fVec3*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(tgx::fVec3) * count, 4));
    g_wire.strip_indices = static_cast<uint16_t*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(uint16_t) * count, 2));
    if ((!g_wire.strip_vertices) || (!g_wire.strip_indices)) return false;

    for (int i = 0; i < count; ++i)
        {
        const float t = (float)i / (float)(count - 1);
        const float x = -1.15f + 2.30f * t;
        const float lane = (i & 1) ? 0.32f : -0.32f;
        const float y = lane + 0.16f * tgx::tgx_fast_sin_deg_clamped(wrapDeg180(t * 360.0f * 2.0f));
        const float z = 0.22f * tgx::tgx_fast_cos_deg_clamped(wrapDeg180(t * 360.0f * 1.5f));
        g_wire.strip_vertices[i] = tgx::fVec3{x, y, z};
        g_wire.strip_indices[i] = (uint16_t)i;
        }

    g_wire.strip_index_count = count;
    return true;
}

uint32_t writeWireMeshPayload(uint32_t* payload)
{
    static const tgx::fVec3 vertices[8] = {
        tgx::fVec3{-0.82f, -0.64f, -0.72f},
        tgx::fVec3{ 0.78f, -0.70f, -0.60f},
        tgx::fVec3{ 0.70f,  0.64f, -0.78f},
        tgx::fVec3{-0.76f,  0.72f, -0.66f},
        tgx::fVec3{-0.60f, -0.58f,  0.70f},
        tgx::fVec3{ 0.86f, -0.50f,  0.62f},
        tgx::fVec3{ 0.58f,  0.84f,  0.72f},
        tgx::fVec3{-0.88f,  0.54f,  0.58f}
    };
    static const uint8_t triangles[WIREFRAME_MESH_FACE_COUNT][3] = {
        {0, 1, 2}, {0, 2, 3},
        {4, 6, 5}, {4, 7, 6},
        {0, 4, 5}, {0, 5, 1},
        {1, 5, 6}, {1, 6, 2},
        {2, 6, 7}, {2, 7, 3},
        {3, 7, 4}, {3, 4, 0}
    };

    uint8_t* const base = reinterpret_cast<uint8_t*>(payload);
    uint8_t* p = base;
    int16_t* qv = reinterpret_cast<int16_t*>(p);
    p += 8 * 3 * sizeof(int16_t);

    for (int i = 0; i < 8; ++i)
        {
        qv[i * 3 + 0] = qSnorm(vertices[i].x);
        qv[i * 3 + 1] = qSnorm(vertices[i].y);
        qv[i * 3 + 2] = qSnorm(vertices[i].z);
        }

    for (int t = 0; t < WIREFRAME_MESH_FACE_COUNT; ++t)
        {
        *(p++) = 1;
        *(p++) = triangles[t][0];
        *(p++) = triangles[t][1];
        *(p++) = triangles[t][2];
        }
    *(p++) = 0;
    p = align4(p, base);
    return (uint32_t)((p - base) / 4);
}

bool buildWireMeshes()
{
    static const tgx::fVec3 legacy_vertices[8] = {
        tgx::fVec3{-0.82f, -0.64f, -0.72f},
        tgx::fVec3{ 0.78f, -0.70f, -0.60f},
        tgx::fVec3{ 0.70f,  0.64f, -0.78f},
        tgx::fVec3{-0.76f,  0.72f, -0.66f},
        tgx::fVec3{-0.60f, -0.58f,  0.70f},
        tgx::fVec3{ 0.86f, -0.50f,  0.62f},
        tgx::fVec3{ 0.58f,  0.84f,  0.72f},
        tgx::fVec3{-0.88f,  0.54f,  0.58f}
    };
    static const uint16_t triangles[WIREFRAME_MESH_FACE_COUNT][3] = {
        {0, 1, 2}, {0, 2, 3},
        {4, 6, 5}, {4, 7, 6},
        {0, 4, 5}, {0, 5, 1},
        {1, 5, 6}, {1, 6, 2},
        {2, 6, 7}, {2, 7, 3},
        {3, 7, 4}, {3, 4, 0}
    };

    g_wire.v2_payload = static_cast<uint32_t*>(
        benchAlloc(BenchMemoryKind::AssetCache, 128 * sizeof(uint32_t), 4));
    g_wire.legacy_vertices = static_cast<tgx::fVec3*>(
        benchAlloc(BenchMemoryKind::AssetCache, sizeof(tgx::fVec3) * 8, 4));
    g_wire.legacy_faces = static_cast<uint16_t*>(
        benchAlloc(BenchMemoryKind::AssetCache, sizeof(uint16_t) * (WIREFRAME_MESH_FACE_COUNT * 4 + 1), 2));
    if ((!g_wire.v2_payload) || (!g_wire.legacy_vertices) || (!g_wire.legacy_faces)) return false;

    writeWireMeshPayload(g_wire.v2_payload);

    g_wire.v2_material.texture = nullptr;
    g_wire.v2_material.color = tgx::RGBf{0.92f, 0.88f, 0.72f};
    g_wire.v2_material.ambiant_strength = 1.0f;
    g_wire.v2_material.diffuse_strength = 0.0f;
    g_wire.v2_material.specular_strength = 0.0f;
    g_wire.v2_material.specular_exponent = 1;

    g_wire.v2_meshlet.sphere_center[0] = 0;
    g_wire.v2_meshlet.sphere_center[1] = 0;
    g_wire.v2_meshlet.sphere_center[2] = 0;
    g_wire.v2_meshlet.cone_dir[0] = 0;
    g_wire.v2_meshlet.cone_dir[1] = 0;
    g_wire.v2_meshlet.cone_dir[2] = 0;
    g_wire.v2_meshlet.sphere_radius = 32767;
    g_wire.v2_meshlet.cone_cos = -32767;
    g_wire.v2_meshlet.payload_offset32 = 0;
    g_wire.v2_meshlet.nb_vertices = 8;
    g_wire.v2_meshlet.nb_normals = 0;
    g_wire.v2_meshlet.nb_texcoords = 0;
    g_wire.v2_meshlet.material_index = 0;

    g_wire.v2_mesh.id = 2162;
    g_wire.v2_mesh.nb_meshlets = 1;
    g_wire.v2_mesh.nb_materials = 1;
    g_wire.v2_mesh.materials = &g_wire.v2_material;
    g_wire.v2_mesh.meshlets = &g_wire.v2_meshlet;
    g_wire.v2_mesh.payload = g_wire.v2_payload;
    g_wire.v2_mesh.bounding_box = tgx::fBox3(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
    g_wire.v2_mesh.name = "wire_mesh_v2";
    g_wire.v2_mesh.material_extras = nullptr;

    for (int i = 0; i < 8; ++i) g_wire.legacy_vertices[i] = legacy_vertices[i];
    int f = 0;
    for (int t = 0; t < WIREFRAME_MESH_FACE_COUNT; ++t)
        {
        g_wire.legacy_faces[f++] = 1;
        g_wire.legacy_faces[f++] = triangles[t][0];
        g_wire.legacy_faces[f++] = triangles[t][1];
        g_wire.legacy_faces[f++] = triangles[t][2];
        }
    g_wire.legacy_faces[f++] = 0;

    g_wire.legacy_mesh.id = 1;
    g_wire.legacy_mesh.nb_vertices = 8;
    g_wire.legacy_mesh.nb_texcoords = 0;
    g_wire.legacy_mesh.nb_normals = 0;
    g_wire.legacy_mesh.nb_faces = WIREFRAME_MESH_FACE_COUNT;
    g_wire.legacy_mesh.len_face = (uint16_t)f;
    g_wire.legacy_mesh.vertice = g_wire.legacy_vertices;
    g_wire.legacy_mesh.texcoord = nullptr;
    g_wire.legacy_mesh.normal = nullptr;
    g_wire.legacy_mesh.face = g_wire.legacy_faces;
    g_wire.legacy_mesh.texture = nullptr;
    g_wire.legacy_mesh.color = tgx::RGBf{0.86f, 0.82f, 0.66f};
    g_wire.legacy_mesh.ambiant_strength = 1.0f;
    g_wire.legacy_mesh.diffuse_strength = 0.0f;
    g_wire.legacy_mesh.specular_strength = 0.0f;
    g_wire.legacy_mesh.specular_exponent = 1;
    g_wire.legacy_mesh.next = nullptr;
    g_wire.legacy_mesh.bounding_box = tgx::fBox3(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
    g_wire.legacy_mesh.name = "wire_mesh_legacy";
    return true;
}

bool buildPoints(const BenchContext& ctx)
{
    const int count = pointCountForCaps(ctx);
    g_wire.point_positions = static_cast<tgx::fVec3*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(tgx::fVec3) * count, 4));
    g_wire.color_indices = static_cast<int*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(int) * count, 4));
    g_wire.opacity_indices = static_cast<int*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(int) * count, 4));
    g_wire.radius_indices = static_cast<int*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(int) * count, 4));
    if ((!g_wire.point_positions) || (!g_wire.color_indices) ||
        (!g_wire.opacity_indices) || (!g_wire.radius_indices)) return false;

    g_wire.point_colors[0] = tgx::RGB565(31, 24, 7);
    g_wire.point_colors[1] = tgx::RGB565(6, 50, 31);
    g_wire.point_colors[2] = tgx::RGB565(8, 28, 31);
    g_wire.point_colors[3] = tgx::RGB565(31, 12, 20);
    g_wire.point_colors[4] = tgx::RGB565(24, 44, 8);
    g_wire.point_colors[5] = tgx::RGB565(31, 56, 28);

    g_wire.point_opacities[0] = 0.35f;
    g_wire.point_opacities[1] = 0.55f;
    g_wire.point_opacities[2] = 0.75f;
    g_wire.point_opacities[3] = 0.95f;

    g_wire.point_radii[0] = 1;
    g_wire.point_radii[1] = 2;
    g_wire.point_radii[2] = 3;
    g_wire.point_radii[3] = 4;

    for (int i = 0; i < count; ++i)
        {
        const float a = wrapDeg180((float)i * 137.5f);
        const float b = wrapDeg180((float)i * 47.0f + 20.0f);
        const float ring = 0.35f + 0.80f * (float)(i % 37) / 36.0f;
        const float x = ring * tgx::tgx_fast_cos_deg_clamped(a);
        const float y = 0.72f * tgx::tgx_fast_sin_deg_clamped(b);
        const float z = ring * tgx::tgx_fast_sin_deg_clamped(a) * 0.72f;
        g_wire.point_positions[i] = tgx::fVec3{x, y, z};
        g_wire.color_indices[i] = i % 6;
        g_wire.opacity_indices[i] = (i / 3) % 4;
        g_wire.radius_indices[i] = (i / 5) % 4;
        }

    g_wire.point_count = count;
    return true;
}

bool ensureWireState(BenchContext& ctx)
{
    if (g_wire.ready && (g_wire.built_power_class == ctx.caps.power_class)) return true;
    if (!buildLines(ctx)) return false;
    if (!buildSurface(ctx)) return false;
    if (!buildStrip(ctx)) return false;
    if (!buildWireMeshes()) return false;
    if (!buildPoints(ctx)) return false;
    g_wire.built_power_class = ctx.caps.power_class;
    g_wire.ready = true;
    return true;
}

bool setupWireFast(BenchContext& ctx)
{
    if (!ensureWireState(ctx)) return false;
    setBaseScene(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE);
    return true;
}

bool setupWireNoZ(BenchContext& ctx)
{
    if (!ensureWireState(ctx)) return false;
    setBaseScene(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_NOZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE);
    return true;
}

void renderLineIndividualClipped(BenchContext& ctx, uint32_t frame_index)
{
    const float phase = frame_index ? visualPhaseDeg(frame_index) : 0.0f;
    ctx.renderer.setModelPosScaleRot(tgx::fVec3{0.0f, 0.0f, -2.45f}, tgx::fVec3{1.0f, 1.0f, 1.0f},
                                     phase * 0.12f, tgx::fVec3{0.0f, 1.0f, 0.0f});
    ctx.renderer.drawWireFrameLine(tgx::fVec3{-3.2f, -1.1f, 0.0f}, tgx::fVec3{ 3.2f,  1.1f, 0.0f});
    ctx.renderer.drawWireFrameLine(tgx::fVec3{-2.6f,  1.4f, 0.2f}, tgx::fVec3{ 2.6f, -1.4f, 0.2f});
    ctx.renderer.drawWireFrameLine(tgx::fVec3{-1.6f, -2.5f, 0.0f}, tgx::fVec3{ 0.4f,  2.5f, 0.0f});
    ctx.renderer.drawWireFrameLine(tgx::fVec3{ 1.8f, -2.8f, 0.1f}, tgx::fVec3{-1.4f,  2.8f, 0.1f});
    ctx.renderer.drawWireFrameLine(tgx::fVec3{-0.8f, -0.6f, 1.55f}, tgx::fVec3{0.9f, 0.8f, -0.6f});
    ctx.renderer.drawWireFrameLine(tgx::fVec3{-4.5f,  0.0f, 0.0f}, tgx::fVec3{-3.2f, 1.2f, 0.0f});
}

void renderLinesBatchFast(BenchContext& ctx, uint32_t frame_index)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.90f, 0.78f, 0.26f}, 1.0f, 0.0f, 0.0f, 16);
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.0f}, tgx::fVec3{1.25f, 1.25f, 1.25f}, 12.0f);
    ctx.renderer.drawWireFrameLines(g_wire.line_count, g_wire.line_indices, g_wire.line_vertices);
}

void renderLinesBatchAA(BenchContext& ctx, uint32_t frame_index)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.44f, 0.88f, 0.96f}, 1.0f, 0.0f, 0.0f, 16);
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.0f}, tgx::fVec3{1.25f, 1.25f, 1.25f}, -18.0f);
    ctx.renderer.drawWireFrameLinesAA(g_wire.line_count, g_wire.line_indices, g_wire.line_vertices);
}

void renderLinesBatchThick(BenchContext& ctx, uint32_t frame_index)
{
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.0f}, tgx::fVec3{1.25f, 1.25f, 1.25f}, 18.0f);
    ctx.renderer.drawWireFrameLines(g_wire.line_count, g_wire.line_indices, g_wire.line_vertices,
                                    1.6f, tgx::RGB565(31, 42, 8), 0.82f);
}

void renderTrianglesBatchFast(BenchContext& ctx, uint32_t frame_index)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.94f, 0.58f, 0.24f}, 1.0f, 0.0f, 0.0f, 16);
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.2f}, tgx::fVec3{1.45f, 1.45f, 1.45f}, 28.0f);
    ctx.renderer.drawWireFrameTriangles(g_wire.triangle_count, g_wire.triangle_indices, g_wire.surface_vertices);
}

void renderTriangleStripAA(BenchContext& ctx, uint32_t frame_index)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.42f, 0.92f, 0.58f}, 1.0f, 0.0f, 0.0f, 16);
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.0f}, tgx::fVec3{1.35f, 1.35f, 1.35f}, -16.0f);
    ctx.renderer.drawWireFrameTriangleStripAA(g_wire.strip_index_count, g_wire.strip_indices, g_wire.strip_vertices);
}

void renderQuadsBatchThick(BenchContext& ctx, uint32_t frame_index)
{
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.2f}, tgx::fVec3{1.45f, 1.45f, 1.45f}, -24.0f);
    ctx.renderer.drawWireFrameQuads(g_wire.quad_count, g_wire.quad_indices, g_wire.surface_vertices,
                                    1.45f, tgx::RGB565(8, 54, 30), 0.78f);
}

void renderMeshV2Fast(BenchContext& ctx, uint32_t frame_index)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.90f, 0.86f, 0.34f}, 1.0f, 0.0f, 0.0f, 16);
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.3f}, tgx::fVec3{1.18f, 1.18f, 1.18f}, 20.0f);
    ctx.renderer.drawWireFrameMesh(&g_wire.v2_mesh);
}

void renderMeshV2AA(BenchContext& ctx, uint32_t frame_index)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.64f, 0.86f, 1.0f}, 1.0f, 0.0f, 0.0f, 16);
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.3f}, tgx::fVec3{1.18f, 1.18f, 1.18f}, -24.0f);
    ctx.renderer.drawWireFrameMeshAA(&g_wire.v2_mesh);
}

void renderMeshV2Thick(BenchContext& ctx, uint32_t frame_index)
{
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.3f}, tgx::fVec3{1.18f, 1.18f, 1.18f}, 32.0f);
    ctx.renderer.drawWireFrameMesh(&g_wire.v2_mesh, 1.6f, tgx::RGB565(31, 20, 22), 0.82f);
}

void renderMeshLegacyFast(BenchContext& ctx, uint32_t frame_index)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.80f, 0.96f, 0.58f}, 1.0f, 0.0f, 0.0f, 16);
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.3f}, tgx::fVec3{1.18f, 1.18f, 1.18f}, -18.0f);
    ctx.renderer.drawWireFrameMesh(&g_wire.legacy_mesh, true);
}

void renderPixelsSingle(BenchContext& ctx, uint32_t frame_index)
{
    const float phase = frame_index ? visualPhaseDeg(frame_index) : 0.0f;
    ctx.renderer.setModelPosScaleRot(tgx::fVec3{0.0f, 0.0f, -5.0f}, tgx::fVec3{1.45f, 1.45f, 1.45f},
                                     20.0f + phase * 0.38f, tgx::fVec3{0.22f, 0.94f, 0.28f});
    for (int i = 0; i < g_wire.point_count; ++i)
        {
        ctx.renderer.drawPixel(g_wire.point_positions[i],
                               g_wire.point_colors[g_wire.color_indices[i]],
                               g_wire.point_opacities[g_wire.opacity_indices[i]]);
        }
}

void renderPixelsBatchDefault(BenchContext& ctx, uint32_t frame_index)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.88f, 0.88f, 0.92f}, 1.0f, 0.0f, 0.0f, 16);
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.0f}, tgx::fVec3{1.45f, 1.45f, 1.45f}, 28.0f);
    ctx.renderer.drawPixels(g_wire.point_count, g_wire.point_positions);
}

void renderPixelsBatchPalette(BenchContext& ctx, uint32_t frame_index)
{
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.0f}, tgx::fVec3{1.45f, 1.45f, 1.45f}, -22.0f);
    ctx.renderer.drawPixels(g_wire.point_count, g_wire.point_positions,
                            g_wire.color_indices, g_wire.point_colors,
                            g_wire.opacity_indices, g_wire.point_opacities);
}

void renderDotsSingle(BenchContext& ctx, uint32_t frame_index)
{
    const float phase = frame_index ? visualPhaseDeg(frame_index) : 0.0f;
    ctx.renderer.setModelPosScaleRot(tgx::fVec3{0.0f, 0.0f, -5.0f}, tgx::fVec3{1.38f, 1.38f, 1.38f},
                                     -18.0f + phase * 0.36f, tgx::fVec3{0.34f, 0.88f, 0.26f});
    const int count = g_wire.point_count / 2;
    for (int i = 0; i < count; ++i)
        {
        ctx.renderer.drawDot(g_wire.point_positions[i],
                             g_wire.point_radii[g_wire.radius_indices[i]],
                             g_wire.point_colors[g_wire.color_indices[i]],
                             g_wire.point_opacities[g_wire.opacity_indices[i]]);
        }
}

void renderDotsBatchFixed(BenchContext& ctx, uint32_t frame_index)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.96f, 0.82f, 0.35f}, 1.0f, 0.0f, 0.0f, 16);
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.0f}, tgx::fVec3{1.38f, 1.38f, 1.38f}, 18.0f);
    ctx.renderer.drawDots(g_wire.point_count, g_wire.point_positions, 2);
}

void renderDotsBatchPalette(BenchContext& ctx, uint32_t frame_index)
{
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.0f}, tgx::fVec3{1.38f, 1.38f, 1.38f}, -28.0f);
    ctx.renderer.drawDots(g_wire.point_count, g_wire.point_positions,
                          g_wire.radius_indices, g_wire.point_radii,
                          g_wire.color_indices, g_wire.point_colors,
                          g_wire.opacity_indices, g_wire.point_opacities);
}

const BenchTest WIRE_POINTS_TESTS[] = {
    {
        "wire.line.single.clipped",
        "individual 3D wireframe lines with viewport and near-plane clipping",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupWireFast,
        renderLineIndividualClipped,
        nullptr
    },
    {
        "wire.lines.batch.fast",
        "indexed batch wireframe lines fast path",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupWireFast,
        renderLinesBatchFast,
        nullptr
    },
    {
        "wire.lines.batch.aa",
        "indexed batch wireframe lines antialiased path",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupWireFast,
        renderLinesBatchAA,
        nullptr
    },
    {
        "wire.lines.batch.thick",
        "indexed batch wireframe lines thick blended path",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupWireFast,
        renderLinesBatchThick,
        nullptr
    },
    {
        "wire.triangles.batch.fast",
        "indexed wireframe triangle batch over small triangles",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupWireFast,
        renderTrianglesBatchFast,
        nullptr
    },
    {
        "wire.triangle_strip.aa",
        "wireframe triangle strip antialiased path",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupWireFast,
        renderTriangleStripAA,
        nullptr
    },
    {
        "wire.quads.batch.thick",
        "indexed wireframe quad batch thick blended path",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupWireFast,
        renderQuadsBatchThick,
        nullptr
    },
    {
        "wire.mesh.v2.fast",
        "Mesh3Dv2 wireframe fast path",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupWireFast,
        renderMeshV2Fast,
        nullptr
    },
    {
        "wire.mesh.v2.aa",
        "Mesh3Dv2 wireframe antialiased path",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupWireFast,
        renderMeshV2AA,
        nullptr
    },
    {
        "wire.mesh.v2.thick",
        "Mesh3Dv2 wireframe thick blended path",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupWireFast,
        renderMeshV2Thick,
        nullptr
    },
    {
        "wire.mesh.legacy.fast",
        "legacy Mesh3D wireframe fast path",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupWireFast,
        renderMeshLegacyFast,
        nullptr
    },
    {
        "points.pixel.single.blend",
        "individual projected pixels with color and opacity",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupWireFast,
        renderPixelsSingle,
        nullptr
    },
    {
        "points.pixels.batch.default",
        "batch projected pixels using current renderer color",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupWireFast,
        renderPixelsBatchDefault,
        nullptr
    },
    {
        "points.pixels.batch.palette.noz",
        "batch projected pixels with palette and opacity without zbuffer",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_NOZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupWireNoZ,
        renderPixelsBatchPalette,
        nullptr
    },
    {
        "points.dot.single.blend",
        "individual projected dots with variable radius color and opacity",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupWireFast,
        renderDotsSingle,
        nullptr
    },
    {
        "points.dots.batch.fixed",
        "batch projected dots with fixed radius",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupWireFast,
        renderDotsBatchFixed,
        nullptr
    },
    {
        "points.dots.batch.palette.noz",
        "batch projected dots with palette opacity and radius without zbuffer",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_NOZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupWireNoZ,
        renderDotsBatchPalette,
        nullptr
    }
};

const BenchModule WIRE_POINTS_MODULE = {
    "wire_points",
    "Wireframe projected pixels and dots",
    WIRE_POINTS_TESTS,
    sizeof(WIRE_POINTS_TESTS) / sizeof(WIRE_POINTS_TESTS[0])
};

} // namespace

const BenchModule& getWirePointsBenchModule()
{
    return WIRE_POINTS_MODULE;
}

} // namespace tgxbench
