@mainpage TGX

@section about About the library

TGX is a tiny but full featured C++ library for drawing 2D and 3D graphics onto a memory framebuffer.
The library runs on both microprocessor and microcontollers but specifically targets 'powerful' 32 bits MCUs such as ESP32, RP2040, Teensy 3/4...
 
**Features**

- Hardware agnostic: the library simply draws on a memory framebuffer.
- Optimized RAM usage and no dynamic allocation. 
- No external dependency (and no STL dependency): just include the `.h` and `.cpp` files to your project and your are set!
- Library in Arduino friendly format. 
- Mutiple color formats supported: \ref tgx::RGB565 "RGB565",  \ref tgx::RGB24 "RGB24", \ref tgx::RGB32 "RGB32", \ref tgx::RGB64 "RGB64", \ref tgx::RGBf "RGBf", \ref tgx::HSV "HSV"
- \ref tgx::Image "Image" class that encapsulates a memory buffer and allows the creation of sub-images for clipping any drawing operations.    
- **Extensive 2D API:** 
    - Methods for converting, blittings, copying and resizing sprites with high quality (bilinear filtering, sub-pixel precision). 
    - Methods for drawing lines, polylines, polygons, triangles, rectangles, rounded rectangles, quads, circles, arcs, ellipses, Bezier curve and splines ! 
    - Methods for drawing Text with support for multiple font formats.
    - All drawings operations support alpha-blending. 
    - Many drawing methods come in two flavours: fast' methods and 'high quality' methods which is slower but draws using anti-aliasing and/or sub-pixel precision.     
- **Extensive 3D API:**
    - Optimized 'pixel perfect' triangle rasterizer with adjustable sub-pixel precision. 
    - Depth buffer testing (selectable 16 bits or 32 bits precision).
    - Multiple drawing modes: wireframe (selectable low/high quality) and solid (i.e. with shaders). 
    - Flat and Gouraud Shading.
    - Phong lightning's model with separate ambient/diffuse/specular color components.
    - Per object material properties.
    - Perspective-correct texture mapping with selectable point sampling/bilinear filtering and multiple wrapping modes.
    - Perspective and orthographic projections.
    - Optional backface culling.
    - Tile rasterization: it is possible to render the viewport in multiple passes to save RAM. 
    - Templates classes for all the needed maths: \ref tgx::Vec2 "Vec2", \ref tgx::Vec3 "Vec3", \ref tgx::Vec4 "Vec4" (vectors), \ref tgx::Mat4 "Mat4" (4x4 matrix) and \ref tgx::Box2 "Box2", \ref tgx::Box3 "Box3" (boxes).
    - Optimized \ref tgx::Mesh3D "mesh" data format: meshes and textures can be read directly from flash memory to save RAM.
    - Python scripts provided for easy conversion of texture images and 3D meshes from Wavefront's .obj format into C files.
    
    
    
      
    
@section getting_started Getting started

1. @ref intro_concepts "Installation"
2. @ref intro_tgximage "The Image class"
3. @ref intro_api_2D "Using the 2D API."
4. @ref intro_api_3D "Using the 3D API."



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
