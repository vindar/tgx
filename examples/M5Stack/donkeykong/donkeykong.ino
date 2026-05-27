/********************************************************************
* tgx library example : displaying the Donkey Kong 3D mesh...
*
*              EXAMPLE FOR M5STACK Core2 and CoreS3
*
* Instructions:
*
* 1. download and install LovyanGFX library via Arduino's library manager or
*    directly from: https://github.com/lovyan03/LovyanGFX
*
* 2. Select the board model and serial port and upload the sketch
*
* ---
* This example was tested with M5Stack Core2 and M5Stack CoreS3.
********************************************************************/

/**************** DMA NOTE ****************
* LovyanGFX DMA transfers can improve display upload speed on M5Stack boards.
* DMA is enabled by default here.  The sketch first tries to allocate a
* DMA-capable transfer buffer; if that fails, it automatically falls back to
* the blocking pushImage() path.  Uncomment the line below to force that
* non-DMA path.
*******************************************/
// #define DISABLE_DMA

// the screen display library (Lovyan is fast and specifically tailored to work well with M5Stack)
#define LGFX_AUTODETECT   // detect board automatically: works with M5Stack Core2/CoreS3
#include <LovyanGFX.hpp>  // the Lovyan library: https://github.com/lovyan03/LovyanGFX

// graphic library
#include <tgx.h>

// the mesh to draw
#include "donkeykong_small.h"
#define MESH &donkeykong_small

// let's not burden ourselves with the tgx:: prefix
using namespace tgx;

// drawing size (limited by the amount of RAM on ESP32/ESP32S3)
#define SLX 240
#define SLY 180

// the framebuffer we draw onto, the optional DMA transfer buffer, and the
// z-buffer in 16 bits precision.
uint16_t* fb = nullptr;
uint16_t* fb2 = nullptr;
uint16_t* zbuf = nullptr;
bool use_dma = false;

// the tgx::image object that encapsulate framebuffer fb
Image<RGB565> imfb;

// we only load the shaders we need.
const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT | SHADER_GOURAUD | SHADER_NOTEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2;

// the renderer object that performs the 3D drawings
Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;

// LovyanGFX display object used to draw the framebuffer on the screen
LGFX lcd;


// Print per-second FPS and frame timing on Serial.
void telemetryBegin();
void telemetryStartFrame();
void telemetryEndFrame();

void telemetrySetScene(const char* scene);


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


bool allocateRenderBuffers()
    {
    const size_t bytes = (size_t)SLX * (size_t)SLY * sizeof(uint16_t);

#if !defined(DISABLE_DMA)
    // DMA needs a contiguous DMA-capable internal RAM block.  Allocate it
    // first, then fall back cleanly if keeping it prevents fb/zbuf allocation.
    fb2 = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_DMA);
#endif

    fb = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
    zbuf = (uint16_t*)heap_caps_malloc(bytes, MALLOC_CAP_8BIT);

    if ((fb == nullptr || zbuf == nullptr) && fb2 != nullptr)
        {
        Serial.println("donkeykong M5Stack DMA buffer released to keep framebuffer/zbuffer");
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
        Serial.println("donkeykong M5Stack display DMA enabled");
        }
    else
        {
        Serial.println("donkeykong M5Stack display DMA unavailable, using pushImage");
        }
#else
    Serial.println("donkeykong M5Stack display DMA disabled, using pushImage");
#endif

    return true;
    }


void setup(void)
    {
    Serial.begin(115200);
    telemetryBegin();

    // setup the screen driver
    lcd.init();
    lcd.setRotation(1);
    lcd.setBrightness(200);
    lcd.setColorDepth(16);
    lcd.setTextFont(1);
    lcd.fillScreen(TFT_BLACK);

    while (!allocateRenderBuffers())
        {
        Serial.println("Error: cannot allocate framebuffer/zbuffer");
        delay(1000);
        }
    // LovyanGFX byteswapping is convenient for pushImage(), but DMA needs the
    // pixels already byteswapped in the transfer buffer.
    lcd.setSwapBytes(!use_dma);

    // setup the 3D renderer.
    imfb.set(fb, SLX, SLY);
    renderer.setViewportSize(SLX, SLY);
    renderer.setOffset(0, 0);
    renderer.setImage(&imfb);                                               // set the image to draw onto (ie the screen framebuffer)
    renderer.setZbuffer(zbuf);                                              // set the z buffer for depth testing
    renderer.setPerspective(45, ((float)SLX) / SLY, 1.0f, 100.0f);          // set the perspective projection matrix.
    renderer.setMaterial(RGBf(0.85f, 0.55f, 0.25f), 0.2f, 0.7f, 0.8f, 64);  // bronze color with a lot of specular reflexion.
    renderer.setCulling(1);
    renderer.setTextureQuality(SHADER_TEXTURE_NEAREST);
    renderer.setTextureWrappingMode(SHADER_TEXTURE_WRAP_POW2);
    }



/** Compute the model matrix according to the current time */
tgx::fMat4 moveModel(int &loopnumber)
    {
    const float end1 = 6000;
    const float end2 = 2000;
    const float end3 = 6000;
    const float end4 = 2000;

    int tot = (int)(end1 + end2 + end3 + end4);
    int m = millis();

    loopnumber = m / tot;
    float t = m % tot;
    const float roty = 360 * (t / 4000);  // rotate 1 turn every 4 seconds
    float tz, ty;
    const float dilat = 6.5;                // scale model
    const float nearz = 13; // how much we zoom in
    const float upy = 2.8; // how much we move up to focus on the head
    if (t < end1) {  // far away
        tz = -25;
        ty = 0;
    } else {
        t -= end1;
        if (t < end2) {  // zooming in
            t /= end2;
            tz = -25 + (25-nearz) * t;
            ty = -upy * t;
        } else {
            t -= end2;
            if (t < end3) {  // close up
                tz = -nearz;
                ty = -upy;
            } else {  // zooming out
                t -= end3;
                t /= end4;
                tz = -nearz - (25 -nearz) * t;
                ty = upy*(t-1);
            }
        }
    }
    fMat4 M;
    M.setScale({ dilat, dilat, dilat });  // scale the model
    M.multRotate(-roty, { 0, 1, 0 });     // rotate around y
    M.multTranslate({ 0, ty, tz });       // translate
    return M;
    }


/** Display additional infos on the screen (drawing mode and fps) **/
void infos(int loopnumber)
    {
    static int prev_loopnumber = -1;
    static uint32_t prev_millis = 0;
    static int nbframes = -1;
    uint32_t m = millis();
    nbframes++;
    if (prev_loopnumber != loopnumber)
        { // update the text for the drawing mode
        prev_loopnumber = loopnumber;
        lcd.setTextDatum(BL_DATUM);
        if (use_dma) lcd.waitDMA();
        switch (loopnumber % 4)
            {
            case 0: lcd.drawString("Gouraud/texture", 0, lcd.height()-1); telemetrySetScene("gouraud_texture"); break;
            case 1: lcd.drawString("Wireframe      ", 0, lcd.height()-1); telemetrySetScene("wireframe"); break;
            case 2: lcd.drawString("Flat Shading   ", 0, lcd.height()-1); telemetrySetScene("flat"); break;
            case 3: lcd.drawString("Gouraud shading", 0, lcd.height()-1); telemetrySetScene("gouraud"); break;
            }
        }
    if (m > prev_millis + 1000)  // every second
        { // update the fps counter
        char tfps[10] = "FPS:     ";
        itoa(nbframes,tfps + 5,10);
        tfps[strlen(tfps)] = ' ';
        lcd.setTextDatum(TL_DATUM);
        if (use_dma) lcd.waitDMA();
        lcd.drawString(tfps,0,0);
        Serial.print("donkeykong M5Stack display=");
        Serial.println(use_dma ? "DMA" : "pushImage");
        prev_millis = m;
        nbframes = 0;
        }
    }



int loopnumber = 0;

/** Main loop */
void loop()
    {
    telemetryStartFrame();

    // compute the model position
    fMat4 M = moveModel(loopnumber);
    renderer.setModelMatrix(M);

    // draw the 3D mesh
    imfb.fillScreen(RGB565_Cyan);  // clear the framebuffer (black background)
    renderer.clearZbuffer();       // clear the z-buffer

    // choose the shader to use and perform the drawing
    switch (loopnumber % 4)
        {
        case 0:
            renderer.setShaders(SHADER_GOURAUD | SHADER_TEXTURE);
            renderer.drawMesh(MESH, false);
            break;
        case 1:
            renderer.drawWireFrameMesh(MESH);
            break;
        case 2:
            renderer.setShaders(SHADER_FLAT);
            renderer.drawMesh(MESH, false);
            break;
        case 3:
            renderer.setShaders(SHADER_GOURAUD);
            renderer.drawMesh(MESH, false);
            break;
        }

    // display additional information on the screen
    infos(loopnumber);

    const int x = (lcd.width() - SLX) / 2;
    const int y = (lcd.height() - SLY) / 2;
    if (use_dma)
        {
        lcd.waitDMA();
        // Copy fb to fb2 with byteswapping.  The DMA buffer may not be safely
        // byte-addressable, so keep the conversion word-based.
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

    telemetryEndFrame();
    }


// Print per-second FPS and frame timing on Serial.
uint32_t telemetry_last_ms = 0;
uint32_t telemetry_frame_start_us = 0;
uint32_t telemetry_frames = 0;
uint32_t telemetry_sum_us = 0;
uint32_t telemetry_min_us = 0xFFFFFFFFu;
uint32_t telemetry_max_us = 0;
uint32_t telemetry_cycle = 0;
const char* telemetry_scene = "startup";

void telemetryBegin()
    {
    telemetry_last_ms = millis();
    telemetry_frames = 0;
    telemetry_sum_us = 0;
    telemetry_min_us = 0xFFFFFFFFu;
    telemetry_max_us = 0;
    }


void telemetryStartFrame()
    {
    if (telemetry_frames == 0) telemetry_last_ms = millis();
    telemetry_frame_start_us = micros();
    }


static void telemetryPrintScene()
    {
    for (const char* p = telemetry_scene; *p != 0; p++)
        {
        const char c = *p;
        Serial.print((c <= ' ' || c == '=') ? '_' : c);
        }
    }

void telemetrySetScene(const char* scene)
    {
    if (scene == nullptr) scene = "unnamed";
    telemetry_scene = scene;
    telemetry_cycle++;
    telemetry_last_ms = millis();
    telemetry_frames = 0;
    telemetry_sum_us = 0;
    telemetry_min_us = 0xFFFFFFFFu;
    telemetry_max_us = 0;
    Serial.print("\n[TGX scene] cycle=");
    Serial.print(telemetry_cycle);
    Serial.print(" scene=");
    telemetryPrintScene();
    Serial.println();
    }


void telemetryEndFrame()
    {
    const uint32_t dt = micros() - telemetry_frame_start_us;
    telemetry_frames++;
    telemetry_sum_us += dt;
    if (dt < telemetry_min_us) telemetry_min_us = dt;
    if (dt > telemetry_max_us) telemetry_max_us = dt;

    const uint32_t now = millis();
    const uint32_t elapsed_ms = now - telemetry_last_ms;
    if (elapsed_ms >= 1000)
        {
        Serial.print("\n[TGX telemetry] cycle=");
        Serial.print(telemetry_cycle);
        Serial.print(" scene=");
        telemetryPrintScene();
        Serial.print(" fps=");
        Serial.print((1000.0f * telemetry_frames) / elapsed_ms, 2);
        Serial.print(" frame_avg_us=");
        Serial.print(((float)telemetry_sum_us) / telemetry_frames, 1);
        Serial.print(" frame_min_us=");
        Serial.print(telemetry_min_us);
        Serial.print(" frame_max_us=");
        Serial.print(telemetry_max_us);
        Serial.print(" frames=");
        Serial.println(telemetry_frames);

        telemetry_last_ms = now;
        telemetry_frames = 0;
        telemetry_sum_us = 0;
        telemetry_min_us = 0xFFFFFFFFu;
        telemetry_max_us = 0;
        }
    }

/** end of file */


