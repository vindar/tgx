// CPU spotlight render comparator used by the 2026-06-26 specular optimization.

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <tgx.h>
#include "CImg.h"

namespace
{

using namespace tgx;
using namespace cimg_library;

constexpr int W = 240;
constexpr int H = 180;
constexpr int FLOOR_NX = 12;
constexpr int FLOOR_NY = 9;
constexpr int FLOOR_VERTICES = (FLOOR_NX + 1) * (FLOOR_NY + 1);
constexpr int FLOOR_INDICES = FLOOR_NX * FLOOR_NY * 6;

const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER |
                              SHADER_GOURAUD | SHADER_UNLIT |
                              SHADER_NOTEXTURE;

std::vector<RGB32> fb(W * H);
std::vector<float> zbuf(W * H);
Image<RGB32> im(fb.data(), W, H);
Renderer3D<RGB32, LOADED_SHADERS, float, 1, 2> renderer;

fVec3 floor_vertices[FLOOR_VERTICES];
uint16_t floor_indices[FLOOR_INDICES];
uint16_t floor_normal_indices[FLOOR_INDICES];
fVec3 floor_normals[1] = { { 0.0f, 1.0f, 0.0f } };

uint64_t fnv1a(const std::vector<RGB32>& pixels)
{
    uint64_t h = 1469598103934665603ULL;
    for (const RGB32& p : pixels)
        {
        h ^= p.R; h *= 1099511628211ULL;
        h ^= p.G; h *= 1099511628211ULL;
        h ^= p.B; h *= 1099511628211ULL;
        }
    return h;
}

void saveBmp(const std::filesystem::path& path)
{
    CImg<unsigned char> out(W, H, 1, 3, 0);
    for (int y = 0; y < H; y++)
        {
        for (int x = 0; x < W; x++)
            {
            const RGB32 p = fb[x + W * y];
            out(x, y, 0, 0) = p.R;
            out(x, y, 0, 1) = p.G;
            out(x, y, 0, 2) = p.B;
            }
        }
    out.save_bmp(path.string().c_str());
}

void buildFloor()
{
    int vi = 0;
    for (int y = 0; y <= FLOOR_NY; y++)
        {
        const float fy = -4.0f + 8.0f * ((float)y / (float)FLOOR_NY);
        for (int x = 0; x <= FLOOR_NX; x++)
            {
            const float fx = -4.4f + 8.8f * ((float)x / (float)FLOOR_NX);
            floor_vertices[vi++] = fVec3(fx, -1.05f, fy - 1.0f);
            }
        }

    int ii = 0;
    for (int y = 0; y < FLOOR_NY; y++)
        {
        for (int x = 0; x < FLOOR_NX; x++)
            {
            const uint16_t i0 = (uint16_t)(x + (FLOOR_NX + 1) * y);
            const uint16_t i1 = (uint16_t)(i0 + 1);
            const uint16_t i2 = (uint16_t)(i0 + (FLOOR_NX + 1));
            const uint16_t i3 = (uint16_t)(i2 + 1);
            floor_indices[ii++] = i0; floor_indices[ii++] = i2; floor_indices[ii++] = i1;
            floor_indices[ii++] = i1; floor_indices[ii++] = i2; floor_indices[ii++] = i3;
            for (int k = 0; k < 6; k++) floor_normal_indices[ii - 1 - k] = 0;
            }
        }
}

void setupRenderer()
{
    renderer.setViewportSize(W, H);
    renderer.setOffset(0, 0);
    renderer.setImage(&im);
    renderer.setZbuffer(zbuf.data());
    renderer.setCulling(0);
    renderer.setPerspective(48.0f, (float)W / (float)H, 0.1f, 80.0f);
    renderer.setLight({ -0.35f, -0.75f, -0.35f },
                      RGBf(0.008f, 0.010f, 0.015f),
                      RGBf(0.035f, 0.038f, 0.044f),
                      RGBf(0.0f, 0.0f, 0.0f));
    renderer.setSpotLightCount(2);
}

void drawFrame(float phase)
{
    const float a = phase * 6.28318530718f;
    renderer.setLookAt({ 7.2f * sinf(0.7f * a), 2.2f + 0.2f * sinf(1.3f * a), -0.8f + 7.2f * cosf(0.7f * a) },
                       { 0.0f, -0.12f, -1.0f },
                       { 0.0f, 1.0f, 0.0f });

    const fVec3 warm = { -2.8f + 5.2f * phase, 1.1f + 0.7f * sinf(a), 1.9f * cosf(1.7f * a) - 1.1f };
    const fVec3 cool = {  2.9f * cosf(1.2f * a), 1.5f + 0.55f * cosf(0.8f * a), -3.0f + 3.9f * phase };
    renderer.setSpotLight(0, warm, 5.7f, RGBf(3.15f, 0.76f, 0.30f), RGBf(1.45f, 0.62f, 0.30f));
    renderer.setSpotLight(1, cool, 5.5f, RGBf(0.28f, 1.02f, 3.25f), RGBf(0.30f, 0.70f, 1.85f));

    im.fillScreen(RGB32_Black);
    renderer.clearZbuffer();

    renderer.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_NOTEXTURE);
    renderer.setMaterial(RGBf(0.82f, 0.83f, 0.80f), 0.06f, 0.94f, 0.08f, 18);
    renderer.setModelPosScaleRot({ 0.0f, 0.0f, 0.0f });
    renderer.drawTriangles(FLOOR_INDICES / 3, floor_indices, floor_vertices, floor_normal_indices, floor_normals);

    renderer.setMaterial(RGBf(0.82f, 0.80f, 0.74f), 0.04f, 0.96f, 0.95f, 32);
    renderer.setModelPosScaleRot({ -1.45f, -0.45f, -1.05f }, { 0.82f, 0.82f, 0.82f });
    renderer.drawSphere(16, 9);

    renderer.setMaterial(RGBf(0.72f, 0.78f, 0.86f), 0.04f, 0.96f, 1.10f, 36);
    renderer.setModelPosScaleRot({ 1.40f, -0.55f, -1.85f }, { 0.70f, 0.70f, 0.70f });
    renderer.drawSphere(14, 8);

    renderer.setMaterial(RGBf(0.78f, 0.70f, 0.62f), 0.05f, 0.94f, 0.55f, 24);
    renderer.setModelPosScaleRot({ 0.10f, -0.55f, 0.45f }, { 0.58f, 0.68f, 0.58f }, 12.0f, { 0.0f, 1.0f, 0.0f });
    renderer.drawCube();
}

} // namespace

int main(int argc, char** argv)
{
    std::filesystem::path outDir = "tmp/spotlight_cpu_compare";
    std::string label = "current";
    for (int i = 1; i < argc; i++)
        {
        const std::string arg = argv[i];
        if ((arg == "--out") && (i + 1 < argc)) outDir = argv[++i];
        else if ((arg == "--label") && (i + 1 < argc)) label = argv[++i];
        }

    std::filesystem::create_directories(outDir);
    buildFloor();
    setupRenderer();

    std::ofstream csv(outDir / (label + "_hashes.csv"));
    csv << "label,frame,hash,path\n";
    const float phases[3] = { 0.12f, 0.46f, 0.83f };
    for (int frame = 0; frame < 3; frame++)
        {
        drawFrame(phases[frame]);
        const auto path = outDir / (label + "_frame" + std::to_string(frame) + ".bmp");
        saveBmp(path);
        const uint64_t hash = fnv1a(fb);
        csv << label << "," << frame << "," << hash << "," << path.string() << "\n";
        std::cout << label << " frame=" << frame << " hash=" << hash << " path=" << path.string() << "\n";
        }

    return 0;
}

