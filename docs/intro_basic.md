@page intro_basic Basic concepts. 

The TGX library's main class is the \ref tgx::Image class which, as its name suggests, encapsulate an **image**, that is a rectangular array of **pixels** of a given **color** type. 
The library provides many methods to alter an image, i.e. change the color of its pixels by providing an extensive set of 2D and 3D drawing procedures. 

All methods/classes are defined inside the `tgx` namespace. To use the library, just include "tgx.h":
~~~{.cpp}
#include "tgx.h"
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


# Image class, memory layout and sub-images

An image object is lightweight structure (only 16 bytes in memory) similar to a Numpy view in Python. The object records:
- the dimension of the image (`lx`, `ly`).
- a pointer to the memory buffer `buffer[]` of pixel colors of type `color_t` (the template color type of the image). 
- a `stride` value that is the number of pixels per image row (which may be larger than `ly` when working with sub-images). 

In particular, **the image object does not contain the pixels buffer itself** but only a pointer to its memory location. Therefore, several images 
can reference the same memory buffer: this permits to create images/sub-images that share the same pixels buffer.

The position of a pixel in a image is,

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

@note Creating image/sub-image is very fast and cost free so the user should not refrain from creating Image/sub-images whenever needed ! It is the most efficient solution to clip/restrict any drawing operation to a given (rectangular) region of an image. 

# Vectors and Boxes

The tgx library defines classes for 2D vectors and boxes:

- \ref tgx::iVec2 : integer-valued 2D vector. ** Use for pixel location in images**
- \ref tgx::fVec2 : floating point-valued 2D vector. **Used for pixel location in images when using sub-pixel precision**
- \ref tgx::iBox2 : integer-valued 2D box. **Used to represent a rectangular region insid an image**
- \ref tgx::fBox2 : floating point-valued 2D box. **represent a rectangular region insid an image when using sub-pixel precision**

3D variants are also available: \ref tgx::iVec3, \ref tgx::fVec3, \ref tgx::iBox3, \ref tgx::fBox3 c.f. the 3D API for details. 



# Copying/converting images. 


# blending and opacity parameter. 


section: copy, blitting, subimage

section: misc

image type, view (shared buffer) et copy. 

color types

opacity, blending

ivec et positionning 

A faire. 