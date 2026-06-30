/********************************************************************
* TGX Core2 Donkey Kong infos probe.
*
* Same rendering/display pattern as examples/M5Stack/donkeykong, with
* detailed telemetry for:
* - TGX render work,
* - the original infos() block, including lcd.waitDMA(),
* - framebuffer upload/copy.
********************************************************************/

// #define DISABLE_DMA

#define LGFX_AUTODETECT
#include <LovyanGFX.hpp>

#include <tgx.h>

#include "../../../../../examples/M5Stack/donkeykong/donkeykong_small_v2.h"

using namespace tgx;

#define SLX 240
#define SLY 180

static uint16_t* fb = nullptr;
static uint16_t* fb2 = nullptr;
static uint16_t* zbuf = nullptr;
static bool use_dma = false;

static Image<RGB565> imfb;
static LGFX lcd;

const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER |
                              SHADER_FLAT | SHADER_GOURAUD |
                              SHADER_NOTEXTURE | SHADER_TEXTURE |
                              SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2;

static Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;

static uint32_t telemetry_last_ms = 0;
static uint32_t telemetry_frames = 0;
static uint32_t telemetry_render_sum_us = 0;
static uint32_t telemetry_render_min_us = 0xFFFFFFFFu;
static uint32_t telemetry_render_max_us = 0;
static uint32_t telemetry_info_sum_us = 0;
static uint32_t telemetry_info_min_us = 0xFFFFFFFFu;
static uint32_t telemetry_info_max_us = 0;
static uint32_t telemetry_upload_sum_us = 0;
static uint32_t telemetry_upload_min_us = 0xFFFFFFFFu;
static uint32_t telemetry_upload_max_us = 0;
static uint32_t telemetry_cycle = 0;
static const char* telemetry_scene = "startup";


static void telemetryReset()
    {
    telemetry_last_ms = millis();
    telemetry_frames = 0;
    telemetry_render_sum_us = 0;
    telemetry_render_min_us = 0xFFFFFFFFu;
    telemetry_render_max_us = 0;
    telemetry_info_sum_us = 0;
    telemetry_info_min_us = 0xFFFFFFFFu;
    telemetry_info_max_us = 0;
    telemetry_upload_sum_us = 0;
    telemetry_upload_min_us = 0xFFFFFFFFu;
    telemetry_upload_max_us = 0;
    }


static void telemetryPrintScene()
    {
    for (const char* p = telemetry_scene; *p != 0; p++)
        {
        const char c = *p;
        Serial.print((c <= ' ' || c == '=') ? '_' : c);
        }
    }


static void telemetrySetScene(const char* scene)
    {
    telemetry_scene = scene ? scene : "unnamed";
    telemetry_cycle++;
    telemetryReset();
    Serial.print("\n[TGX scene] cycle=");
    Serial.print(telemetry_cycle);
    Serial.print(" scene=");
    telemetryPrintScene();
    Serial.println();
    }


static void telemetryEndFrame(uint32_t render_us, uint32_t info_us, uint32_t upload_us)
    {
    telemetry_frames++;
    telemetry_render_sum_us += render_us;
    if (render_us < telemetry_render_min_us) telemetry_render_min_us = render_us;
    if (render_us > telemetry_render_max_us) telemetry_render_max_us = render_us;
    telemetry_info_sum_us += info_us;
    if (info_us < telemetry_info_min_us) telemetry_info_min_us = info_us;
    if (info_us > telemetry_info_max_us) telemetry_info_max_us = info_us;
    telemetry_upload_sum_us += upload_us;
    if (upload_us < telemetry_upload_min_us) telemetry_upload_min_us = upload_us;
    if (upload_us > telemetry_upload_max_us) telemetry_upload_max_us = upload_us;

    const uint32_t now = millis();
    const uint32_t elapsed_ms = now - telemetry_last_ms;
    if (elapsed_ms >= 1000)
        {
        const float render_avg = ((float)telemetry_render_sum_us) / telemetry_frames;
        const float info_avg = ((float)telemetry_info_sum_us) / telemetry_frames;
        const float upload_avg = ((float)telemetry_upload_sum_us) / telemetry_frames;
        Serial.print("\n[TGX telemetry] cycle=");
        Serial.print(telemetry_cycle);
        Serial.print(" scene=");
        telemetryPrintScene();
        Serial.print(" fps=");
        Serial.print((1000.0f * telemetry_frames) / elapsed_ms, 2);
        Serial.print(" frame_avg_us=");
        Serial.print(render_avg + info_avg, 1);
        Serial.print(" frame_min_us=");
        Serial.print(telemetry_render_min_us + telemetry_info_min_us);
        Serial.print(" frame_max_us=");
        Serial.print(telemetry_render_max_us + telemetry_info_max_us);
        Serial.print(" render_avg_us=");
        Serial.print(render_avg, 1);
        Serial.print(" info_avg_us=");
        Serial.print(info_avg, 1);
        Serial.print(" upload_avg_us=");
        Serial.print(upload_avg, 1);
        Serial.print(" info_max_us=");
        Serial.print(telemetry_info_max_us);
        Serial.print(" upload_max_us=");
        Serial.print(telemetry_upload_max_us);
        Serial.print(" frames=");
        Serial.println(telemetry_frames);
        telemetryReset();
        }
    }


static void freeRenderBuffers()
    {
    if (fb) free(fb);
    if (fb2) free(fb2);
    if (zbuf) free(zbuf);
    fb = nullptr;
    fb2 = nullptr;
    zbuf = nullptr;
    use_dma = false;
    }


static bool allocateRenderBuffers()
    {
    const size_t bytes = (size_t)SLX * (size_t)SLY * sizeof(uint16_t);

#if !defined(DISABLE_DMA)
    fb2 = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_DMA);
#endif

    fb = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
    zbuf = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_8BIT);

    if ((fb == nullptr || zbuf == nullptr) && fb2 != nullptr)
        {
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
        }
#endif

    return true;
    }


static fMat4 moveModel(int& loopnumber)
    {
    const float end1 = 6000;
    const float end2 = 2000;
    const float end3 = 6000;
    const float end4 = 2000;

    const int tot = (int)(end1 + end2 + end3 + end4);
    int m = millis();
    loopnumber = m / tot;
    float t = m % tot;
    const float roty = 360 * (t / 4000);
    float tz, ty;
    const float dilat = 6.5f;
    const float nearz = 13;
    const float upy = 2.8f;
    if (t < end1)
        {
        tz = -25;
        ty = 0;
        }
    else
        {
        t -= end1;
        if (t < end2)
            {
            t /= end2;
            tz = -25 + (25 - nearz) * t;
            ty = -upy * t;
            }
        else
            {
            t -= end2;
            if (t < end3)
                {
                tz = -nearz;
                ty = -upy;
                }
            else
                {
                t -= end3;
                t /= end4;
                tz = -nearz - (25 - nearz) * t;
                ty = upy * (t - 1);
                }
            }
        }

    fMat4 M;
    M.setScale({ dilat, dilat, dilat });
    M.multRotate(-roty, { 0, 1, 0 });
    M.multTranslate({ 0, ty, tz });
    return M;
    }


static void infos(int loopnumber)
    {
    static int prev_loopnumber = -1;
    static uint32_t prev_millis = 0;
    static int nbframes = -1;
    uint32_t m = millis();
    nbframes++;
    if (prev_loopnumber != loopnumber)
        {
        prev_loopnumber = loopnumber;
        lcd.setTextDatum(BL_DATUM);
        if (use_dma) lcd.waitDMA();
        switch (loopnumber % 4)
            {
            case 0: lcd.drawString("Gouraud/texture", 0, lcd.height() - 1); telemetrySetScene("gouraud_texture"); break;
            case 1: lcd.drawString("Wireframe      ", 0, lcd.height() - 1); telemetrySetScene("wireframe"); break;
            case 2: lcd.drawString("Flat Shading   ", 0, lcd.height() - 1); telemetrySetScene("flat"); break;
            case 3: lcd.drawString("Gouraud shading", 0, lcd.height() - 1); telemetrySetScene("gouraud"); break;
            }
        }
    if (m > prev_millis + 1000)
        {
        char tfps[10] = "FPS:     ";
        itoa(nbframes, tfps + 5, 10);
        tfps[strlen(tfps)] = ' ';
        lcd.setTextDatum(TL_DATUM);
        if (use_dma) lcd.waitDMA();
        lcd.drawString(tfps, 0, 0);
        prev_millis = m;
        nbframes = 0;
        }
    }


static void uploadFrame()
    {
    const int x = (lcd.width() - SLX) / 2;
    const int y = (lcd.height() - SLY) / 2;
    if (use_dma)
        {
        lcd.waitDMA();
        for (int i = 0; i < SLX * SLY; i++)
            {
            const uint16_t a = fb[i];
            fb2[i] = (a << 8) | (a >> 8);
            }
        lcd.pushImageDMA(x, y, SLX, SLY, fb2);
        }
    else
        {
        lcd.pushImage(x, y, SLX, SLY, fb);
        }
    }


void setup()
    {
    Serial.begin(115200);
    delay(800);
    Serial.println("TGX_DONKEY_INFOS_PROBE_BEGIN");

    lcd.init();
    lcd.setRotation(1);
    lcd.setBrightness(200);
    lcd.setColorDepth(16);
    lcd.setTextFont(1);
    lcd.fillScreen(TFT_BLACK);

    while (!allocateRenderBuffers())
        {
        Serial.println("TGX_DONKEY_INFOS_PROBE_ALLOC_FAILED");
        delay(1000);
        }
    lcd.setSwapBytes(!use_dma);

    imfb.set(fb, SLX, SLY);
    renderer.setViewportSize(SLX, SLY);
    renderer.setOffset(0, 0);
    renderer.setImage(&imfb);
    renderer.setZbuffer(zbuf);
    renderer.setPerspective(45.0f, ((float)SLX) / SLY, 1.0f, 100.0f);
    renderer.setMaterial(RGBf(0.85f, 0.55f, 0.25f), 0.2f, 0.7f, 0.8f, 64);
    renderer.setCulling(1);
    renderer.setTextureQuality(SHADER_TEXTURE_NEAREST);
    renderer.setTextureWrappingMode(SHADER_TEXTURE_WRAP_POW2);

    Serial.print("TGX_DONKEY_INFOS_PROBE_RENDERER_SIZE=");
    Serial.println(sizeof(renderer));
    Serial.print("TGX_DONKEY_INFOS_PROBE_DISPLAY=");
    Serial.println(use_dma ? "DMA" : "pushImage");
    Serial.println("TGX_DONKEY_INFOS_PROBE_READY");
    telemetryReset();
    }


void loop()
    {
    int loopnumber = 0;
    const fMat4 M = moveModel(loopnumber);
    const int mode = loopnumber & 3;

    const uint32_t render_start_us = micros();
    renderer.setModelMatrix(M);
    imfb.fillScreen(RGB565_Cyan);
    renderer.clearZbuffer();
    switch (mode)
        {
        case 0:
            renderer.setShaders(SHADER_GOURAUD | SHADER_TEXTURE);
            renderer.drawMesh(&donkeykong_small, false);
            break;
        case 1:
            renderer.drawWireFrameMeshAA(&donkeykong_small);
            break;
        case 2:
            renderer.setShaders(SHADER_FLAT);
            renderer.drawMesh(&donkeykong_small, false);
            break;
        default:
            renderer.setShaders(SHADER_GOURAUD);
            renderer.drawMesh(&donkeykong_small, false);
            break;
        }
    const uint32_t render_us = micros() - render_start_us;

    const uint32_t info_start_us = micros();
    infos(loopnumber);
    const uint32_t info_us = micros() - info_start_us;

    const uint32_t upload_start_us = micros();
    uploadFrame();
    const uint32_t upload_us = micros() - upload_start_us;

    telemetryEndFrame(render_us, info_us, upload_us);
    }
