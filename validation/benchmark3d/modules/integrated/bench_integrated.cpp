#include "bench_integrated.h"

namespace tgxbench
{

namespace
{

constexpr int INTEGRATED_TEX_W = 48;
constexpr int INTEGRATED_TEX_H = 48;
constexpr int SKY_TEX_W = 32;
constexpr int SKY_TEX_H = 32;

struct IntegratedState
    {
    tgx::RGB565* checker_pixels;
    tgx::RGB565* object_pixels;
    tgx::RGB565* sky_pixels[6];
    tgx::Image<tgx::RGB565> checker_texture;
    tgx::Image<tgx::RGB565> object_texture;
    tgx::Image<tgx::RGB565> sky_faces[6];

    int grid_n;
    int vertex_count;
    int triangle_count;
    tgx::fVec3* vertices;
    tgx::fVec3* normals;
    tgx::fVec2* texcoords;
    uint16_t* indices;

    tgx::Mesh3D<tgx::RGB565> mesh;
    uint16_t* mesh_faces;
    bool ready;
    };

static IntegratedState g_scene = {};

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
    if (ctx.caps.power_class >= 7) return 12;
    if (ctx.caps.power_class >= 5) return 10;
    if (ctx.caps.power_class >= 4) return 9;
    return 6;
}

tgx::RGB565 checkerColor(int x, int y)
{
    const bool checker = (((x >> 3) ^ (y >> 3)) & 1) != 0;
    const bool line = ((x & 15) == 0) || ((y & 15) == 0);
    if (line) return tgx::RGB565(31, 52, 30);
    return checker ? tgx::RGB565(27, 30, 8) : tgx::RGB565(3, 28, 30);
}

tgx::RGB565 objectColor(int x, int y)
{
    const bool tile = (((x >> 4) ^ (y >> 4)) & 1) != 0;
    const int diag = (x * 3 + y * 5) & 31;
    int r = tile ? 28 : 7;
    int g = tile ? 18 : 42;
    int b = tile ? 8 : 25;
    if (diag < 7)
        {
        r += tile ? 3 : 12;
        g += tile ? 22 : 18;
        b += tile ? 4 : 6;
        }
    return tgx::RGB565(r, g, b);
}

tgx::RGB565 skyColor(int face, int x, int y)
{
    const int v = (y * 255) / (SKY_TEX_H - 1);
    const int wave = (int)(26.0f * tgx::tgx_fast_sin_deg_clamped(wrapDeg180((float)(x * 9 + y * 5 + face * 27))));
    const bool star = (((x * 37 + y * 101 + face * 17) & 255) > 251) && (face != 3);
    int r = 10 + face * 6 + v / 10;
    int g = 24 + face * 4 + v / 5;
    int b = 82 + face * 11 + wave - v / 10;
    if (face == 2) { r += 20; g += 22; b += 16; }
    if (face == 3) { r = 24 + v / 5; g = 22 + v / 6; b = 28 + v / 7; }
    if (star) { r = 245; g = 238; b = 210; }
    return rgb888(r, g, b);
}

float surfaceWave(float x, float y)
{
    return 0.18f * tgx::tgx_fast_sin_deg_clamped(wrapDeg180(x * 210.0f)) *
                   tgx::tgx_fast_cos_deg_clamped(wrapDeg180(y * 170.0f)) +
           0.07f * tgx::tgx_fast_sin_deg_clamped(wrapDeg180((x + y) * 150.0f));
}

tgx::fVec3 surfaceNormal(float x, float y)
{
    const float e = 0.025f;
    const float dzdx = (surfaceWave(x + e, y) - surfaceWave(x - e, y)) / (2.0f * e);
    const float dzdy = (surfaceWave(x, y + e) - surfaceWave(x, y - e)) / (2.0f * e);
    tgx::fVec3 n{-dzdx, -dzdy, 1.0f};
    n.normalize_fast();
    return n;
}

bool buildTextures()
{
    if (g_scene.checker_pixels) return true;

    g_scene.checker_pixels = static_cast<tgx::RGB565*>(
        benchAlloc(BenchMemoryKind::AssetCache, sizeof(tgx::RGB565) * INTEGRATED_TEX_W * INTEGRATED_TEX_H, 4));
    g_scene.object_pixels = static_cast<tgx::RGB565*>(
        benchAlloc(BenchMemoryKind::AssetCache, sizeof(tgx::RGB565) * INTEGRATED_TEX_W * INTEGRATED_TEX_H, 4));
    if ((!g_scene.checker_pixels) || (!g_scene.object_pixels)) return false;

    for (int y = 0; y < INTEGRATED_TEX_H; ++y)
        {
        for (int x = 0; x < INTEGRATED_TEX_W; ++x)
            {
            g_scene.checker_pixels[x + y * INTEGRATED_TEX_W] = checkerColor(x, y);
            g_scene.object_pixels[x + y * INTEGRATED_TEX_W] = objectColor(x, y);
            }
        }
    g_scene.checker_texture.set(g_scene.checker_pixels, INTEGRATED_TEX_W, INTEGRATED_TEX_H);
    g_scene.object_texture.set(g_scene.object_pixels, INTEGRATED_TEX_W, INTEGRATED_TEX_H);

    for (int face = 0; face < 6; ++face)
        {
        g_scene.sky_pixels[face] = static_cast<tgx::RGB565*>(
            benchAlloc(BenchMemoryKind::AssetCache, sizeof(tgx::RGB565) * SKY_TEX_W * SKY_TEX_H, 4));
        if (!g_scene.sky_pixels[face]) return false;
        for (int y = 0; y < SKY_TEX_H; ++y)
            {
            for (int x = 0; x < SKY_TEX_W; ++x)
                {
                g_scene.sky_pixels[face][x + y * SKY_TEX_W] = skyColor(face, x, y);
                }
            }
        g_scene.sky_faces[face].set(g_scene.sky_pixels[face], SKY_TEX_W, SKY_TEX_H);
        }

    return true;
}

bool buildGeometry(BenchContext& ctx)
{
    const int grid_n = gridNForCaps(ctx);
    if ((g_scene.vertices) && (g_scene.grid_n == grid_n)) return true;

    g_scene.grid_n = grid_n;
    g_scene.vertex_count = (grid_n + 1) * (grid_n + 1);
    g_scene.triangle_count = grid_n * grid_n * 2;

    g_scene.vertices = static_cast<tgx::fVec3*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(tgx::fVec3) * g_scene.vertex_count, 4));
    g_scene.normals = static_cast<tgx::fVec3*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(tgx::fVec3) * g_scene.vertex_count, 4));
    g_scene.texcoords = static_cast<tgx::fVec2*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(tgx::fVec2) * g_scene.vertex_count, 4));
    g_scene.indices = static_cast<uint16_t*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(uint16_t) * g_scene.triangle_count * 3, 2));
    g_scene.mesh_faces = static_cast<uint16_t*>(
        benchAlloc(BenchMemoryKind::Bulk, sizeof(uint16_t) * (g_scene.triangle_count * 10 + 1), 2));
    if ((!g_scene.vertices) || (!g_scene.normals) || (!g_scene.texcoords) ||
        (!g_scene.indices) || (!g_scene.mesh_faces)) return false;

    for (int y = 0; y <= grid_n; ++y)
        {
        const float fy01 = (float)y / (float)grid_n;
        const float yy = fy01 * 2.0f - 1.0f;
        for (int x = 0; x <= grid_n; ++x)
            {
            const float fx01 = (float)x / (float)grid_n;
            const float xx = fx01 * 2.0f - 1.0f;
            const int k = x + y * (grid_n + 1);
            g_scene.vertices[k] = tgx::fVec3{xx * 2.15f, yy * 1.55f, surfaceWave(xx, yy)};
            g_scene.normals[k] = surfaceNormal(xx, yy);
            g_scene.texcoords[k] = tgx::fVec2{fx01 * 3.0f, fy01 * 2.0f};
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
                g_scene.indices[t++] = tris[tri][0];
                g_scene.indices[t++] = tris[tri][1];
                g_scene.indices[t++] = tris[tri][2];

                g_scene.mesh_faces[f++] = 1;
                for (int k = 0; k < 3; ++k)
                    {
                    const uint16_t i = tris[tri][k];
                    g_scene.mesh_faces[f++] = i;
                    g_scene.mesh_faces[f++] = i;
                    g_scene.mesh_faces[f++] = i;
                    }
                }
            }
        }
    g_scene.mesh_faces[f++] = 0;

    g_scene.mesh.id = 1;
    g_scene.mesh.nb_vertices = (uint16_t)g_scene.vertex_count;
    g_scene.mesh.nb_texcoords = (uint16_t)g_scene.vertex_count;
    g_scene.mesh.nb_normals = (uint16_t)g_scene.vertex_count;
    g_scene.mesh.nb_faces = (uint16_t)g_scene.triangle_count;
    g_scene.mesh.len_face = (uint16_t)f;
    g_scene.mesh.vertice = g_scene.vertices;
    g_scene.mesh.texcoord = g_scene.texcoords;
    g_scene.mesh.normal = g_scene.normals;
    g_scene.mesh.face = g_scene.mesh_faces;
    g_scene.mesh.texture = &g_scene.object_texture;
    g_scene.mesh.color = tgx::RGBf{1.0f, 1.0f, 1.0f};
    g_scene.mesh.ambiant_strength = 0.14f;
    g_scene.mesh.diffuse_strength = 0.88f;
    g_scene.mesh.specular_strength = 0.26f;
    g_scene.mesh.specular_exponent = 24;
    g_scene.mesh.next = nullptr;
    g_scene.mesh.bounding_box = tgx::fBox3(-2.2f, 2.2f, -1.6f, 1.6f, -0.35f, 0.35f);
    g_scene.mesh.name = "integrated_synthetic_mesh";
    return true;
}

bool ensureScene(BenchContext& ctx)
{
    if (g_scene.ready && (g_scene.grid_n == gridNForCaps(ctx))) return true;
    if (!buildTextures()) return false;
    if (!buildGeometry(ctx)) return false;
    g_scene.ready = true;
    return true;
}

void setCamera(BenchContext& ctx, uint32_t frame_index, float yaw_offset, float distance)
{
    const float phase = frame_index ? visualPhaseDeg(frame_index) : 0.0f;
    const float yaw = yaw_offset + 0.45f * phase;
    const tgx::fVec3 eye{
        distance * tgx::tgx_fast_sin_deg_clamped(yaw),
        1.1f + 0.15f * tgx::tgx_fast_sin_deg_clamped(phase * 0.7f),
        distance * tgx::tgx_fast_cos_deg_clamped(yaw)
    };
    const tgx::fVec3 target{0.0f, 0.0f, -5.4f};
    ctx.renderer.setLookAt(eye, target, tgx::fVec3{0.0f, 1.0f, 0.0f});
}

void resetCommon(BenchContext& ctx, tgx::Shader shaders)
{
    ctx.renderer.setShaders(shaders);
    ctx.renderer.setTextureQuality(tgx::SHADER_TEXTURE_NEAREST);
    ctx.renderer.setTextureWrappingMode(tgx::SHADER_TEXTURE_WRAP_POW2);
    ctx.renderer.setPerspective(48.0f, ((float)ctx.width) / ((float)ctx.height), 1.0f, 100.0f);
    ctx.renderer.setLookAt(tgx::fVec3{0.0f, 0.0f, 0.0f}, tgx::fVec3{0.0f, 0.0f, -1.0f}, tgx::fVec3{0.0f, 1.0f, 0.0f});
    ctx.renderer.setCulling(0);
    ctx.renderer.setDirectionalLightCount(1);
    ctx.renderer.setDirectionalLightAmbiant(tgx::RGBf{0.06f, 0.06f, 0.07f});
    ctx.renderer.setDirectionalLight(0, tgx::fVec3{0.30f, -0.55f, -1.0f},
                                     tgx::RGBf{0.20f, 0.20f, 0.22f},
                                     tgx::RGBf{0.16f, 0.15f, 0.13f});
    ctx.renderer.setSpotLightCount(0);
    ctx.renderer.setMaterial(tgx::RGBf{0.78f, 0.78f, 0.76f}, 0.16f, 0.86f, 0.20f, 24);
}

bool setupTexturedMeshPoint(BenchContext& ctx)
{
    if (!ensureScene(ctx)) return false;
    resetCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
                     tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
    ctx.renderer.setDirectionalLightAmbiant(tgx::RGBf{0.04f, 0.04f, 0.05f});
    ctx.renderer.setSpotLight(0, tgx::fVec3{-1.1f, 0.9f, -3.8f}, 4.1f,
                              tgx::RGBf{1.0f, 0.55f, 0.25f},
                              tgx::RGBf{0.55f, 0.50f, 0.42f});
    ctx.renderer.setSpotLightCount(1);
    return true;
}

void renderTexturedMeshPoint(BenchContext& ctx, uint32_t frame_index)
{
    const float phase = visualPhaseDeg(frame_index);
    ctx.renderer.setSpotLightPosition(0, tgx::fVec3{1.45f * tgx::tgx_fast_sin_deg_clamped(phase),
                                                    0.95f * tgx::tgx_fast_cos_deg_clamped(phase * 0.7f),
                                                    -3.65f});
    setCamera(ctx, frame_index, 0.0f, 0.0f);
    ctx.renderer.setModelPosScaleRot(tgx::fVec3{0.0f, 0.0f, -5.7f}, tgx::fVec3{0.92f, 0.92f, 0.92f},
                                     15.0f + phase * 0.18f, tgx::fVec3{0.18f, 0.92f, 0.30f});
    ctx.renderer.drawMesh(&g_scene.mesh, true, true);
}

bool setupCheckerSpotSweep(BenchContext& ctx)
{
    if (!ensureScene(ctx)) return false;
    resetCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
                     tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
    ctx.renderer.setDirectionalLightAmbiant(tgx::RGBf{0.06f, 0.06f, 0.07f});
    ctx.renderer.setDirectionalLight(0, tgx::fVec3{0.0f, -1.0f, -0.2f},
                                     tgx::RGBf{0.14f, 0.14f, 0.15f},
                                     tgx::RGBf{0.0f, 0.0f, 0.0f});
    ctx.renderer.setSpotLight(0, tgx::fVec3{0.0f, 1.65f, -3.55f},
                              tgx::fVec3{0.0f, -0.70f, -1.0f},
                              7.2f, 32.0f, 16.0f,
                              tgx::RGBf{1.0f, 0.90f, 0.48f},
                              tgx::RGBf{0.45f, 0.40f, 0.30f});
    ctx.renderer.setSpotLightCount(1);
    ctx.renderer.setMaterial(tgx::RGBf{1.0f, 1.0f, 1.0f}, 0.22f, 0.95f, 0.14f, 20);
    return true;
}

void renderCheckerSpotSweep(BenchContext& ctx, uint32_t frame_index)
{
    const float phase = visualPhaseDeg(frame_index);
    tgx::fVec3 dir{
        0.55f * tgx::tgx_fast_sin_deg_clamped(phase),
        -0.70f,
        -1.0f + 0.26f * tgx::tgx_fast_cos_deg_clamped(phase * 1.4f)
    };
    ctx.renderer.setSpotLightDirection(0, dir);
    ctx.renderer.setLookAt(tgx::fVec3{0.0f, 1.25f, 0.20f},
                           tgx::fVec3{0.0f, -0.72f, -5.35f},
                           tgx::fVec3{0.0f, 1.0f, 0.0f});
    ctx.renderer.setModelPosScaleRot(tgx::fVec3{0.0f, -0.95f, -5.35f}, tgx::fVec3{1.85f, 1.0f, 1.85f},
                                     68.0f, tgx::fVec3{1.0f, 0.0f, 0.0f});
    ctx.renderer.drawTriangles(g_scene.triangle_count,
                               g_scene.indices, g_scene.vertices,
                               g_scene.indices, g_scene.normals,
                               g_scene.indices, g_scene.texcoords,
                               &g_scene.checker_texture);
}

bool setupRoomTwoPoints(BenchContext& ctx)
{
    if (!ensureScene(ctx)) return false;
    resetCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE);
    ctx.renderer.setDirectionalLightAmbiant(tgx::RGBf{0.04f, 0.04f, 0.05f});
    ctx.renderer.setDirectionalLight(0, tgx::fVec3{0.20f, -0.60f, -1.0f},
                                     tgx::RGBf{0.12f, 0.12f, 0.13f},
                                     tgx::RGBf{0.0f, 0.0f, 0.0f});
    ctx.renderer.setSpotLight(0, tgx::fVec3{-1.7f, 0.9f, -3.7f}, 3.8f, tgx::RGBf{1.0f, 0.25f, 0.12f});
    ctx.renderer.setSpotLight(1, tgx::fVec3{1.7f, -0.7f, -4.0f}, 3.6f, tgx::RGBf{0.15f, 0.32f, 1.0f});
    ctx.renderer.setSpotLightCount(2);
    return true;
}

void renderRoomTwoPoints(BenchContext& ctx, uint32_t frame_index)
{
    const float phase = visualPhaseDeg(frame_index);
    ctx.renderer.setSpotLightPosition(0, tgx::fVec3{-1.45f + 0.45f * tgx::tgx_fast_sin_deg_clamped(phase), 0.85f, -3.6f});
    ctx.renderer.setSpotLightPosition(1, tgx::fVec3{1.35f, -0.75f + 0.35f * tgx::tgx_fast_cos_deg_clamped(phase), -4.0f});
    setCamera(ctx, frame_index, 12.0f, 0.0f);

    ctx.renderer.setMaterial(tgx::RGBf{0.78f, 0.78f, 0.74f}, 0.13f, 0.88f, 0.25f, 24);
    ctx.renderer.setModelPosScaleRot(tgx::fVec3{-1.1f, -0.35f, -5.4f}, tgx::fVec3{0.62f, 0.82f, 0.62f},
                                     18.0f + phase * 0.18f, tgx::fVec3{0.2f, 1.0f, 0.3f});
    ctx.renderer.drawCube();

    ctx.renderer.setMaterial(tgx::RGBf{0.46f, 0.70f, 0.90f}, 0.12f, 0.88f, 0.42f, 20);
    ctx.renderer.setModelPosScaleRot(tgx::fVec3{0.8f, 0.1f, -5.9f}, tgx::fVec3{0.62f, 0.62f, 0.62f},
                                     0.0f);
    ctx.renderer.drawSphere(14, 8);

    ctx.renderer.setMaterial(tgx::RGBf{0.86f, 0.58f, 0.32f}, 0.13f, 0.86f, 0.25f, 24);
    ctx.renderer.setModelPosScaleRot(tgx::fVec3{1.8f, -0.45f, -5.0f}, tgx::fVec3{0.42f, 0.78f, 0.42f},
                                     -20.0f + phase * 0.12f, tgx::fVec3{0.1f, 1.0f, 0.2f});
    ctx.renderer.drawTruncatedCone(14, 1.0f, 0.45f, true, true);
}

bool setupSkyboxMixed(BenchContext& ctx)
{
    if (!ensureScene(ctx)) return false;
    resetCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
                     tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
    ctx.renderer.setSpotLight(0, tgx::fVec3{-1.2f, 1.0f, -3.6f}, 4.2f, tgx::RGBf{0.85f, 0.55f, 0.28f});
    ctx.renderer.setSpotLightCount(1);
    return true;
}

void drawSky(BenchContext& ctx, uint32_t frame_index)
{
    ctx.renderer.drawSkyBox(&g_scene.sky_faces[0], &g_scene.sky_faces[1], &g_scene.sky_faces[2],
                            &g_scene.sky_faces[3], &g_scene.sky_faces[4], &g_scene.sky_faces[5],
                            frame_index ? visualPhaseDeg(frame_index) * 0.18f : 0.0f,
                            0.0f, 850.0f,
                            tgx::SHADER_TEXTURE_NEAREST, tgx::SHADER_TEXTURE_WRAP_POW2);
}

void renderSkyboxMixed(BenchContext& ctx, uint32_t frame_index)
{
    const float phase = visualPhaseDeg(frame_index);
    setCamera(ctx, frame_index, -10.0f, 0.0f);
    drawSky(ctx, frame_index);
    ctx.renderer.setSpotLightPosition(0, tgx::fVec3{-1.25f + 0.55f * tgx::tgx_fast_sin_deg_clamped(phase), 0.95f, -3.7f});
    ctx.renderer.setModelPosScaleRot(tgx::fVec3{-0.8f, -0.2f, -5.4f}, tgx::fVec3{0.72f, 0.72f, 0.72f},
                                     18.0f + phase * 0.24f, tgx::fVec3{0.2f, 1.0f, 0.2f});
    ctx.renderer.drawCube(&g_scene.object_texture, &g_scene.object_texture, &g_scene.object_texture,
                          &g_scene.object_texture, &g_scene.object_texture, &g_scene.object_texture);

    ctx.renderer.setModelPosScaleRot(tgx::fVec3{1.0f, 0.0f, -6.0f}, tgx::fVec3{0.55f, 0.55f, 0.55f}, 0.0f);
    ctx.renderer.drawSphere(14, 8, &g_scene.object_texture);
}

bool setupNoZOverlay(BenchContext& ctx)
{
    if (!ensureScene(ctx)) return false;
    resetCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
                     tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
    ctx.renderer.setSpotLight(0, tgx::fVec3{-1.2f, 0.9f, -3.6f}, 3.8f, tgx::RGBf{0.95f, 0.58f, 0.24f});
    ctx.renderer.setSpotLightCount(1);
    return true;
}

void renderNoZOverlay(BenchContext& ctx, uint32_t frame_index)
{
    renderTexturedMeshPoint(ctx, frame_index);
    ctx.renderer.setShaders(tgx::SHADER_PERSPECTIVE | tgx::SHADER_NOZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE);
    ctx.renderer.setMaterial(tgx::RGBf{0.95f, 0.92f, 0.35f}, 1.0f, 0.0f, 0.0f, 16);
    ctx.renderer.setModelPosScaleRot(tgx::fVec3{0.0f, 0.0f, -4.0f}, tgx::fVec3{1.0f, 1.0f, 1.0f}, 0.0f);
    ctx.renderer.drawWireFrameLine(tgx::fVec3{-1.6f, -1.0f, 0.0f}, tgx::fVec3{ 1.6f,  1.0f, 0.0f});
    ctx.renderer.drawWireFrameLine(tgx::fVec3{-1.6f,  1.0f, 0.0f}, tgx::fVec3{ 1.6f, -1.0f, 0.0f});
    ctx.renderer.drawDot(tgx::fVec3{0.0f, 0.0f, 0.0f}, 3, tgx::RGB565(31, 54, 12), 0.9f);
}

const BenchTest INTEGRATED_TESTS[] = {
    {
        "integrated.mesh_point_light.texture",
        "textured mesh with moving point light",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupTexturedMeshPoint,
        renderTexturedMeshPoint,
        nullptr
    },
    {
        "integrated.checker_spotlight.sweep",
        "checker floor with sweeping soft spotlight",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupCheckerSpotSweep,
        renderCheckerSpotSweep,
        nullptr
    },
    {
        "integrated.room.two_point_lights",
        "primitive room with two colored point lights",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupRoomTwoPoints,
        renderRoomTwoPoints,
        nullptr
    },
    {
        "integrated.skybox.mixed_scene",
        "skybox plus textured cube and sphere with local light",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupSkyboxMixed,
        renderSkyboxMixed,
        nullptr
    },
    {
        "integrated.overlay.noz",
        "zbuffered textured mesh with no-zbuffer overlay primitives",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_NOZBUFFER |
            tgx::SHADER_GOURAUD | tgx::SHADER_UNLIT | tgx::SHADER_TEXTURE |
            tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2 | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupNoZOverlay,
        renderNoZOverlay,
        nullptr
    }
};

const BenchModule INTEGRATED_MODULE = {
    "integrated",
    "Integrated realistic scenes",
    INTEGRATED_TESTS,
    sizeof(INTEGRATED_TESTS) / sizeof(INTEGRATED_TESTS[0])
};

} // namespace

const BenchModule& getIntegratedBenchModule()
{
    return INTEGRATED_MODULE;
}

} // namespace tgxbench
