/********************************************************************
*
* TGX library example: S.A.R.C.A.S.M. screen demo.
*
* EXAMPLE FOR TEENSY 4.1
*
* DISPLAY: ILI9341 (320x240) with ILI9341_T4.
*
* This sketch is a compact demo extracted from the S.A.R.C.A.S.M.
* Rubik's cube solving robot. It keeps the TGX display style, espeak
* voice and fake cube-solving sequence, but removes the solver,
* motors, sensors, SD music, LED strip and multithreaded robot runtime.
*
********************************************************************/

#include <Arduino.h>
#include <Audio.h>
#include <ILI9341_T4.h>
#include <tgx.h>
#include <espeak-ng_T4.h>

#include "src/assets/font_SourceCodePro_AA4.h"
#include "src/assets/font_SourceCodePro_Bold_AA4.h"
#include "src/assets/intro_face.h"
#include "src/assets/intro_rubik.h"
#include "src/assets/sarcasm_bk.h"
#include "src/assets/sarcasm_bk_blink.h"
#include "src/assets/sarcasm_bk_closemouth.h"
#include "src/assets/sarcasm_bk_closemouth_blink.h"
#include "src/assets/sarcasm_bk_openmouth.h"
#include "src/assets/sarcasm_bk_openmouth_blink.h"
#include "src/assets/sarcasm_voice.h"
#include "src/cube/RubikCube3D.h"

#include "src/3Dmodels/cam_small.h"
#include "src/3Dmodels/mainframe_small.h"
#include "src/3Dmodels/cover_small.h"
#include "src/3Dmodels/bouton_noir_small.h"
#include "src/3Dmodels/bouton_rouge_small.h"
#include "src/3Dmodels/usb_small.h"
#include "src/3Dmodels/cradle_small.h"
#include "src/3Dmodels/reddots_small.h"
#include "src/3Dmodels/gripper_small.h"
#include "src/3Dmodels/gripper_red_small.h"
#include "src/3Dmodels/tumble_small.h"
#include "src/3Dmodels/sarcasm_aconym_small_texture.h"
#include "src/3Dmodels/companion_cube.h"
#include "src/3Dmodels/companion_cube_texture.h"

using namespace tgx;

//
// DEFAULT WIRING USING SPI 0 ON TEENSY 4/4.1
//
#define PIN_SCK       13
#define PIN_MISO      12
#define PIN_MOSI      11
#define PIN_DC        10
#define PIN_CS        9
#define PIN_RESET     6
#define PIN_BACKLIGHT 255
#define PIN_TOUCH_IRQ 255
#define PIN_TOUCH_CS  255

#define SPI_SPEED 30000000

// Set this to 1 to use the espeak-ng_T4 voice output through Teensy Audio I2S.
#define SARCASM_DEMO_ENABLE_VOICE 1

// Set to a real pin only if your I2S amplifier needs a power-enable signal.
#define SARCASM_DEMO_AUDIO_PWR_PIN 255

static const int SLX = 320;
static const int SLY = 240;

ILI9341_T4::ILI9341Driver tft(PIN_CS, PIN_DC, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_RESET, PIN_TOUCH_CS, PIN_TOUCH_IRQ);
DMAMEM ILI9341_T4::DiffBuffStatic<8000> diff1;
DMAMEM ILI9341_T4::DiffBuffStatic<8000> diff2;

uint16_t fb[SLX * SLY];
DMAMEM uint16_t internal_fb[SLX * SLY];
DMAMEM uint16_t zbuf[SLX * SLY];
EXTMEM uint16_t alt_fb[SLX * SLY];

Image<RGB565> im(fb, SLX, SLY);

static const int STICKER_LX = 32;
static const int STICKER_COUNT = 64;
DMAMEM uint16_t sticker_pixels[STICKER_LX * STICKER_LX * STICKER_COUNT];
Image<RGB565> sticker_texture(sticker_pixels, STICKER_LX, STICKER_LX * STICKER_COUNT);

const Shader LOADED_SHADERS =
    SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_FLAT | SHADER_NOTEXTURE |
    SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_CLAMP;

Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;
RubikCube3D cube3D;

#if SARCASM_DEMO_ENABLE_VOICE
extern "C" uint8_t external_psram_size;
AudioOutputI2S audioOut;
AudioAmplifier voiceAmp;
AudioConnection patchCord1(espeak, 0, voiceAmp, 0);
AudioConnection patchCord2(voiceAmp, 0, audioOut, 0);
AudioConnection patchCord3(voiceAmp, 0, audioOut, 1);
static bool voice_ready = false;
#endif

struct ConsoleLine
    {
    char text[28];
    RGB565 color;
    };

static const int CONSOLE_ROWS = 10;
static const int CONSOLE_COLS = 25;
static ConsoleLine console_lines[CONSOLE_ROWS];
static int console_count = 0;
static bool console_cursor = true;
static int console_cursor_x = 0;
static int console_cursor_y = 0;
static RGB565 console_current_color = RGB565_Yellow;
static bool voice_active = false;
static uint32_t talking_until_ms = 0;
static uint32_t rng_state = 0x53415243u;

static const RGB565 COLOR_UI = RGB32(65, 135, 113);
static const RGB565 COLOR_UI_DIM = RGB32(45, 95, 79);
static const RGB565 COLOR_TEXT = RGB565_Yellow;
static const RGB565 COLOR_TAG = RGB565(8, 34, 14);
static const RGB565 COLOR_DONE = RGB565(0, 35, 5);
static const RGB565 COLOR_ABORT = RGB565(18, 0, 0);

uint32_t rng32()
    {
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
    }

int rngInt(int n)
    {
    return (int)(rng32() % (uint32_t)n);
    }

void copyConsoleText(char* dst, size_t dstSize, const char* src)
    {
    if (dstSize == 0) return;
    size_t n = strlen(src);
    if (n >= dstSize) n = dstSize - 1;
    memcpy(dst, src, n);
    dst[n] = 0;
    }

int rngInt(int a, int b)
    {
    return a + rngInt(b - a + 1);
    }

float clamp01(float x)
    {
    return (x < 0.0f) ? 0.0f : ((x > 1.0f) ? 1.0f : x);
    }

namespace DemoFaceAction
    {
    struct FaceParams
        {
        float op;
        int bitquant;
        float wave_speed;
        float wave_amplitude;
        float wave_length;
        };

    static elapsedMillis face_em;
    static uint32_t face_len = 1;
    static FaceParams face_start = { 0.0f, 7, 20.0f, 10.0f, 5.0f };
    static FaceParams face_cur = { 0.0f, 7, 20.0f, 10.0f, 5.0f };
    static FaceParams face_end = { 0.0f, 7, 20.0f, 10.0f, 5.0f };
    static float face_phase = 0.0f;
    static int face_state = 0;
    static int face_substate = 0;
    static float face_multscale = 1.0f;
    static int prev_good_state = -1;
    static Image<RGB565> src_image;
    static bool speaking_now = false;
    static uint32_t speaking_recent_until = 0;

    static const FaceParams state_visible_cube_replay = { 0.7f, 0, 0.0f, 0.0f, 0.0f };
    static const FaceParams state_visible_cube = { 0.6f, 0, 0.0f, 0.0f, 0.0f };
    static const FaceParams state_visible_cube_solved = { 0.7f, 0, 0.0f, 0.0f, 0.0f };
    static const FaceParams state_visible_good = { 0.4f, 0, 0.0f, 0.0f, 0.0f };
    static const FaceParams state_visible_dark = { 0.2f, 3, 0.0f, 0.0f, 0.0f };
    static const FaceParams state_visible_hlines = { 0.4f, 7, 0.0f, 0.0f, 0.0f };
    static const FaceParams state_visible_waves = { 0.4f, 1, 1.0f, 4.0f, 2.0f };
    static const FaceParams state_invisible = { 0.0f, 7, 20.0f, 10.0f, 5.0f };
    static const FaceParams state_visible_dreaming = { 0.7f, 0, 0.0f, 0.0f, 30.0f };
    static const FaceParams state_visible_dreaming_end = { 0.9f, 7, 0.1f, 10.0f, 50.0f };

    static const uint32_t BLINK_MIN_MS = 1300;
    static const uint32_t BLINK_MAX_MS = 5000;
    static const uint32_t BLINK_LEN_MIN_MS = 60;
    static const uint32_t BLINK_LEN_MAX_MS = 120;
    static const uint8_t BLINK_DOUBLE_CHANCE = 15;
    static bool blink_active = false;
    static bool blink_inited = false;
    static uint32_t blink_len = 0;
    static elapsedMillis blink_em = 0;
    static elapsedMillis blink_gap_em = 0;
    static uint32_t blink_next_ms = 0;
    static int prev_mouth = 0;
    static uint32_t shuteyes_len = 0;
    static elapsedMillis shuteyes_em = 0;

    float normalize(float x, float tmin, float tmax)
        {
        const float d = tmax - tmin;
        if (d <= 0.0f) return 0.5f;
        if (x < tmin) return 0.0f;
        if (x > tmax) return 1.0f;
        return (x - tmin) / d;
        }

    float lerpFloat(float x, float a, float b)
        {
        return a + (b - a) * x;
        }

    FaceParams lerpParams(float r, const FaceParams& a, const FaceParams& b)
        {
        FaceParams p;
        p.op = lerpFloat(r, a.op, b.op);
        p.bitquant = roundf(lerpFloat(r, (float)a.bitquant, (float)b.bitquant));
        p.wave_speed = lerpFloat(r, a.wave_speed, b.wave_speed);
        p.wave_amplitude = lerpFloat(r, a.wave_amplitude, b.wave_amplitude);
        p.wave_length = lerpFloat(r, a.wave_length, b.wave_length);
        return p;
        }

    void setSpeaking(bool speaking)
        {
        speaking_now = speaking;
        if (speaking) speaking_recent_until = millis() + 250;
        }

    void useFace()
        {
        src_image = Image<RGB565>();
        }

    void useImageInsteadOfFace(const Image<RGB565>& src)
        {
        src_image = src;
        }

    FLASHMEM void blendIm(Image<RGB565> dst, Image<RGB565> src, const FaceParams& p, float phase)
        {
        if (p.op <= 0.0f) return;
        if (p.op >= 1.0f)
            {
            dst.blit(src, { 0, 0 });
            return;
            }
        const float wave_freq = 1.0f / p.wave_length;
        const int mask = (1 << p.bitquant) - 1;
        const uint32_t alpha = (uint32_t)(p.op * 255.0f);
        const uint32_t a1 = alpha >> p.bitquant;
        const uint32_t rem = alpha - (a1 << p.bitquant);
        for (int y = 0; y < SLY; ++y)
            {
            RGB565* d = dst.data() + y * SLX;
            const RGB565* s = src.data() + y * SLX;
            uint32_t a = (((rng32() & mask) < rem) ? (a1 + 1) : a1) << p.bitquant;
            const int off = sinf((y + phase) * wave_freq) * p.wave_amplitude;
            if (off >= 0)
                {
                for (int x = 0; x < SLX - off; ++x) d[x + off].blend256(s[x], a);
                }
            else
                {
                for (int x = 0; x < SLX + off; ++x) d[x].blend256(s[x - off], a);
                }
            }
        }

    int mouthState()
        {
        const bool speaking = speaking_now || (millis() < speaking_recent_until);
        if (!speaking) return 0;
        return ((millis() / 115) & 1) ? 1 : 2; // 0 neutral, 1 open, 2 close
        }

    FLASHMEM void advanceState()
        {
        face_start = face_end;
        face_cur = face_end;
        switch (face_state)
            {
            case 0:
                face_end = state_invisible;
                face_len = 10000;
                face_em = 0;
                break;
            case 1:
                switch (face_substate)
                    {
                    case 0:
                        face_end = face_cur;
                        face_substate = 1;
                        face_len = rngInt(10, 6000);
                        face_em = 0;
                        break;
                    case 1:
                        {
                        int next_state = 0;
                        for (int k = 0; k < 2; ++k)
                            {
                            const int a = rngInt(100);
                            next_state = (a < 33) ? 0 : ((a < 66) ? 1 : 2);
                            if (next_state != prev_good_state) break;
                            }
                        prev_good_state = next_state;
                        face_em = 0;
                        if (next_state == 0)
                            {
                            face_end = state_visible_dark;
                            face_end.op = rngInt(15, 30) / 100.0f;
                            face_substate = 2;
                            face_len = rngInt(1000, 3000);
                            }
                        else if (next_state == 1)
                            {
                            face_end = state_visible_hlines;
                            face_end.bitquant = rngInt(5, 10);
                            if (face_end.bitquant > 7) face_end.bitquant = 7;
                            face_substate = 4;
                            face_len = rngInt(500, 1000);
                            }
                        else
                            {
                            face_end = state_visible_waves;
                            face_end.bitquant = rngInt(0, 10);
                            if (face_end.bitquant > 7) face_end.bitquant = 0;
                            face_end.wave_speed = rngInt(5, 20) / 10.0f;
                            face_end.wave_amplitude = rngInt(2, 7);
                            face_end.wave_length = rngInt(2, 6);
                            face_substate = 6;
                            face_len = rngInt(200, 2000);
                            }
                        break;
                        }
                    case 2:
                        face_end = face_cur;
                        face_substate = 3;
                        face_len = rngInt(10, 1500);
                        face_em = 0;
                        break;
                    case 3:
                        face_end = state_visible_good;
                        face_substate = 0;
                        face_len = rngInt(1000, 2000);
                        face_em = 0;
                        break;
                    case 4:
                        face_end = face_cur;
                        face_substate = 5;
                        face_len = rngInt(10, 1500);
                        face_em = 0;
                        break;
                    case 5:
                        face_end = state_visible_good;
                        face_substate = 0;
                        face_len = rngInt(100, 1000);
                        face_em = 0;
                        break;
                    case 6:
                        face_end = face_cur;
                        face_substate = 7;
                        face_len = rngInt(10, 1500);
                        face_em = 0;
                        break;
                    default:
                        face_end = state_visible_good;
                        face_substate = 0;
                        face_len = rngInt(100, 1000);
                        face_em = 0;
                        break;
                    }
                break;
            case 4:
                if (face_substate == 0)
                    {
                    face_end = { 0.55f, 5, 2.0f, 4.0f, 20.0f };
                    face_substate = 1;
                    face_len = (uint32_t)(1000 * face_multscale);
                    face_em = 0;
                    }
                else if (face_substate == 1)
                    {
                    face_end = { 0.45f, 1, 1.0f, 2.0f, 25.0f };
                    face_substate = 2;
                    face_len = (uint32_t)(2000 * face_multscale);
                    face_em = 0;
                    }
                else if (face_substate == 2)
                    {
                    face_end = state_visible_good;
                    face_substate = 3;
                    face_len = (uint32_t)(1000 * face_multscale);
                    face_em = 0;
                    }
                else
                    {
                    face_end = state_visible_good;
                    face_state = 1;
                    face_substate = 0;
                    face_len = rngInt(1000, 3000);
                    face_em = 0;
                    }
                break;
            case 5:
                face_end = state_visible_good;
                face_state = 1;
                face_substate = 0;
                face_len = rngInt(500, 2000);
                face_em = 0;
                break;
            case 6:
            case 8:
                face_end = state_invisible;
                face_state = 0;
                face_substate = 0;
                face_len = 10;
                face_em = 0;
                break;
            case 7:
                face_end = state_visible_cube;
                face_state = 9;
                face_substate = 0;
                face_len = 1000;
                face_em = 0;
                break;
            case 9:
                if (face_substate == 0)
                    {
                    face_end = face_cur;
                    face_substate = 1;
                    face_len = rngInt(4000, 8000);
                    face_em = 0;
                    }
                else if (face_substate == 1)
                    {
                    const int next_state = (rngInt(100) < 30) ? 0 : 1;
                    prev_good_state = next_state;
                    face_em = 0;
                    if (next_state == 0)
                        {
                        face_end = state_visible_hlines;
                        face_end.bitquant = rngInt(4, 10);
                        if (face_end.bitquant > 7) face_end.bitquant = 7;
                        face_substate = 2;
                        face_len = rngInt(500, 1000);
                        }
                    else
                        {
                        face_end = state_visible_waves;
                        face_end.bitquant = 0;
                        face_end.wave_speed = rngInt(3, 20) / 10.0f;
                        face_end.wave_length = rngInt(6, 40);
                        face_end.wave_amplitude = rngInt((int)(face_end.wave_length / 4), (int)(face_end.wave_length / 3));
                        face_substate = 4;
                        face_len = rngInt(1000, 2500);
                        }
                    }
                else if (face_substate == 2 || face_substate == 4)
                    {
                    face_end = face_cur;
                    face_substate++;
                    face_len = rngInt(100, 1500);
                    face_em = 0;
                    }
                else
                    {
                    face_end = state_visible_cube;
                    face_substate = 0;
                    face_len = rngInt(100, 1500);
                    face_em = 0;
                    }
                break;
            case 10:
                face_end = state_visible_cube_solved;
                face_len = 10000;
                face_em = 0;
                break;
            case 11:
                if (face_substate == 0)
                    {
                    face_end = face_cur;
                    face_substate = 1;
                    face_len = rngInt(1000, 3000);
                    face_em = 0;
                    }
                else
                    {
                    face_end.wave_length = rngInt(5, 80);
                    face_end.wave_amplitude = face_end.wave_length / 3.0f;
                    face_end.wave_speed = rngInt(10, 60) / 100.0f;
                    face_substate = 0;
                    face_len = rngInt(2000, 4000);
                    face_em = 0;
                    }
                break;
            case 12:
                if (face_substate == 0)
                    {
                    face_end = face_cur;
                    face_substate = 1;
                    face_len = rngInt(3000, 7000);
                    face_em = 0;
                    }
                else if (face_substate == 1)
                    {
                    face_end = face_start;
                    face_end.bitquant = 0;
                    face_substate = 2;
                    face_len = rngInt(100, 1000);
                    face_em = 0;
                    }
                else
                    {
                    face_end = state_visible_cube_replay;
                    face_substate = 0;
                    face_len = rngInt(500, 2000);
                    face_em = 0;
                    }
                break;
            case 13:
                face_end = state_visible_dreaming_end;
                face_substate = 0;
                face_len = 10000;
                face_em = 0;
                break;
            default:
                face_state = 1;
                face_end = state_visible_good;
                face_len = 1000;
                face_em = 0;
                break;
            }
        }

    FLASHMEM void drawFace(Image<RGB565>& dst)
        {
        const uint32_t t = face_em;
        const float r = normalize((float)t, 0.0f, (float)face_len);
        face_cur = lerpParams(r, face_start, face_end);
        if (rngInt(2) == 1) face_phase += face_cur.wave_speed;
        else face_phase -= face_cur.wave_speed;

        if (!blink_inited)
            {
            blink_inited = true;
            blink_next_ms = rngInt(BLINK_MIN_MS, BLINK_MAX_MS);
            }

        const int mouth = mouthState();
        const bool speaking = speaking_now || (millis() < speaking_recent_until);
        if (!blink_active && prev_mouth == 1 && !speaking)
            {
            blink_active = true;
            blink_len = rngInt(BLINK_LEN_MIN_MS, BLINK_LEN_MAX_MS);
            blink_em = 0;
            blink_gap_em = 0;
            blink_next_ms = rngInt(BLINK_MIN_MS, BLINK_MAX_MS);
            }
        if (!blink_active)
            {
            if (blink_gap_em >= blink_next_ms && !speaking)
                {
                blink_active = true;
                blink_len = rngInt(BLINK_LEN_MIN_MS, BLINK_LEN_MAX_MS);
                blink_em = 0;
                blink_gap_em = 0;
                }
            else if (speaking)
                {
                blink_next_ms += 40;
                }
            }
        else if (blink_em >= blink_len)
            {
            blink_active = false;
            blink_next_ms = (!speaking && rngInt(100) < BLINK_DOUBLE_CHANCE) ? rngInt(120, 220) : rngInt(BLINK_MIN_MS, BLINK_MAX_MS);
            }
        const bool useblink = blink_active || (shuteyes_em < shuteyes_len);
        prev_mouth = mouth;

        Image<RGB565> face = sarcasm_bk;
        if (mouth == 1) face = useblink ? sarcasm_bk_openmouth_blink : sarcasm_bk_openmouth;
        else if (mouth == 2) face = useblink ? sarcasm_bk_closemouth_blink : sarcasm_bk_closemouth;
        else face = useblink ? sarcasm_bk_blink : sarcasm_bk;

        if (face_cur.op < 1.0f) dst.clear(RGB565_Black);
        blendIm(dst, src_image.isValid() ? src_image : face, face_cur, face_phase);

        if (t >= face_len) advanceState();
        }

    FLASHMEM void appear_intro(float scale = 1.0f)
        {
        useFace();
        face_multscale = scale;
        face_start = state_invisible;
        face_cur = state_invisible;
        face_end = { 0.4f, 6, 10.0f, 4.0f, 10.0f };
        face_state = 4;
        face_substate = 0;
        face_len = (uint32_t)(3000 * face_multscale);
        face_em = 0;
        }

    FLASHMEM void appear(int ms = -1)
        {
        useFace();
        if (ms <= 0) ms = rngInt(500, 2000);
        int q = rngInt(9);
        if (q > 7) q = 7;
        if (q < 3) q = 3;
        const int sp = rngInt(1, 5);
        const int length = rngInt(2, 6);
        int ampl = rngInt(0, 10);
        if (ampl > 5) ampl = 0;
        face_start = (face_cur.op < 0.1f) ? FaceParams { 0.0f, q, (float)sp, (float)ampl, (float)length } : face_cur;
        face_end = state_visible_good;
        int a = rngInt(10);
        if (a > 6) a = 0;
        face_end.bitquant = a;
        face_state = 5;
        face_substate = 0;
        face_len = ms;
        face_multscale = 1.0f;
        face_em = 0;
        }

    FLASHMEM void disapear(int ms = -1)
        {
        if (ms <= 0) ms = rngInt(1500, 3500);
        int q = rngInt(12);
        if (q > 7) q = 7;
        if (q < 3) q = 3;
        const int sp = rngInt(1, 5);
        const int length = rngInt(2, 6);
        int ampl = rngInt(0, 10);
        if (ampl > 5) ampl = 0;
        face_end = { 0.0f, q, (float)sp, (float)ampl, (float)length };
        face_start = face_cur;
        face_state = 6;
        face_substate = 0;
        face_len = ms;
        face_multscale = 1.0f;
        face_em = 0;
        }

    FLASHMEM void cubeAppear(const Image<RGB565>& src, int ms = -1, bool solved = false, bool replay = false)
        {
        useImageInsteadOfFace(src);
        if (ms <= 0) ms = rngInt(1000, 2000);
        face_start = replay ? state_invisible : face_cur;
        face_end = replay ? state_visible_cube_replay : (solved ? state_visible_cube_solved : state_visible_cube);
        face_state = replay ? 12 : (solved ? 10 : 7);
        face_substate = 0;
        face_len = ms;
        face_multscale = 1.0f;
        face_em = 0;
        }

    FLASHMEM void cubeDisappear(int ms = -1)
        {
        if (ms <= 0) ms = rngInt(1500, 3500);
        int q = rngInt(12);
        if (q > 7) q = 7;
        if (q < 3) q = 3;
        const int sp = rngInt(1, 5);
        const int length = rngInt(7, 30);
        int ampl = rngInt(0, 10);
        if (ampl > length) ampl = length;
        face_end = { 0.0f, q, (float)sp, (float)ampl, (float)length };
        face_start = face_cur;
        face_state = 8;
        face_substate = 0;
        face_len = ms;
        face_multscale = 1.0f;
        face_em = 0;
        }

    FLASHMEM void dreamingAppear(const Image<RGB565>& src, int ms)
        {
        useImageInsteadOfFace(src);
        if (ms <= 0) ms = rngInt(1000, 3000);
        const float sp = rngInt(60, 120) / 100.0f;
        const float length = rngInt(30, 100);
        const float ampl = length / 3.0f;
        face_start = (face_cur.op < 0.1f) ? FaceParams { 0.0f, 3, sp, ampl, length } : face_cur;
        face_end = state_visible_dreaming;
        face_state = 11;
        face_substate = 0;
        face_len = ms;
        face_multscale = 1.0f;
        face_em = 0;
        }

    FLASHMEM void dreamingEnd(const Image<RGB565>& src, int ms)
        {
        useImageInsteadOfFace(src);
        face_end = state_visible_dreaming_end;
        face_state = 13;
        face_substate = 0;
        face_len = ms;
        face_multscale = 1.0f;
        face_em = 0;
        }

    FLASHMEM void dreamingDisappear(int ms)
        {
        face_end = { 0.0f, 3, 0.1f, 2.0f, 5.0f };
        face_start = face_cur;
        face_state = 8;
        face_substate = 0;
        face_len = ms;
        face_multscale = 1.0f;
        face_em = 0;
        }
    }

void waitUpdate()
    {
    while (tft.asyncUpdateActive()) delay(1);
    }

void updateScreen(bool force = false)
    {
    waitUpdate();
    tft.update((const uint16_t*)im.data(), force);
    }

void updateScreenRegion(iBox2 region, bool redrawNow)
    {
    waitUpdate();
    tft.updateRegion(redrawNow, (const uint16_t*)&im(region.minX, region.minY),
                     region.minX, region.maxX, region.minY, region.maxY, im.stride());
    }

void consoleClear()
    {
    console_count = 0;
    console_cursor_x = 0;
    console_cursor_y = 0;
    for (int i = 0; i < CONSOLE_ROWS; ++i)
        {
        console_lines[i].text[0] = 0;
        console_lines[i].color = COLOR_TEXT;
        }
    }

void consoleScroll()
    {
    for (int i = 1; i < CONSOLE_ROWS; ++i) console_lines[i - 1] = console_lines[i];
    console_lines[CONSOLE_ROWS - 1].text[0] = 0;
    console_lines[CONSOLE_ROWS - 1].color = COLOR_TEXT;
    if (console_count > 0) console_count--;
    if (console_cursor_y > 0) console_cursor_y--;
    }

void consoleSetCursor(int x, int y)
    {
    if (x < 0) x = 0;
    if (x > CONSOLE_COLS - 1) x = CONSOLE_COLS - 1;
    if (y < 0) y = 0;
    while (y >= CONSOLE_ROWS) { consoleScroll(); y--; }
    console_cursor_x = x;
    console_cursor_y = y;
    if (console_count <= y) console_count = y + 1;
    }

void consoleNewLine()
    {
    console_cursor_x = 0;
    console_cursor_y++;
    if (console_cursor_y >= CONSOLE_ROWS)
        {
        consoleScroll();
        console_cursor_y = CONSOLE_ROWS - 1;
        }
    if (console_count <= console_cursor_y)
        {
        console_lines[console_cursor_y].text[0] = 0;
        console_lines[console_cursor_y].color = console_current_color;
        console_count = console_cursor_y + 1;
        }
    }

void consolePutChar(char c)
    {
    if (c == '\n')
        {
        consoleNewLine();
        return;
        }
    if (c < 32) c = ' ';
    if (console_cursor_x >= CONSOLE_COLS) consoleNewLine();
    if (console_count <= console_cursor_y) console_count = console_cursor_y + 1;

    char* line = console_lines[console_cursor_y].text;
    int len = strlen(line);
    while (len < console_cursor_x && len < CONSOLE_COLS) line[len++] = ' ';
    if (console_cursor_x < CONSOLE_COLS)
        {
        line[console_cursor_x] = c;
        if (console_cursor_x >= len) line[console_cursor_x + 1] = 0;
        console_lines[console_cursor_y].color = console_current_color;
        console_cursor_x++;
        }
    if (console_cursor_x >= CONSOLE_COLS) consoleNewLine();
    }

void consolePush(const char* text, RGB565 color = COLOR_TEXT)
    {
    if (console_count >= CONSOLE_ROWS)
        {
        consoleScroll();
        }
    copyConsoleText(console_lines[console_count].text, sizeof(console_lines[console_count].text), text);
    console_lines[console_count].color = color;
    console_current_color = color;
    console_cursor_y = console_count;
    console_cursor_x = min(CONSOLE_COLS - 1, (int)strlen(console_lines[console_count].text));
    console_count++;
    }

void consoleSetLast(const char* text, RGB565 color)
    {
    if (console_count <= 0) consolePush("", color);
    copyConsoleText(console_lines[console_count - 1].text, sizeof(console_lines[console_count - 1].text), text);
    console_lines[console_count - 1].color = color;
    console_current_color = color;
    console_cursor_y = console_count - 1;
    console_cursor_x = min(CONSOLE_COLS - 1, (int)strlen(console_lines[console_count - 1].text));
    }

const Image<RGB565>& selectedFaceImage()
    {
    const uint32_t now = millis();
    const bool speaking = voice_active || (now < talking_until_ms);
    const bool blink = ((now % 4300) > 4150) || (((now + 700) % 6700) > 6580);

    if (blink) return sarcasm_bk_blink;
    if (!speaking) return sarcasm_bk;
    return (((now / 130) & 1) == 0) ? sarcasm_bk_openmouth : sarcasm_bk_closemouth;
    }

void blendFaceEffect(Image<RGB565>& dst, const Image<RGB565>& src, float opacity, int bitquant, float waveAmp, float waveLen, float phase)
    {
    if (opacity <= 0.0f) return;
    if (opacity >= 0.99f && waveAmp <= 0.01f)
        {
        dst.blit(src, { 0, 0 });
        return;
        }

    const uint32_t alpha = (uint32_t)(clamp01(opacity) * 255.0f);
    const uint32_t mask = (1u << bitquant) - 1u;
    const uint32_t a1 = alpha >> bitquant;
    const uint32_t rem = alpha - (a1 << bitquant);
    const float freq = 1.0f / waveLen;

    for (int y = 0; y < SLY; ++y)
        {
        RGB565* p = dst.data() + y * SLX;
        const RGB565* q = src.data() + y * SLX;
        uint32_t a = (((rng32() & mask) < rem) ? (a1 + 1u) : a1) << bitquant;
        int off = (int)(sinf((y + phase) * freq) * waveAmp);

        if (off >= 0)
            {
            for (int x = 0; x < SLX - off; ++x) p[x + off].blend256(q[x], a);
            }
        else
            {
            for (int x = 0; x < SLX + off; ++x) p[x].blend256(q[x - off], a);
            }
        }
    }

void drawHeaderAndConsole(int activity = -1, const char* state = nullptr)
    {
    im.drawTextEx("SARCASM CMD", { 7, 5 }, font_SourceCodePro_Bold_AA4_16, Anchor::TOPLEFT, false, false, COLOR_UI, 1.0f);
    im.drawThickRoundRectAA(fBox2(3, 317, 20, 237), 9, 3, COLOR_UI, 1.0f);

    const int ax = 125;
    const int ay = 4;
    const int aw = 11;
    const int ah = 11;
    im.drawRect({ ax, ax + 9 * (aw + 1), ay, ay + ah + 1 }, COLOR_UI);
    for (int k = 1; k < 9; ++k) im.drawFastVLine({ ax + k * (aw + 1), ay }, ah + 1, COLOR_UI);
    for (int k = 0; k < 9; ++k)
        {
        if (activity >= 0 && k <= activity)
            {
            im.fillRect({ ax + k * (aw + 1) + 1, ax + (k + 1) * (aw + 1) - 1, ay + 1, ay + ah }, COLOR_UI_DIM);
            }
        }

    im.drawTextEx("7.42V", { 266, 5 }, font_SourceCodePro_AA4_16, Anchor::TOPLEFT, false, false, COLOR_UI, 1.0f);
    (void)state;

    for (int i = 0; i < console_count; ++i)
        {
        const float opacity = (console_lines[i].color == COLOR_TEXT) ? 0.60f : 1.0f;
        im.drawTextEx(console_lines[i].text, { 11, 30 + i * 20 + 15 }, font_SourceCodePro_AA4_20, Anchor::DEFAULT_TEXT_ANCHOR, false, false, console_lines[i].color, opacity);
        }

    if (console_cursor && ((millis() / 450) & 1))
        {
        const int x = 12 + 12 * console_cursor_x;
        const int y = 43 + 20 * console_cursor_y;
        im.fillRect({ x, x + 8, y, y + 3 }, console_current_color);
        }
    }

void drawConsoleSpinner()
    {
    const fVec2 cc = { 300.0f, 40.0f };
    const float t = fmodf(millis() / 800.0f, 2.0f);
    const float r = 8.0f;
    const RGB565 color = RGB565(18, 0, 0);
    if (t < 1.0f)
        {
        im.drawThickCircleArcAA(cc, r, 0, 360.0f * t, r * t, color, 1.0f);
        }
    else
        {
        im.drawThickCircleArcAA(cc, r, (t - 1.0f) * 360.0f, 360.0f, r * (2.0f - t), color, 1.0f);
        }
    }

void drawFaceFrame(int activity = -1, const char* state = nullptr)
    {
    DemoFaceAction::setSpeaking(voice_active || (millis() < talking_until_ms));
    DemoFaceAction::drawFace(im);
    drawHeaderAndConsole(activity, state);
    }

void drawUIOnlyFrame()
    {
    DemoFaceAction::setSpeaking(false);
    DemoFaceAction::drawFace(im);
    im.drawTextEx("SARCASM CMD", { 7, 5 }, font_SourceCodePro_Bold_AA4_16, Anchor::TOPLEFT, false, false, COLOR_UI, 1.0f);
    im.drawThickRoundRectAA(fBox2(3, 317, 20, 237), 9, 3, COLOR_UI, 1.0f);
    const int ax = 125;
    const int ay = 4;
    const int aw = 11;
    const int ah = 11;
    im.drawRect({ ax, ax + 9 * (aw + 1), ay, ay + ah + 1 }, COLOR_UI);
    for (int k = 1; k < 9; ++k) im.drawFastVLine({ ax + k * (aw + 1), ay }, ah + 1, COLOR_UI);
    im.drawTextEx("7.42V", { 266, 5 }, font_SourceCodePro_AA4_16, Anchor::TOPLEFT, false, false, COLOR_UI, 1.0f);
    }

void drawFinalUIFrameTo(Image<RGB565>& dst)
    {
    DemoFaceAction::setSpeaking(false);
    DemoFaceAction::drawFace(dst);
    dst.drawTextEx("SARCASM CMD", { 7, 5 }, font_SourceCodePro_Bold_AA4_16, Anchor::TOPLEFT, false, false, COLOR_UI, 1.0f);
    dst.drawThickRoundRectAA(fBox2(3, 317, 20, 237), 9, 3, COLOR_UI, 1.0f);
    const int ax = 125;
    const int ay = 4;
    const int aw = 11;
    const int ah = 11;
    dst.drawRect({ ax, ax + 9 * (aw + 1), ay, ay + ah + 1 }, COLOR_UI);
    for (int k = 1; k < 9; ++k) dst.drawFastVLine({ ax + k * (aw + 1), ay }, ah + 1, COLOR_UI);
    dst.drawTextEx("7.42V", { 266, 5 }, font_SourceCodePro_AA4_16, Anchor::TOPLEFT, false, false, COLOR_UI, 1.0f);
    }

void copyIntroWave(uint16_t* dest, const uint16_t* src)
    {
    static int phase = 0;
    phase += 2.5f;
    int off = 0;
    for (int y = 1; y < SLY - 1; ++y)
        {
        memcpy(dest + SLX * y, src + SLX * y + off, SLX * sizeof(uint16_t));
        off = sinf((y + phase) * (1.0f / 40.0f)) * 20.0f;
        }
    }

void drawIntroFrame(uint32_t elapsedMs)
    {
    Image<RGB565> intro_rubik_cache((RGB565*)zbuf, SLX, SLY);
    const float r0 = ((float)elapsedMs) / 3800.0f;
    const float r = (r0 > 1.0f) ? 1.0f : r0;
    const float u = (r - 0.65f) / 0.35f;
    im.clear(RGB565_Black);
    copyIntroWave((uint16_t*)im.data(), (const uint16_t*)intro_rubik_cache.data());

    int alpha = rng32() & 0xff;
    if (alpha > 120) alpha = 120;
    if (alpha > 50) im.blit(intro_face, { 0, 0 }, alpha / 255.0f);

    uint32_t noiselvl = (uint32_t)(255.0f * (1.0f - r * 3.0f));
    for (int y = 0; y < SLY; ++y)
        {
        for (int x = 0; x < SLX; ++x)
            {
            uint32_t a = rng32();
            if ((a & 0xffu) < noiselvl)
                {
                im(x, y) = (RGB565)((uint16_t)((a >> 8) & 0xffff));
                }
            if (u > 0.0f && ((rng32() & 0xffu) < (uint32_t)(r * 256.0f)))
                {
                im(x, y) = RGB565_Black;
                }
            }
        }

    if (u > 0.0f)
        {
        im.blitScaledRotated(sarcasm_bk, { 160, 120 }, { 160, 120 }, u, (1.0f - u) * (1.0f - u) * 360.0f, 1.0f - r);
        }
    }

Image<RGB565> stickerImage(int index)
    {
    if (index < 0 || index >= STICKER_COUNT) index = STICKER_COUNT - 1;
    return sticker_texture.getCrop({ 0, STICKER_LX - 1, index * STICKER_LX, index * STICKER_LX + STICKER_LX - 1 });
    }

void makeSticker(int index, RGB565 color)
    {
    Image<RGB565> st = stickerImage(index);
    st.clear(RGB32(45, 95, 79));
    st.fillRoundRect({ 3, STICKER_LX - 4, 3, STICKER_LX - 4 }, 5, color);
    st.drawRoundRect({ 3, STICKER_LX - 4, 3, STICKER_LX - 4 }, 5, RGB32(20, 55, 44));
    }

void initStickers()
    {
    const RGB565 colors[6] = { RGB565_White, RGB565_Red, RGB565_Green, RGB565_Yellow, RGB565_Orange, RGB565_Blue };
    for (int face = 0; face < 6; ++face)
        {
        for (int i = 0; i < 9; ++i)
            {
            makeSticker(face * 9 + i, colors[face]);
            }
        }
    makeSticker(63, RGB32(65, 135, 113));
    }

void initCube()
    {
    initStickers();
    cube3D.init(sticker_texture, STICKER_COUNT, STICKER_COUNT - 1, RGB32(45, 95, 79));
    int stickers[54];
    for (int i = 0; i < 54; ++i) stickers[i] = i;
    cube3D.setStickers_int(stickers);
    cube3D.resetPosition();
    }

void initRenderer()
    {
    renderer.setViewportSize(SLX, SLY);
    renderer.setOffset(0, 0);
    renderer.setImage(&im);
    renderer.setZbuffer(zbuf);
    renderer.setPerspective(45, (float)SLX / SLY, 1.0f, 100.0f);
    renderer.setLookAt({ 0, 25, 36 }, { 0, 2.5f, 0 }, { 0, 1, 0 });
    renderer.setLight({ -1, -2, -1 }, RGBf(1.0f, 1.0f, 1.0f), RGBf(1.0f, 1.0f, 1.0f), RGBf(1.0f, 1.0f, 1.0f));
    renderer.setCulling(1);
    renderer.setTextureWrappingMode(SHADER_TEXTURE_CLAMP);
    }

Image<RGB565> transitionSourceImage()
    {
    return Image<RGB565>((RGB565*)alt_fb, SLX, SLY);
    }

namespace MachineAnimation
    {
    static Image<RGB565> target;
    static const fVec3 Cscreen = { 93.996328f, -0.036100f, 28.937351f };
    static const fVec3 Nscreen = { 0.967075f, 0.0f, 0.254493f };
    static const float GRIPPER_OPEN_DEG = 0.0f;
    static const float GRIPPER_CLOSED_DEG = 90.0f;
    static const float TUMBLE_PARKED_DEG = 55.0f;
    static const float TUMBLE_UP_DEG = -70.0f;

    static const char* seq = nullptr;
    static int ii = 0;
    static int nbtot = 0;
    static int phase = 0;
    static elapsedMillis em;
    static elapsedMillis ema;
    static elapsedMillis emb;

    float accel(float t, float a0, float a1)
        {
        t = clamp01(t);
        const float t0 = (a0 == 1.0f) ? t : powf(t, a0);
        const float t1 = (a1 == 1.0f) ? (1.0f - t) : powf(1.0f - t, a1);
        return t0 / (t0 + t1);
        }

    float lerp(float x, float x0, float x1, float y0 = 0.0f, float y1 = 1.0f, float a0 = 1.0f, float a1 = 1.0f)
        {
        const float t = (x - x0) / (x1 - x0);
        if (t <= 0.0f) return y0;
        if (t >= 1.0f) return y1;
        return y0 + (y1 - y0) * accel(t, a0, a1);
        }

    FLASHMEM void setupRenderer(const Image<RGB565>& dst)
        {
        target = dst;
        renderer.setViewportSize(SLX, SLY);
        renderer.setOffset(0, 0);
        renderer.setImage(&target);
        renderer.setZbuffer(zbuf);
        renderer.setPerspective(60.0f, (float)SLX / SLY, 10.0f, 5000.0f);
        renderer.setLight({ -1, -2, -1 }, RGBf(1.0f, 1.0f, 1.0f), RGBf(1.0f, 1.0f, 1.0f), RGBf(1.0f, 1.0f, 1.0f));
        renderer.setCulling(1);
        }

    FLASHMEM fMat4 cubePositionInCradle(float cradleRot, const fMat4& modelTransform)
        {
        fMat4 M;
        M.setIdentity();
        M.multRotate(cradleRot, { 0, 0, 1 });
        M.multScale({ 27, 27, 27 });
        M.multRotate(6, { 0, -1, 0 });
        M.multTranslate({ 0, 0, 24 });
        M.multTranslate({ 52, 0, 72 });
        return modelTransform * M;
        }

    FLASHMEM void drawTexturedQuad(const fMat4& modelTransform, const Image<RGB565>& texture)
        {
        const fVec2 t1 = { 0.0f, 1.0f };
        const fVec2 t2 = { 0.0f, 0.0f };
        const fVec2 t3 = { 1.0f, 0.0f };
        const fVec2 t4 = { 1.0f, 1.0f };
        fMat4 M;
        M.setIdentity();
        M.multScale({ 20.0f, 20.0f, 20.0f });
        M.multTranslate({ -42, -55.5f, 72 });
        renderer.setModelMatrix(modelTransform * M);
        const fVec3 P1 = { -1.0f, 0.0f, 2.0f };
        const fVec3 P2 = { -1.0f, 0.0f, -2.0f };
        const fVec3 P3 = { 1.0f, 0.0f, -2.0f };
        const fVec3 P4 = { 1.0f, 0.0f, 2.0f };
        renderer.setMaterial(RGBf(1.0f, 1.0f, 1.0f), 0.2f, 0.7f, 0.5f, 8);
        renderer.setShaders(SHADER_FLAT | SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_CLAMP);
        renderer.drawQuad(P1, P2, P3, P4, nullptr, nullptr, nullptr, nullptr, &t1, &t2, &t3, &t4, &texture);
        }

    FLASHMEM void drawDisplayQuad(const Image<RGB565>* displayImage)
        {
        if (!displayImage) displayImage = &im;
        const fVec2 t1 = { 0.0f, 1.0f };
        const fVec2 t2 = { 0.0f, 0.0f };
        const fVec2 t3 = { 1.0f, 0.0f };
        const fVec2 t4 = { 1.0f, 1.0f };
        const fVec3 Q1 = { 89.323831f, -24.5161f, 46.692845f };
        const fVec3 Q2 = { 98.668825f, -24.5161f, 11.181857f };
        const fVec3 Q3 = { 98.668825f, 24.4439f, 11.181857f };
        const fVec3 Q4 = { 89.323831f, 24.4439f, 46.692845f };
        renderer.setShaders(SHADER_FLAT | SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_CLAMP);
        renderer.setTextureWrappingMode(SHADER_TEXTURE_CLAMP);
        renderer.setMaterial(RGBf(1.0f, 1.0f, 1.0f), 0.2f, 0.7f, 0.5f, 8);
        renderer.drawQuad(Q1, Q2, Q3, Q4, nullptr, nullptr, nullptr, nullptr, &t1, &t4, &t3, &t2, displayImage);
        }

    FLASHMEM void drawSARCASM3D(float cradleRot, float gripperRot, float tumbleRot, const fMat4& modelTransform, const Image<RGB565>* displayImage = nullptr)
        {
        fMat4 M;
        M.setIdentity();

        renderer.setModelMatrix(modelTransform * M);
        renderer.setShaders(SHADER_FLAT | SHADER_NOTEXTURE);
        renderer.setMaterial(RGBf(0.35f, 0.35f, 0.35f), 0.2f, 0.7f, 0.5f, 8);
        renderer.drawMesh(&cover_small, false);

        renderer.setMaterial(RGBf(1.0f, 1.0f, 1.0f), 0.2f, 0.7f, 0.5f, 8);
        renderer.drawMesh(&mainframe_small, false);

        drawDisplayQuad(displayImage);
        renderer.setShaders(SHADER_GOURAUD | SHADER_NOTEXTURE);
        renderer.setMaterial(RGBf(0.3f, 0.3f, 0.3f), 0.2f, 0.7f, 0.5f, 8);
        renderer.drawMesh(&bouton_noir_small, false);
        renderer.setMaterial(RGBf(1.0f, 0.85f, 0.0f), 0.2f, 0.7f, 0.5f, 8);
        renderer.drawMesh(&bouton_rouge_small, false);
        renderer.drawMesh(&usb_small, false);

        renderer.setMaterial(RGBf(0.4f, 0.3f, 0.6f), 0.1f, 0.3f, 0.8f, 64);
        renderer.drawMesh(&cam_small, false);
        drawTexturedQuad(modelTransform, sarcasm_aconym_small_texture);

        M.setIdentity();
        M.multRotate(cradleRot, { 0, 0, 1 });
        M.multRotate(6, { 0, -1, 0 });
        M.multTranslate({ 52, 0, 72 });
        renderer.setModelMatrix(modelTransform * M);
        renderer.setShaders(SHADER_FLAT | SHADER_NOTEXTURE);
        renderer.setMaterial(RGBf(1.0f, 1.0f, 1.0f), 0.2f, 0.7f, 0.5f, 8);
        renderer.drawMesh(&cradle_small, false);
        renderer.setMaterial(RGBf(1.0f, 0.85f, 0.0f), 0.3f, 0.3f, 0.8f, 64);
        renderer.drawMesh(&reddots_small_1, false);

        M.setIdentity();
        M.multRotate(gripperRot, { 0, 1, 0 });
        M.multTranslate({ 2, 0, 127 });
        renderer.setModelMatrix(modelTransform * M);
        renderer.setMaterial(RGBf(1.0f, 1.0f, 1.0f), 0.2f, 0.7f, 0.5f, 8);
        renderer.drawMesh(&gripper_small, false);
        renderer.setMaterial(RGBf(1.0f, 0.85f, 0.0f), 0.3f, 0.3f, 0.8f, 64);
        renderer.drawMesh(&gripper_red_small, false);

        M.setIdentity();
        M.multRotate(tumbleRot, { 0, 1, 0 });
        M.multTranslate({ -19, 0, 72 });
        renderer.setModelMatrix(modelTransform * M);
        renderer.setMaterial(RGBf(1.0f, 1.0f, 1.0f), 0.2f, 0.7f, 0.5f, 8);
        renderer.drawMesh(&tumble_small, false);
        }

    FLASHMEM void drawOpenWithCube(const fMat4& M, float cradleRot)
        {
        drawSARCASM3D(cradleRot, GRIPPER_OPEN_DEG, TUMBLE_PARKED_DEG, M);
        fMat4 MC;
        MC.setScale(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f);
        MC.multRotate(90, { 1, 0, 0 });
        MC.multRotate(cradleRot + 90, { 0, 0, 1 });
        cube3D.drawCube(cubePositionInCradle(0, M) * MC, renderer);
        }

    FLASHMEM void drawOpeningGrip(const fMat4& M, float r)
        {
        drawSARCASM3D(0, lerp(r, 0, 1, GRIPPER_CLOSED_DEG, GRIPPER_OPEN_DEG), TUMBLE_PARKED_DEG, M);
        fMat4 MC;
        MC.setScale(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f);
        MC.multRotate(90, { 1, 0, 0 });
        MC.multRotate(90, { 0, 0, 1 });
        cube3D.drawCube(cubePositionInCradle(0, M) * MC, renderer);
        }

    FLASHMEM void drawClosingGrip(const fMat4& M, float r)
        {
        drawSARCASM3D(0, lerp(r, 0, 1, GRIPPER_OPEN_DEG, GRIPPER_CLOSED_DEG), TUMBLE_PARKED_DEG, M);
        fMat4 MC;
        MC.setScale(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f);
        MC.multRotate(90, { 1, 0, 0 });
        MC.multRotate(90, { 0, 0, 1 });
        cube3D.drawCube(cubePositionInCradle(0, M) * MC, renderer);
        }

    FLASHMEM void drawGripOpened(const fMat4& M) { drawClosingGrip(M, 0); }

    FLASHMEM void drawRotatingGripOpen(const fMat4& M, int dir, float r)
        {
        r = lerp(r, 0, 1, 0, 1, 2, 2);
        const float cradleRot = -90.0f * dir * r;
        drawSARCASM3D(cradleRot, GRIPPER_OPEN_DEG, TUMBLE_PARKED_DEG, M);
        fMat4 MC;
        MC.setScale(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f);
        MC.multRotate(90, { 1, 0, 0 });
        MC.multRotate(90, { 0, 0, 1 });
        cube3D.draw_turn(cubePositionInCradle(0, M) * MC, renderer, dir, r);
        }

    FLASHMEM void drawRotatingGripClosed(const fMat4& M, int dir, float r)
        {
        r = lerp(r, 0, 1, 0, 1, 2, 2);
        const float cradleRot = -90.0f * dir * r;
        drawSARCASM3D(cradleRot, GRIPPER_CLOSED_DEG, TUMBLE_PARKED_DEG, M);
        fMat4 MC;
        MC.setScale(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f);
        MC.multRotate(90, { 1, 0, 0 });
        MC.multRotate(90, { 0, 0, 1 });
        cube3D.draw_turn_bottom(cubePositionInCradle(0, M) * MC, renderer, dir, r);
        }

    FLASHMEM void drawTumbling(const fMat4& M, float r)
        {
        const float s = (r < 0.45f) ? lerp(r, 0, 0.45f, 0, 0.5f, 1, 2) : ((r > 0.55f) ? lerp(r, 0.55f, 1, 0.5f, 1, 2, 1) : 0.5f);
        const float tumbleRot = (r < 0.5f) ? lerp(r, 0, 0.5f, TUMBLE_PARKED_DEG, TUMBLE_UP_DEG, 1, 2) : lerp(r, 0.5f, 1, TUMBLE_UP_DEG, TUMBLE_PARKED_DEG, 2, 1);
        drawSARCASM3D(0, GRIPPER_OPEN_DEG, tumbleRot, M);
        fMat4 MC;
        MC.setScale(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f);
        MC.multRotate(90, { 1, 0, 0 });
        MC.multRotate(90, { 0, 0, 1 });
        cube3D.draw_tumble(cubePositionInCradle(0, M) * MC, renderer, s);
        }

    FLASHMEM int makeReverseCube3D(const char* s)
        {
        if (!s) return 0;
        const int cdir = 1;
        int total = 0;
        while (s[total]) total++;
        for (int i = total - 1; i >= 0; --i)
            {
            switch (s[i])
                {
                case 'T': cube3D.tumble(); cube3D.tumble(); cube3D.tumble(); break;
                case 'R': cube3D.turn(-cdir); break;
                case 'V': cube3D.turn(cdir); break;
                case 'A': cube3D.turn(2); break;
                case 'r': cube3D.turn_bottom(-cdir); break;
                case 'v': cube3D.turn_bottom(cdir); break;
                case 'a': cube3D.turn_bottom(2); break;
                default: break;
                }
            }
        return total;
        }

    FLASHMEM void initSolving(const Image<RGB565>& dst, const char* s)
        {
        setupRenderer(dst);
        cube3D.resetPosition();
        nbtot = makeReverseCube3D(s);
        seq = s;
        ii = 0;
        phase = -1;
        em = 0;
        ema = 0;
        emb = 0;
        fMat4 M;
        M.setIdentity();
        cube3D.drawCubePartial(M, -2, renderer, true);
        }

    FLASHMEM int totalMoves() { return nbtot; }
    FLASHMEM int remainingMoves() { return nbtot - ii; }

    FLASHMEM bool solvingSub(const fMat4& M)
        {
        const float TUMBLING_MS = 600.0f;
        const float ROT_MS = 500.0f;
        const float ROT2_MS = 800.0f;
        const float GRIP_CLOSE_MS = 180.0f;
        const float GRIP_OPEN_MS = 180.0f;
        const float PAUSE_AFTER_TUMBLE_MS = 450.0f;
        static int adir = 2;
        const int cdir = 1;
        const char mm = seq[ii];
        const float t = (float)em;
        if (mm == 0) { drawGripOpened(M); ema = 0; return true; }
        switch (mm)
            {
            case 'T':
                if (phase == 0)
                    {
                    drawTumbling(M, lerp(t, 0, TUMBLING_MS));
                    if (t >= TUMBLING_MS) { cube3D.tumble(); em = 0; phase = 1; }
                    }
                else
                    {
                    drawGripOpened(M);
                    if (t >= PAUSE_AFTER_TUMBLE_MS) { ii++; em = 0; phase = 0; }
                    }
                return false;
            case 'R':
            case 'V':
                {
                const int dir = (mm == 'R') ? cdir : -cdir;
                drawRotatingGripOpen(M, dir, lerp(t, 0, ROT_MS, 0, 1, 2, 2));
                if (t >= ROT_MS) { cube3D.turn(dir); ii++; em = 0; phase = 0; }
                return false;
                }
            case 'A':
                drawRotatingGripOpen(M, adir, lerp(t, 0, ROT2_MS, 0, 1, 2, 2));
                if (t >= ROT2_MS) { cube3D.turn(adir); ii++; em = 0; phase = 0; adir = -adir; }
                return false;
            }
        if (phase == 0)
            {
            drawClosingGrip(M, lerp(t, 0, GRIP_CLOSE_MS, 0, 1, 2, 2));
            if (t >= GRIP_CLOSE_MS) { em = 0; phase = 1; }
            return false;
            }
        if (phase == 2)
            {
            drawOpeningGrip(M, lerp(t, 0, GRIP_OPEN_MS, 0, 1, 2, 2));
            if (t >= GRIP_OPEN_MS) { ii++; em = 0; phase = 0; }
            return false;
            }
        switch (mm)
            {
            case 'r':
                drawRotatingGripClosed(M, cdir, lerp(t, 0, ROT_MS, 0, 1, 2, 2));
                if (t >= ROT_MS) { cube3D.turn_bottom(cdir); em = 0; phase = 2; }
                return false;
            case 'v':
                drawRotatingGripClosed(M, -cdir, lerp(t, 0, ROT_MS, 0, 1, 2, 2));
                if (t >= ROT_MS) { cube3D.turn_bottom(-cdir); em = 0; phase = 2; }
                return false;
            case 'a':
                drawRotatingGripClosed(M, adir, lerp(t, 0, ROT2_MS, 0, 1, 2, 2));
                if (t >= ROT2_MS) { cube3D.turn_bottom(adir); em = 0; phase = 2; adir = -adir; }
                return false;
            default:
                ii++;
                em = 0;
                phase = 0;
                return false;
            }
        }

    FLASHMEM int solvingStep()
        {
        const float ms_before_stop_close = 3000.0f;
        const float ms_before_wirecube_appear = 4000.0f;
        const float ms_before_cubelets_appear = 4500.0f;
        const float ms_before_cubelets_end = 7000.0f;
        const float ms_before_start_solve = 8000.0f;
        const float ms_before_dive = 2000.0f;
        const float ms_dive = 5000.0f;
        const float ms_end = 3000.0f;
        const float camera_height = 120.0f;
        const float camera_look_height = 92.0f;
        const float camera_distance_far = 4000.0f;
        float camera_distance = 117.0f - 50.0f * cosf((((int)emb - ms_before_stop_close) / 3300.0f));
        float modelRotStart = sinf(emb / 2000.0f) * 45.0f;
        if (phase == -1 && em < ms_before_stop_close) modelRotStart += 360.0f * lerp(em, 0, ms_before_stop_close, 0, 1, 1, 2);

        if (phase == -1)
            {
            const float l = lerp(em, 0, ms_before_stop_close, camera_distance_far, camera_distance, 1, 3);
            renderer.setLookAt(Cscreen + Nscreen * l + fVec3(0, 0, camera_height), Cscreen + fVec3(0, 0, camera_look_height), { 0, 0, 1 });
            renderer.setLightDirection(-Cscreen - Nscreen * l - fVec3(0, 0, camera_height));
            }
        else if (phase == -3)
            {
            const float l = camera_distance - lerp(em, 0, ms_dive, 0, 25, 2, 2);
            renderer.setLookAt(Cscreen + Nscreen * l + fVec3(0, 0, camera_height), Cscreen + fVec3(0, 0, camera_look_height), { 0, 0, 1 });
            }
        else
            {
            renderer.setLookAt(Cscreen + Nscreen * camera_distance + fVec3(0, 0, camera_height), Cscreen + fVec3(0, 0, camera_look_height), { 0, 0, 1 });
            }

        fMat4 M;
        if (phase >= 0)
            {
            M.setRotate(modelRotStart, { 0, 0, 1 });
            if (solvingSub(M)) { em = 0; phase = -2; }
            return 0;
            }
        if (phase == -1)
            {
            M.setRotate(modelRotStart, { 0, 0, 1 });
            float cradleRot = 0;
            if (ema < ms_before_stop_close) cradleRot = -90.0f * lerp(ema, 0, ms_before_stop_close, 70, 0, 1, 3.5f);
            else if (ema < ms_before_wirecube_appear) cradleRot = 0;
            else if (ema < ms_before_cubelets_end) cradleRot = lerp(ema, ms_before_wirecube_appear, ms_before_cubelets_end, 0, 360, 1.5f, 1.5f);
            drawSARCASM3D(cradleRot, GRIPPER_OPEN_DEG, TUMBLE_PARKED_DEG, M);
            fMat4 MC;
            MC.setScale(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f);
            MC.multRotate(90, { 1, 0, 0 });
            MC.multRotate(cradleRot + 90, { 0, 0, 1 });
            if (ema > ms_before_stop_close)
                {
                const float rr = lerp(ema, ms_before_cubelets_appear, ms_before_cubelets_end, 0, 1);
                cube3D.drawCubePartial(cubePositionInCradle(0, M) * MC, rr, renderer, true);
                }
            if (em >= ms_before_start_solve) { phase = 0; em = 0; }
            return 0;
            }
        const float cradleRot = lerp(ema, 0, ms_before_dive + ms_dive, 0, 10 * 90, 2, 2);
        if (phase == -2)
            {
            M.setRotate(modelRotStart, { 0, 0, 1 });
            drawOpenWithCube(M, cradleRot);
            if (em >= ms_before_dive) { phase = -3; em = 0; }
            return 0;
            }
        if (phase == -3)
            {
            const float modelRot = lerp(em, 0, ms_dive - 1000, modelRotStart, 0, 2, 1);
            M.setRotate(modelRot, { 0, 0, 1 });
            drawOpenWithCube(M, cradleRot);
            return (em >= ms_end) ? 2 : 0;
            }
        return 2;
        }

    FLASHMEM void initDreaming(const Image<RGB565>& dst)
        {
        setupRenderer(dst);
        em = 0;
        }

    FLASHMEM float dreamingRatioDone()
        {
        return ((float)em) / 26000.0f;
        }

    FLASHMEM int dreamingStep()
        {
        const float t = (uint32_t)em;
        const float startModelRot = 4000.0f;
        const float endModelRot = 22000.0f;
        const float startDive = 20000.0f;
        const float endDive = 26000.0f;
        const float camera_height = 120.0f;
        const float camera_look_height = 80.0f;
        const float camera_distance = 200.0f;
        renderer.setLightDiffuse(RGBf(0.9f + 0.3f * sinf(t * 0.001f), 0.9f + 0.2f * sinf(t * 0.0013f), 1.0f));
        const float l = lerp(t, startDive, endDive, camera_distance, 15.0f, 2, 1);
        const float he = lerp(t, startDive, endDive, camera_height, 0.0f, 2, 2);
        const float hd = lerp(t, startDive, endDive, camera_look_height, 0.0f, 2, 2);
        renderer.setLookAt(Cscreen + Nscreen * l + fVec3(0, 0, he), Cscreen + fVec3(0, 0, hd), { 0, 0, 1 });

        fMat4 M;
        M.setRotate(lerp(t, startModelRot, endModelRot, 350.0f, 0.0f, 2, 2), { 0, 0, 1 });
        const float cradleRot = lerp(t, 0, endDive, 0, 5 * 360, 1, 1);
        drawSARCASM3D(cradleRot, GRIPPER_OPEN_DEG, TUMBLE_PARKED_DEG, M);

        renderer.setShaders(SHADER_GOURAUD | SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_CLAMP);
        renderer.setMaterial(RGBf(1.0f, 1.0f, 0.0f), 0.2f, 0.7f, 0.5f, 8);
        fMat4 S;
        S.setRotate(0, { 0, 0, 1 });
        S.multScale({ 27, 27, 27 });
        S.multTranslate({ 0, 160, 0 });
        renderer.setModelMatrix(M * S);
        renderer.drawMesh(&companion_cube_1, false);
        S.setRotate(0, { 0, 0, 1 });
        S.multScale({ 27, 27, 27 });
        S.multTranslate({ -30, 150, 54 });
        renderer.setModelMatrix(M * S);
        renderer.drawMesh(&companion_cube_1, false);
        S.setRotate(0, { 0, 0, 1 });
        S.multScale({ 27, 27, 27 });
        S.multTranslate({ -65, 140, 0 });
        renderer.setModelMatrix(M * S);
        renderer.drawMesh(&companion_cube_1, false);
        renderer.setModelMatrix(cubePositionInCradle(cradleRot, M));
        renderer.drawMesh(&companion_cube_1, false);

        if (t >= endDive) return 2;
        if (t > startDive) return 1;
        return 0;
        }
    }

FLASHMEM void captureRenderedFrameForFaceTransition()
    {
    memcpy(alt_fb, im.data(), SLX * SLY * sizeof(uint16_t));
    DemoFaceAction::useImageInsteadOfFace(transitionSourceImage());
    }

FLASHMEM void drawCubeFrame(const fMat4& M, int activity, const char* state)
    {
    im.clear(RGB565_Black);
    renderer.setViewportSize(SLX, SLY);
    renderer.setOffset(0, 0);
    renderer.setImage(&im);
    renderer.clearZbuffer();
    cube3D.drawCube(M, renderer);
    captureRenderedFrameForFaceTransition();
    DemoFaceAction::drawFace(im);
    drawHeaderAndConsole(activity, state);
    }

FLASHMEM void drawCubeSliceFrame(const fMat4& M, int axe, int slice, float angle, int activity, const char* state)
    {
    im.clear(RGB565_Black);
    renderer.setViewportSize(SLX, SLY);
    renderer.setOffset(0, 0);
    renderer.setImage(&im);
    renderer.clearZbuffer();
    cube3D.drawCube_rotateSlice(M, renderer, axe, slice, angle);
    captureRenderedFrameForFaceTransition();
    DemoFaceAction::drawFace(im);
    drawHeaderAndConsole(activity, state);
    }

FLASHMEM void drawCubePartialFrame(const fMat4& M, float ratio, bool frame, int activity, const char* state)
    {
    im.clear(RGB565_Black);
    renderer.setViewportSize(SLX, SLY);
    renderer.setOffset(0, 0);
    renderer.setImage(&im);
    renderer.clearZbuffer();
    cube3D.drawCubePartial(M, ratio, renderer, frame);
    captureRenderedFrameForFaceTransition();
    DemoFaceAction::drawFace(im);
    drawHeaderAndConsole(activity, state);
    }

FLASHMEM void idleDelay(uint32_t ms, const char* state = nullptr)
    {
    uint32_t end = millis() + ms;
    while ((int32_t)(millis() - end) < 0)
        {
        drawFaceFrame(-1, state);
        updateScreen();
        delay(16);
        }
    }

#if SARCASM_DEMO_ENABLE_VOICE
void espeakDelay(uint32_t ms)
    {
    uint32_t end = millis() + ms;
    while ((int32_t)(millis() - end) < 0)
        {
        drawFaceFrame(2 + (millis() / 110) % 6, "VOICE");
        updateScreen();
        delay(1);
        }
    }
#endif

class DemoConsolePrint : public Print
    {
    public:
        size_t write(uint8_t c) override
            {
            consolePutChar((char)c);
            talking_until_ms = millis() + 240;
            return 1;
            }
    };

static DemoConsolePrint console_print;

FLASHMEM void typeLine(const char* text, RGB565 color = COLOR_TEXT, uint16_t charDelayMs = 60, bool animateMouth = false)
    {
    char buf[28];
    size_t len = strlen(text);
    if (len >= sizeof(buf)) len = sizeof(buf) - 1;
    consolePush("", color);
    const bool old_voice = voice_active;
    if (animateMouth) voice_active = true;
    for (size_t i = 0; i <= len; ++i)
        {
        memcpy(buf, text, i);
        buf[i] = 0;
        consoleSetLast(buf, color);
        if (animateMouth) talking_until_ms = millis() + charDelayMs + 180;
        drawFaceFrame(1 + (i % 7), animateMouth ? "VOICE" : "CMD");
        updateScreen();
        delay(charDelayMs);
        }
    voice_active = old_voice;
    }

FLASHMEM void speakNow(const char* text)
    {
    Serial.print("SARCASM says: ");
    Serial.println(text);
    talking_until_ms = millis() + 1400;
#if SARCASM_DEMO_ENABLE_VOICE
    voice_active = true;
    if (voice_ready)
        {
        consolePush("", COLOR_TEXT);
        console_current_color = COLOR_TEXT;
        espeak.play(text, &console_print);
        if (console_cursor_x > 0) consoleNewLine();
        drawFaceFrame(-1, "VOICE");
        updateScreen();
        }
    else
        {
        typeLine(text, COLOR_TEXT, 70, true);
        idleDelay(400, "VOICE OFF");
        }
    voice_active = false;
#else
    typeLine(text, COLOR_TEXT, 70, true);
    idleDelay(400, "VOICE");
#endif
    }

FLASHMEM void makeFakeSolution(uint8_t* moveAxe, int8_t* moveSlice, int8_t* moveDir, uint8_t* moveHalf, int& count)
    {
    cube3D.resetPosition();
    count = 12;
    int prevAxe = -1;
    int prevSlice = -4;
    for (int i = 0; i < count; ++i)
        {
        int axe;
        int slice;
        do
            {
            axe = rngInt(3);
            slice = rngInt(3) - 1;
            }
        while (axe == prevAxe && slice == prevSlice);
        prevAxe = axe;
        prevSlice = slice;
        int dir = rngInt(2) ? 1 : -1;
        int half = rngInt(5) == 0 ? 1 : 0;
        cube3D.rotateSlice(90.0f * dir * (half + 1), axe, slice);
        const int dst = count - 1 - i;
        moveAxe[dst] = (uint8_t)axe;
        moveSlice[dst] = (int8_t)slice;
        moveDir[dst] = (int8_t)-dir;
        moveHalf[dst] = (uint8_t)half;
        }
    }

FLASHMEM void playIntro()
    {
    Serial.println("SARCASM demo: intro");
    DemoFaceAction::appear_intro(1.0f);
    Image<RGB565> intro_rubik_cache((RGB565*)zbuf, SLX, SLY);
    intro_rubik_cache.blit(intro_rubik, { 0, 0 });
    uint32_t start = millis();
    while (millis() - start < 3800)
        {
        drawIntroFrame(millis() - start);
        updateScreen();
        }
    waitUpdate();

    Image<RGB565> final_ui((RGB565*)zbuf, SLX, SLY);
    drawFinalUIFrameTo(final_ui);
    im.blit(final_ui, { 0, 0 });
    start = millis();
    while (millis() - start < 300)
        {
        const int a = (int)((millis() - start) * SLX / (2 * 300));
        updateScreenRegion({ max(SLX / 2 - a, 0), min(SLX / 2 + a, SLX - 1), 0, 18 }, true);
        }
    updateScreenRegion({ 0, SLX - 1, 0, 18 }, false);

    start = millis();
    while (millis() - start < 300)
        {
        const int a = (int)((millis() - start) * SLX / (2 * 300));
        updateScreenRegion({ max(SLX / 2 - a, 0), min(SLX / 2 + a, SLX - 1), 16, 24 }, false);
        updateScreenRegion({ max(SLX / 2 - a, 0), min(SLX / 2 + a, SLX - 1), 233, 239 }, true);
        }

    start = millis();
    while (millis() - start < 300)
        {
        const int a = (int)((millis() - start) * 110 / 300);
        updateScreenRegion({ 0, 6, max(130 - a, 0), min(130 + a, SLY - 1) }, false);
        updateScreenRegion({ 313, 319, max(130 - a, 0), min(130 + a, SLY - 1) }, true);
        }
    updateScreen(true);
    waitUpdate();

    consoleClear();
    typeLine("BOOTING SARCASM...", COLOR_TAG, 70);
    typeLine("graphics: TGX online", COLOR_TEXT, 58);
    typeLine("voice: optional I2S", COLOR_TEXT, 58);
    typeLine("motors: politely absent", COLOR_TEXT, 58);
    speakNow("SARCASM online. I am awake, against my better judgement.");
    }

FLASHMEM void fakeSolve()
    {
    Serial.println("SARCASM demo: fake solve");
    consoleClear();
    typeLine("[demo mode]", COLOR_TAG, 75);
    typeLine("cube detected", COLOR_TEXT, 62);
    typeLine("estimating human error", COLOR_TEXT, 62);
    speakNow("Cube detected. Estimating the amount of human contribution required.");

    speakNow("Solving. Please remain impressed.");

    static const char solve_seq[] = "TvTTvVTvTTvVTvTTvVTaTaVTaTaRTaTaVTaVTrTTrTa";
    DemoFaceAction::disapear(1500);
    idleDelay(1500, nullptr);

    Image<RGB565> alt = transitionSourceImage();
    alt.clear(RGB565_Black);
    renderer.clearZbuffer();
    MachineAnimation::initSolving(alt, solve_seq);
    DemoFaceAction::cubeAppear(alt, 3000);

    const float nbmoves = max(1.0f, (float)MachineAnimation::totalMoves());
    bool fading = false;
    while (true)
        {
        alt.clear(RGB565_Black);
        renderer.clearZbuffer();
        const int done = MachineAnimation::solvingStep();
        DemoFaceAction::useImageInsteadOfFace(alt);
        const float progress = (nbmoves - MachineAnimation::remainingMoves()) / nbmoves;
        im.clear(RGB565_Black);
        DemoFaceAction::drawFace(im);
        drawHeaderAndConsole((int)(8.0f * progress), nullptr);
        drawConsoleSpinner();
        updateScreen();
        if (done == 2) break;
        if (done == 1 && !fading)
            {
            fading = true;
            consoleSetLast("almost solved", COLOR_TAG);
            }
        delay(1);
        }

    consoleSetLast("SOLVED", COLOR_DONE);
    speakNow("Solved. I accept your applause as statistically inevitable.");
    DemoFaceAction::cubeAppear(alt, 600, true);
    uint32_t end = millis() + 4000;
    while ((int32_t)(millis() - end) < 0)
        {
        alt.clear(RGB565_Black);
        renderer.clearZbuffer();
        MachineAnimation::solvingStep();
        DemoFaceAction::useImageInsteadOfFace(alt);
        im.clear(RGB565_Black);
        DemoFaceAction::drawFace(im);
        drawHeaderAndConsole(8, nullptr);
        updateScreen();
        delay(1);
        }
    }

FLASHMEM void replayAnimation()
    {
    consoleClear();
    typeLine("[solve replay]", COLOR_TAG, 60);
    speakNow("Replaying a previous solution. The dramatic tension is simulated.");

    static const char replay_seq[] = "TvTTvVTvTTvVTvTTvVTaTaVTaTaRTaTaVTaVTrTTrTa";
    Image<RGB565> alt = transitionSourceImage();
    alt.clear(RGB565_Black);
    renderer.clearZbuffer();
    MachineAnimation::initSolving(alt, replay_seq);
    DemoFaceAction::cubeAppear(transitionSourceImage(), 3000, false, true);

    const float nbmoves = max(1.0f, (float)MachineAnimation::totalMoves());
    bool fading = false;
    while (true)
        {
        alt.clear(RGB565_Black);
        renderer.clearZbuffer();
        const int done = MachineAnimation::solvingStep();
        DemoFaceAction::useImageInsteadOfFace(alt);
        const float progress = (nbmoves - MachineAnimation::remainingMoves()) / nbmoves;
        im.clear(RGB565_Black);
        DemoFaceAction::drawFace(im);
        drawHeaderAndConsole((int)(8.0f * progress), nullptr);
        drawConsoleSpinner();
        updateScreen();
        if (done == 2) break;
        if (done == 1 && !fading)
            {
            fading = true;
            consoleSetLast("DONE", COLOR_DONE);
            }
        delay(1);
        }

    consoleSetCursor(21, 0);
    consolePutChar('D');
    consolePutChar('O');
    consolePutChar('N');
    consolePutChar('E');
    DemoFaceAction::cubeDisappear(2000);
    uint32_t end = millis() + 2000;
    while ((int32_t)(millis() - end) < 0)
        {
        alt.clear(RGB565_Black);
        renderer.clearZbuffer();
        MachineAnimation::solvingStep();
        DemoFaceAction::useImageInsteadOfFace(alt);
        im.clear(RGB565_Black);
        DemoFaceAction::drawFace(im);
        drawHeaderAndConsole(8, nullptr);
        updateScreen();
        delay(1);
        }
    }

FLASHMEM void dreamingAnimation()
    {
    consoleClear();
    typeLine("[idle thoughts]", COLOR_TAG, 60);
    speakNow("Entering low power existential dread.");

    Image<RGB565> alt = transitionSourceImage();
    alt.clear(RGB565_Black);
    renderer.clearZbuffer();
    MachineAnimation::initDreaming(alt);
    DemoFaceAction::dreamingAppear(transitionSourceImage(), 3000);
    bool lightup = false;
    while (true)
        {
        alt.clear(RGB565_Black);
        renderer.clearZbuffer();
        const int done = MachineAnimation::dreamingStep();
        DemoFaceAction::useImageInsteadOfFace(alt);
        if (done == 1 && !lightup)
            {
            lightup = true;
            DemoFaceAction::dreamingEnd(alt, 4000);
            }
        im.clear(RGB565_Black);
        DemoFaceAction::drawFace(im);
        drawHeaderAndConsole((int)(8.0f * MachineAnimation::dreamingRatioDone()), nullptr);
        drawConsoleSpinner();
        updateScreen();
        if (done == 2) break;
        delay(1);
        }

    const char* endmsg = "DONE";
    consoleSetCursor(25 - (int)strlen(endmsg), 0);
    console_current_color = COLOR_DONE;
    for (int i = 0; endmsg[i]; ++i) consolePutChar(endmsg[i]);
    DemoFaceAction::dreamingDisappear(900);
    uint32_t end = millis() + 1000;
    while ((int32_t)(millis() - end) < 0)
        {
        alt.clear(RGB565_Black);
        renderer.clearZbuffer();
        MachineAnimation::dreamingStep();
        DemoFaceAction::useImageInsteadOfFace(alt);
        im.clear(RGB565_Black);
        DemoFaceAction::drawFace(im);
        drawHeaderAndConsole(8, nullptr);
        updateScreen();
        delay(1);
        }
    }

FLASHMEM void dreamLoop()
    {
    consoleClear();
    typeLine("[idle thoughts]", COLOR_TAG, 75);
    const char* lines[] =
        {
        "I can solve cubes.",
        "I cannot solve humans.",
        "fair trade, really.",
        };
    for (int i = 0; i < 3; ++i)
        {
        typeLine(lines[i], COLOR_TEXT, 62);
        speakNow(lines[i]);
        idleDelay(600, "IDLE");
        }
    }

FLASHMEM void initVoice()
    {
#if SARCASM_DEMO_ENABLE_VOICE
    if (external_psram_size == 0)
        {
        Serial.println("SARCASM demo: no external PSRAM, espeak voice disabled");
        voice_ready = false;
        return;
        }
    AudioMemory(12);
    if (SARCASM_DEMO_AUDIO_PWR_PIN != 255)
        {
        pinMode(SARCASM_DEMO_AUDIO_PWR_PIN, OUTPUT);
        digitalWrite(SARCASM_DEMO_AUDIO_PWR_PIN, HIGH);
        }
    espeak.begin(1);
    espeak.setDelayFunction(espeakDelay);
    espeak.registerVariant("sarcasm_voice", espeak_ng_data_sarcasm_voice, espeak_ng_data_sarcasm_voice_len);
    espeak.setRate(120);
    espeak.setVoice("en+sarcasm_voice");
    voiceAmp.gain(0.7f);
    voice_ready = true;
    Serial.println("SARCASM demo: espeak voice enabled");
#endif
    }

FLASHMEM void setup()
    {
    Serial.begin(9600);
    delay(300);
    Serial.println("SARCASM TGX demo boot");

    while (!tft.begin(SPI_SPEED));
    if (PIN_BACKLIGHT != 255)
        {
        pinMode(PIN_BACKLIGHT, OUTPUT);
        digitalWrite(PIN_BACKLIGHT, HIGH);
        }

    tft.setRotation(3);
    tft.setFramebuffer(internal_fb);
    tft.setDiffBuffers(&diff1, &diff2);
    tft.setDiffGap(4);
    tft.setRefreshRate(120);
    tft.setVSyncSpacing(1);

    randomSeed(analogRead(A0) ^ micros());
    rng_state ^= micros();

    initRenderer();
    initCube();
    initVoice();
    updateScreen(true);
    }

void loop()
    {
    playIntro();
    fakeSolve();
    replayAnimation();
    dreamingAnimation();
    }
