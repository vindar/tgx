cmake_minimum_required(VERSION 3.10)

project(tgx_cpu_validation_tests LANGUAGES CXX)

add_executable(tgx_text_validation
    text_validation.cpp
    ../../src/Color.cpp
    ../../src/Fonts.cpp
    ../../src/font_tgx_OpenSans.cpp
)

target_include_directories(tgx_text_validation PRIVATE ../..)
target_compile_features(tgx_text_validation PRIVATE cxx_std_17)

add_executable(tgx_blit_copy_validation
    blit_copy_validation.cpp
    ../../src/Color.cpp
)

target_include_directories(tgx_blit_copy_validation PRIVATE ../..)
target_compile_features(tgx_blit_copy_validation PRIVATE cxx_std_17)
