/********************************************************************
* tgx library example : displaying the Donkey Kong 3D mesh...
*
*              EXAMPLE FOR M5STACK Core2 and CoreS3
*
* Instructions:
*
* 1. download and install LovyanGFX library via Arduino's libray manager or 
*    directly from: https://github.com/lovyan03/LovyanGFX 
*
* 2. Select the board model and serial port and upload the sketch 
*
* ---
* This example was tested with M5Stack Core2 and M5Stack Core3SE
********************************************************************/

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
#define SLX 220
#define SLY 180

// the framebuffer we draw onto
uint16_t fb[SLX * SLY];

// second framebuffer used for DMA update
#if defined(CONFIG_IDF_TARGET_ESP32S3)
    uint16_t fb2[SLX * SLY];  // statically allocated on CoreS3...
#else
    uint16_t* fb2;  // ...but malloced on Core2
#endif

// the z-buffer in 16 bits precision
#if defined(CONFIG_IDF_TARGET_ESP32S3)
    uint16_t zbuf[SLX * SLY];  // statically allocated on CoreS3...
#else
    uint16_t* zbuf; // ..but malloced on Core2
#endif

// the tgx::image object that encapsulate framebuffer fb
Image<RGB565> imfb;

// we only load the shaders we need.
const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT | SHADER_GOURAUD | SHADER_NOTEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2;

// the renderer object that performs the 3D drawings
Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;

// LovyanGFX display object used to draw the framebuffer on the screen
LGFX lcd;


void setup(void) 
    {
    Serial.begin(115200);

    // first, we allocate memory for the buffers: only on Core2 (not on CoreS3)
    #if not defined(CONFIG_IDF_TARGET_ESP32S3)
        fb2 = (uint16_t *)heap_caps_malloc(SLX * SLY * sizeof(uint16_t), MALLOC_CAP_DMA);  // allocate the second framebuffer
        while (fb2 == nullptr) { Serial.println("Error: cannot allocate memory for fb2"); delay(1000); }
        zbuf = (uint16_t*)malloc(SLX * SLY * sizeof(uint16_t)); // allocate the zbuffer
        while (zbuf == nullptr) { Serial.println("Error: cannot allocate memory for zbuf"); delay(1000);}
    #endif

    // setup the screen driver
    lcd.init();
    lcd.setRotation(1);
    lcd.setBrightness(200);
    lcd.setColorDepth(16);
    lcd.setSwapBytes(false);  // do not use the library 'builtin byteswapping' because it disabled DMA...
    lcd.setTextFont(1);
    lcd.fillScreen(TFT_BLACK);
    lcd.initDMA();
    lcd.startWrite();

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
        lcd.waitDMA();
        switch (loopnumber % 4)
            {
            case 0: lcd.drawString("Gouraud/texture", 0, lcd.height()-1); break;
            case 1: lcd.drawString("Wireframe      ", 0, lcd.height()-1); break;
            case 2: lcd.drawString("Flat Shading   ", 0, lcd.height()-1); break;
            case 3: lcd.drawString("Gouraud shading", 0, lcd.height()-1); break;
            }
        }
    if (m > prev_millis + 1000)  // every second
        { // update the fps counter
        char tfps[10] = "FPS:     ";
        itoa(nbframes,tfps + 5,10);
        tfps[strlen(tfps)] = ' ';
        lcd.setTextDatum(TL_DATUM);
        lcd.waitDMA();
        lcd.drawString(tfps,0,0);
        prev_millis = m;
        nbframes = 0;
        }
    }



int loopnumber = 0;

/** Main loop */
void loop() 
    {
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
            renderer.drawWireFrameMesh(MESH, true);
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

    // display additonal informations on the screen
    infos(loopnumber);

    // copy fb to fb2 with byteswapping (beware that fb2 may not be uint8_t* addressable)
    for (int i = 0; i < SLX * SLY; i++) 
        { 
        const uint16_t a = fb[i];
        fb2[i] = (a << 8) | (a >> 8);
        }
    // start DMA upload of the framebuffer to the screen.
    lcd.pushImageDMA((lcd.width() - SLX) / 2, (lcd.height() - SLY) / 2, SLX, SLY, fb2);  
    }


/** end of file */


