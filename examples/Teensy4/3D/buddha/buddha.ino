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


// DEFAULT WIRING USING SPI 0 ON TEENSY 4/4.1
// Recall that DC must be on a valid cs pin !!! 
#define PIN_SCK     13      // mandatory 
#define PIN_MISO    12      // mandatory
#define PIN_MOSI    11      // mandatory
#define PIN_DC      10      // mandatory
#define PIN_CS      9       // mandatory (but can be any digital pin)
#define PIN_RESET   6       // could be omitted (set to 255) yet it is better to use (any) digital pin whenever possible.
#define PIN_BACKLIGHT 255   // optional. Set this only if the screen LED pin is connected directly to the Teensy 
#define PIN_TOUCH_IRQ 255   // optional. Set this only if touch is connected on the same spi bus (otherwise, set it to 255)
#define PIN_TOUCH_CS  255   // optional. Set this only if touch is connected on the same spi bus (otherwise, set it to 255)


// ALTERNATE WIRING USING SPI 1 ON TEENSY 4/4.1
// Recall that DC must be on a valid cs pin !!! 

//#define PIN_SCK     27      // mandatory 
//#define PIN_MISO    1       // mandatory
//#define PIN_MOSI    26      // mandatory
//#define PIN_DC      0       // mandatory
//#define PIN_CS      30      // mandatory (but can be any digital pin)
//#define PIN_RESET   29      // could be omitted (set to 255) yet it is better to use (any) digital pin whenever possible.
//#define PIN_BACKLIGHT 255   // optional. Set this only if the screen LED pin is connected directly to the Teensy 
//#define PIN_TOUCH_IRQ 255   // optional. Set this only if touch is connected on the same spi bus (otherwise, set it to 255)
//#define PIN_TOUCH_CS  255   // optional. Set this only if touch is connected on the same spi bus (otherwise, set it to 255)



// 20mhz is enough to get a stable 30 FPS because we are using differential updates. 
// (with full redraws, 20mhz SPI would caps at 17 FPS)
#define SPI_SPEED       20000000


// the screen driver object
ILI9341_T4::ILI9341Driver tft(PIN_CS, PIN_DC, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_RESET, PIN_TOUCH_CS, PIN_TOUCH_IRQ);

// 2 x 3K diff buffers (used by tft) for differential updates
ILI9341_T4::DiffBuffStatic<3000> diff1;
ILI9341_T4::DiffBuffStatic<3000> diff2;

// screen dimension (portrait mode)
static const int SLX = 240;
static const int SLY = 320;

// main screen framebuffer (150K in DTCM)
uint16_t fb[SLX * SLY];                 

// internal framebuffer (150K in DMAMEM) used by the ILI9431_T4 library for double buffering.
DMAMEM uint16_t internal_fb[SLX * SLY]; 

// zbuffer (300K in DMAMEM)
DMAMEM float zbuf[SLX * SLY];           

// image that encapsulates fb.
Image<RGB565> im(fb, SLX, SLY);

// the 3D mesh drawer (with zbuffer, perspective projection, backface culling)
Renderer3D<RGB565, SLX, SLY, true, false> renderer;

    


void setup()
    {
    Serial.begin(9600);

    tft.output(&Serial);                // output debug infos to serial port. 

    // initialize the ILI9341 screen
    while (!tft.begin(SPI_SPEED))
        {
        Serial.println("Initialization error...");
        delay(1000);
        }

    // ok. turn on backlight
    pinMode(PIN_BACKLIGHT, OUTPUT);
    digitalWrite(PIN_BACKLIGHT, HIGH);

    // setup the screen driver 
    tft.setRotation(0); // portrait mode
    tft.setFramebuffers(internal_fb); // double buffering
    tft.setDiffBuffers(&diff1, &diff2); // 2 diff buffers
    tft.setDiffGap(4); // small gap
    tft.setRefreshRate(60); // refresh at 60hz
    tft.setVSyncSpacing(2); // lock the framerate at 60/2 = 30fps. 

    // setup the 3D renderer.
    renderer.setOffset(0, 0); //  image = viewport
    renderer.setImage(&im); // set the image to draw onto (ie the screen framebuffer)
    renderer.setZbuffer(zbuf, SLX * SLY); // set the z buffer for depth testing
    renderer.setPerspective(45, ((float)SLX) / SLY, 0.1f, 1000.0f);  // set the perspective projection matrix.     
    renderer.setMaterial(RGBf(0.85f, 0.55f, 0.25f), 0.2f, 0.7f, 0.8f, 64); // bronze color with a lot of specular reflexion. 
    }

                  
int nbf = 0; // number of frames drawn
float a = 0; // current angle

void loop()
    {
     // erase the screen
     im.fillScreen(RGB565_Blue);

     // clear the z-buffer
     renderer.clearZbuffer();

     // position the model
     fMat4 M;
     M.setScale(13, 13, 13);
     M.multRotate(a, { 0,1,0 });
     M.multTranslate({ 0, 0.5f, -35 });
     renderer.setModelMatrix(M);

     // draw the model onto the memory framebuffer
     renderer.drawMesh(TGX_SHADER_GOURAUD, &buddha, false);

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
        }
     }
       

/** end of file */

