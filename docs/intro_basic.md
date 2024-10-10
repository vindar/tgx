@page intro_basic Basic concepts. 

The TGX library's main class is the \ref tgx::Image class which, as its name suggests, encapsulate an **image**, that is a rectangular array of **pixels** of a given **color** type. 
The library provides an extensive set of 2D and 3D drawing procedures to operates on image objects. 

All methods/classes are defined inside the `tgx` namespace. To use the library, just include `tgx.h`:
~~~{.cpp}
#include <tgx.h>
using namespace tgx; // optional, so we no not need to prefix all methods by tgx::
~~~

# Colors types

The library defines several color classes

- \ref tgx::RGB565 : 16 bits colors: 5 bits red, 6 bits green, 5 bits blue. *Aligned as and convertible to `uint16_t`.* **Preferred type when working with embedded platforms/MCUs**.  
- \ref tgx::RGB24 : 24 bits color: 8 bits red, 8 bits: green, 8 bits blue. *No alignement*. 
- \ref tgx::RGB32 : 32 bits color: 8 bits red, 8 bits: green, 8 bits blue. 8 bits (pre-multipled) Alpha channel. *Aligned as and convertible to `uint32_t`*. **Useful for blending**. 
- \ref tgx::RGBf : 96 bits floating point color: 32 bits red (float), 32 bits green (float), 32 bits blue  (float). *Aligned as `float`, no alpha channel*. **Used in the 3D API**.  
- \ref tgx::RGB64 : 64 bits color: 16 bits red, 16 bits: green, 16 bits blue. 16 bits (pre-multiplied) Alpha channel. *Aligned as and convertible to `uint64_t`*. **Experimental, use RGB32 or RGBf if instead**. 
- \ref tgx::HSV : 96 bits colors: 16 bits Hue, 16 bits Saturation, 16 bits Values. *Partial support only*. **Slow, use only to convert between color space, not for drawing operations**.  


Color types are handled just like usual basic type `int` , `char`... Any color type can be converted into any other color type. 

Examples:
~~~{.cpp}
tgx::RGB565 color1; // create a 16 bits color, undefined value
tgx::RGB565 color2(8, 32, 16); // create a 16 bits color and set the color channels: 8 red (5bits), 32 green (6bits), 16 blue (5bits)
tgx::RGB565 color3(0.25f, 0.5f, 0.5f); // the same as color2 but defined with floats in [0.0f,1.0f] (works for any color type, irrespective of the channel color depths) 

uint16_t c2 = (uint16_t)color3; // conversion to uint16_t. 
tgx::RGB565 color4 = c2; // create 16 bit color from uint16_t.

int8_t R = color4.R; //   
int8_t G = color4.G; // access the raw channels values  
int8_t B = color4.B; //   

tgx::RGB32 color5(color2); // conversion: create a RGB32 color from RGB565, this yields RGB32(64, 128, 128, 255) - the alpha channel is set to fully opaque.
color5.setOpacity(0.5f); // adjust the alpha channel in order to make the color half transparent. Other channels are multiplied accordingly (pre-multiplied alpha). 
~~~

Predefined colors, *Black*, *White*, *Blue*, *Green*, *Purple*, *Orange*, *Cyan*, *Lime*, *Salmon*, *Maroon*, *Yellow*, *Magenta*, *Olive*, *Teal*, *Gray*, *Silver*, *Navy*, *Transparent*. 

Example:
~~~{.cpp}
tgx::RGB32_Yellow; // yellow color in RGB32 
tgx::RGB565_Green; //  green color in RGB565
~~~

For additional details on color manipulations, look directly into the color classes definitions in \ref color.h. 


## blending and opacity parameter. 

Color types \ref tgx::RGB32 and \ref tgx::RGB64 have an alpha channel which is used for alpha-blending during drawing operations. 

\note Colors with an alpha channel are always assumed to have [pre-multiplied alpha](https://en.wikipedia.org/wiki/Alpha_compositing). 

Other color type do not have an alpha channel but the library still support simple blending operations through the (optional) `opacity` parameter added to all drawing primitives. This parameter ranges in `[0.0f, 1.0f]` with
- `opacity = 1.0f`: fully opaque drawing. This is the default value when the parameter is omited.
- `opacity = 0.0f`: fully transparent drawing (so this draws nothing).

Example:
~~~{.cpp}

// assume below that `im` is a tgx::Image<tgx::RGB565> object
// the method Image::drawLine has signature  drawLine(iVec2 P1, iVec2 P2, color_t color, float opacity). 

im.drawLine({10, 20}, {30, 40}, tgx::RGB565_Red); // draw a fully opaque red line from (10,20) to (30, 40).

im.drawLine({10, 20}, {30, 40}, tgx::RGB565_Red, 0.5f); // here opacity=0.5f so we draw a half-transparent red line instead.

~~~


---

# Image class, memory layout and sub-images

An image object is a lightweight structure (only 16 bytes in memory) similar to a Numpy view in Python. The object records:
- the dimension of the image (`lx`, `ly`).
- a pointer to the memory buffer `buffer[]` of pixel colors of type `color_t` (the template color type of the image). 
- a `stride` value that is the number of pixels per image row (which may be larger than `ly` when working with sub-images). 

In particular, **the image object does not contain the pixels buffer itself** but only a pointer to its memory location. **As a general rule, the TGX library never perform any dynamic memory allocation**. 

Several images can reference the same memory buffer (or a part of it). Creating images/sub-images that share the same pixels buffer is useful to restrict drawing operations to a given rectangular region 

The position of a pixel in an image is

```Pixel(i, j) = buffer[i + stride*j]```

for 0 &le; `i` &lt; `lx` and 0 &le; `j` &lt; `ly`.

Example
~~~{.cpp}
tgx::RGB32 buff[320*240];  // memory buffer for a 320x240 image in RGB32 color format.

tgx::Image<tgx::RGB32> im0; // create an empty (invalid) image of type RGB32. 
im0.set(buff, 320,240) // set the image pixel buffer and dimension (default stride  = ly). 

tgx::Image<tgx::RGB32> im1(buff, 320,240); // construct and set the image parameter at the same time (same as im0)

tgx::Image<tgx::RGB32> im2 = im({20, 50, 100, 200}); // create a sub-image restricted ot the region [20,50]x[100,200] of im2. 

im0.lx(); // width of im1, here it is 320; 
im2.ly(); // height of im2, here it is 101 = 200 - 100 + 1

im0.data(); // return the adress of the buffer (here = buff)
im2.data(); // return the adress of the buffer (here = buff + 20 + 100*stride)

im0.stride(); // im1 stride, (here = 320)
im2.stride(); // im2 stride, (here = 320 same as im1)
~~~

See also: \ref tgx::Image::set() "Image::set()", \ref tgx::Image::crop() "Image::crop()", \ref tgx::Image::getCrop() "Image::getCrop()", \ref tgx::Image::operator()() "Image::operator()", \ref tgx::Image::isValid() "Image::isValid()", \ref tgx::Image::setInvalid() "Image::setInvalid()", \ref tgx::Image::imageBox() "Image::imageBox()". 

@note Creating image/sub-image is very fast and "cost free" so one should not refrain from creating Image/sub-images whenever needed ! It is the most efficient solution to clip/restrict any drawing operation to a given (rectangular) region of an image. 

---

# Vectors and Boxes

The tgx library defines classes for 2D vectors and boxes:

- \ref tgx::iVec2 : integer-valued 2D vector. ** Use for pixel location in images**
- \ref tgx::fVec2 : floating point-valued 2D vector. **Used for pixel location in images when using sub-pixel precision**
- \ref tgx::iBox2 : integer-valued 2D box. **Used to represent a rectangular region inside an image**
- \ref tgx::fBox2 : floating point-valued 2D box. **represent a rectangular region inside an image when using sub-pixel precision**

@note 3D variants are also available: \ref tgx::iVec3, \ref tgx::fVec3, \ref tgx::iBox3, \ref tgx::fBox3 c.f. the 3D API for more details. 

@warning Pixel adressing varies slightly when using integer valued coordinates vs floating point valued coordinates.
- **Integer valued cordinates**: `iVec2(i,j)` represents the location of the pixel `(i,j)` in the image and the whole image corresponds to the (integer valued) box `iBox2(0, lx - 1, 0, ly - 1)`.
- **Floating point valued coordinates**: `fVec2(i,j)` represents the **center** of pixel `(i,j)` in the image. The pixel itself is a unit lenght square box centered around this location i.e. represented by `fBox2( i-0.5f, i+0.5f, j-0.5f, j+0.5f)`. In this case, the whole image corresponds to the (floating point valued) box `fBox2( -0.5f, lx - 0.5f, -0.5f, ly - 0.5f)`.

Vector and boxes support all the usual operations: arithmetics, copy, type conversion...

Most drawing methods take vectors and boxes as input parameters instead of scalars. Using initializer lists makes the code more readable: we can simply write `{10, 20}` instead of `tgx::iVec2(10, 20)`. 

Example:
~~~{.cpp}
tgx::iVec2 v1(10, 30); // integer valued vector (10,30)

tgx::fVec2 v2 = v1; // conversion: floating point valued vector (10.0f, 30.0f)

float x = 2.0f;
tgx::fVec2 v3 = { x, 7 }; // construct the vector from an initializer list. vector (2.0f, 7.0f)
    
(v2 + v3) * 3.0f; // usual maths operations are  available...

v3.x = 4; // direct access to coordinate values v3 is now (2.0f, 4.0f)

tgx::iBox2 B(40, 80, 20, 30); // format (minx, maxx, miny, maxy) -> integer valued box [40,80]x[20,30]

// assume below that `im` is a tgx::Image<tgx::RGB32> object
// method `Image::drawCircle(iVec2 center, int r, color_t color, float opacity)` takes integer value parameters. 
im.drawCircle(v1, 12, tgx::RGB32_Red); // draw a red circle centered at v1 = (10, 30) of radius 12. 

// method `Image::drawCircleAA(fVec2 center, float r, color_t color, float opacity)` takes floating point valued parameters for sub-pixel precision: 
im.drawCircleAA({50.0f, 60.0f} , 11.5f, tgx::RGB32_Red); // draw a blue circle centered at (50.0f, 60.0f) of radius 11.5f. Use an initalizer list to define the center. 

// method `Image::fillRect(const iBox2 & B, color_t color, float opacity)` takes an integer valued box as input parameter.
im.fillRect(B, tgx::RGB32_Black); // draw a filled black box [40, 80]x[20,30]
im.fillRect({50, 60 , 70, 80}, tgx::RGB32_Black); // using an initializer list: draw a filled black box [50, 60]x[70,80]
~~~


---


# Storing image in .cpp files

The memory buffer that an image points to may reside in flash/ROM and it if possible to define 'const' image directly in `.cpp` files for easy inclusion in a project. 

For example, the 10x10 image

![smiley_icon](../smileybig.png)   

Can be stored as:

~~~{.cpp}
// macro to make the array more readable/compact.  
#define C(val) tgx::RGB565((uint16_t)val)

// image data, the PROGMEM keyword is to insure that the image is store in FLASH memory. 
const tgx::RGB565 smiley_data[] PROGMEM = {
C(0xffff), C(0xffff), C(0xf714), C(0xe609), C(0xdd40), C(0xdd40), C(0xe609), C(0xf714), C(0xffff), C(0xffff),
C(0xffff), C(0xee8f), C(0xddc6), C(0xeef1), C(0xfffb), C(0xfffb), C(0xeef1), C(0xddc6), C(0xee8f), C(0xffff),
C(0xeef4), C(0xdda5), C(0xf775), C(0xffd8), C(0xffd8), C(0xffd8), C(0xffd8), C(0xf775), C(0xdda5), C(0xeef4),
C(0xd5a9), C(0xee8d), C(0xff94), C(0x9b20), C(0xff94), C(0xff94), C(0x9b20), C(0xff94), C(0xee8d), C(0xd5a9),
C(0xc4a0), C(0xff50), C(0xff50), C(0xff50), C(0xff50), C(0xff50), C(0xff50), C(0xff50), C(0xff50), C(0xc4a0),
C(0xbc60), C(0xff0b), C(0xff0b), C(0xff0b), C(0xff0b), C(0xff0b), C(0xff0b), C(0xff0b), C(0xff0b), C(0xbc60),
C(0xc4e8), C(0xe5c4), C(0xfea6), C(0xaba0), C(0xfea6), C(0xfea6), C(0xaba0), C(0xfea6), C(0xe5c4), C(0xc4e8),
C(0xd612), C(0xc481), C(0xf622), C(0xdd40), C(0x9b20), C(0x9b20), C(0xdd40), C(0xf622), C(0xc481), C(0xd612),
C(0xf79e), C(0xc54d), C(0xbc60), C(0xdd60), C(0xfe40), C(0xfe40), C(0xdd60), C(0xbc60), C(0xc54d), C(0xf79e),
C(0xffff), C(0xef7d), C(0xcdd2), C(0xb4a8), C(0xaba0), C(0xaba0), C(0xb4a8), C(0xcdd2), C(0xef7d), C(0xffff) };

// image object, 10x10 image with smiley_data as pixel buffer. 
const tgx::Image<tgx::RGB565> smiley(smiley_data, 10, 10);
~~~

The `/tools/` subdirectory of the library contain the Python script `image_converter.py` which allows to easily convert an image from a classical format (jpeg/png/bmp...) into a .cpp file.

@note It is also easy to directly load PNG, JPEG and GIF images (which may be stored in RAM/FLASH or on another media such as a SD card) by using bindings between TGX and external libraries. See section \ref sec_extensions for more details. 



# Copying/converting images to different color types. 

Recall that a \ref tgx::Image object is just a view into a buffer of pixels of a given color type. Thus, copies of image object are shallow (they share the same image buffer / same color type). 

- Method \ref tgx::Image<color_t>::copyFrom() "Image<color_t>::copyFrom(src_im, opacity)" can be used to create deep copy of images and it can also change the color type and resize them (using bilinear interpolation). 
- Method \ref tgx::Image<color_t>::convert() "Image<color_t>::convert<color_dst>()" can be used to change the color type of an image "in place". 

~~~{.cpp}
tgx::RGB32 buf1[100*100]; // buffer for a 100x100 image in RGB32 color format
tgx::Image<tgx::RGB32> im1(buf1, 100, 100); // image that encapsulates the pixel buffer buf1
    
// ... draw some things on im...
    
tgx::RGB24 buf2[50 * 50]; // buffer for a 50x50 image in RGB24 color format
tgx::Image<tgx::RGB24> im2(buf2, 50, 50); // image that encapsulates the pixel buffer buf2

// copy the content of im1/buf1 into im2/buf2, resize and perform color conversion automatically !   
im2.copyFrom(im1);

// we can directly change the color type of im1 in place from RGB32 to RGB565
tgx::Image<tgx::RGB565> im3 = im1.convert<tgx::RGB565>();

// ok, now im3 has the same content as im1 had, but in RGB565 color format. 
// Beware that im1 now contains 'garbage' (seen as RGB32 data) but writing 
// on im1 will change the content of im3 as they share the same buffer !

~~~




# Drawing things on an images...

Here comes the good part ! Now that can can create and manipulate images, we obviously want to draw thing on them !

TGX defines many primitives to draw shapes (such as lines, rectangle, circles, bezier curves), to blit sprites or write text... As well as a simple (but full-featured!) 3D engine. 

- **Details for 2D drawing are provided in** @ref intro_api_2D "the 2D API tutorial"
- **Details for 3D drawing are provided in** @ref intro_api_3D "the 3D API tutorial"


