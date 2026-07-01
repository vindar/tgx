#include "../boards/cpu/board_config.h"

#include "../common/tgx_bench.h"

#if defined(TGX_BENCH_MODULE_MIXED_STRESS)
#include "../modules/mixed_stress/bench_mixed_stress.h"
#elif defined(TGX_BENCH_MODULE_INTEGRATED)
#include "../modules/integrated/bench_integrated.h"
#elif defined(TGX_BENCH_MODULE_SKYBOX_NOZ)
#include "../modules/skybox_noz/bench_skybox_noz.h"
#elif defined(TGX_BENCH_MODULE_WIRE_POINTS)
#include "../modules/wire_points/bench_wire_points.h"
#elif defined(TGX_BENCH_MODULE_MESH)
#include "../modules/mesh/bench_mesh.h"
#elif defined(TGX_BENCH_MODULE_PRIMITIVES)
#include "../modules/primitives/bench_primitives.h"
#elif defined(TGX_BENCH_MODULE_LIGHTING_SHADING)
#include "../modules/lighting_shading/bench_lighting_shading.h"
#elif defined(TGX_BENCH_MODULE_TEXTURE_RASTER)
#include "../modules/texture_raster/bench_texture_raster.h"
#else
#include "../modules/core_raster/bench_core_raster.h"
#endif

#if defined(_WIN32)
#define cimg_display 2
#else
#define cimg_display 0
#endif
#include "CImg.h"

#include <cstdio>
#include <cstring>
#include <string>

#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include "../common/tgx_bench.cpp"
#include "../boards/cpu/board_backend.cpp"

#if defined(TGX_BENCH_MODULE_MIXED_STRESS)
#include "../modules/mixed_stress/bench_mixed_stress.cpp"
#elif defined(TGX_BENCH_MODULE_INTEGRATED)
#include "../modules/integrated/bench_integrated.cpp"
#elif defined(TGX_BENCH_MODULE_SKYBOX_NOZ)
#include "../modules/skybox_noz/bench_skybox_noz.cpp"
#elif defined(TGX_BENCH_MODULE_WIRE_POINTS)
#include "../modules/wire_points/bench_wire_points.cpp"
#elif defined(TGX_BENCH_MODULE_MESH)
#include "../modules/mesh/bench_mesh.cpp"
#elif defined(TGX_BENCH_MODULE_PRIMITIVES)
#include "../modules/primitives/bench_primitives.cpp"
#elif defined(TGX_BENCH_MODULE_LIGHTING_SHADING)
#include "../modules/lighting_shading/bench_lighting_shading.cpp"
#elif defined(TGX_BENCH_MODULE_TEXTURE_RASTER)
#include "../modules/texture_raster/bench_texture_raster.cpp"
#else
#include "../modules/core_raster/bench_core_raster.cpp"
#endif

namespace
{

using namespace tgxbench;
using cimg_library::CImg;

constexpr int MOSAIC_COLS = 4;
constexpr int MOSAIC_ROWS = 5;
constexpr int MOSAIC_FRAMES = MOSAIC_COLS * MOSAIC_ROWS;
constexpr int TILE_BORDER = 8;
constexpr int LABEL_HEIGHT = 32;
constexpr uint32_t VISUAL_FRAME_CAP_MS = 16;

uint32_t visualFrameForMosaicSample(uint32_t sample_index)
{
    return (uint32_t)(((uint64_t)sample_index * (uint64_t)TGX_BENCH_VISUAL_FRAME_COUNT) / (uint64_t)MOSAIC_FRAMES);
}

void copyFramebufferToTile(const BenchContext& ctx, CImg<unsigned char>& mosaic, int tile_x, int tile_y)
{
    const int image_x = tile_x + TILE_BORDER;
    const int image_y = tile_y + TILE_BORDER + LABEL_HEIGHT;
    for (int y = 0; y < ctx.height; ++y)
        {
        for (int x = 0; x < ctx.width; ++x)
            {
            const tgx::RGB565 c = ctx.framebuffer[x + y * ctx.width];
            mosaic(image_x + x, image_y + y, 0, 0) = (unsigned char)((int)c.R * 255 / 31);
            mosaic(image_x + x, image_y + y, 0, 1) = (unsigned char)((int)c.G * 255 / 63);
            mosaic(image_x + x, image_y + y, 0, 2) = (unsigned char)((int)c.B * 255 / 31);
            }
        }
}

bool renderVisualTest(BenchContext& ctx, const BenchTest& test, uint32_t frame_index, uint64_t& checksum)
{
    if ((test.setup) && (!test.setup(ctx))) return false;
    benchClearTarget(ctx);
    if (test.render) test.render(ctx, frame_index);
    checksum = benchFramebufferChecksum(ctx);
    if (test.teardown) test.teardown(ctx);
    return true;
}

void drawFrameTile(BenchContext& ctx, const BenchTest& test, uint32_t frame_index,
                   CImg<unsigned char>& image, int tile_x, int tile_y,
                   int tile_w, int tile_h, bool show_test_id)
{
    const unsigned char tile_bg[] = { 46, 48, 52 };
    const unsigned char border[] = { 95, 102, 112 };
    const unsigned char text[] = { 235, 238, 242 };
    const unsigned char fail[] = { 255, 96, 96 };

    image.draw_rectangle(tile_x, tile_y, tile_x + tile_w - 1, tile_y + tile_h - 1, tile_bg);
    image.draw_rectangle(tile_x, tile_y, tile_x + tile_w - 1, tile_y + tile_h - 1, border, 1.0f, ~0U);

    uint64_t checksum = 0;
    const bool ok = renderVisualTest(ctx, test, frame_index, checksum);

    char line1[192];
    char line2[192];
    if (show_test_id)
        {
        std::snprintf(line1, sizeof(line1), "%s", test.id);
        }
    else
        {
        std::snprintf(line1, sizeof(line1), "frame %lu", (unsigned long)(frame_index + 1));
        }
    std::snprintf(line2, sizeof(line2), ok ? "frame=%lu checksum=%08lx%08lx" : "render failed",
                  (unsigned long)(frame_index + 1),
                  (unsigned long)(checksum >> 32), (unsigned long)(checksum & 0xffffffffUL));

    image.draw_text(tile_x + TILE_BORDER, tile_y + 6, line1, ok ? text : fail, tile_bg, 1.0f, 12);
    image.draw_text(tile_x + TILE_BORDER, tile_y + 19, line2, ok ? text : fail, tile_bg, 1.0f, 11);

    if (ok) copyFramebufferToTile(ctx, image, tile_x, tile_y);
}

void makeDirectory(const char* path)
{
    if ((!path) || (!path[0])) return;
#if defined(_WIN32)
    _mkdir(path);
#else
    mkdir(path, 0777);
#endif
}

void createParentDirectories(const char* path)
{
    if (!path) return;
    std::string current;
    for (const char* p = path; *p; ++p)
        {
        const char c = *p;
        if ((c == '/') || (c == '\\'))
            {
            if (!current.empty()) makeDirectory(current.c_str());
            current.push_back(c);
            }
        else
            {
            current.push_back(c);
            }
    }
}

std::string sanitizeFilePart(const char* text)
{
    std::string out;
    for (const char* p = text; p && *p; ++p)
        {
        const char c = *p;
        const bool ok = ((c >= 'a') && (c <= 'z')) ||
                        ((c >= 'A') && (c <= 'Z')) ||
                        ((c >= '0') && (c <= '9'));
        out.push_back(ok ? c : '_');
        }
    return out;
}

std::string mosaicPathForTest(const char* base_path, const BenchTest& test)
{
    const std::string base = base_path ? base_path : "tmp/benchmark3d/bench3d_mosaic.bmp";
    const size_t slash = base.find_last_of("/\\");
    const size_t dot = base.find_last_of('.');
    const bool has_dot = (dot != std::string::npos) && ((slash == std::string::npos) || (dot > slash));

    const std::string dir = (slash == std::string::npos) ? std::string() : base.substr(0, slash + 1);
    const std::string stem = has_dot ? base.substr(slash == std::string::npos ? 0 : slash + 1,
                                                   dot - (slash == std::string::npos ? 0 : slash + 1))
                                     : base.substr(slash == std::string::npos ? 0 : slash + 1);
    const std::string ext = has_dot ? base.substr(dot) : ".bmp";
    return dir + stem + "_" + sanitizeFilePart(test.id) + ext;
}

void saveTestMosaic(BenchContext& ctx, const BenchTest& test, const char* base_out_path)
{
    const int tile_w = ctx.width + 2 * TILE_BORDER;
    const int tile_h = ctx.height + LABEL_HEIGHT + 2 * TILE_BORDER;
    CImg<unsigned char> mosaic(tile_w * MOSAIC_COLS, tile_h * MOSAIC_ROWS, 1, 3, 24);
    mosaic.fill(24);

    for (uint32_t frame = 0; frame < MOSAIC_FRAMES; ++frame)
        {
        const int col = (int)(frame % MOSAIC_COLS);
        const int row = (int)(frame / MOSAIC_COLS);
        drawFrameTile(ctx, test, visualFrameForMosaicSample(frame), mosaic, col * tile_w, row * tile_h, tile_w, tile_h, false);
        }

    const std::string path = mosaicPathForTest(base_out_path, test);
    createParentDirectories(path.c_str());
    mosaic.save(path.c_str());
    std::printf("TGXNB3D_VISUAL_MOSAIC,test=%s,frames=%d,cols=%d,rows=%d,width=%d,height=%d,path=%s\n",
                test.id, MOSAIC_FRAMES, MOSAIC_COLS, MOSAIC_ROWS, mosaic.width(), mosaic.height(), path.c_str());
}

bool displayAnimatedTest(BenchContext& ctx, const BenchTest& test, CImg<unsigned char>& frame_image, cimg_library::CImgDisplay& display)
{
    const int tile_w = ctx.width + 2 * TILE_BORDER;
    const int tile_h = ctx.height + LABEL_HEIGHT + 2 * TILE_BORDER;

    for (uint32_t frame = 0; frame < TGX_BENCH_VISUAL_FRAME_COUNT; ++frame)
        {
        if (display.is_closed()) return false;

        const uint32_t start_ms = benchMillis();
        frame_image.fill(24);
        drawFrameTile(ctx, test, frame, frame_image, 0, 0, tile_w, tile_h, true);
        display.set_title("TGX benchmark3d - %s - frame %lu/%lu", test.id,
                          (unsigned long)(frame + 1), (unsigned long)TGX_BENCH_VISUAL_FRAME_COUNT);
        display.display(frame_image);

        const uint32_t elapsed_ms = benchMillis() - start_ms;
        if (elapsed_ms < VISUAL_FRAME_CAP_MS)
            {
            display.wait(VISUAL_FRAME_CAP_MS - elapsed_ms);
            }
        else
            {
            display.wait(1);
            }
        }
    return !display.is_closed();
}

int runVisual(const BenchModule& module, const char* out_path, bool show_window)
{
    if (!benchBackendSetup()) return 1;
    benchResetAllocators();

    BenchContext ctx;
    if (!benchInitContext(ctx)) return 1;

    for (uint32_t i = 0; i < module.test_count; ++i)
        {
        saveTestMosaic(ctx, module.tests[i], out_path);
        }

    std::printf("TGXNB3D_VISUAL,module=%s,tests=%lu,mosaic_frames=%d,cols=%d,rows=%d\n",
                module.id, (unsigned long)module.test_count, MOSAIC_FRAMES, MOSAIC_COLS, MOSAIC_ROWS);

    if (show_window)
        {
#if cimg_display != 0
        const int tile_w = ctx.width + 2 * TILE_BORDER;
        const int tile_h = ctx.height + LABEL_HEIGHT + 2 * TILE_BORDER;
        CImg<unsigned char> frame_image(tile_w, tile_h, 1, 3, 24);
        cimg_library::CImgDisplay display(frame_image, "TGX benchmark3d");
        while (!display.is_closed())
            {
            for (uint32_t i = 0; i < module.test_count; ++i)
                {
                if (!displayAnimatedTest(ctx, module.tests[i], frame_image, display)) break;
                }
            }
#else
        std::printf("TGXNB3D_VISUAL_DISPLAY_UNAVAILABLE\n");
#endif
        }

    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    bool visual = false;
    bool show_window = false;
#if defined(TGX_BENCH_MODULE_MIXED_STRESS)
    const char* module_id = "mixed_stress";
#elif defined(TGX_BENCH_MODULE_INTEGRATED)
    const char* module_id = "integrated";
#elif defined(TGX_BENCH_MODULE_SKYBOX_NOZ)
    const char* module_id = "skybox_noz";
#elif defined(TGX_BENCH_MODULE_WIRE_POINTS)
    const char* module_id = "wire_points";
#elif defined(TGX_BENCH_MODULE_MESH)
    const char* module_id = "mesh";
#elif defined(TGX_BENCH_MODULE_TEXTURE_RASTER)
    const char* module_id = "texture_raster";
#elif defined(TGX_BENCH_MODULE_LIGHTING_SHADING)
    const char* module_id = "lighting_shading";
#elif defined(TGX_BENCH_MODULE_PRIMITIVES)
    const char* module_id = "primitives";
#else
    const char* module_id = "core_raster";
#endif
    const char* out_path = "tmp/benchmark3d/bench3d_mosaic.bmp";

    for (int i = 1; i < argc; ++i)
        {
        if ((std::strcmp(argv[i], "--module") == 0) && (i + 1 < argc))
            {
            module_id = argv[++i];
            }
        else if (std::strcmp(argv[i], "--visual") == 0)
            {
            visual = true;
            }
        else if ((std::strcmp(argv[i], "--show") == 0) || (std::strcmp(argv[i], "--display") == 0))
            {
            visual = true;
            show_window = true;
            }
        else if ((std::strcmp(argv[i], "--visual-out") == 0) && (i + 1 < argc))
            {
            visual = true;
            out_path = argv[++i];
            }
        }

    const tgxbench::BenchModule* module = nullptr;
#if defined(TGX_BENCH_MODULE_MIXED_STRESS)
    if (std::strcmp(module_id, "mixed_stress") == 0)
        {
        module = &tgxbench::getMixedStressBenchModule();
        }
#elif defined(TGX_BENCH_MODULE_INTEGRATED)
    if (std::strcmp(module_id, "integrated") == 0)
        {
        module = &tgxbench::getIntegratedBenchModule();
        }
#elif defined(TGX_BENCH_MODULE_SKYBOX_NOZ)
    if (std::strcmp(module_id, "skybox_noz") == 0)
        {
        module = &tgxbench::getSkyboxNoZBenchModule();
        }
#elif defined(TGX_BENCH_MODULE_WIRE_POINTS)
    if (std::strcmp(module_id, "wire_points") == 0)
        {
        module = &tgxbench::getWirePointsBenchModule();
        }
#elif defined(TGX_BENCH_MODULE_MESH)
    if (std::strcmp(module_id, "mesh") == 0)
        {
        module = &tgxbench::getMeshBenchModule();
        }
#elif defined(TGX_BENCH_MODULE_PRIMITIVES)
    if (std::strcmp(module_id, "primitives") == 0)
        {
        module = &tgxbench::getPrimitivesBenchModule();
        }
#elif defined(TGX_BENCH_MODULE_LIGHTING_SHADING)
    if (std::strcmp(module_id, "lighting_shading") == 0)
        {
        module = &tgxbench::getLightingShadingBenchModule();
        }
#elif defined(TGX_BENCH_MODULE_TEXTURE_RASTER)
    if (std::strcmp(module_id, "texture_raster") == 0)
        {
        module = &tgxbench::getTextureRasterBenchModule();
        }
#else
    if (std::strcmp(module_id, "core_raster") == 0)
        {
        module = &tgxbench::getCoreRasterBenchModule();
        }
#endif
    else
        {
        std::fprintf(stderr, "module %s was not compiled into this benchmark binary\n", module_id);
        return 2;
        }

    if (visual) return runVisual(*module, out_path, show_window);
    return tgxbench::benchRunModule(*module);
}
