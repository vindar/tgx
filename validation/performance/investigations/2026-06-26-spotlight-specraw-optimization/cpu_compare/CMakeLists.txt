cmake_minimum_required(VERSION 3.10)

project(spotlight_cpu_compare)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

if (NOT DEFINED TGX_ROOT)
    set(TGX_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../..")
endif ()

include_directories("${TGX_ROOT}/src")
include_directories("${TGX_ROOT}/examples/CPU/buddhaOnCPU")

file(GLOB tgx_SRC "${TGX_ROOT}/src/*.cpp")

add_executable(${PROJECT_NAME}
    spotlight_cpu_compare.cpp
    ${tgx_SRC}
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_compile_definitions(${PROJECT_NAME} PUBLIC TGX_RGB32_ORDER_BGR=0)

if (WIN32)
    target_link_libraries(${PROJECT_NAME} PUBLIC user32 gdi32 shell32)
    target_compile_options(${PROJECT_NAME} PRIVATE /wd4305)
endif ()

