/********************************************************************
*
* tgx library example
* 
* Minimal example for displaying a mesh. 
* Happy buddha  (20 000 triangles) at 30FPS on an ILI9341 screen with
* v-sync on (i.e. no screen tearing)
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

// Let's not burden ourselves with the tgx:: prefix.
using namespace tgx;

// The mesh to display.
#include "buddha.h"


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



// 20 MHz is enough to get a stable 30 FPS because we are using differential updates.
// With full redraws, 20 MHz SPI would be capped at 17 FPS.
#define SPI_SPEED       20000000


// The screen driver object.
ILI9341_T4::ILI9341Driver tft(PIN_CS, PIN_DC, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_RESET, PIN_TOUCH_CS, PIN_TOUCH_IRQ);

// 2 x 3K diff buffers used by tft for differential updates.
DMAMEM ILI9341_T4::DiffBuffStatic<3000> diff1;
DMAMEM ILI9341_T4::DiffBuffStatic<3000> diff2;

// Screen dimension (portrait mode).
static const int SLX = 240;
static const int SLY = 320;

// Main screen framebuffer (150K in DMAMEM).
DMAMEM uint16_t fb[SLX * SLY];

// Internal framebuffer (150K in DMAMEM) used by the ILI9341_T4 library for double buffering.
DMAMEM uint16_t internal_fb[SLX * SLY];

// Z-buffer in 16-bit precision (150K in DMAMEM).
DMAMEM uint16_t zbuf[SLX * SLY];           

// Image that encapsulates fb.
Image<RGB565> im(fb, SLX, SLY);

// We only use Gouraud shading with perspective projection and a z-buffer.
const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD;

// The renderer object that performs the 3D drawings.
Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;


// DTCM buffer used to cache the Mesh3Dv2 payload and metadata into RAM.
// This is faster than progmem and keeps the mesh payload contiguous.

const int DTCM_buf_size = 330000; // adjust this value to fill unused DTCM but leave enough room for the stack.
char buf_DTCM[DTCM_buf_size];

// Pointer to the cached mesh.
const Mesh3Dv2<RGB565> * buddha_cached;


#include "tgx_example_telemetry.h"

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
    tft.setRotation(0); // portrait mode
    tft.setFramebuffer(internal_fb); // double buffering
    tft.setDiffBuffers(&diff1, &diff2); // 2 diff buffers
    tft.setDiffGap(4); // small gap
    tft.setRefreshRate(60); // refresh at 60hz
    tft.setVSyncSpacing(2); // lock the framerate at 60/2 = 30fps. 

    // setup the 3D renderer.
    renderer.setViewportSize(SLX,SLY); // viewport = screen
    renderer.setOffset(0, 0); //  image = viewport
    renderer.setImage(&im); // set the image to draw onto (ie the screen framebuffer)
    renderer.setZbuffer(zbuf); // set the z buffer for depth testing
    renderer.setPerspective(45, ((float)SLX) / SLY, 1.0f, 100.0f);  // set the perspective projection matrix.     
    renderer.setMaterial(RGBf(0.85f, 0.55f, 0.25f), 0.2f, 0.7f, 0.8f, 64); // bronze color with a lot of specular reflection.
    renderer.setShaders(SHADER_GOURAUD);

    // cache the mesh payload and small metadata arrays in RAM.
    buddha_cached = tgx::cacheMesh(&buddha, buf_DTCM, DTCM_buf_size);
    telemetrySetScene("buddha_rotation");

    }


float a = 0;  // current angle

void loop()
    {
    telemetryStartFrame();

    // erase the screen
    im.fillScreen(RGB565_Blue);

    // clear the z-buffer
    renderer.clearZbuffer();

    // position the model
    renderer.setModelPosScaleRot({ 0, 0.5f, -35 }, { 13,13,13 }, a);

    // draw the model onto the memory framebuffer
    renderer.drawMesh(buddha_cached, false);

    // overlay FPS counter on the framebuffer
    tft.overlayFPS(fb); 
    
    // update the screen (asynchronous). 
    tft.update(fb);

    telemetryEndFrame();
  
    // increase the angle by 3 degrees.
    a += 3;
    if (a >= 360)
        {
        a -= 360;
        telemetrySetScene("buddha_rotation");
        }
    }
