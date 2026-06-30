#pragma once

#ifndef CHECKERBOARD_NX
#define CHECKERBOARD_NX 14
#endif

#ifndef CHECKERBOARD_NZ
#define CHECKERBOARD_NZ 12
#endif

static const float CHECKER_TWO_PI = 6.28318530718f;
static const uint32_t CHECKER_LOOP_MS = 10000;
static const fVec3 CHECKER_SPOT_POSITION = { 0.0f, 2.35f, 2.20f };
static const RGBf CHECKER_SPOT_DIFFUSE = RGBf(3.10f, 2.86f, 1.82f);
static const RGBf CHECKER_SPOT_SPECULAR = RGBf(0.85f, 0.78f, 0.42f);


static float checkerPhase(uint32_t period_ms)
    {
    return ((float)(millis() % period_ms)) / (float)period_ms;
    }


static fVec3 checkerSpotTarget()
    {
    const float a = CHECKER_TWO_PI * checkerPhase(CHECKER_LOOP_MS);
    return {
        2.85f * sinf(a),
        -1.05f,
        -0.85f + 2.35f * sinf(2.0f * a + 0.65f)
        };
    }


static fVec3 checkerSpotDirection()
    {
    fVec3 dir = checkerSpotTarget() - CHECKER_SPOT_POSITION;
    dir.normalize_fast();
    return dir;
    }


static RGBf checkerMarkerColor(const RGBf& color)
    {
    const float m = tgx::max(tgx::max(color.R, color.G), color.B);
    if (m < 0.001f) return RGBf(1.0f, 1.0f, 1.0f);
    RGBf c(0.70f, 0.70f, 0.70f);
    c += color * (0.30f / m);
    c.clamp();
    return c;
    }


static void setupSpotlightCheckerboardScene(float aspect)
    {
    renderer.setViewportSize(render_lx, render_ly);
    renderer.setOffset(0, 0);
    renderer.setImage(&im);
    renderer.setZbuffer(zbuf);
    renderer.setCulling(0);
    renderer.setPerspective(47.0f, aspect, 0.1f, 70.0f);
    renderer.setLookAt({ 0.0f, 2.65f, 6.15f },
                       { 0.0f, -0.92f, -0.95f },
                       { 0.0f, 1.0f, 0.0f });

    renderer.setLight({ -0.25f, -0.80f, -0.35f },
                      RGBf(0.010f, 0.012f, 0.016f),
                      RGBf(0.020f, 0.022f, 0.026f),
                      RGBf(0.0f, 0.0f, 0.0f));

    renderer.setSpotLightCount(1);
    renderer.setSpotLight(0, CHECKER_SPOT_POSITION, checkerSpotDirection(), 6.6f,
                          26.0f, 17.0f,
                          CHECKER_SPOT_DIFFUSE, CHECKER_SPOT_SPECULAR);
    }


static void drawCheckerboardFloor()
    {
    static const fVec3 N = { 0.0f, 1.0f, 0.0f };
    const float x_min = -3.85f;
    const float x_max =  3.85f;
    const float z_min = -4.15f;
    const float z_max =  2.15f;
    const float y = -1.05f;
    const float dx = (x_max - x_min) / (float)CHECKERBOARD_NX;
    const float dz = (z_max - z_min) / (float)CHECKERBOARD_NZ;

    renderer.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_NOTEXTURE);
    renderer.setModelPosScaleRot({ 0.0f, 0.0f, 0.0f });

    for (int iz = 0; iz < CHECKERBOARD_NZ; iz++)
        {
        const float z0 = z_min + dz * iz;
        const float z1 = z0 + dz;
        for (int ix = 0; ix < CHECKERBOARD_NX; ix++)
            {
            const float x0 = x_min + dx * ix;
            const float x1 = x0 + dx;
            if (((ix + iz) & 1) == 0)
                {
                renderer.setMaterial(RGBf(0.86f, 0.91f, 0.96f), 0.035f, 0.965f, 0.18f, 20);
                }
            else
                {
                renderer.setMaterial(RGBf(0.045f, 0.20f, 0.86f), 0.045f, 0.955f, 0.08f, 16);
                }
            renderer.drawQuad({ x0, y, z0 }, { x0, y, z1 }, { x1, y, z1 }, { x1, y, z0 },
                              &N, &N, &N, &N);
            }
        }
    }


static void drawSpotlightConeMarker(const fVec3& position, const fVec3& direction)
    {
    fVec3 dir = direction;
    dir.normalize_fast();
    fVec3 right = crossProduct({ 0.0f, 1.0f, 0.0f }, dir);
    if (dotProduct(right, right) < 0.0001f)
        {
        right = crossProduct({ 1.0f, 0.0f, 0.0f }, dir);
        }
    right.normalize_fast();
    fVec3 up = crossProduct(dir, right);
    up.normalize_fast();

    const float len = 0.84f;
    const float radius = 0.38f;
    const fVec3 center = position + dir * len;
    const int segments = 14;

    renderer.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_UNLIT | SHADER_NOTEXTURE);
    renderer.setMaterialColor(checkerMarkerColor(CHECKER_SPOT_DIFFUSE));
    renderer.setModelPosScaleRot({ 0.0f, 0.0f, 0.0f });

    for (int i = 0; i < segments; i += 2)
        {
        const float a0 = CHECKER_TWO_PI * (float)i / (float)segments;
        const float a1 = CHECKER_TWO_PI * (float)(i + 1) / (float)segments;
        const fVec3 p0 = center + right * (radius * cosf(a0)) + up * (radius * sinf(a0));
        const fVec3 p1 = center + right * (radius * cosf(a1)) + up * (radius * sinf(a1));
        renderer.drawTriangle(position, p0, p1);
        }

    renderer.setModelPosScaleRot(position, { 0.038f, 0.038f, 0.038f });
    renderer.drawSphere(10, 6);
    }


static void drawSpotlightCheckerboardFrame()
    {
    const fVec3 dir = checkerSpotDirection();
    renderer.setSpotLightDirection(0, dir);

    im.fillScreen(RGB565_Black);
    renderer.clearZbuffer();

    drawCheckerboardFloor();
    drawSpotlightConeMarker(CHECKER_SPOT_POSITION, dir);
    }
