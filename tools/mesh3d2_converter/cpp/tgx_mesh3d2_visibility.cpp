/*
 * Offline TGX visibility helper for the Mesh3D2 converter.
 *
 * The Python converter writes a compact binary scene, this program renders it
 * from many orthographic directions with TGX itself and writes a view/meshlet
 * visibility matrix. It intentionally avoids any dependency outside TGX/STL.
 */

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "tgx.h"

namespace
{

constexpr std::array<char, 8> INPUT_MAGIC = {'T', 'G', 'V', 'I', 'S', '1', '\0', '\0'};
constexpr std::array<char, 8> OUTPUT_MAGIC = {'T', 'G', 'V', 'O', 'U', '1', '\0', '\0'};
constexpr int COLOR_BASE = 32;
constexpr int MAX_COLORS = COLOR_BASE * COLOR_BASE * COLOR_BASE - 1;

struct Triangle
{
    uint32_t v[3];
    uint32_t meshlet;
};

struct Scene
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t nb_views = 0;
    uint32_t nb_vertices = 0;
    uint32_t nb_triangles = 0;
    uint32_t nb_meshlets = 0;
    std::vector<tgx::fVec3> views;
    std::vector<tgx::fVec3> vertices;
    std::vector<Triangle> triangles;
};

template<typename T>
void read_value(std::istream& in, T& value)
{
    in.read(reinterpret_cast<char*>(&value), sizeof(T));
    if (!in) throw std::runtime_error("unexpected end of input file");
}

void read_exact(std::istream& in, char* dst, std::streamsize size)
{
    in.read(dst, size);
    if (!in) throw std::runtime_error("unexpected end of input file");
}

template<typename T>
void write_value(std::ostream& out, const T& value)
{
    out.write(reinterpret_cast<const char*>(&value), sizeof(T));
    if (!out) throw std::runtime_error("could not write output file");
}

tgx::RGBf color_from_index(uint32_t index)
{
    if (index >= static_cast<uint32_t>(MAX_COLORS)) throw std::runtime_error("too many meshlets for RGB32 color indexing");
    const int j = static_cast<int>(index) + 1;
    const float r = static_cast<float>(j % COLOR_BASE) / COLOR_BASE;
    const float g = static_cast<float>((j / COLOR_BASE) % COLOR_BASE) / COLOR_BASE;
    const float b = static_cast<float>((COLOR_BASE - 1) - ((j / (COLOR_BASE * COLOR_BASE)) % COLOR_BASE)) / COLOR_BASE;
    return tgx::RGBf(r, g, b);
}

tgx::fVec3 orthogonal(tgx::fVec3 v)
{
    const float ax = std::abs(v.x);
    const float ay = std::abs(v.y);
    const float az = std::abs(v.z);
    tgx::fVec3 r;
    if ((ax <= ay) && (ax <= az))
        r = tgx::fVec3(0.0f, -v.z, v.y);
    else if (ay <= az)
        r = tgx::fVec3(-v.z, 0.0f, v.x);
    else
        r = tgx::fVec3(-v.y, v.x, 0.0f);
    r.normalize();
    return r;
}

Scene read_scene(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("could not open input file: " + path);

    std::array<char, 8> magic{};
    read_exact(in, magic.data(), static_cast<std::streamsize>(magic.size()));
    if (magic != INPUT_MAGIC) throw std::runtime_error("invalid visibility input magic");

    Scene scene;
    read_value(in, scene.width);
    read_value(in, scene.height);
    read_value(in, scene.nb_views);
    read_value(in, scene.nb_vertices);
    read_value(in, scene.nb_triangles);
    read_value(in, scene.nb_meshlets);
    if ((scene.width == 0) || (scene.height == 0) || (scene.nb_views == 0) || (scene.nb_vertices == 0))
        throw std::runtime_error("invalid empty visibility scene");
    if (scene.nb_meshlets > static_cast<uint32_t>(MAX_COLORS))
        throw std::runtime_error("too many meshlets for color-indexed visibility rendering");

    scene.views.resize(scene.nb_views);
    scene.vertices.resize(scene.nb_vertices);
    scene.triangles.resize(scene.nb_triangles);

    for (auto& v : scene.views)
        {
        read_value(in, v.x);
        read_value(in, v.y);
        read_value(in, v.z);
        }
    for (auto& v : scene.vertices)
        {
        read_value(in, v.x);
        read_value(in, v.y);
        read_value(in, v.z);
        }
    for (auto& t : scene.triangles)
        {
        read_value(in, t.v[0]);
        read_value(in, t.v[1]);
        read_value(in, t.v[2]);
        read_value(in, t.meshlet);
        if ((t.v[0] >= scene.nb_vertices) || (t.v[1] >= scene.nb_vertices) || (t.v[2] >= scene.nb_vertices))
            throw std::runtime_error("triangle references a missing vertex");
        if (t.meshlet >= scene.nb_meshlets)
            throw std::runtime_error("triangle references a missing meshlet");
        }
    return scene;
}

std::unordered_map<uint32_t, int> build_color_map(uint32_t nb_meshlets)
{
    std::vector<float> zbuf(25);
    std::vector<tgx::RGB32> buffer(25);
    tgx::Image<tgx::RGB32> image(buffer.data(), 5, 5);
    tgx::Renderer3D<tgx::RGB32> renderer;
    renderer.setViewportSize(5, 5);
    renderer.setOffset(0, 0);
    renderer.setImage(&image);
    renderer.setZbuffer(zbuf.data());
    renderer.setShaders(tgx::SHADER_FLAT);
    renderer.setOrtho(-1.0f, 1.0f, -1.0f, 1.0f, 0.05f, 10.0f);
    renderer.setLookAt({0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
    renderer.setModelPosScaleRot({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 0.0f);

    std::unordered_map<uint32_t, int> map;
    image.clear(tgx::RGB32_Black);
    map.emplace(static_cast<const uint32_t&>(image(2, 2)), -1);
    for (uint32_t i = 0; i < nb_meshlets; ++i)
        {
        image.clear(tgx::RGB32_Black);
        renderer.clearZbuffer();
        renderer.setMaterial(color_from_index(i), 1.0f, 0.0f, 0.0f, 1);
        renderer.drawQuad({-0.5f, -0.5f, 0.0f}, {0.5f, -0.5f, 0.0f}, {0.5f, 0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f});
        map.emplace(static_cast<const uint32_t&>(image(2, 2)), static_cast<int>(i));
        }
    return map;
}

std::vector<uint8_t> compute_visibility(const Scene& scene)
{
    std::vector<float> zbuf(static_cast<size_t>(scene.width) * scene.height);
    std::vector<tgx::RGB32> buffer(static_cast<size_t>(scene.width) * scene.height);
    tgx::Image<tgx::RGB32> image(buffer.data(), static_cast<int>(scene.width), static_cast<int>(scene.height));
    tgx::Renderer3D<tgx::RGB32> renderer;
    renderer.setViewportSize(static_cast<int>(scene.width), static_cast<int>(scene.height));
    renderer.setOffset(0, 0);
    renderer.setImage(&image);
    renderer.setZbuffer(zbuf.data());
    renderer.setShaders(tgx::SHADER_FLAT);
    renderer.setModelPosScaleRot({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 0.0f);

    const auto color_map = build_color_map(scene.nb_meshlets);
    std::vector<uint8_t> visibility(static_cast<size_t>(scene.nb_views) * scene.nb_meshlets, 0);

    for (uint32_t nv = 0; nv < scene.nb_views; ++nv)
        {
        image.clear(tgx::RGB32_Black);
        renderer.clearZbuffer();

        tgx::fVec3 oz = scene.views[nv];
        oz.normalize();
        const tgx::fVec3 oy = orthogonal(oz);
        const tgx::fVec3 ox = tgx::crossProduct(oy, oz).getNormalize();

        float minx = tgx::dotProduct(scene.vertices[0], -ox);
        float maxx = minx;
        float miny = tgx::dotProduct(scene.vertices[0], -oy);
        float maxy = miny;
        float minz = tgx::dotProduct(scene.vertices[0], oz);
        float maxz = minz;
        for (const auto& v : scene.vertices)
            {
            const float x = tgx::dotProduct(v, -ox);
            const float y = tgx::dotProduct(v, -oy);
            const float z = tgx::dotProduct(v, oz);
            minx = std::min(minx, x);
            maxx = std::max(maxx, x);
            miny = std::min(miny, y);
            maxy = std::max(maxy, y);
            minz = std::min(minz, z);
            maxz = std::max(maxz, z);
            }

        const float cx = (minx + maxx) * 0.5f;
        const float cy = (miny + maxy) * 0.5f;
        const float r = 1.1f * std::max((maxx - minx) * 0.5f, (maxy - miny) * 0.5f);
        const float lz = std::max(1.0e-5f, 2.0f * (maxz - minz));
        renderer.setOrtho(cx - r, cx + r, cy - r, cy + r, lz * 0.1f, lz);
        renderer.setLookAt(oz * (minz - lz * 0.25f), {0.0f, 0.0f, 0.0f}, oy);

        uint32_t current_meshlet = UINT32_MAX;
        for (const auto& tri : scene.triangles)
            {
            if (tri.meshlet != current_meshlet)
                {
                current_meshlet = tri.meshlet;
                renderer.setMaterial(color_from_index(current_meshlet), 1.0f, 0.0f, 0.0f, 1);
                }
            renderer.drawTriangle(scene.vertices[tri.v[0]], scene.vertices[tri.v[1]], scene.vertices[tri.v[2]]);
            }

        uint8_t* row = visibility.data() + static_cast<size_t>(nv) * scene.nb_meshlets;
        for (const auto& pixel : buffer)
            {
            const uint32_t key = static_cast<const uint32_t&>(pixel);
            const auto it = color_map.find(key);
            if (it == color_map.end())
                throw std::runtime_error("rendered color is not in the meshlet color map");
            if (it->second >= 0) row[it->second] = 1;
            }
        }
    return visibility;
}

void write_visibility(const std::string& path, uint32_t nb_views, uint32_t nb_meshlets, const std::vector<uint8_t>& visibility)
{
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("could not open output file: " + path);
    out.write(OUTPUT_MAGIC.data(), static_cast<std::streamsize>(OUTPUT_MAGIC.size()));
    write_value(out, nb_views);
    write_value(out, nb_meshlets);
    out.write(reinterpret_cast<const char*>(visibility.data()), static_cast<std::streamsize>(visibility.size()));
    if (!out) throw std::runtime_error("could not write output visibility matrix");
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 3)
        {
        std::cerr << "usage: tgx_mesh3d2_visibility <input.bin> <output.bin>\n";
        return 2;
        }
    try
        {
        const Scene scene = read_scene(argv[1]);
        const auto visibility = compute_visibility(scene);
        write_visibility(argv[2], scene.nb_views, scene.nb_meshlets, visibility);
        std::cerr << "TGX visibility: " << scene.nb_views << " views, " << scene.nb_meshlets
                  << " meshlets, " << scene.width << "x" << scene.height << "\n";
        }
    catch (const std::exception& e)
        {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
        }
    return 0;
}

