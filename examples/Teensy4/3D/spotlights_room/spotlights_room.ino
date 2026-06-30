/********************************************************************
*
* tgx library example: moving local spot lights.
*
* EXAMPLE FOR TEENSY 4 / 4.1
*
* DISPLAY: ILI9341 (320x240)
*
********************************************************************/

// This example runs on Teensy 4.0/4.1 with ILI9341 via SPI.
// The screen driver library: https://github.com/vindar/ILI9341_T4
#include <ILI9341_T4.h>

// The tgx library.
#include <tgx.h>

using namespace tgx;


//
// DEFAULT WIRING USING SPI 0 ON TEENSY 4/4.1
//
#define PIN_SCK     13
#define PIN_MISO    12
#define PIN_MOSI    11
#define PIN_DC      10

#define PIN_CS      9
#define PIN_RESET   6
#define PIN_BACKLIGHT 255
#define PIN_TOUCH_IRQ 255
#define PIN_TOUCH_CS  255


// 30 MHz SPI, we can go higher with short wires.
#define SPI_SPEED       30000000


ILI9341_T4::ILI9341Driver tft(PIN_CS, PIN_DC, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_RESET, PIN_TOUCH_CS, PIN_TOUCH_IRQ);

DMAMEM ILI9341_T4::DiffBuffStatic<8000> diff1;
DMAMEM ILI9341_T4::DiffBuffStatic<8000> diff2;

static const int SLX = 320;
static const int SLY = 240;

DMAMEM uint16_t fb[SLX * SLY];
DMAMEM uint16_t internal_fb[SLX * SLY];
DMAMEM uint16_t zbuf[SLX * SLY];

Image<RGB565> im(fb, SLX, SLY);


const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_UNLIT | SHADER_NOTEXTURE;

// The last template argument gives this renderer two local spot-light slots.
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


void updateFPS(uint32_t render_us)
    {
    fps_frames++;
    fps_render_sum_us += render_us;
    const uint32_t now = millis();
    if (now - fps_last_ms >= 1000)
        {
        Serial.print("spotlights_room Teensy4 fps=");
        Serial.println(fps_render_sum_us ? (uint32_t)((1000000ULL * fps_frames) / fps_render_sum_us) : 0);
        fps_frames = 0;
        fps_render_sum_us = 0;
        fps_last_ms = now;
        }
    }


void setup()
    {
    Serial.begin(9600);

    while (!tft.begin(SPI_SPEED));

    pinMode(PIN_BACKLIGHT, OUTPUT);
    digitalWrite(PIN_BACKLIGHT, HIGH);

    tft.setRotation(3);
    tft.setFramebuffer(internal_fb);
    tft.setDiffBuffers(&diff1, &diff2);
    tft.setDiffGap(4);
    tft.setVSyncSpacing(0);
    tft.setRefreshRate(140);

    buildGrid(floor_vertices, floor_indices, floor_normal_indices,
              FLOOR_NX, FLOOR_NY,
              { -5.5f, -1.25f, -4.5f },
              { 11.0f, 0.0f, 0.0f },
              { 0.0f, 0.0f, 9.0f });

    renderer.setViewportSize(SLX, SLY);
    renderer.setOffset(0, 0);
    renderer.setImage(&im);
    renderer.setZbuffer(zbuf);
    renderer.setCulling(0);
    renderer.setPerspective(48.0f, ((float)SLX) / SLY, 0.1f, 80.0f);
    renderer.setLookAt({ 0.0f, 2.25f, 7.6f },
                       { 0.0f, 0.0f, -1.0f },
                       { 0.0f, 1.0f, 0.0f });

    // Keep the directional light weak so the moving local spot lights dominate.
    renderer.setLight({ -0.35f, -0.75f, -0.35f },
                      RGBf(0.008f, 0.010f, 0.015f),
                      RGBf(0.035f, 0.038f, 0.044f),
                      RGBf(0.0f, 0.0f, 0.0f));

    const fVec3 warm = sampleClosedPath(warm_path, (int)(sizeof(warm_path) / sizeof(warm_path[0])), 0.0f);
    const fVec3 cool = sampleClosedPath(cool_path, (int)(sizeof(cool_path) / sizeof(cool_path[0])), 0.0f);

    renderer.setSpotLightCount(2);
    renderer.setSpotLight(0, warm, 5.8f, WARM_DIFFUSE, WARM_SPECULAR);
    renderer.setSpotLight(1, cool, 5.6f, COOL_DIFFUSE, COOL_SPECULAR);
    fps_last_ms = millis();
    }


void loop()
    {
    const float phase = animationPhase();

    updateCamera();
    updateMovingPointLights(phase);

    const uint32_t render_start_us = micros();

    im.fillScreen(RGB565_Black);
    renderer.clearZbuffer();
    drawRoom();
    drawLightMarkers(phase);
    const uint32_t render_us = micros() - render_start_us;

    tft.overlayFPS(fb);
    tft.update(fb);
    updateFPS(render_us);
    }
