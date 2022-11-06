@mainpage TGX

@section about About the library

TGX is a tiny but full featured C++ library for drawing 2D and 3D graphics onto a memory framebuffer.
The library is hardware agnostic. It runs on both microprocessor and microcontollers but specifically targets
'powerful' 32 bits MCUs such as ESP32, RP2040, Teensy 3/4...
 
**Features**

- Hardware agnostic: the library simply draws on a memory framebuffer.
- Minimum RAM usage and no dynamic allocation. 
- No external dependency (and no STL): just include the .h and .cpp files to your project and your are set!
- Library in Arduino friendly format. 
- Mutiple color formats: `RGB565`,  `RGB24`, `RGB32`, `RGB64`, `RGBf`, `HSV`
- `Image` class that encapsulates a buffer and allows the creation of sub-images for clipping any drawing operations.    
- Extensive 2D API: 
    - Methods for converting, blittings copying, resizing sprites with high quality (bilinear filtering, sub-pixel precision). 
    - Methods for drawing lines, polylines, polygons, triangle, rectangles, rounded rectangles, quads, circles arc, ellipses, Bezier curve and slines ! 
    - Methods for drawing Text, using regular Adafruit font, ILI9341_t3 font or special anti-aliased ILI9341_t3 v21 fonts. 
    - All drawings operations support alpha-blending. 
    - Most drawing methods come in two flavours: a 'fast' method and a 'high quality' which is slower but draws using anti-aliasing and sub-pixel precision.     
- Extensive 3D API.
    - Optimized 'pixel perfect' triangle rasterizer with adjustable sub-pixel precision. 
    - Depth buffer testing (selectable 16 bits or 32 bits precision).
    - Multiple drawing mode: Wireframe (low quality), Wireframe (high quality) and solid (i.e. with shaders). 
    - Flat and Gouraud Shading
    - Phong lightning's model with separate ambient/diffuse/specular color components
    - Per object material properties.
    - Perspective-correct texture mapping with selectable point sampling / bilinear filtering and muliple wrapping mode.
    - Perspective and Orthographic projection.
    - Optional backface culling.
    - Tile rasterization: it is possible to render only part of the viewport at a time to save RAM. 
    - Templates classes for all the needed maths: Vec2, Vec3, Vec4 (vectors), Mat4 (4x4 matrix) and Box2, Box3 (boxes).
    - Optimized mesh data format: meshes and textures can be read directly from flash memory to save RAM.
    - Python scripts for easy conversion of texture images and 3D meshes (in Wavefront's .obj format) into C files.
    
    

    
    
    
    
@section getting_started Getting started

1. @ref intro_concepts "Basic concepts"
2. @ref intro_api_2D "Using the 2D API."
3. @ref intro_api_3D "Using the 3D API."



@note Do not forget to look at the examples located in `\examples` subfolder of the library !




@section References


**Basic classes (2D graphics)**

|                                   |                                                                       |
|-----------------------------------|-----------------------------------------------------------------------|
| Main image object                 | #tgx::Image                                                           |
| Color types                       | #tgx::RGB565,#tgx::RGB32,#tgx::RGB24,#tgx::RGBf,#tgx::RGB64,#tgx::HSV |
| 2D vectors                        | #tgx::Vec2                                                            |
| 2D box                            | #tgx::Box2                                                            |



**Additional classes specific to drawing 3D graphics**

|                                   |                            |
|-----------------------------------|----------------------------|
| Main class for drawing a 3D scene | #tgx::Renderer3D           |
| 3D model class                    | #tgx::Mesh3D               |
| 3D Box                            | #tgx::Box3                 |
| 3D and 4D vectors                 | #tgx::Vec3, #tgx::Vec4     |
| 4x4 matrix for 3D (quaternions)   | #tgx::Mat4                 |
