/********************************************************************
*
* tgx library example: comparing wireframe, flat and Gouraud shading.
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
#include <font_tgx_OpenSans_Bold.h>


// Let's not burden ourselves with the tgx:: prefix.
using namespace tgx;


// meshes (stored in PROGMEM)
#if __has_include("teapot.h")
    // Arduino IDE may flatten the example directory tree.
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


// 30 MHz SPI, we can go higher with short wires.
#define SPI_SPEED       30000000


// The screen driver object.
ILI9341_T4::ILI9341Driver tft(PIN_CS, PIN_DC, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_RESET, PIN_TOUCH_CS, PIN_TOUCH_IRQ);


// 2 x 8K diff buffers used by tft for differential updates.
DMAMEM ILI9341_T4::DiffBuffStatic<8000> diff1;
DMAMEM ILI9341_T4::DiffBuffStatic<8000> diff2;

// Screen dimension (landscape mode).
static const int SLX = 320;
static const int SLY = 240;

// Main screen framebuffer (150K in DMAMEM).
DMAMEM uint16_t fb[SLX * SLY];

// Internal framebuffer (150K in DMAMEM) used by the ILI9341_T4 library for double buffering.
DMAMEM uint16_t internal_fb[SLX * SLY];

// Z-buffer in 16-bit precision (150K in DMAMEM).
DMAMEM uint16_t zbuf[SLX * SLY];

// Image that encapsulates fb.
Image<RGB565> im(fb, SLX, SLY);



// Only load the shaders we need.
const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT | SHADER_GOURAUD;

// The renderer object that performs the 3D drawings.
Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;



// DTCM buffer used to cache the Mesh3Dv2 payload and metadata into RAM.
// This is faster than progmem and keeps the mesh payload contiguous.
static const size_t DTCM_buf_size = 330000; // adjust this value to fill unused DTCM but leave enough room for the stack.
char buf_DTCM[DTCM_buf_size];


#include "tgx_example_telemetry.h"

/**
* Overlay some info about the current mesh on the screen
**/
void drawInfo(tgx::Image<tgx::RGB565>& im, int t, const char* mesh_name, int nb_triangles)  // remark: need to keep the tgx:: prefix in function signatures because arduino messes with ino files....
    {
    // display some info
    char buf[80];
    im.drawText((mesh_name != nullptr ? mesh_name : "[unnamed mesh]"), { 3,12 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
    sprintf(buf, "%d triangles", nb_triangles);
    im.drawText(buf, { 3,SLY - 21 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
    sprintf(buf, "%s", (t == 0) ? "Wireframe AA" : ((t == 1) ? "Flat shading" : "Gouraud shading"));
    im.drawText(buf, { 3, SLY - 5 }, font_tgx_OpenSans_Bold_10, RGB565_Red);
    }



void setup()
    {
    Serial.begin(9600);
    telemetryBegin();

    // initialize the ILI9341 screen.
    while (!tft.begin(SPI_SPEED));

    // turn on backlight.
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


void drawModel(const Mesh3Dv2<RGB565>* mesh, int nb_triangles, float scale, const char* scene_wireframe, const char* scene_flat, const char* scene_gouraud, float tilt = 0.0f)
{
    // cache the mesh payload and small metadata arrays in RAM to improve framerate.
    const Mesh3Dv2<RGB565>* cached_mesh = tgx::cacheMesh(mesh, buf_DTCM, DTCM_buf_size);

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
            renderer.drawWireFrameMeshAA(cached_mesh);
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
        drawInfo(im, t, cached_mesh->name, nb_triangles);

        // and the current framerate
        tft.overlayFPS(fb);

        telemetryEndFrame();

        // update the screen (asynchronously)
        tft.update(fb);
        }
}


void loop()
{
    renderer.setMaterial(RGBf(0.15f, 0.7f, 0.39f), 0.2f, 0.8f, 0.5f, 8); // teapot
    drawModel(&teapot, 2256, 15, "teapot_wireframe_aa", "teapot_flat", "teapot_gouraud", 30);

    renderer.setMaterial(RGBf(1.0f, 1.0f, 1.0f), 0.15f, 0.7f, 0.8f, 48); // bunny
    drawModel(&bunny, 4968, 12, "bunny_wireframe_aa", "bunny_flat", "bunny_gouraud");

    renderer.setMaterial(RGBf(166 / 256.0f, 130 / 256.0f, 110.0f / 256.0f), 0.15f, 0.7f, 0.4f, 16); // skull
    drawModel(&skull_1, 9535, 12, "skull_wireframe_aa", "skull_flat", "skull_gouraud");

    // let's have some fun with lighting
    renderer.setLightAmbiant({ 0, 0, 1.0f });  // blue
    renderer.setLightDiffuse({ 1.0f, 0, 0 });  // red
    renderer.setLightSpecular({ 1.0f, 1.0f, 1.0f }); // white

    renderer.setMaterial(RGBf(1.0f, 1.0f, 1.0f), 0.2f, 0.8f, 0.8f, 32); // suzanne
    drawModel(&suzanne, 15744, 13, "suzanne_wireframe_aa", "suzanne_flat", "suzanne_gouraud");

    // back to normal lighting
    renderer.setLightAmbiant({ 1.0f, 1.0f, 1.0f }); // white
    renderer.setLightDiffuse({ 1.0f, 1.0f, 1.0f }); // white
    renderer.setLightSpecular({ 1.0f, 1.0f, 1.0f }); // white

    renderer.setMaterial(RGBf(0.85f, 0.55f, 0.25f), 0.2f, 0.7f, 0.8f, 64); // dragon
    drawModel(&dragon, 23000, 15, "dragon_wireframe_aa", "dragon_flat", "dragon_gouraud");

    // choose new random light orientation.
    const float angle = M_PI * random(0, 360) / 180.0f;
    renderer.setLightDirection({ cosf(angle) , sinf(angle) , -0.3f });
}
