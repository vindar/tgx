cmake_minimum_required(VERSION 3.15)
project(tgx_mesh3d2_validation CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(tgx_mesh3d2_validation
    mesh3d2_validation.cpp
    ../../../src/Color.cpp
    ../../../src/Fonts.cpp
    ../../../src/Renderer3D.cpp
    ../../../src/font_tgx_Arial.cpp
    ../../../src/font_tgx_Arial_Bold.cpp
    ../../../src/font_tgx_OpenSans.cpp
    ../../../src/font_tgx_OpenSans_Bold.cpp
    ../../../src/font_tgx_OpenSans_Italic.cpp
)

target_include_directories(tgx_mesh3d2_validation PRIVATE
    ../../../src
    generated
)
