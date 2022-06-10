/********************************************************************
*
* tgx library example : testing texture mapping.
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
#include "3Dmodels/spot/spot.h"
#include "3Dmodels/bob/bob.h"
#include "3Dmodels/blub/blub.h"


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
uint16_t zbuf[SLX * SLY];           

// image that encapsulates fb.
Image<RGB565> im(fb, SLX, SLY);


// only load the shaders we need (note that TGX_SHADER_NOTEXTURE is needed in order to draw without texturing !). 
const int LOADED_SHADERS = TGX_SHADER_PERSPECTIVE | TGX_SHADER_ZBUFFER | TGX_SHADER_GOURAUD | TGX_SHADER_NOTEXTURE | TGX_SHADER_TEXTURE_BILINEAR | TGX_SHADER_TEXTURE_NEAREST | TGX_SHADER_TEXTURE_WRAP_POW2;

// the renderer object that performs the 3D drawings
Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;


// DTCM and DMAMEM buffers used to cache meshes into RAM
// which is faster than progmem: caching may lead to significant speedup. 

const int DTCM_buf_size = 10000; // adjust this value to fill unused DTCM but leave at least 20K for the stack to be sure
char buf_DTCM[DTCM_buf_size];

const int DMAMEM_buf_size = 330000; // adjust this value to fill unused DMAMEM,  leave at least 10k for additional serial objects. 
DMAMEM char buf_DMAMEM[DMAMEM_buf_size];

const tgx::Mesh3D<tgx::RGB565> * cached_mesh; // pointer to the currently cached mesh. 



/**
* Overlay some info about the current mesh on the screen
**/
void drawInfo(tgx::Image<tgx::RGB565>& im, int shader, const tgx::Mesh3D<tgx::RGB565> & mesh)  // remark: need to keep the tgx:: prefix in function signatures because arduino messes with ino files....
    {
    // count the number of triangles in the mesh (by iterating over linked meshes)
    const Mesh3D<RGB565> * m = &mesh;
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
    sprintf(buf, "%s%s", (shader & TGX_SHADER_GOURAUD ? "Gouraud shading" : "flat shading"), (shader & TGX_SHADER_TEXTURE_BILINEAR ? " / texturing (bilinear)" : (shader & TGX_SHADER_TEXTURE_NEAREST ? " / texturing (nearest neighbour)" : "")));
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
    renderer.setMaterial(RGBf(0.75f, 0.75f, 0.75f), 0.15f, 0.7f, 0.7f, 32);   
    renderer.setCulling(1);
    renderer.setTextureWrappingMode(TGX_SHADER_TEXTURE_WRAP_POW2);
    }


void drawMesh(const Mesh3D<RGB565>* mesh, float scale)
    {
    // cache the first mesh to display in RAM to improve framerate
    cached_mesh  = tgx::cacheMesh(mesh, buf_DTCM, DTCM_buf_size,  buf_DMAMEM, DMAMEM_buf_size);
  
    const int maxT = 18000; // display model for 15 seconds. 
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
        M.multRotate((1440.0f * em) / maxT, { 0,1,0 }); // 4 rotations per display
        M.multRotate((800.0f * em) / maxT, { 1, 0, 0}); 
        M.multTranslate({ 0,0, -40 });
        renderer.setModelMatrix(M);

        // change shader type after every turn
        int part = (em * 3) / maxT;
 
        // choose the shader to use
        int shader = (part == 0) ? TGX_SHADER_GOURAUD : ((part == 1) ? (TGX_SHADER_GOURAUD | TGX_SHADER_TEXTURE_NEAREST) : (TGX_SHADER_GOURAUD | TGX_SHADER_TEXTURE_BILINEAR));

        renderer.setShaders(shader);

        // draw the mesh on the image
        renderer.drawMesh(cached_mesh, false);

        // add the FPs counter
        tft.overlayFPS(fb); 
        
        // overlay some info 
        drawInfo(im, shader, *mesh);

        tft.update(fb);

        }
    }


void loop()
    {
    
    drawMesh(&spot, 13);
    
    drawMesh(&bob, 15);

    drawMesh(&blub, 15);

    // random light orientation.
    const float angle = M_PI * random(0, 360) / 180.0f;
    renderer.setLightDirection({ cosf(angle) , sinf(angle) , -0.3f });
    }


/** end of file */

