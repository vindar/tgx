/********************************************************************
*
* tgx library: AnimatedGIF demo
*
* This example shows how to display an animated GIF with tgx using 
* the AnimatedGIF library: https://github.com/bitbank2/AnimatedGIF
* 
* EXAMPLE FOR TEENSY 4 / 4.1
*
* DISPLAY: ILI9341 (320x240)
*
********************************************************************/

//
// This example runs on teensy 4.0/4.1 with an ILI9341 screen connected via SPI. 
//

// the screen driver library:
// Install it from Arduino's library manager or download directly from https://github.com/vindar/ILI9341_T4
#include <ILI9341_T4.h> 

// the tgx library 
#include <tgx.h> 


// the animated GIF library
// Install it from Arduino's library manager or download directly from https://github.com/bitbank2/AnimatedGIF
#include <AnimatedGIF.h>


// this is the GIF animation we are going to display
#include "earth_128x128.h"


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

#define SPI_SPEED       40000000


// the screen driver object
ILI9341_T4::ILI9341Driver tft(PIN_CS, PIN_DC, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_RESET, PIN_TOUCH_CS, PIN_TOUCH_IRQ);


// 2 x 6K diff buffers (used by tft) for differential updates
ILI9341_T4::DiffBuffStatic<6000> diff1;
ILI9341_T4::DiffBuffStatic<6000> diff2;

// screen dimension (landscape mode)
static const int SLX = 320;
static const int SLY = 240;

// main screen framebuffer (150K)
uint16_t fb[SLX * SLY];

// internal framebuffer (150K in DMAMEM) used by the ILI9431_T4 library for double buffering.
DMAMEM uint16_t internal_fb[SLX * SLY];

// image that encapsulates the framebuffer.
tgx::Image<tgx::RGB565> im(fb, SLX, SLY);



// the gif decoder object
AnimatedGIF gif; 

bool firstTelemetryPrinted = false;


uint32_t hashFramebuffer()
    {
    uint32_t h = 2166136261u;
    const int n = SLX * SLY;
    const int step = (n > 4096) ? (n / 4096) : 1;
    for (int i = 0; i < n; i += step)
        {
        const uint16_t v = fb[i];
        h ^= (uint8_t)(v & 0xffu);
        h *= 16777619u;
        h ^= (uint8_t)(v >> 8);
        h *= 16777619u;
        }
    return h;
    }


void setup()
    {
    Serial.begin(115200);
    const uint32_t serialDeadline = millis() + 3000;
    while (!Serial && millis() < serialDeadline) delay(10);
#if defined(ESP32)
    delay(5000);
#else
    delay(100);
#endif

    Serial.println("TGX_EXTENSION_EXAMPLE_BEGIN");
    Serial.println("DECODER=GIF");
    Serial.println("BOARD=Teensy41");
    Serial.flush();

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
    tft.setRefreshRate(140); // 140Hz refreshrate
    tft.setVSyncSpacing(2); // 140/2 = 70hz framerate

    // clear the image: full black
    im.clear(tgx::RGB565_Black);

    // open the image and bind the gif decoder with tgx
    const int openResult = gif.open((uint8_t*)earth_128x128, sizeof(earth_128x128), TGX_GIFDraw);
    Serial.print("OPEN_RESULT=");
    Serial.println(openResult);
    }



void loop()
    {
    // draw the image (centered)
    const uint32_t startUs = micros();
    int rc = im.GIFplayFrame(gif, { 96, 56 }); 
    const uint32_t frameUs = micros() - startUs;

    // update the screen
    tft.update(fb); 

    if (!firstTelemetryPrinted)
        {
        Serial.print("RESULT=");
        Serial.println(rc);
        Serial.print("SCREEN_W=");
        Serial.println(SLX);
        Serial.print("SCREEN_H=");
        Serial.println(SLY);
        Serial.print("DRAW_X=");
        Serial.println(96);
        Serial.print("DRAW_Y=");
        Serial.println(56);
        Serial.print("TIME_US=");
        Serial.println(frameUs);
        Serial.print("FB_HASH=");
        Serial.println(hashFramebuffer(), HEX);
        Serial.println("TGX_EXTENSION_EXAMPLE_END");
        Serial.flush();
        firstTelemetryPrinted = true;
        }

    // start again when we are done. 
    if (!rc) gif.reset(); 

    // we are too fast: wait a bit...
    delay(30); 
    }


/** end of file */


