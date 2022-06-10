/********************************************************************
*
* tgx library: Crazy clock demo.
*
* This example show how to use blitting of rotated/rescaled sprites
* with transparency. 
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

// the sprites
#include "antique_clock.h"  // the clock, RGB565 format, no transparency
#include "long_hand.h"  // long hand, RGB32 format (with transparency)
#include "small_hand.h" // small hand, RGB32 format (with transparency)


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


#define SPI_SPEED       40000000


// the screen driver object
ILI9341_T4::ILI9341Driver tft(PIN_CS, PIN_DC, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_RESET, PIN_TOUCH_CS, PIN_TOUCH_IRQ);


// 2 x 10K diff buffers (used by tft) for differential updates (in DMAMEM)
DMAMEM ILI9341_T4::DiffBuffStatic<6000> diff1;
DMAMEM ILI9341_T4::DiffBuffStatic<6000> diff2;

// screen dimension (landscape mode)
static const int SLX = 320;
static const int SLY = 240;

// main screen framebuffer (150K in DTCM for fastest access)
uint16_t fb[SLX * SLY];

// internal framebuffer (150K in DMAMEM) used by the ILI9431_T4 library for double buffering.
DMAMEM uint16_t internal_fb[SLX * SLY];

// image that encapsulates fb.
Image<RGB565> im(fb, SLX, SLY);




/* draw the clock with given angle, scale and opacity */
void drawClock(float angle, float scale= 1.0f, float opacity = 1.0f)
    {
    if (opacity == 1.0f)
        { // the clock does not use trnasparency so we can use the faster method when opacity = 1.0f 
        im.blitScaledRotated(antique_clock, { 120,120 }, { 160,120 }, scale, angle);  
        }
    else 
        {
        im.blitScaledRotated(antique_clock, { 120,120 }, { 160,120 }, scale, angle, opacity);  
        }
    } 


/* draw the small hand with given angle, scale and opacity */
void drawSmallHand(float angle, float scale= 1.0f, float opacity = 1.0f)
    { 
    // always use the method with blending, even when opacity = 1.0f because the hand has transparency
    im.blitScaledRotated(small_hand, { 22,117 }, { 160,120 }, 0.55f * scale, angle, opacity);
    } 


/* draw long hand with given angle, scale and opacity */
void drawLongHand(float angle, float scale= 1.0f, float opacity = 1.0f)
    {
    // always use the method with blending, even when opacity = 1.0f because the hand has transparency
    im.blitScaledRotated(long_hand, { 14,197 }, { 160,120 }, 0.5f * scale, angle, opacity);
    } 




void setup()
    {
    Serial.begin(9600);

    // output debug infos to serial port. 
    tft.output(&Serial);                

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
    tft.setVSyncSpacing(2);
    }





// used to keep track of the elapsed time
elapsedMillis em;  


void loop()
{
    int t;

    // PART 0 : title
    em = 0;
    while(em < 3000)
        {
        int y1 = (em < 1000) ? (130 * em) / 1000 - 50 : 80;
        int y2 = (em < 1000) ? 240 - (110 * em) / 1000 : 130;
        im.fillScreen(RGB565_Black);  
        im.drawText("TGX library",iVec2{110,y1}, RGB565_Red, font_tgx_OpenSans_Bold_18, true);
        im.drawText("Crazy clock demo",iVec2{35,y2}, RGB565_White, font_tgx_OpenSans_Bold_28, true);      
        tft.overlayFPS(fb);
        tft.update(fb);
        yield(); // to keep the board responsive
        }

    // PART 1 : the clock appears. 
    em = 0;
    while ((t = em) < 1000)
        {      
        float sc = 0.1f + 0.9f*t/1000.0f;
        im.fillScreen(RGB565_Black);  
        im.drawText("TGX library",iVec2{110,80}, RGB565_Red, font_tgx_OpenSans_Bold_18, true, 1.0f - t/1000.0f);
        im.drawText("Crazy clock demo",iVec2{35,130}, RGB565_White, font_tgx_OpenSans_Bold_28, true, 1.0f - t/1000.0f);
        drawClock(200 - t/5, sc);
        drawSmallHand(t/10, sc);
        drawLongHand(t/2, sc); 
        tft.overlayFPS(fb);        
        tft.update(fb);
        yield(); // to keep the board responsive
        }

    // PART 2: rotation
    em = 0;
    while ((t = em) < 10000)
        {
        im.fillScreen(RGB565_Black);  
        drawClock(0, 1.0f);
        drawSmallHand(100 +360*sin(t / 1500.0f), 1.0f);
        drawLongHand(500*cos(t / 5000.0f), 1.0f); 
        tft.overlayFPS(fb);        
        tft.update(fb);              
        yield(); // to keep the board responsive
        }  

   // PART 3: changing hands sizes
    while ((t = em) < 20000)
        {
        im.fillScreen(RGB565_Black);  
        drawClock(0, 1.0f);
        drawSmallHand(100 +360*sin(t / 1500.0f), 0.292893 + abs(sin( 0.7853981 + (t-10000.0f) / 3000.0f)));
        drawLongHand(500*cos(t / 5000.0f), 0.292893 + abs(cos( 0.7853981 + (t-10000.0f) / 3000.0f))); 
        tft.overlayFPS(fb);        
        tft.update(fb);              
        yield(); // to keep the board responsive
        }      

   // PART 4: rotating the whole clock
    while ((t = em) < 35000)
        {
        im.fillScreen(RGB565_Black);  
        drawClock( 150.0f*sin((t-20000.0f) / 2000.0f), 1.0f);
        drawSmallHand(100 +360*sin(t / 1500.0f), 0.292893 + abs(sin( 0.7853981 + (t-10000.0f) / 3000.0f)));
        drawLongHand(500*cos(t / 5000.0f), 0.292893 + abs(cos( 0.7853981 + (t-10000.0f) / 3000.0f))); 
        tft.overlayFPS(fb);        
        tft.update(fb);              
        yield(); // to keep the board responsive
        }  

    // PART 5: fading
    while ((t = em) < 40000)
        {
        im.fillScreen(RGB565_Black);  
        drawClock( 150.0f*sin((t-20000.0f) / 2000.0f), 1.0f - (t - 35000.0f)/5000.0f, 1.0f - (t - 35000.0f)/5000.0f);
        drawSmallHand(100 +360*sin(t / 1500.0f), 0.292893 + abs(sin( 0.7853981 + (t-10000.0f) / 3000.0f)), 1.0f - (t - 35000.0f)/5000.0f);
        drawLongHand(500*cos(t / 5000.0f), 0.292893 + abs(cos( 0.7853981 + (t-10000.0f) / 3000.0f)), 1.0f - (t - 35000.0f)/5000.0f); 
        tft.overlayFPS(fb);        
        tft.update(fb);              
        yield();
        }  

    // wait a bit before starting over.
     delay(2000);
    }


/** end of file */
