@page intro_api_2D Using the 2D API.


**Below is a description, with examples, of most 2D drawing primitives implemented in TGX.** 

- For additional details, look directly into the main header file \ref Image.h
- Do not forget to check the examples located in the `/examples/`  subdirectory of the library. In particular, the example `TestPrimitives` contains all the code list below. 

@section sec_2Dprimitives 2D Drawing methods. 


@subsection subsec_filling Filling the screen. 

![test_screen_filling](../test_screen_filling.png)   

*Code used to generate the image:*
~~~{.cpp}
tgx::RGB32 buffer[320*240];						//
tgx::Image<tgx::RGB32> im(buffer, 320, 240); 	// create a 320x240 image. 

im.fillScreen(tgx::RGB32_Black); 							  // same as im.clear(tgx::RGB32_Black);
auto im1 = im(0, 106, 0, 240); 								  // create subimage = left third of the image
im1.fillScreenVGradient(tgx::RGB32_Blue, tgx::RGB32_Red); 	  // fill with vertical gradient from blue to red
auto im2 = im(214, 320, 0, 240); 							  // create subimage = right third of the image
im2.fillScreenHGradient(tgx::RGB32_Green, tgx::RGB32_Orange); // fill with horizontal gradient from green to orange
~~~
**Methods:** 
- \ref tgx::Image::clear "clear()" : set all pixels of the image with the same color.
- \ref tgx::Image::fillScreen "fillScreen()" : same as clear().
- \ref tgx::Image::fillScreenVGradient "fillScreenVGradient()" : fill the image with a vertical gradient between two colors.
- \ref tgx::Image::fillScreenHGradient "fillScreenHGradient()" : fill the image with an horizontal gradient between two colors.


---


@subsection subsec_rwpixels Reading/writing pixels

![test_points](../test_points.png)

*Code used to generate the image:*
~~~{.cpp}
tgx::RGB32 buffer[320*240];					  //
tgx::Image<tgx::RGB32> im(buffer, 320, 240);  // create a black 320x240 image. 
im.clear(tgx::RGB32_Black);					  //

for (int i = 0; i < 310; i += 15) {
    for (int j = 0; j < 230; j += 15){
        im.drawPixel({ i, j }, tgx::RGB32_White);              // integer valued position vector
        im.drawPixelf({ i + 4.4f, j + 5.6f }, tgx::RGB32_Red); // floating-point valued position vector
        im(i+10, j+10) =  tgx::RGB32_Green;                    // direct acces to memory loaction of the pixel. 
        }
    }
~~~
**Methods:**
- \ref tgx::Image::drawPixel "drawPixel()" : set the color of agiven pixel.
- \ref tgx::Image::drawPixelf "drawPixelf()" : set the color of agiven pixel, use floating point values position vector.
- \ref tgx::Image::readPixel() "readpixel()" : read the color of a pixel.
- \ref tgx::Image::operator()(tgx::iVec2) "operator()" : direct access to the pixel color memory location (read/write).




---

@subsection subsec_floodfill Flood-filling a region 

![test_region_filling](../test_region_filling.png)

*Code used to generate the image:*
~~~{.cpp}
tgx::RGB32 buffer[320*240];					  //
tgx::Image<tgx::RGB32> im(buffer, 320, 240);  // create a black 320x240 image. 
im.clear(tgx::RGB32_Black);					  //

im.drawLine({ 0,50 }, { 320,160 }, tgx::RGB32_White);  
im.drawLine({ 0,240 }, { 320,10 }, tgx::RGB32_Red);
im.fill({ 319,239 }, tgx::RGB32_Red, tgx::RGB32_Blue); // fill starting {319,239}, region delimited by the red line
im.fill({ 0,0 }, tgx::RGB32_Olive);                    // fill starting {0,0}
~~~
**Methods:** 
- \ref tgx::Image<color_t>::fill(tgx::iVec2, color_t) "fill(start_pos, new_color)" : fill a connected componenet with a given color.
- \ref tgx::Image<color_t>::fill(tgx::iVec2, color_t, color_t) "fill(start_pos, border_color, new_color)" :  fill a region delimited by a given color. 
 

---

@subsection subsec_blitting Blitting sprites

![test_blitting](../test_blitting.png)

*Code used to generate the image:* 
~~~{.cpp}
    // create a sprite: 
    // #include <font_tgx_OpenSans_Bold.h> is needed to use the font_tgx_Arial_Bold_20. 
    tgx::RGB32 buf[120 * 40]; // memory for the sprite image
    tgx::Image<tgx::RGB32> sprite(buf, 120, 40); // the sprite image...
    sprite.clear(tgx::RGB32_Green); // ... has green background
    sprite.fillCircle({ 20, 20 }, 13, tgx::RGB32_Black, tgx::RGB32_Black); // ... a black circle
    sprite.drawTextEx("Sprite", { 76, 20 }, font_tgx_Arial_Bold_20, tgx::Anchor::CENTER, false, false, tgx::RGB32_Black); // ... and a black text "sprite".

    im.fillScreenHGradient(tgx::RGB32_Blue, tgx::RGB32_Red); // fill the screen with horizontal gradient from green to orange

    im.blit(sprite, { 10, 10 });  // simple blitting
    im.blitRotated(sprite, { 10, 80 }, 270, 0.25f); // blit the sprite rotated by 270 degrees clockwise, 0.25% opacity
    im.blitMasked(sprite, tgx::RGB32_Black, { 120,60 }); // blit the sprite with black as a the transparent color.
    im.blitScaledRotated(sprite, { 60, 20 }, { 100, 160 }, 0.6f, 60.0f, 0.5f); // blit the sprite scaled at 0.6, rotated by 60 degrees, half opacity
    im.blitScaledRotatedMasked(sprite, tgx::RGB32_Green, { 60, 20 }, { 230, 160 }, 1.5f, -25.0f); // blit the sprite scaled at 1.5, rotated by -25 degrees, with green set as the transparent color.
~~~
**Methods:** 
- \ref tgx::Image<color_t>::blit() "blit(sprite, pos, opacity)" : blit a sprite image onto the image. 
- \ref tgx::Image<color_t>::blitRotated() "blitRotated(sprite, pos, angle, opacity)" : rotate a sprite (by quarter turns) and then blit it onto the image. 
- \ref tgx::Image<color_t>::blitMasked() "blitMasked(sprite, mask_color, pos, opacity)" : blit a sprite onto the image with one color set as transparent. 
- \ref tgx::Image<color_t>::blitScaledRotated() "blitScaledRotated(sprite, pos_src, pos_dst, scale, angle, opacity)" : rescale and rotate a sprite (by arbitrary angle) and then blit it onto the image. 
- \ref tgx::Image<color_t>::blitScaledRotatedMasked() "blitScaledRotatedMasked(sprite, mask_color, pos_src, pos_dst, scale, angle, opacity)" : rescale and rotate a sprite (by arbitrary angle) and then blit it onto the image with one color set as transparent. 

@note All the blit methods above also have an 'advanced' version which takes as input a user-defined blending operator instead of the opacity parameter and can operate on sprites with different color types than the destination image. see \ref tgx::Image for details...

See also \ref tgx::Image<color_t>::blitBackward() "blitBackward()" and methods for image copy/resizing/type conversion \ref tgx::Image<color_t>::copyFrom() "copyFrom()", \ref tgx::Image<color_t>::copyReduceHalf() "copyReduceHalf()",  \ref tgx::Image<color_t>::reduceHalf() "reduceHalf()" and \ref tgx::Image<color_t>::convert() "convert()"

---

@subsection subsec_vhlines drawing horizontal and vertical lines

![test_vhlines](../test_vhlines.png)

*Code used to generate the image:*
~~~{.cpp}
tgx::RGB32 buffer[320*240];					  //
tgx::Image<tgx::RGB32> im(buffer, 320, 240);  // create a black 320x240 image. 
im.clear(tgx::RGB32_Black);					  //

for (int i = 0; i < 20; i++) {
    im.drawFastVLine({ i * 16, i * 3 }, 100, tgx::RGB32_Red, i / 20.0f);	// draw a vertical line
    im.drawFastHLine({ i * 3, i * 12 }, 200, tgx::RGB32_Green, i / 20.0f);	// draw an horizontal line
    }
~~~
**Methods:** use these methods when drawing parallel lines as they are faster that the 'general' line drawing methods.
- \ref tgx::Image::drawFastVLine "drawFastVLine()" : draw a vertical line segment.
- \ref tgx::Image::drawFastHLine "drawFastHLine()" : draw an horizontal line segment.

    
	

---

@subsection subsec_lines drawing lines

![test_lines](../test_lines.png)

*Code used to generate the image:*
~~~{.cpp}
tgx::RGB32 buffer[320*240];					  //
tgx::Image<tgx::RGB32> im(buffer, 320, 240);  // create a black 320x240 image. 
im.clear(tgx::RGB32_Black);					  //

im.drawLine({ 10,10 }, { 300, 60 }, tgx::RGB32_Red);       // simple red line
im.drawLineAA({ 200,200 }, { 300, 10 }, tgx::RGB32_Green); // green line with anti-aliasing
im.drawThickLineAA({ 10, 120 }, { 250, 10 }, 10, tgx::END_STRAIGHT, tgx::END_ROUNDED, tgx::RGB32_White); // thick line, rounded at the start, straight at the end.
im.drawWedgeLineAA({ 300, 220 }, { 150, 100 }, 10, tgx::END_ARROW_1, 20, tgx::END_ARROW_SKEWED_5,tgx::RGB32_Orange, 0.5f); // wedge line ending with arrows, half opacity.
~~~
**Methods:**
- \ref tgx::Image::drawLine "drawLine()" : draw a basic straight line (single pixel thick, no anti-aliasing). Fastest method. 
- \ref tgx::Image::drawLineAA "drawLineAA()" : draw a line segment with anti-aliasing (single pixel thick). 
- \ref tgx::Image::drawThickLineAA "drawThickLineAA()" : draw a thick line with anti-aliasing and specify how the ends look like (rounded, flat, arrows...)
- \ref tgx::Image::drawWedgeLineAA "drawWedgeLineAA()" : draw a line segment with anti-aliasing with vraying thickness, specify how the ends look like (rounded, flat, arrows...)




---

@subsection subsec_rect drawing rectangles

![test_rectangles](../test_rectangles.png)

*Code used to generate the image:*
~~~{.cpp}
tgx::RGB32 buffer[320*240];					  //
tgx::Image<tgx::RGB32> im(buffer, 320, 240);  // create a black 320x240 image. 
im.clear(tgx::RGB32_Black);					  //

im.drawRect({ 10,70,10,70 }, tgx::RGB32_White); // simple green square, full opacity
im.drawThickRect({ 30,180,30,140 }, 7,  tgx::RGB32_Orange, 0.5f); // 7 pixels thick orange rectangle, half opacity
im.fillRect({5, 80, 20, 200}, tgx::RGB32_Yellow, 0.25f); // yellow filled rectangle, 25% opacity
im.fillThickRect({ 50, 150, 150, 220 }, 5, tgx::RGB32_Gray, tgx::RGB32_Red, 0.5f); // gray filled, 5 pixels thick red rectangle, 50% opacity
im.fillRectHGradient({ 140, 310, 100, 160 }, tgx::RGB32_Red, tgx::RGB32_Blue); // rectangle filled with horizontal gradient from red to blue. full opacity.
im.fillRectVGradient({ 190, 300, 10, 230 }, tgx::RGB32_Cyan, tgx::RGB32_Purple, 0.5f); // rectangle filled with vertical gradient from cyan to purple. half opacity.
~~~
**Methods:**
- \ref tgx::Image::drawRect "drawRect()" : draw a basic rectangle. Fastest method. 
- \ref tgx::Image::drawThickRect "drawThickRect()" : draw a rectangle with a thick outline. 
- \ref tgx::Image::fillRect "fillRect()" : draw a filled rectangle.
- \ref tgx::Image::fillThickRect "fillThickRect()" : draw a filled rectangle with a thick outline of a different color.
- \ref tgx::Image::fillRectHGradient "fillRectHGradient()" : draw a filled rectangle with an horizontal gradient between two colors.
- \ref tgx::Image::fillRectVGradient "fillRectVGradient()" : draw a filled rectangle with a vertical gradient between two colors.

See also \ref tgx::Image::drawThickRectAA "drawThickRectAA()", \ref tgx::Image::fillRectAA "drawThickRectAA()", \ref tgx::Image::fillThickRectAA "drawThickRectAA()" (useful for smooth animation but not for static display). 




---

@subsection subsec_roundrect drawing rounded rectangles

![test_rounded_rectangles](../test_rounded_rectangles.png)

*Code used to generate the image:*
~~~{.cpp}
tgx::RGB32 buffer[320*240];					  //
tgx::Image<tgx::RGB32> im(buffer, 320, 240);  // create a black 320x240 image. 
im.clear(tgx::RGB32_Black);					  //

im.drawRoundRectAA({ 10,70,10,70}, 4, tgx::RGB32_White); // simple 4-pixels rounded square
im.drawThickRoundRectAA({ 30,180,30,140 }, 20, 7, tgx::RGB32_Orange, 0.5f); // 7 pixels thick, 20 pixels rounded corner rectangle, half opacity
im.fillRoundRectAA({ 140, 310, 170, 230 }, 25, tgx::RGB32_Blue); // filled 25-pixel rounded corner rectangle
im.fillThickRoundRectAA({ 100, 250, 60, 200 }, 10, 5, tgx::RGB32_Gray, tgx::RGB32_Red, 0.5f); // 5 pixels thick, 10 pixel rounded corner rectangle, 50% opacity
~~~
**Methods:**
- \ref tgx::Image::drawRoundRectAA "drawRoundRectAA()" : draw a single pixel thick rectangle with rounded corners, with antiliasing.
- \ref tgx::Image::drawThickRoundRectAA "drawThickRoundRectAA()" : draw a rounded corners rectangle with thick outline, with antiliasing.
- \ref tgx::Image::fillRoundRectAA "fillRoundRectAA()" : draw a filled rectangle with rounded corners, with anti-aliasing. 
- \ref tgx::Image::fillThickRoundRectAA "fillThickRoundRectAA()" : draw a filled rectangle  with rounded corners and thick outline of a different color, with anti-aliasing.



---

@subsection subsec_triangles drawing triangles

![test_triangles](../test_triangles.png)

*Code used to generate the image:*
~~~{.cpp}
tgx::RGB32 buffer[320*240];					  //
tgx::Image<tgx::RGB32> im(buffer, 320, 240);  // create a black 320x240 image. 
im.clear(tgx::RGB32_Black);					  //

im.drawTriangle({ 10,10 }, { 60, 30 }, {40, 80}, tgx::RGB32_White); // simple triangle
im.fillTriangle({150, 5}, {80, 180}, {300, 20} , tgx::RGB32_Purple, tgx::RGB32_Magenta); // filled triangle with border
im.drawTriangleAA({ 20,120 }, { 150, 100 }, { 70, 239}, tgx::RGB32_Green); // green triangle with AA
im.drawThickTriangleAA({ 160, 60 }, {240,30}, {300, 80}, 10, tgx::RGB32_Cyan, 0.5f); // 10 pixels thick triangle, 50% opacity, with AA
im.fillTriangleAA({ 200, 220 }, { 315,210 }, { 150, 120 }, tgx::RGB32_Yellow); // filled triangle with AA.
im.fillThickTriangleAA({ 130,200 }, { 280, 110 }, {250, 170}, 5, tgx::RGB32_Lime, tgx::RGB32_Orange, 0.5f); // filled 5 pixels thick triangle, 50% opacity, with AA
~~~
**Methods:**
- \ref tgx::Image::drawTriangle "drawTriangle()" : draw a simple triangle. Fastest method.
- \ref tgx::Image::fillTriangle "fillTriangle()" : draw a simple filled triangle. Fastest method
- \ref tgx::Image::drawTriangleAA "drawTriangleAA()" : draw a triangle with single pixel thick outline, with anti-aliasing.
- \ref tgx::Image::drawThickTriangleAA "drawThickTriangleAA()" :  draw a triangle with thick outline, with anti-aliasing.
- \ref tgx::Image::fillTriangleAA "fillTriangleAA()" : draw a filled triangle  with anti-aliasing.
- \ref tgx::Image::fillThickTriangleAA "fillThickTriangleAA()" : draw a filled triangle  with a thick outline of a different color, with anti-aliasing.




---

@subsection subsec_triangles_advanced drawing triangles (advanced)

![test_triangles_advanced](../test_triangles_advanced.png)

*Code used to generate the image:*
~~~{.cpp}
tgx::RGB32 buffer[320*240];					  //
tgx::Image<tgx::RGB32> im(buffer, 320, 240);  // create a black 320x240 image. 
im.clear(tgx::RGB32_Black);					  //

tgx::RGB32 buf[50 * 50]; // memory for texture image
tgx::Image<tgx::RGB32> tex(buf, 50, 50); // the texture image...
tex.clear(tgx::RGB32_Blue); // ... has blue background ..
tex.fillThickCircleAA({ 25,25 }, 24, 5, tgx::RGB32_Red, tgx::RGB32_Green); // ...and red-filled circle with green outline in the center. 

im.drawTexturedTriangle(tex, { 0,0 }, { 50, 0 }, { 50, 50 }, { 10, 10 }, { 100, 20 }, { 30, 100 }); // textured triangle, full opacity
im.drawGradientTriangle({ 20, 140 }, { 160, 50 }, {230, 200}, tgx::RGB32_Blue, tgx::RGB32_Orange, tgx::RGB32_Black); // gradient triangle, full opacity. 
im.drawTexturedGradientTriangle(tex, { 0,0 }, { 50, 0 }, { 50, 50 } , { 120, 230 }, { 300, 20 }, { 280, 170 }, tgx::RGB32_Red, tgx::RGB32_Green, tgx::RGB32_Blue, 0.5f); // texture triangle with color gradient. half opacity.
~~~
**Methods:** these methods make use of the 3D rasterizer:
- \ref tgx::Image::drawTexturedTriangle "drawTexturedTriangle()" : draw a filled triangle with texture mapping.
- \ref tgx::Image::drawGradientTriangle "drawGradientTriangle()" : draw a filled triangle with gradient color (i.e. Gouraud shading)
- \ref tgx::Image::drawTexturedGradientTriangle "drawTexturedGradientTriangle()" : draw a filled trinagle with both texture mapping and gradient color. 

See also \ref tgx::Image::drawTexturedMaskedTriangle "drawTexturedMaskedTriangle()", \ref tgx::Image::drawTexturedGradientMaskedTriangle "drawTexturedGradientMaskedTriangle()" for additonal masking features. 




---

@subsection subsec_quads drawing quads

![test_quads](../test_quads.png)

*Code used to generate the image:*
~~~{.cpp}
tgx::RGB32 buffer[320*240];					  //
tgx::Image<tgx::RGB32> im(buffer, 320, 240);  // create a black 320x240 image. 
im.clear(tgx::RGB32_Black);					  //

im.drawQuad({ 10,10 }, { 60, 30 }, { 40, 80 }, {5, 40}, tgx::RGB32_White); // simple quad
im.fillQuad({ 150, 5 }, { 80, 180 }, {170, 40}, { 300, 20 }, tgx::RGB32_Purple); // filled quad
im.drawQuadAA({ 20,120 }, { 150, 100 }, { 150, 140 },  { 70, 239 }, tgx::RGB32_Green); // green quad with AA
im.drawThickQuadAA({ 160, 60 }, { 240,30 }, { 300, 80 }, {230, 120}, 10, tgx::RGB32_Cyan, 0.5f); // 10 pixels thick quad, 50% opacity, with AA
im.fillQuadAA({ 200, 220 }, { 315,210 }, {250, 150}, { 150, 120 }, tgx::RGB32_Yellow); // filled quad with AA
im.fillThickQuadAA({ 130,200 }, { 160, 140 }, { 230, 100 }, { 250, 170 } , 5, tgx::RGB32_Lime, tgx::RGB32_Orange, 0.5f); //filled, 5 pixels thick quad, 50% opacity, with AA
~~~
**Methods:**
- \ref tgx::Image::drawQuad "drawQuad()" : draw a simple quad. Fastest method.
- \ref tgx::Image::fillQuad "fillQuad()" : draw a simple filled quad. Fastest method
- \ref tgx::Image::drawQuadAA "drawQuadAA()" : draw a quad with single pixel thick outline, with anti-aliasing.
- \ref tgx::Image::drawThickQuadAA "drawThickQuadAA()" :  draw a quad with thick outline, with anti-aliasing.
- \ref tgx::Image::fillQuadAA "fillQuadAA()" : draw a filled quad  with anti-aliasing.
- \ref tgx::Image::fillThickQuadAA "fillThickQuadAA()" : draw a filled quad with a thick outline of a different color, with anti-aliasing.




---

@subsection subsec_quads_advanced drawing quads (advanced)

![test_quad_advanced](../test_quad_advanced.png)

*Code used to generate the image:*
~~~{.cpp}
tgx::RGB32 buffer[320*240];					  //
tgx::Image<tgx::RGB32> im(buffer, 320, 240);  // create a black 320x240 image. 
im.clear(tgx::RGB32_Black);					  //

im.drawTexturedQuad(tex, { 0,0 }, { 50, 0 }, { 50, 50 }, { 0, 50 }, { 10, 10 }, { 100, 20 }, { 80, 40 }, {30, 120}); // textured quad, 
im.drawGradientQuad({ 20, 140 }, { 160, 50 }, {180, 55}, { 230, 200 }, tgx::RGB32_Blue, tgx::RGB32_Orange, tgx::RGB32_Green, tgx::RGB32_Black); // gradient quad
im.drawTexturedGradientQuad(tex, { 0,0 }, { 50, 0 }, { 50, 50 }, { 0, 50 }, { 120, 230 }, { 280, 20 }, { 310, 170 }, {250,210}, tgx::RGB32_Red, tgx::RGB32_Green, tgx::RGB32_Blue, tgx::RGB32_Navy,0.5f); // texture quad with color gradient. half opacity.
~~~
**Methods:** these methods make use of the 3D rasterizer:
- \ref tgx::Image::drawTexturedQuad "drawTexturedQuad()" : draw a filled quad with texture mapping.
- \ref tgx::Image::drawGradientQuad "drawGradientQuad()" : draw a filled quad with gradient color (i.e. Gouraud shading)
- \ref tgx::Image::drawTexturedGradientQuad "drawTexturedGradientQuad()" : draw a filled quad with both texture mapping and gradient color. 

See also \ref tgx::Image::drawTexturedMaskedQuad "drawTexturedMaskedQuad()", \ref tgx::Image::drawTexturedGradientMaskedQuad "drawTexturedGradientMaskedQuad()" for additonal masking features. 



---

@subsection subsec_polylines drawing polylines

![test_polylines](../test_polylines.png)
    
*Code used to generate the image:*
~~~{.cpp}
tgx::RGB32 buffer[320*240];					  //
tgx::Image<tgx::RGB32> im(buffer, 320, 240);  // create a black 320x240 image. 
im.clear(tgx::RGB32_Black);					  //

const tgx::iVec2 tabPoints1[] = { {10, 10}, {50, 10}, {80, 50}, {20, 110}, {20, 180}, {300, 150} };
im.drawPolyline(6, tabPoints1, tgx::RGB32_White); // simple polyline

const tgx::fVec2 tabPoints2[] = { {200, 100}, {100, 20}, {130, 120}, {10, 200}, {60, 180}, {300, 230} };
im.drawPolylineAA(6, tabPoints2, tgx::RGB32_Red); // polyline with AA

const tgx::fVec2 tabPoints3[] = { {30, 30}, {200, 15}, {300, 200}, {140, 230}, {80, 150}, {20, 230} };
im.drawThickPolylineAA(6, tabPoints3, 12, tgx::END_ARROW_1, tgx::END_ROUNDED, tgx::RGB32_Orange, 0.5f); // thick polyline, arrow at start, rounded at end, half opacity, with AA
~~~
**Methods:**
- \ref tgx::Image::drawPolyline "drawPolyline()" : draw a simple polyline. Fastest method.
- \ref tgx::Image::drawPolylineAA "drawPolylineAA()" : draw a polyline, with anti-aliasing.
- \ref tgx::Image::drawThickPolylineAA "drawThickPolylineAA()" : draw a thick polyline with anti-aliasing and specify how the ending look like. 

	

---

@subsection subsec_polygons drawing polygons
	
![test_polygons](../test_polygons.png)

*Code used to generate the image:*
~~~{.cpp}
tgx::RGB32 buffer[320*240];					  //
tgx::Image<tgx::RGB32> im(buffer, 320, 240);  // create a black 320x240 image. 
im.clear(tgx::RGB32_Black);					  //

tgx::iVec2 tabPoints1[] = { {10, 10}, {50, 10}, {80, 40}, {50, 90}, {10, 70} };
im.drawPolygon(5, tabPoints1, tgx::RGB32_White); // simple polygon

tgx::iVec2 tabPoints2[] = { {90, 20}, {50, 40}, {60, 80}, {150, 90}, {190, 45} };
im.fillPolygon(5, tabPoints2, tgx::RGB32_Yellow, 0.5f); // filled polygon, half opacity.

tgx::fVec2 tabPoints3[] = { {140, 60}, {250, 10}, {310, 100}, {300, 150}, {240, 170}, {220, 120} };
im.drawPolygonAA(6, tabPoints3, tgx::RGB32_Red); // polygon with AA

tgx::fVec2 tabPoints4[] = { {10, 120}, {50, 80}, {100, 150}, {40, 200}, {50, 170} };
im.drawThickPolygonAA(5, tabPoints4, 6, tgx::RGB32_Cyan, 0.5f); // thick polygon, 50% opacity, with AA

tgx::fVec2 tabPoints5[] = { {50, 120}, {130, 70}, {170, 100}, {210, 180}, {150, 190} };
im.fillPolygonAA(5, tabPoints5, tgx::RGB32_Lime, 0.5f); // filled polygon, 50% opacity, with AA

tgx::fVec2 tabPoints6[] = { {120, 150}, {210, 50}, {250, 70}, {300, 100}, {290, 170}, {230, 210} };
im.fillThickPolygonAA(6, tabPoints6, 10, tgx::RGB32_Purple, tgx::RGB32_Orange, 0.5f); // filled thick polygon, 50% opacity, with AA
~~~
**Methods:**
- \ref tgx::Image::drawPolygon "drawPolygon()" : draw a simple polygon. Fastest method.
- \ref tgx::Image::fillPolygon "fillPolygon()" : draw a simple filled polygon. Fastest method.
- \ref tgx::Image::drawPolygonAA "drawPolygonAA()" : draw a polygon with anti-aliasing.
- \ref tgx::Image::drawThickPolygonAA "drawThickPolygonAA()" : draw a polygon with thick outline, with anti-aliasing.
- \ref tgx::Image::fillPolygonAA "fillPolygonAA()" : draw a filled polygon, with anti-aliasing.
- \ref tgx::Image::fillThickPolygonAA "fillThickPolygonAA()" : draw a filled polygon with thick outline, with anti-aliasing.




---

@subsection subsec_circles drawing circles

![test_circles](../test_circles.png)

*Code used to generate the image:*
~~~{.cpp}
tgx::RGB32 buffer[320*240];					  //
tgx::Image<tgx::RGB32> im(buffer, 320, 240);  // create a black 320x240 image. 
im.clear(tgx::RGB32_Black);					  //

im.drawCircle({50, 50}, 48, tgx::RGB32_White); // simple circle
im.fillCircle({ 120, 80 }, 70, tgx::RGB32_Blue, tgx::RGB32_Red, 0.5f); // filled circle, 50% opacity
im.drawCircleAA({210, 90}, 60, tgx::RGB32_Green); // green circle with AA
im.drawThickCircleAA({60, 180}, 40, 10, tgx::RGB32_Purple); //thick maroon circle, with AA
im.fillCircleAA({230, 150}, 70, tgx::RGB32_Yellow, 0.5f); // filled circle with AA, 50% opacity
im.fillThickCircleAA({160,160}, 55, 8, tgx::RGB32_Teal, tgx::RGB32_Orange, 0.5f); // filled circle with thick outline, with AA, 50% opacity 
~~~
**Methods:**
- \ref tgx::Image::drawCircle "drawCircle()" : draw a simple circle. Fastest method.
- \ref tgx::Image::fillCircle "fillCircle()" : draw a simple filled circle. Fastest method.
- \ref tgx::Image::drawCircleAA "drawCircleAA()" : draw a circle with anti-aliasing.
- \ref tgx::Image::drawThickCircleAA "drawThickCircleAA()" : draw a circle with thick outline, with anti-aliasing.
- \ref tgx::Image::fillCircleAA "fillCircleAA()" : draw a filled circle, with anti-aliasing.
- \ref tgx::Image::fillThickCircleAA "fillThickCircleAA()" : draw a filled circle with thick outline, with anti-aliasing.



---

@subsection subsec_ellipses drawing ellipses

![test_ellipses](../test_ellipses.png)

*Code used to generate the image:*
~~~{.cpp}
tgx::RGB32 buffer[320*240];					  //
tgx::Image<tgx::RGB32> im(buffer, 320, 240);  // create a black 320x240 image. 
im.clear(tgx::RGB32_Black);					  //

im.drawEllipse({ 50, 50 }, { 48, 20 }, tgx::RGB32_White); // simple ellipse
im.fillEllipse({ 120, 80 }, { 70, 50 }, tgx::RGB32_Blue, tgx::RGB32_Red, 0.5f); // filled ellipse, 50% opacity
im.drawEllipseAA({ 210, 90 }, { 60, 30 }, tgx::RGB32_Green); // ellipse with AA
im.drawThickEllipseAA({ 60, 180 }, { 35, 55 }, 10, tgx::RGB32_Purple); // thick ellipse, with AA
im.fillEllipseAA({ 230, 150 }, { 45, 70 }, tgx::RGB32_Yellow, 0.5f); // filled ellipse with AA, 50% opacity
im.fillThickEllipseAA({ 160,160 }, { 55, 65 }, 8, tgx::RGB32_Teal, tgx::RGB32_Orange, 0.5f); // filled ellipse with thick outline, with AA, 50% opacity 
~~~
**Methods:**
- \ref tgx::Image::drawEllipse "drawEllipse()" : draw a simple ellipse. Fastest method.
- \ref tgx::Image::fillEllipse "fillEllipse()" : draw a simple filled ellipse. Fastest method.
- \ref tgx::Image::drawEllipseAA "drawEllipseAA()" : draw an ellipse with anti-aliasing.
- \ref tgx::Image::drawThickEllipseAA "drawThickEllipseAA()" : draw an ellipse with thick outline, with anti-aliasing.
- \ref tgx::Image::fillEllipseAA "fillEllipseAA()" : draw a filled ellipse, with anti-aliasing.
- \ref tgx::Image::fillThickEllipseAA "fillThickEllipseAA()" : draw a filled ellipse with thick outline, with anti-aliasing.




---

@subsection subsec_arcpies drawing circle arcs and pies

![test_arc_pies](../test_arc_pies.png)

*Code used to generate the image:*
~~~{.cpp}
tgx::RGB32 buffer[320*240];					  //
tgx::Image<tgx::RGB32> im(buffer, 320, 240);  // create a black 320x240 image. 
im.clear(tgx::RGB32_Black);					  //

im.fillCircleSectorAA({ 60,60 }, 40, 50, 270, tgx::RGB32_Red); // filled circle sector between 50 and 270 degrees
im.drawCircleArcAA({250, 60}, 50, 250, 90, tgx::RGB32_Green); // green circle arc from 250 to 90 degrees
im.drawThickCircleArcAA({80, 160}, 75, 220, 160, 20, tgx::RGB32_Purple, 0.5f); // thick circle arc from 220 to 160 degrees, 50% opacity
im.fillThickCircleSectorAA({230, 120}, 100, 140, 340, 10, tgx::RGB32_Olive, tgx::RGB32_Orange, 0.5f); // filled thick circle sector from 140 to 340 degrees, 50% opacity
~~~
**Methods:**
- \ref tgx::Image::fillCircleSectorAA "fillCircleSectorAA()" : draw a circle sector (pie), with AA.
- \ref tgx::Image::drawCircleArcAA "drawCircleArcAA()" : draw a circle arc, single pixel thickness, with AA
- \ref tgx::Image::drawThickCircleArcAA "drawThickCircleArcAA()" : draw a thick circle arc, with AA
- \ref tgx::Image::fillThickCircleSectorAA "fillThickCircleSectorAA()" : draw a circle sector (pie) with thick outline (circle arc), with AA. 




---

@subsection subsec_bezier drawing Bezier curves

![test_bezier_curves](../test_bezier_curves.png)


*Code used to generate the image:*
~~~{.cpp}
tgx::RGB32 buffer[320*240];					  //
tgx::Image<tgx::RGB32> im(buffer, 320, 240);  // create a black 320x240 image. 
im.clear(tgx::RGB32_Black);					  //

im.drawQuadBezier({ 10, 10 }, { 250, 180 }, {280, 20}, 1.0f, true, tgx::RGB32_White); // simple quadratic bezier curve
im.drawCubicBezier({ 10, 40 }, { 280, 50 }, { 20, 80 }, { 300, 200 }, true, tgx::RGB32_Green); // simple cubic bezier curve
im.drawThickQuadBezierAA({ 30, 150 }, { 300, 20 }, {0,0}, 2.0f, 10, tgx::END_STRAIGHT, tgx::END_ROUNDED, tgx::RGB32_Red, 0.5f); // thick quadratic bezier curve, with AA, 50% opacity.
im.drawThickCubicBezierAA({ 80, 80 }, { 305, 150 }, { 0, 240 }, {290, 240}, 10, tgx::END_ARROW_5, tgx::END_ARROW_SKEWED_2, tgx::RGB32_Orange, 0.5f); // thick cubic bezier curve, with AA, 50% opacity.
~~~
**Methods:**
- \ref tgx::Image::drawQuadBezier "drawQuadBezier()" : draw a quadratic Bezier curve.
- \ref tgx::Image::drawCubicBezier "drawCubicBezier()" : draw a cubic Bezier curve.
- \ref tgx::Image::drawThickQuadBezierAA "drawThickQuadBezierAA()" : draw a thick quadratic Bezier curve, with AA
- \ref tgx::Image::drawThickCubicBezierAA "drawThickCubicBezierAA()" : draw a thick cubic Bezier curve, with AA. 


---

@subsection subsec_splines drawing splines

![test_splines](../test_splines.png)


*Code used to generate the image:*
~~~{.cpp}
tgx::RGB32 buffer[320*240];					  //
tgx::Image<tgx::RGB32> im(buffer, 320, 240);  // create a black 320x240 image. 
im.clear(tgx::RGB32_Black);					  //

const tgx::iVec2 tabPoints1[] = { {30, 10}, {10, 40}, {40, 70}, {10, 100}, {60, 130}, {10, 160}, {80, 190}, {10, 220} };
im.drawQuadSpline(8, tabPoints1, true, tgx::RGB32_White); // simple quadratic spline
    
const tgx::iVec2 tabPoints2[] = { {10, 10}, {50, 40}, {10, 70}, {70, 100}, {10, 130}, {90, 160}, {10, 190}, {120, 220} };
im.drawCubicSpline(8, tabPoints2, true, tgx::RGB32_Red); // simple cubic spline

const tgx::fVec2 tabPoints3[] = { {70, 10}, {40, 40}, {80, 70}, {30, 100}, {120, 130}, {60, 160}, {150, 190}, {80, 220} };
im.drawThickQuadSplineAA(8, tabPoints3, 5, tgx::END_ROUNDED, tgx::END_ROUNDED, tgx::RGB32_Yellow, 0.5f); // thick quadratic spline, 50% opacity, with AA

const tgx::fVec2 tabPoints4[] = { {60, 20}, {120, 40}, {50, 70}, {130, 100}, {60, 130}, {110, 160}, {70, 190}, {60, 220} };
im.drawThickCubicSplineAA(8, tabPoints4, 6, tgx::END_ARROW_3, tgx::END_ARROW_SKEWED_3, tgx::RGB32_Green, 0.5f); // thick cubic spline, 50% opacity, with AA

const tgx::fVec2 tabPoints5[] = { {140, 20}, {160, 10}, {200, 50}, {250, 20}, {300, 100}, {250, 160}, {200, 100}, {160, 150} };
im.drawThickClosedSplineAA(8, tabPoints5, 12, tgx::RGB32_Blue); // thick closed spline, with AA

const tgx::iVec2 tabPoints6[] = { {240, 10}, {310, 50}, {200, 130} };
im.drawClosedSpline(3, tabPoints6, tgx::RGB32_Magenta, 0.5f); // simple closed spline, 50% opacity

const tgx::fVec2 tabPoints7[] = { {300, 220}, {240, 220}, {240, 180}, {300, 190} };
im.fillClosedSplineAA(4, tabPoints7, tgx::RGB32_Cyan); // interior filled closed spline, with AA

const tgx::fVec2 tabPoints8[] = { {150, 210}, {160, 150}, {250, 120}, {280, 150}, {260, 190} };
im.fillThickClosedSplineAA(5, tabPoints8, 5, tgx::RGB32_Gray ,tgx::RGB32_Red, 0.5f); // interior filled thick closed spline, 50% opacity, with AA
~~~
**Methods:**
- \ref tgx::Image::drawQuadSpline "drawQuadSpline()" : draw a simple quadratic spline
- \ref tgx::Image::drawCubicSpline "drawCubicSpline()" : draw a simple cubic spline
- \ref tgx::Image::drawThickQuadSplineAA "drawThickQuadSplineAA()" : draw a thick quadratic spline, with AA
- \ref tgx::Image::drawThickCubicSplineAA "drawThickCubicSplineAA()" : draw a thick cubic spline, with AA
- \ref tgx::Image::drawThickClosedSplineAA "drawThickClosedSplineAA()" : draw a thick closed spline, with AA
- \ref tgx::Image::drawClosedSpline "drawClosedSpline()" : draw a simple thick closed spline
- \ref tgx::Image::fillClosedSplineAA "fillClosedSplineAA()" : draw a closed spline and fill its interior, with AA
- \ref tgx::Image::fillThickClosedSplineAA "fillThickClosedSplineAA()" : draw a  thick closed spline and fill its interior, with AA


---


@subsection subsec_text drawing text

![test_text](../test_text.png)

*additional includes for the text fonts*
~~~{.cpp}
#include "font_tgx_Arial.h"
#include "font_tgx_OpenSans.h"
~~~

*Code used to generate the image:*
~~~{.cpp}
tgx::RGB32 buffer[320*240];					  //
tgx::Image<tgx::RGB32> im(buffer, 320, 240);  // create a black 320x240 image. 
im.clear(tgx::RGB32_Black);					  //

// drawing text: the anchor point use the baseline. 
im.drawText("non-AA font are ugly...", { 5, 10 },font_tgx_Arial_8, tgx::RGB32_Blue); // a non AA font
im.drawText("AA font, 9pt", { 5, 25 }, font_tgx_OpenSans_9, tgx::RGB32_Green);     // AA font, 9pt
im.drawText("AA font, 10pt", { 5, 40 }, font_tgx_OpenSans_10, tgx::RGB32_Green);   // AA font, 10pt
im.drawText("AA font, 14pt", { 5, 60 }, font_tgx_OpenSans_14, tgx::RGB32_Green);   // AA font, 14pt
im.drawText("AA font, 18pt", { 5, 83 }, font_tgx_OpenSans_20, tgx::RGB32_Green);   // AA font, 18pt

// drawing on the baseline
im.drawFastHLine({ 130, 30 }, 160, tgx::RGB32_White, 0.5f);
im.drawFastHLine({ 130, 45 }, 160, tgx::RGB32_White, 0.5f);
im.drawText("The quick brown fox jumps", { 130, 30 }, font_tgx_OpenSans_12, tgx::RGB32_Orange);
im.drawText("over the lazy dog !", { 130, 45 }, font_tgx_OpenSans_12, tgx::RGB32_Orange);

// finding the bounding box of a text. 
const char * tbb = "Text bounding box";
tgx::iBox2 bb = im.measureText(tbb, { 230, 70 }, font_tgx_OpenSans_14, tgx::Anchor::CENTER, false, false);
im.fillRect(bb, tgx::RGB32_Yellow, 0.5f);
im.drawTextEx(tbb, { 230, 70 }, font_tgx_OpenSans_14, tgx::Anchor::CENTER, false, false, tgx::RGB32_Blue);


// draw text with different anchor points using drawTextEx()
// -> the anchor is relative to the text bounding box
im.drawRect({ 20, 300,100, 230 }, tgx::RGB32_Gray);
im.drawFastHLine({ 20, 165 }, 280, tgx::RGB32_Gray);
im.drawFastVLine({ 160, 100 }, 130, tgx::RGB32_Gray);
tgx::iVec2 anchor_point;

anchor_point = {20, 100 };
im.drawTextEx("TOPLEFT", anchor_point, font_tgx_OpenSans_10, tgx::Anchor::TOPLEFT, true, true, tgx::RGB32_Red);
im.fillCircleAA(anchor_point,1.5, tgx::RGB32_Green);

anchor_point = { 300, 100 };
im.drawTextEx("TOPRIGHT", anchor_point, font_tgx_OpenSans_10, tgx::Anchor::TOPRIGHT, true, true, tgx::RGB32_Red);
im.fillCircleAA(anchor_point, 1.5, tgx::RGB32_Green);

anchor_point = { 20, 230 };
im.drawTextEx("BOTTOMLEFT", anchor_point, font_tgx_OpenSans_10, tgx::Anchor::BOTTOMLEFT, true, true, tgx::RGB32_Red);
im.fillCircleAA(anchor_point, 1.5, tgx::RGB32_Green);

anchor_point = { 300, 230 };
im.drawTextEx("BOTTOMRIGHT", anchor_point, font_tgx_OpenSans_10, tgx::Anchor::BOTTOMRIGHT, true, true, tgx::RGB32_Red);
im.fillCircleAA(anchor_point, 1.5, tgx::RGB32_Green);

anchor_point = { 160, 100 };
im.drawTextEx("CENTERTOP", anchor_point, font_tgx_OpenSans_10, tgx::Anchor::CENTERTOP, true, true, tgx::RGB32_Red);
im.fillCircleAA(anchor_point, 1.5, tgx::RGB32_Green);

anchor_point = { 160, 230 };
im.drawTextEx("CENTERBOTTOM", anchor_point, font_tgx_OpenSans_10, tgx::Anchor::CENTERBOTTOM, true, true, tgx::RGB32_Red);
im.fillCircleAA(anchor_point, 1.5, tgx::RGB32_Green);

anchor_point = { 20, 165 };
im.drawTextEx("CENTERLEFT", anchor_point, font_tgx_OpenSans_10, tgx::Anchor::CENTERLEFT, true, true, tgx::RGB32_Red);
im.fillCircleAA(anchor_point, 1.5, tgx::RGB32_Green);

anchor_point = { 300, 165 };
im.drawTextEx("CENTERRIGHT", anchor_point, font_tgx_OpenSans_10, tgx::Anchor::CENTERRIGHT, true, true, tgx::RGB32_Red);
im.fillCircleAA(anchor_point, 1.5, tgx::RGB32_Green);

anchor_point = { 160, 165 };
im.drawTextEx("CENTER", anchor_point, font_tgx_OpenSans_10, tgx::Anchor::CENTER, true, true, tgx::RGB32_Red);
im.fillCircleAA(anchor_point, 1.5, tgx::RGB32_Green);
~~~

The TGX library support font using:
- AdafruitGFX format:                   (https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts)
- ILI9341_t3 v1 format                  (https://forum.pjrc.com/threads/54316-ILI9341_t-font-structure-format)
- ILI9341_t3 v23 format (antialiased)   (https://github.com/projectitis/packedbdf/blob/master/packedbdf.md)

@note tgx-font (https://github.com/vindar/tgx-font) contains a collection ILI9341_t3 v1 and v2 (antialiased) fonts 
that can be used directly with the methods below (and the instruction on how to convert a ttf font to  this format).

The following fonts are already packaged with TGX but must be included explicitely in the project when used:
- *Arial* (non AA font): `#include "font_tgx_Arial.h"` 
- *Arial Bold* (non AA font): `#include "font_tgx_Arial_Bold.h"`
- *OpenSans* (AA font): `#include "font_tgx_OpenSans.h"`
- *OpenSans Bold* (AA font): `#include "font_tgx_OpenSans_Bold.h"`
- *OpenSans Italic* (AA font): `#include "font_tgx_OpenSans_Italic.h"`

Each font above is avaailble in fontsize: 8pt, 9pt, 10pt, 11pt, 12pt, 13pt, 14pt, 16pt, 18pt, 20pt, 24pt, 28pt, 32pt, 40pt, 48pt, 60pt, 72pt, 96pt.


**Methods:**
- \ref tgx::Image::drawText "drawText()" : draw a text using a given font. simple method
- \ref tgx::Image::drawTextEx "drawTextEx()" : draw a text using a given font. Extended method with anchor placement (c.f. \ref tgx::Anchor).
- \ref tgx::Image::measureText "measureText()" : compute the bounding box of a text (fast, does not draw it)
- \ref tgx::Image::measureChar "measureChar()" : compute the bounding box of a character (fast, does not draw it)


---


@section sec_extensions TGX extensions via external librairies

TGX implements bindings that makes it very easy to use external libraries to add new capabilities to the Image class, such as displaying classic image format (PNG, GIF, JPEG) and TrueType fonts... 

@subsection subsec_openfontrender drawing text with TrueType fonts 

Install Takkao's **OpenRenderFont** library from  https://github.com/takkaO/OpenFontRender/.

We just need to include both libraries into the project to activate the bindings between TGX and OpenFontRender:
~~~{.cpp}
#include <tgx.h>
#include <OpenFontRender.h>
~~~
Now, we can bind any `OpenFontRender` object to an image with the method: \ref tgx::Image<color_t>::setOpenFontRender() "Image::setOpenFontRender(ofr)" and subsequently use the OpenFontRender library as usual. All texts written with `ofr` will automatically be drawn onto the selected image. 

Example:
~~~{.cpp}
// create a 100x50 image in RGB565 color format. 
tgx::RGB565 buf[100*50];
tgx::Image<tgx::RGB565> im(buf, 100, 50);
im.clear(tgx::RGB565_Black); // clear to full black

// create a font rendering object
OpenFontRender ofr; 

// we link it wih image im
im.setOpenFontRender(ofr); 

// set up the font rendering object properties before drawing
ofr.loadFont(NotoSans_Bold, sizeof(NotoSans_Bold))  // assume here that NotoSans_Bold is a Truetype font in memory. 
ofr.setFontColor((uint16_t)tgx::RGB565_Yellow, (uint16_t)tgx::RGB565_Black); // set foreground and background colors for drawing text
ofr.setCursor(10, 20); // Set the cursor position
ofr.setFontSize(15); // set font size. 

// and let's draw text !
ofr.cprintf("Hello World"); // -> drawing in done ont im. 
~~~

@note A complete example is available in the subfolder `/examples/Teensy4/OpenFontRender_test/`. 


Check the [OpenfontRender documentation](https://github.com/takkaO/OpenFontRender/blob/master/README.md) for additional details.


@subsection subsec_PNGdec drawing PNG images

Install Bitbank2's **PNGdec** library from the Arduino library manager / platformio of directly from https://github.com/bitbank2/PNGdec/.

We just need to include both libraries into the project to activate the bindings between TGX and PNGdec:
~~~{.cpp}
#include <tgx.h>
#include <PNGdec.h>
~~~
Now, we can bind any `PNG` decoder object tgx images by providing it with the generic callback `TGX_PNGDraw` and then we can susequently decode PNGs using the \ref tgx::Image<color_t>::PNGDecode "Image::PNGDecode()" method. 

Example:
~~~{.cpp}
// create a 150x100 image in RGB565 color format. 
tgx::RGB565 buf[150*100];
tgx::Image<tgx::RGB565> im(buf, 150, 100);
im.clear(tgx::RGB565_Black); // clear to full black

// PNG decoder object from PNGdec library
PNG png; 

// Load the png image "octocat_4bpp" stored in RAM and link the decoder to TGX
// -> note that 'TGX_PNGDraw' is passed as callback parameter.
png.openRAM((uint8_t*)octocat_4bpp, sizeof(octocat_4bpp), TGX_PNGDraw); 

// Draw the PNG onto im, with half opacity and upper left corner at pos (20, 10) inside im. 
im.PNGDecode(png, {20, 10}, 0.5f);
~~~

@note A complete example is available in the subfolder `/examples/Teensy4/PNG_test/`. 

Check the [PNGdec documentation](https://github.com/bitbank2/PNGdec/wiki) for additional details.


@subsection subsec_JPEGDEC drawing JPEG images

Install Bitbank2's **JPEGDEC** library from the Arduino library manager / platformio of directly from https://github.com/bitbank2/JPEGDEC.

We just need to include both libraries into the project to activate the bindings between TGX and JPEGDEC:
~~~{.cpp}
#include <tgx.h>
#include <JPEGDEC.h>
~~~
Now, we can bind any `JPEGDEC` decoder object tgx images by providing it with the generic callback `TGX_JPEGDraw` and then we can susequently decode JPEGs using the \ref tgx::Image<color_t>::JPEGDecode "Image::JPEGDecode()" method. 

Example:
~~~{.cpp}
// create a 150x100 image in RGB565 color format. 
tgx::RGB565 buf[150*100];
tgx::Image<tgx::RGB565> im(buf, 150, 100);
im.clear(tgx::RGB565_Black); // clear to full black

// JPEG decoder object from JPEGDEC library
JPEGDEC jpeg; 

// Load the jpeg image "batman" stored in RAM and link the decoder to TGX
// -> note that 'TGX_JPEGDraw' is passed as callback parameter.
jpeg.openRAM((uint8_t*)batman, sizeof(batman), TGX_JPEGDraw); 

// Draw the JPEG onto im at half scale, half opacity and upper left corner at pos (20, 10) inside im. 
im.JPEGDecode(png, {20, 10}, JPEG_SCALE_HALF, 0.5f);

~~~

@note A complete example is available in the subfolder `/examples/Teensy4/JPEG_test/`. 

Check the [JPEGDEC documentation](https://github.com/bitbank2/JPEGDEC/wiki) for additional details.




@subsection subsec_AnimateGIF drawing GIF images

Install Bitbank2's **AnimateGIF** library from the Arduino library manager / platformio of directly from https://github.com/bitbank2/AnimatedGIF

We just need to include both libraries into the project to activate the bindings between TGX and AnimateGIF:
~~~{.cpp}
#include <tgx.h>
#include <AnimateGIF.h>
~~~
Now, we can bind any `AnimateGIF` decoder object to tgx images by providing it with the generic callback `TGX_GIFDraw` and then we can susequently decode GIFs using the \ref tgx::Image<color_t>::GIFplayFrame() "Image::GIFplayFrame()" method. 

This method simply wraps around the `playFrame()` method fom the animateGIF library and can be called repeatedly to display successives frame in case of an animated GIF. 

Example:

~~~{.cpp}
// create a 150x100 image in RGB565 color format. 
tgx::RGB565 buf[150*100];
tgx::Image<tgx::RGB565> im(buf, 150, 100);
im.clear(tgx::RGB565_Black); // clear to full black

// GIF decoder object from AnimateGIF library
AnimatedGIF gif;

// Load the GIF image "earth_128x128" stored in RAM and link the decoder to TGX
// -> note that 'TGX_GIFDraw' is passed as callback parameter.
gif.open((uint8_t*)earth_128x128, sizeof(earth_128x128), TGX_GIFDraw); 

// Draw the GIF onto im with upper left corner at pos (20, 10) inside im. 
im.GIFplayFrame(gif, { 20, 10 }); 

// we can call im.GIFplayFrame(gif, { 20, 10 }) again to draw the next frame if it is an animated GIF... 
~~~

@note A complete example is available in the subfolder `/examples/Teensy4/GIF_test/`. 

Check the [AnimateGIf documentation](https://github.com/bitbank2/AnimatedGIF/wiki) for additional details.




---



