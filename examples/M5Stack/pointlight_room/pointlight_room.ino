/********************************************************************
* tgx library example : moving local point lights.
*
*                  EXAMPLE FOR M5STACK Core2 and CoreS3
*
* This sketch renders a small room-like 3D scene lit by two animated local
* point lights.  It draws into a TGX framebuffer, then sends that framebuffer
* to the display with LovyanGFX.
*
* Instructions:
*
* 1. download and install LovyanGFX via Arduino's library manager or directly
*    from: https://github.com/lovyan03/LovyanGFX
*
* 2. Select the board model and serial port and upload the sketch.
********************************************************************/

/**************** DMA NOTE ****************
* LovyanGFX DMA transfers can improve display upload speed on M5Stack boards.
* DMA is enabled by default here.  The sketch first tries to allocate a
* DMA-capable transfer buffer; if that fails, it automatically falls back to
* the blocking pushImage() path.  Uncomment the line below to force that
* non-DMA path.
*******************************************/
// #define DISABLE_DMA

#define LGFX_AUTODETECT
#include <LovyanGFX.hpp>

#include <tgx.h>

using namespace tgx;

// The sketch renders into a smaller off-screen framebuffer, then centers it
// on the physical display.  This keeps RAM use reasonable on Core2/CoreS3.
static const int MAX_LX = 240;
static const int MAX_LY = 180;

uint16_t* fb = nullptr;
uint16_t* fb2 = nullptr;
uint16_t* zbuf = nullptr;
bool use_dma = false;

Image<RGB565> im;
LGFX lcd;

int render_lx = 0;
int render_ly = 0;
float render_ratio = 1.0f;

const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER |
                              SHADER_GOURAUD | SHADER_UNLIT |
                              SHADER_NOTEXTURE;

Renderer3D<RGB565, LOADED_SHADERS, uint16_t, 1, 2> renderer;

static const int FLOOR_NX = 20;
static const int FLOOR_NY = 18;

static const int FLOOR_VERTICES = (FLOOR_NX + 1) * (FLOOR_NY + 1);
static const int FLOOR_INDICES = FLOOR_NX * FLOOR_NY * 6;

fVec3 floor_vertices[FLOOR_VERTICES];
uint16_t floor_indices[FLOOR_INDICES];
uint16_t floor_normal_indices[FLOOR_INDICES];
fVec3 floor_normals[1] = { { 0.0f, 1.0f, 0.0f } };

uint32_t fps_last_ms = 0;
uint32_t fps_frames = 0;
uint32_t fps_render_sum_us = 0;


static const RGBf WARM_DIFFUSE = RGBf(3.15f, 0.76f, 0.30f);
static const RGBf WARM_SPECULAR = RGBf(1.45f, 0.62f, 0.30f);
static const RGBf COOL_DIFFUSE = RGBf(0.28f, 1.02f, 3.25f);
static const RGBf COOL_SPECULAR = RGBf(0.30f, 0.70f, 1.85f);

static const uint32_t LOOP_DURATION_MS = 28000;
static const uint32_t CAMERA_ORBIT_DURATION_MS = 73000;
static const float TWO_PI_F = 6.28318530718f;


static const fVec3 warm_path[] =
    {
        { -3.70f, 2.60f, 3.30f },
        { -2.35f, 1.15f, 0.95f },
        {  0.10f, 0.68f,-2.85f },
        {  3.55f, 1.55f,-3.55f },
        {  2.25f, 2.85f, 0.85f },
        {  0.00f, 2.35f, 3.60f },
        { -2.95f, 0.82f,-3.25f },
        { -0.80f, 1.08f,-0.35f },
        {  2.85f, 0.78f, 2.75f },
        {  3.75f, 2.70f,-2.60f },
        {  0.75f, 2.10f, 3.20f },
    };


static const fVec3 cool_path[] =
    {
        {  3.60f, 0.90f,-1.30f },
        {  1.30f, 2.65f, 3.40f },
        { -1.40f, 1.15f, 0.10f },
        { -3.60f, 2.20f,-3.40f },
        { -1.75f, 0.74f, 2.80f },
        {  0.42f, 2.12f,-1.80f },
        {  2.65f, 2.65f, 3.60f },
        {  3.45f, 1.25f,-2.40f },
        {  0.20f, 2.85f, 0.50f },
        { -2.90f, 0.78f, 3.10f },
        { -0.20f, 1.45f,-3.10f },
    };


fVec3 catmullRom(const fVec3& p0, const fVec3& p1, const fVec3& p2, const fVec3& p3, float t)
    {
    const float t2 = t * t;
    const float t3 = t2 * t;
    return (p1 * 2.0f
          + (p2 - p0) * t
          + (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2
          + (-p0 + p1 * 3.0f - p2 * 3.0f + p3) * t3) * 0.5f;
    }


int wrapIndex(int index, int count)
    {
    if (index < 0) return index + count;
    if (index >= count) return index - count;
    return index;
    }


fVec3 sampleClosedPath(const fVec3* path, int count, float phase)
    {
    const float scaled = phase * count;
    int i1 = (int)scaled;
    if (i1 >= count) i1 = 0;
    const float t = scaled - i1;

    const int i0 = wrapIndex(i1 - 1, count);
    const int i2 = wrapIndex(i1 + 1, count);
    const int i3 = wrapIndex(i1 + 2, count);

    return catmullRom(path[i0], path[i1], path[i2], path[i3], t);
    }


RGBf markerColor(const RGBf& c)
    {
    const float m = tgx::max(tgx::max(c.R, c.G), c.B);
    if (m <= 0.000001f) return RGBf(1.0f, 1.0f, 1.0f);
    RGBf marker(0.10f, 0.10f, 0.10f);
    const RGBf normalized = c * (1.0f / m);
    marker += normalized * normalized * 0.90f;
    marker.clamp();
    return marker;
    }


void buildGrid(fVec3* vertices, uint16_t* indices, uint16_t* normal_indices,
               int nx, int ny, const fVec3& origin, const fVec3& u, const fVec3& v)
    {
    const int row = nx + 1;
    int k = 0;

    for (int y = 0; y <= ny; y++)
        {
        const float fy = ((float)y) / ny;
        for (int x = 0; x <= nx; x++)
            {
            const float fx = ((float)x) / nx;
            vertices[y * row + x] = origin + u * fx + v * fy;
            }
        }

    for (int y = 0; y < ny; y++)
        {
        for (int x = 0; x < nx; x++)
            {
            const uint16_t i00 = (uint16_t)(y * row + x);
            const uint16_t i10 = (uint16_t)(y * row + x + 1);
            const uint16_t i01 = (uint16_t)((y + 1) * row + x);
            const uint16_t i11 = (uint16_t)((y + 1) * row + x + 1);

            indices[k] = i00; normal_indices[k++] = 0;
            indices[k] = i01; normal_indices[k++] = 0;
            indices[k] = i11; normal_indices[k++] = 0;
            indices[k] = i00; normal_indices[k++] = 0;
            indices[k] = i11; normal_indices[k++] = 0;
            indices[k] = i10; normal_indices[k++] = 0;
            }
        }
    }


float animationPhase()
    {
    return ((float)(millis() % LOOP_DURATION_MS)) / LOOP_DURATION_MS;
    }


void updateCamera()
    {
    const float phase = ((float)(millis() % CAMERA_ORBIT_DURATION_MS)) / CAMERA_ORBIT_DURATION_MS;
    const float a = phase * TWO_PI_F;
    const fVec3 target = { 0.0f, -0.05f, -0.75f };
    const float radius = 7.6f;
    const float height = 2.18f + 0.18f * sinf(2.0f * a + 0.45f);

    renderer.setLookAt({ target.x + radius * sinf(a), height, target.z + radius * cosf(a) },
                       target,
                       { 0.0f, 1.0f, 0.0f });
    }


void updateMovingPointLights(float phase)
    {
    const fVec3 warm = sampleClosedPath(warm_path, (int)(sizeof(warm_path) / sizeof(warm_path[0])), phase);
    const fVec3 cool = sampleClosedPath(cool_path, (int)(sizeof(cool_path) / sizeof(cool_path[0])), phase);

    renderer.setSpotLightPosition(0, warm);
    renderer.setSpotLightPosition(1, cool);
    }


void drawGridMesh(const fVec3* vertices, const uint16_t* indices, const uint16_t* normal_indices,
                  const fVec3* normals, int triangle_count, const RGBf& color, float specular_strength)
    {
    renderer.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_NOTEXTURE);
    renderer.setMaterial(color, 0.06f, 0.94f, specular_strength, 18);
    renderer.setModelPosScaleRot({ 0.0f, 0.0f, 0.0f });
    renderer.drawTriangles(triangle_count, indices, vertices, normal_indices, normals);
    }


void drawRoom()
    {
    drawGridMesh(floor_vertices, floor_indices, floor_normal_indices, floor_normals,
                 FLOOR_INDICES / 3, RGBf(0.82f, 0.83f, 0.80f), 0.08f);

    renderer.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_NOTEXTURE);

    renderer.setMaterial(RGBf(0.82f, 0.80f, 0.74f), 0.04f, 0.96f, 0.95f, 32);
    renderer.setModelPosScaleRot({ -1.65f, -0.43f, -0.90f }, { 0.82f, 0.82f, 0.82f });
    renderer.drawSphere(20, 11);

    renderer.setMaterial(RGBf(0.72f, 0.78f, 0.86f), 0.04f, 0.96f, 1.10f, 36);
    renderer.setModelPosScaleRot({ 1.40f, -0.55f, -1.85f }, { 0.70f, 0.70f, 0.70f });
    renderer.drawSphere(18, 10);

    renderer.setMaterial(RGBf(0.78f, 0.70f, 0.62f), 0.05f, 0.94f, 0.55f, 24);
    renderer.setModelPosScaleRot({ 0.15f, -0.55f, 0.45f }, { 0.58f, 0.68f, 0.58f },
                                 0.0f, { 0.0f, 1.0f, 0.0f });
    renderer.drawCube();

    renderer.setMaterial(RGBf(0.62f, 0.66f, 0.70f), 0.05f, 0.92f, 0.45f, 20);
    renderer.setModelPosScaleRot({ 2.65f, -0.72f, 0.10f }, { 0.44f, 0.52f, 0.44f },
                                 18.0f, { 0.0f, 1.0f, 0.0f });
    renderer.drawCube();

    renderer.setMaterial(RGBf(0.70f, 0.63f, 0.56f), 0.05f, 0.92f, 0.40f, 18);
    renderer.setModelPosScaleRot({ -3.05f, -0.76f, -0.15f }, { 0.42f, 0.48f, 0.42f },
                                 -16.0f, { 0.0f, 1.0f, 0.0f });
    renderer.drawCube();
    }


void drawLightMarker(const fVec3& position, const RGBf& color)
    {
    renderer.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_UNLIT | SHADER_NOTEXTURE);
    renderer.setMaterialColor(markerColor(color));
    renderer.setModelPosScaleRot(position, { 0.0375f, 0.0375f, 0.0375f });
    renderer.drawSphere(10, 6);
    }


void drawLightMarkers(float phase)
    {
    drawLightMarker(sampleClosedPath(warm_path, (int)(sizeof(warm_path) / sizeof(warm_path[0])), phase), WARM_DIFFUSE);
    drawLightMarker(sampleClosedPath(cool_path, (int)(sizeof(cool_path) / sizeof(cool_path[0])), phase), COOL_DIFFUSE);
    }


void setupRenderer()
    {
    renderer.setViewportSize(render_lx, render_ly);
    renderer.setOffset(0, 0);
    renderer.setImage(&im);
    renderer.setZbuffer(zbuf);
    renderer.setCulling(0);
    renderer.setPerspective(48.0f, render_ratio, 0.1f, 80.0f);
    renderer.setLookAt({ 0.0f, 2.25f, 7.6f },
                       { 0.0f, 0.0f, -1.0f },
                       { 0.0f, 1.0f, 0.0f });

    renderer.setLight({ -0.35f, -0.75f, -0.35f },
                      RGBf(0.008f, 0.010f, 0.015f),
                      RGBf(0.035f, 0.038f, 0.044f),
                      RGBf(0.0f, 0.0f, 0.0f));

    const fVec3 warm = sampleClosedPath(warm_path, (int)(sizeof(warm_path) / sizeof(warm_path[0])), 0.0f);
    const fVec3 cool = sampleClosedPath(cool_path, (int)(sizeof(cool_path) / sizeof(cool_path[0])), 0.0f);

    renderer.setSpotLightCount(2);
    renderer.setSpotLight(0, warm, 5.8f, WARM_DIFFUSE, WARM_SPECULAR);
    renderer.setSpotLight(1, cool, 5.6f, COOL_DIFFUSE, COOL_SPECULAR);
    }


void drawFrame()
    {
    const float phase = animationPhase();

    updateCamera();
    updateMovingPointLights(phase);

    im.fillScreen(RGB565_Black);
    renderer.clearZbuffer();

    drawRoom();
    drawLightMarkers(phase);
    }


void updateFPS(uint32_t render_us)
    {
    fps_frames++;
    fps_render_sum_us += render_us;
    uint32_t now = millis();
    if (now - fps_last_ms >= 1000)
        {
        Serial.print("pointlight_room M5Stack fps=");
        Serial.println(fps_render_sum_us ? (uint32_t)((1000000ULL * fps_frames) / fps_render_sum_us) : 0);
        Serial.print("pointlight_room M5Stack display=");
        Serial.println(use_dma ? "DMA" : "pushImage");
        fps_frames = 0;
        fps_render_sum_us = 0;
        fps_last_ms = now;
        }
    }


void freeRenderBuffers()
    {
    if (fb != nullptr)
        {
        free(fb);
        fb = nullptr;
        }
    if (zbuf != nullptr)
        {
        free(zbuf);
        zbuf = nullptr;
        }
    if (fb2 != nullptr)
        {
        free(fb2);
        fb2 = nullptr;
        }
    use_dma = false;
    }


bool allocateRenderBuffers(size_t pixel_count)
    {
    const size_t bytes = pixel_count * sizeof(uint16_t);

#if !defined(DISABLE_DMA)
    fb2 = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_DMA);
#endif

    fb = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
    zbuf = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_8BIT);

    if ((fb == nullptr || zbuf == nullptr) && fb2 != nullptr)
        {
        Serial.println("pointlight_room M5Stack DMA buffer released to keep framebuffer/zbuffer");
        freeRenderBuffers();
        fb = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
        zbuf = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
        }

    if (fb == nullptr || zbuf == nullptr)
        {
        freeRenderBuffers();
        return false;
        }

#if !defined(DISABLE_DMA)
    if (fb2 != nullptr)
        {
        lcd.initDMA();
        lcd.startWrite();
        use_dma = true;
        Serial.println("pointlight_room M5Stack display DMA enabled");
        }
    else
        {
        Serial.println("pointlight_room M5Stack display DMA unavailable, using pushImage");
        }
#else
    Serial.println("pointlight_room M5Stack display DMA disabled, using pushImage");
#endif

    return true;
    }


void copySwapBuffer()
    {
    const int n = render_lx * render_ly;
    for (int i = 0; i < n; i++)
        {
        const uint16_t a = fb[i];
        fb2[i] = (a << 8) | (a >> 8);
        }
    }


void pushFrame()
    {
    const int x = (lcd.width() - render_lx) / 2;
    const int y = (lcd.height() - render_ly) / 2;
    if (use_dma)
        {
        lcd.waitDMA();
        copySwapBuffer();
        lcd.pushImageDMA(x, y, render_lx, render_ly, fb2);
        }
    else
        {
        lcd.pushImage(x, y, render_lx, render_ly, fb);
        }
    }


void setup()
    {
    Serial.begin(115200);

    lcd.init();
    lcd.setRotation(1);
    lcd.setBrightness(200);
    lcd.setColorDepth(16);
    lcd.setSwapBytes(true);
    lcd.fillScreen(TFT_BLACK);

    render_lx = lcd.width();
    render_ly = lcd.height();
    if (render_lx > MAX_LX) render_lx = MAX_LX;
    if (render_ly > MAX_LY) render_ly = MAX_LY;
    render_ratio = (float)render_lx / (float)render_ly;

    const size_t pixel_count = (size_t)render_lx * (size_t)render_ly;
    while (!allocateRenderBuffers(pixel_count))
        {
        Serial.println("Error: cannot allocate framebuffer/zbuffer");
        delay(1000);
        }
    lcd.setSwapBytes(!use_dma);

    im.set(fb, render_lx, render_ly);

    buildGrid(floor_vertices, floor_indices, floor_normal_indices,
              FLOOR_NX, FLOOR_NY,
              { -5.5f, -1.25f, -4.5f },
              { 11.0f, 0.0f, 0.0f },
              { 0.0f, 0.0f, 9.0f });

    setupRenderer();
    fps_last_ms = millis();
    }


void loop()
    {
    const uint32_t render_start_us = micros();
    drawFrame();
    const uint32_t render_us = micros() - render_start_us;
    pushFrame();
    updateFPS(render_us);
    }
