// TGX 2D CPU validation and benchmark suite.
//
// This executable renders a broad 2D API exercise image for RGB24, RGB32 and
// RGBf, writes BMP snapshots, and records simple timing/hash data to CSV.

#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <tgx.h>
#include <font_tgx_Arial.h>
#include <font_tgx_OpenSans.h>
#include <font_tgx_OpenSans_Bold.h>

namespace
{

constexpr int W = 800;
constexpr int H = 600;

uint8_t gfx_bitmap[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x81, 0xbd, 0xa5, 0xa5, 0xbd, 0x81, 0xff, 0x00,
    0x18, 0x3c, 0x7e, 0xdb, 0xff, 0x24, 0x24, 0x66
};

GFXglyph gfx_glyphs[] = {
    { 0,  8, 8, 8, 0, -8 },
    { 8,  8, 8, 8, 0, -8 },
    { 16, 8, 8, 8, 0, -8 }
};

GFXfont gfx_font = {
    gfx_bitmap,
    gfx_glyphs,
    'A',
    'C',
    10
};

struct BenchRow
{
    std::string platform = "cpu";
    std::string color_type;
    std::string group;
    std::string kind;
    int iterations = 0;
    double total_us = 0.0;
    double per_iter_us = 0.0;
    uint64_t hash = 0;
    int changed_pixels = 0;
    bool ok = false;
    std::string note;
};

struct BaselineEntry
{
    uint64_t hash = 0;
    int changed_pixels = 0;
};

struct Options
{
    std::filesystem::path out_dir = "tmp/tgx_2d_cpu_suite";
    std::filesystem::path baseline_path;
    std::filesystem::path write_baseline_path;
    std::filesystem::path golden_dir;
    std::filesystem::path write_golden_dir;
    std::filesystem::path golden_diff_dir;
    bool strict_baseline = true;
    bool large = false;
    int large_size = 2048;
};

struct SnapshotFile
{
    std::string color_type;
    std::string group;
    std::string filename;
    std::filesystem::path path;
};

struct IPointSource
{
    const tgx::iVec2* points;
    int count;
    int index = 0;

    bool operator()(tgx::iVec2& p)
    {
        if (count <= 0) return false;
        if (index == count) index = 0;
        p = points[index++];
        return index < count;
    }
};

struct FPointSource
{
    const tgx::fVec2* points;
    int count;
    int index = 0;

    bool operator()(tgx::fVec2& p)
    {
        if (count <= 0) return false;
        if (index == count) index = 0;
        p = points[index++];
        return index < count;
    }
};

uint64_t fnv1a(const uint8_t* data, size_t len, uint64_t h = 1469598103934665603ull)
{
    for (size_t i = 0; i < len; i++)
        {
        h ^= data[i];
        h *= 1099511628211ull;
        }
    return h;
}

template<typename color_t>
tgx::RGB32 toRGB32(color_t c)
{
    return tgx::RGB32(c);
}

template<typename color_t>
uint64_t imageHash(const tgx::Image<color_t>& im)
{
    uint64_t h = 1469598103934665603ull;
    for (int y = 0; y < im.ly(); y++)
        {
        for (int x = 0; x < im.lx(); x++)
            {
            const tgx::RGB32 c = toRGB32(im(x, y));
            const uint8_t b[4] = { c.R, c.G, c.B, c.A };
            h = fnv1a(b, sizeof(b), h);
            }
        }
    return h;
}

template<typename color_t>
int countChanged(const tgx::Image<color_t>& im, color_t bg)
{
    int n = 0;
    for (int y = 0; y < im.ly(); y++)
        {
        for (int x = 0; x < im.lx(); x++)
            {
            if (im(x, y) != bg) n++;
            }
        }
    return n;
}

template<typename color_t>
bool imageEquals(const tgx::Image<color_t>& a, const tgx::Image<color_t>& b)
{
    if ((a.lx() != b.lx()) || (a.ly() != b.ly())) return false;
    for (int y = 0; y < a.ly(); y++)
        {
        for (int x = 0; x < a.lx(); x++)
            {
            if (a(x, y) != b(x, y)) return false;
            }
        }
    return true;
}

template<>
int countChanged<tgx::RGBf>(const tgx::Image<tgx::RGBf>& im, tgx::RGBf bg)
{
    int n = 0;
    for (int y = 0; y < im.ly(); y++)
        {
        for (int x = 0; x < im.lx(); x++)
            {
            const auto c = im(x, y);
            if ((c.R != bg.R) || (c.G != bg.G) || (c.B != bg.B)) n++;
            }
        }
    return n;
}

template<typename color_t>
color_t C(int r, int g, int b, int a = 255)
{
    return color_t(tgx::RGB32(r, g, b, a));
}

template<typename color_t>
color_t bgColor()
{
    return C<color_t>(11, 15, 22);
}

template<>
tgx::RGBf bgColor<tgx::RGBf>()
{
    return tgx::RGBf(11.0f / 255.0f, 15.0f / 255.0f, 22.0f / 255.0f);
}

template<typename color_t>
bool sameColor(color_t a, color_t b)
{
    const tgx::RGB32 ca(a);
    const tgx::RGB32 cb(b);
    return (ca.R == cb.R) && (ca.G == cb.G) && (ca.B == cb.B) && (ca.A == cb.A);
}

template<typename color_t>
bool nearColor(color_t a, color_t b, int tolerance = 1)
{
    const tgx::RGB32 ca(a);
    const tgx::RGB32 cb(b);
    return (std::abs(int(ca.R) - int(cb.R)) <= tolerance) &&
           (std::abs(int(ca.G) - int(cb.G)) <= tolerance) &&
           (std::abs(int(ca.B) - int(cb.B)) <= tolerance);
}

std::string rowKey(const std::string& platform, const std::string& color_type, const std::string& group, const std::string& kind)
{
    return platform + "|" + color_type + "|" + group + "|" + kind;
}

std::vector<std::string> splitCsvLine(const std::string& line)
{
    std::vector<std::string> cols;
    std::string cur;
    bool quoted = false;
    for (char ch : line)
        {
        if (ch == '"')
            {
            quoted = !quoted;
            }
        else if ((ch == ',') && (!quoted))
            {
            cols.push_back(cur);
            cur.clear();
            }
        else
            {
            cur.push_back(ch);
            }
        }
    cols.push_back(cur);
    return cols;
}

uint64_t parseHash(const std::string& s)
{
    size_t pos = 0;
    int base = 10;
    if ((s.size() > 2) && (s[0] == '0') && ((s[1] == 'x') || (s[1] == 'X')))
        {
        pos = 2;
        base = 16;
        }
    return std::stoull(s.substr(pos), nullptr, base);
}

std::string csvEscape(const std::string& s)
{
    if (s.find_first_of(",\"\r\n") == std::string::npos) return s;
    std::string out = "\"";
    for (char ch : s)
        {
        if (ch == '"') out += "\"\"";
        else out.push_back(ch);
        }
    out += '"';
    return out;
}

std::map<std::string, BaselineEntry> loadBaseline(const std::filesystem::path& path)
{
    std::map<std::string, BaselineEntry> baseline;
    std::ifstream in(path);
    if (!in) return baseline;

    std::string line;
    std::getline(in, line);
    while (std::getline(in, line))
        {
        if (line.empty()) continue;
        const auto cols = splitCsvLine(line);
        if (cols.size() < 6) continue;
        baseline[rowKey(cols[0], cols[1], cols[2], cols[3])] = { parseHash(cols[4]), std::stoi(cols[5]) };
        }
    return baseline;
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

uint16_t readU16(const std::vector<uint8_t>& data, size_t off)
{
    return uint16_t(data[off] | (uint16_t(data[off + 1]) << 8));
}

uint32_t readU32(const std::vector<uint8_t>& data, size_t off)
{
    return uint32_t(readU16(data, off)) | (uint32_t(readU16(data, off + 2)) << 16);
}

int32_t readI32(const std::vector<uint8_t>& data, size_t off)
{
    return int32_t(readU32(data, off));
}

template<typename color_t>
bool writeBMP(const std::filesystem::path& path, const tgx::Image<color_t>& im)
{
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
            const tgx::RGB32 c = toRGB32(im(x, y));
            row[3 * x + 0] = c.B;
            row[3 * x + 1] = c.G;
            row[3 * x + 2] = c.R;
            }
        out.write(reinterpret_cast<const char*>(row.data()), row.size());
        }
    return bool(out);
}

struct BmpImage
{
    int w = 0;
    int h = 0;
    std::vector<tgx::RGB32> pixels;
};

bool readBMP24(const std::filesystem::path& path, BmpImage& out, std::string& error)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
        {
        error = "cannot open BMP";
        return false;
        }

    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (data.size() < 54 || data[0] != 'B' || data[1] != 'M')
        {
        error = "not a BMP file";
        return false;
        }

    const uint32_t pixel_offset = readU32(data, 10);
    const uint32_t dib_size = readU32(data, 14);
    const int32_t width = readI32(data, 18);
    const int32_t height_signed = readI32(data, 22);
    const uint16_t planes = readU16(data, 26);
    const uint16_t bpp = readU16(data, 28);
    const uint32_t compression = readU32(data, 30);
    if (dib_size < 40 || width <= 0 || height_signed == 0 || planes != 1 || bpp != 24 || compression != 0)
        {
        error = "unsupported BMP format";
        return false;
        }

    const int height = std::abs(height_signed);
    const int row_stride = ((width * 3 + 3) / 4) * 4;
    if (pixel_offset + size_t(row_stride) * size_t(height) > data.size())
        {
        error = "truncated BMP data";
        return false;
        }

    out.w = width;
    out.h = height;
    out.pixels.assign(size_t(width) * size_t(height), tgx::RGB32_Black);
    const bool bottom_up = height_signed > 0;
    for (int y = 0; y < height; y++)
        {
        const int src_y = bottom_up ? (height - 1 - y) : y;
        const size_t row_off = pixel_offset + size_t(src_y) * size_t(row_stride);
        for (int x = 0; x < width; x++)
            {
            const size_t off = row_off + size_t(3 * x);
            out.pixels[size_t(y) * size_t(width) + size_t(x)] = tgx::RGB32(data[off + 2], data[off + 1], data[off + 0]);
            }
        }
    return true;
}

bool writeBMP32Pixels(const std::filesystem::path& path, int w, int h, const std::vector<tgx::RGB32>& pixels)
{
    std::vector<tgx::RGB32> copy = pixels;
    tgx::Image<tgx::RGB32> im(copy.data(), w, h);
    return writeBMP(path, im);
}

uint64_t fileHash(const std::filesystem::path& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) return 0;
    uint64_t h = 1469598103934665603ull;
    char buf[4096];
    while (in)
        {
        in.read(buf, sizeof(buf));
        const std::streamsize n = in.gcount();
        h = fnv1a(reinterpret_cast<const uint8_t*>(buf), size_t(n), h);
        }
    return h;
}

struct GoldenCompareResult
{
    bool ok = false;
    int diff_pixels = 0;
    int max_delta = 0;
    std::string note;
};

GoldenCompareResult compareGoldenBMP(const SnapshotFile& snapshot, const std::filesystem::path& golden_path, const std::filesystem::path& diff_path)
{
    GoldenCompareResult result;
    BmpImage current;
    BmpImage golden;
    std::string error;
    if (!std::filesystem::exists(golden_path))
        {
        result.note = "missing golden image";
        return result;
        }
    if (!readBMP24(snapshot.path, current, error))
        {
        result.note = "current BMP read failed: " + error;
        return result;
        }
    if (!readBMP24(golden_path, golden, error))
        {
        result.note = "golden BMP read failed: " + error;
        return result;
        }
    if ((current.w != golden.w) || (current.h != golden.h))
        {
        std::ostringstream oss;
        oss << "dimension mismatch current=" << current.w << "x" << current.h
            << " golden=" << golden.w << "x" << golden.h;
        result.note = oss.str();
        return result;
        }

    std::vector<tgx::RGB32> diff(size_t(current.w) * size_t(current.h), tgx::RGB32(6, 8, 12));
    for (size_t i = 0; i < current.pixels.size(); i++)
        {
        const tgx::RGB32 a = current.pixels[i];
        const tgx::RGB32 b = golden.pixels[i];
        const int dr = std::abs(int(a.R) - int(b.R));
        const int dg = std::abs(int(a.G) - int(b.G));
        const int db = std::abs(int(a.B) - int(b.B));
        const int d = std::max(dr, std::max(dg, db));
        if (d > 0)
            {
            result.diff_pixels++;
            result.max_delta = std::max(result.max_delta, d);
            diff[i] = tgx::RGB32(
                uint8_t(std::min(255, 48 + dr * 3)),
                uint8_t(std::min(255, 24 + dg * 3)),
                uint8_t(std::min(255, 24 + db * 3)));
            }
        else
            {
            diff[i] = tgx::RGB32(uint8_t(a.R / 8), uint8_t(a.G / 8), uint8_t(a.B / 8));
            }
        }

    result.ok = (result.diff_pixels == 0);
    if (!result.ok)
        {
        std::filesystem::create_directories(diff_path.parent_path());
        if (writeBMP32Pixels(diff_path, current.w, current.h, diff))
            {
            std::ostringstream oss;
            oss << "diff_pixels=" << result.diff_pixels << " max_delta=" << result.max_delta
                << " diff=" << diff_path.string();
            result.note = oss.str();
            }
        else
            {
            std::ostringstream oss;
            oss << "diff_pixels=" << result.diff_pixels << " max_delta=" << result.max_delta
                << " diff write failed";
            result.note = oss.str();
            }
        }
    return result;
}

template<typename F>
double timeMicroseconds(int iterations, F&& fn)
{
    const auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) fn();
    const auto t1 = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::micro>(t1 - t0).count();
}

template<typename color_t>
void drawBackdrop(tgx::Image<color_t>& im, int x0, int y0, int w, int h, const char* label)
{
    im.fillRect(tgx::iBox2(x0, x0 + w - 1, y0, y0 + h - 1), C<color_t>(22, 28, 38));
    im.drawRect(tgx::iBox2(x0, x0 + w - 1, y0, y0 + h - 1), C<color_t>(80, 100, 125));
    im.drawTextEx(label, { x0 + 8, y0 + 18 }, font_tgx_OpenSans_10, tgx::BASELINE | tgx::LEFT, false, false, C<color_t>(220, 230, 245));
}

template<typename color_t>
void makeSprite(std::vector<color_t>& pixels, int w, int h, bool masked)
{
    tgx::Image<color_t> sp(pixels.data(), w, h);
    sp.fillScreenHGradient(C<color_t>(30, 120, 230), C<color_t>(235, 190, 40));
    sp.fillCircleAA({ 18.0f, 20.0f }, 12.0f, C<color_t>(240, 80, 50), 0.9f);
    sp.drawThickRectAA(tgx::fBox2(1.0f, w - 2.0f, 1.0f, h - 2.0f), 2.0f, C<color_t>(255, 255, 255), 0.8f);
    sp.drawTextEx("TGX", { w / 2, h / 2 + 5 }, font_tgx_OpenSans_Bold_14, tgx::CENTER, false, false, C<color_t>(5, 8, 12), 0.75f);
    if (masked)
        {
        const auto m = C<color_t>(255, 0, 255);
        sp.fillCircleAA({ 8.0f, 8.0f }, 7.0f, m);
        sp.drawThickLineAA({ 7.0f, h - 7.0f }, { w - 7.0f, 7.0f }, 5.0f, tgx::END_ROUNDED, tgx::END_ROUNDED, m);
        }
}

template<typename color_t>
bool validateSamples(tgx::Image<color_t>& im)
{
    const auto pixel_col = C<color_t>(255, 40, 40);
    im.drawPixel({ 3, 3 }, pixel_col);
    if (!(im(3, 3) == pixel_col)) return false;

    const auto rect_col = C<color_t>(40, 220, 120);
    im.fillRect(tgx::iBox2(20, 39, 20, 39), rect_col);
    if (!(im(20, 20) == rect_col)) return false;
    if (!(im(39, 39) == rect_col)) return false;

    std::vector<color_t> spx(4 * 3, C<color_t>(80, 120, 240));
    tgx::Image<color_t> sp(spx.data(), 4, 3);
    im.blit(sp, { 50, 50 }, -1.0f);
    if (!(im(50, 50) == C<color_t>(80, 120, 240))) return false;
    if (!(im(53, 52) == C<color_t>(80, 120, 240))) return false;

    return true;
}

template<>
bool validateSamples<tgx::RGBf>(tgx::Image<tgx::RGBf>& im)
{
    const auto pixel_col = C<tgx::RGBf>(255, 40, 40);
    im.drawPixel({ 3, 3 }, pixel_col);
    if (im(3, 3) != pixel_col) return false;

    const auto rect_col = C<tgx::RGBf>(40, 220, 120);
    im.fillRect(tgx::iBox2(20, 39, 20, 39), rect_col);
    if (im(20, 20) != rect_col) return false;

    std::vector<tgx::RGBf> spx(4 * 3, C<tgx::RGBf>(80, 120, 240));
    tgx::Image<tgx::RGBf> sp(spx.data(), 4, 3);
    im.blit(sp, { 50, 50 }, -1.0f);
    return im(53, 52) == C<tgx::RGBf>(80, 120, 240);
}

template<typename color_t>
bool validateImageStateOps(std::string& note)
{
    const auto bg = C<color_t>(5, 7, 9);
    const auto red = C<color_t>(240, 20, 30);
    const auto green = C<color_t>(20, 230, 90);
    const auto blue = C<color_t>(30, 90, 240);
    const auto outside = C<color_t>(1, 2, 3);

    tgx::Image<color_t> invalid;
    if (invalid.isValid() || invalid.lx() != 0 || invalid.ly() != 0 || invalid.stride() != 0)
        {
        note = "default invalid image state failed";
        return false;
        }

    std::vector<color_t> pixels(8 * 5, bg);
    tgx::Image<color_t> im;
    im.set(pixels.data(), 6, 4, 8);
    if (!im.isValid() || im.lx() != 6 || im.ly() != 4 || im.stride() != 8)
        {
        note = "set() dimensions/stride failed";
        return false;
        }
    const auto box = im.imageBox();
    if ((im.width() != 6) || (im.height() != 4) || (im.dim() != tgx::iVec2{ 6, 4 }) ||
        (box.minX != 0) || (box.maxX != 5) || (box.minY != 0) || (box.maxY != 3))
        {
        note = "attribute accessors failed";
        return false;
        }
    if ((im.data() != pixels.data()) || (static_cast<const tgx::Image<color_t>&>(im).data() != pixels.data()))
        {
        note = "data() accessor failed";
        return false;
        }

    im.drawPixel({ 2, 1 }, red);
    if (!sameColor(im.readPixel({ 2, 1 }), red) || !sameColor(im.readPixel({ -1, 1 }, outside), outside))
        {
        note = "readPixel()/drawPixel() failed";
        return false;
        }
    im.drawPixelf({ 4.4f, 2.4f }, green);
    if (!sameColor(im.readPixelf({ 4.4f, 2.4f }), green))
        {
        note = "readPixelf()/drawPixelf() failed";
        return false;
        }

    auto sub = im(1, 4, 1, 3);
    if (!sub.isValid() || sub.lx() != 4 || sub.ly() != 3 || sub.stride() != 8)
        {
        note = "operator() subimage dimensions failed";
        return false;
        }
    sub.fillScreen(blue);
    if (!sameColor(im(1, 1), blue) || !sameColor(im(4, 3), blue) || !sameColor(im(0, 1), bg))
        {
        note = "subimage shared buffer mapping failed";
        return false;
        }

    auto cropped = im.getCrop(tgx::iBox2(-3, 2, -2, 2));
    if (!cropped.isValid() || cropped.lx() != 3 || cropped.ly() != 3 || cropped.stride() != 8)
        {
        note = "getCrop() clipping failed";
        return false;
        }
    cropped.clear(green);
    if (!sameColor(im(0, 0), green) || !sameColor(im(2, 2), green))
        {
        note = "getCrop() shared buffer failed";
        return false;
        }

    tgx::Image<color_t> cropped_in_place(pixels.data(), 6, 4, 8);
    cropped_in_place.crop(tgx::iBox2(2, 5, 1, 2));
    if (!cropped_in_place.isValid() || cropped_in_place.lx() != 4 || cropped_in_place.ly() != 2 || cropped_in_place.stride() != 8)
        {
        note = "crop() in-place dimensions failed";
        return false;
        }
    cropped_in_place.clear(red);
    if (!sameColor(im(2, 1), red) || !sameColor(im(5, 2), red))
        {
        note = "crop() in-place shared buffer failed";
        return false;
        }

    int visited = 0;
    im.iterate([&](tgx::iVec2, color_t& c)
        {
        c = bg;
        visited++;
        return true;
        }, tgx::iBox2(1, 3, 1, 2));
    if (visited != 6 || !sameColor(im(1, 1), bg) || !sameColor(im(3, 2), bg))
        {
        note = "iterate(box) failed";
        return false;
        }
    const tgx::Image<color_t>& cim = im;
    int const_visited = 0;
    bool const_ok = true;
    cim.iterate([&](tgx::iVec2, const color_t& c)
        {
        const_visited++;
        const_ok = const_ok && sameColor(c, c);
        return const_visited < 5;
        }, tgx::iBox2(1, 3, 1, 2));
    if ((const_visited != 5) || (!const_ok))
        {
        note = "const iterate early-stop failed";
        return false;
        }

    im.setInvalid();
    if (im.isValid() || im.lx() != 0 || im.ly() != 0 || im.stride() != 0)
        {
        note = "setInvalid() failed";
        return false;
        }

    return true;
}

template<typename color_t>
bool validateTransferAndFillOps(std::string& note)
{
    const auto bg = C<color_t>(4, 7, 12);
    const auto border = C<color_t>(230, 230, 230);
    const auto red = C<color_t>(220, 35, 30);
    const auto green = C<color_t>(40, 210, 110);
    const auto blue = C<color_t>(50, 80, 230);

    std::vector<color_t> pixels(48 * 36, bg);
    tgx::Image<color_t> im(pixels.data(), 48, 36);

    im.drawRect(tgx::iBox2(6, 25, 5, 21), border);
    const int fill_stack = im.template fill<1024>({ 10, 10 }, border, red);
    if (fill_stack < 0 || !sameColor(im(10, 10), red) || !sameColor(im(6, 5), border) || !sameColor(im(3, 3), bg))
        {
        note = "border flood fill failed";
        return false;
        }
    const int fill_stack_2 = im.template fill<1024>({ 0, 0 }, blue);
    if (fill_stack_2 < 0 || !sameColor(im(0, 0), blue) || !sameColor(im(5, 5), blue) || !sameColor(im(10, 10), red))
        {
        note = "unicolor flood fill failed";
        return false;
        }

    std::vector<color_t> src_pixels(12 * 8);
    tgx::Image<color_t> src(src_pixels.data(), 12, 8);
    src.fillScreenHGradient(red, green);
    src.fillRect(tgx::iBox2(3, 8, 2, 5), blue);

    std::vector<color_t> reduced_pixels(6 * 4, bg);
    tgx::Image<color_t> reduced(reduced_pixels.data(), 6, 4);
    auto reduced_view = reduced.copyReduceHalf(src);
    if (!reduced_view.isValid() || reduced_view.lx() != 6 || reduced_view.ly() != 4)
        {
        note = "copyReduceHalf() returned invalid dimensions";
        return false;
        }
    if (countChanged(reduced, bg) == 0)
        {
        note = "copyReduceHalf() did not write pixels";
        return false;
        }

    std::vector<color_t> inplace_pixels = src_pixels;
    tgx::Image<color_t> inplace(inplace_pixels.data(), 12, 8);
    auto half = inplace.reduceHalf();
    if (!half.isValid() || half.lx() != 6 || half.ly() != 4)
        {
        note = "reduceHalf() returned invalid dimensions";
        return false;
        }

    std::vector<color_t> back_pixels(5 * 4, bg);
    tgx::Image<color_t> back(back_pixels.data(), 5, 4);
    src.blitBackward(back, { 2, 3 });
    if (!sameColor(back(0, 0), src(2, 3)) || !sameColor(back(4, 3), src(6, 6)))
        {
        note = "blitBackward() source mapping failed";
        return false;
        }

    std::vector<tgx::RGB32> src32_pixels(7 * 5, tgx::RGB32(40, 170, 210));
    tgx::Image<tgx::RGB32> src32(src32_pixels.data(), 7, 5);
    std::vector<color_t> copy_pixels(14 * 10, bg);
    tgx::Image<color_t> copy(copy_pixels.data(), 14, 10);
    copy.copyFrom(src32, -1.0f);
    if (countChanged(copy, bg) == 0)
        {
        note = "copyFrom() color conversion/resizing wrote no pixels";
        return false;
        }

    copy.clear(bg);
    copy.blit(src32, { 2, 2 }, [](tgx::RGB32, color_t) { return color_t(tgx::RGB32(200, 40, 160)); });
    if (!sameColor(copy(2, 2), C<color_t>(200, 40, 160)))
        {
        note = "custom blit operator failed";
        return false;
        }

    return true;
}

bool validateConvertOps(std::string& note)
{
    std::vector<tgx::RGB32> rgba(5 * 4, tgx::RGB32(12, 80, 220, 255));
    tgx::Image<tgx::RGB32> im32(rgba.data(), 5, 4);
    auto im565 = im32.convert<tgx::RGB565>();
    if (!im565.isValid() || im565.lx() != 5 || im565.ly() != 4 || im565.stride() != 5)
        {
        note = "RGB32 to RGB565 convert() dimensions/stride failed";
        return false;
        }
    if (!sameColor(im565(0, 0), tgx::RGB565(tgx::RGB32(12, 80, 220))))
        {
        note = "RGB32 to RGB565 convert() pixel failed";
        return false;
        }

    std::vector<tgx::RGBf> rgbf(4 * 3, tgx::RGBf(0.25f, 0.5f, 0.75f));
    tgx::Image<tgx::RGBf> imf(rgbf.data(), 4, 3);
    auto imf32 = imf.convert<tgx::RGB32>();
    if (!imf32.isValid() || imf32.lx() != 4 || imf32.ly() != 3 || imf32.stride() != 4)
        {
        note = "RGBf to RGB32 convert() dimensions/stride failed";
        return false;
        }
    if (countChanged(imf32, tgx::RGB32(0, 0, 0)) != 12)
        {
        note = "RGBf to RGB32 convert() pixel conversion failed";
        return false;
        }

    return true;
}

template<typename color_t>
color_t idColor(int id)
{
    return C<color_t>((id * 53 + 31) & 255, (id * 97 + 43) & 255, (id * 151 + 59) & 255);
}

template<typename color_t>
bool validateRotatedBlits(std::string& note)
{
    const auto bg = C<color_t>(3, 5, 7);
    std::vector<color_t> src_pixels(4 * 3);
    for (int y = 0; y < 3; y++)
        {
        for (int x = 0; x < 4; x++) src_pixels[x + 4 * y] = idColor<color_t>(1 + x + 4 * y);
        }
    tgx::Image<color_t> src(src_pixels.data(), 4, 3);

    auto checkPixel = [&](const tgx::Image<color_t>& dst, int x, int y, color_t expected, const char* label)
        {
        if (!sameColor(dst(x, y), expected))
            {
            note = label;
            return false;
            }
        return true;
        };

    for (int angle : { 0, 90, 180, 270 })
        {
        std::vector<color_t> dst_pixels(8 * 8, bg);
        tgx::Image<color_t> dst(dst_pixels.data(), 8, 8);
        dst.blitRotated(src, { 2, 2 }, angle, -1.0f);

        if (angle == 0)
            {
            for (int y = 0; y < 3; y++)
                for (int x = 0; x < 4; x++)
                    if (!checkPixel(dst, 2 + x, 2 + y, src(x, y), "blitRotated 0 mapping failed")) return false;
            }
        else if (angle == 90)
            {
            for (int y = 0; y < 4; y++)
                for (int x = 0; x < 3; x++)
                    if (!checkPixel(dst, 2 + x, 2 + y, src(y, 2 - x), "blitRotated 90 mapping failed")) return false;
            }
        else if (angle == 180)
            {
            for (int y = 0; y < 3; y++)
                for (int x = 0; x < 4; x++)
                    if (!checkPixel(dst, 2 + x, 2 + y, src(3 - x, 2 - y), "blitRotated 180 mapping failed")) return false;
            }
        else
            {
            for (int y = 0; y < 4; y++)
                for (int x = 0; x < 3; x++)
                    if (!checkPixel(dst, 2 + x, 2 + y, src(3 - y, x), "blitRotated 270 mapping failed")) return false;
            }

        if (!sameColor(dst(0, 0), bg) || !sameColor(dst(7, 7), bg))
            {
            note = "blitRotated wrote outside expected region";
            return false;
            }
        }

    std::vector<tgx::RGB32> src32_pixels(4 * 3, tgx::RGB32(10, 20, 30));
    tgx::Image<tgx::RGB32> src32(src32_pixels.data(), 4, 3);
    std::vector<color_t> dst_pixels(8 * 8, bg);
    tgx::Image<color_t> dst(dst_pixels.data(), 8, 8);
    const auto custom = C<color_t>(210, 40, 170);
    dst.blitRotated(src32, { 2, 2 }, 90, [&](tgx::RGB32, color_t) { return custom; });
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 3; x++)
            if (!sameColor(dst(2 + x, 2 + y), custom))
                {
                note = "custom blitRotated operator failed";
                return false;
                }

    return true;
}

template<typename color_t>
bool validateTextLayout(std::string& note)
{
    std::vector<color_t> pixels(96 * 70, bgColor<color_t>());
    tgx::Image<color_t> im(pixels.data(), 96, 70);
    const char* text = "TGX text";

    const auto left = im.measureText(text, { 14, 24 }, font_tgx_OpenSans_10, tgx::BASELINE | tgx::LEFT, false, false);
    const auto center = im.measureText(text, { 48, 24 }, font_tgx_OpenSans_10, tgx::BASELINE | tgx::CENTER, false, false);
    const auto right = im.measureText(text, { 82, 24 }, font_tgx_OpenSans_10, tgx::BASELINE | tgx::RIGHT, false, false);
    if (left.minX != 14 || center.minX >= 48 || center.maxX <= 48 || right.maxX != 82)
        {
        note = "measureText anchor placement failed";
        return false;
        }

    const tgx::iVec2 anchor_pos{ 48, 34 };
    const auto top_left = im.measureText(text, anchor_pos, font_tgx_OpenSans_10, tgx::TOPLEFT, false, false);
    const auto top_right = im.measureText(text, anchor_pos, font_tgx_OpenSans_10, tgx::TOPRIGHT, false, false);
    const auto bottom_right = im.measureText(text, anchor_pos, font_tgx_OpenSans_10, tgx::BOTTOMRIGHT, false, false);
    const auto centered = im.measureText(text, anchor_pos, font_tgx_OpenSans_10, tgx::CENTER, false, false);
    if ((top_left.minX != anchor_pos.x) || (top_left.minY != anchor_pos.y) ||
        (top_right.maxX != anchor_pos.x) || (top_right.minY != anchor_pos.y) ||
        (bottom_right.maxX != anchor_pos.x) || (bottom_right.maxY != anchor_pos.y) ||
        (centered.minX >= anchor_pos.x) || (centered.maxX <= anchor_pos.x) ||
        (centered.minY >= anchor_pos.y) || (centered.maxY <= anchor_pos.y))
        {
        note = "measureText non-baseline anchors failed";
        return false;
        }

    const auto nowrap = im.measureText("wrap wrap wrap wrap", { 70, 14 }, font_tgx_OpenSans_10, tgx::BASELINE | tgx::LEFT, false, false);
    const auto wrap = im.measureText("wrap wrap wrap wrap", { 70, 14 }, font_tgx_OpenSans_10, tgx::BASELINE | tgx::LEFT, true, false);
    if ((wrap.maxY - wrap.minY) <= (nowrap.maxY - nowrap.minY))
        {
        note = "measureText wrapping did not increase text height";
        return false;
        }

    int line_advance = 0;
    im.measureChar('A', { 14, 24 }, font_tgx_OpenSans_10, tgx::DEFAULT_TEXT_ANCHOR, &line_advance);
    const int line_height = im.fontHeight(font_tgx_OpenSans_10);
    if (line_advance <= 0 || line_height <= 0)
        {
        note = "font metrics unavailable for text layout validation";
        return false;
        }

    const auto multiline_local = im.measureText("A\nA", { 14, 24 }, font_tgx_OpenSans_10, tgx::BASELINE | tgx::LEFT, false, false);
    const auto multiline_zero = im.measureText("A\nA", { 14, 24 }, font_tgx_OpenSans_10, tgx::BASELINE | tgx::LEFT, false, true);
    if ((multiline_local.maxY - multiline_local.minY) < line_height ||
        (multiline_zero.minX != 0) || (multiline_zero.maxY != multiline_local.maxY))
        {
        note = "measureText multiline start_newline_at_0 failed";
        return false;
        }

    im.clear(bgColor<color_t>());
    auto cursor_local = im.drawTextEx("A\nA", { 14, 24 }, font_tgx_OpenSans_10, tgx::DEFAULT_TEXT_ANCHOR, false, false, C<color_t>(240, 240, 240), -1.0f);
    auto cursor_zero = im.drawTextEx("A\nA", { 14, 24 }, font_tgx_OpenSans_10, tgx::DEFAULT_TEXT_ANCHOR, false, true, C<color_t>(240, 240, 240), -1.0f);
    if ((cursor_local.x != 14 + line_advance) || (cursor_local.y != 24 + line_height) ||
        (cursor_zero.x != line_advance) || (cursor_zero.y != cursor_local.y))
        {
        note = "drawTextEx newline cursor failed";
        return false;
        }

    int wide_advance = 0;
    im.measureChar('W', { 0, 24 }, font_tgx_OpenSans_10, tgx::DEFAULT_TEXT_ANCHOR, &wide_advance);
    if (wide_advance <= 0)
        {
        note = "wide glyph advance unavailable";
        return false;
        }
    const int wrap_start_x = im.lx() - (wide_advance / 2);
    auto wrap_cursor = im.drawTextEx("W", { wrap_start_x, 24 }, font_tgx_OpenSans_10, tgx::DEFAULT_TEXT_ANCHOR, true, true, C<color_t>(240, 240, 240), -1.0f);
    if ((wrap_cursor.x != wide_advance) || (wrap_cursor.y != 24 + line_height))
        {
        note = "drawTextEx wrap to x=0 failed";
        return false;
        }

    const int before = countChanged(im, bgColor<color_t>());
    auto cursor = im.drawTextEx(text, { 14, 24 }, font_tgx_OpenSans_10, tgx::BASELINE | tgx::LEFT, false, false, C<color_t>(240, 240, 240), -1.0f);
    const int after = countChanged(im, bgColor<color_t>());
    if (after <= before || cursor.x <= 14)
        {
        note = "drawTextEx did not draw or advance cursor";
        return false;
        }

    auto deprecated_box = im.measureText(text, { 14, 24 }, font_tgx_OpenSans_10, false);
    auto modern_box = im.measureText(text, { 14, 24 }, font_tgx_OpenSans_10, tgx::DEFAULT_TEXT_ANCHOR, false, false);
    if ((deprecated_box.minX != modern_box.minX) || (deprecated_box.maxX != modern_box.maxX) ||
        (deprecated_box.minY != modern_box.minY) || (deprecated_box.maxY != modern_box.maxY))
        {
        note = "deprecated measureText overload mismatch";
        return false;
        }

    im.clear(bgColor<color_t>());
    cursor = im.drawTextEx(text, { 14, 24 }, font_tgx_OpenSans_10, tgx::BASELINE | tgx::LEFT, false, false, C<color_t>(240, 240, 240), 0.0f);
    if ((countChanged(im, bgColor<color_t>()) != 0) || (cursor.x <= 14))
        {
        note = "opacity 0.0 drawTextEx modified pixels or failed to advance";
        return false;
        }

    std::vector<color_t> modern_pixels(96 * 70, bgColor<color_t>());
    std::vector<color_t> legacy_pixels(96 * 70, bgColor<color_t>());
    tgx::Image<color_t> modern(modern_pixels.data(), 96, 70);
    tgx::Image<color_t> legacy(legacy_pixels.data(), 96, 70);
    const auto modern_cursor = modern.drawTextEx(text, { 14, 24 }, font_tgx_OpenSans_10, tgx::DEFAULT_TEXT_ANCHOR, false, false, C<color_t>(240, 240, 240), -1.0f);
    const auto legacy_cursor = legacy.drawText(text, { 14, 24 }, C<color_t>(240, 240, 240), font_tgx_OpenSans_10, false, -1.0f);
    if ((modern_cursor != legacy_cursor) || (imageHash(modern) != imageHash(legacy)))
        {
        note = "deprecated drawText overload image mismatch";
        return false;
        }

    return true;
}

template<typename color_t>
bool validatePrimitiveExact(std::string& note)
{
    const auto bg = bgColor<color_t>();
    const auto red = C<color_t>(230, 30, 35);
    const auto green = C<color_t>(35, 220, 95);
    const auto blue = C<color_t>(45, 95, 230);
    const auto yellow = C<color_t>(240, 210, 55);

    std::vector<color_t> pixels(16 * 16, bg);
    tgx::Image<color_t> im(pixels.data(), 16, 16);

    im.drawPixel({ -1, 2 }, red);
    im.drawPixel({ 16, 2 }, red);
    if (countChanged(im, bg) != 0)
        {
        note = "drawPixel outside image wrote pixels";
        return false;
        }

    im.drawFastHLine({ 2, 5 }, 5, red);
    if (countChanged(im, bg) != 5 || !sameColor(im(2, 5), red) || !sameColor(im(6, 5), red) || !sameColor(im(7, 5), bg))
        {
        note = "drawFastHLine exact pixels failed";
        return false;
        }

    im.clear(bg);
    im.drawFastVLine({ 7, 1 }, 4, green);
    if (countChanged(im, bg) != 4 || !sameColor(im(7, 1), green) || !sameColor(im(7, 4), green) || !sameColor(im(7, 5), bg))
        {
        note = "drawFastVLine exact pixels failed";
        return false;
        }

    im.clear(bg);
    im.drawLine({ 1, 1 }, { 6, 1 }, blue);
    if (countChanged(im, bg) != 6 || !sameColor(im(1, 1), blue) || !sameColor(im(6, 1), blue))
        {
        note = "drawLine horizontal exact pixels failed";
        return false;
        }

    im.clear(bg);
    im.drawSegment({ 1, 3 }, false, { 6, 3 }, false, yellow);
    if (countChanged(im, bg) != 4 || !sameColor(im(1, 3), bg) || !sameColor(im(2, 3), yellow) ||
        !sameColor(im(5, 3), yellow) || !sameColor(im(6, 3), bg))
        {
        note = "drawSegment endpoint flags failed";
        return false;
        }

    im.clear(bg);
    im.drawRect(tgx::iBox2(2, 6, 3, 7), red);
    if (countChanged(im, bg) != 16 || !sameColor(im(2, 3), red) || !sameColor(im(6, 7), red) || !sameColor(im(4, 5), bg))
        {
        note = "drawRect perimeter failed";
        return false;
        }

    im.clear(bg);
    im.fillRect(tgx::iBox2(-2, 3, -1, 2), green);
    if (countChanged(im, bg) != 12 || !sameColor(im(0, 0), green) || !sameColor(im(3, 2), green) || !sameColor(im(4, 2), bg))
        {
        note = "fillRect clipping failed";
        return false;
        }

    im.clear(bg);
    im.fillThickRect(tgx::iBox2(2, 9, 2, 9), 2, blue, red);
    if (!sameColor(im(2, 2), red) || !sameColor(im(3, 3), red) || !sameColor(im(4, 4), blue) || !sameColor(im(9, 9), red))
        {
        note = "fillThickRect border/interior failed";
        return false;
        }

    return true;
}

template<typename color_t>
bool validateClipAndBlit(std::string& note)
{
    const auto bg = bgColor<color_t>();
    const auto red = C<color_t>(230, 30, 35);
    const auto green = C<color_t>(35, 220, 95);
    const auto blue = C<color_t>(45, 95, 230);
    const auto mask = C<color_t>(255, 0, 255);

    std::vector<color_t> pixels(10 * 8, bg);
    tgx::Image<color_t> im(pixels.data(), 10, 8);

    im.drawFastHLine({ -5, 2 }, 8, red);
    if (countChanged(im, bg) != 3 || !sameColor(im(0, 2), red) || !sameColor(im(2, 2), red) || !sameColor(im(3, 2), bg))
        {
        note = "clipped drawFastHLine failed";
        return false;
        }

    im.clear(bg);
    im.drawFastVLine({ 4, -4 }, 7, green);
    if (countChanged(im, bg) != 3 || !sameColor(im(4, 0), green) || !sameColor(im(4, 2), green) || !sameColor(im(4, 3), bg))
        {
        note = "clipped drawFastVLine failed";
        return false;
        }

    std::vector<color_t> src_pixels(4 * 3);
    for (int y = 0; y < 3; y++)
        for (int x = 0; x < 4; x++)
            src_pixels[x + 4 * y] = idColor<color_t>(1 + x + 4 * y);
    tgx::Image<color_t> src(src_pixels.data(), 4, 3);

    im.clear(bg);
    im.blit(src, { -2, -1 }, -1.0f);
    if (countChanged(im, bg) != 4 || !sameColor(im(0, 0), src(2, 1)) || !sameColor(im(1, 0), src(3, 1)) ||
        !sameColor(im(0, 1), src(2, 2)) || !sameColor(im(1, 1), src(3, 2)) || !sameColor(im(2, 0), bg))
        {
        note = "clipped blit mapping failed";
        return false;
        }

    std::vector<color_t> masked_pixels = {
        red, mask, blue,
        mask, green, red
    };
    tgx::Image<color_t> masked(masked_pixels.data(), 3, 2);
    im.clear(bg);
    im.blitMasked(masked, mask, { 8, 7 }, -1.0f);
    if (countChanged(im, bg) != 1 || !sameColor(im(8, 7), red) || !sameColor(im(9, 7), bg))
        {
        note = "clipped blitMasked failed";
        return false;
        }

    im.clear(bg);
    im.blit(src, { 2, 2 }, [](color_t, color_t) { return C<color_t>(12, 34, 56); });
    if (countChanged(im, bg) != 12 || !sameColor(im(2, 2), C<color_t>(12, 34, 56)) || !sameColor(im(5, 4), C<color_t>(12, 34, 56)))
        {
        note = "custom same-color blit operator failed";
        return false;
        }

    return true;
}

template<typename color_t>
bool validateImageEdgeCases(std::string& note)
{
    const auto bg = bgColor<color_t>();
    const auto red = C<color_t>(230, 30, 35);
    const auto green = C<color_t>(35, 220, 95);
    const auto blue = C<color_t>(45, 95, 230);

    tgx::Image<color_t> null_buffer(static_cast<color_t*>(nullptr), 4, 4);
    if (null_buffer.isValid())
        {
        note = "null buffer image should be invalid";
        return false;
        }

    std::vector<color_t> pixels(12 * 8, bg);
    tgx::Image<color_t> invalid_stride(pixels.data(), 8, 4, 7);
    if (invalid_stride.isValid())
        {
        note = "stride smaller than width should be invalid";
        return false;
        }

    tgx::Image<color_t> im(pixels.data(), 10, 8, 12);
    auto outside_crop = im.getCrop(tgx::iBox2(20, 25, 1, 3));
    if (outside_crop.isValid())
        {
        note = "fully outside getCrop() should be invalid";
        return false;
        }

    auto crop1 = im.getCrop(tgx::iBox2(2, 8, 1, 6));
    auto crop2 = crop1.getCrop(tgx::iBox2(1, 3, 2, 4));
    if (!crop2.isValid() || crop2.lx() != 3 || crop2.ly() != 3 || crop2.stride() != 12)
        {
        note = "nested crop dimensions/stride failed";
        return false;
        }
    crop2.clear(red);
    if (!sameColor(im(3, 3), red) || !sameColor(im(5, 5), red) || !sameColor(im(2, 3), bg) || !sameColor(im(6, 5), bg))
        {
        note = "nested crop buffer mapping failed";
        return false;
        }

    int visited = 0;
    im.iterate([&](tgx::iVec2, color_t& c)
        {
        c = green;
        visited++;
        return true;
        }, tgx::iBox2(-2, 2, -3, 1));
    if (visited != 6 || !sameColor(im(0, 0), green) || !sameColor(im(2, 1), green))
        {
        note = "iterate clipped box count/mapping failed";
        return false;
        }

    std::vector<color_t> src_odd_pixels(5 * 3, blue);
    std::vector<color_t> dst_odd_pixels(2 * 1, bg);
    tgx::Image<color_t> src_odd(src_odd_pixels.data(), 5, 3);
    tgx::Image<color_t> dst_odd(dst_odd_pixels.data(), 2, 1);
    auto reduced_odd = dst_odd.copyReduceHalf(src_odd);
    if (!reduced_odd.isValid() || reduced_odd.lx() != 2 || reduced_odd.ly() != 1 ||
        !sameColor(dst_odd(0, 0), blue) || !sameColor(dst_odd(1, 0), blue))
        {
        note = "copyReduceHalf() odd dimensions failed";
        return false;
        }

    std::vector<color_t> too_small_pixels(1, bg);
    tgx::Image<color_t> too_small(too_small_pixels.data(), 1, 1);
    if (too_small.copyReduceHalf(src_odd).isValid())
        {
        note = "copyReduceHalf() too-small destination should fail";
        return false;
        }

    std::vector<color_t> one_col_pixels(1 * 5, red);
    std::vector<color_t> one_col_dst_pixels(1 * 2, bg);
    tgx::Image<color_t> one_col(one_col_pixels.data(), 1, 5);
    tgx::Image<color_t> one_col_dst(one_col_dst_pixels.data(), 1, 2);
    auto one_col_reduced = one_col_dst.copyReduceHalf(one_col);
    if (!one_col_reduced.isValid() || one_col_reduced.lx() != 1 || one_col_reduced.ly() != 2 ||
        !sameColor(one_col_dst(0, 0), red) || !sameColor(one_col_dst(0, 1), red))
        {
        note = "copyReduceHalf() one-column path failed";
        return false;
        }

    std::vector<color_t> one_row_pixels(5, green);
    std::vector<color_t> one_row_dst_pixels(2, bg);
    tgx::Image<color_t> one_row(one_row_pixels.data(), 5, 1);
    tgx::Image<color_t> one_row_dst(one_row_dst_pixels.data(), 2, 1);
    auto one_row_reduced = one_row_dst.copyReduceHalf(one_row);
    if (!one_row_reduced.isValid() || one_row_reduced.lx() != 2 || one_row_reduced.ly() != 1 ||
        !sameColor(one_row_dst(0, 0), green) || !sameColor(one_row_dst(1, 0), green))
        {
        note = "copyReduceHalf() one-row path failed";
        return false;
        }

    return true;
}

template<typename color_t>
bool validateGradientAndOpacity(std::string& note)
{
    const auto bg = bgColor<color_t>();
    const auto red = C<color_t>(230, 30, 35);
    const auto green = C<color_t>(35, 220, 95);
    const auto blue = C<color_t>(45, 95, 230);

    std::vector<color_t> pixels(8 * 6, bg);
    tgx::Image<color_t> im(pixels.data(), 8, 6);

    im.fillScreenHGradient(red, blue);
    if (!nearColor(im(0, 0), red) || !nearColor(im(7, 0), blue) || !nearColor(im(0, 5), red) || !nearColor(im(7, 5), blue))
        {
        note = "fillScreenHGradient endpoints failed";
        return false;
        }

    im.clear(bg);
    im.fillScreenVGradient(green, blue);
    if (!nearColor(im(0, 0), green) || !nearColor(im(7, 0), green) || !nearColor(im(0, 5), blue) || !nearColor(im(7, 5), blue))
        {
        note = "fillScreenVGradient endpoints failed";
        return false;
        }

    im.clear(bg);
    im.fillRectHGradient(tgx::iBox2(-2, 3, 1, 2), red, green);
    if (!nearColor(im(0, 1), red) || !nearColor(im(3, 2), green) || !sameColor(im(4, 1), bg))
        {
        note = "fillRectHGradient clipping/endpoints failed";
        return false;
        }

    im.clear(bg);
    im.fillRectVGradient(tgx::iBox2(2, 5, -1, 3), red, blue);
    if (!nearColor(im(2, 0), red) || !nearColor(im(5, 3), blue) || !sameColor(im(1, 0), bg))
        {
        note = "fillRectVGradient clipping/endpoints failed";
        return false;
        }

    im.clear(bg);
    im.drawFastHLine({ 1, 1 }, 4, red, 0.0f);
    if (countChanged(im, bg) != 0)
        {
        note = "opacity 0.0 line modified pixels";
        return false;
        }

    im.drawFastVLine({ 2, 0 }, 4, green, 1.0f);
    if (countChanged(im, bg) != 4 || !sameColor(im(2, 0), green) || !sameColor(im(2, 3), green))
        {
        note = "opacity 1.0 line failed";
        return false;
        }

    im.fillRect(tgx::iBox2(0, 3, 4, 5), blue, 0.0f);
    if (countChanged(im, bg) != 4)
        {
        note = "opacity 0.0 fillRect modified pixels";
        return false;
        }

    return true;
}

template<typename color_t>
bool validateLineRectExact(std::string& note)
{
    const auto bg = bgColor<color_t>();
    const auto red = C<color_t>(230, 30, 35);
    const auto green = C<color_t>(35, 220, 95);
    const auto blue = C<color_t>(45, 95, 230);

    std::vector<color_t> pixels(16 * 16, bg);
    tgx::Image<color_t> im(pixels.data(), 16, 16);

    im.drawLine({ 1, 1 }, { 6, 6 }, red);
    if (countChanged(im, bg) != 6)
        {
        note = "45-degree drawLine changed count failed";
        return false;
        }
    for (int i = 1; i <= 6; i++)
        {
        if (!sameColor(im(i, i), red))
            {
            note = "45-degree drawLine pixel mapping failed";
            return false;
            }
        }
    if (!sameColor(im(2, 3), bg) || !sameColor(im(6, 5), bg))
        {
        note = "45-degree drawLine wrote off diagonal";
        return false;
        }

    im.clear(bg);
    im.drawLine({ 6, 6 }, { 1, 1 }, red);
    if (countChanged(im, bg) != 6 || !sameColor(im(1, 1), red) || !sameColor(im(6, 6), red))
        {
        note = "reversed 45-degree drawLine failed";
        return false;
        }

    im.clear(bg);
    im.drawLine({ 4, 4 }, { 4, 4 }, green);
    if (countChanged(im, bg) != 1 || !sameColor(im(4, 4), green))
        {
        note = "single-point drawLine failed";
        return false;
        }

    im.clear(bg);
    im.drawSegment({ 1, 1 }, false, { 6, 6 }, true, blue);
    if (countChanged(im, bg) != 5 || !sameColor(im(1, 1), bg) || !sameColor(im(2, 2), blue) || !sameColor(im(6, 6), blue))
        {
        note = "diagonal drawSegment endpoint exclusion failed";
        return false;
        }

    im.clear(bg);
    im.drawLine({ -3, 7 }, { 3, 7 }, red);
    if (countChanged(im, bg) != 4 || !sameColor(im(0, 7), red) || !sameColor(im(3, 7), red) || !sameColor(im(4, 7), bg))
        {
        note = "clipped horizontal drawLine failed";
        return false;
        }

    im.clear(bg);
    im.drawRect(tgx::iBox2(2, 7, 4, 4), green);
    if (countChanged(im, bg) != 6 || !sameColor(im(2, 4), green) || !sameColor(im(7, 4), green) || !sameColor(im(8, 4), bg))
        {
        note = "one-row drawRect failed";
        return false;
        }

    im.clear(bg);
    im.drawRect(tgx::iBox2(5, 5, 2, 8), blue);
    if (countChanged(im, bg) != 7 || !sameColor(im(5, 2), blue) || !sameColor(im(5, 8), blue) || !sameColor(im(6, 5), bg))
        {
        note = "one-column drawRect failed";
        return false;
        }

    im.clear(bg);
    im.drawThickRect(tgx::iBox2(2, 3, 2, 6), 3, red);
    if (countChanged(im, bg) != 10 || !sameColor(im(2, 2), red) || !sameColor(im(3, 6), red))
        {
        note = "tiny drawThickRect fill fallback failed";
        return false;
        }

    im.clear(bg);
    im.drawLine({ 1, 1 }, { 6, 6 }, red, 0.0f);
    if (countChanged(im, bg) != 0)
        {
        note = "opacity 0.0 drawLine modified pixels";
        return false;
        }

    return true;
}

template<typename color_t>
bool validateTriangleQuadExact(std::string& note)
{
    const auto bg = bgColor<color_t>();
    const auto red = C<color_t>(230, 30, 35);
    const auto green = C<color_t>(35, 220, 95);
    const auto blue = C<color_t>(45, 95, 230);
    const auto yellow = C<color_t>(240, 210, 55);

    std::vector<color_t> pixels(16 * 16, bg);
    tgx::Image<color_t> im(pixels.data(), 16, 16);

    im.fillQuad({ 2, 3 }, { 8, 3 }, { 8, 7 }, { 2, 7 }, red);
    if (countChanged(im, bg) != 35 || !nearColor(im(2, 3), red) || !nearColor(im(8, 7), red) ||
        !nearColor(im(5, 5), red) || !sameColor(im(9, 5), bg) || !sameColor(im(5, 8), bg))
        {
        note = "axis-aligned fillQuad rectangle path failed";
        return false;
        }

    im.clear(bg);
    im.drawQuad({ 2, 3 }, { 8, 3 }, { 8, 7 }, { 2, 7 }, green);
    if (!nearColor(im(2, 3), green) || !nearColor(im(8, 3), green) || !nearColor(im(8, 7), green) ||
        !nearColor(im(2, 7), green) || !sameColor(im(5, 5), bg))
        {
        note = "drawQuad rectangular corners/interior failed";
        return false;
        }

    im.clear(bg);
    im.fillTriangle({ 2, 2 }, { 9, 2 }, { 2, 9 }, blue, yellow);
    if (!nearColor(im(2, 2), yellow) || !nearColor(im(9, 2), yellow) || !nearColor(im(2, 9), yellow) ||
        !nearColor(im(3, 4), blue) || !sameColor(im(10, 9), bg))
        {
        note = "fillTriangle simple right triangle failed";
        return false;
        }

    const uint64_t tri_hash = imageHash(im);
    im.clear(bg);
    im.fillTriangle({ 2, 2 }, { 2, 9 }, { 9, 2 }, blue, yellow);
    if (imageHash(im) != tri_hash)
        {
        note = "fillTriangle orientation changed raster";
        return false;
        }

    im.clear(bg);
    im.drawTriangle({ 2, 2 }, { 9, 2 }, { 2, 9 }, red);
    if (!nearColor(im(2, 2), red) || !nearColor(im(9, 2), red) || !nearColor(im(2, 9), red) ||
        !sameColor(im(3, 4), bg) || countChanged(im, bg) == 0)
        {
        note = "drawTriangle simple outline failed";
        return false;
        }

    im.clear(bg);
    im.fillQuad({ -3, -2 }, { 4, -2 }, { 4, 4 }, { -3, 4 }, green);
    if (countChanged(im, bg) != 25 || !nearColor(im(0, 0), green) || !nearColor(im(4, 4), green) || !sameColor(im(5, 4), bg))
        {
        note = "clipped axis-aligned fillQuad failed";
        return false;
        }

    return true;
}

template<typename color_t>
bool validateCircleRoundExact(std::string& note)
{
    const auto bg = bgColor<color_t>();
    const auto red = C<color_t>(230, 30, 35);
    const auto green = C<color_t>(35, 220, 95);
    const auto blue = C<color_t>(45, 95, 230);
    const auto yellow = C<color_t>(240, 210, 55);

    std::vector<color_t> pixels(20 * 18, bg);
    tgx::Image<color_t> im(pixels.data(), 20, 18);

    im.drawCircle({ 5, 5 }, 1, red);
    if (countChanged(im, bg) != 4 || !sameColor(im(4, 5), red) || !sameColor(im(6, 5), red) ||
        !sameColor(im(5, 4), red) || !sameColor(im(5, 6), red) || !sameColor(im(5, 5), bg))
        {
        note = "drawCircle radius 1 exact pixels failed";
        return false;
        }

    im.clear(bg);
    im.fillCircle({ 5, 5 }, 1, green, red);
    if (countChanged(im, bg) != 5 || !sameColor(im(5, 5), green) || !sameColor(im(4, 5), red) ||
        !sameColor(im(6, 5), red) || !sameColor(im(5, 4), red) || !sameColor(im(5, 6), red))
        {
        note = "fillCircle radius 1 exact pixels failed";
        return false;
        }

    im.clear(bg);
    im.drawCircle({ 8, 8 }, 4, blue);
    if (!sameColor(im(4, 8), blue) || !sameColor(im(12, 8), blue) ||
        !sameColor(im(8, 4), blue) || !sameColor(im(8, 12), blue) ||
        !sameColor(im(8, 8), bg) || !sameColor(im(3, 8), bg))
        {
        note = "drawCircle cardinal/clipping pixels failed";
        return false;
        }

    im.clear(bg);
    im.fillCircle({ 8, 8 }, 4, green, red);
    if (!sameColor(im(8, 8), green) || !sameColor(im(4, 8), red) || !sameColor(im(12, 8), red) ||
        !sameColor(im(8, 4), red) || !sameColor(im(8, 12), red) || !sameColor(im(3, 8), bg))
        {
        note = "fillCircle interior/outline pixels failed";
        return false;
        }

    im.clear(bg);
    im.drawCircle({ 1, 1 }, 4, blue);
    if (!sameColor(im(1, 5), blue) || !sameColor(im(5, 1), blue) || !sameColor(im(6, 1), bg))
        {
        note = "clipped drawCircle cardinal pixels failed";
        return false;
        }

    im.clear(bg);
    im.drawEllipse({ 9, 8 }, { 5, 3 }, yellow);
    if (!sameColor(im(4, 8), yellow) || !sameColor(im(14, 8), yellow) ||
        !sameColor(im(9, 5), yellow) || !sameColor(im(9, 11), yellow) ||
        !sameColor(im(9, 8), bg))
        {
        note = "drawEllipse cardinal pixels failed";
        return false;
        }

    im.clear(bg);
    im.fillEllipse({ 9, 8 }, { 5, 3 }, green, red);
    if (!sameColor(im(9, 8), green) || !sameColor(im(4, 8), red) || !sameColor(im(14, 8), red) ||
        !sameColor(im(9, 5), red) || !sameColor(im(9, 11), red) || !sameColor(im(3, 8), bg))
        {
        note = "fillEllipse interior/outline pixels failed";
        return false;
        }

    im.clear(bg);
    im.drawRoundRect(tgx::iBox2(2, 15, 2, 13), 3, blue);
    if (!sameColor(im(5, 2), blue) || !sameColor(im(12, 2), blue) ||
        !sameColor(im(2, 5), blue) || !sameColor(im(15, 10), blue) ||
        !sameColor(im(8, 8), bg) || !sameColor(im(2, 2), bg))
        {
        note = "drawRoundRect edge/corner pixels failed";
        return false;
        }

    im.clear(bg);
    im.fillRoundRect(tgx::iBox2(2, 15, 2, 13), 3, green);
    if (!sameColor(im(8, 8), green) || !sameColor(im(5, 2), green) || !sameColor(im(2, 5), green) ||
        !sameColor(im(2, 2), bg))
        {
        note = "fillRoundRect center/edge/corner pixels failed";
        return false;
        }

    im.clear(bg);
    im.fillCircle({ 8, 8 }, 4, red, blue, 0.0f);
    im.fillEllipse({ 9, 8 }, { 5, 3 }, red, blue, 0.0f);
    im.fillRoundRect(tgx::iBox2(2, 15, 2, 13), 3, red, 0.0f);
    if (countChanged(im, bg) != 0)
        {
        note = "opacity 0.0 round primitives modified pixels";
        return false;
        }

    return true;
}

template<typename color_t>
bool validateAAPrimitives(std::string& note)
{
    const auto bg = bgColor<color_t>();
    const auto red = C<color_t>(230, 30, 35);
    const auto green = C<color_t>(35, 220, 95);
    const auto blue = C<color_t>(45, 95, 230);
    const auto yellow = C<color_t>(240, 210, 55);
    const auto cyan = C<color_t>(60, 220, 240);

    std::vector<color_t> pixels(48 * 48, bg);
    tgx::Image<color_t> im(pixels.data(), 48, 48);

    im.drawLineAA({ 5.2f, 5.4f }, { 40.7f, 30.8f }, red);
    if (countChanged(im, bg) < 25 || !sameColor(im(0, 0), bg) || !sameColor(im(47, 47), bg))
        {
        note = "drawLineAA did not draw expected clipped footprint";
        return false;
        }

    im.clear(bg);
    im.drawLineAA({ -12.5f, 10.2f }, { 20.2f, 10.2f }, green);
    if (countChanged(im, bg) < 15 || !sameColor(im(47, 10), bg))
        {
        note = "clipped drawLineAA failed";
        return false;
        }

    im.clear(bg);
    im.drawLineAA({ 5.2f, 5.4f }, { 40.7f, 30.8f }, red, 0.0f);
    im.drawThickLineAA({ 8.0f, 10.0f }, { 42.0f, 10.0f }, 6.0f, tgx::END_ROUNDED, tgx::END_ROUNDED, red, 0.0f);
    im.drawWedgeLineAA({ 8.0f, 20.0f }, { 42.0f, 24.0f }, 2.0f, tgx::END_ROUNDED, 8.0f, tgx::END_ROUNDED, red, 0.0f);
    if (countChanged(im, bg) != 0)
        {
        note = "opacity 0.0 AA lines modified pixels";
        return false;
        }

    im.clear(bg);
    im.drawThickLineAA({ 7.0f, 12.0f }, { 41.0f, 12.0f }, 6.0f, tgx::END_ROUNDED, tgx::END_ARROW_2, blue);
    const int thick_line_pixels = countChanged(im, bg);
    if (thick_line_pixels < 150 || !nearColor(im(24, 12), blue) || !sameColor(im(24, 24), bg))
        {
        note = "drawThickLineAA footprint failed";
        return false;
        }

    im.clear(bg);
    im.drawWedgeLineAA({ 7.0f, 24.0f }, { 41.0f, 28.0f }, 2.0f, tgx::END_ROUNDED, 10.0f, tgx::END_ROUNDED, cyan);
    if (countChanged(im, bg) < 120 || !sameColor(im(0, 0), bg) || !sameColor(im(47, 47), bg))
        {
        note = "drawWedgeLineAA footprint failed";
        return false;
        }

    im.clear(bg);
    im.fillRectAA(tgx::fBox2(8.35f, 28.75f, 6.25f, 19.6f), green);
    if (countChanged(im, bg) < 200 || !nearColor(im(18, 12), green) || !sameColor(im(6, 12), bg) || !sameColor(im(31, 12), bg))
        {
        note = "fillRectAA footprint/interior failed";
        return false;
        }

    im.clear(bg);
    im.drawThickRectAA(tgx::fBox2(8.0f, 40.0f, 6.0f, 20.0f), 3.0f, yellow);
    if (countChanged(im, bg) < 80 || !sameColor(im(4, 13), bg) || !sameColor(im(44, 13), bg))
        {
        note = "drawThickRectAA edge/interior failed";
        return false;
        }

    im.clear(bg);
    im.fillThickRectAA(tgx::fBox2(8.0f, 40.0f, 6.0f, 20.0f), 3.0f, blue, red);
    if (countChanged(im, bg) < 300 || !nearColor(im(24, 13), blue) || !sameColor(im(4, 13), bg))
        {
        note = "fillThickRectAA border/interior failed";
        return false;
        }

    im.clear(bg);
    im.drawRoundRectAA(tgx::fBox2(8.0f, 40.0f, 5.0f, 24.0f), 5.0f, cyan);
    if (countChanged(im, bg) < 90 || !nearColor(im(24, 5), cyan) || !sameColor(im(8, 5), bg) || !sameColor(im(24, 15), bg))
        {
        note = "drawRoundRectAA edge/corner failed";
        return false;
        }

    im.clear(bg);
    im.fillRoundRectAA(tgx::fBox2(8.0f, 40.0f, 5.0f, 24.0f), 5.0f, green);
    if (countChanged(im, bg) < 450 || !nearColor(im(24, 15), green) || !sameColor(im(8, 5), bg))
        {
        note = "fillRoundRectAA interior/corner failed";
        return false;
        }

    im.clear(bg);
    im.drawThickRoundRectAA(tgx::fBox2(8.0f, 40.0f, 5.0f, 24.0f), 5.0f, 3.0f, yellow);
    if (countChanged(im, bg) < 170 || !nearColor(im(24, 5), yellow) || !sameColor(im(24, 15), bg))
        {
        note = "drawThickRoundRectAA border/interior failed";
        return false;
        }

    im.clear(bg);
    im.fillThickRoundRectAA(tgx::fBox2(8.0f, 40.0f, 5.0f, 24.0f), 5.0f, 3.0f, blue, red);
    if (countChanged(im, bg) < 450 || !nearColor(im(24, 15), blue) || !nearColor(im(24, 5), red))
        {
        note = "fillThickRoundRectAA border/interior failed";
        return false;
        }

    im.clear(bg);
    im.drawCircleAA({ 24.0f, 24.0f }, 10.0f, red);
    if (countChanged(im, bg) < 55 || !nearColor(im(24, 14), red) || !nearColor(im(34, 24), red) || !sameColor(im(24, 24), bg))
        {
        note = "drawCircleAA cardinal/interior failed";
        return false;
        }

    im.clear(bg);
    im.fillCircleAA({ 24.0f, 24.0f }, 10.0f, green);
    if (countChanged(im, bg) < 300 || !nearColor(im(24, 24), green) || !sameColor(im(12, 24), bg))
        {
        note = "fillCircleAA interior/footprint failed";
        return false;
        }

    im.clear(bg);
    im.fillThickCircleAA({ 24.0f, 24.0f }, 12.0f, 4.0f, blue, red);
    if (countChanged(im, bg) < 350 || !nearColor(im(24, 24), blue) || !nearColor(im(24, 12), red))
        {
        note = "fillThickCircleAA border/interior failed";
        return false;
        }

    im.clear(bg);
    im.drawEllipseAA({ 24.0f, 24.0f }, { 14.0f, 7.0f }, yellow);
    if (countChanged(im, bg) < 60 || !nearColor(im(10, 24), yellow) || !nearColor(im(38, 24), yellow) ||
        !nearColor(im(24, 17), yellow) || !sameColor(im(24, 24), bg))
        {
        note = "drawEllipseAA cardinal/interior failed";
        return false;
        }

    im.clear(bg);
    im.fillEllipseAA({ 24.0f, 24.0f }, { 14.0f, 7.0f }, cyan);
    if (countChanged(im, bg) < 280 || !nearColor(im(24, 24), cyan) || !sameColor(im(8, 24), bg))
        {
        note = "fillEllipseAA interior/footprint failed";
        return false;
        }

    im.clear(bg);
    im.fillThickEllipseAA({ 24.0f, 24.0f }, { 15.0f, 8.0f }, 3.0f, green, red);
    if (countChanged(im, bg) < 300 || !nearColor(im(24, 24), green) || !nearColor(im(39, 24), red))
        {
        note = "fillThickEllipseAA border/interior failed";
        return false;
        }

    im.clear(bg);
    im.drawTriangleAA({ 6.0f, 39.0f }, { 22.0f, 7.0f }, { 40.0f, 39.0f }, red);
    if (countChanged(im, bg) < 80 || !sameColor(im(23, 28), bg))
        {
        note = "drawTriangleAA outline failed";
        return false;
        }

    im.clear(bg);
    im.fillTriangleAA({ 6.0f, 39.0f }, { 22.0f, 7.0f }, { 40.0f, 39.0f }, green);
    if (countChanged(im, bg) < 450 || !nearColor(im(23, 28), green) || !sameColor(im(3, 3), bg))
        {
        note = "fillTriangleAA interior/footprint failed";
        return false;
        }

    im.clear(bg);
    im.fillThickTriangleAA({ 6.0f, 39.0f }, { 22.0f, 7.0f }, { 40.0f, 39.0f }, 3.0f, blue, red);
    if (countChanged(im, bg) < 450 || !nearColor(im(23, 28), blue) || !nearColor(im(22, 8), red))
        {
        note = "fillThickTriangleAA border/interior failed";
        return false;
        }

    im.clear(bg);
    im.drawQuadAA({ 8.0f, 10.0f }, { 39.0f, 8.0f }, { 41.0f, 36.0f }, { 7.0f, 39.0f }, yellow);
    if (countChanged(im, bg) < 100 || !sameColor(im(24, 24), bg))
        {
        note = "drawQuadAA outline failed";
        return false;
        }

    im.clear(bg);
    im.fillQuadAA({ 8.0f, 10.0f }, { 39.0f, 8.0f }, { 41.0f, 36.0f }, { 7.0f, 39.0f }, cyan);
    if (countChanged(im, bg) < 700 || !nearColor(im(24, 24), cyan) || !sameColor(im(3, 3), bg))
        {
        note = "fillQuadAA interior/footprint failed";
        return false;
        }

    im.clear(bg);
    im.fillThickQuadAA({ 8.0f, 10.0f }, { 39.0f, 8.0f }, { 41.0f, 36.0f }, { 7.0f, 39.0f }, 3.0f, green, red);
    if (countChanged(im, bg) < 700 || !nearColor(im(24, 24), green) || !sameColor(im(3, 3), bg))
        {
        note = "fillThickQuadAA border/interior failed";
        return false;
        }

    im.clear(bg);
    im.fillRectAA(tgx::fBox2(8.35f, 28.75f, 6.25f, 19.6f), red, 0.0f);
    im.fillRoundRectAA(tgx::fBox2(8.0f, 40.0f, 5.0f, 24.0f), 5.0f, red, 0.0f);
    im.fillCircleAA({ 24.0f, 24.0f }, 10.0f, red, 0.0f);
    im.fillEllipseAA({ 24.0f, 24.0f }, { 14.0f, 7.0f }, red, 0.0f);
    im.fillTriangleAA({ 6.0f, 39.0f }, { 22.0f, 7.0f }, { 40.0f, 39.0f }, red, 0.0f);
    im.fillQuadAA({ 8.0f, 10.0f }, { 39.0f, 8.0f }, { 41.0f, 36.0f }, { 7.0f, 39.0f }, red, 0.0f);
    if (countChanged(im, bg) != 0)
        {
        note = "opacity 0.0 filled AA primitives modified pixels";
        return false;
        }

    return true;
}

template<typename color_t>
bool validateCurvePolygonAA(std::string& note)
{
    const auto bg = bgColor<color_t>();
    const auto red = C<color_t>(230, 30, 35);
    const auto green = C<color_t>(35, 220, 95);
    const auto blue = C<color_t>(45, 95, 230);
    const auto yellow = C<color_t>(240, 210, 55);
    const auto cyan = C<color_t>(60, 220, 240);

    std::vector<color_t> pixels(72 * 72, bg);
    std::vector<color_t> pixels2(72 * 72, bg);
    tgx::Image<color_t> im(pixels.data(), 72, 72);
    tgx::Image<color_t> ref(pixels2.data(), 72, 72);

    tgx::fVec2 polyline[] = { { 8.5f, 55.0f }, { 18.0f, 14.5f }, { 34.0f, 50.5f }, { 49.5f, 15.0f }, { 63.0f, 57.0f } };
    im.drawPolylineAA(5, polyline, red);
    ref.clear(bg);
    FPointSource polyline_src{ polyline, 5 };
    ref.drawPolylineAA(polyline_src, red);
    if ((countChanged(im, bg) < 60) || !sameColor(im(0, 0), bg) || !sameColor(im(71, 71), bg))
        {
        note = "drawPolylineAA footprint failed";
        return false;
        }
    if (!imageEquals(im, ref))
        {
        note = "drawPolylineAA functor overload mismatch";
        return false;
        }

    im.clear(bg);
    ref.clear(bg);
    im.drawThickPolylineAA(5, polyline, 5.0f, tgx::END_ROUNDED, tgx::END_ARROW_2, blue);
    FPointSource thick_polyline_src{ polyline, 5 };
    ref.drawThickPolylineAA(thick_polyline_src, 5.0f, tgx::END_ROUNDED, tgx::END_ARROW_2, blue);
    if ((countChanged(im, bg) < 360) || !nearColor(im(18, 15), blue, 2) || !sameColor(im(0, 0), bg) || !imageEquals(im, ref))
        {
        note = "drawThickPolylineAA footprint or functor overload failed";
        return false;
        }

    tgx::fVec2 polygon[] = { { 10.0f, 11.0f }, { 58.0f, 10.0f }, { 63.0f, 42.0f }, { 36.0f, 61.0f }, { 8.0f, 43.0f } };
    im.clear(bg);
    ref.clear(bg);
    im.drawPolygonAA(5, polygon, yellow);
    FPointSource polygon_src{ polygon, 5 };
    ref.drawPolygonAA(polygon_src, yellow);
    if ((countChanged(im, bg) < 120) || !sameColor(im(35, 34), bg) || !sameColor(im(1, 1), bg) || !imageEquals(im, ref))
        {
        note = "drawPolygonAA outline or functor overload failed";
        return false;
        }

    im.clear(bg);
    ref.clear(bg);
    im.fillPolygonAA(5, polygon, green);
    FPointSource fill_polygon_src{ polygon, 5 };
    ref.fillPolygonAA(fill_polygon_src, green);
    if ((countChanged(im, bg) < 1700) || !nearColor(im(35, 34), green, 2) || !sameColor(im(1, 1), bg))
        {
        note = "fillPolygonAA interior/footprint failed";
        return false;
        }
    if (!imageEquals(im, ref))
        {
        note = "fillPolygonAA functor overload mismatch";
        return false;
        }

    im.clear(bg);
    ref.clear(bg);
    im.drawThickPolygonAA(5, polygon, 4.0f, cyan);
    FPointSource thick_polygon_src{ polygon, 5 };
    ref.drawThickPolygonAA(thick_polygon_src, 4.0f, cyan);
    if ((countChanged(im, bg) < 450) || !sameColor(im(35, 34), bg) || !sameColor(im(1, 1), bg) || !imageEquals(im, ref))
        {
        note = "drawThickPolygonAA outline or functor overload failed";
        return false;
        }

    im.clear(bg);
    ref.clear(bg);
    im.fillThickPolygonAA(5, polygon, 4.0f, blue, red);
    FPointSource fill_thick_polygon_src{ polygon, 5 };
    ref.fillThickPolygonAA(fill_thick_polygon_src, 4.0f, blue, red);
    if ((countChanged(im, bg) < 1800) || !nearColor(im(35, 34), blue, 2) || !nearColor(im(10, 43), red, 3) || !sameColor(im(1, 1), bg) || !imageEquals(im, ref))
        {
        note = "fillThickPolygonAA border/interior or functor overload failed";
        return false;
        }

    im.clear(bg);
    im.drawQuadBezier({ 6, 54 }, { 66, 54 }, { 36, 4 }, 1.0f, true, red);
    if ((countChanged(im, bg) < 55) || !sameColor(im(0, 0), bg) || !sameColor(im(36, 54), bg))
        {
        note = "drawQuadBezier footprint failed";
        return false;
        }

    im.clear(bg);
    im.drawCubicBezier({ 6, 52 }, { 66, 52 }, { 10, 2 }, { 62, 70 }, true, yellow);
    if ((countChanged(im, bg) < 55) || !sameColor(im(0, 0), bg))
        {
        note = "drawCubicBezier footprint failed";
        return false;
        }

    tgx::iVec2 spline_i[] = { { 8, 58 }, { 20, 16 }, { 36, 10 }, { 58, 26 }, { 52, 56 }, { 30, 62 } };
    im.clear(bg);
    im.drawQuadSpline(6, spline_i, true, green);
    if ((countChanged(im, bg) < 70) || !sameColor(im(0, 0), bg))
        {
        note = "drawQuadSpline footprint failed";
        return false;
        }

    im.clear(bg);
    im.drawCubicSpline(6, spline_i, true, blue);
    if ((countChanged(im, bg) < 70) || !sameColor(im(0, 0), bg))
        {
        note = "drawCubicSpline footprint failed";
        return false;
        }

    im.clear(bg);
    im.drawClosedSpline(6, spline_i, cyan);
    if ((countChanged(im, bg) < 70) || !sameColor(im(0, 0), bg))
        {
        note = "drawClosedSpline footprint failed";
        return false;
        }

    tgx::fVec2 spline_f[] = { { 8.0f, 58.0f }, { 20.0f, 16.0f }, { 36.0f, 10.0f }, { 58.0f, 26.0f }, { 52.0f, 56.0f }, { 30.0f, 62.0f } };
    im.clear(bg);
    im.drawThickQuadBezierAA({ 6.0f, 54.0f }, { 66.0f, 54.0f }, { 36.0f, 4.0f }, 1.0f, 5.0f, tgx::END_ROUNDED, tgx::END_ROUNDED, red);
    if ((countChanged(im, bg) < 320) || !sameColor(im(0, 0), bg))
        {
        note = "drawThickQuadBezierAA footprint failed";
        return false;
        }

    im.clear(bg);
    im.drawThickCubicBezierAA({ 6.0f, 52.0f }, { 66.0f, 52.0f }, { 10.0f, 2.0f }, { 62.0f, 70.0f }, 5.0f, tgx::END_ROUNDED, tgx::END_ROUNDED, yellow);
    if ((countChanged(im, bg) < 320) || !sameColor(im(0, 0), bg))
        {
        note = "drawThickCubicBezierAA footprint failed";
        return false;
        }

    im.clear(bg);
    im.drawThickQuadSplineAA(6, spline_f, 4.0f, tgx::END_ROUNDED, tgx::END_ARROW_2, green);
    if ((countChanged(im, bg) < 420) || !sameColor(im(0, 0), bg))
        {
        note = "drawThickQuadSplineAA footprint failed";
        return false;
        }

    im.clear(bg);
    im.drawThickCubicSplineAA(6, spline_f, 4.0f, tgx::END_ROUNDED, tgx::END_ROUNDED, blue);
    if ((countChanged(im, bg) < 420) || !sameColor(im(0, 0), bg))
        {
        note = "drawThickCubicSplineAA footprint failed";
        return false;
        }

    im.clear(bg);
    im.drawThickClosedSplineAA(6, spline_f, 4.0f, cyan);
    if ((countChanged(im, bg) < 420) || !sameColor(im(0, 0), bg))
        {
        note = "drawThickClosedSplineAA footprint failed";
        return false;
        }

    im.clear(bg);
    im.fillClosedSplineAA(6, spline_f, green);
    if ((countChanged(im, bg) < 900) || !sameColor(im(0, 0), bg))
        {
        note = "fillClosedSplineAA footprint failed";
        return false;
        }

    im.clear(bg);
    im.fillThickClosedSplineAA(6, spline_f, 5.0f, blue, red);
    if ((countChanged(im, bg) < 1100) || !sameColor(im(0, 0), bg))
        {
        note = "fillThickClosedSplineAA footprint failed";
        return false;
        }

    im.clear(bg);
    im.drawPolylineAA(5, polyline, red, 0.0f);
    im.drawThickPolylineAA(5, polyline, 5.0f, tgx::END_ROUNDED, tgx::END_ROUNDED, red, 0.0f);
    im.fillPolygonAA(5, polygon, red, 0.0f);
    im.fillThickPolygonAA(5, polygon, 4.0f, red, blue, 0.0f);
    im.drawThickQuadBezierAA({ 6.0f, 54.0f }, { 66.0f, 54.0f }, { 36.0f, 4.0f }, 1.0f, 5.0f, tgx::END_ROUNDED, tgx::END_ROUNDED, red, 0.0f);
    im.fillClosedSplineAA(6, spline_f, red, 0.0f);
    if (countChanged(im, bg) != 0)
        {
        note = "opacity 0.0 curve/polygon AA methods modified pixels";
        return false;
        }

    return true;
}

template<typename color_t>
bool validateTexturedEdges(std::string& note)
{
    const auto bg = bgColor<color_t>();
    const auto red = C<color_t>(230, 35, 40);
    const auto green = C<color_t>(35, 220, 95);
    const auto blue = C<color_t>(45, 95, 230);
    const auto yellow = C<color_t>(240, 210, 55);
    const auto magenta = C<color_t>(255, 0, 255);
    const auto custom = C<color_t>(120, 45, 210);

    std::vector<color_t> pixels(64 * 64, bg);
    std::vector<color_t> ref_pixels(64 * 64, bg);
    std::vector<color_t> tex_pixels(8 * 8, red);
    std::vector<color_t> mask_pixels(8 * 8, magenta);
    std::vector<color_t> mixed_mask_pixels(8 * 8, green);
    tgx::Image<color_t> im(pixels.data(), 64, 64);
    tgx::Image<color_t> ref(ref_pixels.data(), 64, 64);
    tgx::Image<color_t> tex(tex_pixels.data(), 8, 8);
    tgx::Image<color_t> all_mask(mask_pixels.data(), 8, 8);
    tgx::Image<color_t> mixed_mask(mixed_mask_pixels.data(), 8, 8);

    for (int y = 2; y < 6; y++)
        {
        for (int x = 2; x < 6; x++) mixed_mask(x, y) = magenta;
        }

    im.drawTexturedTriangle(tex, { 0, 0 }, { 7, 0 }, { 0, 7 }, { 8, 8 }, { 46, 8 }, { 8, 46 }, -1.0f);
    const int plain_triangle_count = countChanged(im, bg);
    if ((plain_triangle_count < 600) || !nearColor(im(12, 12), red, 2) || !sameColor(im(0, 0), bg) || !sameColor(im(63, 63), bg))
        {
        note = "drawTexturedTriangle footprint/color failed";
        return false;
        }

    im.clear(bg);
    im.drawTexturedTriangle(tex, { 0, 0 }, { 7, 0 }, { 0, 7 }, { -20, 10 }, { 30, 8 }, { 4, 58 }, -1.0f);
    if ((countChanged(im, bg) < 400) || !sameColor(im(63, 0), bg) || !sameColor(im(63, 63), bg))
        {
        note = "clipped drawTexturedTriangle failed";
        return false;
        }

    im.clear(bg);
    im.drawTexturedTriangle(tex, { 0, 0 }, { 7, 0 }, { 0, 7 }, { 8, 8 }, { 46, 8 }, { 8, 46 },
        [custom](color_t, color_t) { return custom; });
    if ((countChanged(im, bg) < 600) || !nearColor(im(12, 12), custom, 2) || !sameColor(im(0, 0), bg))
        {
        note = "drawTexturedTriangle blend operator failed";
        return false;
        }

    im.clear(bg);
    im.drawTexturedMaskedTriangle(all_mask, magenta, { 0, 0 }, { 7, 0 }, { 0, 7 }, { 8, 8 }, { 46, 8 }, { 8, 46 }, -1.0f);
    if (countChanged(im, bg) != 0)
        {
        note = "fully masked drawTexturedMaskedTriangle modified pixels";
        return false;
        }

    im.clear(bg);
    im.drawTexturedMaskedTriangle(mixed_mask, magenta, { 0, 0 }, { 7, 0 }, { 0, 7 }, { 8, 8 }, { 46, 8 }, { 8, 46 }, -1.0f);
    const int mixed_mask_count = countChanged(im, bg);
    if ((mixed_mask_count <= 0) || (mixed_mask_count >= plain_triangle_count) || !sameColor(im(0, 0), bg))
        {
        note = "partially masked drawTexturedMaskedTriangle failed";
        return false;
        }

    im.clear(bg);
    im.drawTexturedGradientTriangle(tex, { 0, 0 }, { 7, 0 }, { 0, 7 }, { 8, 8 }, { 46, 8 }, { 8, 46 }, red, green, blue, -1.0f);
    const uint64_t gradient_triangle_hash = imageHash(im);
    if ((countChanged(im, bg) < 600) || !sameColor(im(0, 0), bg) || (gradient_triangle_hash == 0))
        {
        note = "drawTexturedGradientTriangle footprint failed";
        return false;
        }

    im.clear(bg);
    im.drawTexturedGradientMaskedTriangle(all_mask, magenta, { 0, 0 }, { 7, 0 }, { 0, 7 }, { 8, 8 }, { 46, 8 }, { 8, 46 }, red, green, blue, -1.0f);
    if (countChanged(im, bg) != 0)
        {
        note = "fully masked drawTexturedGradientMaskedTriangle modified pixels";
        return false;
        }

    im.clear(bg);
    ref.clear(bg);
    im.drawTexturedQuad(tex, { 0, 0 }, { 7, 0 }, { 7, 7 }, { 0, 7 }, { 10, 10 }, { 48, 10 }, { 48, 42 }, { 10, 42 }, -1.0f);
    ref.drawTexturedTriangle(tex, { 0, 0 }, { 7, 0 }, { 7, 7 }, { 10, 10 }, { 48, 10 }, { 48, 42 }, -1.0f);
    ref.drawTexturedTriangle(tex, { 0, 0 }, { 7, 7 }, { 0, 7 }, { 10, 10 }, { 48, 42 }, { 10, 42 }, -1.0f);
    if ((countChanged(im, bg) < 1100) || !nearColor(im(20, 20), red, 2) || !sameColor(im(4, 4), bg) || !imageEquals(im, ref))
        {
        note = "drawTexturedQuad footprint or triangle decomposition failed";
        return false;
        }

    im.clear(bg);
    ref.clear(bg);
    im.drawTexturedQuad(tex, { 0, 0 }, { 7, 0 }, { 7, 7 }, { 0, 7 }, { 10, 10 }, { 48, 10 }, { 48, 42 }, { 10, 42 },
        [custom](color_t, color_t) { return custom; });
    ref.drawTexturedTriangle(tex, { 0, 0 }, { 7, 0 }, { 7, 7 }, { 10, 10 }, { 48, 10 }, { 48, 42 },
        [custom](color_t, color_t) { return custom; });
    ref.drawTexturedTriangle(tex, { 0, 0 }, { 7, 7 }, { 0, 7 }, { 10, 10 }, { 48, 42 }, { 10, 42 },
        [custom](color_t, color_t) { return custom; });
    if ((countChanged(im, bg) < 1100) || !nearColor(im(20, 20), custom, 2) || !imageEquals(im, ref))
        {
        note = "drawTexturedQuad blend operator failed";
        return false;
        }

    im.clear(bg);
    ref.clear(bg);
    im.drawTexturedGradientQuad(tex, { 0, 0 }, { 7, 0 }, { 7, 7 }, { 0, 7 }, { 10, 10 }, { 48, 10 }, { 48, 42 }, { 10, 42 }, red, green, blue, yellow, -1.0f);
    ref.drawTexturedGradientTriangle(tex, { 0, 0 }, { 7, 0 }, { 7, 7 }, { 10, 10 }, { 48, 10 }, { 48, 42 }, red, green, blue, -1.0f);
    ref.drawTexturedGradientTriangle(tex, { 0, 0 }, { 7, 7 }, { 0, 7 }, { 10, 10 }, { 48, 42 }, { 10, 42 }, red, blue, yellow, -1.0f);
    if ((countChanged(im, bg) < 1100) || !imageEquals(im, ref))
        {
        note = "drawTexturedGradientQuad triangle decomposition failed";
        return false;
        }

    im.clear(bg);
    im.drawTexturedMaskedQuad(all_mask, magenta, { 0, 0 }, { 7, 0 }, { 7, 7 }, { 0, 7 }, { 10, 10 }, { 48, 10 }, { 48, 42 }, { 10, 42 }, -1.0f);
    if (countChanged(im, bg) != 0)
        {
        note = "fully masked drawTexturedMaskedQuad modified pixels";
        return false;
        }

    im.clear(bg);
    im.drawTexturedGradientMaskedQuad(all_mask, magenta, { 0, 0 }, { 7, 0 }, { 7, 7 }, { 0, 7 }, { 10, 10 }, { 48, 10 }, { 48, 42 }, { 10, 42 }, red, green, blue, yellow, -1.0f);
    if (countChanged(im, bg) != 0)
        {
        note = "fully masked drawTexturedGradientMaskedQuad modified pixels";
        return false;
        }

    tgx::Image<color_t> invalid_src;
    im.clear(bg);
    im.drawTexturedTriangle(invalid_src, { 0, 0 }, { 7, 0 }, { 0, 7 }, { 8, 8 }, { 46, 8 }, { 8, 46 }, -1.0f);
    im.drawTexturedQuad(invalid_src, { 0, 0 }, { 7, 0 }, { 7, 7 }, { 0, 7 }, { 10, 10 }, { 48, 10 }, { 48, 42 }, { 10, 42 }, -1.0f);
    if (countChanged(im, bg) != 0)
        {
        note = "invalid texture source modified pixels";
        return false;
        }

    im.clear(bg);
    im.drawTexturedTriangle(tex, { 0, 0 }, { 7, 0 }, { 0, 7 }, { 8, 8 }, { 46, 8 }, { 8, 46 }, 0.0f);
    im.drawTexturedGradientTriangle(tex, { 0, 0 }, { 7, 0 }, { 0, 7 }, { 8, 8 }, { 46, 8 }, { 8, 46 }, red, green, blue, 0.0f);
    im.drawTexturedMaskedTriangle(tex, magenta, { 0, 0 }, { 7, 0 }, { 0, 7 }, { 8, 8 }, { 46, 8 }, { 8, 46 }, 0.0f);
    im.drawTexturedQuad(tex, { 0, 0 }, { 7, 0 }, { 7, 7 }, { 0, 7 }, { 10, 10 }, { 48, 10 }, { 48, 42 }, { 10, 42 }, 0.0f);
    im.drawTexturedGradientQuad(tex, { 0, 0 }, { 7, 0 }, { 7, 7 }, { 0, 7 }, { 10, 10 }, { 48, 10 }, { 48, 42 }, { 10, 42 }, red, green, blue, yellow, 0.0f);
    im.drawTexturedMaskedQuad(tex, magenta, { 0, 0 }, { 7, 0 }, { 7, 7 }, { 0, 7 }, { 10, 10 }, { 48, 10 }, { 48, 42 }, { 10, 42 }, 0.0f);
    im.drawTexturedGradientMaskedQuad(tex, magenta, { 0, 0 }, { 7, 0 }, { 7, 7 }, { 0, 7 }, { 10, 10 }, { 48, 10 }, { 48, 42 }, { 10, 42 }, red, green, blue, yellow, 0.0f);
    if (countChanged(im, bg) != 0)
        {
        note = "opacity 0.0 textured methods modified pixels";
        return false;
        }

    return true;
}

template<typename color_t>
bool validateFillFailure(std::string& note)
{
    std::vector<color_t> pixels(12 * 8, bgColor<color_t>());
    tgx::Image<color_t> im(pixels.data(), 12, 8);
    const int ret = im.template fill<6>({ 0, 0 }, C<color_t>(200, 40, 40));
    if (ret != -1)
        {
        note = "tiny-stack fill did not report overflow";
        return false;
        }
    return true;
}

template<typename color_t>
bool validateGfxText(std::string& note)
{
    std::vector<color_t> pixels(96 * 70, bgColor<color_t>());
    tgx::Image<color_t> im(pixels.data(), 96, 70);

    if (im.fontHeight(gfx_font) != 10)
        {
        note = "GFX fontHeight failed";
        return false;
        }

    int advance = 0;
    const auto box = im.measureChar('A', { 8, 20 }, gfx_font, tgx::BASELINE | tgx::LEFT, &advance);
    if ((advance != 8) || (box.minX != 8) || (box.maxX != 15) || (box.minY != 12) || (box.maxY != 19))
        {
        note = "GFX measureChar failed";
        return false;
        }

    auto cursor = im.drawChar('A', { 8, 20 }, gfx_font, C<color_t>(255, 80, 80), -1.0f);
    if ((cursor.x != 16) || (countChanged(im, bgColor<color_t>()) != 64))
        {
        note = "GFX drawChar overwrite failed";
        return false;
        }

    const auto text_box = im.measureText("ABC", { 48, 38 }, gfx_font, tgx::BASELINE | tgx::CENTER, false, false);
    if (((text_box.maxX - text_box.minX + 1) != 24) || (text_box.minX > 48) || (text_box.maxX < 48))
        {
        note = "GFX measureText center anchor failed";
        return false;
        }

    const int before = countChanged(im, bgColor<color_t>());
    cursor = im.drawTextEx("ABC", { 48, 38 }, gfx_font, tgx::BASELINE | tgx::CENTER, false, false, C<color_t>(80, 220, 180), 0.7f);
    if ((countChanged(im, bgColor<color_t>()) <= before) || (cursor.x <= 48))
        {
        note = "GFX drawTextEx failed";
        return false;
        }

    auto dep_cursor = im.drawText("ABC", { 8, 60 }, C<color_t>(240, 240, 80), gfx_font, false, -1.0f);
    auto modern_cursor = im.drawText("ABC", { 8, 60 }, gfx_font, C<color_t>(240, 240, 80), -1.0f);
    if (dep_cursor != modern_cursor)
        {
        note = "GFX deprecated drawText overload mismatch";
        return false;
        }

    return true;
}

template<typename color_t>
bool validateTextStress(std::string& note)
{
    const auto bg = bgColor<color_t>();
    const auto white = C<color_t>(245, 245, 245);
    const auto cyan = C<color_t>(80, 220, 240);
    const auto yellow = C<color_t>(240, 210, 60);
    const auto red = C<color_t>(240, 80, 80);

    std::vector<color_t> pixels(180 * 130, bg);
    std::vector<color_t> zero_pixels(180 * 130, bg);
    tgx::Image<color_t> im(pixels.data(), 180, 130);
    tgx::Image<color_t> zero(zero_pixels.data(), 180, 130);

    const char* paragraph = "TGX wraps text across narrow columns and keeps drawing close to image edges.\nSecond line starts here and continues.";
    const auto nowrap_box = im.measureText(paragraph, { 12, 18 }, font_tgx_OpenSans_10, tgx::BASELINE | tgx::LEFT, false, false);
    const auto wrap_box = im.measureText(paragraph, { 12, 18 }, font_tgx_OpenSans_10, tgx::BASELINE | tgx::LEFT, true, true);
    if ((wrap_box.maxY - wrap_box.minY) <= (nowrap_box.maxY - nowrap_box.minY) || wrap_box.minX != 0)
        {
        note = "long ILI wrap measurement failed";
        return false;
        }

    const auto cursor = im.drawTextEx(paragraph, { 12, 18 }, font_tgx_OpenSans_10, tgx::BASELINE | tgx::LEFT, true, true, white, -1.0f);
    const int wrapped_count = countChanged(im, bg);
    if ((wrapped_count < 350) || (cursor.y <= 18) || (cursor.x < 0) || (cursor.x >= im.lx()))
        {
        note = "long ILI wrapped draw failed";
        return false;
        }

    const auto zero_cursor = zero.drawTextEx(paragraph, { 12, 18 }, font_tgx_OpenSans_10, tgx::BASELINE | tgx::LEFT, true, true, white, 0.0f);
    if ((zero_cursor != cursor) || (countChanged(zero, bg) != 0))
        {
        note = "long ILI opacity 0.0 draw failed";
        return false;
        }

    const int before_edge = countChanged(im, bg);
    const auto edge_box = im.measureText("EDGE", { 178, 128 }, font_tgx_OpenSans_Bold_14, tgx::BOTTOMRIGHT, false, false);
    if ((edge_box.maxX != 178) || (edge_box.maxY != 128) || (edge_box.minX < 0) || (edge_box.minY < 0))
        {
        note = "bottom-right text measurement failed";
        return false;
        }
    im.drawTextEx("EDGE", { 178, 128 }, font_tgx_OpenSans_Bold_14, tgx::BOTTOMRIGHT, false, false, cyan, 0.75f);
    if (countChanged(im, bg) <= before_edge)
        {
        note = "bottom-right text draw failed";
        return false;
        }

    const char* gfx_text = "ABC\nBCA\nCAB";
    const auto gfx_box = im.measureText(gfx_text, { 90, 78 }, gfx_font, tgx::CENTER, false, false);
    if ((gfx_box.minX >= 90) || (gfx_box.maxX <= 90) || (gfx_box.minY >= 78) || (gfx_box.maxY <= 78))
        {
        note = "GFX multiline centered measurement failed";
        return false;
        }

    const int before_gfx = countChanged(im, bg);
    const auto gfx_cursor = im.drawTextEx(gfx_text, { 90, 78 }, gfx_font, tgx::CENTER, false, false, yellow, -1.0f);
    if ((countChanged(im, bg) <= before_gfx + 120) || (gfx_cursor.y <= 78))
        {
        note = "GFX multiline draw failed";
        return false;
        }

    zero.clear(bg);
    const auto gfx_zero_cursor = zero.drawTextEx(gfx_text, { 90, 78 }, gfx_font, tgx::CENTER, false, false, yellow, 0.0f);
    if ((gfx_zero_cursor != gfx_cursor) || (countChanged(zero, bg) != 0))
        {
        note = "GFX opacity 0.0 draw failed";
        return false;
        }

    const int before_mix = countChanged(im, bg);
    im.drawTextEx("ABC", { 24, 116 }, gfx_font, tgx::BASELINE | tgx::LEFT, false, false, red, 0.55f);
    im.drawTextEx("mixed fonts", { 68, 116 }, font_tgx_OpenSans_10, tgx::BASELINE | tgx::LEFT, false, false, cyan, 0.65f);
    if (countChanged(im, bg) <= before_mix)
        {
        note = "mixed ILI/GFX text draw failed";
        return false;
        }

    return true;
}

template<typename color_t>
void drawCoreAndPixels(tgx::Image<color_t>& im)
{
    drawBackdrop(im, 0, 0, 198, 148, "pixels/subimages");
    for (int i = 0; i < 80; i++)
        {
        im.drawPixel({ 10 + i, 30 + (i % 37) }, C<color_t>(250, 60, 60));
        im.drawPixelf({ 105.2f + float(i % 45), 32.6f + float(i / 2) }, C<color_t>(60, 180, 250), 0.65f);
        }
    auto sub = im(18, 86, 86, 130);
    sub.fillScreenVGradient(C<color_t>(40, 80, 170), C<color_t>(220, 80, 50));
    sub.drawRect(sub.imageBox(), C<color_t>(255, 255, 255));
    im.iterate([](tgx::iVec2 pos, color_t& c)
        {
        if ((pos.x > 120) && (pos.x < 182) && (pos.y > 88) && (pos.y < 132)) c.mult256(180, 256, 220);
        return true;
        }, tgx::iBox2(120, 181, 88, 131));
}

template<typename color_t>
void drawLinesAndRects(tgx::Image<color_t>& im)
{
    drawBackdrop(im, 200, 0, 198, 148, "lines/rects");
    im.drawFastHLine({ 212, 33 }, 160, C<color_t>(255, 220, 80));
    im.drawFastVLine({ 218, 36 }, 96, C<color_t>(80, 220, 255));
    im.drawLine({ 210, 135 }, { 382, 28 }, C<color_t>(255, 80, 100));
    im.drawSegment({ 218, 48 }, false, { 380, 118 }, true, C<color_t>(130, 255, 110), 0.65f);
    im.drawLineAA({ 212.4f, 102.2f }, { 385.7f, 48.1f }, C<color_t>(245, 245, 255), 0.8f);
    im.drawThickLineAA({ 232.0f, 120.0f }, { 366.0f, 128.0f }, 7.0f, tgx::END_ROUNDED, tgx::END_ARROW_2, C<color_t>(240, 150, 40), 0.85f);
    im.drawWedgeLineAA({ 240.0f, 60.0f }, { 350.0f, 88.0f }, 2.0f, tgx::END_ROUNDED, 11.0f, tgx::END_ROUNDED, C<color_t>(190, 110, 255), 0.7f);
    im.fillRectHGradient(tgx::iBox2(235, 315, 78, 104), C<color_t>(20, 80, 220), C<color_t>(240, 210, 50), 0.75f);
    im.drawThickRect(tgx::iBox2(322, 382, 80, 134), 4, C<color_t>(255, 255, 255), 0.75f);
    im.fillRectAA(tgx::fBox2(250.4f, 330.7f, 30.3f, 63.4f), C<color_t>(70, 200, 150), 0.55f);
    im.fillRectVGradient(tgx::iBox2(340, 378, 30, 66), C<color_t>(255, 70, 120), C<color_t>(70, 120, 255), 0.7f);
    im.fillThickRect(tgx::iBox2(340, 382, 108, 136), 3, C<color_t>(70, 110, 220), C<color_t>(255, 255, 255), 0.55f);
    im.drawThickRectAA(tgx::fBox2(222.2f, 278.6f, 106.3f, 136.5f), 3.0f, C<color_t>(255, 220, 100), 0.75f);
    im.fillThickRectAA(tgx::fBox2(286.2f, 332.7f, 108.5f, 136.2f), 3.0f, C<color_t>(110, 230, 170), C<color_t>(255, 255, 255), 0.55f);
    im.drawWideLine({ 242.0f, 44.0f }, { 318.0f, 42.0f }, 4.0f, C<color_t>(255, 255, 255), 0.7f);
    im.drawWedgeLine({ 226.0f, 132.0f }, { 290.0f, 112.0f }, 1.5f, 7.0f, C<color_t>(70, 230, 190), 0.7f);
    im.drawSpot({ 364.0f, 116.0f }, 11.0f, C<color_t>(255, 80, 80), 0.65f);
}

template<typename color_t>
void drawCirclesAndEllipses(tgx::Image<color_t>& im)
{
    drawBackdrop(im, 400, 0, 198, 148, "circles/ellipses");
    im.drawCircle({ 438, 58 }, 28, C<color_t>(255, 180, 50));
    im.fillCircle({ 500, 58 }, 28, C<color_t>(80, 200, 255), C<color_t>(255, 255, 255), 0.8f);
    im.drawCircleAA({ 558.2f, 58.7f }, 29.0f, C<color_t>(240, 80, 160), 0.85f);
    im.drawThickCircleAA({ 445.0f, 112.0f }, 24.0f, 7.0f, C<color_t>(120, 255, 100), 0.85f);
    im.fillThickCircleAA({ 508.0f, 112.0f }, 25.0f, 9.0f, C<color_t>(220, 80, 40), C<color_t>(255, 245, 180), 0.8f);
    im.drawEllipse({ 558, 111 }, { 34, 18 }, C<color_t>(120, 180, 255));
    im.fillEllipse({ 610, 111 }, { 27, 16 }, C<color_t>(180, 100, 255), C<color_t>(255, 255, 255), 0.65f);
    im.drawEllipseAA({ 438.0f, 112.0f }, { 28.0f, 15.0f }, C<color_t>(255, 220, 120), 0.8f);
    im.drawThickEllipseAA({ 498.0f, 112.0f }, { 29.0f, 17.0f }, 5.0f, C<color_t>(120, 255, 180), 0.8f);
    im.fillEllipseAA({ 558.0f, 111.0f }, { 28.0f, 13.0f }, C<color_t>(80, 220, 190), 0.5f);
    im.drawThickCircleArcAA({ 500.0f, 72.0f }, 43.0f, 210.0f, 330.0f, 6.0f, C<color_t>(255, 255, 255), 0.7f);
    im.fillThickEllipseAA({ 610.0f, 58.0f }, { 25.0f, 16.0f }, 6.0f, C<color_t>(80, 120, 255), C<color_t>(255, 255, 255), 0.65f);
}

template<typename color_t>
void drawTrianglesQuads(tgx::Image<color_t>& im)
{
    drawBackdrop(im, 600, 0, 200, 148, "triangles/quads");
    im.drawTriangle({ 616, 125 }, { 660, 32 }, { 702, 126 }, C<color_t>(255, 220, 70), 0.8f);
    im.fillTriangle({ 682, 120 }, { 734, 30 }, { 782, 122 }, C<color_t>(80, 170, 255), C<color_t>(250, 250, 250), 0.65f);
    im.drawTriangleAA({ 612.5f, 44.2f }, { 650.5f, 18.3f }, { 688.7f, 48.7f }, C<color_t>(255, 100, 160), 0.85f);
    im.drawThickTriangleAA({ 612.0f, 82.0f }, { 656.0f, 58.0f }, { 688.0f, 95.0f }, 4.0f, C<color_t>(255, 255, 255), 0.75f);
    im.fillTriangleAA({ 622.4f, 58.5f }, { 676.8f, 58.2f }, { 650.1f, 103.6f }, C<color_t>(80, 240, 170), 0.55f);
    im.fillThickTriangleAA({ 612.0f, 104.0f }, { 666.0f, 104.0f }, { 640.0f, 136.0f }, 5.0f, C<color_t>(70, 130, 250), C<color_t>(255, 235, 150), 0.65f);
    im.drawGradientTriangle({ 704.0f, 50.0f }, { 774.0f, 50.0f }, { 740.0f, 110.0f }, C<color_t>(255, 0, 0), C<color_t>(0, 255, 0), C<color_t>(0, 80, 255));
    im.drawQuad({ 610, 24 }, { 655, 20 }, { 670, 45 }, { 615, 53 }, C<color_t>(255, 255, 255), 0.8f);
    im.fillQuad({ 704, 112 }, { 778, 105 }, { 786, 132 }, { 710, 138 }, C<color_t>(80, 200, 255), 0.45f);
    im.drawQuadAA({ 704.0f, 20.0f }, { 784.0f, 28.0f }, { 772.0f, 44.0f }, { 712.0f, 42.0f }, C<color_t>(255, 255, 255), 0.7f);
    im.drawThickQuadAA({ 700.0f, 76.0f }, { 785.0f, 80.0f }, { 770.0f, 98.0f }, { 708.0f, 96.0f }, 4.0f, C<color_t>(255, 120, 120), 0.75f);
    im.fillQuadAA({ 704.0f, 20.0f }, { 784.0f, 28.0f }, { 772.0f, 44.0f }, { 712.0f, 42.0f }, C<color_t>(240, 190, 70), 0.65f);
    im.fillThickQuadAA({ 704.0f, 50.0f }, { 782.0f, 56.0f }, { 770.0f, 72.0f }, { 710.0f, 70.0f }, 4.0f, C<color_t>(60, 210, 140), C<color_t>(255, 255, 255), 0.7f);
    im.drawGradientQuad({ 708.0f, 142.0f }, { 786.0f, 146.0f }, { 774.0f, 170.0f }, { 710.0f, 166.0f },
                        C<color_t>(255, 50, 50), C<color_t>(50, 255, 50), C<color_t>(50, 50, 255), C<color_t>(255, 255, 80), 0.75f);
}

template<typename color_t>
void drawPolygonsAndSplines(tgx::Image<color_t>& im)
{
    drawBackdrop(im, 0, 150, 265, 220, "polygons/splines");
    tgx::iVec2 poly[] = { { 20, 320 }, { 52, 185 }, { 98, 300 }, { 152, 182 }, { 228, 340 } };
    im.drawPolyline(5, poly, C<color_t>(240, 240, 90));
    im.drawPolygon(5, poly, C<color_t>(255, 255, 255), 0.55f);
    tgx::fVec2 fpoly[] = { { 28.0f, 335.0f }, { 70.0f, 228.0f }, { 118.0f, 338.0f }, { 175.0f, 230.0f }, { 235.0f, 350.0f } };
    im.fillPolygonAA(5, fpoly, C<color_t>(80, 150, 245), 0.35f);
    im.drawPolygonAA(5, fpoly, C<color_t>(220, 240, 255), 0.8f);
    im.drawThickPolygonAA(5, fpoly, 3.5f, C<color_t>(255, 170, 70), 0.65f);
    im.fillThickPolygonAA(5, fpoly, 4.5f, C<color_t>(80, 220, 160), C<color_t>(255, 255, 255), 0.28f);
    im.drawThickPolylineAA(5, fpoly, 5.0f, tgx::END_ROUNDED, tgx::END_ARROW_3, C<color_t>(255, 90, 120), 0.8f);
    IPointSource ipoly_src{ poly, 5 };
    im.fillPolygon(ipoly_src, C<color_t>(70, 80, 170), 0.18f);
    FPointSource fpoly_src{ fpoly, 5 };
    im.drawPolylineAA(fpoly_src, C<color_t>(255, 255, 255), 0.55f);
    im.drawQuadBezier({ 22, 210 }, { 230, 205 }, { 124, 260 }, 1.0f, true, C<color_t>(120, 255, 170), 0.85f);
    im.drawCubicBezier({ 26, 286 }, { 232, 286 }, { 65, 150 }, { 195, 405 }, true, C<color_t>(255, 180, 60), 0.8f);
    tgx::iVec2 spline[] = { { 30, 360 }, { 80, 382 }, { 130, 342 }, { 180, 388 }, { 235, 356 } };
    im.drawQuadSpline(5, spline, true, C<color_t>(255, 120, 190), 0.75f);
    im.drawCubicSpline(5, spline, true, C<color_t>(230, 230, 255), 0.8f);
    im.drawClosedSpline(5, spline, C<color_t>(80, 220, 255), 0.55f);
}

template<typename color_t>
void drawBlitsAndTextures(tgx::Image<color_t>& im)
{
    drawBackdrop(im, 270, 150, 260, 220, "blit/textures");
    constexpr int SW = 72;
    constexpr int SH = 48;
    std::vector<color_t> sprite_pixels(SW * SH);
    std::vector<color_t> masked_pixels(SW * SH);
    makeSprite(sprite_pixels, SW, SH, false);
    makeSprite(masked_pixels, SW, SH, true);
    tgx::Image<color_t> sprite(sprite_pixels.data(), SW, SH);
    tgx::Image<color_t> masked(masked_pixels.data(), SW, SH);
    const color_t mask = C<color_t>(255, 0, 255);

    im.blit(sprite, { 282, 188 }, -1.0f);
    im.blit(sprite, { 370, 188 }, 0.45f);
    im.blitMasked(masked, mask, { 450, 184 }, -1.0f);
    im.blitRotated(sprite, { 292, 270 }, 90, 0.8f);
    im.blitScaledRotated(sprite, { SW / 2.0f, SH / 2.0f }, { 400.0f, 303.0f }, 1.05f, -25.0f, 0.75f);
    im.blitScaledRotatedMasked(masked, mask, { SW / 2.0f, SH / 2.0f }, { 480.0f, 306.0f }, 1.25f, 28.0f, -1.0f);
    auto target = im(292, 352, 316, 358);
    target.copyFrom(sprite, 0.35f);
}

template<typename color_t>
void drawTextAndFonts(tgx::Image<color_t>& im)
{
    drawBackdrop(im, 535, 150, 265, 220, "text/fonts");
    im.drawText("OpenSans 18 baseline", { 548, 202 }, font_tgx_OpenSans_18, C<color_t>(245, 245, 255), -1.0f);
    im.drawText("opacity 0.5", { 548, 232 }, font_tgx_OpenSans_Bold_20, C<color_t>(255, 190, 70), 0.5f);
    im.drawTextEx("CENTER", { 666, 280 }, font_tgx_OpenSans_24, tgx::CENTER, false, false, C<color_t>(90, 230, 200), 0.8f);
    im.drawTextEx("wrap text across a narrow column", { 548, 252 }, font_tgx_OpenSans_10, tgx::BASELINE | tgx::LEFT, true, true, C<color_t>(180, 210, 255), 0.85f);
    im.drawTextEx("RIGHT", { 785, 330 }, font_tgx_Arial_20, tgx::BASELINE | tgx::RIGHT, false, false, C<color_t>(255, 110, 130), -1.0f);
    im.drawFastHLine({ 546, 280 }, 238, C<color_t>(100, 130, 160), 0.6f);
    auto box = im.measureText("CENTER", { 666, 280 }, font_tgx_OpenSans_24, tgx::CENTER, false, false);
    im.drawRect(box, C<color_t>(255, 255, 255), 0.45f);
}

template<typename color_t>
void drawArcsAndRounded(tgx::Image<color_t>& im)
{
    drawBackdrop(im, 0, 375, 398, 225, "rounded/arcs/sectors");
    im.drawRoundRect(tgx::iBox2(20, 410, 150, 465), 14, C<color_t>(255, 220, 80), 0.85f);
    im.fillRoundRect(tgx::iBox2(170, 410, 360, 465), 18, C<color_t>(70, 170, 235), 0.55f);
    im.drawRoundRectAA(tgx::fBox2(28.0f, 180.0f, 488.0f, 570.0f), 20.0f, C<color_t>(255, 120, 150), 0.8f);
    im.drawThickRoundRectAA(tgx::fBox2(205.0f, 376.0f, 488.0f, 570.0f), 22.0f, 8.0f, C<color_t>(255, 255, 255), 0.75f);
    im.fillRoundRectAA(tgx::fBox2(45.0f, 160.0f, 430.0f, 462.0f), 15.0f, C<color_t>(170, 80, 240), 0.45f);
    im.fillThickRoundRectAA(tgx::fBox2(205.0f, 376.0f, 488.0f, 570.0f), 22.0f, 8.0f, C<color_t>(85, 210, 170), C<color_t>(250, 250, 250), 0.75f);
    im.drawCircleArcAA({ 90.0f, 430.0f }, 38.0f, 40.0f, 260.0f, C<color_t>(255, 255, 255), 0.75f);
    im.fillCircleSectorAA({ 96.0f, 520.0f }, 42.0f, 25.0f, 300.0f, C<color_t>(255, 180, 60), 0.75f);
    im.fillThickCircleSectorAA({ 300.0f, 520.0f }, 45.0f, 210.0f, 520.0f, 13.0f, C<color_t>(90, 100, 255), C<color_t>(255, 255, 255), 0.75f);
}

template<typename color_t>
void drawTextureTriangles(tgx::Image<color_t>& im)
{
    drawBackdrop(im, 400, 375, 400, 225, "textured triangles/quads");
    constexpr int TW = 96;
    constexpr int TH = 72;
    std::vector<color_t> tex_pixels(TW * TH);
    makeSprite(tex_pixels, TW, TH, true);
    tgx::Image<color_t> tex(tex_pixels.data(), TW, TH);
    const color_t mask = C<color_t>(255, 0, 255);

    im.drawTexturedTriangle(tex, { 0, 0 }, { TW, 0 }, { TW / 2.0f, TH }, { 430, 420 }, { 570, 405 }, { 492, 548 }, 0.85f);
    im.drawTexturedGradientTriangle(tex, { 0, 0 }, { TW, 0 }, { TW / 2.0f, TH }, { 420, 390 }, { 540, 386 }, { 482, 456 },
                                    C<color_t>(255, 70, 70), C<color_t>(70, 255, 70), C<color_t>(70, 70, 255), 0.7f);
    im.drawTexturedMaskedTriangle(tex, mask, { 0, 0 }, { TW, 0 }, { TW / 2.0f, TH }, { 590, 420 }, { 766, 412 }, { 678, 555 }, -1.0f);
    im.drawTexturedGradientMaskedTriangle(tex, mask, { 0, 0 }, { TW, 0 }, { TW / 2.0f, TH }, { 610, 392 }, { 760, 388 }, { 690, 460 },
                                          C<color_t>(255, 100, 100), C<color_t>(100, 255, 100), C<color_t>(100, 100, 255), 0.7f);
    im.drawTexturedQuad(tex, { 0, 0 }, { TW, 0 }, { TW, TH }, { 0, TH }, { 565, 560 }, { 640, 565 }, { 636, 596 }, { 560, 590 }, 0.65f);
    im.drawTexturedMaskedQuad(tex, mask, { 0, 0 }, { TW, 0 }, { TW, TH }, { 0, TH }, { 660, 560 }, { 778, 566 }, { 770, 596 }, { 662, 590 }, -1.0f);
    im.drawTexturedGradientQuad(tex, { 0, 0 }, { TW, 0 }, { TW, TH }, { 0, TH }, { 420, 555 }, { 545, 565 }, { 540, 595 }, { 425, 588 },
                                C<color_t>(255, 80, 80), C<color_t>(80, 255, 80), C<color_t>(80, 80, 255), C<color_t>(255, 255, 80), 0.8f);
    im.drawTexturedGradientMaskedQuad(tex, mask, { 0, 0 }, { TW, 0 }, { TW, TH }, { 0, TH }, { 680, 500 }, { 780, 508 }, { 770, 548 }, { 676, 540 },
                                      C<color_t>(255, 80, 80), C<color_t>(80, 255, 80), C<color_t>(80, 80, 255), C<color_t>(255, 255, 80), 0.8f);
}

template<typename color_t>
void drawImageOps(tgx::Image<color_t>& im)
{
    drawBackdrop(im, 0, 0, 260, 180, "image/copy/fill ops");
    const auto bg = C<color_t>(16, 22, 34);
    const auto border = C<color_t>(245, 245, 245);
    const auto red = C<color_t>(220, 40, 40);
    const auto green = C<color_t>(60, 210, 120);
    const auto blue = C<color_t>(50, 90, 230);

    im.fillRect(tgx::iBox2(24, 120, 42, 128), bg);
    im.drawRect(tgx::iBox2(34, 104, 54, 116), border);
    im.template fill<2048>({ 45, 70 }, border, red);
    im.template fill<2048>({ 26, 44 }, blue);

    constexpr int SW = 48;
    constexpr int SH = 32;
    std::vector<color_t> spx(SW * SH);
    makeSprite(spx, SW, SH, false);
    tgx::Image<color_t> sp(spx.data(), SW, SH);
    auto target = im(144, 235, 42, 128);
    target.copyFrom(sp, 0.85f);

    std::vector<color_t> half_pixels((SW / 2) * (SH / 2), green);
    tgx::Image<color_t> half(half_pixels.data(), SW / 2, SH / 2);
    half.copyReduceHalf(sp);
    im.blit(half, { 150, 140 }, -1.0f);

    std::vector<color_t> back_pixels(24 * 18);
    tgx::Image<color_t> back(back_pixels.data(), 24, 18);
    sp.blitBackward(back, { 10, 8 });
    im.blit(back, { 200, 140 }, -1.0f);
}

template<typename color_t>
void drawThickSplines(tgx::Image<color_t>& im)
{
    drawBackdrop(im, 270, 0, 260, 180, "thick/fill splines");
    tgx::fVec2 pts[] = { { 290.0f, 146.0f }, { 330.0f, 52.0f }, { 390.0f, 145.0f }, { 444.0f, 58.0f }, { 505.0f, 150.0f } };
    im.drawThickQuadBezierAA({ 290.0f, 60.0f }, { 505.0f, 58.0f }, { 392.0f, 118.0f }, 1.0f, 5.0f, tgx::END_ROUNDED, tgx::END_ARROW_2, C<color_t>(255, 210, 80), 0.85f);
    im.drawThickCubicBezierAA({ 292.0f, 98.0f }, { 504.0f, 98.0f }, { 328.0f, 10.0f }, { 464.0f, 184.0f }, 4.0f, tgx::END_ROUNDED, tgx::END_ROUNDED, C<color_t>(90, 220, 255), 0.8f);
    im.drawThickQuadSplineAA(5, pts, 4.0f, tgx::END_ROUNDED, tgx::END_ARROW_3, C<color_t>(255, 100, 150), 0.8f);
    im.drawThickCubicSplineAA(5, pts, 3.0f, tgx::END_ROUNDED, tgx::END_ROUNDED, C<color_t>(100, 255, 170), 0.65f);
    im.drawThickClosedSplineAA(5, pts, 3.0f, C<color_t>(255, 255, 255), 0.5f);
    im.fillClosedSplineAA(5, pts, C<color_t>(80, 120, 240), 0.25f);
    im.fillThickClosedSplineAA(5, pts, 5.0f, C<color_t>(80, 200, 160), C<color_t>(255, 255, 255), 0.35f);
}

template<typename color_t>
void drawLayoutAndRotations(tgx::Image<color_t>& im)
{
    drawBackdrop(im, 535, 0, 265, 180, "text layout / rotations");
    constexpr int SW = 42;
    constexpr int SH = 28;
    std::vector<color_t> spx(SW * SH);
    makeSprite(spx, SW, SH, false);
    tgx::Image<color_t> sp(spx.data(), SW, SH);

    im.blitRotated(sp, { 550, 48 }, 0, -1.0f);
    im.blitRotated(sp, { 602, 44 }, 90, -1.0f);
    im.blitRotated(sp, { 650, 48 }, 180, -1.0f);
    im.blitRotated(sp, { 702, 44 }, 270, -1.0f);
    im.drawTextEx("LEFT", { 550, 122 }, font_tgx_OpenSans_10, tgx::BASELINE | tgx::LEFT, false, false, C<color_t>(255, 255, 255), -1.0f);
    im.drawTextEx("CENTER", { 665, 146 }, font_tgx_OpenSans_10, tgx::CENTER, false, false, C<color_t>(90, 230, 210), 0.85f);
    im.drawTextEx("RIGHT", { 786, 166 }, font_tgx_OpenSans_10, tgx::BASELINE | tgx::RIGHT, false, false, C<color_t>(255, 180, 80), -1.0f);
    im.drawTextEx("wrap wrap wrap wrap", { 710, 120 }, font_tgx_OpenSans_10, tgx::BASELINE | tgx::LEFT, true, false, C<color_t>(180, 210, 255), 0.85f);
}

template<typename color_t>
void drawGfxFontPanel(tgx::Image<color_t>& im)
{
    drawBackdrop(im, 0, 185, 260, 180, "GFXfont path");
    im.drawChar('A', { 22, 235 }, gfx_font, C<color_t>(255, 90, 90), -1.0f);
    im.drawChar('B', { 46, 235 }, gfx_font, C<color_t>(90, 220, 150), 0.75f);
    im.drawChar('C', { 70, 235 }, gfx_font, C<color_t>(90, 160, 255), 0.55f);
    im.drawText("ABC ABC", { 22, 270 }, gfx_font, C<color_t>(255, 240, 120), -1.0f);
    im.drawTextEx("ABC\nBCA", { 150, 280 }, gfx_font, tgx::CENTER, false, false, C<color_t>(120, 230, 255), 0.8f);
    auto box = im.measureText("ABC", { 22, 270 }, gfx_font, tgx::BASELINE | tgx::LEFT, false, false);
    im.drawRect(box, C<color_t>(255, 255, 255), 0.55f);
}

template<typename color_t>
void drawBulkOps(tgx::Image<color_t>& im)
{
    drawBackdrop(im, 270, 185, 530, 180, "bulk fill / copy / gradients");
    auto area = im(290, 780, 224, 342);
    area.fillScreenHGradient(C<color_t>(30, 70, 170), C<color_t>(240, 200, 60));
    area.fillRectVGradient(tgx::iBox2(16, 250, 22, 92), C<color_t>(240, 70, 110), C<color_t>(40, 220, 180), 0.75f);

    constexpr int BW = 160;
    constexpr int BH = 90;
    std::vector<color_t> src_pixels(BW * BH);
    tgx::Image<color_t> src(src_pixels.data(), BW, BH);
    src.fillScreenVGradient(C<color_t>(70, 80, 230), C<color_t>(230, 80, 70));
    src.fillCircleAA({ 80.0f, 45.0f }, 32.0f, C<color_t>(240, 230, 120), 0.85f);
    src.drawTextEx("copyFrom", { 80, 52 }, font_tgx_OpenSans_14, tgx::CENTER, false, false, C<color_t>(10, 15, 20), 0.85f);

    auto dst = im(420, 700, 238, 330);
    dst.copyFrom(src, 0.82f);
    im.blit(src, { 300, 244 }, 0.45f);
}

template<typename color_t>
void drawOpsBottomPanels(tgx::Image<color_t>& im)
{
    drawBackdrop(im, 0, 370, 395, 230, "scaled/rotated/masked blits");
    drawBackdrop(im, 400, 370, 400, 230, "clipped textured gradients");

    constexpr int SW = 74;
    constexpr int SH = 52;
    std::vector<color_t> sprite_pixels(SW * SH);
    std::vector<color_t> masked_pixels(SW * SH);
    makeSprite(sprite_pixels, SW, SH, false);
    makeSprite(masked_pixels, SW, SH, true);
    tgx::Image<color_t> sprite(sprite_pixels.data(), SW, SH);
    tgx::Image<color_t> masked(masked_pixels.data(), SW, SH);
    const color_t mask = C<color_t>(255, 0, 255);

    auto glow_blend = [](color_t src, color_t dst)
        {
        const tgx::RGB32 s(src);
        const tgx::RGB32 d(dst);
        const int r = tgx::min(255, 20 + ((int)s.R * 3 + (int)d.R) / 4);
        const int g = tgx::min(255, 20 + ((int)s.G * 3 + (int)d.G) / 4);
        const int b = tgx::min(255, 35 + ((int)s.B * 3 + (int)d.B) / 4);
        return color_t(tgx::RGB32(r, g, b));
        };

    im.fillRectHGradient(tgx::iBox2(18, 372, 410, 565), C<color_t>(25, 45, 95), C<color_t>(215, 80, 130), 0.45f);
    im.drawTextEx("scale", { 30, 402 }, font_tgx_OpenSans_10, tgx::BASELINE | tgx::LEFT, false, false, C<color_t>(230, 240, 255), 0.9f);
    im.blitScaledRotated(sprite, { SW / 2.0f, SH / 2.0f }, { 76.0f, 474.0f }, 0.70f, -38.0f, 0.82f);
    im.blitScaledRotated(sprite, { SW / 2.0f, SH / 2.0f }, { 155.0f, 486.0f }, 1.05f, 18.0f, glow_blend);
    im.blitScaledRotatedMasked(masked, mask, { SW / 2.0f, SH / 2.0f }, { 252.0f, 482.0f }, 1.30f, 47.0f, -1.0f);
    im.blitRotated(masked, { 300, 404 }, 270, 0.70f);
    im.blit(sprite, { 26, 530 }, glow_blend);
    im.blitMasked(masked, mask, { 124, 526 }, 0.88f);
    im.drawThickLineAA({ 228.0f, 552.0f }, { 366.0f, 506.0f }, 8.0f, tgx::END_ARROW_2, tgx::END_ROUNDED, C<color_t>(255, 230, 80), 0.78f);

    constexpr int TW = 100;
    constexpr int TH = 78;
    std::vector<color_t> tex_pixels(TW * TH);
    std::vector<color_t> tex_masked_pixels(TW * TH);
    makeSprite(tex_pixels, TW, TH, false);
    makeSprite(tex_masked_pixels, TW, TH, true);
    tgx::Image<color_t> tex(tex_pixels.data(), TW, TH);
    tgx::Image<color_t> tex_masked(tex_masked_pixels.data(), TW, TH);

    im.fillRectVGradient(tgx::iBox2(418, 782, 408, 566), C<color_t>(28, 36, 82), C<color_t>(90, 30, 80), 0.45f);
    im.drawTexturedGradientQuad(tex,
        { 0, 0 }, { TW, 0 }, { TW, TH }, { 0, TH },
        { 425, 418 }, { 560, 398 }, { 548, 512 }, { 410, 536 },
        C<color_t>(255, 70, 80), C<color_t>(80, 250, 120), C<color_t>(80, 130, 255), C<color_t>(255, 230, 80), 0.82f);
    im.drawTexturedGradientMaskedQuad(tex_masked, mask,
        { 0, 0 }, { TW, 0 }, { TW, TH }, { 0, TH },
        { 584, 412 }, { 736, 430 }, { 708, 552 }, { 560, 532 },
        C<color_t>(255, 120, 120), C<color_t>(120, 255, 160), C<color_t>(110, 160, 255), C<color_t>(255, 245, 100), 0.85f);
    im.drawTexturedTriangle(tex,
        { 0, 0 }, { TW, 0 }, { TW / 2.0f, TH },
        { 718, 466 }, { 838, 440 }, { 785, 628 }, glow_blend);
    im.drawTexturedMaskedTriangle(tex_masked, mask,
        { 0, 0 }, { TW, 0 }, { TW / 2.0f, TH },
        { 392, 570 }, { 506, 522 }, { 468, 636 }, 0.92f);
    im.drawRoundRectAA(tgx::fBox2(430.0f, 776.0f, 388.0f, 582.0f), 16.0f, C<color_t>(245, 245, 255), 0.42f);
    im.drawTextEx("clip", { 774, 586 }, font_tgx_OpenSans_10, tgx::BASELINE | tgx::RIGHT, false, false, C<color_t>(230, 240, 255), 0.9f);
}

template<typename color_t>
void renderFullSuite(tgx::Image<color_t>& im)
{
    im.fillScreen(bgColor<color_t>());
    drawCoreAndPixels(im);
    drawLinesAndRects(im);
    drawCirclesAndEllipses(im);
    drawTrianglesQuads(im);
    drawPolygonsAndSplines(im);
    drawBlitsAndTextures(im);
    drawTextAndFonts(im);
    drawArcsAndRounded(im);
    drawTextureTriangles(im);
}

template<typename color_t>
void renderOpsSuite(tgx::Image<color_t>& im)
{
    im.fillScreen(bgColor<color_t>());
    drawImageOps(im);
    drawThickSplines(im);
    drawLayoutAndRotations(im);
    drawGfxFontPanel(im);
    drawBulkOps(im);
    drawOpsBottomPanels(im);
}

template<typename color_t, typename DrawFn>
BenchRow benchGroup(const std::string& color_name, const std::string& group, int iterations, DrawFn&& draw)
{
    std::vector<color_t> pixels(W * H);
    tgx::Image<color_t> im(pixels.data(), W, H);

    const double us = timeMicroseconds(iterations, [&]()
        {
        im.fillScreen(bgColor<color_t>());
        draw(im);
        });

    const int changed = countChanged(im, bgColor<color_t>());
    return BenchRow{ "cpu", color_name, group, "benchmark", iterations, us, us / iterations, imageHash(im), changed, changed > 0, "" };
}

template<typename ValidateFn>
BenchRow validationGroup(const std::string& color_name, const std::string& group, int iterations, ValidateFn&& validate)
{
    std::string note;
    bool valid = false;
    const double us = timeMicroseconds(iterations, [&]()
        {
        std::string local_note;
        valid = validate(local_note);
        if (!valid && note.empty()) note = local_note;
        });
    return BenchRow{ "cpu", color_name, group, "validation", iterations, us, us / iterations, 0, 0, valid, note };
}

template<typename color_t>
void drawLargeGradients(tgx::Image<color_t>& im)
{
    const int w = im.lx();
    const int h = im.ly();
    im.fillScreenHGradient(C<color_t>(10, 18, 35), C<color_t>(230, 190, 55));
    for (int i = 0; i < 16; i++)
        {
        const int x0 = (i * w) / 16;
        const int x1 = (((i + 1) * w) / 16) - 1;
        const int y0 = (i * h) / 32;
        const int y1 = h - 1 - ((i * h) / 48);
        im.fillRectVGradient(tgx::iBox2(x0, x1, y0, y1),
            C<color_t>((30 + i * 11) & 255, (80 + i * 7) & 255, (160 + i * 5) & 255),
            C<color_t>((220 - i * 9) & 255, (70 + i * 13) & 255, (50 + i * 17) & 255),
            0.55f);
        }
    im.fillRectHGradient(tgx::iBox2(w / 12, (11 * w) / 12, (5 * h) / 12, (7 * h) / 12),
        C<color_t>(230, 70, 120), C<color_t>(60, 220, 190), 0.72f);
}

template<typename color_t>
void drawLargePrimitives(tgx::Image<color_t>& im)
{
    const int w = im.lx();
    const int h = im.ly();
    for (int i = 0; i < 220; i++)
        {
        const int x0 = (i * 73) % w;
        const int y0 = (i * 131) % h;
        const int x1 = (w - 1) - ((i * 97) % w);
        const int y1 = (h - 1) - ((i * 53) % h);
        im.drawLine({ x0, y0 }, { x1, y1 }, C<color_t>((i * 37) & 255, (i * 71) & 255, (i * 113) & 255), 0.55f);
        if ((i % 5) == 0)
            {
            im.drawLineAA({ float(x0) + 0.35f, float(y0) + 0.65f }, { float(x1) + 0.25f, float(y1) + 0.2f },
                C<color_t>(245, 245, 255), 0.5f);
            }
        }

    for (int i = 0; i < 90; i++)
        {
        const int cx = (80 + i * 211) % w;
        const int cy = (90 + i * 157) % h;
        const int r = 12 + (i % 48);
        im.fillCircleAA({ float(cx), float(cy) }, float(r), C<color_t>((60 + i * 9) & 255, (170 + i * 5) & 255, (220 - i * 3) & 255), 0.45f);
        im.drawThickRectAA(tgx::fBox2(float(tgx::max(0, cx - 2 * r)), float(tgx::min(w - 1, cx + 2 * r)),
                                      float(tgx::max(0, cy - r)), float(tgx::min(h - 1, cy + r))),
            3.0f + float(i % 6), C<color_t>(255, 255, 255), 0.35f);
        }
}

template<typename color_t>
void drawLargeBlitsTextures(tgx::Image<color_t>& im)
{
    const int w = im.lx();
    const int h = im.ly();

    constexpr int SW = 128;
    constexpr int SH = 96;
    std::vector<color_t> sprite_pixels(SW * SH);
    std::vector<color_t> masked_pixels(SW * SH);
    makeSprite(sprite_pixels, SW, SH, false);
    makeSprite(masked_pixels, SW, SH, true);
    tgx::Image<color_t> sprite(sprite_pixels.data(), SW, SH);
    tgx::Image<color_t> masked(masked_pixels.data(), SW, SH);
    const color_t mask = C<color_t>(255, 0, 255);

    for (int y = 40; y < h - SH; y += tgx::max(140, h / 10))
        {
        for (int x = 35; x < w - SW; x += tgx::max(180, w / 9))
            {
            im.blit(sprite, { x, y }, 0.72f);
            im.blitMasked(masked, mask, { x + 56, y + 42 }, -1.0f);
            }
        }

    for (int i = 0; i < 28; i++)
        {
        const float x = float((i * 173) % (w - 160));
        const float y = float((i * 227) % (h - 160));
        im.drawTexturedTriangle(sprite, { 0, 0 }, { SW, 0 }, { SW / 2.0f, SH },
            { x + 20.0f, y + 20.0f }, { x + 150.0f, y + 36.0f }, { x + 78.0f, y + 154.0f }, 0.62f);
        im.drawTexturedGradientMaskedQuad(masked, mask, { 0, 0 }, { SW, 0 }, { SW, SH }, { 0, SH },
            { x + 18.0f, y + 170.0f }, { x + 170.0f, y + 178.0f }, { x + 150.0f, y + 260.0f }, { x + 8.0f, y + 244.0f },
            C<color_t>(255, 80, 80), C<color_t>(80, 255, 100), C<color_t>(80, 120, 255), C<color_t>(255, 230, 80), 0.65f);
        }
}

template<typename color_t>
void drawLargeText(tgx::Image<color_t>& im)
{
    const int w = im.lx();
    const int h = im.ly();
    const char* paragraph = "TGX large frame text stress wraps paragraphs, mixes opacity, and keeps anchors near the borders.";
    const int col_w = tgx::max(220, w / 5);
    for (int y = 80; y < h - 100; y += 150)
        {
        for (int x = 40; x < w - col_w; x += col_w)
            {
            im.drawTextEx(paragraph, { x, y }, font_tgx_OpenSans_14, tgx::BASELINE | tgx::LEFT, true, true,
                C<color_t>(230, 240, 255), 0.78f);
            im.drawTextEx("ABC\nBCA", { x + col_w / 2, y + 72 }, gfx_font, tgx::CENTER, false, false,
                C<color_t>(255, 220, 80), 0.75f);
            }
        }
    im.drawTextEx("BOTTOM RIGHT LARGE", { w - 18, h - 18 }, font_tgx_OpenSans_Bold_20, tgx::BOTTOMRIGHT, false, false,
        C<color_t>(80, 230, 220), 0.9f);
}

template<typename color_t>
void renderLargeStressSuite(tgx::Image<color_t>& im)
{
    drawLargeGradients(im);
    drawLargePrimitives(im);
    drawLargeBlitsTextures(im);
    drawLargeText(im);
}

template<typename color_t, typename DrawFn>
BenchRow benchLargeGroup(const std::string& color_name, const std::string& group, int iterations, int size, DrawFn&& draw)
{
    std::vector<color_t> pixels(size_t(size) * size_t(size));
    tgx::Image<color_t> im(pixels.data(), size, size);
    const double us = timeMicroseconds(iterations, [&]()
        {
        im.fillScreen(bgColor<color_t>());
        draw(im);
        });

    const int changed = countChanged(im, bgColor<color_t>());
    return BenchRow{ "cpu-large", color_name, group, "benchmark", iterations, us, us / iterations, imageHash(im), changed, changed > 0, "" };
}

template<typename color_t>
std::vector<BenchRow> runLargeForColor(const std::string& color_name, const std::filesystem::path& out_dir, int size)
{
    std::vector<BenchRow> rows;
    std::vector<color_t> pixels(size_t(size) * size_t(size), bgColor<color_t>());
    tgx::Image<color_t> im(pixels.data(), size, size);

    renderLargeStressSuite(im);
    rows.push_back(BenchRow{ "cpu-large", color_name, "large_snapshot", "snapshot", 1, 0.0, 0.0, imageHash(im), countChanged(im, bgColor<color_t>()), true, "" });
    const auto bmp = out_dir / ("tgx_2d_cpu_large_" + color_name + "_" + std::to_string(size) + ".bmp");
    if (!writeBMP(bmp, im)) rows.back().ok = false;

    rows.push_back(benchLargeGroup<color_t>(color_name, "large_gradients", 3, size, drawLargeGradients<color_t>));
    rows.push_back(benchLargeGroup<color_t>(color_name, "large_primitives", 2, size, drawLargePrimitives<color_t>));
    rows.push_back(benchLargeGroup<color_t>(color_name, "large_blits_textures", 2, size, drawLargeBlitsTextures<color_t>));
    rows.push_back(benchLargeGroup<color_t>(color_name, "large_text", 2, size, drawLargeText<color_t>));
    rows.push_back(benchLargeGroup<color_t>(color_name, "large_full_suite", 1, size, renderLargeStressSuite<color_t>));
    return rows;
}

template<typename color_t>
std::vector<BenchRow> runForColor(const std::string& color_name, const std::filesystem::path& out_dir, std::vector<SnapshotFile>& snapshots)
{
    std::vector<BenchRow> rows;
    std::vector<color_t> pixels(W * H);
    tgx::Image<color_t> im(pixels.data(), W, H);

    im.fillScreen(bgColor<color_t>());
    const bool samples_ok = validateSamples(im);
    renderFullSuite(im);

    rows.push_back(BenchRow{ "cpu", color_name, "sample_validation", "snapshot", 1, 0.0, 0.0, imageHash(im), countChanged(im, bgColor<color_t>()), samples_ok, samples_ok ? "" : "sample pixel validation failed" });
    const auto bmp = out_dir / ("tgx_2d_cpu_" + color_name + ".bmp");
    const bool wrote = writeBMP(bmp, im);
    if (!wrote) rows.back().ok = false;
    else snapshots.push_back({ color_name, "full_suite_image", bmp.filename().string(), bmp });

    im.fillScreen(bgColor<color_t>());
    renderOpsSuite(im);
    const auto ops_bmp = out_dir / ("tgx_2d_cpu_ops_" + color_name + ".bmp");
    if (!writeBMP(ops_bmp, im)) rows.back().ok = false;
    else snapshots.push_back({ color_name, "ops_suite_image", ops_bmp.filename().string(), ops_bmp });

    rows.push_back(validationGroup(color_name, "image_state_validation", 200, validateImageStateOps<color_t>));
    rows.push_back(validationGroup(color_name, "transfer_fill_validation", 80, validateTransferAndFillOps<color_t>));
    rows.push_back(validationGroup(color_name, "rotated_blit_validation", 120, validateRotatedBlits<color_t>));
    rows.push_back(validationGroup(color_name, "text_layout_validation", 120, validateTextLayout<color_t>));
    rows.push_back(validationGroup(color_name, "primitive_exact_validation", 200, validatePrimitiveExact<color_t>));
    rows.push_back(validationGroup(color_name, "clip_blit_validation", 160, validateClipAndBlit<color_t>));
    rows.push_back(validationGroup(color_name, "image_edge_validation", 160, validateImageEdgeCases<color_t>));
    rows.push_back(validationGroup(color_name, "gradient_opacity_validation", 160, validateGradientAndOpacity<color_t>));
    rows.push_back(validationGroup(color_name, "line_rect_exact_validation", 200, validateLineRectExact<color_t>));
    rows.push_back(validationGroup(color_name, "triangle_quad_validation", 160, validateTriangleQuadExact<color_t>));
    rows.push_back(validationGroup(color_name, "circle_round_validation", 160, validateCircleRoundExact<color_t>));
    rows.push_back(validationGroup(color_name, "aa_primitives_validation", 80, validateAAPrimitives<color_t>));
    rows.push_back(validationGroup(color_name, "curve_polygon_validation", 80, validateCurvePolygonAA<color_t>));
    rows.push_back(validationGroup(color_name, "textured_edge_validation", 80, validateTexturedEdges<color_t>));
    rows.push_back(validationGroup(color_name, "fill_failure_validation", 120, validateFillFailure<color_t>));
    rows.push_back(validationGroup(color_name, "gfx_text_validation", 120, validateGfxText<color_t>));
    rows.push_back(validationGroup(color_name, "text_stress_validation", 80, validateTextStress<color_t>));
    rows.push_back(benchGroup<color_t>(color_name, "pixels_subimages", 50, drawCoreAndPixels<color_t>));
    rows.push_back(benchGroup<color_t>(color_name, "lines_rects", 40, drawLinesAndRects<color_t>));
    rows.push_back(benchGroup<color_t>(color_name, "circles_ellipses", 30, drawCirclesAndEllipses<color_t>));
    rows.push_back(benchGroup<color_t>(color_name, "triangles_quads", 30, drawTrianglesQuads<color_t>));
    rows.push_back(benchGroup<color_t>(color_name, "polygons_splines", 15, drawPolygonsAndSplines<color_t>));
    rows.push_back(benchGroup<color_t>(color_name, "blits_textures", 20, drawBlitsAndTextures<color_t>));
    rows.push_back(benchGroup<color_t>(color_name, "text_fonts", 20, drawTextAndFonts<color_t>));
    rows.push_back(benchGroup<color_t>(color_name, "rounded_arcs", 15, drawArcsAndRounded<color_t>));
    rows.push_back(benchGroup<color_t>(color_name, "textured_triangles", 15, drawTextureTriangles<color_t>));
    rows.push_back(benchGroup<color_t>(color_name, "image_ops", 20, drawImageOps<color_t>));
    rows.push_back(benchGroup<color_t>(color_name, "thick_splines", 12, drawThickSplines<color_t>));
    rows.push_back(benchGroup<color_t>(color_name, "layout_rotations", 20, drawLayoutAndRotations<color_t>));
    rows.push_back(benchGroup<color_t>(color_name, "gfx_text", 30, drawGfxFontPanel<color_t>));
    rows.push_back(benchGroup<color_t>(color_name, "bulk_ops", 20, drawBulkOps<color_t>));
    rows.push_back(benchGroup<color_t>(color_name, "ops_bottom_panels", 15, drawOpsBottomPanels<color_t>));
    rows.push_back(benchGroup<color_t>(color_name, "full_suite", 5, renderFullSuite<color_t>));
    return rows;
}

void writeCSV(const std::filesystem::path& path, const std::vector<BenchRow>& rows)
{
    std::ofstream out(path);
    out << "schema_version,platform,color_type,group,kind,iterations,total_us,per_iter_us,hash,changed_pixels,ok,note\n";
    for (const auto& r : rows)
        {
        out << "1,"
            << r.platform << ','
            << r.color_type << ','
            << r.group << ','
            << r.kind << ','
            << r.iterations << ','
            << std::fixed << std::setprecision(3) << r.total_us << ','
            << std::fixed << std::setprecision(3) << r.per_iter_us << ','
            << "0x" << std::hex << r.hash << std::dec << ','
            << r.changed_pixels << ','
            << (r.ok ? "true" : "false") << ','
            << csvEscape(r.note) << '\n';
        }
}

void writeBaseline(const std::filesystem::path& path, const std::vector<BenchRow>& rows)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path);
    out << "platform,color_type,group,kind,hash,changed_pixels\n";
    for (const auto& r : rows)
        {
        out << r.platform << ','
            << r.color_type << ','
            << r.group << ','
            << r.kind << ','
            << "0x" << std::hex << r.hash << std::dec << ','
            << r.changed_pixels << '\n';
        }
}

bool applyBaseline(const std::map<std::string, BaselineEntry>& baseline, std::vector<BenchRow>& rows, bool strict)
{
    if (baseline.empty()) return true;

    bool ok = true;
    for (auto& r : rows)
        {
        const auto it = baseline.find(rowKey(r.platform, r.color_type, r.group, r.kind));
        if (it == baseline.end())
            {
            if (strict)
                {
                r.ok = false;
                r.note = r.note.empty() ? "missing baseline row" : (r.note + "; missing baseline row");
                ok = false;
                }
            continue;
            }
        const bool same_hash = (r.hash == it->second.hash);
        const bool same_changed = (r.changed_pixels == it->second.changed_pixels);
        if (!same_hash || !same_changed)
            {
            r.ok = false;
            std::ostringstream oss;
            if (!r.note.empty()) oss << r.note << "; ";
            oss << "baseline mismatch expected hash=0x" << std::hex << it->second.hash << std::dec
                << " changed=" << it->second.changed_pixels;
            r.note = oss.str();
            ok = false;
            }
        }

    return ok;
}

void writeGoldenImages(const std::filesystem::path& golden_dir, const std::vector<SnapshotFile>& snapshots)
{
    std::filesystem::create_directories(golden_dir);
    for (const auto& s : snapshots)
        {
        std::filesystem::copy_file(s.path, golden_dir / s.filename, std::filesystem::copy_options::overwrite_existing);
        }
}

bool applyGoldenImages(const std::filesystem::path& golden_dir, const std::filesystem::path& diff_dir, const std::vector<SnapshotFile>& snapshots, std::vector<BenchRow>& rows)
{
    bool ok = true;
    std::error_code ec;
    std::filesystem::remove_all(diff_dir, ec);
    for (const auto& s : snapshots)
        {
        const auto golden_path = golden_dir / s.filename;
        const auto diff_path = diff_dir / ("diff_" + s.filename);
        const auto cmp = compareGoldenBMP(s, golden_path, diff_path);
        rows.push_back(BenchRow{
            "cpu",
            s.color_type,
            s.group,
            "golden",
            1,
            0.0,
            0.0,
            fileHash(s.path),
            cmp.diff_pixels,
            cmp.ok,
            cmp.note
            });
        ok = ok && cmp.ok;
        }
    return ok;
}

Options parseOptions(int argc, char** argv)
{
    Options options;
    for (int i = 1; i < argc; i++)
        {
        const std::string arg = argv[i];
        if ((arg == "--out") && (i + 1 < argc))
            {
            options.out_dir = argv[++i];
            }
        else if ((arg == "--baseline") && (i + 1 < argc))
            {
            options.baseline_path = argv[++i];
            }
        else if ((arg == "--write-baseline") && (i + 1 < argc))
            {
            options.write_baseline_path = argv[++i];
            }
        else if ((arg == "--golden") && (i + 1 < argc))
            {
            options.golden_dir = argv[++i];
            }
        else if ((arg == "--write-golden") && (i + 1 < argc))
            {
            options.write_golden_dir = argv[++i];
            }
        else if ((arg == "--golden-diff") && (i + 1 < argc))
            {
            options.golden_diff_dir = argv[++i];
            }
        else if (arg == "--allow-missing-baseline")
            {
            options.strict_baseline = false;
            }
        else if (arg == "--large")
            {
            options.large = true;
            }
        else if ((arg == "--large-size") && (i + 1 < argc))
            {
            options.large_size = std::max(256, std::stoi(argv[++i]));
            }
        else if (arg.rfind("--", 0) != 0)
            {
            options.out_dir = arg;
            }
        }
    return options;
}

} // namespace

int main(int argc, char** argv)
{
    const auto options = parseOptions(argc, argv);
    const auto out_dir = options.out_dir;
    std::filesystem::create_directories(out_dir);

    std::vector<BenchRow> rows;
    std::vector<SnapshotFile> snapshots;
    auto addRows = [&](std::vector<BenchRow> r)
        {
        rows.insert(rows.end(), r.begin(), r.end());
        };

    addRows(runForColor<tgx::RGB24>("RGB24", out_dir, snapshots));
    addRows(runForColor<tgx::RGB32>("RGB32", out_dir, snapshots));
    addRows(runForColor<tgx::RGBf>("RGBf", out_dir, snapshots));
    rows.push_back(validationGroup("cross", "convert_validation", 200, validateConvertOps));
    if (options.large)
        {
        addRows(runLargeForColor<tgx::RGB24>("RGB24", out_dir, options.large_size));
        addRows(runLargeForColor<tgx::RGB32>("RGB32", out_dir, options.large_size));
        addRows(runLargeForColor<tgx::RGBf>("RGBf", out_dir, options.large_size));
        }

    bool baseline_ok = true;
    if (!options.baseline_path.empty())
        {
        baseline_ok = applyBaseline(loadBaseline(options.baseline_path), rows, options.strict_baseline);
        }

    if (!options.write_baseline_path.empty()) writeBaseline(options.write_baseline_path, rows);
    if (!options.write_golden_dir.empty()) writeGoldenImages(options.write_golden_dir, snapshots);

    bool golden_ok = true;
    if (!options.golden_dir.empty())
        {
        const auto diff_dir = options.golden_diff_dir.empty() ? (out_dir / "golden_diff") : options.golden_diff_dir;
        golden_ok = applyGoldenImages(options.golden_dir, diff_dir, snapshots, rows);
        }

    writeCSV(out_dir / "tgx_2d_cpu_results.csv", rows);

    bool ok = true;
    for (const auto& r : rows)
        {
        ok = ok && r.ok;
        std::cout << std::left << std::setw(8) << r.color_type
                  << " " << std::setw(22) << r.group
                  << " " << std::right << std::setw(10) << std::fixed << std::setprecision(1) << r.per_iter_us
                  << " us/iter"
                  << " changed=" << r.changed_pixels
                  << " hash=0x" << std::hex << r.hash << std::dec
                  << " " << (r.ok ? "OK" : "FAIL") << "\n";
        }

    ok = ok && baseline_ok;
    ok = ok && golden_ok;
    std::cout << "Outputs: " << out_dir.string() << "\n";
    return ok ? 0 : 1;
}
