#include "../../../boards/espidf/board_config.h"

#include "../../../common/tgx_bench.h"

#if defined(TGX_BENCH_MODULE_MIXED_STRESS)
#include "../../../modules/mixed_stress/bench_mixed_stress.h"
#elif defined(TGX_BENCH_MODULE_INTEGRATED)
#include "../../../modules/integrated/bench_integrated.h"
#elif defined(TGX_BENCH_MODULE_SKYBOX_NOZ)
#include "../../../modules/skybox_noz/bench_skybox_noz.h"
#elif defined(TGX_BENCH_MODULE_WIRE_POINTS)
#include "../../../modules/wire_points/bench_wire_points.h"
#elif defined(TGX_BENCH_MODULE_MESH)
#include "../../../modules/mesh/bench_mesh.h"
#elif defined(TGX_BENCH_MODULE_PRIMITIVES)
#include "../../../modules/primitives/bench_primitives.h"
#elif defined(TGX_BENCH_MODULE_LIGHTING_SHADING)
#include "../../../modules/lighting_shading/bench_lighting_shading.h"
#elif defined(TGX_BENCH_MODULE_TEXTURE_RASTER)
#include "../../../modules/texture_raster/bench_texture_raster.h"
#else
#include "../../../modules/core_raster/bench_core_raster.h"
#endif

#include "../../../common/tgx_bench.cpp"
#include "../../../boards/espidf/board_backend.cpp"

#if defined(TGX_BENCH_MODULE_MIXED_STRESS)
#include "../../../modules/mixed_stress/bench_mixed_stress.cpp"
#elif defined(TGX_BENCH_MODULE_INTEGRATED)
#include "../../../modules/integrated/bench_integrated.cpp"
#elif defined(TGX_BENCH_MODULE_SKYBOX_NOZ)
#include "../../../modules/skybox_noz/bench_skybox_noz.cpp"
#elif defined(TGX_BENCH_MODULE_WIRE_POINTS)
#include "../../../modules/wire_points/bench_wire_points.cpp"
#elif defined(TGX_BENCH_MODULE_MESH)
#include "../../../modules/mesh/bench_mesh.cpp"
#elif defined(TGX_BENCH_MODULE_PRIMITIVES)
#include "../../../modules/primitives/bench_primitives.cpp"
#elif defined(TGX_BENCH_MODULE_LIGHTING_SHADING)
#include "../../../modules/lighting_shading/bench_lighting_shading.cpp"
#elif defined(TGX_BENCH_MODULE_TEXTURE_RASTER)
#include "../../../modules/texture_raster/bench_texture_raster.cpp"
#else
#include "../../../modules/core_raster/bench_core_raster.cpp"
#endif

extern "C" void app_main(void)
{
#if defined(TGX_BENCH_MODULE_MIXED_STRESS)
    (void)tgxbench::benchRunModule(tgxbench::getMixedStressBenchModule());
#elif defined(TGX_BENCH_MODULE_INTEGRATED)
    (void)tgxbench::benchRunModule(tgxbench::getIntegratedBenchModule());
#elif defined(TGX_BENCH_MODULE_SKYBOX_NOZ)
    (void)tgxbench::benchRunModule(tgxbench::getSkyboxNoZBenchModule());
#elif defined(TGX_BENCH_MODULE_WIRE_POINTS)
    (void)tgxbench::benchRunModule(tgxbench::getWirePointsBenchModule());
#elif defined(TGX_BENCH_MODULE_MESH)
    (void)tgxbench::benchRunModule(tgxbench::getMeshBenchModule());
#elif defined(TGX_BENCH_MODULE_PRIMITIVES)
    (void)tgxbench::benchRunModule(tgxbench::getPrimitivesBenchModule());
#elif defined(TGX_BENCH_MODULE_LIGHTING_SHADING)
    (void)tgxbench::benchRunModule(tgxbench::getLightingShadingBenchModule());
#elif defined(TGX_BENCH_MODULE_TEXTURE_RASTER)
    (void)tgxbench::benchRunModule(tgxbench::getTextureRasterBenchModule());
#else
    (void)tgxbench::benchRunModule(tgxbench::getCoreRasterBenchModule());
#endif

    while (true)
        {
        tgxbench::benchDelayMs(1000);
        }
}
