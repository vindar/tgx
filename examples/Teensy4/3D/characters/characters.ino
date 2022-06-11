/********************************************************************
*
* tgx library example : displaying some nice meshes...
*
* EXAMPLE FOR TEENSY 4 / 4.1 (TEENSY 4.1 HAS ADDITIONNAL MODELS)
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


// meshes (stored in PROGMEM) for Teensy 4.0 and 4.1
#include "3Dmodels/nanosuit/nanosuit.h"
#include "3Dmodels/R2D2/R2D2.h"

// additional meshes for Teensy 4.1 / MicroMod since it has more flash. 
#if defined(ARDUINO_TEENSY41) || defined(ARDUINO_TEENSY_MICROMOD)
#include "3Dmodels/elementalist/elementalist.h"
#include "3Dmodels/sinbad/sinbad.h"
#include "3Dmodels/cyborg/cyborg.h"
#include "3Dmodels/dennis/dennis.h"
#include "3Dmodels/manga3/manga3.h"
#include "3Dmodels/naruto/naruto.h"
#include "3Dmodels/stormtrooper/stormtrooper.h"
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


// 2 x 6K diff buffers (used by tft) for differential updates
ILI9341_T4::DiffBuffStatic<6000> diff1;
ILI9341_T4::DiffBuffStatic<6000> diff2;

// screen dimension (portrait mode)
static const int SLX = 240;
static const int SLY = 320;

// main screen framebuffer (150K in DTCM)
uint16_t fb[SLX * SLY];

// internal framebuffer (150K in DMAMEM) used by the ILI9431_T4 library for double buffering.
DMAMEM uint16_t internal_fb[SLX * SLY];

// zbuffer in 16bits precision (150K in DTCM)
uint16_t zbuf[SLX * SLY];

// image that encapsulates fb.
Image<RGB565> im(fb, SLX, SLY);


// we only use nearest neighbour texturing for power of 2 textures, combined texturing with gouraud shading, a z buffer and perspective projection
const int LOADED_SHADERS = TGX_SHADER_PERSPECTIVE | TGX_SHADER_ZBUFFER | TGX_SHADER_GOURAUD | TGX_SHADER_TEXTURE_NEAREST |TGX_SHADER_TEXTURE_WRAP_POW2;

// the renderer object that performs the 3D drawings
Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;


// shaders to use
const int shader = TGX_SHADER_GOURAUD | TGX_SHADER_TEXTURE_NEAREST | TGX_SHADER_TEXTURE_WRAP_POW2;


// list of meshes to display
#if defined(ARDUINO_TEENSY41) || defined(ARDUINO_TEENSY_MICROMOD)
const Mesh3D<RGB565>* meshes[9] = { &nanosuit_1, &elementalist_1, &sinbad_1, &cyborg, &naruto_1, &manga3_1, &dennis, &R2D2, &stormtrooper };
#else
const Mesh3D<RGB565>* meshes[2] = { &nanosuit_1,  &R2D2 };
#endif


// DTCM and DMAMEM buffers used to cache meshes into RAM
// which is faster than progmem: caching may lead to significant speedup. 

const int DTCM_buf_size = 35000; // adjust this value to fill unused DTCM but leave at least 20K for the stack to be sure
char buf_DTCM[DTCM_buf_size];

const int DMAMEM_buf_size = 330000; // adjust this value to fill unused DMAMEM,  leave at least 10k for additional serial objects. 
DMAMEM char buf_DMAMEM[DMAMEM_buf_size];

const tgx::Mesh3D<tgx::RGB565> * cached_mesh; // pointer to the currently cached mesh. 



/**
* This function computes the object position
* according to the current time.
* Return true when it is time to change model.
**/
bool  moveModel() // remark: need to keep the tgx:: prefix in function signatures because arduino messes with ino files....
    {
    static elapsedMillis em = 0; // timer
    const float end1 = 6000;
    const float end2 = 2000;
    const float end3 = 6000;
    const float end4 = 2000;
    const float tot = end1 + end2 + end3 + end4;

    bool change = false;
    while (em > tot) { em -= tot; change = true; } // check if it is time to change the mesh. 

    float t = em; // current time
    const float dilat = 9; // scale model
    const float roty = 360 * (t / 4000); // rotate 1 turn every 4 seconds        
    float tz, ty;
    if (t < end1)
        { // far away
        tz = -25;
        ty = 0;
        }
    else
        {
        t -= end1;
        if (t < end2)
            { // zooming in
            t /= end2;
            tz = -25 + 15 * t;
            ty = -6 * t;
            }
        else
            {
            t -= end2;
            if (t < end3)
                { // close up
                tz = -10;
                ty = -6;
                }
            else
                { // zooming out
                t -= end3;
                t /= end4;
                tz = -10 - 15 * t;
                ty = -6 + 6 * t;
                }
            }
        }
    fMat4 M;
    M.setScale({ dilat, dilat, dilat }); // scale the model
    M.multRotate(-roty, { 0,1,0 }); // rotate around y
    M.multTranslate({ 0,ty, tz }); // translate
    renderer.setModelMatrix(M);
    return change;
    }



/**
* Overlay some info about the current mesh on the screen
**/
void drawInfo(tgx::Image<tgx::RGB565>& im, int shader, const tgx::Mesh3D<tgx::RGB565>* mesh)  // remark: need to keep the tgx:: prefix in function signatures because arduino messes with ino files....
    {
    // count the number of triangles in the mesh (by iterating over linked meshes)
    const Mesh3D<RGB565>* m = mesh;
    int nbt = 0;
    while (m != nullptr)
        {
        nbt += m->nb_faces;
        m = m->next;
        }
    // display some info 
    char buf[80];
    im.drawText((mesh->name != nullptr ? mesh->name : "[unnamed mesh]"), { 3,12 }, RGB565_Red, font_tgx_OpenSans_Bold_10, false);
    sprintf(buf, "%d triangles", nbt);
    im.drawText(buf, { 3,SLY - 21 }, RGB565_Red, font_tgx_OpenSans_Bold_10, false);
    sprintf(buf, "%s%s", (shader & TGX_SHADER_GOURAUD ? "Gouraud shading" : "flat shading"), (shader & TGX_SHADER_TEXTURE_NEAREST ? " / texturing" : ""));
    im.drawText(buf, { 3, SLY - 5 }, RGB565_Red, font_tgx_OpenSans_Bold_10, false);
    }




// index of the mesh currently displayed
int meshindex = 0;

// number of frame drawn
int nbf = 0;


void setup()
    {
    Serial.begin(9600);

    tft.output(&Serial);                // output debug infos to serial port. 

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
    tft.setVSyncSpacing(0); // do not use v-sync because we want to measure the max framerate; 
    tft.setRefreshRate(140); // max refreshrate. 

    // setup the 3D renderer.
    renderer.setViewportSize(SLX,SLY); // viewport = screen 
    renderer.setOffset(0, 0); //  image = viewport
    renderer.setImage(&im); // set the image to draw onto (ie the screen framebuffer)
    renderer.setZbuffer(zbuf); // set the z buffer for depth testing
    renderer.setPerspective(45, ((float)SLX) / SLY, 1.0f, 100.0f);  // set the perspective projection matrix. 
    renderer.setCulling(1);
    renderer.setShaders(shader);

    // if external ram is present, copy model textures to extram because it gives a few more fps. 
    #if defined(ARDUINO_TEENSY41)
    if (external_psram_size > 0)
        {
        for (auto& m : meshes)  m = copyMeshEXTMEM(m);
        }
    #endif

    // cache the first mesh to display in RAM to improve framerate
    cached_mesh  = tgx::cacheMesh(meshes[meshindex], buf_DTCM, DTCM_buf_size,  buf_DMAMEM, DMAMEM_buf_size);

    }





void loop()
    {
    // erase the screen
    im.fillScreen(RGB565_Black);

    // clear the z buffer
    renderer.clearZbuffer();

    // move the model to it correct position (depending on current time). 
    if (moveModel())
        {
        // selct next mesh
        meshindex = (meshindex + 1) % (sizeof(meshes) / sizeof(meshes[0]));

        // cache it in RAM to improve framerate
        cached_mesh  = tgx::cacheMesh(meshes[meshindex], buf_DTCM, DTCM_buf_size,  buf_DMAMEM, DMAMEM_buf_size);
        }


    // draw the mesh on the image
    renderer.drawMesh(cached_mesh);

    // overlay some info 
    drawInfo(im, shader, meshes[meshindex]);

    // add FPS counter
    tft.overlayFPS(fb); 
    
    // update the screen
    tft.update(fb);

    // periodically print some info about the display driver
    if (nbf++ % 200 == 0)
        {
        tft.printStats();
        diff1.printStats();
        diff2.printStats();
        }
    }


/** end of file */

