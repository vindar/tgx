#include "bench_primitives.h"

namespace tgxbench
{

namespace
{

struct PrimitiveTextureState
    {
    tgx::RGB565* side_pixels;
    tgx::RGB565* cap_pixels;
    tgx::Image<tgx::RGB565> side_texture;
    tgx::Image<tgx::RGB565> cap_texture;
    };

static PrimitiveTextureState g_texture = { nullptr, nullptr, tgx::Image<tgx::RGB565>(), tgx::Image<tgx::RGB565>() };

constexpr int PRIM_TEX_W = 64;
constexpr int PRIM_TEX_H = 64;

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

int sectorsLow(const BenchContext& ctx)
{
    if (ctx.caps.power_class >= 8) return 18;
    if (ctx.caps.power_class >= 5) return 16;
    if (ctx.caps.power_class >= 4) return 14;
    return 10;
}

int sectorsHigh(const BenchContext& ctx)
{
    if (ctx.caps.power_class >= 8) return 30;
    if (ctx.caps.power_class >= 5) return 24;
    if (ctx.caps.power_class >= 4) return 20;
    return 14;
}

tgx::RGB565 sideTextureColor(int x, int y)
{
    const bool checker = (((x >> 3) ^ (y >> 3)) & 1) != 0;
    const int band = (y * 7 + x * 2) & 31;
    int r = checker ? 29 : 5;
    int g = checker ? 30 : 48;
    int b = checker ? 5 : 26;
    if (band < 8)
        {
        r = checker ? 31 : 10;
        g = checker ? 44 : 62;
        b = checker ? 7 : 30;
        }
    return tgx::RGB565(r, g, b);
}

tgx::RGB565 capTextureColor(int x, int y)
{
    const int dx = x - PRIM_TEX_W / 2;
    const int dy = y - PRIM_TEX_H / 2;
    const int r2 = dx * dx + dy * dy;
    const bool ring = ((r2 >> 6) & 1) != 0;
    return ring ? tgx::RGB565(8, 54, 30) : tgx::RGB565(28, 16, 7);
}

bool ensureTextures(BenchContext&)
{
    if (g_texture.side_pixels && g_texture.cap_pixels) return true;

    g_texture.side_pixels = static_cast<tgx::RGB565*>(
        benchAlloc(BenchMemoryKind::AssetCache, sizeof(tgx::RGB565) * PRIM_TEX_W * PRIM_TEX_H, 4));
    g_texture.cap_pixels = static_cast<tgx::RGB565*>(
        benchAlloc(BenchMemoryKind::AssetCache, sizeof(tgx::RGB565) * PRIM_TEX_W * PRIM_TEX_H, 4));
    if ((!g_texture.side_pixels) || (!g_texture.cap_pixels))
        {
        return false;
        }

    for (int y = 0; y < PRIM_TEX_H; ++y)
        {
        for (int x = 0; x < PRIM_TEX_W; ++x)
            {
            g_texture.side_pixels[x + y * PRIM_TEX_W] = sideTextureColor(x, y);
            g_texture.cap_pixels[x + y * PRIM_TEX_W] = capTextureColor(x, y);
            }
        }

    g_texture.side_texture.set(g_texture.side_pixels, PRIM_TEX_W, PRIM_TEX_H);
    g_texture.cap_texture.set(g_texture.cap_pixels, PRIM_TEX_W, PRIM_TEX_H);
    return true;
}

void setBaseScene(BenchContext& ctx)
{
    ctx.renderer.setPerspective(48.0f, ((float)ctx.width) / ((float)ctx.height), 1.0f, 80.0f);
    ctx.renderer.setLookAt(tgx::fVec3{0.0f, 0.0f, 0.0f}, tgx::fVec3{0.0f, 0.0f, -1.0f}, tgx::fVec3{0.0f, 1.0f, 0.0f});
    ctx.renderer.setCulling(1);
    ctx.renderer.setLight(tgx::fVec3{0.35f, -0.55f, -1.0f},
                          tgx::RGBf{0.12f, 0.12f, 0.13f},
                          tgx::RGBf{0.82f, 0.78f, 0.68f},
                          tgx::RGBf{0.35f, 0.34f, 0.30f});
    ctx.renderer.setMaterial(tgx::RGBf{0.70f, 0.72f, 0.76f}, 0.20f, 0.86f, 0.35f, 28);
}

void applyTextureMode(BenchContext& ctx, tgx::Shader shaders)
{
    ctx.renderer.setShaders(shaders);
    ctx.renderer.setTextureQuality((shaders & tgx::SHADER_TEXTURE_BILINEAR) ? tgx::SHADER_TEXTURE_BILINEAR : tgx::SHADER_TEXTURE_NEAREST);
    ctx.renderer.setTextureWrappingMode((shaders & tgx::SHADER_TEXTURE_CLAMP) ? tgx::SHADER_TEXTURE_CLAMP : tgx::SHADER_TEXTURE_WRAP_POW2);
}

bool setupCommon(BenchContext& ctx, tgx::Shader shaders)
{
    if (!ensureTextures(ctx)) return false;
    applyTextureMode(ctx, shaders);
    setBaseScene(ctx);
    return true;
}

void setModel(BenchContext& ctx, const tgx::fVec3& pos, const tgx::fVec3& scale,
              float angle, const tgx::fVec3& axis)
{
    ctx.renderer.setModelPosScaleRot(pos, scale, angle, axis);
}

void setAnimatedModel(BenchContext& ctx, uint32_t frame_index, const tgx::fVec3& pos,
                      const tgx::fVec3& scale, float angle_offset)
{
    const float phase = visualPhaseDeg(frame_index);
    const float angle = angle_offset + (frame_index ? phase * 0.35f : 0.0f);
    setModel(ctx, pos, scale, angle, tgx::fVec3{0.25f, 0.90f, 0.35f});
}

bool setupCubeGouraud(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE);
}

void renderCubeGouraud(BenchContext& ctx, uint32_t frame_index)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.88f, 0.50f, 0.28f}, 0.18f, 0.85f, 0.20f, 24);
    setAnimatedModel(ctx, frame_index, tgx::fVec3{-1.05f, 0.0f, -5.6f}, tgx::fVec3{0.85f, 0.85f, 0.85f}, 18.0f);
    ctx.renderer.drawCube();
    ctx.renderer.setMaterial(tgx::RGBf{0.25f, 0.60f, 0.95f}, 0.18f, 0.85f, 0.20f, 24);
    setAnimatedModel(ctx, frame_index, tgx::fVec3{1.15f, 0.1f, -6.2f}, tgx::fVec3{0.65f, 0.95f, 0.65f}, -22.0f);
    ctx.renderer.drawCube();
}

bool setupSphereLow(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE);
}

void renderSphereLow(BenchContext& ctx, uint32_t frame_index)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.44f, 0.72f, 0.96f}, 0.16f, 0.84f, 0.30f, 24);
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.8f}, tgx::fVec3{1.35f, 1.35f, 1.35f}, 0.0f);
    const int sectors = sectorsLow(ctx);
    ctx.renderer.drawSphere(sectors, sectors / 2 + 1);
}

bool setupSphereHigh(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE);
}

void renderSphereHigh(BenchContext& ctx, uint32_t frame_index)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.72f, 0.76f, 0.82f}, 0.15f, 0.82f, 0.58f, 20);
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.9f}, tgx::fVec3{1.45f, 1.45f, 1.45f}, 0.0f);
    const int sectors = sectorsHigh(ctx);
    ctx.renderer.drawSphere(sectors, sectors / 2 + 1);
}

float adaptiveSphereQuality(const BenchContext& ctx)
{
    if (ctx.caps.power_class >= 8) return 1.15f;
    if (ctx.caps.power_class >= 5) return 0.95f;
    if (ctx.caps.power_class >= 4) return 0.78f;
    return 0.58f;
}

bool setupAdaptiveSphere(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE);
}

void renderAdaptiveSphere(BenchContext& ctx, uint32_t frame_index)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.82f, 0.80f, 0.72f}, 0.15f, 0.84f, 0.42f, 22);
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.8f}, tgx::fVec3{1.36f, 1.36f, 1.36f}, 8.0f);
    ctx.renderer.drawAdaptativeSphere(adaptiveSphereQuality(ctx));
}

bool setupSphereTexturePerspective(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
                            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
}

void renderSphereTexture(BenchContext& ctx, uint32_t frame_index)
{
    ctx.renderer.setMaterial(tgx::RGBf{1.0f, 1.0f, 1.0f}, 0.16f, 0.86f, 0.20f, 28);
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.9f}, tgx::fVec3{1.35f, 1.35f, 1.35f}, 12.0f);
    const int sectors = sectorsLow(ctx) + 2;
    ctx.renderer.drawSphere(sectors, sectors / 2 + 1, &g_texture.side_texture);
}

bool setupSphereTextureAffine(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
                            tgx::SHADER_TEXTURE_AFFINE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
}

bool setupCylinderClosed(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE);
}

void renderCylinderClosed(BenchContext& ctx, uint32_t frame_index)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.80f, 0.62f, 0.32f}, 0.16f, 0.84f, 0.22f, 24);
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -6.0f}, tgx::fVec3{1.0f, 1.20f, 1.0f}, 18.0f);
    ctx.renderer.drawCylinder(sectorsHigh(ctx), true, true);
}

bool setupCylinderOpen(BenchContext& ctx)
{
    if (!setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE)) return false;
    ctx.renderer.setCulling(1);
    return true;
}

void renderCylinderOpen(BenchContext& ctx, uint32_t frame_index)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.52f, 0.78f, 0.58f}, 0.16f, 0.84f, 0.18f, 24);
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.8f}, tgx::fVec3{1.05f, 1.18f, 1.05f}, -20.0f);
    ctx.renderer.drawCylinder(sectorsHigh(ctx), false, false);
}

bool setupCylinderOneCap(BenchContext& ctx)
{
    if (!setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE)) return false;
    ctx.renderer.setCulling(1);
    return true;
}

void renderCylinderOneCap(BenchContext& ctx, uint32_t frame_index)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.92f, 0.76f, 0.36f}, 0.16f, 0.84f, 0.20f, 24);
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.8f}, tgx::fVec3{1.05f, 1.18f, 1.05f}, 12.0f);
    ctx.renderer.drawCylinder(sectorsHigh(ctx), true, false);
}

bool setupCylinderTextured(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
                            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
}

void renderCylinderTextured(BenchContext& ctx, uint32_t frame_index)
{
    ctx.renderer.setMaterial(tgx::RGBf{1.0f, 1.0f, 1.0f}, 0.16f, 0.86f, 0.18f, 28);
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.9f}, tgx::fVec3{1.0f, 1.15f, 1.0f}, 25.0f);
    ctx.renderer.drawCylinder(sectorsHigh(ctx), &g_texture.side_texture, &g_texture.cap_texture, &g_texture.cap_texture, true, true);
}

bool setupConeClosed(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE);
}

void renderConeClosed(BenchContext& ctx, uint32_t frame_index)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.95f, 0.48f, 0.30f}, 0.16f, 0.84f, 0.20f, 24);
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.9f}, tgx::fVec3{1.15f, 1.30f, 1.15f}, 14.0f);
    ctx.renderer.drawCone(sectorsHigh(ctx), true);
}

bool setupConeOpen(BenchContext& ctx)
{
    if (!setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE)) return false;
    ctx.renderer.setCulling(1);
    return true;
}

void renderConeOpen(BenchContext& ctx, uint32_t frame_index)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.64f, 0.54f, 0.95f}, 0.16f, 0.84f, 0.18f, 24);
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.8f}, tgx::fVec3{1.18f, 1.28f, 1.18f}, -15.0f);
    ctx.renderer.drawCone(sectorsHigh(ctx), false);
}

bool setupConeTextured(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
                            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
}

void renderConeTextured(BenchContext& ctx, uint32_t frame_index)
{
    ctx.renderer.setMaterial(tgx::RGBf{1.0f, 1.0f, 1.0f}, 0.16f, 0.86f, 0.18f, 28);
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.9f}, tgx::fVec3{1.15f, 1.30f, 1.15f}, 18.0f);
    ctx.renderer.drawCone(sectorsHigh(ctx), &g_texture.side_texture, &g_texture.cap_texture, true);
}

bool setupTruncatedTextured(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
                            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2);
}

void renderTruncatedTextured(BenchContext& ctx, uint32_t frame_index)
{
    ctx.renderer.setMaterial(tgx::RGBf{1.0f, 1.0f, 1.0f}, 0.16f, 0.86f, 0.20f, 28);
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.9f}, tgx::fVec3{1.08f, 1.18f, 1.08f}, -28.0f);
    ctx.renderer.drawTruncatedCone(sectorsHigh(ctx), 1.0f, 0.42f, &g_texture.side_texture, &g_texture.cap_texture, &g_texture.cap_texture, true, true);
}

bool setupTruncatedOpenClipped(BenchContext& ctx)
{
    if (!setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE)) return false;
    ctx.renderer.setCulling(1);
    return true;
}

void renderTruncatedOpenClipped(BenchContext& ctx, uint32_t frame_index)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.34f, 0.82f, 0.86f}, 0.16f, 0.84f, 0.22f, 24);
    const float phase = frame_index ? visualPhaseDeg(frame_index) : 0.0f;
    const float x = frame_index ? 0.18f * tgx::tgx_fast_sin_deg_clamped(phase) : 0.0f;
    setModel(ctx, tgx::fVec3{x, 0.0f, -1.45f}, tgx::fVec3{1.25f, 1.30f, 1.25f}, 20.0f + phase * 0.15f, tgx::fVec3{0.45f, 1.0f, 0.25f});
    ctx.renderer.drawTruncatedCone(sectorsLow(ctx), 1.0f, 0.35f, false, true);
}

bool setupWireframeFast(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE);
}

void renderWireframeFast(BenchContext& ctx, uint32_t frame_index)
{
    const int sectors = sectorsLow(ctx);
    ctx.renderer.setMaterial(tgx::RGBf{0.95f, 0.80f, 0.25f}, 1.0f, 0.0f, 0.0f, 16);
    setAnimatedModel(ctx, frame_index, tgx::fVec3{-1.55f, 0.0f, -5.8f}, tgx::fVec3{0.72f, 1.0f, 0.72f}, 20.0f);
    ctx.renderer.drawWireFrameCylinder(sectors, true, true);
    ctx.renderer.setMaterial(tgx::RGBf{0.30f, 0.75f, 0.95f}, 1.0f, 0.0f, 0.0f, 16);
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.8f}, tgx::fVec3{0.72f, 1.0f, 0.72f}, -15.0f);
    ctx.renderer.drawWireFrameCone(sectors, true);
    ctx.renderer.setMaterial(tgx::RGBf{0.95f, 0.34f, 0.38f}, 1.0f, 0.0f, 0.0f, 16);
    setAnimatedModel(ctx, frame_index, tgx::fVec3{1.55f, 0.0f, -5.8f}, tgx::fVec3{0.72f, 1.0f, 0.72f}, 35.0f);
    ctx.renderer.drawWireFrameTruncatedCone(sectors, 1.0f, 0.45f, true, true);
}

bool setupWireframeAA(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE);
}

void renderWireframeAA(BenchContext& ctx, uint32_t frame_index)
{
    ctx.renderer.setMaterial(tgx::RGBf{0.90f, 0.92f, 0.95f}, 1.0f, 0.0f, 0.0f, 16);
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.8f}, tgx::fVec3{1.20f, 1.20f, 1.20f}, 20.0f);
    ctx.renderer.drawWireFrameTruncatedConeAA(sectorsLow(ctx), 1.0f, 0.35f, true, true);
}

bool setupWireframeThick(BenchContext& ctx)
{
    return setupCommon(ctx, tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE);
}

void renderWireframeThick(BenchContext& ctx, uint32_t frame_index)
{
    setAnimatedModel(ctx, frame_index, tgx::fVec3{0.0f, 0.0f, -5.8f}, tgx::fVec3{1.15f, 1.15f, 1.15f}, 20.0f);
    ctx.renderer.drawWireFrameCylinder(sectorsLow(ctx), true, true, 1.4f, tgx::RGB565(31, 42, 7), 0.85f);
}

const BenchTest PRIMITIVE_TESTS[] = {
    {
        "primitives.cube.gouraud",
        "two transformed Gouraud cubes",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupCubeGouraud,
        renderCubeGouraud,
        nullptr
    },
    {
        "primitives.sphere.gouraud.low",
        "generated sphere low tessellation Gouraud",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupSphereLow,
        renderSphereLow,
        nullptr
    },
    {
        "primitives.sphere.gouraud.high",
        "generated sphere higher tessellation Gouraud with specular",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupSphereHigh,
        renderSphereHigh,
        nullptr
    },
    {
        "primitives.sphere.adaptive.gouraud",
        "adaptive generated sphere Gouraud",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupAdaptiveSphere,
        renderAdaptiveSphere,
        nullptr
    },
    {
        "primitives.sphere.texture.perspective",
        "generated textured sphere perspective mapping",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupSphereTexturePerspective,
        renderSphereTexture,
        nullptr
    },
    {
        "primitives.sphere.texture.affine",
        "generated textured sphere affine mapping",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
            tgx::SHADER_TEXTURE_AFFINE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupSphereTextureAffine,
        renderSphereTexture,
        nullptr
    },
    {
        "primitives.cylinder.closed.gouraud",
        "closed generated cylinder Gouraud",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupCylinderClosed,
        renderCylinderClosed,
        nullptr
    },
    {
        "primitives.cylinder.open.gouraud",
        "open generated cylinder with culling enabled globally",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupCylinderOpen,
        renderCylinderOpen,
        nullptr
    },
    {
        "primitives.cylinder.one_cap.gouraud",
        "generated cylinder with only the bottom cap",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupCylinderOneCap,
        renderCylinderOneCap,
        nullptr
    },
    {
        "primitives.cylinder.texture.caps",
        "textured cylinder side and caps",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupCylinderTextured,
        renderCylinderTextured,
        nullptr
    },
    {
        "primitives.cone.closed.gouraud",
        "closed generated cone Gouraud",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupConeClosed,
        renderConeClosed,
        nullptr
    },
    {
        "primitives.cone.open.gouraud",
        "open generated cone with culling enabled globally",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupConeOpen,
        renderConeOpen,
        nullptr
    },
    {
        "primitives.cone.texture.cap",
        "textured cone side and cap",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupConeTextured,
        renderConeTextured,
        nullptr
    },
    {
        "primitives.truncated.texture.caps",
        "textured truncated cone side and caps",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD |
            tgx::SHADER_TEXTURE | tgx::SHADER_TEXTURE_NEAREST | tgx::SHADER_TEXTURE_WRAP_POW2,
        0, 0, 0,
        setupTruncatedTextured,
        renderTruncatedTextured,
        nullptr
    },
    {
        "primitives.truncated.open.near_clip",
        "open truncated cone crossing the near plane",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_GOURAUD | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupTruncatedOpenClipped,
        renderTruncatedOpenClipped,
        nullptr
    },
    {
        "primitives.wireframe.fast",
        "fast generated primitive wireframes",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupWireframeFast,
        renderWireframeFast,
        nullptr
    },
    {
        "primitives.wireframe.aa",
        "antialiased generated primitive wireframe",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupWireframeAA,
        renderWireframeAA,
        nullptr
    },
    {
        "primitives.wireframe.thick",
        "adjustable thickness generated primitive wireframe",
        tgx::SHADER_PERSPECTIVE | tgx::SHADER_ZBUFFER | tgx::SHADER_UNLIT | tgx::SHADER_NOTEXTURE,
        0, 0, 0,
        setupWireframeThick,
        renderWireframeThick,
        nullptr
    }
};

const BenchModule PRIMITIVES_MODULE = {
    "primitives",
    "Generated primitive drawing",
    PRIMITIVE_TESTS,
    sizeof(PRIMITIVE_TESTS) / sizeof(PRIMITIVE_TESTS[0])
};

} // namespace

const BenchModule& getPrimitivesBenchModule()
{
    return PRIMITIVES_MODULE;
}

} // namespace tgxbench
