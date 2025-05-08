/********************************************************************
*
* tgx library: 2D API
*
* Interfacing TGX with the PNGdec library to draw PNG images.  
* 
* -> Install PNGDec from the library manager or https://github.com/bitbank2/PNGdec
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

// the PNG decoder library
// install from Arduino's library manager or directly from https://github.com/bitbank2/PNGdec
#include <PNGdec.h> 

// the octocat image we will draw from memory
#include "octocat_4bpp.h" 


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


#define SPI_SPEED       30000000

// the screen driver object
ILI9341_T4::ILI9341Driver tft(PIN_CS, PIN_DC, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_RESET, PIN_TOUCH_CS, PIN_TOUCH_IRQ);

// screen dimension (landscape mode)
static const int SLX = 320;
static const int SLY = 240;

// framebuffer
uint16_t fb[SLX * SLY];

// image that encapsulates fb.
tgx::Image<tgx::RGB565> im(fb, SLX, SLY);


// PNG decoder object from PNGDec
PNG png; 


void setup()
    {
    Serial.begin(9600);
    
    // initialize the ILI9341 screen
    tft.output(&Serial);                // output debug infos to serial port.
    while (!tft.begin(SPI_SPEED));      // initialize the ILI9341 screen
    tft.setRotation(3);                 // landscape mode. 
    pinMode(PIN_BACKLIGHT, OUTPUT);     // ok...
    digitalWrite(PIN_BACKLIGHT, HIGH);  // ... turn on backlight

    // set background (horizontal gradient from blue to red)
    im.fillScreenHGradient(tgx::RGB565_Blue, tgx::RGB565_Red);
    
    // load the image in the png decoder and link the decoder to TGX
    png.openRAM((uint8_t*)octocat_4bpp, sizeof(octocat_4bpp), TGX_PNGDraw); 

    // copy the png image at 12 different positions
    float op = 1.0f;
    for (int j = 0; j < 3; j++)
        {
        for (int i = 0; i < 4; i++)
            {
            // decode the image at a specific position with a specific opacity
            im.PNGDecode(png, { i*80 - 20, j*100 -30 }, op); 
            op -= 1.0f / 13;
            }
        }

    // we are done with this image
    png.close();
    
    // update the screen with the image. 
    tft.update(fb);
    }



void loop()
    {
    delay(100);
    }


/** end of file */

