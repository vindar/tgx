/********************************************************************
* tgx library example : displaying the Bunny figurine 3D mesh...
*
*                    EXAMPLE FOR RP2040 / RP2350
*
* Instructions:
*
* 1. download and install Bodmer's TFT_eSPI library via Arduino's library manager or
*    directly from: https://github.com/Bodmer/TFT_eSPI
*
* 2. Configure the TFT_eSPI library for the screen used
*    (customize "TFT_eSPI/User_Setup.h" and/or "TFT_eSPI/User_Setup_Select.h")
*
* 3. Select the board model and serial port and upload the sketch
*
* ---
* This example was tested with:
*    - boards:  Raspberry Pico W (RP2040) and Raspberry Pico 2 (RP2350)
*    - screens: ILI9341 (320x240) and ST7735 (160x128)
********************************************************************/

// screen driver library
#include <TFT_eSPI.h>

// tgx graphic library
#include <tgx.h>

// the mesh to draw
#include "bunny_fig_small.h"
#define MESH &bunny_fig_small

// let's not burden ourselves with the tgx:: prefix
using namespace tgx;

// Default drawing size.  Pico 2 has enough RAM for a much larger viewport,
// while Pico W keeps the smaller size to leave safe stack/heap margin.
#if defined(ARDUINO_ARCH_RP2350) || defined(PICO_RP2350) || defined(__ARM_ARCH_8M_MAIN__)
#define SLX 240
#define SLY 300
#else
#define SLX 140
#define SLY 160
#endif

// real drawing size
int slx, sly;

// the framebuffer we draw onto
uint16_t fb[SLX * SLY];

// second framebuffer used for DMA update
uint16_t fb2[SLX * SLY];
bool use_dma = false;
int draw_buffer_index = 0;

// the zbuffer
uint16_t zbuf[SLX * SLY];

// the tgx::image object that encapsulates framebuffer fb
Image<RGB565> imfb;

// only load the shaders we need.
const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT | SHADER_GOURAUD | SHADER_NOTEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2;

// the renderer object that performs the 3D drawings
Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;

// the screen driver
TFT_eSPI tft = TFT_eSPI();


#include "tgx_example_telemetry.h"

// the setup function runs once when you press reset or power the board
void setup()
    {
    Serial.begin(115200);
    telemetryBegin();

    // we initialize the screen driver
    tft.init();
    tft.setRotation(0);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED,TFT_BLACK,true);
    use_dma = tft.initDMA();
    if (use_dma)
        {
        tft.startWrite();
        Serial.println("bunny_fig display DMA enabled");
        }
    else
        {
        Serial.println("bunny_fig display DMA unavailable, using pushImage");
        }

    // Resize the drawing size in case the screen is smaller than SLXxSLY.
    // Keep a little vertical room for the direct-to-screen FPS/mode labels.
    slx = std::min<int>(tft.width(), SLX);
    const int available_y = (tft.height() > 20) ? (tft.height() - 20) : tft.height();
    sly = std::min<int>(available_y, SLY);

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
    const float dilat = 9;                // scale model
    const float nearz = 15; // how much we zoom in
    const float upy = 2; // how much we move up to focus on the head
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
        if (use_dma) tft.dmaWait();
        tft.drawString(tfps,0,0);
        prev_millis = m;
        nbframes = 0;
        }
    if (prev_loopnumber != loopnumber)
        { // update the text for the drawing mode
        prev_loopnumber = loopnumber;
        tft.setTextDatum(BL_DATUM);
        if (use_dma) tft.dmaWait();
        switch (loopnumber % 4)
            {
            case 0: tft.drawString("Gouraud/texture", 0, tft.height()-1); telemetrySetScene("gouraud_texture"); break;
            case 1: tft.drawString("Wireframe      ", 0, tft.height()-1); telemetrySetScene("wireframe"); break;
            case 2: tft.drawString("Flat Shading   ", 0, tft.height()-1); telemetrySetScene("flat"); break;
            case 3: tft.drawString("Gouraud shading", 0, tft.height()-1); telemetrySetScene("gouraud"); break;
            }
        }
    }


int loopnumber = 0;

/** Main loop */
void loop()
    {
    telemetryStartFrame();

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
        case 1: renderer.drawWireFrameMesh(MESH);
                break;
        case 2: renderer.setShaders(SHADER_FLAT);
                renderer.drawMesh(MESH, false);
                break;
        case 3: renderer.setShaders(SHADER_GOURAUD);
                renderer.drawMesh(MESH, false);
                break;
        }

    // display additional information (drawing type and FPS)
    infos(loopnumber);

    // Upload the framebuffer to the screen.  When DMA is available, the two
    // framebuffers are alternated so the renderer never writes into the buffer
    // currently being read by DMA.
    const int x = (tft.width() - slx) / 2;
    const int y = (tft.height() - sly) / 2;
    if (use_dma)
        {
        uint16_t* current = (draw_buffer_index == 0) ? fb : fb2;
        tft.pushImageDMA(x, y, slx, sly, current);
        draw_buffer_index = 1 - draw_buffer_index;
        imfb.set((draw_buffer_index == 0) ? fb : fb2, slx, sly);
        renderer.setImage(&imfb);
        }
    else
        {
        tft.pushImage(x, y, slx, sly, fb);
        }

    telemetryEndFrame();
    }
