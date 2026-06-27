#pragma once

#include "spot.h"
#include "stormtrooper_v2.h"
#include "donkeykong_small_v2.h"

static const float POINTLIGHT_TWO_PI = 6.28318530718f;
static const uint32_t POINTLIGHT_MESH_DURATION_MS = 11000;
static const uint32_t POINTLIGHT_LOOP_MS = 9000;


struct PointlightMeshScene
    {
    const Mesh3Dv2<RGB565>* mesh;
    const char* name;
    fVec3 position;
    fVec3 scale;
    float angle;
    fVec3 axis;
    float orbitRadius;
    float range;
    };


static const PointlightMeshScene pointlight_mesh_scenes[] =
    {
        { &spot,             "spot",             { 0.0f, -0.35f, 0.0f }, { 1.52f, 1.52f, 1.52f }, -12.0f, { 0.0f, 1.0f, 0.0f }, 2.45f, 4.9f },
        { &stormtrooper_v2,  "stormtrooper",     { 0.0f, -0.95f, 0.0f }, { 1.78f, 1.78f, 1.78f },  18.0f, { 0.0f, 1.0f, 0.0f }, 2.70f, 5.2f },
        { &donkeykong_small, "donkeykong_small", { 0.0f, -0.55f, 0.0f }, { 1.42f, 1.42f, 1.42f },   0.0f, { 0.0f, 1.0f, 0.0f }, 2.55f, 5.0f },
    };


static const int pointlight_mesh_scene_count =
    (int)(sizeof(pointlight_mesh_scenes) / sizeof(pointlight_mesh_scenes[0]));


static float pointlightPhase(uint32_t period_ms)
    {
    return ((float)(millis() % period_ms)) / (float)period_ms;
    }


static int pointlightSceneIndex()
    {
    return (int)((millis() / POINTLIGHT_MESH_DURATION_MS) % pointlight_mesh_scene_count);
    }


static RGBf pointlightMarkerColor(const RGBf& color)
    {
    float m = color.R;
    if (color.G > m) m = color.G;
    if (color.B > m) m = color.B;
    if (m < 0.001f) return RGBf(1.0f, 1.0f, 1.0f);
    RGBf c = color * (1.0f / m);
    c.R = 0.72f + 0.28f * c.R;
    c.G = 0.72f + 0.28f * c.G;
    c.B = 0.72f + 0.28f * c.B;
    c.clamp();
    return c;
    }


static fVec3 currentPointLightPosition(const PointlightMeshScene& scene)
    {
    const float t = pointlightPhase(POINTLIGHT_LOOP_MS);
    const float a = POINTLIGHT_TWO_PI * t;
    const float r = scene.orbitRadius;
    return {
        scene.position.x + r * cosf(a),
        scene.position.y + 1.20f + 0.78f * (0.5f + 0.5f * sinf(2.0f * a + 0.45f)),
        scene.position.z + r * sinf(a)
        };
    }


static void setupPointlightTexturedScene(float aspect)
    {
    renderer.setViewportSize(render_lx, render_ly);
    renderer.setOffset(0, 0);
    renderer.setImage(&im);
    renderer.setZbuffer(zbuf);
    renderer.setCulling(0);
    renderer.setPerspective(45.0f, aspect, 0.1f, 60.0f);
    renderer.setLookAt({ 0.0f, 1.25f, 5.3f },
                       { 0.0f, -0.15f, 0.0f },
                       { 0.0f, 1.0f, 0.0f });
    renderer.setTextureQuality(SHADER_TEXTURE_NEAREST);
    renderer.setTextureWrappingMode(SHADER_TEXTURE_WRAP_POW2);

    renderer.setLight({ -0.35f, -0.55f, -0.55f },
                      RGBf(0.018f, 0.020f, 0.024f),
                      RGBf(0.035f, 0.038f, 0.042f),
                      RGBf(0.0f, 0.0f, 0.0f));

    const PointlightMeshScene& scene = pointlight_mesh_scenes[0];
    renderer.setSpotLightCount(1);
    renderer.setSpotLight(0, currentPointLightPosition(scene), scene.range,
                          RGBf(2.95f, 2.30f, 1.42f),
                          RGBf(0.98f, 0.82f, 0.55f));
    }


static void drawPointlightMarker(const fVec3& position, const RGBf& color)
    {
    renderer.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_UNLIT | SHADER_NOTEXTURE);
    renderer.setMaterialColor(pointlightMarkerColor(color));
    renderer.setModelPosScaleRot(position, { 0.070f, 0.070f, 0.070f });
    renderer.drawSphere(10, 6);
    }


static void drawPointlightTexturedFrame()
    {
    const PointlightMeshScene& scene = pointlight_mesh_scenes[pointlightSceneIndex()];
    const fVec3 light_pos = currentPointLightPosition(scene);
    const RGBf light_diffuse = RGBf(2.95f, 2.30f, 1.42f);
    const RGBf light_specular = RGBf(0.98f, 0.82f, 0.55f);

    renderer.setSpotLight(0, light_pos, scene.range, light_diffuse, light_specular);

    im.fillScreen(RGB565_Black);
    renderer.clearZbuffer();

    renderer.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD |
                        TEXTURE_SHADER | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2);
    renderer.setModelPosScaleRot(scene.position, scene.scale, scene.angle, scene.axis);
    renderer.drawMesh(scene.mesh);

    drawPointlightMarker(light_pos, light_diffuse);
    }
