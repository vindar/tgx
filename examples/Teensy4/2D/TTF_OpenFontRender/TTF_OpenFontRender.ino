/********************************************************************
*
* tgx library: 2D API
*
* This example show how to use Takkao's OpenFont Renderer to 
* render text using TrueType fonts.
*
* EXAMPLE FOR TEENSY 4 / 4.1
*
* DISPLAY: ILI9341 (320x240)
*
* REQUIRES INSTALLING TAKKAO'S OFR LIBRARY: https://github.com/takkaO/OpenFontRender/
********************************************************************/


// This example runs on teensy 4.0/4.1 with ILI9341 via SPI. 
// the screen driver library : https://github.com/vindar/ILI9341_T4
#include <ILI9341_T4.h> 

// the tgx library 
#include <tgx.h> 

// Install OpenFontRender from: https://github.com/takkaO/OpenFontRender/
#include <OpenFontRender.h> 

#include "NotoSans_Bold.h" 


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
DMAMEM uint16_t fb[SLX * SLY];

// internal framebuffer (150K in DMAMEM) used by the ILI9431_T4 library for double buffering.
DMAMEM uint16_t internal_fb[SLX * SLY];

// image that encapsulates fb.
Image<RGB565> im(fb, SLX, SLY);


OpenFontRender ofr; // the font drawer object


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

    // Load the font and check it can be read OK
    if (ofr.loadFont(NotoSans_Bold, sizeof(NotoSans_Bold))) 
        {
        Serial.println("Initialise error");
        while (1) { delay(1); }
        }

    }


float df = 1.0f; // scale factor
float op = 1.0f; // opacity


void renderTTF(const char* str)
    {
    const uint8_t fsize = 20;
    const float mult = 1.05;
    ofr.setFontSize(fsize * df);
    ofr.cprintf(str);
    df = df * mult;
    }


void loop()
    {

    im.setTakkaoOFR(ofr, op); // link the image to the OpenFontRender object and set the opacity
    im.clear(RGB565_Black); // clear the image fully black    

    op -= 0.005f; 
    if (op < 0.0f) op = 1.0f;
    df = 1.0f;

    ofr.setFontColor((uint16_t)RGB565_Yellow, (uint16_t)RGB565_Black); // set foreground and background colors
    ofr.setCursor(tft.width() / 2, 10); // Set the cursor    
    ofr.setLineSpaceRatio(0.8); // A neat feature is that line spacing can be tightened up (by a factor of 0.8 here)

    renderTTF("It is a period of civil war.\n");
    renderTTF("Rebel spaceships, striking\n");
    renderTTF("from a hidden base, have won\n");
    renderTTF("their first victory against\n");
    renderTTF("the evil Galactic Empire.\n\n");
    renderTTF("During the battle, Rebel\n");
    renderTTF("spies managed to steal secret\n");
    renderTTF("plans to the Empire's\n");
    renderTTF("ultimate weapon, the DEATH\n");
    renderTTF("STAR, an armored space\n");
    renderTTF("station with enough power to\n");
    renderTTF("destroy an entire planet.\n\n");
    renderTTF("Pursued by the Empire's\n");
    renderTTF("sinister agents, Princess\n");
    renderTTF("Leia races home aboard her\n");
    renderTTF("starship, custodian of the\n");
    renderTTF("stolen plans that can save\n");
    renderTTF("her people and restore\n");
    renderTTF("freedom to the galaxy....\n");

    tft.update(fb); 

    }


/** end of file */

