/********************************************************************
*
* tgx library example : changing the geometry at runtime. 
* 
* 'Explosion' the scream Painting !
*
* EXAMPLE FOR TEENSY 4 / 4.1
*
* DISPLAY: ILI9341 (320x240)
*
********************************************************************/


// This example runs on teensy 4.0/4.1 with ILI9341 via SPI. 
// the screen driver library : https://github.com/vindar/ILI9341_T4
#include <ILI9341_T4.h> 

// the tgx library 
#include <tgx.h> 
#include <font_tgx_OpenSans_Bold.h>


// let's not burden ourselves with the tgx:: prefix
using namespace tgx;

// the texture image of the sheet
#include "scream_texture.h"


// 30mhz is ok but higher is really better in this case if you can afford it. 
#define SPI_SPEED       30000000


//
// DEFAULT WIRING USING SPI 0 ON TEENSY 4/4.1
//
#define PIN_SCK     13      // mandatory
#define PIN_MISO    12      // mandatory
#define PIN_MOSI    11      // mandatory
#define PIN_DC      10      // mandatory, can be any pin but using pin 10 (or 36 or 37 on T4.1) provides greater performance

#define PIN_CS      9       // optional (but recommended), can be any pin.  
#define PIN_RESET   6       // optional (but recommended), can be any pin. 
#define PIN_BACKLIGHT 255   // optional, set this only if the screen LED pin is connected directly to the Teensy.
#define PIN_TOUCH_IRQ 255   // optional. set this only if the touchscreen is connected on the same SPI bus
#define PIN_TOUCH_CS  255   // optional. set this only if the touchscreen is connected on the same spi bus


//
// ALTERNATE WIRING USING SPI 1 ON TEENSY 4/4.1 
//
//#define PIN_SCK     27      // mandatory 
//#define PIN_MISO    1       // mandatory
//#define PIN_MOSI    26      // mandatory
//#define PIN_DC      0       // mandatory, can be any pin but using pin 0 (or 38 on T4.1) provides greater performance

//#define PIN_CS      30      // optional (but recommended), can be any pin.  
//#define PIN_RESET   29      // optional (but recommended), can be any pin.  
//#define PIN_BACKLIGHT 255   // optional, set this only if the screen LED pin is connected directly to the Teensy. 
//#define PIN_TOUCH_IRQ 255   // optional. set this only if the touchscreen is connected on the same SPI bus
//#define PIN_TOUCH_CS  255   // optional. set this only if the touchscreen is connected on the same spi bus


// the screen driver object
ILI9341_T4::ILI9341Driver tft(PIN_CS, PIN_DC, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_RESET, PIN_TOUCH_CS, PIN_TOUCH_IRQ);


// 2 x 10K diff buffers (used by tft) for differential updates (in DMAMEM)
DMAMEM ILI9341_T4::DiffBuffStatic<10000> diff1;
DMAMEM ILI9341_T4::DiffBuffStatic<10000> diff2;

// screen dimension (landscape mode)
static const int SLX = 320;
static const int SLY = 240;

// main screen framebuffer (150K in DTCM for fastest access)
uint16_t fb[SLX * SLY];                 

// internal framebuffer (150K in DMAMEM) used by the ILI9431_T4 library for double buffering.
DMAMEM uint16_t internal_fb[SLX * SLY]; 

// zbuffer in 16 bits precision (150K in DMAMEM)
DMAMEM uint16_t zbuf[SLX * SLY];           

// image that encapsulates fb.
Image<RGB565> im(fb, SLX, SLY);

// only load the shaders we need.
const int LOADED_SHADERS = TGX_SHADER_PERSPECTIVE | TGX_SHADER_ZBUFFER | TGX_SHADER_GOURAUD | TGX_SHADER_NOTEXTURE | TGX_SHADER_TEXTURE_NEAREST |TGX_SHADER_TEXTURE_WRAP_POW2;

// the renderer object that performs the 3D drawings
Renderer3D<RGB565, LOADED_SHADERS, uint16_t> renderer;

static const int N = 45; // [-1,1]x[-1,1] is subdivided in NxM subsquares
static const int M = 45; // total number of triangles is 2*N*M

const float dx = 2.0f/N; // each subsquare has size  dx x dy
const float dy = 2.0f/M;

fVec3 vertices[(N+1)*(M+1)];        //  arrays of vertices : grid with y coordinate giving the height of the sheet at that point
fVec3 normals[(N + 1) * (M + 1)];   // arrays of normals
fVec2 texcoords[(N+1)*(M+1)];       // array of texture coordinates
DMAMEM uint16_t faces[4*N*M];       // arrays of quads (in DMAMEM because when are running out of memory in DTCM).  


/**
* Initialise the vertices, texcoords and faces arrays
**/
void initSheet()
    {
    const float dx = 2.0f/N; 
    const float dy = 2.0f/M;
    for (int j = 0; j <= M; j++)
        {
        for (int i = 0; i <= N; i++)
            {
            vertices[(N + 1)*j + i] = fVec3(i*dx - 1, 0, j*dy - 1);
            texcoords[(N + 1)*j + i] = fVec2(i*dx / 2, j*dy / 2);
            }
        }
    for (int j = 0; j < M; j++)
        {
        for (int i = 0; i < N; i++)
            {
            faces[4 * (N * j + i) + 0] = (uint16_t)(j*(N+1) + i);
            faces[4 * (N * j + i) + 1] = (uint16_t)((j + 1) * (N + 1) + i);
            faces[4 * (N * j + i) + 2] = (uint16_t)((j+1)*(N+1) + i + 1);
            faces[4 * (N * j + i) + 3] = (uint16_t)(j * (N + 1) + i + 1);
            }
        }
    }


/**
* Compute the face normal of the CCW triangle with index i1, i2, i3
* and add the vector to the normal vector of each of its vertices
**/
void faceN(int i1, int i2, int i3)
    {
    const fVec3 P1 = vertices[i1]; 
    const fVec3 P2 = vertices[i2];
    const fVec3 P3 = vertices[i3];
    const fVec3 N = crossProduct(P1 - P3, P2 - P1);
    normals[i1] += N;
    normals[i2] += N;    
    normals[i3] += N;
    }


/**
* Recompute the normal arrays. 
**/
void updateNormals()
    {
    // set all normals to 0
    for (int k = 0; k < (N + 1) * (M + 1); k++) normals[k] = fVec3(0, 0, 0);
    // add the normal of the adjacent faces
    for (int j = 0; j < M; j++)
        {
        for (int i = 0; i < N; i++)
            {
            faceN((N + 1) * j + i, (N + 1) * (j + 1) + i, (N + 1) * j + i + 1);
            faceN((N + 1) * (j + 1) + i, (N + 1) * (j + 1) + i + 1, (N + 1) * j + i + 1);
            }
        }
    // normalize the normals
    for (int k = 0; k < (N + 1) * (M + 1); k++) normals[k].normalize();
    }


/** sine cardinal function (could maybe speedup with a precomputed table ?)*/
float sinc(float x)
    {
    if (abs(x) < 0.01f) return 1.0f; 
    return sinf(x)/x; 
    }


/** return uniform random number in [a,b[ */
float unif(float a, float b)
    {
    const int step = 1000000;
    return a + ((b - a) * random(step)) / step;
    }


/** return the camera's curent position (depending on time) */
tgx::fVec3 cameraPosition()
    {   
    static elapsedMillis em; 
    const float T = 30.0f; // rotation period (in seconds). 
    const float d = 1.5 + 0.5*sin(em/10000.0f); // distance from sheet move closer / farther away with time. 
    const float w = em * 2 * M_PI / (1000 * T) + M_PI*0.6f;
    return fVec3(d * sinf(w), d*0.8f, d * cosf(w));
    }



/** display an explosion */
void explosion(fVec2 center, float h, float w, float s, float start_delay = 0)
    {
    Serial.print("\n\nExplosion\n - position: ");
    center.print();
    Serial.printf(" - height  : %.2f\n - size    : %.2f\n - speed   : %2f\n\n", h, w, s);

    s /= 1000.0f; // normalise speed

    elapsedMillis et = 0; 

    const float t0 = start_delay;          // time the bump starts to grow. 
    const float t1 = start_delay + 2000;   // time of explosion 
    const float t2 = start_delay + 5000;   // time when explosion is finished

    bool exploded = false; 

    while (et < t2)
        {
        float t = et; 

        // erase the screen
        im.fillScreen(RGB565_Black);

        // clear the z-buffer
        renderer.clearZbuffer();

        // amplitude multiplier 
        const float alpha = ((t <= t0) || (t >= t2)) ? 0 : ((t < t1) ? (t - t0) / (t1 - t0) : (1 - (t - t1) / (t2 - t1)));

        // compute heights
        for (int j = 0; j <= M; j++)
            {
            for (int i = 0; i <= N; i++)
                {
                const float x = dx*i - 1.0f - center.x;
                const float y = dy*j - 1.0f - center.y;
                const float r = sqrt(x * x + y * y); // distance to center. 
                vertices[(N + 1) * j + i].y = alpha * h * sinc(w* (r - max(0, t - t1)*s));
                }
            }

        // compute normals
        updateNormals();

        // increase time
        t += 0.5f;

        // set the camera position
        renderer.setLookAt(cameraPosition(), { 0,0,0 }, { 0,1,0 });

        // draw !        
        renderer.drawQuads(N * M, faces, vertices, faces, normals, faces, texcoords, &scream_texture);

       // remove the line above ('renderer.drawQuads(...') and uncomment the code below 
       // to draw the sheet without texturing but where the color depend in the height. 
       /*
       for(int j=0; j < N*M; j++)
          {
          const fVec3 V1 = vertices[faces[4*j]];
          const fVec3 V2 = vertices[faces[4*j + 1]];
          const fVec3 V3 = vertices[faces[4*j + 2]];
          const fVec3 V4 = vertices[faces[4*j + 3]];      

          const fVec3 N1 = normals[faces[4*j]];
          const fVec3 N2 = normals[faces[4*j + 1]];
          const fVec3 N3 = normals[faces[4*j + 2]];
          const fVec3 N4 = normals[faces[4*j + 3]];
     
          const float h1 = V1.y * 3.0f +0.4f;
          const float h2 = V2.y * 3.0f +0.4f;
          const float h3 = V3.y * 3.0f +0.4f;
          const float h4 = V4.y * 3.0f +0.4f;
          const RGBf C1(h1,h1*(1-h1)*3,1-h1);
          const RGBf C2(h2,h2*(1-h2)*3,1-h2);
          const RGBf C3(h3,h3*(1-h3)*3,1-h3);
          const RGBf C4(h4,h4*(1-h4)*3,1-h4);

          renderer.drawQuadWithVertexColor(V1,V2,V3,V4, C1, C2, C3, C4, &N1, &N2, &N3, &N4);
         }
        */

        // draw the current framerate on the framebuffer
        tft.overlayFPS(fb);

        // update the screen (asynchronous). 
        tft.update(fb);

        if (t < t1)
            Serial.print("-");
        else if (exploded)
            Serial.print(".");
        else
            {
            Serial.print("[BOOM!]");
            exploded = true;
            }
        }
    }


void setup()
    {
    Serial.begin(9600);

    tft.output(&Serial);                // output debug infos to serial port. 

    // initialize the ILI9341 screen
    while (!tft.begin(SPI_SPEED));

    // ok. turn on backlight
    pinMode(PIN_BACKLIGHT, OUTPUT);
    digitalWrite(PIN_BACKLIGHT, HIGH);

    // setup the screen driver 
    tft.setRotation(3); // portrait mode
    tft.setFramebuffer(internal_fb); // double buffering
    tft.setDiffBuffers(&diff1, &diff2); // 2 diff buffers
    tft.setDiffGap(4); // small gap
    tft.setRefreshRate(140); // refresh at 60hz
    tft.setVSyncSpacing(0); // lock the framerate at 60/2 = 30fps. 

    // setup the 3D renderer.
    renderer.setViewportSize(SLX,SLY); // viewport = screen     
    renderer.setOffset(0, 0); //  image = viewport
    renderer.setImage(&im); // set the image to draw onto (ie the screen framebuffer)
    renderer.setZbuffer(zbuf); // set the z buffer for depth testing    
    renderer.setPerspective(45, ((float)SLX) / SLY, 0.1f, 50.0f);  // set the perspective projection matrix. 
    renderer.setShaders(TGX_SHADER_GOURAUD | TGX_SHADER_TEXTURE_NEAREST | TGX_SHADER_TEXTURE_WRAP_POW2); // shader ot use
    renderer.setCulling(0); // in case we see below the sheet. 

    fMat4 MV;
    MV.setIdentity();
    renderer.setModelMatrix(MV); // identity model matrix. 

    // intialize the sheet. 
    initSheet(); 

    // the default seed give a nice sequence of explosion 
    randomSeed(0);
    }

                  

void loop()
    {
    // first an explosion carefully chosen...
    const float h = 0.4;
    const float w = 10;
    const float s = 0.4;
    fVec2 center(0,0);
    explosion(center, h, w, s, 2000);

    // ...and then random explosions
    while (1)
        { 
        const float h = unif(0.0f, 0.3f);   // bump height
        const float w = unif(3, 18);        // wave size
        const float s = unif(0.2, 2);       // wave speed; 
        fVec2 center(unif(-0.7, 0.7), unif(-0.7, 0.7));  // explosion center   
        explosion(center,h,w,s);

        //tft.printStats();         // for optimization  purposes. 
        //diff1.printStats();       //
        }
     }
       

/** end of file */

