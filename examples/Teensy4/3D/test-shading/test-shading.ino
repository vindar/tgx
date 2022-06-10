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
#include "3Dmodels/teapot/teapot.h"
#include "3Dmodels/skull/skull.h"
#include "3Dmodels/suzanne/suzanne.h"
#include "3Dmodels/bunny/bunny.h"
#include "3Dmodels/dragon/dragon.h"


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
const int LOADED_SHADERS = TGX_SHADER_PERSPECTIVE | TGX_SHADER_ZBUFFER | TGX_SHADER_FLAT | TGX_SHADER_GOURAUD;

// the renderer object that performs the 3D drawings
Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;



// DTCM and DMAMEM buffers used to cache meshes into RAM
// which is faster than progmem: caching may lead to significant speedup. 

const int DTCM_buf_size = 160000; // adjust this value to fill unused DTCM but leave at least 20K for the stack to be sure
char buf_DTCM[DTCM_buf_size];

const int DMAMEM_buf_size = 190000; // adjust this value to fill unused DMAMEM,  leave at least 10k for additional serial objects. 
DMAMEM char buf_DMAMEM[DMAMEM_buf_size];

const tgx::Mesh3D<tgx::RGB565> * cached_mesh; // pointer to the currently cached mesh. 




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
    im.drawText((mesh.name != nullptr ? mesh.name : "[unnamed mesh]"), { 3,12 }, RGB565_Red, font_tgx_OpenSans_Bold_10, false);
    sprintf(buf, "%d triangles", nbt);
    im.drawText(buf, { 3,SLY - 21 }, RGB565_Red, font_tgx_OpenSans_Bold_10, false);
    sprintf(buf, "%s", (t == 0) ? "Wireframe" : ((t == 1) ? "Flat shading" : "Gouraud shading"));
    im.drawText(buf, { 3, SLY - 5 }, RGB565_Red, font_tgx_OpenSans_Bold_10, false);
    }



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


void drawMesh(const Mesh3D<RGB565>* mesh, float scale, float tilt = 0.0f)
{
    // cache the first mesh to display in RAM to improve framerate
    cached_mesh  = tgx::cacheMesh(mesh, buf_DTCM, DTCM_buf_size,  buf_DMAMEM, DMAMEM_buf_size);
    
    const int maxT = 12000; // display model for 12 seconds. 
    elapsedMillis em = 0;
    while (em < maxT)
        {
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

        if (t == 0)
            renderer.drawWireFrameMesh(cached_mesh);
        else if (t == 1)
            {
            renderer.setShaders(TGX_SHADER_FLAT);
            renderer.drawMesh(cached_mesh, false);
            }            
        else
            {
            renderer.setShaders(TGX_SHADER_GOURAUD);
            renderer.drawMesh(cached_mesh, false);
            }

        // overlay some info 
        drawInfo(im, t, *cached_mesh);

        // and the current framerate
        tft.overlayFPS(fb); 
        
        // update the screen (asynchronously)
        tft.update(fb);
        }
}


void loop()
{
    renderer.setMaterial(RGBf(0.15f, 0.7f, 0.39f), 0.2f, 0.8f, 0.5f, 8); // teapot
    drawMesh(&teapot, 15, 30);

    renderer.setMaterial(RGBf(1.0f, 1.0f, 1.0f), 0.15f, 0.7f, 0.8f, 48); // bunny
    drawMesh(&bunny, 12);

    renderer.setMaterial(RGBf(166 / 256.0f, 130 / 256.0f, 110.0f / 256.0f), 0.15f, 0.7f, 0.4f, 16); // skull
    drawMesh(&skull_1, 12);

    // let's have some fun with lightning
    renderer.setLightAmbiant({ 0, 0, 1.0f });  // blue
    renderer.setLightDiffuse({ 1.0f, 0, 0 });  // red    
    renderer.setLightSpecular({ 1.0f, 1.0f, 1.0f }); // white

    renderer.setMaterial(RGBf(1.0f, 1.0f, 1.0f), 0.2f, 0.8f, 0.8f, 32); // suzanne
    drawMesh(&suzanne, 13);

    // back to normal lightning
    renderer.setLightAmbiant({ 1.0f, 1.0f, 1.0f }); // white
    renderer.setLightDiffuse({ 1.0f, 1.0f, 1.0f }); // white
    renderer.setLightSpecular({ 1.0f, 1.0f, 1.0f }); // white

    renderer.setMaterial(RGBf(0.85f, 0.55f, 0.25f), 0.2f, 0.7f, 0.8f, 64); // dragon
    drawMesh(&dragon, 15);

    // chooose new random light orientation.
    const float angle = M_PI * random(0, 360) / 180.0f;
    renderer.setLightDirection({ cosf(angle) , sinf(angle) , -0.3f });
}


/** end of file */


