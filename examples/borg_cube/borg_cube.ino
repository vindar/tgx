/********************************************************************
*
* tgx library example.
*
* The Borg cube !!! ... Well, just a stupid textured cube in fact...
* 
********************************************************************/


// This example runs on teensy 4.1 with ILI9341 via SPI. 
// the screen driver library : https://github.com/vindar/ILI9341_T4
#include <StatsVar.h>
#include <DiffBuff.h>
#include <ILI9341Driver.h> 

// the tgx library 
#include "tgx.h"

// let's not burden ourselves with the tgx:: prefix
using namespace tgx;

// font 
#include "font_Arial_10.h"

// 30mhz is enough.
#define SPI_SPEED		30000000

// pins (here on SPI1)
// !!! the ILI9341_T4 screen driver only works with hardware SPI and DC 
//     must be a valid cs pin for the corresponding SPI bus !!!
#define PIN_SCK			27
#define PIN_MISO		1
#define PIN_MOSI		26
#define PIN_DC			0
#define PIN_RESET		29
#define PIN_CS			30
#define PIN_BACKLIGHT   28  // set this to 255 if not connected to MCU. 
#define PIN_TOUCH_IRQ	32  // set this to  255 if not used (or not on the same spi bus)
#define PIN_TOUCH_CS	31  // set this to  255 if not used (or not on the same spi bus)

// the screen driver object
ILI9341_T4::ILI9341Driver tft(PIN_CS, PIN_DC, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_RESET, PIN_TOUCH_CS, PIN_TOUCH_IRQ);

// 2 x 5K diff buffers (used by tft) for differential updates
ILI9341_T4::DiffBuffStatic<5000> diff1;
ILI9341_T4::DiffBuffStatic<5000> diff2;

// screen dimension (landscape mode)
static const int SLX = 320;
static const int SLY = 240;

// main screen framebuffer (150K in DTCM for fastest access)
uint16_t fb[SLX * SLY];                 

// internal framebuffer (150K) used by the ILI9431_T4 library for double buffering.
uint16_t internal_fb[SLX * SLY]; 

// zbuffer (300K in DMAMEM)
DMAMEM float zbuf[SLX * SLY];           

// image that encapsulates the framebuffer fb.
Image<RGB565> im(fb, SLX, SLY);

// the texture image
const int tex_size = 128;
RGB565 texture_data[tex_size*tex_size];
Image<RGB565> texture(texture_data, tex_size, tex_size);

// 3D mesh drawer : using perspective projection.
Renderer3D<RGB565, SLX, SLY, true, false> rendererP;

// 3D mesh drawer : using orthoscopic projection.
Renderer3D<RGB565, SLX, SLY, true, true> rendererO;


// the cube 8 vertices
fVec3 tab_vertices[8] = 
    {
    {-1,-1,1},
    {1,-1,1},
    {1,1,1},
    {-1,1,1},
    {-1,-1,-1},
    {1,-1,-1},
    {1,1,-1},
    {-1,1,-1}
    };


// texture indices
fVec2 tab_tex[4] = 
    {
    {0.0f, 0.0f},
    {1.0f, 0.0f},
    {1.0f, 1.0f},
    {0.0f, 1.0f},
    };


// list of quads. 
uint16_t vert_ind[6 * 4] =
    {
    0, 1, 2, 3, // front
    1, 5, 6, 2, // right
    5, 4, 7, 6, // back
    4, 0, 3, 7, // left
    0, 4, 5, 1, // bottom
    3, 2, 6, 7  // top
    };


// use the same texture on each quad
uint16_t tex_ind[6 * 4] =
    {
    0, 1, 2, 3,
    0, 1, 2, 3,
    0, 1, 2, 3,
    0, 1, 2, 3,
    0, 1, 2, 3,
    0, 1, 2, 3
    };


void setup()
    {
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
    tft.setRotation(3); // portrait mode
    tft.setFramebuffers(internal_fb); // double buffering
    tft.setDiffBuffers(&diff1, &diff2); // 2 diff buffers
    tft.setDiffGap(4); // small gap
    tft.setRefreshRate(140); // refresh at 60hz
    tft.setVSyncSpacing(0); // lock the framerate at 60/2 = 30fps. 

    const float ratio = ((float)SLX) / SLY;

    // setup the 3D renderer with perspective projection
    rendererP.setOffset(0, 0);
    rendererP.setImage(&im);
    rendererP.setZbuffer(zbuf, SLX * SLY);
    rendererP.setPerspective(45, ratio, 0.1f, 1000.0f);
    rendererP.setCulling(1);

    // setup the 3D renderer with orthoscopic projection
    rendererO.setOffset(0, 0);
    rendererO.setImage(&im);
    rendererO.setZbuffer(zbuf, SLX * SLY);
    rendererO.setOrtho(-1.8*ratio, 1.8 *ratio, -1.8, 1.8, 0.1f, 1000.0f);
    rendererO.setCulling(1);

    // initial textrure color
    texture.fillScreen(RGB565_Blue);
    }

             

/** draw a random rectangle on the texture */
void splash()
    {
    static int count = 0;    
    static RGB565 color;
    if (count == 0)
        color = RGB565((int)random(32), (int)random(64), (int)random(32)); 
    count = ++count % 400;
    iVec2 pos(random(tex_size), random(tex_size));
    int r = random(10);
    texture.drawRect(iBox2( pos.x - r, pos.x + r, pos.y - r, pos.y + r ), color);
    }


/** draw the current fps on the image */
void fps(const char* str)
    {
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
    im.drawText(str, {3,12 }, RGB565_Red, Arial_10, false);
    char buf[10];
    sprintf(buf, "%d FPS", fps);
    auto B = im.measureText(buf, { 0,0 }, Arial_10, false);
    im.drawText(buf, { SLX - B.lx() - 3,12 }, RGB565_Red, Arial_10, false);
    }



elapsedMillis em = 0; // time
int nbf = 0; ; // number frames drawn
int projtype = 0; // current projection used. 


void loop()
    {
   
    // model matrix
    fMat4 M;
    M.setRotate(em / 11.0f, { 0,1,0 });
    M.multRotate(em / 23.0f, { 1,0,0 });
    M.multRotate(em / 41.0f, { 0,0,1 });
    M.multTranslate({ 0, 0, -5 });

    if (projtype)
        {        
        im.fillScreen(RGB565_Black); // erase the screen
        rendererP.clearZbuffer(); // clear the z buffer        
        rendererP.setModelMatrix(M);// position the model        
        rendererP.drawQuads(TGX_SHADER_TEXTURE, 6, vert_ind, tab_vertices, nullptr, nullptr, tex_ind, tab_tex, &texture); // draw !        
        fps("Perspective projection"); // overlay some infos
        }
    else
        {
        im.fillScreen(RGB565_Gray); // erase the screen
        rendererO.clearZbuffer();
        rendererO.setModelMatrix(M);
        rendererO.drawQuads(TGX_SHADER_TEXTURE, 6, vert_ind, tab_vertices, nullptr, nullptr, tex_ind, tab_tex, &texture); // draw !
        fps("Orthoscopic projection"); // overlay some infos
        }

    // update the screen (async). 
    tft.update(fb);

    // add a random rect on the texture.
    splash();

    // swith between perspective and orthogonal projection every 1000 frames.
        if (nbf++ % 1000 == 0)
            {
            projtype = 1 - projtype;
            tft.printStats();
            diff1.printStats();
            diff2.printStats();
            }
    }
       

/** end of file */

