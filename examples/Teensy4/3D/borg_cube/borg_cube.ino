/********************************************************************
*
* tgx library example.
*
* The Borg cube !!! ... Well, just a stupid textured cube in fact...
* - textured is drawn onto in real time
* - switch between ortoscopic and perspective projection
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



// 30MHz SPI. Can do much better with short wires
#define SPI_SPEED       30000000


// the screen driver object
ILI9341_T4::ILI9341Driver tft(PIN_CS, PIN_DC, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_RESET, PIN_TOUCH_CS, PIN_TOUCH_IRQ);


// 2 x 5K diff buffers (used by tft) for differential updates
ILI9341_T4::DiffBuffStatic<5000> diff1;
ILI9341_T4::DiffBuffStatic<5000> diff2;

// screen dimension (landscape mode)
static const int SLX = 320;
static const int SLY = 240;

const float ratio = ((float)SLX) / SLY; // aspect ratio

// main screen framebuffer (150K in DTCM for fastest access)
uint16_t fb[SLX * SLY];                 

// internal framebuffer (150K) used by the ILI9431_T4 library for double buffering.
DMAMEM uint16_t internal_fb[SLX * SLY]; 

// zbuffer in 16bits precision (150K in DTCM)
uint16_t zbuf[SLX * SLY];           

// image that encapsulates the framebuffer fb.
Image<RGB565> im(fb, SLX, SLY);

// the texture image
const int tex_size = 128;
RGB565 texture_data[tex_size*tex_size];
Image<RGB565> texture(texture_data, tex_size, tex_size);


// we only use bilinear texturing for power of 2 texture, combined texturing with flat shading, a z buffer and both perspective and orthographic projection
const int LOADED_SHADERS = TGX_SHADER_ORTHO | TGX_SHADER_PERSPECTIVE | TGX_SHADER_ZBUFFER | TGX_SHADER_FLAT | TGX_SHADER_TEXTURE_BILINEAR |TGX_SHADER_TEXTURE_WRAP_POW2;

// the renderer object that performs the 3D drawings
Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;


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
    tft.setRotation(3); // portrait mode
    tft.setFramebuffer(internal_fb); // double buffering
    tft.setDiffBuffers(&diff1, &diff2); // 2 diff buffers
    tft.setDiffGap(4); // small gap
    tft.setRefreshRate(140); // refresh at 60hz
    tft.setVSyncSpacing(0); // lock the framerate at 60/2 = 30fps. 

    // setup the 3D renderer with perspective projection
    renderer.setViewportSize(SLX,SLY);
    renderer.setOffset(0, 0);
    renderer.setImage(&im);
    renderer.setZbuffer(zbuf);
    renderer.setCulling(1);
    renderer.setTextureQuality(TGX_SHADER_TEXTURE_BILINEAR);
    renderer.setTextureWrappingMode(TGX_SHADER_TEXTURE_WRAP_POW2);
    renderer.setShaders(TGX_SHADER_FLAT | TGX_SHADER_TEXTURE );
    renderer.setPerspective(45, ratio, 1.0f, 100.0f);
    
    // initial texture color
    texture.fillScreen(RGB565_Blue);
    }

             

/** draw a random rectangle on the texture */
void splash()
    {
    static int count = 0;    
    static RGB565 color;
    if (count == 0)
        color = RGB565((int)random(32), (int)random(64), (int)random(32)); 
    count = (count + 1) % 400;
    iVec2 pos(random(tex_size), random(tex_size));
    int r = random(10);
    texture.drawRect(iBox2( pos.x - r, pos.x + r, pos.y - r, pos.y + r ), color);
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

    im.fillScreen((projtype) ? RGB565_Black : RGB565_Gray); // erase the screen, black in perspective and grey in orthographic projection
    renderer.clearZbuffer(); // clear the z buffer        
    renderer.setModelMatrix(M);// position the model

    renderer.drawCube(&texture, & texture, & texture, & texture, & texture, & texture); // draw the textured cube


    // info about the projection type
    im.drawText((projtype) ? "Perspective projection" : "Orthographic projection", {3,12 }, RGB565_Red, font_tgx_OpenSans_Bold_10, false);

    // add fps counter
    tft.overlayFPS(fb); 
    
    // update the screen (async). 
    tft.update(fb);

    // add a random rect on the texture.
    splash();

    // switch between perspective and orthogonal projection every 1000 frames.
    if (nbf++ % 1000 == 0)
        {
        projtype = 1 - projtype;

        if (projtype)
            renderer.setPerspective(45, ratio, 1.0f, 100.0f);
        else
            renderer.setOrtho(-1.8 * ratio, 1.8 * ratio, -1.8, 1.8, 1.0f, 100.0f);

        tft.printStats();
        diff1.printStats();
        diff2.printStats();
        }
    }
       

/** end of file */
