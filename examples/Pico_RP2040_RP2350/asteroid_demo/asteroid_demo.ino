/********************************************************************
* tgx library example : Mesh3Dv2 asteroid demo.
*
*              EXAMPLE FOR RP2040/RP2350 boards with a TFT_eSPI display
*
* This is a small 3D demo film rather than a minimal tutorial.  The scene
* is built from separate systems: a continuous ship flight path, a smooth
* camera rig, inertial asteroids, formation ships and camera-space stars.
*
* This demo is designed for Pico 2 / RP2350.  It also runs on Pico / RP2040
* boards, but with a much smaller viewport and a low framerate.
********************************************************************/

#include <TFT_eSPI.h>

#include <math.h>
#include <tgx.h>
#include <font_tgx_OpenSans_Bold.h>

#include "xwing_demo.h"
#include "tie.h"
#include "asteroid_a.h"
#include "asteroid_b.h"

#include "movie_stars.h"

using namespace tgx;

// Enable while tuning the movie timeline.  It prints the current time/phase
// directly on screen, but it is normally kept disabled for the public demo.
// #define SHOW_MOVIE_DEBUG_TAG

#if defined(__ARM_ARCH_8M_MAIN__)
static const int MAX_LX = 280;
static const int MAX_LY = 200;
#else
static const int MAX_LX = 128;
static const int MAX_LY = 96;
#endif

uint16_t fb[MAX_LX * MAX_LY];
uint16_t fb2[MAX_LX * MAX_LY];
uint16_t zbuf[MAX_LX * MAX_LY];

Image<RGB565> im;
TFT_eSPI tft = TFT_eSPI();

int render_lx = 0;
int render_ly = 0;
float render_ratio = 1.0f;
bool use_dma = false;
int draw_buffer_index = 0;

const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER |
                              SHADER_FLAT | SHADER_GOURAUD |
                              SHADER_TEXTURE_NEAREST |
                              SHADER_TEXTURE_WRAP_POW2 |
                              SHADER_NOTEXTURE;

Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;

uint32_t fps_last_ms = 0;
uint32_t fps_frames = 0;
int telemetry_phase = 0;
float validated_collision_margin = 0.0f;


RGB565 rgb888(int r, int g, int b)
    {
    return RGB565(r / 255.0f, g / 255.0f, b / 255.0f);
    }


void setupRenderer()
    {
    renderer.setViewportSize(render_lx, render_ly);
    renderer.setOffset(0, 0);
    renderer.setImage(&im);
    renderer.setZbuffer(zbuf);
    renderer.setPerspective(58, render_ratio, 0.16f, 460.0f);

    // A mostly fixed key light reads better than a light glued to the camera.
    renderer.setLight({ -0.35f, -0.70f, -0.62f },
                      RGBf(0.18f, 0.20f, 0.28f),
                      RGBf(0.88f, 0.84f, 0.72f),
                      RGBf(0.45f, 0.43f, 0.38f));
    renderer.setCulling(1);
    renderer.setTextureQuality(SHADER_TEXTURE_NEAREST);
    renderer.setTextureWrappingMode(SHADER_TEXTURE_WRAP_POW2);
    }


void setMeshMatrix(float x, float y, float z, float scale, float yaw, float pitch, float roll)
    {
    fMat4 M;
    M.setScale({ scale, scale, scale });
    M.multRotate(yaw, { 0, 1, 0 });
    M.multRotate(pitch, { 1, 0, 0 });
    M.multRotate(roll, { 0, 0, 1 });
    M.multTranslate({ x, y, z });
    renderer.setModelMatrix(M);
    }


void drawHeroShip(const ShipState& ship)
    {
    const float yaw = yawForDirection(ship.v);
    const float pitch = pitchForDirection(ship.v) * 0.55f + ship.pitch;
    setMeshMatrix(ship.p.x, ship.p.y, ship.p.z, ship.scale, yaw, pitch, ship.roll);
    renderer.setShaders(SHADER_GOURAUD | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2);
    renderer.drawMesh(&xwing_demo, true);
    }


void drawWingShip(const WingShipState& wing)
    {
    if (!wing.visible) return;
    const float yaw = yawForDirection(wing.v);
    const float pitch = pitchForDirection(wing.v) * 0.45f;
    setMeshMatrix(wing.p.x, wing.p.y, wing.p.z, wing.scale, yaw, pitch, wing.roll);
    renderer.setShaders(SHADER_GOURAUD | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2);
    renderer.drawMesh(&tie, true);
    }


void drawAsteroid(const AsteroidDraw& a)
    {
    fMat4 M;
    M.setScale({ a.scale, a.scale, a.scale });
    M.multRotate(a.angle, a.axis);
    M.multTranslate(a.p);
    renderer.setModelMatrix(M);
    renderer.setShaders(SHADER_FLAT);
    renderer.drawMesh((a.mesh_id == 0) ? &asteroid_a : &asteroid_b, true);
    }


void drawForegroundAsteroid(float seconds, const CameraState& cam)
    {
    const float show = fadeInOut(seconds, 42.0f, 44.0f, 50.0f, 52.0f);
    if (show <= 0.0f) return;

    const float u = smooth01((seconds - 42.0f) / 10.0f);
    const CameraBasis b = makeCameraBasis(cam.eye, cam.target);
    const fVec3 local = { mixf(-4.4f, 4.6f, u),
                          0.55f * sinf((u - 0.15f) * MOVIE_PI),
                          mixf(2.45f, 3.35f, u) };
    AsteroidDraw a;
    a.mesh_id = 1;
    a.p = cam.eye + b.right * local.x + b.up * local.y + b.forward * local.z;
    a.axis = { 0.25f, 1.0f, 0.45f };
    a.scale = mixf(1.95f, 1.35f, u) * show;
    a.angle = seconds * 58.0f;
    drawAsteroid(a);
    }


void drawOpeningText(float seconds)
    {
    if (seconds > 5.5f) return;
    const float alpha = 1.0f - smooth01((seconds - 3.0f) / 2.5f);
    const int v = (int)(alpha * 190.0f);
    const int x = render_lx / 2 - 58;
    const int y = render_ly / 2 - 8;
    im.drawText("TGX asteroid", { x, y }, font_tgx_OpenSans_Bold_14, rgb888(v, v + 10, v + 25));
    }


void drawDebugTag(float seconds)
    {
    char tag[40];
    snprintf(tag, sizeof(tag), "%03ds  %s", (int)seconds, phaseName(telemetry_phase));
    im.fillRect({ 4, 20, 172, 22 }, rgb888(0, 0, 0), 0.72f);
    im.drawText(tag, { 8, 23 }, font_tgx_OpenSans_Bold_14, rgb888(255, 230, 110));
    }


void drawScene(float seconds)
    {
    const ShipState ship = computeShipState(seconds);
    const CameraState cam = computeCameraState(seconds, ship);
    telemetry_phase = cam.phase;

    im.fillScreen(rgb888(1, 3, 12));
    drawCameraStars(im, cam, render_lx, render_ly, rgb888);

    renderer.setPerspective(cam.fov, render_ratio, 0.16f, 460.0f);
    renderer.setLookAt(cam.eye, cam.target, { 0.0f, 1.0f, 0.0f });
    renderer.clearZbuffer();

    AsteroidDraw a;
    const int max_asteroids =
#if defined(__ARM_ARCH_8M_MAIN__)
        NB_ASTEROIDS;
#else
        12;
#endif
    for (int i = 0; i < max_asteroids; i++)
        {
        if (asteroidDrawState(i, seconds, cam, a)) drawAsteroid(a);
        }

#if defined(__ARM_ARCH_8M_MAIN__)
    drawForegroundAsteroid(seconds, cam);
#endif
    drawWingShip(computeWingShip(seconds, ship, 0));
    drawWingShip(computeWingShip(seconds, ship, 1));
    drawWingShip(computeWingShip(seconds, ship, 2));
    drawHeroShip(ship);
    drawOpeningText(seconds);
#if defined(SHOW_MOVIE_DEBUG_TAG)
    drawDebugTag(seconds);
#endif
    }


void updateFPS()
    {
    fps_frames++;
    const uint32_t now = millis();
    if (now - fps_last_ms >= 1000)
        {
        Serial.print("asteroid_demo phase=");
        Serial.print(phaseName(telemetry_phase));
        Serial.print(" fps=");
        Serial.print(fps_frames);
        Serial.print(" display=");
        Serial.print(use_dma ? "DMA" : "pushImage");
        Serial.print(" margin=");
        Serial.println(validated_collision_margin, 2);
        fps_frames = 0;
        fps_last_ms = now;
        }
    }


void setupDMA()
    {
    use_dma = tft.initDMA();
    if (use_dma)
        {
        tft.startWrite();
        Serial.println("asteroid_demo display DMA enabled");
        }
    else
        {
        Serial.println("asteroid_demo display DMA unavailable, using pushImage");
        }
    }


void pushFrame()
    {
    const int x = (tft.width() - render_lx) / 2;
    const int y = (tft.height() - render_ly) / 2;
    if (use_dma)
        {
        uint16_t* current = (draw_buffer_index == 0) ? fb : fb2;
        tft.pushImageDMA(x, y, render_lx, render_ly, current);
        draw_buffer_index = 1 - draw_buffer_index;
        im.set((draw_buffer_index == 0) ? fb : fb2, render_lx, render_ly);
        renderer.setImage(&im);
        }
    else
        {
        tft.pushImage(x, y, render_lx, render_ly, fb);
        }
    }


void setup()
    {
    Serial.begin(115200);

    tft.init();
    tft.setRotation(1);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLUE);

    render_lx = tft.width();
    render_ly = tft.height();
    if (render_lx > MAX_LX) render_lx = MAX_LX;
    if (render_ly > MAX_LY) render_ly = MAX_LY;
    render_ratio = (float)render_lx / (float)render_ly;
    setupDMA();

    im.set(fb, render_lx, render_ly);
    setupRenderer();
    initStars();

    validated_collision_margin = minDistanceShipAsteroids();
    Serial.print("asteroid_demo collision_margin=");
    Serial.println(validated_collision_margin, 2);
    fps_last_ms = millis();
    }


void loop()
    {
    const float seconds = (float)(millis() % MOVIE_MS) * 0.001f;
    drawScene(seconds);
    pushFrame();
    updateFPS();
    }

/** end of file */
