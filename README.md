# TGX - a tiny/teensy graphics library

<p align="center">
<img src="./tgx.jpg" height="400" />
</p>

<p align="center">
<a href="http://www.youtube.com/watch?feature=player_embedded&v=fivrlLtbYX8
" target="_blank"><img src="http://img.youtube.com/vi/fivrlLtbYX8/0.jpg" 
 width="240" height="180" border="10" /></a>
<a href="http://www.youtube.com/watch?feature=player_embedded&v=arJbbU28FEU
" target="_blank"><img src="http://img.youtube.com/vi/arJbbU28FEU/0.jpg" 
 width="240" height="180" border="10" /></a>  
<a href="http://www.youtube.com/watch?feature=player_embedded&v=96D0j9J2ILs
" target="_blank"><img src="http://img.youtube.com/vi/96D0j9J2ILs/0.jpg" 
 width="240" height="180" border="10" /></a>
</p>

**Note** A new companion library <a href="https://github.com/vindar/tgx-font">tgx-font</a> provides a collection of plain and anti-aliased fonts which can be used with this library.

## 2D and 3D graphics library for 32bits microcontrollers. 

This library implements a sets of classes that makes it simple to draw 2D and 3D graphics onto a memory framebuffer. The library is aimed and optimized toward 32bits MCUs such as Teensy or ESP32 yet it is cross-platform and also works on CPUs (mostly for testing purposes, much better alternatives are available on a real CPU with dedicated hardware). 

The library's purpose is to draw graphics **on  a memory framebuffer**. As such, it does not provide  any hardware/screen support. You will need a screen driver to display the memory framebuffer onto a physical screen. If using an ILI9341 screen via SPI, you may consider [Kurte's ILI93451_t3n library](https://github.com/KurtE/ILI9341_t3n) or [my own optimized driver](https://github.com/vindar/ILI9341_T4) (only for Teensy 4/4.1). 

Here are some of the library best features.

### 2D graphics.

- Support for multiple color types: `RGB565`, `RGB24`, `RGB32`, `RGB64`, `RGBf` and `HSV`. Every 2D/3D drawing operation is available for each color type. 
- Template `Image` class that encapsulates a memory framebuffer and enables the creation of sub-images (i.e. views) that share the same buffer. This provides an elegant and efficient way to clip of all drawing operations to a particular region. 
- API (mostly) compatible with Adafruit's GFX library, but with more primitives and faster ! Primitive for drawing, lines, triangles, rectangles circles, ellipses...
- Methods for blitting sprites (with or without mask), image color type conversion and resizing. 
- Transparency / Alpha blending supported for all drawing methods.
- Anti-aliased methods for drawing thick lines and circles. 
- Support for Adafruit's fonts as well as PJRC's ILI9341_t3 v1 and v2.3 anti-aliased fonts (see <a href="https://github.com/vindar/tgx-font">tgx-font</a>).

### 3D graphics.

- Heavily optimized "pixel perfect" triangle rasterizer with 8bits sub-pixels precision. 
- Depth buffer testing. 
- Flat and Gouraud shading.
- Phong lightning model with separate ambient/diffuse/specular color components (currently only one directional light source). 
- Per object material properties. 
- Perspective-correct texture mapping. 
- Perspective and Orthographic projection supported. 
- Optional backface culling.
- Tile rasterization: it is possible to render only part of the viewport at a time to save RAM by using a smaller image and a smaller zbuffer. 
- Templates classes for all the needed maths: `Vec2`, `Vec3`, `Vec4` (vectors), `Mat4` (4x4 matrix) and `Box2` (2D box). 
- Optimized mesh data format: meshes and textures can be read directly from flash memory to save RAM.
- Python scripts for easy conversion of texture images and 3D meshes (in Wavefront's .obj format) into C files that can be directly imported into an Arduino project. 

## Using the library. 

**TODO**

- Documentation available in the docstrings located in the header files. 

- Basic library usage can be found by looking at the examples located in the `/examples` sub-directory.

- Python conversion scripts needed to import meshes and textures are  located in the `/extras - converters` sub-directory.






