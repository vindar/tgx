// 3D model [ILI9341_small]
//
// - vertices   : 8
// - textures   : 0
// - normals    : 0
// - triangles  : 12
//
// - memory size: 0kb
//
// - model bounding box: [88.36,98.67]x[-24.52,24.44]x[10.93,46.69]
//
// object [ILI9341_small] (tagged [ | g ILI9341 | usemtl 53,53,53]) with 12 triangles (2 chains)

#pragma once

#include <tgx.h>


// vertex array: 0kb.
const tgx::fVec3 ILI9341_small_vert_array[8] PROGMEM = {
{88.356752,-24.5161,46.438351},
{89.323831,24.4439,46.692845},
{88.356752,24.4439,46.438351},
{97.701753,24.4439,10.927365},
{98.668825,24.4439,11.181857},
{98.668825,-24.5161,11.181857},
{89.323831,-24.5161,46.692845},
{97.701753,-24.5161,10.927365}
};


// face array: 0kb.
const uint16_t ILI9341_small_face[19] PROGMEM = {
11, // chain 0
0, 1, 2, 
3, 32772, 32769, 5, 32774, 32768, 7, 32771, 5, 32772, 
1, // chain 1
6, 7, 5, 

 0};


// mesh info for object ILI9341_small (with tag [ | g ILI9341 | usemtl 53,53,53])
const tgx::Mesh3D<tgx::RGB565> ILI9341_small PROGMEM = 
    {
    1, // version/id
    
    8, // number of vertices
    0, // number of texture coords
    0, // number of normal vectors
    12, // number of triangles
    19, // size of the face array. 

    ILI9341_small_vert_array, // array of vertices
    nullptr, // array of texture coords
    nullptr, // array of normal vectors        
    ILI9341_small_face, // array of face vertex indexes   
    
    nullptr, // pointer to texture image 
    
    { 0.75f , 0.75f, 0.75f }, // default color
    
    0.1f, // ambiant light strength 
    0.7f, // diffuse light strength
    0.6f, // specular light strength
    32, // specular exponent
    
    nullptr, // next mesh to draw after this one    
    
    { // mesh bounding box
    88.356752f, 98.668825f, 
    -24.5161f, 24.4439f, 
    10.927365f, 46.692845f
    },
    
    "ILI9341_small" // model name    
    };
    
                
/** end of ILI9341_small.h */
    
    
    