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


// This example runs on teensy 4.0/4.1 with ILI9341 via SPI. 
// the screen driver library : https://github.com/vindar/ILI9341_T4
#include <ILI9341_T4.h> 

// the tgx library 
#include <tgx.h>

// let's not burden ourselves with the tgx:: prefix
using namespace tgx;

// the mesh to display
#include "3Dmodels/buddha/buddha.h"


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



// 20mhz is enough to get a stable 30 FPS because we are using differential updates. 
// (with full redraws, 20mhz SPI would caps at 17 FPS)
#define SPI_SPEED       20000000


// the screen driver object
ILI9341_T4::ILI9341Driver tft(PIN_CS, PIN_DC, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_RESET, PIN_TOUCH_CS, PIN_TOUCH_IRQ);

// 2 x 3K diff buffers (used by tft) for differential updates
DMAMEM ILI9341_T4::DiffBuffStatic<3000> diff1;
DMAMEM ILI9341_T4::DiffBuffStatic<3000> diff2;

// screen dimension (portrait mode)
static const int SLX = 240;
static const int SLY = 320;

// main screen framebuffer (150K in DTCM)
uint16_t fb[SLX * SLY];

// internal framebuffer (150K in DMAMEM) used by the ILI9431_T4 library for double buffering.
DMAMEM uint16_t internal_fb[SLX * SLY];

// zbuffer in 16bits precision (150K in DMAMEM)
DMAMEM uint16_t zbuf[SLX * SLY];           

// image that encapsulates fb.
Image<RGB565> im(fb, SLX, SLY);

// we only use Gouraud shading with perspective projection and a z-buffer
const int LOADED_SHADERS = TGX_SHADER_PERSPECTIVE | TGX_SHADER_ZBUFFER | TGX_SHADER_GOURAUD;

// the renderer object that performs the 3D drawings
Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;


// DTCM and DMAMEM buffers used to cache meshes into RAM
// which is faster than progmem: caching may lead to significant speedup. 

const int DTCM_buf_size = 200000; // adjust this value to fill unused DTCM but leave at least 10K for the stack to be sure
char buf_DTCM[DTCM_buf_size];

const int DMAMEM_buf_size = 170000; // adjust this value to fill unused DMAMEM,  leave at least 10k for additional serial objects. 
DMAMEM char buf_DMAMEM[DMAMEM_buf_size];

// pointer to the cashed mesh.
const Mesh3D<tgx::RGB565> * buddha_cached;


void setup()
    {
    Serial.begin(9600);

    tft.output(&Serial); // output debug infos to serial port. 

    // initialize the ILI9341 screen
    while (!tft.begin(SPI_SPEED));

    // ok. turn on backlight
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
    renderer.setMaterial(RGBf(0.85f, 0.55f, 0.25f), 0.2f, 0.7f, 0.8f, 64); // bronze color with a lot of specular reflexion. 
    renderer.setShaders(TGX_SHADER_GOURAUD);

    // cache the mesh in RAM. 
    buddha_cached = tgx::cacheMesh(&buddha, buf_DTCM, DTCM_buf_size,  buf_DMAMEM, DMAMEM_buf_size);

    }


int nbf = 0;  // number of frames drawn
float a = 0;  // current angle
float rt = 0; // sum of the mesh rendering times

void loop()
    {
    // erase the screen
    im.fillScreen(RGB565_Blue);

    // clear the z-buffer
    renderer.clearZbuffer();

    // position the model
    renderer.setModelPosScaleRot({ 0, 0.5f, -35 }, { 13,13,13 }, a);

    // draw the model onto the memory framebuffer
    elapsedMicros em = 0;
    renderer.drawMesh(buddha_cached, false);
    rt += (1000000.0f / ((int)em));

    // overlay FPS counter on the framebuffer
    tft.overlayFPS(fb); 
    
    // update the screen (asynchronous). 
    tft.update(fb);
  
    // increase the angle by 3 degrees.
    a += 3;

    // print some info about the video driver every 100 frames
    if (nbf++ % 100 == 0)
        {
        tft.printStats();
        diff1.printStats();
        diff2.printStats();
        Serial.printf("\nMesh rendering framerate: %.2f FPS\n\n", (rt / nbf) );
        }
    }


/** end of file */

