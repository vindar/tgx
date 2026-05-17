#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <tgx.h>

#include "all_meshes.h"

using namespace tgx;

struct ViewCase
    {
    const char* name;
    float rx;
    float ry;
    float rz;
    float scale;
    float z;
    };

struct CompareResult
    {
    std::string test;
    std::string model;
    std::string view;
    int different = 0;
    int max_channel_delta = 0;
    bool ok = true;
    };

template<typename T>
void writeU16(std::ofstream& out, T v)
    {
    const uint16_t x = static_cast<uint16_t>(v);
    out.put(static_cast<char>(x & 255));
    out.put(static_cast<char>((x >> 8) & 255));
    }

template<typename T>
void writeU32(std::ofstream& out, T v)
    {
    const uint32_t x = static_cast<uint32_t>(v);
    writeU16(out, x & 65535u);
    writeU16(out, (x >> 16) & 65535u);
    }

bool writeBMP(const std::filesystem::path& path, const std::vector<RGB565>& pixels, int w, int h)
    {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    const int row_bytes = ((3 * w + 3) / 4) * 4;
    const int pixel_bytes = row_bytes * h;
    out.put('B');
    out.put('M');
    writeU32(out, 54 + pixel_bytes);
    writeU16(out, 0);
    writeU16(out, 0);
    writeU32(out, 54);
    writeU32(out, 40);
    writeU32(out, w);
    writeU32(out, h);
    writeU16(out, 1);
    writeU16(out, 24);
    writeU32(out, 0);
    writeU32(out, pixel_bytes);
    writeU32(out, 2835);
    writeU32(out, 2835);
    writeU32(out, 0);
    writeU32(out, 0);

    std::vector<uint8_t> row(row_bytes, 0);
    for (int y = h - 1; y >= 0; --y)
        {
        std::fill(row.begin(), row.end(), 0);
        for (int x = 0; x < w; ++x)
            {
            const RGB32 c = RGB32(pixels[static_cast<size_t>(y) * w + x]);
            row[3 * x + 0] = static_cast<uint8_t>(c.B);
            row[3 * x + 1] = static_cast<uint8_t>(c.G);
            row[3 * x + 2] = static_cast<uint8_t>(c.R);
            }
        out.write(reinterpret_cast<const char*>(row.data()), row.size());
        }
    return bool(out);
    }

void setupRenderer(Renderer3D<RGB565>& renderer, Image<RGB565>& image, float* zbuf, int w, int h)
    {
    renderer.setViewportSize(w, h);
    renderer.setOffset(0, 0);
    renderer.setImage(&image);
    renderer.setZbuffer(zbuf);
    renderer.setPerspective(45.0f, static_cast<float>(w) / h, 1.0f, 100.0f);
    renderer.setCulling(1);
    renderer.setShaders(SHADER_FLAT | SHADER_ZBUFFER | SHADER_PERSPECTIVE);
    renderer.setMaterial(RGBf(0.9f, 0.75f, 0.55f), 1.0f, 0.0f, 0.0f, 1);
    renderer.setLightDirection({ 0.25f, -0.35f, -1.0f });
    }

void setView(Renderer3D<RGB565>& renderer, const ViewCase& view)
    {
    fMat4 M;
    M.setScale(view.scale, view.scale, view.scale);
    M.multRotate(view.rx, { 1.0f, 0.0f, 0.0f });
    M.multRotate(view.ry, { 0.0f, 1.0f, 0.0f });
    M.multRotate(view.rz, { 0.0f, 0.0f, 1.0f });
    M.multTranslate({ 0.0f, 0.0f, view.z });
    renderer.setModelMatrix(M);
    }

template<typename MeshT>
std::vector<RGB565> renderMesh(const MeshT* mesh, const ViewCase& view, int w, int h)
    {
    std::vector<RGB565> pixels(static_cast<size_t>(w) * h);
    std::vector<float> zbuf(static_cast<size_t>(w) * h);
    Image<RGB565> image(pixels.data(), w, h);
    Renderer3D<RGB565> renderer;
    setupRenderer(renderer, image, zbuf.data(), w, h);
    image.fillScreen(RGB565_Black);
    renderer.clearZbuffer();
    setView(renderer, view);
    renderer.drawMesh(mesh, false);
    return pixels;
    }

std::vector<RGB565> renderMesh3D2NoMeshletCull(const Mesh3D2<RGB565>* mesh, const ViewCase& view, int w, int h)
    {
    std::vector<Meshlet3D2> meshlets(mesh->meshlets, mesh->meshlets + mesh->nb_meshlets);
    for (auto& meshlet : meshlets) meshlet.cone_cos = -1.0f;

    Mesh3D2<RGB565> copy = *mesh;
    copy.meshlets = meshlets.data();
    return renderMesh(&copy, view, w, h);
    }

CompareResult compareImages(
    const std::string& test,
    const std::string& model,
    const ViewCase& view,
    const std::vector<RGB565>& legacy,
    const std::vector<RGB565>& mesh3d2,
    const std::filesystem::path& out_dir,
    int w,
    int h)
    {
    CompareResult result;
    result.test = test;
    result.model = model;
    result.view = view.name;
    for (size_t i = 0; i < legacy.size(); ++i)
        {
        if (legacy[i] == mesh3d2[i]) continue;
        result.different++;
        const RGB32 a = RGB32(legacy[i]);
        const RGB32 b = RGB32(mesh3d2[i]);
        result.max_channel_delta = std::max(result.max_channel_delta, std::abs(int(a.R) - int(b.R)));
        result.max_channel_delta = std::max(result.max_channel_delta, std::abs(int(a.G) - int(b.G)));
        result.max_channel_delta = std::max(result.max_channel_delta, std::abs(int(a.B) - int(b.B)));
        }
    result.ok = (result.different == 0);
    if (!result.ok)
        {
        const auto stem = test + "_" + model + "_" + view.name;
        writeBMP(out_dir / (stem + "_legacy.bmp"), legacy, w, h);
        writeBMP(out_dir / (stem + "_mesh3d2.bmp"), mesh3d2, w, h);
        std::vector<RGB565> diff(legacy.size(), RGB565_Black);
        for (size_t i = 0; i < legacy.size(); ++i)
            if (!(legacy[i] == mesh3d2[i])) diff[i] = RGB565_Red;
        writeBMP(out_dir / (stem + "_diff.bmp"), diff, w, h);
        }
    return result;
    }

template<typename LegacyT, typename Mesh2T>
void runModel(
    std::vector<CompareResult>& rows,
    const char* name,
    const LegacyT* legacy,
    const Mesh2T* mesh2,
    const std::vector<ViewCase>& views,
    const std::filesystem::path& out_dir)
    {
    constexpr int W = 320;
    constexpr int H = 240;
    for (const auto& view : views)
        {
        const auto a = renderMesh(legacy, view, W, H);
        const auto b = renderMesh3D2NoMeshletCull(mesh2, view, W, H);
        const auto c = renderMesh(mesh2, view, W, H);

        auto row = compareImages("legacy_vs_mesh3d2", name, view, a, b, out_dir, W, H);
        std::cout << row.test << "," << row.model << "," << row.view << ",diff=" << row.different
                  << ",max_delta=" << row.max_channel_delta << "\n";
        rows.push_back(row);

        row = compareImages("mesh3d2_culling", name, view, b, c, out_dir, W, H);
        std::cout << row.test << "," << row.model << "," << row.view << ",diff=" << row.different
                  << ",max_delta=" << row.max_channel_delta << "\n";
        rows.push_back(row);
        }
    }

void writeCSV(const std::filesystem::path& path, const std::vector<CompareResult>& rows)
    {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path);
    out << "test,model,view,ok,different_pixels,max_channel_delta\n";
    for (const auto& row : rows)
        out << row.test << "," << row.model << "," << row.view << "," << (row.ok ? "true" : "false") << ","
            << row.different << "," << row.max_channel_delta << "\n";
    }

int main(int argc, char** argv)
    {
    const std::filesystem::path out_dir = (argc > 1) ? argv[1] : "mesh3d2_validation_out";
    std::filesystem::create_directories(out_dir);

    const std::vector<ViewCase> views = {
        { "v00", -15.0f, 0.0f, 0.0f, 10.0f, -42.0f },
        { "v01", 22.0f, 35.0f, 5.0f, 10.0f, -42.0f },
        { "v02", -35.0f, 78.0f, -12.0f, 10.0f, -42.0f },
        { "v03", 12.0f, 145.0f, 20.0f, 10.0f, -42.0f },
        { "v04", 40.0f, 222.0f, -18.0f, 10.0f, -42.0f },
        { "v05", -28.0f, 310.0f, 9.0f, 10.0f, -42.0f },
    };

    std::vector<CompareResult> rows;
    runModel(rows, "bunny", &legacy_bunny, &m2_bunny, views, out_dir);
    runModel(rows, "dragon", &legacy_dragon, &m2_dragon, views, out_dir);
    runModel(rows, "suzanne", &legacy_suzanne, &m2_suzanne, views, out_dir);
    runModel(rows, "spot", &legacy_spot, &m2_spot, views, out_dir);

    writeCSV(out_dir / "mesh3d2_validation.csv", rows);
    const int culling_failures = static_cast<int>(std::count_if(rows.begin(), rows.end(), [](const auto& r) {
        return (r.test == "mesh3d2_culling") && !r.ok;
    }));
    const int legacy_failures = static_cast<int>(std::count_if(rows.begin(), rows.end(), [](const auto& r) {
        return (r.test == "legacy_vs_mesh3d2") && !r.ok;
    }));
    std::cout << "Mesh3D2 validation rows=" << rows.size()
              << ", culling_failures=" << culling_failures
              << ", legacy_reference_diffs=" << legacy_failures << "\n";
    return culling_failures == 0 ? 0 : 1;
    }
