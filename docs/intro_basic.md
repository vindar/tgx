@page intro_basic Basic concepts. 

The TGX library's main class is the \ref tgx::Image class which, as its name suggests, encapsulate an **image**, that is a rectangular array of **pixels** of a given **color** type. 
The library provides many methods to alter an image, i.e. change the color of its pixels by providing an extensive set of 2D and 3D drawing procedures. 

# Colors types

The library defines the following color classes

- \ref tgx::RGB565 : 16 bits colors: 5 bits red, 6 bits green, 5 bits blue. *Aligned as and convertible to `uint16_t`.* **Note: Preferred type when working with embedded platforms/MCUs**.  
- \ref tgx::RGB24 : 24 bits color: 8 bits red, 8 bits: green, 8 bits blue. *No alignement*. 
- \ref tgx::RGB32 : 32 bits color: 8 bits red, 8 bits: green, 8 bits blue. 8 bits (pre-multipled) Alpha channel. *Aligned as and convertible to `uint32_t`*. **Note: Preferred type when working on CPUs, using blending/alpha channel**. 
- \ref tgx::RGBf : 96 bits floating point color: 32 bits red (float), 32 bits green (float), 32 bits blue  (float). *Aligned as `float`, no alpha channel*. **Note: Used in the 3D API / Rasterizer. Convertible to other color type without precision loss**.  
- \ref tgx::RGB64 : 64 bits color: 16 bits red, 16 bits: green, 16 bits blue. 16 bits (pre-multiplied) Alpha channel. *Aligned as and convertible to `uint64_t`*. **Experimental, favor RGB32 or RGBf instead whenever possible**. 
- \ref tgx::HSV : 96 bits colors: 16 bits Hue, 16 bits Saturation, 16 bits Values. *Partial support only*. **Slow, use only to convert between color space but not for 2D/3D drawing operations**.  


Color types are handled just like usual basic type `int` , `char`... Any color type can be converted into any other color type. 

Examples:
~~~{.cpp}
tgx::RGB565 color1; // create a 16 bits color, undefined value
tgx::RGB565 color2(8, 32, 16); // create a 16 bits color and set the color channels: 8 red (5bits), 32 green (6bits), 16 blue (5bits)
tgx::RGB565 color3(0.25f, 0.5f, 0.5f); // the same as color2 but defined with floats in [0.0f,1.0f] (works for any color type, irrespective of the channel color depths) 

uint16_t c2 = (uint16_t)color3; // conversion to uint16_t. 
tgx::RGB565 color4 = c2; // create 16 bit color from uint16_t.

int8_t R = color3.R; //   
int8_t G = color3.G; // access the raw channel values  
int8_t B = color3.B; //   

tgx::RGB32 color4(color2) // conversion: create a RGB32 color from RGB565, this yields RGB32(64, 128, 128, 255) - the alpha channel is set to fully opaque.
color4.setOpacity(0.5f); // adjust the alpha channel in order to make the color half transparent. Other channels are alos multiplied accordingly since we use pre-multiplied alpha. 

~~~

For more details about colors, see the file \ref color.h. 


# Memory layout / sub-images


# Vectors and Boxes


# Copying and converting images. 



section: copy, blitting, subimage

section: misc

image type, view (shared buffer) et copy. 

color types

opacity, blending

ivec et positionning 

A faire. 