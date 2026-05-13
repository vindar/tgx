/********************************************************************
*
* tgx library example
*
* 
* Run a very crude 'benchmark' of the library 3D mesh rendering engine
* 
* Used to assert the impact of code changes in the library...
*
********************************************************************/
//
// Copyright 2020 Arvind Singh
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
//version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; If not, see <http://www.gnu.org/licenses/>.


#include <Arduino.h>
#include <tgx.h>
using namespace tgx;

#ifndef PROGMEM
#define PROGMEM
#endif

#include "3Dmodels/buddha.h"
#include "3Dmodels/R2D2.h"
#include "3Dmodels/bunny_fig_small.h"


/**
* Define beloww the platform used.
**/
#define TGX_BENCHMARK_T4            // use  600MHz, fastest without LTO
//#define TGX_BENCHMARK_ESP32         // use  240Mhz, QIO 80MHz (Huge App)
//#define TGX_BENCHMARK_ESP32S2       // use  240Mhz, QIO 80MHz (Huge App)
//#define TGX_BENCHMARK_ESP32S3       // use  240Mhz, QIO 80MHz (Huge App)
//#define TGX_BENCHMARK_RP2040        // use  200MHz, Ofast (RTTI, stack and exception disabled)  
//#define TGX_BENCHMARK_RP2350        // use  150MHz, Ofast (RTTI, stack and exception disabled)
//#define TGX_BENCHMARK_CPU          




#ifdef TGX_BENCHMARK_T4
    #define NBFRAMES 60
    #define DEV_NAME "Teensy 4.0/4.1"
    #define MAX_ALLOC_BYTES (330*1024)
    uint16_t fb[MAX_ALLOC_BYTES/2];
    DMAMEM uint16_t zbuf[MAX_ALLOC_BYTES/2];
    void allocateBuffers() { }
#endif

#ifdef TGX_BENCHMARK_ESP32
    #define NBFRAMES 45
    #define DEV_NAME "ESP32"
    #define MAX_ALLOC_BYTES (100*1024)
    uint16_t fb[MAX_ALLOC_BYTES / 2];
    uint16_t* zbuf = nullptr;
    void allocateBuffers()
        {
        zbuf = (uint16_t*)heap_caps_malloc(MAX_ALLOC_BYTES, MALLOC_CAP_DMA);
        if (!zbuf) { while (1) { Serial.println("Failed to allocate buffers"); delay(3000); } }
        }
#endif

#ifdef TGX_BENCHMARK_ESP32S2
    #define NBFRAMES 30
    #define DEV_NAME "ESP32-S2"
    #define MAX_ALLOC_BYTES (80*1024)
    uint16_t* fb = nullptr;
    uint16_t* zbuf = nullptr;
    void allocateBuffers()
        {
        fb = (uint16_t*)heap_caps_malloc(MAX_ALLOC_BYTES, MALLOC_CAP_DMA);
        zbuf = (uint16_t*)heap_caps_malloc(MAX_ALLOC_BYTES, MALLOC_CAP_DMA);
        if ((!fb) || (!zbuf)) { while (1) { Serial.println("Failed to allocate buffers"); delay(3000); } }
        }
#endif

#ifdef TGX_BENCHMARK_ESP32S3
    #define NBFRAMES 45
    #define DEV_NAME "ESP32-S3"
    #define MAX_ALLOC_BYTES (120*1024)
    uint16_t fb[MAX_ALLOC_BYTES / 2];
    uint16_t* zbuf = nullptr;
    void allocateBuffers()
        {
        zbuf = (uint16_t*)heap_caps_malloc(MAX_ALLOC_BYTES, MALLOC_CAP_DMA);
        if (!zbuf) { while (1) { Serial.println("Failed to allocate buffers"); delay(3000); } }
        }
#endif

#ifdef TGX_BENCHMARK_RP2040
    #define NBFRAMES 30
    #define DEV_NAME "RP2040"
    #define MAX_ALLOC_BYTES (90*1024)
    uint16_t* fb = nullptr;
    uint16_t* zbuf = nullptr;
    void allocateBuffers()
        {
        fb = (uint16_t*)malloc(MAX_ALLOC_BYTES);
        zbuf = (uint16_t*)malloc(MAX_ALLOC_BYTES);
        if ((!fb) || (!zbuf)) { while (1) { Serial.println("Failed to allocate buffers"); delay(3000); } }
        }
#endif

#ifdef TGX_BENCHMARK_RP2350
    #define NBFRAMES 30
    #define DEV_NAME "RP2350"
    #define MAX_ALLOC_BYTES (220*1024)
    uint16_t fb[MAX_ALLOC_BYTES / 2];
    uint16_t* zbuf = nullptr;
    void allocateBuffers()
        {
        zbuf = (uint16_t*)malloc(MAX_ALLOC_BYTES);
        if (!zbuf) { while (1) { Serial.println("*** Failed to allocate buffers ***"); delay(3000); } }
        }
#endif

#ifdef TGX_BENCHMARK_CPU
    #define NBFRAMES 120
    #define DEV_NAME "CPU"
    #define MAX_ALLOC_BYTES (4096*1024)
    uint16_t fb[MAX_ALLOC_BYTES / 2];
    uint16_t zbuf[MAX_ALLOC_BYTES / 2];
    void allocateBuffers() {}
#endif





Image<RGB565> im;
const Shader LOADED_SHADERS = SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_FLAT | SHADER_NOTEXTURE | SHADER_TEXTURE | SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_CLAMP | SHADER_TEXTURE_WRAP_POW2;
Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;


bool setRenderer(int slx, int sly)
    {
    if ((slx * sly * 2) > MAX_ALLOC_BYTES) return false; // too big
    im.set(fb, slx, sly);
    renderer.setViewportSize(slx, sly);
    renderer.setOffset(0, 0);
    renderer.setImage(&im);
    renderer.setZbuffer(zbuf);
    renderer.setPerspective(45, ((float)slx) / sly, 1.0f, 300.0f);
    renderer.setMaterial(RGBf(0.85f, 0.55f, 0.25f), 0.2f, 0.7f, 0.8f, 64);
    return true;
    }


static const int SLX = 100;
static const int SLY = 100;




#define DIST_NORMAL     0
#define DIST_CLOSE      1
#define DIST_CLIPPED    2
#define DIST_FAR        3


struct Score
    {
    uint32_t count; 
    uint32_t min_render;
    uint32_t max_render; 
    uint32_t sum_render; 
    uint64_t sum_render2;
    float avg_render; 
    float std_render; 

    Score()
        {
        reset();
        }

    void reset()
        {
        count = 0;
        min_render = 0xFFFFFFFF;
        max_render = 0;
        sum_render = 0;
        sum_render2 = 0;
        avg_render = 0.0f;
        std_render = 0.0f;
        }

    void add(uint32_t us)
        {
        count++;
        if (us < min_render) min_render = us;
        if (us > max_render) max_render = us;
        sum_render += us;
        sum_render2 += ((uint64_t)us * (uint64_t)us);
        avg_render = ((float)sum_render) / count;
        std_render = sqrtf((((float)sum_render2) / count) - (avg_render * avg_render));
        }

    float fps()
        {
        return 1000000.0f / avg_render;
        }

    void print()
        {
        Serial.printf(" frames: %u\t", count);
        Serial.printf(" min: %u us\t", min_render);
        Serial.printf(" max: %u us\t", max_render);
        Serial.printf(" avg: %.2f us\t", avg_render);
        Serial.printf(" stddev: %.2f us\t", std_render);
        Serial.printf(" fps: %.2f\n", fps());
        }


    void add(Score& s, int weight)
        {
        count += s.count * weight;
        if (s.min_render < min_render) min_render = s.min_render;
        if (s.max_render > max_render) max_render = s.max_render;
        sum_render += s.sum_render * weight;
        sum_render2 += s.sum_render2 * weight;
        avg_render = ((float)sum_render) / count;
        std_render = sqrtf((((float)sum_render2) / count) - (avg_render * avg_render));
        }
         
    };








/**
* Compute the score for a given mesh, with a given set of shaders at a given distance
**/
Score computeScore(Score & finaleS, int weight, const char * text, const tgx::Mesh3D<tgx::RGB565>* mesh, tgx::Shader shaders, int dist, int nb_frames = NBFRAMES)
    {
    Serial.print(text);
    Serial.print(" [");
    Score score;
    int r = 0; 
    for (int i = 0; i < nb_frames; i++)
        {
        float a = i * 360.0f / nb_frames; // angle

        int nr = (i * 10)/nb_frames;
        if (nr > r)
            {
            r = nr;
            Serial.printf("."); // progress indicator
            }

        //set the model position
        switch (dist)
            {
            case DIST_CLOSE: // close
                renderer.setModelPosScaleRot({ 0, -2.0, -10 }, { 13,13,13 }, a); // close
                break;
            case DIST_CLIPPED: // clipped
                renderer.setModelPosScaleRot({ 0, -1.0, -6 }, { 13,13,13 }, a); // clipped
                break;
            case DIST_FAR: // far away 
                renderer.setModelPosScaleRot({ 0, 0.0f, (float)-dist }, { 1,1,1 }, a); // far away.
                break;
            default: // normal
                renderer.setModelPosScaleRot({ 0, 0.5f, -35 }, { 13,13,13 }, a);  // normal
                break;
            }
        im.fillScreen(RGB565_Black); // clear the image        
        renderer.clearZbuffer(); // clear the z-buffer
        renderer.setShaders(shaders);
        const uint32_t m = micros();
        renderer.drawMesh(mesh, false);
        score.add(micros() - m);
        }
    Serial.print("] ");    
    score.print();
    finaleS.add(score, weight);
    return score;
    }






const int NB_RENDERER_SIZES = 5;
const iVec2 renderer_size_tab[NB_RENDERER_SIZES] = { {100,100}, {160,120}, {200,200} , {320,240}, {320,320} };



void setup()
    {
    Serial.begin(9600);
    Serial.flush(); 
    while (1)
        {
        Serial.println("Press any key to start benchmark...\n");
        for (int i = 0; i < 300; i++)
            {
            if (Serial.available()) { break; }
            delay(10);
            }
        if (Serial.available()) { break; }
        }
   
    Serial.print("--------------------------------------\n");
    Serial.print("TGX Benchmark for [");
    Serial.print(DEV_NAME);
    Serial.print("]\n--------------------------------------\n\n");
    allocateBuffers();

    const uint32_t ems = millis(); 

    for (int k = 0; k < NB_RENDERER_SIZES; k++)
        {
        int SLX = renderer_size_tab[k].x;
        int SLY = renderer_size_tab[k].y;
        Serial.print("\n-------------------------\n");
        Serial.printf("RENDERER SIZE: %d x %d\n", SLX, SLY);
        Serial.print("-------------------------\n");
        if (!setRenderer(SLX, SLY))
            {
            Serial.println("-> skipped (too big)\n\n");
            continue;
            }

        Score sfinalBuddha;
        computeScore(sfinalBuddha, 10, "Buddha GOURAUD   ", &buddha, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD, DIST_NORMAL);
        computeScore(sfinalBuddha, 10,  "Buddha FLAT     ", &buddha, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT, DIST_NORMAL);
        computeScore(sfinalBuddha, 3,  "Buddha (close)   ", &buddha, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD, DIST_CLOSE);
        computeScore(sfinalBuddha, 1,  "Buddha (clipped) ", &buddha, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD, DIST_CLIPPED);
        computeScore(sfinalBuddha, 3,  "Buddha (far)     ", &buddha, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD, DIST_FAR);
        Serial.print("total: "); sfinalBuddha.print();
        Serial.printf("-> FINAL SCORE : %.2f fps\n\n", sfinalBuddha.fps());

        Score sfinalR2D2;
        computeScore(sfinalR2D2, 8, "R2D2 GOURAUD        ", &R2D2, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD, DIST_NORMAL);
        computeScore(sfinalR2D2, 8, "R2D2 FLAT           ", &R2D2, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT, DIST_NORMAL);
        computeScore(sfinalR2D2, 10, "R2D2 TEX_NEAREST   ", &R2D2, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2, DIST_NORMAL);
        computeScore(sfinalR2D2, 10, "R2D2 TEX_BILINEAR  ", &R2D2, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_CLAMP, DIST_NORMAL);
        computeScore(sfinalR2D2, 3,  "R2D2 (close)       ", &R2D2, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_WRAP_POW2, DIST_CLOSE);
        computeScore(sfinalR2D2, 3,  "R2D2 (far)         ", &R2D2, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_WRAP_POW2, DIST_CLOSE);
        Serial.print("total: "); sfinalR2D2.print();
        Serial.printf("-> FINAL SCORE : %.2f fps\n\n", sfinalR2D2.fps());

        Score sfinalBunny;
        computeScore(sfinalBunny, 8, "Bunny GOURAUD      ", &bunny_fig_small, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD, DIST_NORMAL);
        computeScore(sfinalBunny, 8, "Bunny FLAT         ", &bunny_fig_small, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_FLAT, DIST_NORMAL);
        computeScore(sfinalBunny, 10, "Bunny TEX_NEAREST ", &bunny_fig_small, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2, DIST_NORMAL);
        computeScore(sfinalBunny, 10, "Bunny TEX_BILINEAR", &bunny_fig_small, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_BILINEAR | SHADER_TEXTURE_CLAMP, DIST_NORMAL);
        computeScore(sfinalBunny, 3, "Bunny (close)      ", &bunny_fig_small, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2, DIST_CLOSE);
        computeScore(sfinalBunny, 3, "Bunny (far)        ", &bunny_fig_small, SHADER_PERSPECTIVE | SHADER_ZBUFFER | SHADER_GOURAUD | SHADER_TEXTURE | SHADER_TEXTURE_NEAREST | SHADER_TEXTURE_WRAP_POW2, DIST_FAR);
        Serial.print("total: "); sfinalBunny.print();
        Serial.printf("-> FINAL SCORE : %.2f fps\n\n", sfinalBunny.fps());

        Score sfinal;
        sfinal.add(sfinalBuddha, 1);
        sfinal.add(sfinalR2D2, 1);
        sfinal.add(sfinalBunny, 1);
        Serial.printf("*** BENCHMARK FINAL SCORE: %0.2f fps ***\n\n", sfinal.fps());
        }

    const float t = (millis() - ems) / 1000.0f;
    Serial.printf("\n\nBenchmark completed in %0.2f seconds\n\n", t);
    while (1) { delay(1000); }
    }





void loop()
    {
    }





/** end of file */
