// TGX 3D CPU validation suite.
//
// This executable renders a fixed set of 3D scenes with RGB565 and RGB32,
// writes BMP snapshots/golden images, records signatures, and can run a short
// CImg animated demo.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <tgx.h>
#include "CImg.h"

#ifndef PROGMEM
#define PROGMEM
#endif

#include "../models/buddha.h"
#include "../models/bunny_fig_small.h"
#include "../models/R2D2.h"
#include "../models/validation_buddha_v2.h"
#include "../models/validation_bunny_v2.h"
#include "../models/validation_R2D2_v2.h"
#include "../models/falcon/validation_falcon_v2.h"

namespace
{

using namespace tgx;
using namespace cimg_library;

constexpr int W = 800;
constexpr int H = 600;
constexpr int GRID_N = 18;
constexpr int STRIP_SEGMENTS = 160;
constexpr int POINT_COUNT = 320;
constexpr int BLOCK_X = 16;
constexpr int BLOCK_Y = 12;

const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ORTHO |
                              SHADER_ZBUFFER |
                              SHADER_GOURAUD | SHADER_FLAT | SHADER_UNLIT |
                              SHADER_NOTEXTURE | SHADER_TEXTURE |
                              SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_BILINEAR |
                              SHADER_TEXTURE_WRAP_POW2 | SHADER_TEXTURE_CLAMP;

enum class SceneId
    {
    BunnyGouraud,
    BunnyTextureNearest,
    BunnyTextureBilinear,
    BunnyMultiLightGouraud,
    BuddhaFlat,
    BunnyV2Gouraud,
    BunnyV2TextureNearest,
    BunnyV2TextureBilinear,
    BunnyV2MultiLightTexture,
    BuddhaV2Flat,
    BuddhaV2Unlit,
    R2D2TextureBilinear,
    R2D2V2TextureBilinear,
    FalconV2TextureNearest,
    FalconV2TextureUnlit,
    OrthoBunny,
    OrthoBunnyV2,
    NearClip,
    NearClipV2,
    FarClip,
    FarClipV2,
    ZBufferOverlap,
    LargeTriangles,
    SmallTriangleGrid,
    TriangleStripRibbon,
    WireTriangleStripFast,
    WireTriangleStripAA,
    WireTriangleStripThickness,
    DirectShapes,
    WirePoints,
    WirePointsV2,
    WireFast,
    WireAA,
    WireThickness05,
    WireThickness10,
    WireThickness20,
    WireV2Fast,
    WireV2AA,
    WireV2Thickness05,
    WireV2Thickness10,
    WireV2Thickness20,
    Count
    };

enum class WirePath
    {
    Fast,
    Antialiased,
    Thickness05,
    Thickness10,
    Thickness20
    };

struct SceneDef
    {
    SceneId id;
    const char* name;
    const char* note;
    };

const SceneDef SCENES[] = {
    { SceneId::BunnyGouraud,        "bunny_gouraud",         "real mesh, Gouraud lighting" },
    { SceneId::BunnyTextureNearest, "bunny_texture_nearest", "real mesh, perspective-correct nearest texture" },
    { SceneId::BunnyTextureBilinear,"bunny_texture_bilinear","real mesh, perspective-correct bilinear texture" },
    { SceneId::BunnyMultiLightGouraud,"bunny_multilight_gouraud","real mesh, advanced 4 directional lights" },
    { SceneId::BuddhaFlat,          "buddha_flat",           "dense real mesh, flat shading" },
    { SceneId::BunnyV2Gouraud,      "bunny_v2_gouraud",      "Mesh3Dv2 mesh, Gouraud lighting" },
    { SceneId::BunnyV2TextureNearest,"bunny_v2_texture_nearest","Mesh3Dv2 mesh, perspective-correct nearest texture" },
    { SceneId::BunnyV2TextureBilinear,"bunny_v2_texture_bilinear","Mesh3Dv2 mesh, perspective-correct bilinear texture" },
    { SceneId::BunnyV2MultiLightTexture,"bunny_v2_multilight_texture","Mesh3Dv2 textured mesh, advanced 4 directional lights" },
    { SceneId::BuddhaV2Flat,        "buddha_v2_flat",        "dense Mesh3Dv2 mesh, flat shading" },
    { SceneId::BuddhaV2Unlit,       "buddha_v2_unlit",       "dense Mesh3Dv2 mesh, unlit material color" },
    { SceneId::R2D2TextureBilinear,  "r2d2_texture_bilinear", "legacy Mesh3D textured model with bilinear filtering" },
    { SceneId::R2D2V2TextureBilinear,"r2d2_v2_texture_bilinear","textured Mesh3Dv2 model with bilinear filtering" },
    { SceneId::FalconV2TextureNearest,"falcon_v2_texture_nearest","multi-material textured Mesh3Dv2 model" },
    { SceneId::FalconV2TextureUnlit, "falcon_v2_texture_unlit","multi-material textured Mesh3Dv2 model, unlit texture" },
    { SceneId::OrthoBunny,          "ortho_bunny",           "orthographic projection" },
    { SceneId::OrthoBunnyV2,        "ortho_bunny_v2",        "Mesh3Dv2 orthographic projection" },
    { SceneId::NearClip,            "near_clip",             "near-plane clipping" },
    { SceneId::NearClipV2,          "near_clip_v2",          "Mesh3Dv2 near-plane clipping" },
    { SceneId::FarClip,             "far_clip",              "far-plane clipping" },
    { SceneId::FarClipV2,           "far_clip_v2",           "Mesh3Dv2 far-plane clipping" },
    { SceneId::ZBufferOverlap,      "zbuffer_overlap",       "overlapping direct geometry" },
    { SceneId::LargeTriangles,      "large_triangles",       "few large filled triangles" },
    { SceneId::SmallTriangleGrid,   "small_triangle_grid",   "many small textured triangles" },
    { SceneId::TriangleStripRibbon, "triangle_strip_ribbon", "textured dynamic geometry drawn as a triangle strip" },
    { SceneId::WireTriangleStripFast, "wire_triangle_strip_fast", "dynamic triangle strip, fast wireframe path" },
    { SceneId::WireTriangleStripAA, "wire_triangle_strip_aa", "dynamic triangle strip, optimized antialiased wireframe path" },
    { SceneId::WireTriangleStripThickness, "wire_triangle_strip_thickness", "dynamic triangle strip, adjustable thickness + AA path" },
    { SceneId::DirectShapes,        "direct_shapes",         "direct cube and sphere helpers" },
    { SceneId::WirePoints,          "wire_points",           "wireframe plus pixels/dots" },
    { SceneId::WirePointsV2,        "wire_points_v2",        "Mesh3Dv2 wireframe plus pixels/dots" },
    { SceneId::WireFast,            "wire_fast",             "legacy Mesh3D fast wireframe path" },
    { SceneId::WireAA,              "wire_aa",               "legacy Mesh3D optimized antialiased wireframe path" },
    { SceneId::WireThickness05,     "wire_thickness_05",     "legacy Mesh3D adjustable thickness + AA path, thickness 0.5" },
    { SceneId::WireThickness10,     "wire_thickness_10",     "legacy Mesh3D adjustable thickness + AA path, thickness 1.0" },
    { SceneId::WireThickness20,     "wire_thickness_20",     "legacy Mesh3D adjustable thickness + AA path, thickness 2.0" },
    { SceneId::WireV2Fast,          "wire_v2_fast",          "Mesh3Dv2 fast wireframe path" },
    { SceneId::WireV2AA,            "wire_v2_aa",            "Mesh3Dv2 optimized antialiased wireframe path" },
    { SceneId::WireV2Thickness05,   "wire_v2_thickness_05",  "Mesh3Dv2 adjustable thickness + AA path, thickness 0.5" },
    { SceneId::WireV2Thickness10,   "wire_v2_thickness_10",  "Mesh3Dv2 adjustable thickness + AA path, thickness 1.0" },
    { SceneId::WireV2Thickness20,   "wire_v2_thickness_20",  "Mesh3Dv2 adjustable thickness + AA path, thickness 2.0" }
};

struct Options
    {
    std::filesystem::path out_dir = "tmp/tgx_3d_cpu_suite";
    std::filesystem::path baseline_path;
    std::filesystem::path write_baseline_path;
    std::filesystem::path write_golden_dir;
    std::string compare_mode = "tolerant";
    bool demo = false;
    bool demo_only = false;
    };

struct Signature
    {
    std::string color_type;
    std::string scene;
    int w = W;
    int h = H;
    uint64_t hash = 0;
    uint64_t coarse_hash = 0;
    int active_pixels = 0;
    int min_x = 0;
    int min_y = 0;
    int max_x = 0;
    int max_y = 0;
    uint64_t sum_r = 0;
    uint64_t sum_g = 0;
    uint64_t sum_b = 0;
    bool ok = false;
    std::string note;
    };

struct Snapshot
    {
    std::string color_type;
    std::string scene;
    std::filesystem::path path;
    std::vector<RGB32> pixels;
    };

template<typename color_t>
color_t C(int r, int g, int b)
{
    return color_t(RGB32(uint8_t(r), uint8_t(g), uint8_t(b)));
}

template<typename color_t>
RGB32 toRGB32(color_t c)
{
    return RGB32(c);
}

template<typename color_t>
MeshMaterial3Dv2<color_t> makeV2Material(const MeshMaterial3Dv2<RGB565>& src, const Image<color_t>* texture)
{
    return {
        texture,
        src.color,
        src.ambiant_strength,
        src.diffuse_strength,
        src.specular_strength,
        src.specular_exponent
    };
}

template<typename color_t>
void copyTexture(const Image<RGB565>* src, std::vector<color_t>& pixels, Image<color_t>& dst)
{
    if (src == nullptr)
        {
        dst.set(static_cast<color_t*>(nullptr), 0, 0);
        return;
        }

    pixels.resize(size_t(src->lx()) * size_t(src->ly()));
    for (int y = 0; y < src->ly(); y++)
        {
        for (int x = 0; x < src->lx(); x++)
            {
            pixels[size_t(x) + size_t(y) * size_t(src->lx())] = color_t((*src)(x, y));
            }
        }
    dst.set(pixels.data(), src->lx(), src->ly());
}

template<typename color_t>
color_t backgroundColor()
{
    return C<color_t>(9, 12, 18);
}

uint64_t fnv1a(const uint8_t* data, size_t len, uint64_t h = 1469598103934665603ull)
{
    for (size_t i = 0; i < len; i++)
        {
        h ^= data[i];
        h *= 1099511628211ull;
        }
    return h;
}

void writeU16(std::ofstream& out, uint16_t v)
{
    out.put(char(v & 255));
    out.put(char((v >> 8) & 255));
}

void writeU32(std::ofstream& out, uint32_t v)
{
    writeU16(out, uint16_t(v & 65535));
    writeU16(out, uint16_t((v >> 16) & 65535));
}

template<typename color_t>
bool writeBMP(const std::filesystem::path& path, const Image<color_t>& im)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;

    const int row_stride = ((im.lx() * 3 + 3) / 4) * 4;
    const int pixel_bytes = row_stride * im.ly();
    const int file_bytes = 54 + pixel_bytes;

    out.put('B');
    out.put('M');
    writeU32(out, uint32_t(file_bytes));
    writeU16(out, 0);
    writeU16(out, 0);
    writeU32(out, 54);
    writeU32(out, 40);
    writeU32(out, uint32_t(im.lx()));
    writeU32(out, uint32_t(im.ly()));
    writeU16(out, 1);
    writeU16(out, 24);
    writeU32(out, 0);
    writeU32(out, uint32_t(pixel_bytes));
    writeU32(out, 2835);
    writeU32(out, 2835);
    writeU32(out, 0);
    writeU32(out, 0);

    std::vector<uint8_t> row(row_stride, 0);
    for (int y = im.ly() - 1; y >= 0; y--)
        {
        std::fill(row.begin(), row.end(), 0);
        for (int x = 0; x < im.lx(); x++)
            {
            const RGB32 c = toRGB32(im(x, y));
            row[3 * x + 0] = c.B;
            row[3 * x + 1] = c.G;
            row[3 * x + 2] = c.R;
            }
        out.write(reinterpret_cast<const char*>(row.data()), row.size());
        }
    return bool(out);
}

bool writeBMP32Pixels(const std::filesystem::path& path, int w, int h, const std::vector<RGB32>& pixels)
{
    std::vector<RGB32> copy = pixels;
    Image<RGB32> im(copy.data(), w, h);
    return writeBMP(path, im);
}

std::vector<std::string> splitCsvLine(const std::string& s)
{
    std::vector<std::string> out;
    std::string cur;
    for (char c : s)
        {
        if (c == ',')
            {
            out.push_back(cur);
            cur.clear();
            }
        else
            {
            cur.push_back(c);
            }
        }
    out.push_back(cur);
    return out;
}

std::string keyFor(const std::string& color_type, const std::string& scene)
{
    return color_type + ":" + scene;
}

std::map<std::string, Signature> readBaseline(const std::filesystem::path& path)
{
    std::map<std::string, Signature> baseline;
    std::ifstream in(path);
    if (!in) return baseline;

    std::string line;
    std::getline(in, line);
    while (std::getline(in, line))
        {
        if (line.empty()) continue;
        const auto f = splitCsvLine(line);
        if (f.size() < 15) continue;
        Signature s;
        s.color_type = f[1];
        s.scene = f[2];
        s.w = std::stoi(f[3]);
        s.h = std::stoi(f[4]);
        s.hash = std::stoull(f[5]);
        s.coarse_hash = std::stoull(f[6]);
        s.active_pixels = std::stoi(f[7]);
        s.min_x = std::stoi(f[8]);
        s.min_y = std::stoi(f[9]);
        s.max_x = std::stoi(f[10]);
        s.max_y = std::stoi(f[11]);
        s.sum_r = std::stoull(f[12]);
        s.sum_g = std::stoull(f[13]);
        s.sum_b = std::stoull(f[14]);
        baseline[keyFor(s.color_type, s.scene)] = s;
        }
    return baseline;
}

void writeBaseline(const std::filesystem::path& path, const std::vector<Signature>& rows)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path);
    out << "schema_version,color_type,scene,w,h,hash,coarse_hash,active_pixels,min_x,min_y,max_x,max_y,sum_r,sum_g,sum_b\n";
    for (const auto& s : rows)
        {
        out << "1," << s.color_type << "," << s.scene << ","
            << s.w << "," << s.h << ","
            << s.hash << "," << s.coarse_hash << ","
            << s.active_pixels << ","
            << s.min_x << "," << s.min_y << "," << s.max_x << "," << s.max_y << ","
            << s.sum_r << "," << s.sum_g << "," << s.sum_b << "\n";
        }
}

void writeResultsCsv(const std::filesystem::path& path, const std::vector<Signature>& rows)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path);
    out << "schema_version,platform,color_type,scene,w,h,hash,coarse_hash,active_pixels,min_x,min_y,max_x,max_y,sum_r,sum_g,sum_b,ok,note\n";
    for (const auto& s : rows)
        {
        out << "1,cpu," << s.color_type << "," << s.scene << ","
            << s.w << "," << s.h << ","
            << s.hash << "," << s.coarse_hash << ","
            << s.active_pixels << ","
            << s.min_x << "," << s.min_y << "," << s.max_x << "," << s.max_y << ","
            << s.sum_r << "," << s.sum_g << "," << s.sum_b << ","
            << (s.ok ? "true" : "false") << "," << s.note << "\n";
        }
}

bool closeEnoughU64(uint64_t a, uint64_t b, double rel, uint64_t abs_margin)
{
    const uint64_t d = (a > b) ? (a - b) : (b - a);
    const uint64_t m = std::max<uint64_t>(abs_margin, uint64_t(std::max(a, b) * rel));
    return d <= m;
}

bool compareSignature(Signature& current, const Signature* base, const std::string& mode)
{
    if (mode == "smoke")
        {
        const bool ok = current.active_pixels > 500 &&
                        current.max_x > current.min_x &&
                        current.max_y > current.min_y &&
                        (current.sum_r + current.sum_g + current.sum_b) > 10000;
        current.note = ok ? "smoke ok" : "smoke failed";
        return ok;
        }

    if (!base)
        {
        current.note = "no baseline";
        return true;
        }

    if ((current.w != base->w) || (current.h != base->h))
        {
        current.note = "dimension mismatch";
        return false;
        }

    if (mode == "strict")
        {
        const bool ok = current.hash == base->hash &&
                        current.coarse_hash == base->coarse_hash &&
                        current.active_pixels == base->active_pixels &&
                        current.sum_r == base->sum_r &&
                        current.sum_g == base->sum_g &&
                        current.sum_b == base->sum_b;
        current.note = ok ? "strict ok" : "strict mismatch";
        return ok;
        }

    const int active_delta = std::abs(current.active_pixels - base->active_pixels);
    const int active_margin = std::max(80, base->active_pixels / 200);
    const bool bbox_ok = std::abs(current.min_x - base->min_x) <= 6 &&
                         std::abs(current.min_y - base->min_y) <= 6 &&
                         std::abs(current.max_x - base->max_x) <= 6 &&
                         std::abs(current.max_y - base->max_y) <= 6;
    const bool sums_ok = closeEnoughU64(current.sum_r, base->sum_r, 0.006, 12000) &&
                         closeEnoughU64(current.sum_g, base->sum_g, 0.006, 12000) &&
                         closeEnoughU64(current.sum_b, base->sum_b, 0.006, 12000);
    const bool coarse_ok = current.coarse_hash == base->coarse_hash;
    const bool ok = active_delta <= active_margin &&
                    bbox_ok &&
                    sums_ok;

    std::ostringstream oss;
    oss << (ok ? "tolerant ok" : "tolerant mismatch")
        << " active_delta=" << active_delta
        << " coarse=" << (coarse_ok ? "ok" : "changed");
    current.note = oss.str();
    return ok;
}

template<typename color_t>
struct MeshAssets
    {
    std::vector<color_t> bunny_texture_pixels;
    std::vector<color_t> r2d2_texture_pixels;
    std::vector<std::vector<color_t>> falcon_texture_pixels;
    Image<color_t> bunny_texture;
    Image<color_t> r2d2_texture;
    std::vector<Image<color_t>> falcon_textures;
    Mesh3D<color_t> bunny;
    Mesh3D<color_t> buddha_mesh;
    Mesh3D<color_t> r2d2_mesh;
    std::vector<MeshMaterial3Dv2<color_t>> bunny_v2_materials;
    std::vector<MeshMaterial3Dv2<color_t>> buddha_v2_materials;
    std::vector<MeshMaterial3Dv2<color_t>> r2d2_v2_materials;
    std::vector<MeshMaterial3Dv2<color_t>> falcon_v2_materials;
    Mesh3Dv2<color_t> bunny_v2;
    Mesh3Dv2<color_t> buddha_v2;
    Mesh3Dv2<color_t> r2d2_v2;
    Mesh3Dv2<color_t> falcon_v2;

    MeshAssets()
        {
        copyTexture(&bunny_fig_texture, bunny_texture_pixels, bunny_texture);
        copyTexture(&R2D2_texture, r2d2_texture_pixels, r2d2_texture);

        bunny = {
            1,
            bunny_fig_small.nb_vertices,
            bunny_fig_small.nb_texcoords,
            bunny_fig_small.nb_normals,
            bunny_fig_small.nb_faces,
            bunny_fig_small.len_face,
            bunny_fig_small.vertice,
            bunny_fig_small.texcoord,
            bunny_fig_small.normal,
            bunny_fig_small.face,
            &bunny_texture,
            bunny_fig_small.color,
            bunny_fig_small.ambiant_strength,
            bunny_fig_small.diffuse_strength,
            bunny_fig_small.specular_strength,
            bunny_fig_small.specular_exponent,
            nullptr,
            bunny_fig_small.bounding_box,
            "bunny_fig_small"
        };

        buddha_mesh = {
            1,
            buddha.nb_vertices,
            0,
            buddha.nb_normals,
            buddha.nb_faces,
            buddha.len_face,
            buddha.vertice,
            nullptr,
            buddha.normal,
            buddha.face,
            nullptr,
            buddha.color,
            buddha.ambiant_strength,
            buddha.diffuse_strength,
            buddha.specular_strength,
            buddha.specular_exponent,
            nullptr,
            buddha.bounding_box,
            "buddha"
        };

        r2d2_mesh = {
            1,
            R2D2.nb_vertices,
            R2D2.nb_texcoords,
            R2D2.nb_normals,
            R2D2.nb_faces,
            R2D2.len_face,
            R2D2.vertice,
            R2D2.texcoord,
            R2D2.normal,
            R2D2.face,
            &r2d2_texture,
            R2D2.color,
            R2D2.ambiant_strength,
            R2D2.diffuse_strength,
            R2D2.specular_strength,
            R2D2.specular_exponent,
            nullptr,
            R2D2.bounding_box,
            "R2D2"
        };

        bunny_v2_materials.reserve(validation_bunny_v2.nb_materials);
        for (int i = 0; i < validation_bunny_v2.nb_materials; i++)
            {
            bunny_v2_materials.push_back(makeV2Material(validation_bunny_v2_materials[i],
                                                        validation_bunny_v2_materials[i].texture ? &bunny_texture : nullptr));
            }
        bunny_v2 = {
            validation_bunny_v2.id,
            validation_bunny_v2.nb_meshlets,
            validation_bunny_v2.nb_materials,
            bunny_v2_materials.data(),
            validation_bunny_v2.meshlets,
            validation_bunny_v2.payload,
            validation_bunny_v2.bounding_box,
            "validation_bunny_v2"
        };

        buddha_v2_materials.reserve(validation_buddha_v2.nb_materials);
        for (int i = 0; i < validation_buddha_v2.nb_materials; i++)
            {
            buddha_v2_materials.push_back(makeV2Material(validation_buddha_v2_materials[i], static_cast<const Image<color_t>*>(nullptr)));
            }
        buddha_v2 = {
            validation_buddha_v2.id,
            validation_buddha_v2.nb_meshlets,
            validation_buddha_v2.nb_materials,
            buddha_v2_materials.data(),
            validation_buddha_v2.meshlets,
            validation_buddha_v2.payload,
            validation_buddha_v2.bounding_box,
            "validation_buddha_v2"
        };

        r2d2_v2_materials.reserve(validation_R2D2_v2.nb_materials);
        for (int i = 0; i < validation_R2D2_v2.nb_materials; i++)
            {
            r2d2_v2_materials.push_back(makeV2Material(validation_R2D2_v2_materials[i],
                                                       validation_R2D2_v2_materials[i].texture ? &r2d2_texture : nullptr));
            }
        r2d2_v2 = {
            validation_R2D2_v2.id,
            validation_R2D2_v2.nb_meshlets,
            validation_R2D2_v2.nb_materials,
            r2d2_v2_materials.data(),
            validation_R2D2_v2.meshlets,
            validation_R2D2_v2.payload,
            validation_R2D2_v2.bounding_box,
            "validation_R2D2_v2"
        };

        falcon_texture_pixels.resize(validation_falcon_v2.nb_materials);
        falcon_textures.resize(validation_falcon_v2.nb_materials);
        falcon_v2_materials.reserve(validation_falcon_v2.nb_materials);
        for (int i = 0; i < validation_falcon_v2.nb_materials; i++)
            {
            const auto* src_texture = validation_falcon_v2_materials[i].texture;
            const Image<color_t>* dst_texture = nullptr;
            if (src_texture != nullptr)
                {
                copyTexture(src_texture, falcon_texture_pixels[i], falcon_textures[i]);
                dst_texture = &falcon_textures[i];
                }
            falcon_v2_materials.push_back(makeV2Material(validation_falcon_v2_materials[i], dst_texture));
            }
        falcon_v2 = {
            validation_falcon_v2.id,
            validation_falcon_v2.nb_meshlets,
            validation_falcon_v2.nb_materials,
            falcon_v2_materials.data(),
            validation_falcon_v2.meshlets,
            validation_falcon_v2.payload,
            validation_falcon_v2.bounding_box,
            "validation_falcon_v2"
        };
        }
    };

template<typename color_t>
struct RenderContext
    {
    std::vector<color_t> pixels;
    std::vector<uint16_t> zbuf;
    Image<color_t> image;
    Renderer3D<color_t, LOADED_SHADERS, uint16_t> renderer;
    Renderer3D<color_t, LOADED_SHADERS, uint16_t, 4> multi_light_renderer;
    MeshAssets<color_t> meshes;
    std::vector<fVec3> grid_vertices;
    std::vector<fVec3> grid_normals;
    std::vector<fVec2> grid_texcoords;
    std::vector<uint16_t> grid_indices;
    std::vector<fVec3> strip_vertices;
    std::vector<fVec3> strip_normals;
    std::vector<fVec2> strip_texcoords;
    std::vector<uint16_t> strip_indices;
    std::vector<fVec3> point_positions;
    std::vector<int> point_colors_ind;
    std::vector<int> point_opacity_ind;
    std::vector<int> point_radius_ind;
    color_t point_colors[6];
    float point_opacities[3] = { 0.45f, 0.75f, 1.0f };
    int point_radii[3] = { 1, 2, 3 };

    RenderContext()
        : pixels(size_t(W) * size_t(H)),
          zbuf(size_t(W) * size_t(H)),
          image(pixels.data(), W, H)
        {
        renderer.setViewportSize(W, H);
        renderer.setOffset(0, 0);
        renderer.setImage(&image);
        renderer.setZbuffer(zbuf.data());
        renderer.setPerspective(45.0f, float(W) / H, 1.0f, 300.0f);
        renderer.setMaterial(RGBf(0.85f, 0.58f, 0.26f), 0.18f, 0.72f, 0.55f, 32);
        renderer.setLight({ -0.25f, -0.45f, -1.0f }, RGBf(0.18f, 0.20f, 0.25f), RGBf(0.85f, 0.82f, 0.72f), RGBf(0.55f, 0.60f, 0.70f));
        multi_light_renderer.setViewportSize(W, H);
        multi_light_renderer.setOffset(0, 0);
        multi_light_renderer.setImage(&image);
        multi_light_renderer.setZbuffer(zbuf.data());
        multi_light_renderer.setPerspective(45.0f, float(W) / H, 1.0f, 300.0f);
        multi_light_renderer.setMaterial(RGBf(0.85f, 0.58f, 0.26f), 0.18f, 0.72f, 0.55f, 32);

        point_colors[0] = C<color_t>(230, 60, 60);
        point_colors[1] = C<color_t>(60, 220, 120);
        point_colors[2] = C<color_t>(70, 130, 245);
        point_colors[3] = C<color_t>(245, 210, 70);
        point_colors[4] = C<color_t>(50, 220, 230);
        point_colors[5] = C<color_t>(230, 80, 200);

        buildGrid();
        buildStrip();
        buildPoints();
        }

    void clear()
        {
        image.fillScreen(backgroundColor<color_t>());
        renderer.clearZbuffer();
        renderer.setCulling(1);
        multi_light_renderer.setCulling(1);
        }

    void buildGrid()
        {
        const int nv = (GRID_N + 1) * (GRID_N + 1);
        grid_vertices.resize(nv);
        grid_normals.resize(nv);
        grid_texcoords.resize(nv);
        int v = 0;
        for (int y = 0; y <= GRID_N; y++)
            {
            const float fy = -1.0f + 2.0f * y / GRID_N;
            for (int x = 0; x <= GRID_N; x++)
                {
                const float fx = -1.0f + 2.0f * x / GRID_N;
                grid_vertices[v] = { fx, fy, 0.10f * std::sin((fx - fy) * 5.0f) };
                grid_normals[v] = { 0.0f, 0.0f, 1.0f };
                grid_texcoords[v] = { float(x) / GRID_N, float(y) / GRID_N };
                v++;
                }
            }

        grid_indices.reserve(GRID_N * GRID_N * 6);
        for (int y = 0; y < GRID_N; y++)
            {
            for (int x = 0; x < GRID_N; x++)
                {
                const uint16_t i00 = uint16_t(x + y * (GRID_N + 1));
                const uint16_t i10 = uint16_t((x + 1) + y * (GRID_N + 1));
                const uint16_t i11 = uint16_t((x + 1) + (y + 1) * (GRID_N + 1));
                const uint16_t i01 = uint16_t(x + (y + 1) * (GRID_N + 1));
                grid_indices.push_back(i00);
                grid_indices.push_back(i10);
                grid_indices.push_back(i11);
                grid_indices.push_back(i00);
                grid_indices.push_back(i11);
                grid_indices.push_back(i01);
                }
            }
        }

    void buildStrip()
        {
        const int nv = 2 * (STRIP_SEGMENTS + 1);
        strip_vertices.resize(nv);
        strip_normals.resize(nv);
        strip_texcoords.resize(nv);
        strip_indices.resize(nv);

        for (int i = 0; i <= STRIP_SEGMENTS; i++)
            {
            const float u = float(i) / STRIP_SEGMENTS;
            const float x = -1.8f + 3.6f * u;
            const float y = 0.28f * std::sin(6.2831853f * 2.25f * u);
            const float z = 0.10f * std::cos(6.2831853f * 3.0f * u);
            const float half_w = 0.20f + 0.05f * std::sin(6.2831853f * 5.0f * u);
            const int a = 2 * i;
            strip_vertices[a] = { x, y + half_w, z };
            strip_vertices[a + 1] = { x, y - half_w, z };
            strip_normals[a] = { 0.0f, 0.0f, 1.0f };
            strip_normals[a + 1] = { 0.0f, 0.0f, 1.0f };
            strip_texcoords[a] = { 4.0f * u, 0.0f };
            strip_texcoords[a + 1] = { 4.0f * u, 1.0f };
            strip_indices[a] = uint16_t(a);
            strip_indices[a + 1] = uint16_t(a + 1);
            }
        }

    void buildPoints()
        {
        point_positions.resize(POINT_COUNT);
        point_colors_ind.resize(POINT_COUNT);
        point_opacity_ind.resize(POINT_COUNT);
        point_radius_ind.resize(POINT_COUNT);
        for (int i = 0; i < POINT_COUNT; i++)
            {
            const float u = float(i) / POINT_COUNT;
            const float a = 6.2831853f * float((i * 37) % POINT_COUNT) / POINT_COUNT;
            const float y = -1.0f + 2.0f * u;
            const float r = std::sqrt(std::max(0.0f, 1.0f - y * y));
            point_positions[i] = { r * std::cos(a), y, r * std::sin(a) };
            point_colors_ind[i] = i % 6;
            point_opacity_ind[i] = i % 3;
            point_radius_ind[i] = i % 3;
            }
        }
    };

inline float wirePathThickness(WirePath path)
{
    switch (path)
        {
        case WirePath::Thickness05: return 0.5f;
        case WirePath::Thickness10: return 1.0f;
        case WirePath::Thickness20: return 2.0f;
        case WirePath::Fast:
        case WirePath::Antialiased:
            return 1.0f;
        }
    return 1.0f;
}

template<typename color_t>
void drawWirePathLine(Renderer3D<color_t, LOADED_SHADERS, uint16_t>& r, WirePath path,
                      const fVec3& a, const fVec3& b, color_t color)
{
    r.setMaterialColor(color);
    if (path == WirePath::Fast) r.drawWireFrameLine(a, b);
    else if (path == WirePath::Antialiased) r.drawWireFrameLineAA(a, b);
    else r.drawWireFrameLine(a, b, wirePathThickness(path), color, 0.88f);
}

template<typename color_t>
void drawWirePathLines(Renderer3D<color_t, LOADED_SHADERS, uint16_t>& r, WirePath path,
                       int n, const uint16_t* indices, const fVec3* vertices, color_t color)
{
    r.setMaterialColor(color);
    if (path == WirePath::Fast) r.drawWireFrameLines(n, indices, vertices);
    else if (path == WirePath::Antialiased) r.drawWireFrameLinesAA(n, indices, vertices);
    else r.drawWireFrameLines(n, indices, vertices, wirePathThickness(path), color, 0.88f);
}

template<typename color_t>
void drawWirePathTriangle(Renderer3D<color_t, LOADED_SHADERS, uint16_t>& r, WirePath path,
                          const fVec3& a, const fVec3& b, const fVec3& c, color_t color)
{
    r.setMaterialColor(color);
    if (path == WirePath::Fast) r.drawWireFrameTriangle(a, b, c);
    else if (path == WirePath::Antialiased) r.drawWireFrameTriangleAA(a, b, c);
    else r.drawWireFrameTriangle(a, b, c, wirePathThickness(path), color, 0.88f);
}

template<typename color_t>
void drawWirePathTriangles(Renderer3D<color_t, LOADED_SHADERS, uint16_t>& r, WirePath path,
                           int n, const uint16_t* indices, const fVec3* vertices, color_t color)
{
    r.setMaterialColor(color);
    if (path == WirePath::Fast) r.drawWireFrameTriangles(n, indices, vertices);
    else if (path == WirePath::Antialiased) r.drawWireFrameTrianglesAA(n, indices, vertices);
    else r.drawWireFrameTriangles(n, indices, vertices, wirePathThickness(path), color, 0.88f);
}

template<typename color_t>
void drawWirePathQuad(Renderer3D<color_t, LOADED_SHADERS, uint16_t>& r, WirePath path,
                      const fVec3& a, const fVec3& b, const fVec3& c, const fVec3& d, color_t color)
{
    r.setMaterialColor(color);
    if (path == WirePath::Fast) r.drawWireFrameQuad(a, b, c, d);
    else if (path == WirePath::Antialiased) r.drawWireFrameQuadAA(a, b, c, d);
    else r.drawWireFrameQuad(a, b, c, d, wirePathThickness(path), color, 0.88f);
}

template<typename color_t>
void drawWirePathQuads(Renderer3D<color_t, LOADED_SHADERS, uint16_t>& r, WirePath path,
                       int n, const uint16_t* indices, const fVec3* vertices, color_t color)
{
    r.setMaterialColor(color);
    if (path == WirePath::Fast) r.drawWireFrameQuads(n, indices, vertices);
    else if (path == WirePath::Antialiased) r.drawWireFrameQuadsAA(n, indices, vertices);
    else r.drawWireFrameQuads(n, indices, vertices, wirePathThickness(path), color, 0.88f);
}

template<typename color_t>
void drawWirePathCube(Renderer3D<color_t, LOADED_SHADERS, uint16_t>& r, WirePath path, color_t color)
{
    r.setMaterialColor(color);
    if (path == WirePath::Fast) r.drawWireFrameCube();
    else if (path == WirePath::Antialiased) r.drawWireFrameCubeAA();
    else r.drawWireFrameCube(wirePathThickness(path), color, 0.88f);
}

template<typename color_t>
void drawWirePathSphere(Renderer3D<color_t, LOADED_SHADERS, uint16_t>& r, WirePath path, color_t color)
{
    r.setMaterialColor(color);
    if (path == WirePath::Fast) r.drawWireFrameSphere(9, 6);
    else if (path == WirePath::Antialiased) r.drawWireFrameSphereAA(9, 6);
    else r.drawWireFrameSphere(9, 6, wirePathThickness(path), color, 0.88f);
}

template<typename color_t>
void drawWirePathAdaptativeSphere(Renderer3D<color_t, LOADED_SHADERS, uint16_t>& r, WirePath path, color_t color)
{
    r.setMaterialColor(color);
    if (path == WirePath::Fast) r.drawWireFrameAdaptativeSphere(0.75f);
    else if (path == WirePath::Antialiased) r.drawWireFrameAdaptativeSphereAA(0.75f);
    else r.drawWireFrameAdaptativeSphere(0.75f, wirePathThickness(path), color, 0.88f);
}

template<typename color_t>
void drawWirePathMesh(RenderContext<color_t>& ctx, WirePath path, bool mesh_v2, color_t color)
{
    auto& r = ctx.renderer;
    r.setMaterialColor(color);
    if (mesh_v2)
        {
        if (path == WirePath::Fast) r.drawWireFrameMesh(&ctx.meshes.bunny_v2);
        else if (path == WirePath::Antialiased) r.drawWireFrameMeshAA(&ctx.meshes.bunny_v2);
        else r.drawWireFrameMesh(&ctx.meshes.bunny_v2, wirePathThickness(path), color, 0.88f);
        }
    else
        {
        if (path == WirePath::Fast) r.drawWireFrameMesh(&ctx.meshes.bunny, true);
        else if (path == WirePath::Antialiased) r.drawWireFrameMeshAA(&ctx.meshes.bunny, true);
        else r.drawWireFrameMesh(&ctx.meshes.bunny, true, wirePathThickness(path), color, 0.88f);
        }
}

template<typename color_t>
void renderWirePathScene(RenderContext<color_t>& ctx, WirePath path, bool mesh_v2)
{
    auto& r = ctx.renderer;
    const color_t mesh_color = C<color_t>(230, 232, 248);
    const color_t orange = C<color_t>(255, 178, 80);
    const color_t cyan = C<color_t>(72, 224, 255);
    const color_t lime = C<color_t>(205, 255, 95);
    const color_t pink = C<color_t>(255, 125, 195);
    const color_t violet = C<color_t>(168, 138, 255);
    const color_t green = C<color_t>(100, 255, 170);
    const color_t yellow = C<color_t>(255, 226, 95);

    const fVec3 line_vertices[] = {
        { -0.95f, -0.55f, 0.0f }, { -0.30f, 0.52f, 0.0f },
        { -0.30f, 0.52f, 0.0f }, {  0.42f, -0.48f, 0.0f },
        {  0.42f, -0.48f, 0.0f }, {  0.95f, 0.50f, 0.0f }
    };
    const uint16_t line_indices[] = { 0, 1, 2, 3, 4, 5 };
    const fVec3 triangle_vertices[] = {
        { -0.82f, -0.55f, 0.0f }, { 0.05f, 0.72f, 0.0f }, { 0.86f, -0.46f, 0.0f },
        { -0.66f,  0.40f, 0.0f }, { 0.12f, -0.70f, 0.0f }, { 0.78f,  0.34f, 0.0f }
    };
    const uint16_t triangle_indices[] = { 0, 1, 2, 3, 4, 5 };
    const fVec3 quad_vertices[] = {
        { -0.72f, -0.56f, 0.0f }, { 0.58f, -0.52f, 0.0f }, { 0.72f, 0.48f, 0.0f }, { -0.62f, 0.56f, 0.0f },
        { -0.45f, -0.42f, 0.0f }, { 0.52f, -0.34f, 0.0f }, { 0.42f, 0.52f, 0.0f }, { -0.55f, 0.32f, 0.0f }
    };
    const uint16_t quad_indices[] = { 0, 1, 2, 3, 4, 5, 6, 7 };

    r.setPerspective(45.0f, float(W) / H, 1.0f, 100.0f);
    r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT);

    r.setModelPosScaleRot({ -1.45f, 0.15f, -9.2f }, { 3.55f, 3.55f, 3.55f }, 36.0f, { 0.1f, 1.0f, 0.0f });
    drawWirePathMesh(ctx, path, mesh_v2, mesh_color);

    r.setModelPosScaleRot({ 2.25f, 1.32f, -7.2f }, { 0.75f, 0.75f, 0.75f }, -14.0f, { 0.2f, 1.0f, 0.0f });
    drawWirePathLine(r, path, { -0.95f, -0.32f, 0.0f }, { 0.92f, 0.28f, 0.0f }, orange);

    r.setModelPosScaleRot({ 2.25f, 0.36f, -7.2f }, { 0.62f, 0.62f, 0.62f }, 12.0f, { 0.0f, 1.0f, 0.0f });
    drawWirePathLines(r, path, 3, line_indices, line_vertices, cyan);

    r.setModelPosScaleRot({ 2.25f, -0.78f, -7.3f }, { 0.58f, 0.58f, 0.58f }, 21.0f, { 0.0f, 1.0f, 0.0f });
    drawWirePathTriangle(r, path, { -0.82f, -0.52f, 0.0f }, { 0.78f, -0.42f, 0.0f }, { -0.02f, 0.74f, 0.0f }, lime);

    r.setModelPosScaleRot({ -2.95f, 1.45f, -7.6f }, { 0.55f, 0.55f, 0.55f }, -19.0f, { 0.2f, 1.0f, 0.0f });
    drawWirePathTriangles(r, path, 2, triangle_indices, triangle_vertices, pink);

    r.setModelPosScaleRot({ -3.0f, 0.18f, -7.6f }, { 0.58f, 0.58f, 0.58f }, 24.0f, { 0.2f, 1.0f, 0.0f });
    drawWirePathQuad(r, path, { -0.76f, -0.50f, 0.0f }, { 0.62f, -0.58f, 0.0f }, { 0.78f, 0.54f, 0.0f }, { -0.64f, 0.45f, 0.0f }, violet);

    r.setModelPosScaleRot({ -3.0f, -1.02f, -7.8f }, { 0.48f, 0.48f, 0.48f }, -28.0f, { 0.2f, 1.0f, 0.0f });
    drawWirePathQuads(r, path, 2, quad_indices, quad_vertices, green);

    r.setModelPosScaleRot({ 0.35f, -1.95f, -7.6f }, { 0.48f, 0.48f, 0.48f }, 31.0f, { 0.3f, 1.0f, 0.1f });
    drawWirePathCube(r, path, yellow);

    r.setModelPosScaleRot({ 1.70f, -1.92f, -7.7f }, { 0.43f, 0.43f, 0.43f }, -18.0f, { 0.0f, 1.0f, 0.0f });
    drawWirePathSphere(r, path, cyan);

    r.setModelPosScaleRot({ 2.88f, -1.90f, -7.7f }, { 0.42f, 0.42f, 0.42f }, 0.0f, { 0.0f, 1.0f, 0.0f });
    drawWirePathAdaptativeSphere(r, path, pink);
}

template<typename RendererT>
void setValidationMultiDirectionalLights(RendererT& r)
{
    r.setDirectionalLightCount(4);
    r.setDirectionalLightAmbiant(RGBf(0.08f, 0.09f, 0.12f));
    r.setDirectionalLight(0, { -0.40f, -0.65f, -1.00f }, RGBf(0.58f, 0.50f, 0.42f), RGBf(0.38f, 0.34f, 0.30f));
    r.setDirectionalLight(1, {  0.80f, -0.25f, -0.55f }, RGBf(0.18f, 0.38f, 0.92f), RGBf(0.08f, 0.15f, 0.35f));
    r.setDirectionalLight(2, { -0.35f,  0.70f, -0.40f }, RGBf(0.85f, 0.22f, 0.20f), RGBf(0.30f, 0.08f, 0.08f));
    r.setDirectionalLight(3, {  0.15f,  0.35f, -0.85f }, RGBf(0.18f, 0.82f, 0.42f), RGBf(0.08f, 0.22f, 0.12f));
}

template<typename color_t>
void renderScene(RenderContext<color_t>& ctx, SceneId scene)
{
    ctx.clear();
    auto& r = ctx.renderer;
    r.setMaterial(RGBf(0.85f, 0.58f, 0.26f), 0.18f, 0.72f, 0.55f, 32);
    r.setLight({ -0.25f, -0.45f, -1.0f }, RGBf(0.18f, 0.20f, 0.25f), RGBf(0.85f, 0.82f, 0.72f), RGBf(0.55f, 0.60f, 0.70f));

    switch (scene)
        {
        case SceneId::BunnyGouraud:
            r.setPerspective(45.0f, float(W) / H, 1.0f, 300.0f);
            r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD);
            r.setModelPosScaleRot({ 0.0f, 0.15f, -11.0f }, { 5.0f, 5.0f, 5.0f }, 24.0f, { 0.1f, 1.0f, 0.0f });
            r.drawMesh(&ctx.meshes.bunny, false);
            break;

        case SceneId::BunnyTextureNearest:
            r.setPerspective(45.0f, float(W) / H, 1.0f, 300.0f);
            r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2);
            r.setModelPosScaleRot({ 0.0f, 0.15f, -10.5f }, { 5.1f, 5.1f, 5.1f }, -34.0f, { 0.15f, 1.0f, 0.0f });
            r.drawMesh(&ctx.meshes.bunny, false);
            break;

        case SceneId::BunnyTextureBilinear:
            r.setPerspective(45.0f, float(W) / H, 1.0f, 300.0f);
            r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_CLAMP);
            r.setModelPosScaleRot({ 0.0f, 0.15f, -10.5f }, { 5.1f, 5.1f, 5.1f }, 42.0f, { 0.2f, 1.0f, 0.0f });
            r.drawMesh(&ctx.meshes.bunny, false);
            break;

        case SceneId::BunnyMultiLightGouraud:
            {
            auto& mr = ctx.multi_light_renderer;
            setValidationMultiDirectionalLights(mr);
            mr.setPerspective(45.0f, float(W) / H, 1.0f, 300.0f);
            mr.setMaterial(RGBf(0.92f, 0.64f, 0.32f), 0.16f, 0.82f, 0.46f, 30);
            mr.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD);
            mr.setModelPosScaleRot({ 0.0f, 0.15f, -11.0f }, { 5.0f, 5.0f, 5.0f }, -32.0f, { 0.1f, 1.0f, 0.0f });
            mr.drawMesh(&ctx.meshes.bunny, false);
            break;
            }

        case SceneId::BuddhaFlat:
            r.setPerspective(45.0f, float(W) / H, 1.0f, 300.0f);
            r.setMaterial(RGBf(0.9f, 0.62f, 0.28f), 0.20f, 0.72f, 0.35f, 18);
            r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT);
            r.setModelPosScaleRot({ 0.0f, 0.45f, -32.0f }, { 13.0f, 13.0f, 13.0f }, -28.0f, { 0.0f, 1.0f, 0.0f });
            r.drawMesh(&ctx.meshes.buddha_mesh, false);
            break;

        case SceneId::BunnyV2Gouraud:
            r.setPerspective(45.0f, float(W) / H, 1.0f, 300.0f);
            r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD);
            r.setModelPosScaleRot({ 0.0f, 0.15f, -11.0f }, { 5.0f, 5.0f, 5.0f }, 24.0f, { 0.1f, 1.0f, 0.0f });
            r.drawMesh(&ctx.meshes.bunny_v2, false);
            break;

        case SceneId::BunnyV2TextureNearest:
            r.setPerspective(45.0f, float(W) / H, 1.0f, 300.0f);
            r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2);
            r.setModelPosScaleRot({ 0.0f, 0.15f, -10.5f }, { 5.1f, 5.1f, 5.1f }, -34.0f, { 0.15f, 1.0f, 0.0f });
            r.drawMesh(&ctx.meshes.bunny_v2, true);
            break;

        case SceneId::BunnyV2TextureBilinear:
            r.setPerspective(45.0f, float(W) / H, 1.0f, 300.0f);
            r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_CLAMP);
            r.setModelPosScaleRot({ 0.0f, 0.15f, -10.5f }, { 5.1f, 5.1f, 5.1f }, 42.0f, { 0.2f, 1.0f, 0.0f });
            r.drawMesh(&ctx.meshes.bunny_v2, true);
            break;

        case SceneId::BunnyV2MultiLightTexture:
            {
            auto& mr = ctx.multi_light_renderer;
            setValidationMultiDirectionalLights(mr);
            mr.setPerspective(45.0f, float(W) / H, 1.0f, 300.0f);
            mr.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2);
            mr.setModelPosScaleRot({ 0.0f, 0.15f, -10.5f }, { 5.1f, 5.1f, 5.1f }, 58.0f, { 0.2f, 1.0f, 0.0f });
            mr.drawMesh(&ctx.meshes.bunny_v2, true);
            break;
            }

        case SceneId::BuddhaV2Flat:
            r.setPerspective(45.0f, float(W) / H, 1.0f, 300.0f);
            r.setMaterial(RGBf(0.9f, 0.62f, 0.28f), 0.20f, 0.72f, 0.35f, 18);
            r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT);
            r.setModelPosScaleRot({ 0.0f, 0.45f, -32.0f }, { 13.0f, 13.0f, 13.0f }, -28.0f, { 0.0f, 1.0f, 0.0f });
            r.drawMesh(&ctx.meshes.buddha_v2, false);
            break;

        case SceneId::BuddhaV2Unlit:
            r.setPerspective(45.0f, float(W) / H, 1.0f, 300.0f);
            r.setMaterial(RGBf(0.9f, 0.62f, 0.28f), 0.20f, 0.72f, 0.35f, 18);
            r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_UNLIT);
            r.setModelPosScaleRot({ 0.0f, 0.45f, -32.0f }, { 13.0f, 13.0f, 13.0f }, -28.0f, { 0.0f, 1.0f, 0.0f });
            r.drawMesh(&ctx.meshes.buddha_v2, false);
            break;

        case SceneId::R2D2TextureBilinear:
            r.setPerspective(45.0f, float(W) / H, 1.0f, 300.0f);
            r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_CLAMP);
            r.setModelPosScaleRot({ 0.0f, 0.15f, -7.8f }, { 3.0f, 3.0f, 3.0f }, -36.0f, { 0.05f, 1.0f, 0.0f });
            r.drawMesh(&ctx.meshes.r2d2_mesh, false);
            break;

        case SceneId::R2D2V2TextureBilinear:
            r.setPerspective(45.0f, float(W) / H, 1.0f, 300.0f);
            r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_CLAMP);
            r.setModelPosScaleRot({ 0.0f, 0.15f, -7.8f }, { 3.0f, 3.0f, 3.0f }, -36.0f, { 0.05f, 1.0f, 0.0f });
            r.drawMesh(&ctx.meshes.r2d2_v2, true);
            break;

        case SceneId::FalconV2TextureNearest:
            r.setPerspective(45.0f, float(W) / H, 1.0f, 300.0f);
            r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2);
            r.setModelPosScaleRot({ 0.0f, -0.05f, -8.4f }, { 3.15f, 3.15f, 3.15f }, 68.0f, { 0.32f, 1.0f, 0.08f });
            r.drawMesh(&ctx.meshes.falcon_v2, true);
            break;

        case SceneId::FalconV2TextureUnlit:
            r.setPerspective(45.0f, float(W) / H, 1.0f, 300.0f);
            r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_UNLIT | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2);
            r.setModelPosScaleRot({ 0.0f, -0.05f, -8.4f }, { 3.15f, 3.15f, 3.15f }, 68.0f, { 0.32f, 1.0f, 0.08f });
            r.drawMesh(&ctx.meshes.falcon_v2, true);
            break;

        case SceneId::OrthoBunny:
            r.setOrtho(-9.0f * float(W) / H, 9.0f * float(W) / H, -9.0f, 9.0f, 1.0f, 300.0f);
            r.setShaders(SHADER_ORTHO | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2);
            r.setModelPosScaleRot({ 0.0f, 0.0f, -35.0f }, { 5.8f, 5.8f, 5.8f }, 62.0f, { 0.0f, 1.0f, 0.0f });
            r.drawMesh(&ctx.meshes.bunny, false);
            break;

        case SceneId::OrthoBunnyV2:
            r.setOrtho(-9.0f * float(W) / H, 9.0f * float(W) / H, -9.0f, 9.0f, 1.0f, 300.0f);
            r.setShaders(SHADER_ORTHO | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2);
            r.setModelPosScaleRot({ 0.0f, 0.0f, -35.0f }, { 5.8f, 5.8f, 5.8f }, 62.0f, { 0.0f, 1.0f, 0.0f });
            r.drawMesh(&ctx.meshes.bunny_v2, true);
            break;

        case SceneId::NearClip:
            r.setPerspective(45.0f, float(W) / H, 1.0f, 300.0f);
            r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2);
            r.setModelPosScaleRot({ 0.0f, -0.55f, -3.4f }, { 5.0f, 5.0f, 5.0f }, 18.0f, { 0.2f, 1.0f, 0.0f });
            r.drawMesh(&ctx.meshes.bunny, false);
            break;

        case SceneId::NearClipV2:
            r.setPerspective(45.0f, float(W) / H, 1.0f, 300.0f);
            r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2);
            r.setModelPosScaleRot({ 0.0f, -0.55f, -3.4f }, { 5.0f, 5.0f, 5.0f }, 18.0f, { 0.2f, 1.0f, 0.0f });
            r.drawMesh(&ctx.meshes.bunny_v2, true);
            break;

        case SceneId::FarClip:
            r.setPerspective(45.0f, float(W) / H, 1.0f, 300.0f);
            r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2);
            r.setModelPosScaleRot({ 0.0f, 0.1f, -296.0f }, { 13.0f, 13.0f, 13.0f }, 20.0f, { 0.0f, 1.0f, 0.0f });
            r.drawMesh(&ctx.meshes.bunny, false);
            break;

        case SceneId::FarClipV2:
            r.setPerspective(45.0f, float(W) / H, 1.0f, 300.0f);
            r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2);
            r.setModelPosScaleRot({ 0.0f, 0.1f, -296.0f }, { 13.0f, 13.0f, 13.0f }, 20.0f, { 0.0f, 1.0f, 0.0f });
            r.drawMesh(&ctx.meshes.bunny_v2, true);
            break;

        case SceneId::ZBufferOverlap:
            {
            const fVec3 N = { 0.0f, 0.0f, 1.0f };
            const fVec3 A = { -1.5f, -1.1f, 0.0f };
            const fVec3 B = {  1.3f, -1.0f, 0.0f };
            const fVec3 Cc = {  1.4f,  1.1f, 0.0f };
            const fVec3 D = { -1.4f,  1.0f, 0.0f };
            r.setPerspective(45.0f, float(W) / H, 1.0f, 100.0f);
            r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD);
            r.setModelPosScaleRot({ 0.25f, 0.0f, -8.5f }, { 2.7f, 2.7f, 2.7f }, -28.0f, { 0.0f, 1.0f, 0.0f });
            r.drawQuadWithVertexColor(A, B, Cc, D, RGBf(0.95f, 0.12f, 0.05f), RGBf(1.0f, 0.62f, 0.05f), RGBf(0.6f, 0.1f, 0.9f), RGBf(0.1f, 0.35f, 1.0f), &N, &N, &N, &N);
            r.setModelPosScaleRot({ -0.25f, 0.0f, -7.0f }, { 2.5f, 2.5f, 2.5f }, 34.0f, { 0.0f, 1.0f, 0.0f });
            r.drawCube();
            break;
            }

        case SceneId::LargeTriangles:
            {
            const fVec3 N = { 0.0f, 0.0f, 1.0f };
            r.setPerspective(45.0f, float(W) / H, 1.0f, 100.0f);
            r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD);
            r.setModelPosScaleRot({ 0.0f, 0.0f, -7.0f }, { 2.5f, 2.2f, 1.0f }, 16.0f, { 0.0f, 1.0f, 0.0f });
            r.drawTriangleWithVertexColor({ -1.8f, -1.25f, 0.0f }, { 1.8f, -1.1f, 0.0f }, { 0.15f, 1.65f, 0.0f },
                                          RGBf(1.0f, 0.18f, 0.10f), RGBf(0.1f, 0.85f, 0.55f), RGBf(0.1f, 0.25f, 1.0f), &N, &N, &N);
            r.drawTriangleWithVertexColor({ -1.55f, 1.1f, -0.15f }, { 1.55f, 1.0f, 0.15f }, { -0.15f, -1.75f, 0.0f },
                                          RGBf(1.0f, 0.8f, 0.12f), RGBf(0.8f, 0.1f, 1.0f), RGBf(0.1f, 0.8f, 1.0f), &N, &N, &N);
            break;
            }

        case SceneId::SmallTriangleGrid:
            r.setPerspective(45.0f, float(W) / H, 1.0f, 100.0f);
            r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2);
            r.setModelPosScaleRot({ 0.0f, 0.0f, -5.2f }, { 3.2f, 2.35f, 1.0f }, -28.0f, { 0.2f, 1.0f, 0.0f });
            r.drawTriangles(int(ctx.grid_indices.size() / 3), ctx.grid_indices.data(), ctx.grid_vertices.data(),
                            ctx.grid_indices.data(), ctx.grid_normals.data(),
                            ctx.grid_indices.data(), ctx.grid_texcoords.data(), &ctx.meshes.bunny_texture);
            break;

        case SceneId::TriangleStripRibbon:
            r.setPerspective(45.0f, float(W) / H, 1.0f, 100.0f);
            r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_WRAP_POW2);
            r.setModelPosScaleRot({ 0.0f, 0.0f, -5.8f }, { 2.25f, 2.25f, 1.0f }, -26.0f, { 0.25f, 1.0f, 0.1f });
            r.drawTriangleStrip(int(ctx.strip_indices.size()), ctx.strip_indices.data(), ctx.strip_vertices.data(),
                                ctx.strip_indices.data(), ctx.strip_normals.data(),
                                ctx.strip_indices.data(), ctx.strip_texcoords.data(), &ctx.meshes.bunny_texture);
            break;

        case SceneId::WireTriangleStripFast:
            r.setPerspective(45.0f, float(W) / H, 1.0f, 100.0f);
            r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT);
            r.setMaterialColor(C<color_t>(255, 230, 110));
            r.setModelPosScaleRot({ 0.0f, 0.0f, -5.8f }, { 2.25f, 2.25f, 1.0f }, -26.0f, { 0.25f, 1.0f, 0.1f });
            r.drawWireFrameTriangleStrip(int(ctx.strip_indices.size()), ctx.strip_indices.data(), ctx.strip_vertices.data());
            break;

        case SceneId::WireTriangleStripAA:
            r.setPerspective(45.0f, float(W) / H, 1.0f, 100.0f);
            r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT);
            r.setMaterialColor(C<color_t>(100, 230, 255));
            r.setModelPosScaleRot({ 0.0f, 0.0f, -5.8f }, { 2.25f, 2.25f, 1.0f }, -26.0f, { 0.25f, 1.0f, 0.1f });
            r.drawWireFrameTriangleStripAA(int(ctx.strip_indices.size()), ctx.strip_indices.data(), ctx.strip_vertices.data());
            break;

        case SceneId::WireTriangleStripThickness:
            r.setPerspective(45.0f, float(W) / H, 1.0f, 100.0f);
            r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT);
            r.setModelPosScaleRot({ 0.0f, 0.0f, -5.8f }, { 2.25f, 2.25f, 1.0f }, -26.0f, { 0.25f, 1.0f, 0.1f });
            r.drawWireFrameTriangleStrip(int(ctx.strip_indices.size()), ctx.strip_indices.data(), ctx.strip_vertices.data(), 1.501f, C<color_t>(255, 150, 210), 1.0f);
            break;

        case SceneId::DirectShapes:
            r.setPerspective(45.0f, float(W) / H, 1.0f, 100.0f);
            r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_CLAMP);
            r.setModelPosScaleRot({ -1.15f, 0.0f, -8.0f }, { 1.7f, 1.7f, 1.7f }, 35.0f, { 0.2f, 1.0f, 0.0f });
            r.drawCube(&ctx.meshes.bunny_texture, &ctx.meshes.bunny_texture, &ctx.meshes.bunny_texture, &ctx.meshes.bunny_texture, &ctx.meshes.bunny_texture, &ctx.meshes.bunny_texture);
            r.setModelPosScaleRot({ 1.35f, 0.0f, -8.2f }, { 1.65f, 1.65f, 1.65f }, -20.0f, { 0.0f, 1.0f, 0.0f });
            r.drawSphere(20, 11, &ctx.meshes.bunny_texture);
            break;

        case SceneId::WirePoints:
            r.setPerspective(45.0f, float(W) / H, 1.0f, 100.0f);
            r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT);
            r.setModelPosScaleRot({ -0.8f, 0.15f, -9.0f }, { 4.4f, 4.4f, 4.4f }, 38.0f, { 0.1f, 1.0f, 0.0f });
            r.setMaterialColor(C<color_t>(230, 230, 245));
            r.drawWireFrameMeshAA(&ctx.meshes.bunny, true);
            r.setModelPosScaleRot({ 1.45f, 0.0f, -8.4f }, { 2.2f, 2.2f, 2.2f }, 20.0f, { 0.2f, 1.0f, 0.0f });
            r.drawDots(POINT_COUNT, ctx.point_positions.data(), ctx.point_radius_ind.data(), ctx.point_radii, ctx.point_colors_ind.data(), ctx.point_colors, ctx.point_opacity_ind.data(), ctx.point_opacities);
            r.setMaterialColor(C<color_t>(255, 180, 80));
            r.setModelPosScaleRot({ -2.65f, -1.55f, -7.2f }, { 1.0f, 1.0f, 1.0f }, 0.0f, { 0.0f, 1.0f, 0.0f });
            r.drawWireFrameLineAA({ -0.7f, -0.35f, 0.0f }, { 0.7f, 0.25f, 0.0f });
            r.setMaterialColor(C<color_t>(80, 220, 255));
            r.setModelPosScaleRot({ -2.1f, 1.45f, -7.5f }, { 0.8f, 0.8f, 0.8f }, 24.0f, { 0.0f, 1.0f, 0.0f });
            r.drawWireFrameTriangleAA({ -0.7f, -0.55f, 0.0f }, { 0.75f, -0.45f, 0.0f }, { 0.05f, 0.7f, 0.0f });
            r.setMaterialColor(C<color_t>(220, 255, 100));
            r.setModelPosScaleRot({ 2.55f, 1.35f, -7.8f }, { 0.75f, 0.75f, 0.75f }, -18.0f, { 0.2f, 1.0f, 0.0f });
            r.drawWireFrameQuadAA({ -0.7f, -0.5f, 0.0f }, { 0.65f, -0.55f, 0.0f }, { 0.75f, 0.55f, 0.0f }, { -0.65f, 0.45f, 0.0f });
            r.setMaterialColor(C<color_t>(255, 120, 190));
            r.setModelPosScaleRot({ 2.7f, -1.6f, -8.0f }, { 0.52f, 0.52f, 0.52f }, 28.0f, { 0.2f, 1.0f, 0.1f });
            r.drawWireFrameCubeAA();
            break;

        case SceneId::WirePointsV2:
            r.setPerspective(45.0f, float(W) / H, 1.0f, 100.0f);
            r.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT);
            r.setModelPosScaleRot({ -0.8f, 0.15f, -9.0f }, { 4.4f, 4.4f, 4.4f }, 38.0f, { 0.1f, 1.0f, 0.0f });
            r.setMaterialColor(C<color_t>(230, 230, 245));
            r.drawWireFrameMeshAA(&ctx.meshes.bunny_v2);
            r.setModelPosScaleRot({ 1.45f, 0.0f, -8.4f }, { 2.2f, 2.2f, 2.2f }, 20.0f, { 0.2f, 1.0f, 0.0f });
            r.drawDots(POINT_COUNT, ctx.point_positions.data(), ctx.point_radius_ind.data(), ctx.point_radii, ctx.point_colors_ind.data(), ctx.point_colors, ctx.point_opacity_ind.data(), ctx.point_opacities);
            r.setMaterialColor(C<color_t>(100, 255, 170));
            r.setModelPosScaleRot({ 2.65f, -1.55f, -8.2f }, { 0.5f, 0.5f, 0.5f }, -20.0f, { 0.2f, 1.0f, 0.2f });
            r.drawWireFrameSphereAA(8, 5);
            break;

        case SceneId::WireFast:
            renderWirePathScene(ctx, WirePath::Fast, false);
            break;

        case SceneId::WireAA:
            renderWirePathScene(ctx, WirePath::Antialiased, false);
            break;

        case SceneId::WireThickness05:
            renderWirePathScene(ctx, WirePath::Thickness05, false);
            break;

        case SceneId::WireThickness10:
            renderWirePathScene(ctx, WirePath::Thickness10, false);
            break;

        case SceneId::WireThickness20:
            renderWirePathScene(ctx, WirePath::Thickness20, false);
            break;

        case SceneId::WireV2Fast:
            renderWirePathScene(ctx, WirePath::Fast, true);
            break;

        case SceneId::WireV2AA:
            renderWirePathScene(ctx, WirePath::Antialiased, true);
            break;

        case SceneId::WireV2Thickness05:
            renderWirePathScene(ctx, WirePath::Thickness05, true);
            break;

        case SceneId::WireV2Thickness10:
            renderWirePathScene(ctx, WirePath::Thickness10, true);
            break;

        case SceneId::WireV2Thickness20:
            renderWirePathScene(ctx, WirePath::Thickness20, true);
            break;

        case SceneId::Count:
            break;
        }
}

template<typename color_t>
Signature computeSignature(const Image<color_t>& im, color_t bg, const std::string& color_type, const std::string& scene)
{
    Signature s;
    s.color_type = color_type;
    s.scene = scene;
    s.w = im.lx();
    s.h = im.ly();
    s.min_x = im.lx();
    s.min_y = im.ly();
    s.max_x = -1;
    s.max_y = -1;

    const RGB32 bg32(bg);
    uint64_t h = 1469598103934665603ull;
    for (int y = 0; y < im.ly(); y++)
        {
        for (int x = 0; x < im.lx(); x++)
            {
            const RGB32 c = RGB32(im(x, y));
            const uint8_t b[3] = { c.R, c.G, c.B };
            h = fnv1a(b, sizeof(b), h);
            s.sum_r += c.R;
            s.sum_g += c.G;
            s.sum_b += c.B;

            const int d = std::abs(int(c.R) - int(bg32.R)) +
                          std::abs(int(c.G) - int(bg32.G)) +
                          std::abs(int(c.B) - int(bg32.B));
            if (d > 6)
                {
                s.active_pixels++;
                s.min_x = std::min(s.min_x, x);
                s.min_y = std::min(s.min_y, y);
                s.max_x = std::max(s.max_x, x);
                s.max_y = std::max(s.max_y, y);
                }
            }
        }
    if (s.active_pixels == 0)
        {
        s.min_x = s.min_y = s.max_x = s.max_y = 0;
        }
    s.hash = h;

    uint64_t ch = 1469598103934665603ull;
    for (int by = 0; by < BLOCK_Y; by++)
        {
        for (int bx = 0; bx < BLOCK_X; bx++)
            {
            const int x0 = bx * im.lx() / BLOCK_X;
            const int x1 = (bx + 1) * im.lx() / BLOCK_X;
            const int y0 = by * im.ly() / BLOCK_Y;
            const int y1 = (by + 1) * im.ly() / BLOCK_Y;
            uint64_t sr = 0, sg = 0, sb = 0, n = 0;
            for (int y = y0; y < y1; y++)
                {
                for (int x = x0; x < x1; x++)
                    {
                    const RGB32 c = RGB32(im(x, y));
                    sr += c.R;
                    sg += c.G;
                    sb += c.B;
                    n++;
                    }
                }
            const uint8_t q[3] = {
                uint8_t((sr / n) / 16),
                uint8_t((sg / n) / 16),
                uint8_t((sb / n) / 16)
            };
            ch = fnv1a(q, sizeof(q), ch);
            }
        }
    s.coarse_hash = ch;
    return s;
}

template<typename color_t>
std::vector<RGB32> copyRGB32(const Image<color_t>& im)
{
    std::vector<RGB32> out(size_t(im.lx()) * size_t(im.ly()));
    for (int y = 0; y < im.ly(); y++)
        {
        for (int x = 0; x < im.lx(); x++)
            {
            out[size_t(x) + size_t(y) * size_t(im.lx())] = RGB32(im(x, y));
            }
        }
    return out;
}

void writeOverviewSheet(const std::filesystem::path& path, const std::string& title, const std::vector<Snapshot>& shots)
{
    const int thumb_w = 400;
    const int thumb_h = 300;
    const int cols = 2;
    const int rows = int((shots.size() + cols - 1) / cols);
    const int margin = 16;
    const int label_h = 28;
    const int sheet_w = cols * thumb_w + (cols + 1) * margin;
    const int sheet_h = rows * (thumb_h + label_h) + (rows + 1) * margin + 38;
    CImg<unsigned char> sheet(sheet_w, sheet_h, 1, 3, 18);
    const unsigned char white[] = { 238, 242, 250 };
    const unsigned char muted[] = { 115, 130, 150 };
    const unsigned char line[] = { 42, 54, 72 };
    const unsigned char* transparent = nullptr;

    sheet.draw_text(margin, 10, title.c_str(), white, transparent, 1.0f, 24);
    for (size_t i = 0; i < shots.size(); i++)
        {
        const int col = int(i % cols);
        const int row = int(i / cols);
        const int px = margin + col * (thumb_w + margin);
        const int py = 48 + margin + row * (thumb_h + label_h + margin);

        CImg<unsigned char> tile(W, H, 1, 3, 0);
        for (int y = 0; y < H; y++)
            {
            for (int x = 0; x < W; x++)
                {
                const RGB32 c = shots[i].pixels[size_t(x) + size_t(y) * W];
                tile(x, y, 0) = c.R;
                tile(x, y, 1) = c.G;
                tile(x, y, 2) = c.B;
                }
            }
        tile.resize(thumb_w, thumb_h, 1, 3, 3);
        sheet.draw_image(px, py, tile);
        sheet.draw_rectangle(px, py + thumb_h, px + thumb_w - 1, py + thumb_h + label_h - 1, line);
        sheet.draw_text(px + 8, py + thumb_h + 6, shots[i].scene.c_str(), white, transparent, 1.0f, 16);
        sheet.draw_text(px + thumb_w - 70, py + thumb_h + 6, shots[i].color_type.c_str(), muted, transparent, 1.0f, 16);
        }

    std::filesystem::create_directories(path.parent_path());
    sheet.save_bmp(path.string().c_str());
}

template<typename color_t>
void runColorSuite(const std::string& color_type, const Options& opt, const std::map<std::string, Signature>& baseline,
                   std::vector<Signature>& rows, std::vector<Snapshot>& snapshots)
{
    RenderContext<color_t> ctx;
    const auto current_dir = opt.out_dir / "current" / color_type;
    std::filesystem::create_directories(current_dir);

    for (const auto& scene : SCENES)
        {
        renderScene(ctx, scene.id);
        const auto bmp_path = current_dir / (std::string(scene.name) + ".bmp");
        writeBMP(bmp_path, ctx.image);

        Signature sig = computeSignature(ctx.image, backgroundColor<color_t>(), color_type, scene.name);
        const auto it = baseline.find(keyFor(color_type, scene.name));
        const Signature* base = (it == baseline.end()) ? nullptr : &it->second;
        sig.ok = compareSignature(sig, base, opt.compare_mode);
        rows.push_back(sig);

        Snapshot shot;
        shot.color_type = color_type;
        shot.scene = scene.name;
        shot.path = bmp_path;
        shot.pixels = copyRGB32(ctx.image);
        snapshots.push_back(std::move(shot));

        std::cout << std::left << std::setw(8) << color_type
                  << " " << std::setw(24) << scene.name
                  << " active=" << std::setw(7) << sig.active_pixels
                  << " hash=" << sig.hash
                  << " coarse=" << sig.coarse_hash
                  << " " << (sig.ok ? "PASS" : "FAIL")
                  << " " << sig.note << "\n";
        }
}

void copyGolden(const Options& opt, const std::vector<Snapshot>& snapshots)
{
    if (opt.write_golden_dir.empty()) return;
    for (const auto& shot : snapshots)
        {
        const auto dst = opt.write_golden_dir / shot.color_type / (shot.scene + ".bmp");
        std::filesystem::create_directories(dst.parent_path());
        std::filesystem::copy_file(shot.path, dst, std::filesystem::copy_options::overwrite_existing);
        }
}

int runValidation(const Options& opt)
{
    std::cout << "TGX 3D CPU validation\n";
    std::cout << "Image size: " << W << "x" << H << "\n";
    std::cout << "Compare mode: " << opt.compare_mode << "\n";
    std::cout << "Scenes: " << int(SceneId::Count) << "\n\n";

    std::filesystem::create_directories(opt.out_dir);
    const auto baseline = opt.baseline_path.empty() ? std::map<std::string, Signature>() : readBaseline(opt.baseline_path);
    std::vector<Signature> rows;
    std::vector<Snapshot> snapshots565;
    std::vector<Snapshot> snapshots32;

    runColorSuite<RGB565>("RGB565", opt, baseline, rows, snapshots565);
    std::cout << "\n";
    runColorSuite<RGB32>("RGB32", opt, baseline, rows, snapshots32);

    writeResultsCsv(opt.out_dir / "tgx_3d_cpu_results.csv", rows);

    writeOverviewSheet(opt.out_dir / "tgx_3d_cpu_overview_RGB565.bmp", "TGX 3D validation - RGB565", snapshots565);
    writeOverviewSheet(opt.out_dir / "tgx_3d_cpu_overview_RGB32.bmp", "TGX 3D validation - RGB32", snapshots32);

    if (!opt.write_golden_dir.empty())
        {
        std::vector<Snapshot> all = snapshots565;
        all.insert(all.end(), snapshots32.begin(), snapshots32.end());
        copyGolden(opt, all);
        writeOverviewSheet(opt.write_golden_dir / "tgx_3d_cpu_overview_RGB565.bmp", "TGX 3D validation - RGB565", snapshots565);
        writeOverviewSheet(opt.write_golden_dir / "tgx_3d_cpu_overview_RGB32.bmp", "TGX 3D validation - RGB32", snapshots32);
        }

    if (!opt.write_baseline_path.empty())
        {
        writeBaseline(opt.write_baseline_path, rows);
        }

    int fail = 0;
    for (const auto& r : rows)
        {
        if (!r.ok) fail++;
        }

    std::cout << "\nResults: " << (rows.size() - fail) << " PASS, " << fail << " FAIL\n";
    std::cout << "CSV: " << (opt.out_dir / "tgx_3d_cpu_results.csv").string() << "\n";
    std::cout << "Overview: " << (opt.out_dir / "tgx_3d_cpu_overview_RGB565.bmp").string() << "\n";
    std::cout << "Overview: " << (opt.out_dir / "tgx_3d_cpu_overview_RGB32.bmp").string() << "\n";
    return fail == 0 ? 0 : 2;
}

void runDemo()
{
    std::cout << "Running CImg demo...\n";
    constexpr int DW = 800;
    constexpr int DH = 600;
    constexpr int DEMO_FRAMES = 360;
    std::vector<RGB32> pixels(size_t(DW) * size_t(DH));
    std::vector<uint16_t> zbuf(size_t(DW) * size_t(DH));
    Image<RGB32> im(pixels.data(), DW, DH);
    Renderer3D<RGB32, LOADED_SHADERS, uint16_t> renderer;
    MeshAssets<RGB32> meshes;
    renderer.setViewportSize(DW, DH);
    renderer.setOffset(0, 0);
    renderer.setImage(&im);
    renderer.setZbuffer(zbuf.data());
    renderer.setPerspective(45.0f, float(DW) / DH, 1.0f, 300.0f);

    CImg<unsigned char> frame(DW, DH, 1, 3, 0);
    CImgDisplay display(frame, "TGX 3D CPU validation demo");
    for (int f = 0; f < DEMO_FRAMES && !display.is_closed(); f++)
        {
        const float t = float(f) / DEMO_FRAMES;
        const float pulse = 0.5f + 0.5f * std::sin(f * 0.035f);
        for (int y = 0; y < DH; y++)
            {
            const float v = float(y) / (DH - 1);
            for (int x = 0; x < DW; x++)
                {
                const float u = float(x) / (DW - 1);
                const float glow = 0.5f + 0.5f * std::sin(8.0f * u + 4.0f * v + f * 0.012f);
                pixels[size_t(x) + size_t(y) * DW] = RGB32(
                    int(5 + 12 * (1.0f - v) + 9 * glow * pulse),
                    int(8 + 14 * (1.0f - v) + 5 * u),
                    int(18 + 30 * (1.0f - v) + 18 * glow));
                }
            }
        for (int s = 0; s < 130; s++)
            {
            const int x = (s * 73 + f * (1 + (s % 3))) % DW;
            const int y = (s * 47 + s * s) % (DH * 2 / 3);
            const int b = 70 + ((s * 53 + f * 3) % 100);
            pixels[size_t(x) + size_t(y) * DW] = RGB32(b, b + 12, std::min(255, b + 40));
            }

        renderer.clearZbuffer();
        const float a = f * 1.15f;
        renderer.setLight({ -0.35f + 0.55f * std::sin(f * 0.020f), -0.48f, -1.0f },
                          RGBf(0.13f, 0.16f, 0.24f),
                          RGBf(0.72f + 0.22f * pulse, 0.76f, 0.86f),
                          RGBf(0.55f, 0.58f, 0.75f));

        renderer.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2);
        renderer.setMaterial(RGBf(0.95f, 0.78f, 0.55f), 0.16f, 0.70f, 0.48f, 28);
        renderer.setModelPosScaleRot({ -1.25f + 0.20f * std::sin(f * 0.020f), 0.08f, -10.5f },
                                     { 4.45f, 4.45f, 4.45f },
                                     a, { 0.12f, 1.0f, 0.04f });
        renderer.drawMesh(&meshes.bunny, false);

        renderer.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_CLAMP);
        renderer.setMaterial(RGBf(0.78f, 0.82f, 1.0f), 0.20f, 0.72f, 0.82f, 48);
        renderer.setModelPosScaleRot({ 1.38f + 0.16f * std::sin(f * 0.030f), 0.18f + 0.12f * std::sin(f * 0.045f), -7.6f },
                                     { 1.45f, 1.45f, 1.45f },
                                     -a * 1.7f, { 0.2f, 1.0f, 0.0f });
        renderer.drawSphere(22, 12, &meshes.bunny_texture);

        renderer.setShaders(SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT);
        renderer.setMaterial(RGBf(0.88f, 0.60f, 0.28f), 0.18f, 0.72f, 0.30f, 18);
        renderer.setModelPosScaleRot({ 1.85f, -0.70f, -10.5f },
                                     { 1.05f, 1.05f, 1.05f },
                                     -a * 0.75f + 20.0f, { 0.0f, 1.0f, 0.0f });
        renderer.drawCube();

        renderer.setModelPosScaleRot({ -0.05f, -1.55f, -7.8f },
                                     { 1.25f, 1.25f, 1.25f },
                                     a * 1.4f, { 0.35f, 1.0f, 0.15f });
        renderer.drawWireFrameCube(1.45f, RGB32(245, 215, 95), 0.85f);

        for (int y = 0; y < DH; y++)
            {
            for (int x = 0; x < DW; x++)
                {
                const RGB32 c = pixels[size_t(x) + size_t(y) * DW];
                frame(x, y, 0) = c.R;
                frame(x, y, 1) = c.G;
                frame(x, y, 2) = c.B;
                }
            }
        display.display(frame);
        cimg::wait(16);
        }
    std::cout << "CImg demo done.\n";
}

Options parseOptions(int argc, char** argv)
{
    Options opt;
    for (int i = 1; i < argc; i++)
        {
        const std::string a = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) throw std::runtime_error("missing value after " + a);
            return argv[++i];
        };
        if (a == "--out") opt.out_dir = next();
        else if (a == "--baseline") opt.baseline_path = next();
        else if (a == "--write-baseline") opt.write_baseline_path = next();
        else if (a == "--write-golden") opt.write_golden_dir = next();
        else if (a == "--compare") opt.compare_mode = next();
        else if (a == "--demo") opt.demo = true;
        else if (a == "--demo-only")
            {
            opt.demo = true;
            opt.demo_only = true;
            }
        else if (a == "--help")
            {
            std::cout << "Usage: tgx_3d_cpu_suite [--out DIR] [--baseline CSV] [--write-baseline CSV]\n"
                      << "                        [--write-golden DIR] [--compare strict|tolerant|smoke]\n"
                      << "                        [--demo] [--demo-only]\n";
            std::exit(0);
            }
        else
            {
            throw std::runtime_error("unknown argument: " + a);
            }
        }
    if (opt.compare_mode != "strict" && opt.compare_mode != "tolerant" && opt.compare_mode != "smoke")
        {
        throw std::runtime_error("invalid compare mode: " + opt.compare_mode);
        }
    return opt;
}

} // namespace

int main(int argc, char** argv)
{
    try
        {
        const Options opt = parseOptions(argc, argv);
        if (opt.demo)
            {
            runDemo();
            if (opt.demo_only) return 0;
            }
        return runValidation(opt);
        }
    catch (const std::exception& e)
        {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
        }
}
