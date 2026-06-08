/********************************************************************
*
* TGX + LVGL + ILI9341_T4 mesh viewer for Teensy 4.1.
*
* tested with LVGL 9.5 and an ILI9341 display.
*
* LVGL provides the widgets and touch dispatch.
* TGX renders the 3D viewport into a memory framebuffer.
* ILI9341_T4 uploads LVGL-rendered regions to the ILI9341 display.
*
* Notes:
* - This sketch is RAM heavy: if the default Teensy build does not fit, compile it with
*   "Smallest Code with LTO".
*
* - If more RAM is needed, reducing LVGL's pool in lv_conf.h to
*   "#define LV_MEM_SIZE (32 * 1024U)" is usually enough for this UI.
*
********************************************************************/

#include <Arduino.h>
#include <ILI9341_T4.h>
#include <lvgl.h>
#include <tgx.h>

using namespace tgx;

// Meshes and textures are stored under 3Dmodels/. The fallback keeps the
// sketch compatible with Arduino builders that flatten sketch subfolders.
#if __has_include("teapot.h")
    #include "teapot.h"
    #include "bunny.h"
    #include "bob.h"
    #include "blub.h"
    #include "falcon_vs_v2.h"
    #include "donkeykong_small.h"
    #include "cyborg_v2.h"
    #include "spot.h"
#else
    #include "3Dmodels/teapot/teapot.h"
    #include "3Dmodels/bunny/bunny.h"
    #include "3Dmodels/bob/bob.h"
    #include "3Dmodels/blub/blub.h"
    #include "3Dmodels/falcon/falcon_vs_v2.h"
    #include "3Dmodels/donkeykong/donkeykong_small.h"
    #include "3Dmodels/cyborg/cyborg_v2.h"
    #include "3Dmodels/spot/spot.h"
#endif




// Set to 0 to skip interactive calibration and use the constants below.
#define TGX_LVGL_MESHVIEWER_RUN_TOUCH_CALIBRATION 1

// These constants are for a specific ILI9341 + XPT2046 shield. If you run the calibration above, 
// update these values with the printed output to get accurate touch mapping without needing to run calibration
#define TOUCH_CALIBRATION_CONSTANTS { 322,3775,3905, 412 }

#define PIN_SCK       13
#define PIN_MISO      12
#define PIN_MOSI      11
#define PIN_DC        10
#define PIN_CS        9
#define PIN_RESET     6
#define PIN_BACKLIGHT 255
#define PIN_TOUCH_CS  4
#define PIN_TOUCH_IRQ 3

#define SPI_SPEED 40000000

static const int SCREEN_W = 320;
static const int SCREEN_H = 240;
static const int LVGL_BUF_LINES = 40;

static const int VIEW_W = 240;
static const int VIEW_H = 160;

// ILI9341_T4 keeps its own full-screen framebuffer and two diff buffers.
// They are in DMAMEM because they are large and do not need fast DTCM access.
DMAMEM ILI9341_T4::DiffBuffStatic<8000> diff1;
DMAMEM ILI9341_T4::DiffBuffStatic<8000> diff2;
DMAMEM uint16_t internal_fb[SCREEN_W * SCREEN_H];

// LVGL renders widgets into this partial draw buffer. The flush callback below
// copies these LVGL pixels to the ILI9341_T4 driver; TGX is not rendered there.
DMAMEM lv_color_t lvgl_buf[SCREEN_W * LVGL_BUF_LINES];

// TGX renders only the 3D viewport into this smaller RGB565 image. LVGL then
// displays the same memory as a canvas object, avoiding an extra copy.
DMAMEM uint16_t viewport_fb[VIEW_W * VIEW_H];
DMAMEM uint16_t zbuf[VIEW_W * VIEW_H];
Image<RGB565> viewport_image(viewport_fb, VIEW_W, VIEW_H);

// The renderer is compiled with all shader variants this UI can select.
static const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT | SHADER_GOURAUD | SHADER_NOTEXTURE | SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_WRAP_POW2;
Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;

// One cache buffer is reused for the currently selected mesh. Large meshes may
// keep some data in PROGMEM if the full converted representation does not fit.
static const size_t MESH_CACHE_SIZE = 96000;
DMAMEM char mesh_cache[MESH_CACHE_SIZE];

// Per-mesh display data used by the dropdown and by renderViewport().
struct MeshChoice
    {
    const char* label;
    const Mesh3Dv2<RGB565>* mesh;
    int triangles;
    uint32_t mesh_bytes;
    float scale;
    float y_offset;
    bool textured;
    };

static const MeshChoice mesh_choices[] =
    {
    { "Teapot",  &teapot,         2256,  21680, 15.0f, 0.0f, false },
    { "Bunny",   &bunny,          4968,  58484, 12.0f, 0.0f, false },
    { "Bob tex", &bob,           10688, 144972, 15.0f, 0.0f, true },
    { "Blub tex", &blub,         14208, 185860, 15.0f, 0.0f, true },
    { "Falcon",  &falcon_vs_v2,   6684,  90104, 14.0f, 0.0f, true },
    { "Donkey",  &donkeykong_small, 3200, 50048, 14.0f, 0.0f, true },
    { "Cyborg",  &cyborg_v2,      5549,  73824, 14.0f, 0.0f, true },
    { "Spot",    &spot,           5856,  77576, 14.0f, 0.0f, true },
    };

static const int MESH_COUNT = sizeof(mesh_choices) / sizeof(mesh_choices[0]);

enum RenderMode
    {
    RENDER_WIRE = 0,
    RENDER_FLAT,
    RENDER_SMOOTH,
    };

ILI9341_T4::ILI9341Driver tft(PIN_CS, PIN_DC, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_RESET, PIN_TOUCH_CS, PIN_TOUCH_IRQ);

lv_display_t* disp = nullptr;
lv_indev_t* indev = nullptr;
lv_obj_t* viewport_canvas = nullptr;
lv_obj_t* stats_label = nullptr;
lv_obj_t* mesh_info_label = nullptr;
lv_obj_t* mesh_dropdown = nullptr;
lv_obj_t* render_mode_dropdown = nullptr;
lv_obj_t* zoom_slider = nullptr;
lv_obj_t* auto_switch = nullptr;

// Current mesh/render state. cached_mesh points either to mesh_cache data or to
// a mixed cached/PROGMEM representation returned by tgx::cacheMesh().
const Mesh3Dv2<RGB565>* cached_mesh = nullptr;
int selected_mesh = 0;
int render_mode = RENDER_SMOOTH;
size_t mesh_cache_used = 0;

// model_orientation is updated by touch drags and by the auto-rotation mode.
fMat4 model_orientation;
int zoom_percent = 100;
bool auto_rotate = true;
bool render_requested = true;

lv_point_t last_drag_point = { 0, 0 };
bool dragging_viewport = false;

uint32_t last_render_ms = 0;
uint32_t last_stats_ms = 0;
uint32_t frames_since_stats = 0;
uint32_t last_render_us = 0;
float last_fps = 0.0f;

void resetModelOrientation()
    {
    model_orientation.setIdentity();
    model_orientation.multRotate(-12.0f, { 1, 0, 0 });
    model_orientation.multRotate(25.0f, { 0, 1, 0 });
    }

static uint32_t lvgl_tick(void)
    {
    return millis();
    }

// LVGL calls this when it has rendered a dirty region into lvgl_buf. The driver
// handles the physical upload; the TGX viewport has already been drawn earlier.
void lvgl_flush(lv_display_t* display, const lv_area_t* area, uint8_t* px_map)
    {
    const bool redraw_now = lv_disp_flush_is_last(display);
    tft.updateRegion(redraw_now, (uint16_t*)px_map, area->x1, area->x2, area->y1, area->y2);
    lv_disp_flush_ready(display);
    }

// Convert ILI9341_T4 touch coordinates into LVGL pointer events.
void lvgl_touch_read(lv_indev_t* input, lv_indev_data_t* data)
    {
    (void)input;

    int touch_x, touch_y, touch_z;
    const bool touched = tft.readTouch(touch_x, touch_y, touch_z);
    if (!touched)
        {
        data->state = LV_INDEV_STATE_REL;
        return;
        }

    data->state = LV_INDEV_STATE_PR;
    data->point.x = touch_x;
    data->point.y = touch_y;
    }

void markRenderNeeded()
    {
    render_requested = true;
    }

float clampFloat(float value, float lo, float hi)
    {
    if (value < lo) return lo;
    if (value > hi) return hi;
    return value;
    }

void cacheSelectedMesh()
    {
    mesh_cache_used = 0;
    const MeshChoice& choice = mesh_choices[selected_mesh];

    // "MLP" asks TGX to cache meshlet/material/light-relevant data while still
    // allowing very large assets to leave overflow data in PROGMEM.
    cached_mesh = tgx::cacheMesh(choice.mesh, mesh_cache, MESH_CACHE_SIZE, "MLP", &mesh_cache_used);

    Serial.print("Selected mesh: ");
    Serial.print(choice.label);
    Serial.print(" (");
    Serial.print(choice.triangles);
    Serial.print(" triangles), cache used: ");
    Serial.print(mesh_cache_used);
    Serial.print(" / ");
    Serial.print(MESH_CACHE_SIZE);
    Serial.print(", mesh bytes: ");
    Serial.println(choice.mesh_bytes);

    updateMeshInfoLabel();
    }

void updateMeshInfoLabel()
    {
    if (mesh_info_label == nullptr) return;

    const MeshChoice& choice = mesh_choices[selected_mesh];
    char buf[64];
    snprintf(buf, sizeof(buf), "%d tris %luKB",
             choice.triangles,
             (unsigned long)((choice.mesh_bytes + 1023) / 1024));
    lv_label_set_text(mesh_info_label, buf);
    }

void updateStatsLabel()
    {
    if (stats_label == nullptr) return;

    char buf[64];
    snprintf(buf, sizeof(buf), "%.1f FPS", (double)last_fps);
    lv_label_set_text(stats_label, buf);
    }

uint16_t makeRGB565(uint8_t r, uint8_t g, uint8_t b)
    {
    return RGB565(r / 255.0f, g / 255.0f, b / 255.0f).val;
    }

void fillViewportBackground()
    {
    // The viewport background is drawn by TGX, independently of the LVGL theme.
    const uint16_t gray = makeRGB565(226, 228, 232);
    for (int y = 0; y < VIEW_H; y++)
        {
        uint16_t* row = viewport_fb + y * VIEW_W;
        for (int x = 0; x < VIEW_W; x++) row[x] = gray;
        }
    }

void renderViewport()
    {
    elapsedMicros render_timer = 0;

    fillViewportBackground();
    renderer.clearZbuffer();

    const MeshChoice& choice = mesh_choices[selected_mesh];
    const float scale = choice.scale * (zoom_percent / 100.0f);

    fMat4 model_scale;
    model_scale.setScale(scale, scale, scale);
    fMat4 model = model_orientation * model_scale;
    model.multTranslate({ 0, choice.y_offset, -36 });
    renderer.setModelMatrix(model);

    if (cached_mesh != nullptr)
        {
        if (render_mode == RENDER_WIRE)
            {
            // Dark wireframe lines remain readable on the light viewport.
            renderer.setMaterial(RGBf(0.16f, 0.17f, 0.18f), 0.15f, 0.70f, 1.0f, 96);
            renderer.drawWireFrameMeshAA(cached_mesh);
            }
        else
            {
            const Shader shading = (render_mode == RENDER_FLAT) ? SHADER_FLAT : SHADER_GOURAUD;
            if (choice.textured)
                {
                renderer.setShaders(shading | SHADER_TEXTURE_BILINEAR);
                renderer.setMaterial(RGBf(1.0f, 1.0f, 1.0f), 0.12f, 0.68f, 1.0f, 96);
                }
            else
                {
                renderer.setShaders(shading | SHADER_NOTEXTURE);
                renderer.setMaterial(RGBf(1.0f, 0.66f, 0.16f), 0.15f, 0.70f, 1.0f, 96);
                }

            renderer.drawMesh(cached_mesh, false);
            }
        }

    last_render_us = render_timer;
    frames_since_stats++;

    const uint32_t now = millis();
    if (now - last_stats_ms >= 1000)
        {
        last_fps = (frames_since_stats * 1000.0f) / (now - last_stats_ms);
        frames_since_stats = 0;
        last_stats_ms = now;
        updateStatsLabel();

        Serial.print("Render: ");
        Serial.print(last_render_us);
        Serial.print(" us, FPS: ");
        Serial.println(last_fps, 1);
        }

    // Mark the LVGL canvas dirty. LVGL will later call lvgl_flush() with the
    // affected screen region.
    lv_obj_invalidate(viewport_canvas);
    render_requested = false;
    }

// Dragging inside the canvas rotates the TGX model. The UI state itself is
// still managed by LVGL; this callback only updates our render state.
void viewportEvent(lv_event_t* e)
    {
    const lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t* active_indev = lv_event_get_indev(e);
    if (active_indev == nullptr) active_indev = lv_indev_active();

    if (code == LV_EVENT_PRESSED)
        {
        dragging_viewport = true;
        if (active_indev != nullptr) lv_indev_get_point(active_indev, &last_drag_point);
        auto_rotate = false;
        lv_obj_remove_state(auto_switch, LV_STATE_CHECKED);
        }
    else if (code == LV_EVENT_PRESSING && dragging_viewport && active_indev != nullptr)
        {
        lv_point_t point;
        lv_indev_get_point(active_indev, &point);
        const int dx = point.x - last_drag_point.x;
        const int dy = point.y - last_drag_point.y;
        last_drag_point = point;

        model_orientation.multRotate(dy * 0.975f, { 1, 0, 0 });
        model_orientation.multRotate(dx * 0.975f, { 0, 1, 0 });
        markRenderNeeded();
        }
    else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST)
        {
        dragging_viewport = false;
        }
    }

void meshDropdownEvent(lv_event_t* e)
    {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;

    selected_mesh = lv_dropdown_get_selected(mesh_dropdown);
    if (selected_mesh < 0 || selected_mesh >= MESH_COUNT) selected_mesh = 0;
    cacheSelectedMesh();
    markRenderNeeded();
    }

void renderModeDropdownEvent(lv_event_t* e)
    {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;

    render_mode = lv_dropdown_get_selected(render_mode_dropdown);
    if (render_mode < RENDER_WIRE || render_mode > RENDER_SMOOTH) render_mode = RENDER_SMOOTH;
    markRenderNeeded();
    }

void zoomSliderEvent(lv_event_t* e)
    {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;

    zoom_percent = lv_slider_get_value(zoom_slider);
    markRenderNeeded();
    }

void autoRotateEvent(lv_event_t* e)
    {
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;

    auto_rotate = lv_obj_has_state(auto_switch, LV_STATE_CHECKED);
    markRenderNeeded();
    }

void resetButtonEvent(lv_event_t* e)
    {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    resetModelOrientation();
    zoom_percent = 100;
    auto_rotate = true;
    lv_slider_set_value(zoom_slider, zoom_percent, LV_ANIM_OFF);
    lv_obj_add_state(auto_switch, LV_STATE_CHECKED);
    markRenderNeeded();
    }

lv_obj_t* makeLabel(lv_obj_t* parent, const char* text, int x, int y)
    {
    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x102a43), 0);
    return label;
    }

void styleDropdownButton(lv_obj_t* dropdown)
    {
    lv_obj_set_style_bg_color(dropdown, lv_color_hex(0xfff16a), 0);
    lv_obj_set_style_bg_opa(dropdown, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(dropdown, lv_color_hex(0x102a43), 0);
    lv_obj_set_style_border_color(dropdown, lv_color_hex(0xff4f7b), 0);
    lv_obj_set_style_border_width(dropdown, 1, 0);
    lv_obj_set_style_radius(dropdown, 2, 0);
    }

void styleCompactDropdownButton(lv_obj_t* dropdown)
    {
    styleDropdownButton(dropdown);
    lv_obj_set_style_text_font(dropdown, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_pad_left(dropdown, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_right(dropdown, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_top(dropdown, 3, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(dropdown, 3, LV_PART_MAIN);
    }

void styleDropdownList(lv_obj_t* dropdown)
    {
    lv_obj_t* list = lv_dropdown_get_list(dropdown);
    if (list == nullptr) return;

    lv_obj_set_style_bg_color(list, lv_color_hex(0xfff16a), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(list, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(list, lv_color_hex(0x102a43), LV_PART_MAIN);
    lv_obj_set_style_border_color(list, lv_color_hex(0xff4f7b), LV_PART_MAIN);
    lv_obj_set_style_border_width(list, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(list, 2, LV_PART_MAIN);
    lv_obj_set_style_bg_color(list, lv_color_hex(0xfff16a), LV_PART_SELECTED);
    lv_obj_set_style_text_color(list, lv_color_hex(0x102a43), LV_PART_SELECTED);
    lv_obj_set_style_bg_color(list, lv_color_hex(0xfff16a), LV_PART_SELECTED | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(list, lv_color_hex(0x102a43), LV_PART_SELECTED | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(list, lv_color_hex(0xfff16a), LV_PART_SELECTED | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(list, lv_color_hex(0x102a43), LV_PART_SELECTED | LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(list, lv_color_hex(0xfff16a), LV_PART_SELECTED | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(list, lv_color_hex(0x102a43), LV_PART_SELECTED | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_set_style_text_font(list, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_font(list, &lv_font_montserrat_12, LV_PART_SELECTED);
    }

void dropdownReadyEvent(lv_event_t* e)
    {
    if (lv_event_get_code(e) != LV_EVENT_READY) return;

    lv_obj_t* dropdown = (lv_obj_t*)lv_event_get_target(e);
    styleDropdownList(dropdown);
    }

void createUI()
    {
    lv_obj_t* screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x00a9a5), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    lv_obj_t* title_shadow = makeLabel(screen, "TGX + LVGL Mesh Viewer", 7, 5);
    lv_obj_set_style_text_font(title_shadow, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title_shadow, lv_color_hex(0x102a43), 0);

    lv_obj_t* title = makeLabel(screen, "TGX + LVGL Mesh Viewer", 6, 5);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x102a43), 0);

    stats_label = makeLabel(screen, "0.0 FPS", 230, 6);
    lv_obj_set_width(stats_label, 86);
    lv_label_set_long_mode(stats_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(stats_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(stats_label, lv_color_hex(0xfff16a), 0);

    // The LVGL canvas points directly at viewport_fb, the same buffer TGX draws
    // into. Re-rendering TGX plus invalidating this object refreshes the view.
    viewport_canvas = lv_canvas_create(screen);
    lv_canvas_set_buffer(viewport_canvas, viewport_fb, VIEW_W, VIEW_H, LV_COLOR_FORMAT_RGB565);
    lv_obj_set_pos(viewport_canvas, 4, 30);
    lv_obj_add_flag(viewport_canvas, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(viewport_canvas, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(viewport_canvas, 1, 0);
    lv_obj_set_style_border_color(viewport_canvas, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_width(viewport_canvas, 8, 0);
    lv_obj_set_style_shadow_opa(viewport_canvas, LV_OPA_30, 0);
    lv_obj_set_style_shadow_color(viewport_canvas, lv_color_hex(0x102a43), 0);
    lv_obj_set_style_radius(viewport_canvas, 2, 0);
    lv_obj_add_event_cb(viewport_canvas, viewportEvent, LV_EVENT_ALL, nullptr);

    mesh_dropdown = lv_dropdown_create(screen);
    lv_dropdown_set_options(mesh_dropdown, "Teapot\nBunny\nBob tex\nBlub tex\nFalcon\nDonkey\nCyborg\nSpot");
    lv_dropdown_set_dir(mesh_dropdown, LV_DIR_TOP);
    lv_dropdown_set_symbol(mesh_dropdown, LV_SYMBOL_UP);
    lv_obj_set_pos(mesh_dropdown, 190, 206);
    lv_obj_set_size(mesh_dropdown, 126, 30);
    styleDropdownButton(mesh_dropdown);
    styleDropdownList(mesh_dropdown);
    lv_obj_add_event_cb(mesh_dropdown, dropdownReadyEvent, LV_EVENT_READY, nullptr);
    lv_obj_add_event_cb(mesh_dropdown, meshDropdownEvent, LV_EVENT_VALUE_CHANGED, nullptr);

    render_mode_dropdown = lv_dropdown_create(screen);
    lv_dropdown_set_options(render_mode_dropdown, "Wire\nFlat\nSmooth");
    lv_dropdown_set_selected(render_mode_dropdown, RENDER_SMOOTH);
    lv_dropdown_set_dir(render_mode_dropdown, LV_DIR_LEFT);
    lv_dropdown_set_symbol(render_mode_dropdown, nullptr);
    lv_obj_set_pos(render_mode_dropdown, 252, 33);
    lv_obj_set_size(render_mode_dropdown, 64, 28);
    styleCompactDropdownButton(render_mode_dropdown);
    styleDropdownList(render_mode_dropdown);
    lv_obj_add_event_cb(render_mode_dropdown, dropdownReadyEvent, LV_EVENT_READY, nullptr);
    lv_obj_add_event_cb(render_mode_dropdown, renderModeDropdownEvent, LV_EVENT_VALUE_CHANGED, nullptr);

    makeLabel(screen, "Zoom", 252, 70);
    zoom_slider = lv_slider_create(screen);
    lv_slider_set_range(zoom_slider, 60, 270);
    lv_slider_set_value(zoom_slider, zoom_percent, LV_ANIM_OFF);
    lv_obj_set_pos(zoom_slider, 252, 90);
    lv_obj_set_size(zoom_slider, 64, 12);
    lv_obj_set_style_bg_color(zoom_slider, lv_color_hex(0x0f766e), LV_PART_MAIN);
    lv_obj_set_style_bg_color(zoom_slider, lv_color_hex(0xff4f7b), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(zoom_slider, lv_color_hex(0xfff16a), LV_PART_KNOB);
    lv_obj_add_event_cb(zoom_slider, zoomSliderEvent, LV_EVENT_VALUE_CHANGED, nullptr);

    lv_obj_t* auto_label = makeLabel(screen, "Auto", 252, 113);
    lv_obj_set_width(auto_label, 64);
    lv_obj_set_style_text_align(auto_label, LV_TEXT_ALIGN_CENTER, 0);

    auto_switch = lv_switch_create(screen);
    lv_obj_set_pos(auto_switch, 262, 132);
    lv_obj_set_size(auto_switch, 44, 24);
    lv_obj_set_style_bg_color(auto_switch, lv_color_hex(0x0f766e), LV_PART_MAIN);
    lv_obj_set_style_bg_color(auto_switch, lv_color_hex(0xff4f7b), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(auto_switch, lv_color_hex(0xfff16a), LV_PART_KNOB);
    lv_obj_add_state(auto_switch, LV_STATE_CHECKED);
    lv_obj_add_event_cb(auto_switch, autoRotateEvent, LV_EVENT_VALUE_CHANGED, nullptr);

    lv_obj_t* reset_button = lv_button_create(screen);
    lv_obj_set_pos(reset_button, 252, 166);
    lv_obj_set_size(reset_button, 64, 28);
    lv_obj_set_style_bg_color(reset_button, lv_color_hex(0xff4f7b), 0);
    lv_obj_set_style_text_color(reset_button, lv_color_hex(0x102a43), 0);
    lv_obj_add_event_cb(reset_button, resetButtonEvent, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* reset_label = lv_label_create(reset_button);
    lv_label_set_text(reset_label, "Reset");
    lv_obj_center(reset_label);

    makeLabel(screen, "Drag viewport to rotate", 6, 194);

    mesh_info_label = makeLabel(screen, "-- tris --KB", 6, 222);
    lv_obj_set_width(mesh_info_label, 178);
    lv_label_set_long_mode(mesh_info_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(mesh_info_label, lv_color_hex(0xff4f7b), 0);
    lv_obj_set_style_text_align(mesh_info_label, LV_TEXT_ALIGN_LEFT, 0);
    }

void initTFT()
    {
    Serial.println("Initializing ILI9341_T4...");
    tft.output(&Serial);
    while (!tft.begin(SPI_SPEED))
        {
        Serial.println("ILI9341_T4 begin failed, retrying...");
        delay(250);
        }

    if (PIN_BACKLIGHT != 255)
        {
        pinMode(PIN_BACKLIGHT, OUTPUT);
        digitalWrite(PIN_BACKLIGHT, HIGH);
        }

    // ILI9341_T4 compares against the previous framebuffer and uploads only the
    // changed regions. This keeps LVGL redraws reasonably cheap on SPI.
    tft.setFramebuffer(internal_fb);
    tft.setDiffBuffers(&diff1, &diff2);
    tft.setRotation(1);
    tft.setDiffGap(4);
    tft.setVSyncSpacing(1);
    tft.setRefreshRate(100);
    tft.clear(0);

#if TGX_LVGL_MESHVIEWER_RUN_TOUCH_CALIBRATION
    Serial.println("Touch calibration mode enabled.");
    int touch_calib[4];
    tft.calibrateTouch(touch_calib);
    Serial.print("Paste these constants into touch_calib: { ");
    Serial.print(touch_calib[0]); Serial.print(", ");
    Serial.print(touch_calib[1]); Serial.print(", ");
    Serial.print(touch_calib[2]); Serial.print(", ");
    Serial.print(touch_calib[3]); Serial.println(" }");
    tft.setTouchCalibration(touch_calib);
#else
    int touch_calib[4] = TOUCH_CALIBRATION_CONSTANTS;
    tft.setTouchCalibration(touch_calib);
    Serial.println("Touch calibration constants loaded.");
#endif

    Serial.println("ILI9341_T4 ready.");
    }

void initLVGL()
    {
    Serial.println("Initializing LVGL...");
    lv_init();
    lv_tick_set_cb(lvgl_tick);

    // Partial mode: LVGL renders into lvgl_buf in strips instead of requiring a
    // second full-screen framebuffer.
    disp = lv_display_create(SCREEN_W, SCREEN_H);
    lv_display_set_flush_cb(disp, lvgl_flush);
    lv_display_set_buffers(disp, lvgl_buf, nullptr, SCREEN_W * LVGL_BUF_LINES * sizeof(int16_t), LV_DISPLAY_RENDER_MODE_PARTIAL);

    indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, lvgl_touch_read);

    createUI();
    Serial.println("LVGL ready.");
    }

void initTGX()
    {
    Serial.println("Initializing TGX renderer...");
    resetModelOrientation();

    // TGX renders into viewport_image only. The final physical upload still
    // goes through LVGL + ILI9341_T4.
    renderer.setViewportSize(VIEW_W, VIEW_H);
    renderer.setOffset(0, 0);
    renderer.setImage(&viewport_image);
    renderer.setZbuffer(zbuf);
    renderer.setPerspective(45, ((float)VIEW_W) / VIEW_H, 1.0f, 100.0f);
    renderer.setCulling(1);
    renderer.setShaders(SHADER_GOURAUD | SHADER_NOTEXTURE);
    renderer.setTextureWrappingMode(SHADER_TEXTURE_WRAP_POW2);
    renderer.setMaterial(RGBf(1.0f, 0.66f, 0.16f), 0.15f, 0.70f, 1.0f, 96);
    renderer.setLightDirection({ -0.35f, -0.55f, -1.0f });

    Serial.print("Screen framebuffer bytes: ");
    Serial.println(sizeof(internal_fb));
    Serial.print("LVGL draw buffer bytes: ");
    Serial.println(sizeof(lvgl_buf));
    Serial.print("TGX viewport buffer bytes: ");
    Serial.println(sizeof(viewport_fb));
    Serial.print("TGX z-buffer bytes: ");
    Serial.println(sizeof(zbuf));
    Serial.print("TGX mesh cache bytes: ");
    Serial.println(MESH_CACHE_SIZE);

    cacheSelectedMesh();
    Serial.println("TGX ready.");
    }

void setup()
    {
    Serial.begin(9600);
    delay(300);
    Serial.println();
    Serial.println("TGX + LVGL Mesh Viewer starting.");

    initTFT();
    initLVGL();
    initTGX();

    last_stats_ms = millis();
    renderViewport();
    }

void loop()
    {
    lv_timer_handler();

    // Keep rendering demand-driven: LVGL can run every loop, while TGX redraws
    // only when the model/UI changed or when auto-rotation advances.
    const uint32_t now = millis();
    if (auto_rotate && now - last_render_ms >= 33)
        {
        model_orientation.multRotate(1.2f, { 0, 1, 0 });
        render_requested = true;
        }

    if (render_requested && now - last_render_ms >= 16)
        {
        last_render_ms = now;
        renderViewport();
        }
    }

/** end of file */
