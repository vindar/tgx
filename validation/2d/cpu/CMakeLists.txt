cmake_minimum_required(VERSION 3.10)

project(tgx_2d_cpu_suite LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

add_executable(tgx_2d_cpu_suite
    tgx_2d_cpu_suite.cpp
    ../../../src/Color.cpp
    ../../../src/Fonts.cpp
    ../../../src/font_tgx_OpenSans.cpp
    ../../../src/font_tgx_OpenSans_Bold.cpp
    ../../../src/font_tgx_Arial.cpp
)

target_include_directories(tgx_2d_cpu_suite PRIVATE ../../.. ../../../src)
target_compile_features(tgx_2d_cpu_suite PRIVATE cxx_std_17)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT tgx_2d_cpu_suite)
