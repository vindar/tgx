/********************************************************************
* tgx library example : displaying a 3D mesh...
*
* EXAMPLE FOR TEENSY 3.5 / 3.6 / 4 / 4.1
*
* WITH DISPLAY: ST7735 160x128 screen 
********************************************************************/


// 3D graphic library
#include <tgx.h>
#include <font_tgx_OpenSans_Bold.h>


// screen driver library (bundled with teensyduino)
#include <ST7735_t3.h> 


#if defined(ARDUINO_TEENSY35)
#define TWO_MODELS 0 // not enough flash memory to load both models
#elif defined(ARDUINO_TEENSY36)
#define TWO_MODELS 1
#elif defined(ARDUINO_TEENSY40)
#define TWO_MODELS 1
#elif defined(ARDUINO_TEENSY41) 
#define TWO_MODELS 1
#else 
#error This sketch only works on Teensy 3.5, 3.6, 4 or 4.1
#endif



// 3D models to draw
#include "3Dmodels/stormtrooper/stormtrooper.h"
#if TWO_MODELS
#include "3DModels/cyborg/cyborg.h" // only for teensy 3.6 / 4 / 4.1
#endif


// do not bother with the tgx prefix
using namespace tgx;


// Pinout, hardware SPI0
#define TFT_MOSI  11
#define TFT_SCK   13
#define TFT_DC   9 
#define TFT_CS   10 
#define TFT_RST  8


// the screen driver: here using a small 160x128 pixels ST7735 screen
ST7735_t3 tft = ST7735_t3(TFT_CS, TFT_DC, TFT_RST);

// screen size (portrait mode)
#define LX 128
#define LY 160


uint16_t fb1[LX * LY];      // framebuffer 1
Image<RGB565> imfb1(fb1, LX, LY); // image that encapsulate framebuffer 1

uint16_t fb2[LX * LY];      // framebuffer 2
Image<RGB565> imfb2(fb2, LX, LY); // image that encapsulate framebuffer 2

uint16_t zbuf[LX * LY]; // zbuffer in 16 bits precision

Image<RGB565> * front_fb, * back_fb; 


// we only use nearest neighbour texturing for power of 2 textures, combined texturing with gouraud shading, a z buffer and perspective projection
const int LOADED_SHADERS = TGX_SHADER_PERSPECTIVE | TGX_SHADER_ZBUFFER | TGX_SHADER_GOURAUD | TGX_SHADER_TEXTURE_NEAREST |TGX_SHADER_TEXTURE_WRAP_POW2;

// the renderer object that performs the 3D drawings
Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;


void setup() 
    {
    front_fb = &imfb1;
    back_fb = &imfb2;

    // set up the screen driver
    tft.initR(INITR_BLACKTAB);
    tft.setRotation(0);
    tft.setFrameBuffer(fb1); 
    tft.useFrameBuffer(true);

    // setup the 3D renderer.
    renderer.setViewportSize(LX,LY);                 
    renderer.setOffset(0, 0);
    renderer.setZbuffer(zbuf); // set the z buffer for depth testing
    renderer.setPerspective(45, ((float)LX) / LY, 1.0f, 100.0f);  // set the perspective projection matrix.     
    renderer.setMaterial(RGBf(0.85f, 0.55f, 0.25f), 0.2f, 0.7f, 0.8f, 64); // bronze color with a lot of specular reflexion. 
    renderer.setCulling(1);
    renderer.setShaders(TGX_SHADER_GOURAUD | TGX_SHADER_TEXTURE);
    renderer.setTextureWrappingMode(TGX_SHADER_TEXTURE_WRAP_POW2);
    renderer.setTextureQuality(TGX_SHADER_TEXTURE_NEAREST);
    }




/** Launch screen update and switch framebuffers **/
void updateAndSwitchFB()
    {
    if (tft.asyncUpdateActive()) tft.waitUpdateAsyncComplete(); // wait for previous update to complete
    tft.setFrameBuffer((uint16_t*)front_fb->data()); // set the framebuffer to upload
    tft.updateScreen(); // launch DMA update; 
    swap(front_fb, back_fb); // swap buffers
    }


/** Return the current framebuffer **/
tgx::Image<tgx::RGB565>* currentFB()
    {
    return front_fb;
    }


/** Compute the model matrix according to the current time */  
tgx::fMat4 moveModel(int & loopnumber) 
    {    
    const float end1 = 3000;
    const float end2 = 2000;
    const float end3 = 4000;
    const float end4 = 2000;

    int tot = (int)(end1 + end2 + end3 + end4);
    int m = millis();

    loopnumber = m / tot; 
    float t = m % tot;

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
            tz = -25 + 18 * t;
            ty = -6.5f * t;
            }
        else
            {
            t -= end2;
            if (t < end3)
                { // close up
                tz = -7;
                ty = -6.5f;
                }
            else
                { // zooming out
                t -= end3;
                t /= end4;
                tz = -7 - 18 * t;
                ty = -6.5f + 6.5f * t;
                }
            }
        }

    fMat4 M;
    M.setScale({ dilat, dilat, dilat }); // scale the model
    M.multRotate(-roty, { 0,1,0 }); // rotate around y
    M.multTranslate({ 0,ty, tz }); // translate
    return M;
    }


/** draw the current fps on the image */
void fps()
    {      
    Image<RGB565> & im = *currentFB();
    static elapsedMillis em = 0; // number of milli elapsed since last fps update
    static int fps = 0;         // last fps 
    static int count = 0;       // number of frames since the last update
    // recompute fps every second. 
    count++;
    if ((int)em > 1000)
        {
        em = 0;
        fps = count;
        count = 0;
        }
    // display 
    char buf[10];
    sprintf(buf, "%d FPS", fps);
    auto B = im.measureText(buf, { 0,0 }, font_tgx_OpenSans_Bold_9, false);
    im.drawText(buf, { LX - B.lx() - 3,9 }, RGB565_Red, font_tgx_OpenSans_Bold_9,false);    
    }

    
void loop() 
    {
    // compute the model position
    int loopnumber;
    fMat4  M = moveModel(loopnumber);
    renderer.setModelMatrix(M);

    // draw the 3D mesh
    Image<RGB565> * im = currentFB();
    renderer.clearZbuffer(); // clear the z-buffer
    renderer.setImage(im); // set the image to draw onto
    im->fillScreen(RGB565_Gray); // background

    // choose the mesh
#if (TWO_MODELS)
    const Mesh3D<RGB565>* mesh = (loopnumber & 1) ? &stormtrooper : &cyborg; 
#else
    const Mesh3D<RGB565>* mesh = &stormtrooper; 
#endif

    renderer.drawMesh(mesh, false); // draw !

    // flash memory just overflows on T3.6 when we try to print FPS :-( 
    // (todo: fixit)  
    #if not defined(ARDUINO_TEENSY36)  
    fps();
    #endif
    
    updateAndSwitchFB(); // update to screen and switch framebuffers

    }


/** end of file */
