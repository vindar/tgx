/********************************************************************
*
* TGX 2D optional integration validation and benchmark suite.
*
* EXAMPLE FOR TEENSY 4 / 4.1
* DISPLAY: ILI9341 (320x240)
*
********************************************************************/

#include <ILI9341_T4.h>
#include <tgx.h>
#include <font_tgx_OpenSans.h>
#include <font_tgx_OpenSans_Bold.h>

#ifndef TGX_2D_BENCH_HAS_PNGDEC
#define TGX_2D_BENCH_HAS_PNGDEC 0
#endif

#ifndef TGX_2D_BENCH_HAS_JPEGDEC
#define TGX_2D_BENCH_HAS_JPEGDEC 0
#endif

#ifndef TGX_2D_BENCH_HAS_ANIMATEDGIF
#define TGX_2D_BENCH_HAS_ANIMATEDGIF 0
#endif

#ifndef TGX_2D_BENCH_HAS_OPENFONTRENDER
#define TGX_2D_BENCH_HAS_OPENFONTRENDER 0
#endif

#if TGX_2D_BENCH_HAS_PNGDEC
#include <PNGdec.h>
#include <examples/Teensy4/2D/PNG_test/octocat_4bpp.h>
#endif

#if TGX_2D_BENCH_HAS_JPEGDEC
#include <JPEGDEC.h>
#include <examples/Teensy4/2D/JPEG_test/batman.h>
#endif

#if TGX_2D_BENCH_HAS_ANIMATEDGIF
#include <AnimatedGIF.h>
#include <examples/Teensy4/2D/GIF_test/earth_128x128.h>
#endif

#if TGX_2D_BENCH_HAS_OPENFONTRENDER
#include <OpenFontRender.h>
#include <examples/Teensy4/2D/OpenFontRender_test/NotoSans_Bold.h>
#endif

using namespace tgx;

#define PIN_SCK     13
#define PIN_MISO    12
#define PIN_MOSI    11
#define PIN_DC      10
#define PIN_CS      9
#define PIN_RESET   6
#define PIN_BACKLIGHT 255
#define PIN_TOUCH_IRQ 255
#define PIN_TOUCH_CS  255

#define SPI_SPEED 40000000

static const int SLX = 320;
static const int SLY = 240;

ILI9341_T4::ILI9341Driver tft(PIN_CS, PIN_DC, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_RESET, PIN_TOUCH_CS, PIN_TOUCH_IRQ);

DMAMEM ILI9341_T4::DiffBuffStatic<6000> diff1;
DMAMEM ILI9341_T4::DiffBuffStatic<6000> diff2;
DMAMEM uint16_t fb[SLX * SLY];
DMAMEM uint16_t internal_fb[SLX * SLY];
Image<RGB565> im(fb, SLX, SLY);

const char* RESULT_PLATFORM = "teensy4";
const char* RESULT_COLOR = "RGB565";

#if TGX_2D_BENCH_HAS_PNGDEC
PNG pngBench;
#endif

#if TGX_2D_BENCH_HAS_JPEGDEC
JPEGDEC jpegBench;
#endif

#if TGX_2D_BENCH_HAS_ANIMATEDGIF
AnimatedGIF gifBench;
#endif

#if TGX_2D_BENCH_HAS_OPENFONTRENDER
OpenFontRender ofrBench;
bool ofrBenchReady = false;
#endif

uint32_t fnv1a(uint32_t h, uint8_t b)
{
    h ^= b;
    h *= 16777619u;
    return h;
}

uint32_t imageHash()
{
    uint32_t h = 2166136261u;
    for (int i = 0; i < SLX * SLY; i++)
        {
        const uint16_t v = fb[i];
        h = fnv1a(h, (uint8_t)(v & 255));
        h = fnv1a(h, (uint8_t)(v >> 8));
        }
    return h;
}

uint32_t countChanged(RGB565 bg)
{
    uint32_t n = 0;
    for (int i = 0; i < SLX * SLY; i++) if (fb[i] != bg.val) n++;
    return n;
}

void title(const char* text)
{
    im.fillRect(iBox2(0, 319, 0, 22), RGB565(2, 6, 11));
    im.drawTextEx(text, { 160, 17 }, font_tgx_OpenSans_Bold_14, CENTER, false, false, RGB565_White);
}

void displayPage()
{
    tft.update(fb);
    delay(900);
}

void printResultCsv(const char* name, const char* kind, uint32_t iterations, uint32_t total, uint32_t hash, uint32_t changed, bool ok)
{
    Serial.print("RESULT,1,");
    Serial.print(RESULT_PLATFORM);
    Serial.print(",");
    Serial.print(RESULT_COLOR);
    Serial.print(",");
    Serial.print(name);
    Serial.print(",");
    Serial.print(kind);
    Serial.print(",");
    Serial.print(iterations);
    Serial.print(",");
    Serial.print(total);
    Serial.print(",");
    Serial.print((float)total / (float)iterations, 2);
    Serial.print(",0x");
    Serial.print(hash, HEX);
    Serial.print(",");
    Serial.print(changed);
    Serial.print(",");
    Serial.print(ok ? "true" : "false");
    Serial.println(",");
}

template<typename ValidateFn>
void runValidation(const char* name, uint32_t iterations, ValidateFn validate)
{
    bool ok = false;
    const uint32_t t0 = micros();
    for (uint32_t i = 0; i < iterations; i++) ok = validate();
    const uint32_t total = micros() - t0;

    Serial.print(name);
    Serial.print(", iterations=");
    Serial.print(iterations);
    Serial.print(", total_us=");
    Serial.print(total);
    Serial.print(", per_iter_us=");
    Serial.print((float)total / (float)iterations, 2);
    Serial.print(", ok=");
    Serial.println(ok ? "true" : "false");
    printResultCsv(name, "validation", iterations, total, 0, 0, ok);
}

template<typename DrawFn>
void runBench(const char* name, uint32_t iterations, DrawFn draw)
{
    const RGB565 bg = RGB565(1, 2, 4);
    const uint32_t t0 = micros();
    for (uint32_t i = 0; i < iterations; i++)
        {
        im.clear(bg);
        draw();
        }
    const uint32_t total = micros() - t0;
    const uint32_t changed = countChanged(bg);
    const uint32_t hash = imageHash();
    const bool ok = changed > 0;

    Serial.print(name);
    Serial.print(", iterations=");
    Serial.print(iterations);
    Serial.print(", total_us=");
    Serial.print(total);
    Serial.print(", per_iter_us=");
    Serial.print((float)total / (float)iterations, 2);
    Serial.print(", changed=");
    Serial.print(changed);
    Serial.print(", hash=0x");
    Serial.print(hash, HEX);
    Serial.print(", ok=");
    Serial.println(ok ? "true" : "false");
    printResultCsv(name, "benchmark", iterations, total, hash, changed, ok);
}

#if TGX_2D_BENCH_HAS_PNGDEC
void drawPNGPanel()
{
    title("PNGdec wrapper");
    im.fillScreenHGradient(RGB565(1, 8, 20), RGB565(28, 8, 6));
    pngBench.openRAM((uint8_t*)octocat_4bpp, sizeof(octocat_4bpp), TGX_PNGDraw);
    im.PNGDecode(pngBench, { 6, 54 }, 1.0f);
    pngBench.close();
    pngBench.openRAM((uint8_t*)octocat_4bpp, sizeof(octocat_4bpp), TGX_PNGDraw);
    im.PNGDecode(pngBench, { 106, 54 }, 0.65f);
    pngBench.close();
    pngBench.openRAM((uint8_t*)octocat_4bpp, sizeof(octocat_4bpp), TGX_PNGDraw);
    im.PNGDecode(pngBench, { 216, 54 }, 0.35f);
    pngBench.close();
}

bool validatePNG()
{
    const RGB565 bg = RGB565(1, 2, 4);
    im.clear(bg);
    pngBench.openRAM((uint8_t*)octocat_4bpp, sizeof(octocat_4bpp), TGX_PNGDraw);
    im.PNGDecode(pngBench, { 24, 44 }, 1.0f);
    pngBench.close();
    const uint32_t fullCount = countChanged(bg);
    if (fullCount < 2500) return false;

    im.clear(bg);
    pngBench.openRAM((uint8_t*)octocat_4bpp, sizeof(octocat_4bpp), TGX_PNGDraw);
    im.PNGDecode(pngBench, { -48, -36 }, 1.0f);
    pngBench.close();
    const uint32_t clippedCount = countChanged(bg);
    if (clippedCount == 0 || clippedCount >= fullCount) return false;

    im.clear(bg);
    pngBench.openRAM((uint8_t*)octocat_4bpp, sizeof(octocat_4bpp), TGX_PNGDraw);
    im.PNGDecode(pngBench, { 24, 44 }, 0.0f);
    pngBench.close();
    return countChanged(bg) == 0;
}
#endif

#if TGX_2D_BENCH_HAS_JPEGDEC
void drawJPEGPanel()
{
    title("JPEGDEC wrapper");
    im.fillScreenVGradient(RGB565(2, 4, 12), RGB565(20, 18, 4));
    jpegBench.openRAM((uint8_t*)batman, sizeof(batman), TGX_JPEGDraw);
    im.JPEGDecode(jpegBench, { 18, 64 }, JPEG_SCALE_HALF, 1.0f);
    jpegBench.close();
    jpegBench.openRAM((uint8_t*)batman, sizeof(batman), TGX_JPEGDraw);
    im.JPEGDecode(jpegBench, { 174, 64 }, JPEG_SCALE_HALF, 0.55f);
    jpegBench.close();
}

bool validateJPEG()
{
    const RGB565 bg = RGB565(1, 2, 4);
    im.clear(bg);
    jpegBench.openRAM((uint8_t*)batman, sizeof(batman), TGX_JPEGDraw);
    im.JPEGDecode(jpegBench, { 72, 48 }, JPEG_SCALE_HALF, 1.0f);
    jpegBench.close();
    const uint32_t fullCount = countChanged(bg);
    if (fullCount < 5000) return false;

    im.clear(bg);
    jpegBench.openRAM((uint8_t*)batman, sizeof(batman), TGX_JPEGDraw);
    im.JPEGDecode(jpegBench, { -64, 32 }, JPEG_SCALE_HALF, 1.0f);
    jpegBench.close();
    const uint32_t clippedCount = countChanged(bg);
    if (clippedCount == 0 || clippedCount >= fullCount) return false;

    im.clear(bg);
    jpegBench.openRAM((uint8_t*)batman, sizeof(batman), TGX_JPEGDraw);
    im.JPEGDecode(jpegBench, { 72, 48 }, JPEG_SCALE_HALF, 0.0f);
    jpegBench.close();
    return countChanged(bg) == 0;
}
#endif

#if TGX_2D_BENCH_HAS_ANIMATEDGIF
void drawGIFPanel()
{
    title("AnimatedGIF wrapper");
    im.fillScreenVGradient(RGB565_Black, RGB565(1, 6, 14));
    gifBench.open((uint8_t*)earth_128x128, sizeof(earth_128x128), TGX_GIFDraw);
    im.GIFplayFrame(gifBench, { 24, 62 }, 1.0f);
    im.GIFplayFrame(gifBench, { 96, 62 }, 0.7f);
    im.GIFplayFrame(gifBench, { 168, 62 }, 0.45f);
    gifBench.close();
}

bool validateGIF()
{
    const RGB565 bg = RGB565(1, 2, 4);
    im.clear(bg);
    gifBench.open((uint8_t*)earth_128x128, sizeof(earth_128x128), TGX_GIFDraw);
    const int rc = im.GIFplayFrame(gifBench, { 96, 56 }, 1.0f);
    gifBench.close();
    const uint32_t fullCount = countChanged(bg);
    if (rc == 0 || fullCount < 5000) return false;

    im.clear(bg);
    gifBench.open((uint8_t*)earth_128x128, sizeof(earth_128x128), TGX_GIFDraw);
    im.GIFplayFrame(gifBench, { -44, 56 }, 1.0f);
    gifBench.close();
    const uint32_t clippedCount = countChanged(bg);
    if (clippedCount == 0 || clippedCount >= fullCount) return false;

    im.clear(bg);
    gifBench.open((uint8_t*)earth_128x128, sizeof(earth_128x128), TGX_GIFDraw);
    im.GIFplayFrame(gifBench, { 96, 56 }, 0.0f);
    gifBench.close();
    return countChanged(bg) == 0;
}
#endif

#if TGX_2D_BENCH_HAS_OPENFONTRENDER
void drawOpenFontRenderPanel()
{
    title("OpenFontRender wrapper");
    im.fillScreenHGradient(RGB565(4, 4, 14), RGB565(4, 18, 12));
    if (!ofrBenchReady)
        {
        im.drawTextEx("font load failed", { 160, 128 }, font_tgx_OpenSans_Bold_14, CENTER, false, false, RGB565_Red);
        return;
        }

    im.setOpenFontRender(ofrBench, 0.86f);
    ofrBench.setFontColor((uint16_t)RGB565_Yellow, (uint16_t)RGB565_Black);
    ofrBench.setLineSpaceRatio(0.85);
    ofrBench.setFontSize(26);
    ofrBench.setCursor(160, 68);
    ofrBench.cprintf("TGX + TTF\n");
    ofrBench.setFontSize(18);
    ofrBench.cprintf("OpenFontRender\n");
    ofrBench.setFontSize(14);
    ofrBench.cprintf("opacity and callbacks");
}

bool validateOpenFontRender()
{
    if (!ofrBenchReady) return false;
    const RGB565 bg = RGB565(1, 2, 4);
    im.clear(bg);
    im.setOpenFontRender(ofrBench, 1.0f);
    ofrBench.setFontColor((uint16_t)RGB565_Cyan, (uint16_t)bg);
    ofrBench.setLineSpaceRatio(0.85);
    ofrBench.setFontSize(24);
    ofrBench.setCursor(160, 60);
    ofrBench.cprintf("TGX OFR");
    if (countChanged(bg) < 200) return false;

    im.clear(bg);
    im.setOpenFontRender(ofrBench, 0.0f);
    ofrBench.setFontColor((uint16_t)RGB565_Yellow, (uint16_t)bg);
    ofrBench.setFontSize(24);
    ofrBench.setCursor(160, 60);
    ofrBench.cprintf("TGX OFR");
    return countChanged(bg) == 0;
}
#endif

void waitForStart()
{
    im.clear(RGB565_Black);
    title("TGX optional 2D ready");
    im.drawTextEx("Send any character", { 160, 114 }, font_tgx_OpenSans_18, CENTER, false, false, RGB565_Cyan);
    im.drawTextEx("PNG/JPEG/GIF/OpenFontRender", { 160, 158 }, font_tgx_OpenSans_10, CENTER, false, false, RGB565_Yellow);
    tft.update(fb);

    while (!Serial) delay(10);
    while (Serial.available()) Serial.read();

    Serial.println("TGX 2D Teensy optional validation/benchmark suite");
    Serial.println("Config: Teensy 4.1, 600MHz, Optimize Fastest, USB Serial");
    Serial.println("RESULT_SCHEMA,schema_version,platform,color_type,group,kind,iterations,total_us,per_iter_us,hash,changed_pixels,ok,note");
    Serial.println("Send any character to start.");

    while (!Serial.available()) delay(10);
    while (Serial.available()) Serial.read();
    Serial.println("Starting...");
}

void runSuite()
{
#if TGX_2D_BENCH_HAS_OPENFONTRENDER
    ofrBenchReady = (ofrBench.loadFont(NotoSans_Bold, sizeof(NotoSans_Bold)) == 0);
#endif

#if TGX_2D_BENCH_HAS_PNGDEC
    runValidation("pngdec_val", 12, validatePNG);
    im.clear(RGB565(1, 2, 4)); drawPNGPanel(); displayPage(); runBench("pngdec", 4, drawPNGPanel);
#else
    Serial.println("pngdec=skipped");
#endif

#if TGX_2D_BENCH_HAS_JPEGDEC
    runValidation("jpegdec_val", 8, validateJPEG);
    im.clear(RGB565(1, 2, 4)); drawJPEGPanel(); displayPage(); runBench("jpegdec", 4, drawJPEGPanel);
#else
    Serial.println("jpegdec=skipped");
#endif

#if TGX_2D_BENCH_HAS_ANIMATEDGIF
    runValidation("animatedgif_val", 4, validateGIF);
    im.clear(RGB565(1, 2, 4)); drawGIFPanel(); displayPage(); runBench("animatedgif", 2, drawGIFPanel);
#else
    Serial.println("animatedgif=skipped");
#endif

#if TGX_2D_BENCH_HAS_OPENFONTRENDER
    runValidation("openfontrender_val", 12, validateOpenFontRender);
    im.clear(RGB565(1, 2, 4)); drawOpenFontRenderPanel(); displayPage(); runBench("openfontrender", 8, drawOpenFontRenderPanel);
#else
    Serial.println("openfontrender=skipped");
#endif

    im.clear(RGB565_Black);
    title("TGX optional 2D done");
    im.drawTextEx("Run complete", { 160, 118 }, font_tgx_OpenSans_Bold_14, CENTER, false, false, RGB565_Green);
    tft.update(fb);
    Serial.println("TGX 2D suite done");
    delay(2500);
}

void setup()
{
    Serial.begin(115200);
    tft.output(&Serial);
    while (!tft.begin(SPI_SPEED));

    pinMode(PIN_BACKLIGHT, OUTPUT);
    digitalWrite(PIN_BACKLIGHT, HIGH);

    tft.setRotation(3);
    tft.setFramebuffer(internal_fb);
    tft.setDiffBuffers(&diff1, &diff2);
    tft.setDiffGap(4);
    tft.setRefreshRate(140);
    tft.setVSyncSpacing(2);
    delay(250);
}

void loop()
{
    waitForStart();
    runSuite();
}
