/********************************************************************
*
* TGX library.
*
* This example show how to call (most of) the 2D drawing methods
* of the tgx library. 
*
* EXAMPLE FOR TEENSY 4 / 4.1
*
* DISPLAY: ILI9341 (320x240)
*
********************************************************************/


// This example runs on teensy 4.0/4.1 with ILI9341 via SPI. 
// install from arduino's library manager or from https://github.com/vindar/ILI9341_T4
#include <ILI9341_T4.h> 

// the tgx library 
#include <tgx.h> 

// font used
#include <font_tgx_OpenSans_Bold.h>
#include "font_tgx_OpenSans.h"
#include "font_tgx_Arial_Bold.h"

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



//
// testing method for reading and writing pixels
//
void test_read_write_pixels()
    {
    im.clear(tgx::RGB565_Black); // set the image fully black
    for (int i = 0; i < 310; i += 15) {
        for (int j = 0; j < 230; j += 15) {
            im.drawPixel({ i, j }, tgx::RGB565_White);              // integer valued position vector
            im.drawPixelf({ i + 4.4f, j + 5.6f }, tgx::RGB565_Red); // floating-point valued position vector
            im(i + 10, j + 10) = tgx::RGB565_Green;                    // direct acces to memory loaction of the pixel. 
            }
        }
    }


//
// testing methods for filling an image
//
void test_filling()
    {
    im.fillScreen(tgx::RGB565_Black); 							  // same as im.clear(tgx::RGB565_Black);
    auto im1 = im(0, 106, 0, 240); 								  // create subimage = left third of the image
    im1.fillScreenVGradient(tgx::RGB565_Blue, tgx::RGB565_Red); 	  // fill with vertical gradient from blue to red
    auto im2 = im(214, 320, 0, 240); 							  // create subimage = right third of the image
    im2.fillScreenHGradient(tgx::RGB565_Green, tgx::RGB565_Orange); // fill with horizontal gradient from green to orange
    }


//
// testing methods for flood-filling a region
//
void test_floodfilling()
    {
    im.clear(tgx::RGB565_Black);
    im.drawLine({ 0,50 }, { 320,160 }, tgx::RGB565_White);
    im.drawLine({ 0,240 }, { 320,10 }, tgx::RGB565_Red);
    im.fill({ 319,239 }, tgx::RGB565_Red, tgx::RGB565_Blue); // fill starting {319,239}, region delimited by the red line
    im.fill({ 0,0 }, tgx::RGB565_Olive);                    // fill starting {0,0}
    }


//
// testing methods for blitting sprites/images. 
//
void test_blitting()
    {

    // create a sprite
    tgx::RGB565 buf[120 * 40]; // memory for the sprite image
    tgx::Image<tgx::RGB565> sprite(buf, 120, 40); // the sprite image...
    sprite.clear(tgx::RGB565_Green); // ... has green background
    sprite.fillCircle({ 20, 20 }, 13, tgx::RGB565_Black, tgx::RGB565_Black); // ... a black circle
    sprite.drawTextEx("Sprite", {76, 20}, font_tgx_Arial_Bold_20, tgx::Anchor::CENTER, false, false, tgx::RGB565_Black); // ... and a black text "sprite".

    im.fillScreenHGradient(tgx::RGB565_Blue, tgx::RGB565_Red); // fill the screen with horizontal gradient from green to orange

    im.blit(sprite, { 10, 10 });  // simple blitting

    im.blitRotated(sprite, { 10, 80 }, 270, 0.25f); // blit the sprite rotated by 270 degrees clockwise, 0.25% opacity

    im.blitMasked(sprite, tgx::RGB565_Black, {120,60}); // blit the sprite with black as a the transparent color.

    im.blitScaledRotated(sprite, {60, 20}, { 100, 160 }, 0.6f, 60.0f, 0.5f); // blit the sprite scaled at 0.6, rotated by 60 degrees, half opacity

    im.blitScaledRotatedMasked(sprite, tgx::RGB565_Green, { 60, 20 }, {230, 160}, 1.5f, -25.0f); // blit the sprite scaled at 1.5, rotated by -25 degrees, with green set as the transparent color.
    }


//
// testing methods for drawing horizontal and vertical lines
// 
void test_hvlines()
    {
    im.clear(tgx::RGB565_Black);
    for (int i = 0; i < 20; i++) {
        im.drawFastVLine({ i * 16, i * 3 }, 100, tgx::RGB565_Red, i / 20.0f);	// draw a vertical line
        im.drawFastHLine({ i * 3, i * 12 }, 200, tgx::RGB565_Green, i / 20.0f);	// draw an horizontal line
        }
    }


//
// testing methods for drawing lines
// 
void test_lines()
    {
    im.clear(tgx::RGB565_Black);					  //
    im.drawLine({ 10,10 }, { 300, 60 }, tgx::RGB565_Red);       // simple red line
    im.drawLineAA({ 200,200 }, { 300, 10 }, tgx::RGB565_Green); // green line with anti-aliasing
    im.drawThickLineAA({ 10, 120 }, { 250, 10 }, 10, tgx::END_STRAIGHT, tgx::END_ROUNDED, tgx::RGB565_White); // thick line, rounded at the start, straight at the end.
    im.drawWedgeLineAA({ 300, 220 }, { 150, 100 }, 10, tgx::END_ARROW_1, 20, tgx::END_ARROW_SKEWED_5, tgx::RGB565_Orange, 0.5f); // wedge line ending with arrows, half opacity.
    }


//
// testing methods for drawing rectangles
// 
void test_rectangles()
    {
    im.clear(tgx::RGB565_Black);					  //
    im.drawRect({ 10,70,10,70 }, tgx::RGB565_White); // simple green square, full opacity
    im.drawThickRect({ 30,180,30,140 }, 7, tgx::RGB565_Orange, 0.5f); // 7 pixels thick orange rectangle, half opacity
    im.fillRect({ 5, 80, 20, 200 }, tgx::RGB565_Yellow, 0.25f); // yellow filled rectangle, 25% opacity
    im.fillThickRect({ 50, 150, 150, 220 }, 5, tgx::RGB565_Gray, tgx::RGB565_Red, 0.5f); // gray filled, 5 pixels thick red rectangle, 50% opacity
    im.fillRectHGradient({ 140, 310, 100, 160 }, tgx::RGB565_Red, tgx::RGB565_Blue); // rectangle filled with horizontal gradient from red to blue. full opacity.
    im.fillRectVGradient({ 190, 300, 10, 230 }, tgx::RGB565_Cyan, tgx::RGB565_Purple, 0.5f); // rectangle filled with vertical gradient from cyan to purple. half opacity.
    }


//
// testing methods for drawing rounded rectangles
// 
void test_rounded_rectangles()
    {
    im.clear(tgx::RGB565_Black);					  //
    im.drawRoundRectAA({ 10,70,10,70 }, 4, tgx::RGB565_White); // simple 4-pixels rounded square
    im.drawThickRoundRectAA({ 30,180,30,140 }, 20, 7, tgx::RGB565_Orange, 0.5f); // 7 pixels thick, 20 pixels rounded corner rectangle, half opacity
    im.fillRoundRectAA({ 140, 310, 170, 230 }, 25, tgx::RGB565_Blue); // filled 25-pixel rounded corner rectangle
    im.fillThickRoundRectAA({ 100, 250, 60, 200 }, 10, 5, tgx::RGB565_Gray, tgx::RGB565_Red, 0.5f); // 5 pixels thick, 10 pixel rounded corner rectangle, 50% opacity
    }


//
// testing methods for drawing triangles
// 
void test_triangles()
    {
    im.clear(tgx::RGB565_Black);					  //
    im.drawTriangle({ 10,10 }, { 60, 30 }, {40, 80}, tgx::RGB565_White); // simple triangle
    im.fillTriangle({150, 5}, {80, 180}, {300, 20} , tgx::RGB565_Purple, tgx::RGB565_Magenta); // filled triangle with border
    im.drawTriangleAA({ 20,120 }, { 150, 100 }, { 70, 239}, tgx::RGB565_Green); // green triangle with AA
    im.drawThickTriangleAA({ 160, 60 }, { 240,30 }, { 300, 80 }, 10, tgx::RGB565_Cyan, 0.5f); // 10 pixels thick triangle, 50% opacity, with AA
    im.fillTriangleAA({ 200, 220 }, { 315,210 }, { 150, 120 }, tgx::RGB565_Yellow); // filled triangle with AA.
    im.fillThickTriangleAA({ 130,200 }, { 280, 110 }, { 250, 170 }, 5, tgx::RGB565_Lime, tgx::RGB565_Orange, 0.5f); // filled 5 pixels thick triangle, 50% opacity, with AA
    }


//
// testing methods for drawing triangles (advanced methods using the 3D rasterizer)
// 
void test_triangles_advanced()
    {
    tgx::RGB565 buf[50 * 50]; // memory for texture image
    tgx::Image<tgx::RGB565> tex(buf, 50, 50); // the texture image...
    tex.clear(tgx::RGB565_Blue); // ... has blue background ..
    tex.fillThickCircleAA({ 25,25 }, 24, 5, tgx::RGB565_Red, tgx::RGB565_Green); // ...and red-filled circle with green outline in the center. 

    im.clear(tgx::RGB565_Black);					  //    
    im.drawTexturedTriangle(tex, { 0,0 }, { 50, 0 }, { 50, 50 }, { 10, 10 }, { 100, 20 }, { 30, 100 }); // textured triangle, full opacity
    im.drawGradientTriangle({ 20, 140 }, { 160, 50 }, { 230, 200 }, tgx::RGB565_Blue, tgx::RGB565_Orange, tgx::RGB565_Black); // gradient triangle, full opacity. 
    im.drawTexturedGradientTriangle(tex, { 0,0 }, { 50, 0 }, { 50, 50 }, { 120, 230 }, { 300, 20 }, { 280, 170 }, tgx::RGB565_Red, tgx::RGB565_Green, tgx::RGB565_Blue, 0.5f); // texture triangle with color gradient. half opacity.
    }


//
// testing methods for drawing quads
// 
void test_quads()
    {
    im.clear(tgx::RGB565_Black);					  //
    im.drawQuad({ 10,10 }, { 60, 30 }, { 40, 80 }, { 5, 40 }, tgx::RGB565_White); // simple quad
    im.fillQuad({ 150, 5 }, { 80, 180 }, { 170, 40 }, { 300, 20 }, tgx::RGB565_Purple); // filled quad
    im.drawQuadAA({ 20,120 }, { 150, 100 }, { 150, 140 }, { 70, 239 }, tgx::RGB565_Green); // green quad with AA
    im.drawThickQuadAA({ 160, 60 }, { 240,30 }, { 300, 80 }, { 230, 120 }, 10, tgx::RGB565_Cyan, 0.5f); // 10 pixels thick quad, 50% opacity, with AA
    im.fillQuadAA({ 200, 220 }, { 315,210 }, { 250, 150 }, { 150, 120 }, tgx::RGB565_Yellow); // filled quad with AA
    im.fillThickQuadAA({ 130,200 }, { 160, 140 }, { 230, 100 }, { 250, 170 }, 5, tgx::RGB565_Lime, tgx::RGB565_Orange, 0.5f); //filled, 5 pixels thick quad, 50% opacity, with AA
    }


//
// testing methods for drawing quads  (advanced methods using the 3D rasterizer)
// 
void test_quads_advanced()
    {
    tgx::RGB565 buf[50 * 50]; // memory for texture image
    tgx::Image<tgx::RGB565> tex(buf, 50, 50); // the texture image...
    tex.clear(tgx::RGB565_Blue); // ... has blue background ..
    tex.fillThickCircleAA({ 25,25 }, 24, 5, tgx::RGB565_Red, tgx::RGB565_Green); // ...and red-filled circle with green outline in the center. 

    im.clear(tgx::RGB565_Black);					  //
    im.drawTexturedQuad(tex, { 0,0 }, { 50, 0 }, { 50, 50 }, { 0, 50 }, { 10, 10 }, { 100, 20 }, { 80, 40 }, { 30, 120 }); // textured quad, 
    im.drawGradientQuad({ 20, 140 }, { 160, 50 }, { 180, 55 }, { 230, 200 }, tgx::RGB565_Blue, tgx::RGB565_Orange, tgx::RGB565_Green, tgx::RGB565_Black); // gradient quad
    im.drawTexturedGradientQuad(tex, { 0,0 }, { 50, 0 }, { 50, 50 }, { 0, 50 }, { 120, 230 }, { 280, 20 }, { 310, 170 }, { 250,210 }, tgx::RGB565_Red, tgx::RGB565_Green, tgx::RGB565_Blue, tgx::RGB565_Navy, 0.5f); // texture quad with color gradient. half opacity.
    }


//
// testing methods for drawing polylines
// 
void test_polylines()
    {
    im.clear(tgx::RGB565_Black);					  //

    const tgx::iVec2 tabPoints1[] = { {10, 10}, {50, 10}, {80, 50}, {20, 110}, {20, 180}, {300, 150} };
    im.drawPolyline(6, tabPoints1, tgx::RGB565_White); // simple polyline

    const tgx::fVec2 tabPoints2[] = { {200, 100}, {100, 20}, {130, 120}, {10, 200}, {60, 180}, {300, 230} };
    im.drawPolylineAA(6, tabPoints2, tgx::RGB565_Red); // polyline with AA

    const tgx::fVec2 tabPoints3[] = { {30, 30}, {200, 15}, {300, 200}, {140, 230}, {80, 150}, {20, 230} };
    im.drawThickPolylineAA(6, tabPoints3, 12, tgx::END_ARROW_1, tgx::END_ROUNDED, tgx::RGB565_Orange, 0.5f); // thick polyline, arrow at start, rounded at end, half opacity, with AA
    }


//
// testing methods for drawing polygons
// 
void test_polygons()
    {
    im.clear(tgx::RGB565_Black);					  //

    tgx::iVec2 tabPoints1[] = { {10, 10}, {50, 10}, {80, 40}, {50, 90}, {10, 70} };
    im.drawPolygon(5, tabPoints1, tgx::RGB565_White); // simple polygon

    tgx::iVec2 tabPoints2[] = { {90, 20}, {50, 40}, {60, 80}, {150, 90}, {190, 45} };
    im.fillPolygon(5, tabPoints2, tgx::RGB565_Yellow, 0.5f); // filled polygon, half opacity.

    tgx::fVec2 tabPoints3[] = { {140, 60}, {250, 10}, {310, 100}, {300, 150}, {240, 170}, {220, 120} };
    im.drawPolygonAA(6, tabPoints3, tgx::RGB565_Red); // polygon with AA

    tgx::fVec2 tabPoints4[] = { {10, 120}, {50, 80}, {100, 150}, {40, 200}, {50, 170} };
    im.drawThickPolygonAA(5, tabPoints4, 6, tgx::RGB565_Cyan, 0.5f); // thick polygon, 50% opacity, with AA

    tgx::fVec2 tabPoints5[] = { {50, 120}, {130, 70}, {170, 100}, {210, 180}, {150, 190} };
    im.fillPolygonAA(5, tabPoints5, tgx::RGB565_Lime, 0.5f); // filled polygon, 50% opacity, with AA

    tgx::fVec2 tabPoints6[] = { {120, 150}, {210, 50}, {250, 70}, {300, 100}, {290, 170}, {230, 210} };
    im.fillThickPolygonAA(6, tabPoints6, 10, tgx::RGB565_Purple, tgx::RGB565_Orange, 0.5f); // filled thick polygon, 50% opacity, with AA
    }


//
// testing methods for drawing circles
// 
void test_circles()
    {
    im.clear(tgx::RGB565_Black);					  //
    im.drawCircle({ 50, 50 }, 48, tgx::RGB565_White); // simple circle
    im.fillCircle({ 120, 80 }, 70, tgx::RGB565_Blue, tgx::RGB565_Red, 0.5f); // filled circle, 50% opacity
    im.drawCircleAA({ 210, 90 }, 60, tgx::RGB565_Green); // green circle with AA
    im.drawThickCircleAA({ 60, 180 }, 40, 10, tgx::RGB565_Purple); //thick maroon circle, with AA
    im.fillCircleAA({ 230, 150 }, 70, tgx::RGB565_Yellow, 0.5f); // filled circle with AA, 50% opacity
    im.fillThickCircleAA({ 160,160 }, 55, 8, tgx::RGB565_Teal, tgx::RGB565_Orange, 0.5f); // filled circle with thick outline, with AA, 50% opacity 
    }


//
// testing methods for drawing ellipses
// 
void test_ellipses()
    {
    im.clear(tgx::RGB565_Black);					  //
    im.drawEllipse({ 50, 50 }, { 48, 20 }, tgx::RGB565_White); // simple ellipse
    im.fillEllipse({ 120, 80 }, { 70, 50 }, tgx::RGB565_Blue, tgx::RGB565_Red, 0.5f); // filled ellipse, 50% opacity
    im.drawEllipseAA({ 210, 90 }, { 60, 30 }, tgx::RGB565_Green); // ellipse with AA
    im.drawThickEllipseAA({ 60, 180 }, { 35, 55 }, 10, tgx::RGB565_Purple); // thick ellipse, with AA
    im.fillEllipseAA({ 230, 150 }, { 45, 70 }, tgx::RGB565_Yellow, 0.5f); // filled ellipse with AA, 50% opacity
    im.fillThickEllipseAA({ 160,160 }, { 55, 65 }, 8, tgx::RGB565_Teal, tgx::RGB565_Orange, 0.5f); // filled ellipse with thick outline, with AA, 50% opacity 
    }


//
// testing methods for drawing arcs and pies
// 
void test_arcs_pies()
    {
    im.clear(tgx::RGB565_Black);					  //
    im.fillCircleSectorAA({ 60,60 }, 40, 50, 270, tgx::RGB565_Red); // filled circle sector between 50 and 270 degrees
    im.drawCircleArcAA({ 250, 60 }, 50, 250, 90, tgx::RGB565_Green); // green circle arc from 250 to 90 degrees
    im.drawThickCircleArcAA({ 80, 160 }, 75, 220, 160, 20, tgx::RGB565_Purple, 0.5f); // thick circle arc from 220 to 160 degrees, 50% opacity
    im.fillThickCircleSectorAA({ 230, 120 }, 100, 140, 340, 10, tgx::RGB565_Olive, tgx::RGB565_Orange, 0.5f); // filled thick circle sector from 140 to 340 degrees, 50% opacity
    }


//
// testing methods for drawing Bezier curves
// 
void test_bezier()
    {
    im.clear(tgx::RGB565_Black);					  //
    im.drawQuadBezier({ 10, 10 }, { 250, 180 }, { 280, 20 }, 1.0f, true, tgx::RGB565_White); // simple quadratic bezier curve
    im.drawCubicBezier({ 10, 40 }, { 280, 50 }, { 20, 80 }, { 300, 200 }, true, tgx::RGB565_Green); // simple cubic bezier curve
    im.drawThickQuadBezierAA({ 30, 150 }, { 300, 20 }, { 0,0 }, 2.0f, 10, tgx::END_STRAIGHT, tgx::END_ROUNDED, tgx::RGB565_Red, 0.5f); // thick quadratic bezier curve, with AA, 50% opacity.
    im.drawThickCubicBezierAA({ 80, 80 }, { 305, 150 }, { 0, 240 }, { 290, 240 }, 10, tgx::END_ARROW_5, tgx::END_ARROW_SKEWED_2, tgx::RGB565_Orange, 0.5f); // thick cubic bezier curve, with AA, 50% opacity.
    }


//
// testing methods for drawing splines
// 
void test_splines()
    {
    im.clear(tgx::RGB565_Black);					  //

    const tgx::iVec2 tabPoints1[] = { {30, 10}, {10, 40}, {40, 70}, {10, 100}, {60, 130}, {10, 160}, {80, 190}, {10, 220} };
    im.drawQuadSpline(8, tabPoints1, true, tgx::RGB565_White); // simple quadratic spline

    const tgx::iVec2 tabPoints2[] = { {10, 10}, {50, 40}, {10, 70}, {70, 100}, {10, 130}, {90, 160}, {10, 190}, {120, 220} };
    im.drawCubicSpline(8, tabPoints2, true, tgx::RGB565_Red); // simple cubic spline

    const tgx::fVec2 tabPoints3[] = { {70, 10}, {40, 40}, {80, 70}, {30, 100}, {120, 130}, {60, 160}, {150, 190}, {80, 220} };
    im.drawThickQuadSplineAA(8, tabPoints3, 5, tgx::END_ROUNDED, tgx::END_ROUNDED, tgx::RGB565_Yellow, 0.5f); // thick quadratic spline, 50% opacity, with AA

    const tgx::fVec2 tabPoints4[] = { {60, 20}, {120, 40}, {50, 70}, {130, 100}, {60, 130}, {110, 160}, {70, 190}, {60, 220} };
    im.drawThickCubicSplineAA(8, tabPoints4, 6, tgx::END_ARROW_3, tgx::END_ARROW_SKEWED_3, tgx::RGB565_Green, 0.5f); // thick cubic spline, 50% opacity, with AA

    const tgx::fVec2 tabPoints5[] = { {140, 20}, {160, 10}, {200, 50}, {250, 20}, {300, 100}, {250, 160}, {200, 100}, {160, 150} };
    im.drawThickClosedSplineAA(8, tabPoints5, 12, tgx::RGB565_Blue); // thick closed spline, with AA

    const tgx::iVec2 tabPoints6[] = { {240, 10}, {310, 50}, {200, 130} };
    im.drawClosedSpline(3, tabPoints6, tgx::RGB565_Magenta, 0.5f); // simple closed spline, 50% opacity

    const tgx::fVec2 tabPoints7[] = { {300, 220}, {240, 220}, {240, 180}, {300, 190} };
    im.fillClosedSplineAA(4, tabPoints7, tgx::RGB565_Cyan); // interior filled closed spline, with AA

    const tgx::fVec2 tabPoints8[] = { {150, 210}, {160, 150}, {250, 120}, {280, 150}, {260, 190} };
    im.fillThickClosedSplineAA(5, tabPoints8, 5, tgx::RGB565_Gray, tgx::RGB565_Red, 0.5f); // interior filled thick closed spline, 50% opacity, with AA
    }


//
// testing methods for drawing text
// 
void test_text()
    {
    im.clear(tgx::RGB565_Black);

    // drawing text: the anchor point use the baseline. 
    im.drawText("non-AA font are ugly...", { 5, 10 }, font_tgx_Arial_Bold_8, tgx::RGB565_Blue); // a non AA font
    
    im.drawText("AA font, 9pt", { 5, 25 }, font_tgx_OpenSans_9, tgx::RGB565_Green);     // AA font, 9pt
    im.drawText("AA font, 10pt", { 5, 40 }, font_tgx_OpenSans_10, tgx::RGB565_Green);   // AA font, 10pt
    im.drawText("AA font, 14pt", { 5, 60 }, font_tgx_OpenSans_14, tgx::RGB565_Green);   // AA font, 14pt
    im.drawText("AA font, 18pt", { 5, 83 }, font_tgx_OpenSans_20, tgx::RGB565_Green);   // AA font, 18pt
    
    // drawing on the baseline
    im.drawFastHLine({ 130, 30 }, 160, tgx::RGB565_White, 0.5f);
    im.drawFastHLine({ 130, 45 }, 160, tgx::RGB565_White, 0.5f);
    im.drawText("The quick brown fox jumps", { 130, 30 }, font_tgx_OpenSans_12, tgx::RGB565_Orange);
    im.drawText("over the lazy dog !", { 130, 45 }, font_tgx_OpenSans_12, tgx::RGB565_Orange);
    
    // finding the bounding box of a text. 
    const char* tbb = "Text bounding box";
    tgx::iBox2 bb = im.measureText(tbb, { 230, 70 }, font_tgx_OpenSans_Bold_14, tgx::Anchor::CENTER, false, false);
    im.fillRect(bb, tgx::RGB565_Yellow, 0.5f);
    im.drawTextEx(tbb, { 230, 70 }, font_tgx_OpenSans_Bold_14, tgx::Anchor::CENTER, false, false, tgx::RGB565_Blue);
    
    // draw text with different anchor points using drawTextEx()
    // -> the anchor is relative to the text bounding box
    im.drawRect({ 20, 300,100, 230 }, tgx::RGB565_Gray);
    im.drawFastHLine({ 20, 165 }, 280, tgx::RGB565_Gray);
    im.drawFastVLine({ 160, 100 }, 130, tgx::RGB565_Gray);
    tgx::iVec2 anchor_point;
    
    anchor_point = { 20, 100 };
    im.drawTextEx("TOPLEFT", anchor_point, font_tgx_OpenSans_10, tgx::Anchor::TOPLEFT, true, true, tgx::RGB565_Red);
    im.fillCircleAA(anchor_point, 1.5, tgx::RGB565_Green);

    anchor_point = { 300, 100 };
    im.drawTextEx("TOPRIGHT", anchor_point, font_tgx_OpenSans_10, tgx::Anchor::TOPRIGHT, true, true, tgx::RGB565_Red);
    im.fillCircleAA(anchor_point, 1.5, tgx::RGB565_Green);

    anchor_point = { 20, 230 };
    im.drawTextEx("BOTTOMLEFT", anchor_point, font_tgx_OpenSans_10, tgx::Anchor::BOTTOMLEFT, true, true, tgx::RGB565_Red);
    im.fillCircleAA(anchor_point, 1.5, tgx::RGB565_Green);
 
    anchor_point = { 300, 230 };
    im.drawTextEx("BOTTOMRIGHT", anchor_point, font_tgx_OpenSans_10, tgx::Anchor::BOTTOMRIGHT, true, true, tgx::RGB565_Red);
    im.fillCircleAA(anchor_point, 1.5, tgx::RGB565_Green);

    anchor_point = { 160, 100 };
    im.drawTextEx("CENTERTOP", anchor_point, font_tgx_OpenSans_10, tgx::Anchor::CENTERTOP, true, true, tgx::RGB565_Red);
    im.fillCircleAA(anchor_point, 1.5, tgx::RGB565_Green);
    
    anchor_point = { 160, 230 };
    im.drawTextEx("CENTERBOTTOM", anchor_point, font_tgx_OpenSans_10, tgx::Anchor::CENTERBOTTOM, true, true, tgx::RGB565_Red);
    im.fillCircleAA(anchor_point, 1.5, tgx::RGB565_Green);
    
    anchor_point = { 20, 165 };
    im.drawTextEx("CENTERLEFT", anchor_point, font_tgx_OpenSans_10, tgx::Anchor::CENTERLEFT, true, true, tgx::RGB565_Red);
    im.fillCircleAA(anchor_point, 1.5, tgx::RGB565_Green);
    
    anchor_point = { 300, 165 };
    im.drawTextEx("CENTERRIGHT", anchor_point, font_tgx_OpenSans_10, tgx::Anchor::CENTERRIGHT, true, true, tgx::RGB565_Red);
    im.fillCircleAA(anchor_point, 1.5, tgx::RGB565_Green);
    
    anchor_point = { 160, 165 };
    im.drawTextEx("CENTER", anchor_point, font_tgx_OpenSans_10, tgx::Anchor::CENTER, true, true, tgx::RGB565_Red);
    im.fillCircleAA(anchor_point, 1.5, tgx::RGB565_Green);        
    }




void setup()
    {    
    Serial.begin(9600); 
    // output debug infos to serial port.
    tft.output(&Serial);
    // initialize the ILI9341 screen
    while(!tft.begin(SPI_SPEED));

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



// add a subtitle, display the image and wait for 2 seconds
void displayAndWait(const char* title)
    {    
    tgx::iBox2 bb = im.measureText(title, { 2, 237 }, font_tgx_OpenSans_14, tgx::Anchor::BOTTOMLEFT, false, false);
    im.fillRect(bb, tgx::RGB565_White);
    im.drawTextEx(title, { 2, 237 }, font_tgx_OpenSans_14, tgx::Anchor::BOTTOMLEFT, false, false, tgx::RGB565_Black);    
    tft.update(fb);
    delay(2000);
    }


void loop()
    {       
    test_read_write_pixels();
    displayAndWait("test: reading and writing pixels...");

    test_filling();
    displayAndWait("test: screen filling...");

    test_floodfilling();
    displayAndWait("test: flood-filling...");

    test_blitting();
    displayAndWait("test: blitting sprites...");

    test_hvlines();
    displayAndWait("test: drawing vertical/horizontal lines...");

    test_lines();
    displayAndWait("test: drawing lines...");

    test_rectangles();
    displayAndWait("test: drawing rectangles...");

    test_rounded_rectangles();
    displayAndWait("test: drawing rounded rectangles...");

    test_triangles();
    displayAndWait("test: drawing triangles...");

    test_triangles_advanced();
    displayAndWait("test: drawing triangles (advanced)...");

    test_quads();
    displayAndWait("test: drawing quads...");

    test_quads_advanced();
    displayAndWait("test: drawing quads (advanced)...");

    test_polylines();
    displayAndWait("test: drawing polylines...");

    test_polygons();
    displayAndWait("test: drawing polygons...");

    test_circles();
    displayAndWait("test: drawing circles...");

    test_ellipses();
    displayAndWait("test: drawing ellipses...");

    test_arcs_pies();
    displayAndWait("test: drawing arcs and pies...");

    test_bezier();
    displayAndWait("test: drawing Bezier curves...");

    test_splines();
    displayAndWait("test: drawing splines...");
    
    test_text();
    displayAndWait("test: drawing text...");  

    }


/** end of file */

