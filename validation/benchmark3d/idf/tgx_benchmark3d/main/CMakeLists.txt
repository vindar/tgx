if(NOT DEFINED TGX_BENCH_MODULE)
    set(TGX_BENCH_MODULE "core_raster")
endif()

idf_component_register(
    SRCS "app_main.cpp"
    INCLUDE_DIRS "."
    REQUIRES tgx esp_timer esp_psram
)

target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_17)
target_compile_definitions(${COMPONENT_LIB} PRIVATE TGX_BENCH_IDF_BUILD=1)

if(TGX_BENCH_MODULE STREQUAL "mixed_stress")
    target_compile_definitions(${COMPONENT_LIB} PRIVATE TGX_BENCH_MODULE_MIXED_STRESS=1)
elseif(TGX_BENCH_MODULE STREQUAL "integrated")
    target_compile_definitions(${COMPONENT_LIB} PRIVATE TGX_BENCH_MODULE_INTEGRATED=1)
elseif(TGX_BENCH_MODULE STREQUAL "skybox_noz")
    target_compile_definitions(${COMPONENT_LIB} PRIVATE TGX_BENCH_MODULE_SKYBOX_NOZ=1)
elseif(TGX_BENCH_MODULE STREQUAL "wire_points")
    target_compile_definitions(${COMPONENT_LIB} PRIVATE TGX_BENCH_MODULE_WIRE_POINTS=1)
elseif(TGX_BENCH_MODULE STREQUAL "mesh")
    target_compile_definitions(${COMPONENT_LIB} PRIVATE TGX_BENCH_MODULE_MESH=1)
elseif(TGX_BENCH_MODULE STREQUAL "primitives")
    target_compile_definitions(${COMPONENT_LIB} PRIVATE TGX_BENCH_MODULE_PRIMITIVES=1)
elseif(TGX_BENCH_MODULE STREQUAL "lighting_shading")
    target_compile_definitions(${COMPONENT_LIB} PRIVATE TGX_BENCH_MODULE_LIGHTING_SHADING=1)
elseif(TGX_BENCH_MODULE STREQUAL "texture_raster")
    target_compile_definitions(${COMPONENT_LIB} PRIVATE TGX_BENCH_MODULE_TEXTURE_RASTER=1)
elseif(TGX_BENCH_MODULE STREQUAL "core_raster")
    target_compile_definitions(${COMPONENT_LIB} PRIVATE TGX_BENCH_MODULE_CORE_RASTER=1)
else()
    message(FATAL_ERROR "Unknown TGX_BENCH_MODULE=${TGX_BENCH_MODULE}")
endif()
