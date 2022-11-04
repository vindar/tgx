@mainpage TGX

@section about About the library

TGX is a tiny but full featured C++ library for drawing 2D and 3D graphics onto a memory framebuffer.
The library is hardware agnostic. It runs on both microprocessor and microcontollers but specifically targets 
'powerful' 32 bits MCUs such as ESP32, RP2040, Teensy 3/4...
 
**Features**

- Hardware agnostic: the library simply draws on a memory framebuffer.
- Minimum RAM usage and no dynamic allocation. 
- Extensive 2D API. 
- Extensive 3D API.
- ...


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
