@mainpage TGX

@section about About the library

TGX is a tiny but full featured C++ library for drawing 2D and 3D graphics onto a memory framebuffer.
The library is hardware agnostic. It runs on both microprocessor and microcontollers but specifically targets 
'powerful' 32 bits MCUs such as ESP32, RP2040, Teensy 3/4...
 
**Features**

- Hardware agnostic: the library simply draws on a memory framebuffer.
- Minimum RAM usage and no dynamic allocation. 
- Complete 2D API. 
- COmplete 3D API.


@section getting_started Getting started

1. @ref intro_concepts "Basic concepts"
2. @ref intro_api_2D "Using the 2D API."
3. @ref intro_api_3D "Using the 3D API."


@section References

- #tgx::Image
- #tgx::Box2
- #tgx::Vec2::norm2()