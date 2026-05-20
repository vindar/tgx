cmake_minimum_required(VERSION 3.10)

project(tgx_3d_cpu_suite)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

include_directories("../../../src/")
include_directories("../../../examples/benchmark/")
include_directories("../../../examples/CPU/buddhaOnCPU/")

file(GLOB tgx_SRC "../../../src/*.cpp")

add_executable(${PROJECT_NAME}
    tgx_3d_cpu_suite.cpp
    ${tgx_SRC}
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_compile_definitions(${PROJECT_NAME} PUBLIC TGX_RGB32_ORDER_BGR=0)

if (WIN32)
    target_link_libraries(${PROJECT_NAME} PUBLIC user32 gdi32 shell32)
    target_compile_options(${PROJECT_NAME} PRIVATE /wd4305)
endif ()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "${PROJECT_NAME}")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(PREDEFINED_TARGETS_FOLDER "CustomTargets")
