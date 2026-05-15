/********************************************************************
*
* tgx library example : comparing flat vs Gouraud shading.
*
* EXAMPLE FOR TEENSY 4 / 4.1
*
* DISPLAY: ILI9341 (320x240)
*
********************************************************************/

// This example runs on teensy 4.0/4.1 with ILI9341 via SPI.
// the screen driver library : https://github.com/vindar/ILI9341_T4
#include <ILI9341_T4.h>

// the tgx library
#include <tgx.h>
#include <font_tgx_OpenSans_Bold.h>


// let's not burden ourselves with the tgx:: prefix
using namespace tgx;


// meshes (stored in PROGMEM)
#if __has_include("teapot.h")
    // Arduino IDE stupidly flattens the example directory tree...
    #include "teapot.h"
    #include "skull.h"
    #include "suzanne.h"
    #include "bunny.h"
    #include "dragon.h"
#else
    // ok, use the normal path
    #include "3Dmodels/teapot/teapot.h"
    #include "3Dmodels/skull/skull.h"
    #include "3Dmodels/suzanne/suzanne.h"
    #include "3Dmodels/bunny/bunny.h"
    #include "3Dmodels/dragon/dragon.h"
#endif



//
// DEFAULT WIRING USING SPI 0 ON TEENSY 4/4.1
//
#define PIN_SCK     13      // mandatory
#define PIN_MISO    12      // mandatory
#define PIN_MOSI    11      // mandatory
#define PIN_DC      10      // mandatory, can be any pin but using pin 10 (or 36 or 37 on T4.1) provides greater performance

#define PIN_CS      9       // optional (but recommended), can be any pin.
#define PIN_RESET   6       // optional (but recommended), can be any pin.
#define PIN_BACKLIGHT 255   // optional, set this only if the screen LED pin is connected directly to the Teensy.
#define PIN_TOUCH_IRQ 255   // optional. set this only if the touchscreen is connected on the same SPI bus
#define PIN_TOUCH_CS  255   // optional. set this only if the touchscreen is connected on the same spi bus


//
// ALTERNATE WIRING USING SPI 1 ON TEENSY 4/4.1
//
//#define PIN_SCK     27      // mandatory
//#define PIN_MISO    1       // mandatory
//#define PIN_MOSI    26      // mandatory
//#define PIN_DC      0       // mandatory, can be any pin but using pin 0 (or 38 on T4.1) provides greater performance

//#define PIN_CS      30      // optional (but recommended), can be any pin.
//#define PIN_RESET   29      // optional (but recommended), can be any pin.
//#define PIN_BACKLIGHT 255   // optional, set this only if the screen LED pin is connected directly to the Teensy.
//#define PIN_TOUCH_IRQ 255   // optional. set this only if the touchscreen is connected on the same SPI bus
//#define PIN_TOUCH_CS  255   // optional. set this only if the touchscreen is connected on the same spi bus


// 30MHz SPI, we can go higher with short wires
#define SPI_SPEED       30000000


// the screen driver object
ILI9341_T4::ILI9341Driver tft(PIN_CS, PIN_DC, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_RESET, PIN_TOUCH_CS, PIN_TOUCH_IRQ);


// 2 x 8K diff buffers (used by tft) for differential updates
ILI9341_T4::DiffBuffStatic<8000> diff1;
ILI9341_T4::DiffBuffStatic<8000> diff2;

// screen dimension (landscape mode)
static const int SLX = 320;
static const int SLY = 240;

// main screen framebuffer (150K in DTCM)
uint16_t fb[SLX * SLY];

// internal framebuffer (150K in DMAMEM) used by the ILI9431_T4 library for double buffering.
DMAMEM uint16_t internal_fb[SLX * SLY];

// zbuffer in 16 bits precision (150K in DTCM)
DMAMEM uint16_t zbuf[SLX * SLY];

// image that encapsulates fb.
Image<RGB565> im(fb, SLX, SLY);



// only load the shaders we need.
const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT | SHADER_GOURAUD;

// the renderer object that performs the 3D drawings
Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;



// DTCM and DMAMEM buffers used to cache meshes into RAM
// which is faster than progmem: caching may lead to significant speedup.

const int DTCM_buf_size = 160000; // adjust this value to fill unused DTCM but leave at least 20K for the stack to be sure
char buf_DTCM[DTCM_buf_size];

const int DMAMEM_buf_size = 190000; // adjust this value to fill unused DMAMEM,  leave at least 10k for additional serial objects.
DMAMEM char buf_DMAMEM[DMAMEM_buf_size];

const tgx::Mesh3D<tgx::RGB565> * cached_mesh; // pointer to the currently cached mesh.


// Print per-second FPS and frame timing on Serial.
void telemetryBegin();
void telemetryStartFrame();
void telemetryEndFrame();

void telemetrySetScene(const char* scene);




/**
* Overlay some info about the current mesh on the screen
**/
void drawInfo(tgx::Image<tgx::RGB565>& im, int t, const tgx::Mesh3D<tgx::RGB565>& mesh)  // remark: need to keep the tgx:: prefix in function signatures because arduino messes with ino files....
    {
    // count the number of triangles in the mesh (by iterating over linked meshes)
    const Mesh3D<RGB565>* m = &mesh;
    int nbt = 0;
    while (m != nullptr)
        {
        nbt += m->nb_faces;
        m = m->next;
        }
    // display some info
    char buf[80];
    im.drawText((mesh.name != nullptr ? mesh.name : "[unnamed mesh]"), { 3,12 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
    sprintf(buf, "%d triangles", nbt);
    im.drawText(buf, { 3,SLY - 21 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
    sprintf(buf, "%s", (t == 0) ? "Wireframe" : ((t == 1) ? "Flat shading" : "Gouraud shading"));
    im.drawText(buf, { 3, SLY - 5 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
    }



void setup()
    {
    Serial.begin(9600);
    telemetryBegin();

    // initialize the ILI9341 screen
    while (!tft.begin(SPI_SPEED));

    // ok. turn on backlight
    pinMode(PIN_BACKLIGHT, OUTPUT);
    digitalWrite(PIN_BACKLIGHT, HIGH);

    // setup the screen driver
    tft.setRotation(3); // landscape
    tft.setFramebuffer(internal_fb); // double buffering
    tft.setDiffBuffers(&diff1, &diff2); // 2 diff buffers
    tft.setDiffGap(4); // small gap
    tft.setVSyncSpacing(0); // do not use v-sync because we want to measure the max framerate;
    tft.setRefreshRate(140); // max refreshrate

    // setup the 3D renderer.
    renderer.setViewportSize(SLX,SLY); // viewport = screen
    renderer.setOffset(0, 0); //  image = viewport
    renderer.setImage(&im); // set the image to draw onto (ie the screen framebuffer)
    renderer.setZbuffer(zbuf); // set the z buffer for depth testing
    renderer.setPerspective(45, ((float)SLX) / SLY, 1.0f, 100.0f);  // set the perspective projection matrix.
    renderer.setCulling(1);
    }


void drawMesh(const Mesh3D<RGB565>* mesh, float scale, const char* scene_wireframe, const char* scene_flat, const char* scene_gouraud, float tilt = 0.0f)
{
    // cache the first mesh to display in RAM to improve framerate
    cached_mesh  = tgx::cacheMesh(mesh, buf_DTCM, DTCM_buf_size,  buf_DMAMEM, DMAMEM_buf_size);

    const int maxT = 12000; // display model for 12 seconds.
    elapsedMillis em = 0;
    int prev_t = -1;
    while (em < maxT)
        {
        telemetryStartFrame();

        // erase the screen
        im.fillScreen(RGB565_Black);

        // clear the z buffer
        renderer.clearZbuffer();

        // move the model to it correct position (depending on the current time).
        fMat4 M;
        M.setScale(scale, scale, scale);
        M.multRotate(tilt, { 0,0,1 }); // 4 rotations per display
        M.multRotate((1440.0f * em) / maxT, { 0,1,0 }); // 4 rotations per display
        M.multTranslate({ 0,0, -40 });
        renderer.setModelMatrix(M);

        // change shader type after every turn
        int t = (((em * 3) / maxT) % 3);
        if (t != prev_t)
            {
            prev_t = t;
            telemetrySetScene((t == 0) ? scene_wireframe : ((t == 1) ? scene_flat : scene_gouraud));
            }

        if (t == 0)
            renderer.drawWireFrameMesh(cached_mesh);
        else if (t == 1)
            {
            renderer.setShaders(SHADER_FLAT);
            renderer.drawMesh(cached_mesh, false);
            }
        else
            {
            renderer.setShaders(SHADER_GOURAUD);
            renderer.drawMesh(cached_mesh, false);
            }

        // overlay some info
        drawInfo(im, t, *cached_mesh);

        // and the current framerate
        tft.overlayFPS(fb);

        // update the screen (asynchronously)
        tft.update(fb);

        telemetryEndFrame();
        }
}


void loop()
{
    renderer.setMaterial(RGBf(0.15f, 0.7f, 0.39f), 0.2f, 0.8f, 0.5f, 8); // teapot
    drawMesh(&teapot, 15, "teapot_wireframe", "teapot_flat", "teapot_gouraud", 30);

    renderer.setMaterial(RGBf(1.0f, 1.0f, 1.0f), 0.15f, 0.7f, 0.8f, 48); // bunny
    drawMesh(&bunny, 12, "bunny_wireframe", "bunny_flat", "bunny_gouraud");

    renderer.setMaterial(RGBf(166 / 256.0f, 130 / 256.0f, 110.0f / 256.0f), 0.15f, 0.7f, 0.4f, 16); // skull
    drawMesh(&skull_1, 12, "skull_wireframe", "skull_flat", "skull_gouraud");

    // let's have some fun with lighting
    renderer.setLightAmbiant({ 0, 0, 1.0f });  // blue
    renderer.setLightDiffuse({ 1.0f, 0, 0 });  // red
    renderer.setLightSpecular({ 1.0f, 1.0f, 1.0f }); // white

    renderer.setMaterial(RGBf(1.0f, 1.0f, 1.0f), 0.2f, 0.8f, 0.8f, 32); // suzanne
    drawMesh(&suzanne, 13, "suzanne_wireframe", "suzanne_flat", "suzanne_gouraud");

    // back to normal lighting
    renderer.setLightAmbiant({ 1.0f, 1.0f, 1.0f }); // white
    renderer.setLightDiffuse({ 1.0f, 1.0f, 1.0f }); // white
    renderer.setLightSpecular({ 1.0f, 1.0f, 1.0f }); // white

    renderer.setMaterial(RGBf(0.85f, 0.55f, 0.25f), 0.2f, 0.7f, 0.8f, 64); // dragon
    drawMesh(&dragon, 15, "dragon_wireframe", "dragon_flat", "dragon_gouraud");

    // chooose new random light orientation.
    const float angle = M_PI * random(0, 360) / 180.0f;
    renderer.setLightDirection({ cosf(angle) , sinf(angle) , -0.3f });
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

