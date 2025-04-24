/********************************************************************
* tgx library example : displaying the Naruto 3D mesh...
*
*           EXAMPLE FOR ESP32 FAMILY (ESP32, ESP32-S2, ESP32-S3 ...)
*
* Instructions:
*
* 1. download and install Bodmer's TFT_eSPI library via Arduino's libray manager or 
*    directly from: https://github.com/Bodmer/TFT_eSPI 
*
* 2. Configure the TFT_eSPI library for the screen used 
*    (customize "TFT_eSPI/User_Setup.h" and/or "TFT_eSPI/User_Setup_Select.h")
*
* 3. Select the board model and serial port and upload the sketch 
*
* ---
* This example was tested with:
*    - boards:  ESP32, ESP32-S2, ESP32-S3
*    - screens: ILI9341 (320x240) and ST7735 (160x128)
********************************************************************/


/**************** WARNING *****************
* DMA transfer is curently bugged on ESP32. 
* Two solutions:  
*   1. Apply the fix provided by @tgghtp in https://github.com/Bodmer/TFT_eSPI/discussions/2233
*   2. ... Or disable DMA by uncommenting the line below:
*******************************************/
// #define DISABLE_DMA   //<-- Uncomment this to disable DMA

// screen driver library
#include <TFT_eSPI.h>

// tgx graphic library
#include <tgx.h>

// the mesh to draw
#include "naruto.h"
#define MESH &naruto_1

// let's not burden ourselves with the tgx:: prefix
using namespace tgx;

// default drawing size (limited by the amount of RAM on ESP32/ESP32S3)
#define SLX 180
#define SLY 220

// real drawing size
int slx, sly; 

// the framebuffer we draw onto
uint16_t fb[SLX * SLY];

// second framebuffer used for DMA update
#if defined(CONFIG_IDF_TARGET_ESP32S3)
    uint16_t fb2[SLX * SLY];  // statically allocated on ESP32 S3...
#else
    uint16_t* fb2;  // ...but malloced on ESP32
#endif

// the z-buffer in 16 bits precision
#if defined(CONFIG_IDF_TARGET_ESP32S3)
    uint16_t zbuf[SLX * SLY];  // statically allocated on ESP32 S3...
#else
    uint16_t* zbuf; // ..but malloced on ESP32
#endif

// the tgx::image object that encapsulate framebuffer fb
Image<RGB565> imfb;

// only load the shaders we need.
const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT | SHADER_GOURAUD | SHADER_NOTEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2;

// the renderer object that performs the 3D drawings
Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;

// the screen driver
TFT_eSPI tft = TFT_eSPI();


// the setup function runs once when you press reset or power the board
void setup()
    {
    Serial.begin(115200);

    // we initialize the screen driver
    tft.init();
    tft.setRotation(0);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED,TFT_BLACK,true);
    tft.initDMA();
    tft.startWrite();

    // fix the drawing size in case the screen is smaller than SLXxSLY. 
    slx =  std::min<int>(tft.width(),SLX);
    sly =  std::min<int>(tft.height()-20,SLY);

    // Allocate memory for the buffers: only on ESP32 (not on ESP32 S3)
    #if not defined(CONFIG_IDF_TARGET_ESP32S3)
        fb2 = (uint16_t *)heap_caps_malloc(slx * sly * sizeof(uint16_t), MALLOC_CAP_DMA);  // allocate the second framebuffer
        while (fb2 == nullptr) { Serial.println("Error: cannot allocate memory for fb2"); delay(1000); }
        zbuf = (uint16_t*)malloc(slx * sly * sizeof(uint16_t)); // allocate the zbuffer
        while (zbuf == nullptr) { Serial.println("Error: cannot allocate memory for zbuf"); delay(1000);}
    #endif


    // setup the 3D renderer.
    imfb.set(fb,slx,sly);
    renderer.setViewportSize(slx,sly);
    renderer.setOffset(0, 0);    
    renderer.setImage(&imfb); // set the image to draw onto (ie the screen framebuffer)
    renderer.setZbuffer(zbuf); // set the z buffer for depth testing
    renderer.setPerspective(45, ((float)slx) / sly, 1.0f, 100.0f);  // set the perspective projection matrix.     
    renderer.setMaterial(RGBf(0.85f, 0.55f, 0.25f), 0.2f, 0.7f, 0.8f, 64); // bronze color with a lot of specular reflexion. 
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
    const float dilat = 9; // scale model
    const float nearz = 7; // how much we zoom in
    const float upy = 6.5; // how much we move up to focus on the head
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
    if (m >  prev_millis + 1000) // update every second
        { // update FPS counter
        char tfps[10] = "FPS:     ";
        itoa(nbframes,tfps + 5,10);
        tfps[strlen(tfps)] = ' ';
        tft.setTextDatum(TL_DATUM);
        tft.dmaWait();
        tft.drawString(tfps,0,0);
        prev_millis = m;
        nbframes = 0;
        }       
    if (prev_loopnumber != loopnumber)
        { // update the text for the drawing mode
        prev_loopnumber = loopnumber;
        tft.setTextDatum(BL_DATUM);
        tft.dmaWait();
        switch (loopnumber % 4)
            {
            case 0: tft.drawString("Gouraud/texture", 0, tft.height()-1); break;
            case 1: tft.drawString("Wireframe      ", 0, tft.height()-1); break;
            case 2: tft.drawString("Flat Shading   ", 0, tft.height()-1); break;
            case 3: tft.drawString("Gouraud shading", 0, tft.height()-1); break;
            }
        }
    }


int loopnumber = 0;


/** Main loop */
void loop()
    {
    // compute the model position
    fMat4  M = moveModel(loopnumber);
    renderer.setModelMatrix(M);

    // draw the 3D mesh
    imfb.fillScreen(RGB565_Cyan);              // clear the framebuffer (black background)
    renderer.clearZbuffer();                    // clear the z-buffer

    // choose the shader to use
    switch (loopnumber % 4)
        {
        case 0: renderer.setShaders(SHADER_GOURAUD | SHADER_TEXTURE);
                renderer.drawMesh(MESH, false); 
                break;                
        case 1: renderer.drawWireFrameMesh(MESH, true);
                break;
        case 2: renderer.setShaders(SHADER_FLAT);
                renderer.drawMesh(MESH, false); 
                break;
        case 3: renderer.setShaders(SHADER_GOURAUD);
                renderer.drawMesh(MESH, false); 
                break;        
        }

    // display additional informations (drawing type and FPS)
    infos(loopnumber);

    // upload the framebuffer to the screen
    #if defined(DISABLE_DMA)     
        tft.pushImage((tft.width() - slx) / 2, (tft.height() - sly) / 2, slx, sly, fb); // direct transfer without DMA
    #else
        tft.pushImageDMA((tft.width() - slx) / 2, (tft.height() - sly) / 2, slx, sly, fb, fb2); // initiate DMA transfer
    #endif
    }



/** end of file */
