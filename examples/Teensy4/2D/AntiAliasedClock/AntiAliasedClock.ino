/**
* CREDITS. Example for tgx created by bruno@DIYLAB.DE. thanks !
* SRC: https://github.com/DIYLAB-DE/AntiAliasedClock
* Adapted from the 'AntiAliasedClock' example by Bodmer for its great TFT_eSPI library: https://github.com/Bodmer/TFT_eSPI
*
*
* EXAMPLE FOR TEENSY 4 / 4.1
*
* DISPLAY: ILI9341 (320x240)
*
**/


// This example runs on teensy 4.0/4.1 with ILI9341 via SPI. 
// the screen driver library : https://github.com/vindar/ILI9341_T4
#include <ILI9341_T4.h> 

// the tgx library 
#include <tgx.h>
#include<font_tgx_OpenSans.h>

// for time keeping
#include <TimeLib.h>


#include "DSEG7_Classic_Bold_20.h"
#include "DSEG7_Classic_Bold_14.h"
#include "watchface1.h"
#include "watchface2.h"
#include "watchface3.h"



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



#define SPI_SPEED       40000000  // SPI speed


// IntervalTimer object 
IntervalTimer interval25ms;

// namespace for draw graphics primitives
using namespace tgx;

// framebuffers
DMAMEM uint16_t ib[240 * 320];  // used for internal buffering
DMAMEM uint16_t fb[240 * 320];  // paint in this buffer

// the screen driver object
ILI9341_T4::ILI9341Driver tft(PIN_CS, PIN_DC, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_RESET, PIN_TOUCH_CS, PIN_TOUCH_IRQ);

// two diff buffers
ILI9341_T4::DiffBuffStatic<6000> diff1;
ILI9341_T4::DiffBuffStatic<6000> diff2;

// image that encapsulates framebuffer
Image<RGB565> im(fb, 240, 320);
const Image<RGB565> wFace1(watchface1, 240, 320);
const Image<RGB565> wFace2(watchface2, 240, 320);
const Image<RGB565> wFace3(watchface3, 240, 320);

#define CLOCK_FG       RGB565_White
#define CLOCK_DIGI     RGB565_White
#define CLOCK_DAY      RGB565_Silver
#define CLOCK_FOOTER   RGB565_Silver
#define CLOCK_BG       RGB32(0,46,88)
#define SECCOND_FG     RGB565_Red
#define LABEL_FG       RGB565_White
#define CLOCK_Y        159.0f
#define CLOCK_R        119.0f

#define DEG2RAD        0.0174532925
#define H_HAND_LENGTH  CLOCK_R/2.0f
#define M_HAND_LENGTH  CLOCK_R/1.4f
#define S_HAND_LENGTH  CLOCK_R/1.3f
#define FACE_W         CLOCK_R * 2 + 1
#define FACE_H         CLOCK_R * 2 + 1

// Calculate 1 second increment angles. Hours and minute hand angles
// change every second so we see smooth sub-pixel movement
#define DAY_ANGLE   360.0f / 30.0f
#define SECOND_ANGLE   360.0f / 60.0f
#define MINUTE_ANGLE   SECOND_ANGLE / 60.0f
#define HOUR_ANGLE     MINUTE_ANGLE / 12.0f

// vars
volatile bool updateFace;
volatile float incT = 0.0f;
float timeSecs = 0.0f;
const char* weekDays[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
const char* monthNames[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September" , "October" , "November" , "December" };

/// <summary>
/// setup
/// </summary>
void setup() {
    // time
    setSyncProvider(getTeensy3Time);
    setTime(now());

    // display
    while (!tft.begin(SPI_SPEED));

    tft.setScroll(0);
    tft.setRotation(0);
    tft.setFramebuffer(ib);              // set 1 internal framebuffer -> activate float buffering
    tft.setDiffBuffers(&diff1, &diff2);  // set the 2 diff buffers => activate differential updates 
    tft.setDiffGap(4);                   // use a small gap for the diff buffers
    tft.setRefreshRate(120);             // around 120hz for the display refresh rate 
    tft.setVSyncSpacing(1);              // set framerate = refreshrate/2 (and enable vsync at the same time) 

    // make sure backlight is on
    if (PIN_BACKLIGHT != 255) {
        pinMode(PIN_BACKLIGHT, OUTPUT);
        digitalWrite(PIN_BACKLIGHT, HIGH);
    }

    // get elapsed seconds today 
    timeSecs = elapsedSecsToday(now());

    // time increment vector
    interval25ms.begin(incrTime, 25000); // 25ms

    // clear display black
    im.fillScreen(RGB565_Black);
}

/// <summary>
/// time increment vector
/// </summary>
void incrTime() {
    incT += 0.025f;
    updateFace = true;
}

/// <summary>
/// main loop
/// </summary>
void loop() {
    // every 25ms
    if (updateFace) {
        updateFace = false;

        // resync time over one second difference
        if (fabs(round(timeSecs + incT) - elapsedSecsToday(now())) > 1.0f) {
            incT = 0.0f;
            timeSecs = elapsedSecsToday(now());
        }

        // render watch face
        renderFace(timeSecs + incT, second() / 20);
    }
}

/// <summary>
/// draw the clock face
/// </summary>
/// <param name="t"></param>
/// <param name="faceType"></param>
static void renderFace(float t, uint16_t faceType) {
    float d_angle = day() * DAY_ANGLE;
    float h_angle = t * HOUR_ANGLE;
    float m_angle = t * MINUTE_ANGLE;
    float s_angle = t * SECOND_ANGLE;
    // use float pixel position for smooth AA motion
    float xp = 0.0f, yp = 0.0f;

    // format digiclock and day
    char bufDigiClock[9];
    sprintf(bufDigiClock, "%02d:%02d:%02d", hour(), minute(), second());
    char bufDay[3];
    sprintf(bufDay, "%02d", day());

    // switch on face
    switch (faceType) {
    case 0:
        // copy watchface to current image
        im.copyFrom(wFace1);
        // draw digiclock
        drawTextCenterX(im, bufDigiClock, 210, 220, CLOCK_DIGI, DSEG7_Classic_Bold_20, 0.5f);
        // draw day
        im.drawText(bufDay, iVec2(191, 165), CLOCK_DAY, font_tgx_OpenSans_14, false, 1.0f);
        // draw hour hand
        getCoord(CLOCK_R, CLOCK_Y, &xp, &yp, H_HAND_LENGTH, h_angle);
        im.drawWideLine({CLOCK_R, CLOCK_Y}, {xp, yp}, 8.0f, CLOCK_FG, 1.0f);
        im.drawWideLine({CLOCK_R, CLOCK_Y}, {xp, yp}, 4.0f, CLOCK_BG, 1.0f);
        // draw minute hand
        getCoord(CLOCK_R, CLOCK_Y, &xp, &yp, M_HAND_LENGTH, m_angle);
        im.drawWideLine({CLOCK_R, CLOCK_Y}, {xp, yp}, 8.0f, CLOCK_FG, 1.0f);
        im.drawWideLine({CLOCK_R, CLOCK_Y}, {xp, yp}, 4.0f, CLOCK_BG, 1.0f);
        // draw the central pivot circle
        im.drawSpot({CLOCK_R, CLOCK_Y}, 8.0f, CLOCK_FG, 1.0f);
        // draw the second hand
        getCoord(CLOCK_R, CLOCK_Y, &xp, &yp, S_HAND_LENGTH, s_angle);
        im.drawWedgeLine({CLOCK_R, CLOCK_Y}, {xp, yp}, 5.0f, 1.0f, SECCOND_FG, 1.0f);
        break;

    case 1:
        // copy watchface to current image
        im.copyFrom(wFace2);
        // draw digiclock
        drawTextCenterX(im, bufDigiClock, 188, 230, CLOCK_DIGI, DSEG7_Classic_Bold_14, 0.5f);
        // draw day
        im.drawText(bufDay, iVec2(113, 240), CLOCK_DAY, font_tgx_OpenSans_14, false, 1.0f);
        // draw hour hand
        getCoord(CLOCK_R, CLOCK_Y, &xp, &yp, H_HAND_LENGTH, h_angle);
        im.drawWedgeLine({CLOCK_R, CLOCK_Y}, {xp, yp}, 7.0f, 2.0f, RGB565_Teal, 1.0f);
        // draw minute hand
        getCoord(CLOCK_R, CLOCK_Y, &xp, &yp, M_HAND_LENGTH, m_angle);
        im.drawWedgeLine({CLOCK_R, CLOCK_Y}, {xp, yp}, 7.0f, 2.0f, RGB565_Teal, 1.0f);
        // draw the central pivot circle
        im.drawSpot({CLOCK_R, CLOCK_Y}, 8.0f, CLOCK_FG, 1.0f);
        // draw the econd hand
        getCoord(CLOCK_R, CLOCK_Y, &xp, &yp, S_HAND_LENGTH, s_angle);
        im.drawWedgeLine({CLOCK_R, CLOCK_Y}, {xp, yp}, 5.0f, 1.0f, SECCOND_FG, 1.0f);
        break;

    case 2:
        // copy watchface to current image
        im.copyFrom(wFace3);
        // draw digiclock
        drawTextCenterX(im, bufDigiClock, 217, 230, CLOCK_DIGI, DSEG7_Classic_Bold_14, 0.5f);
        // draw the second pivot circle
        im.drawSpot({64, CLOCK_Y + 1}, 4.0f, CLOCK_FG, 1.0f);
        // draw the second hand
        getCoord(64, CLOCK_Y + 1, &xp, &yp, 32, s_angle);
        im.drawWedgeLine({64, CLOCK_Y + 1}, {xp, yp}, 5.0f, 1.0f, SECCOND_FG, 1.0f);
        // draw the day pivot circle
        im.drawSpot({175, CLOCK_Y + 1}, 4.0f, CLOCK_FG, 1.0f);
        // draw the day hand
        getCoord(175, CLOCK_Y + 1, &xp, &yp, 32, d_angle);
        im.drawWedgeLine({175, CLOCK_Y + 1}, {xp, yp}, 5.0f, 1.0f, SECCOND_FG, 1.0f);
        // draw hour hand
        getCoord(CLOCK_R, CLOCK_Y, &xp, &yp, H_HAND_LENGTH, h_angle);
        im.drawWideLine({CLOCK_R, CLOCK_Y}, {xp, yp}, 8.0f, RGB32(229, 157, 0), 1.0f);
        im.drawWideLine({CLOCK_R, CLOCK_Y}, {xp, yp}, 4.0f, RGB32(38, 35, 45), 1.0f);
        // draw minute hand
        getCoord(CLOCK_R, CLOCK_Y, &xp, &yp, M_HAND_LENGTH, m_angle);
        im.drawWideLine({CLOCK_R, CLOCK_Y}, {xp, yp}, 8.0f, RGB32(229, 157, 0), 1.0f);
        im.drawWideLine({CLOCK_R, CLOCK_Y}, {xp, yp}, 4.0f, RGB32(38, 35, 45), 1.0f);
        // draw the central pivot circle
        im.drawSpot({CLOCK_R, CLOCK_Y}, 6.0f, CLOCK_FG, 1.0f);
        break;
    }

    // draw footer
    char bufFooter[30];
    sprintf(bufFooter, "%s, %s %02dst, %04d", weekDays[weekday() - 1], monthNames[month() - 1], day(), year());
    drawTextCenterX(im, bufFooter, 305, 240, CLOCK_FOOTER, font_tgx_OpenSans_14, 1.0f);

    // update display
    tft.update(fb, false);
}

/// <summary>
/// get teensy time
/// </summary>
/// <returns></returns>
time_t getTeensy3Time() {
    return Teensy3Clock.get();
}

/// <summary>
/// get coordinates of end of a line, pivot at x,y, length r, angle a
/// coordinates are returned to caller via the xp and yp pointers
/// </summary>
/// <param name="x"></param>
/// <param name="y"></param>
/// <param name="xp"></param>
/// <param name="yp"></param>
/// <param name="r"></param>
/// <param name="a"></param>
void getCoord(int16_t x, int16_t y, float* xp, float* yp, int16_t r, float a) {
    float sx1 = cosf((a - 90.0f) * DEG2RAD);
    float sy1 = sinf((a - 90.0f) * DEG2RAD);
    *xp = sx1 * r + x;
    *yp = sy1 * r + y;
}

/// <summary>
/// center text horizontaly
/// </summary>
/// <param name="img"></param>
/// <param name="text"></param>
/// <param name="y"></param>
/// <param name="w"></param>
/// <param name="color"></param>
/// <param name="font"></param>
/// <param name="opacity"></param>
void drawTextCenterX(Image<RGB565> img, const char* text, int y, int w, RGB565 color, const ILI9341_t3_font_t& font, float opacity) {
    auto b = im.measureText(text, { 0,0 }, font, false);
    img.drawText(text, iVec2((w / 2) - b.lx() / 2, y), color, font, false, opacity);
}
void drawTextCenterX(Image<RGB565> img, const char* text, int y, int w, RGB565 color, const GFXfont& font, float opacity) {
    auto b = im.measureText(text, { 0,0 }, font, false);
    img.drawText(text, iVec2((w / 2) - b.lx() / 2, y), color, font, false, opacity);
}

