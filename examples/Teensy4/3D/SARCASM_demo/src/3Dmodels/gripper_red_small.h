// 3D model [gripper_red_small]
//
// - vertices   : 8
// - textures   : 0
// - normals    : 0
// - triangles  : 12
//
// - memory size: 0kb
//
// - model bounding box: [-7.08,-0.38]x[-7.81,7.79]x[10.35,14.15]
//
// object [gripper_red_small] (tagged [ | g gripper_red | usemtl 224,8,8]) with 12 triangles (2 chains)

#pragma once

#include <tgx.h>


// vertex array: 0kb.
const tgx::fVec3 gripper_red_small_vert_array[8] PROGMEM = {
{-0.379016,-7.810775,10.352888},
{-7.079016,-5.896491,14.152888},
{-7.079016,-5.896491,10.352888},
{-7.079016,5.87722,10.352888},
{-7.079016,5.87722,14.152888},
{-0.379016,7.791507,14.152888},
{-0.379016,-7.810775,14.152888},
{-0.379016,7.791507,10.352888}
};


// face array: 0kb.
const uint16_t gripper_red_small_face[19] PROGMEM = {
11, // chain 0
0, 1, 2, 
3, 32772, 32769, 5, 32774, 32768, 7, 32771, 5, 32772, 
1, // chain 1
6, 7, 5, 

 0};


// mesh info for object gripper_red_small (with tag [ | g gripper_red | usemtl 224,8,8])
const tgx::Mesh3D<tgx::RGB565> gripper_red_small PROGMEM = 
    {
    1, // version/id
    
    8, // number of vertices
    0, // number of texture coords
    0, // number of normal vectors
    12, // number of triangles
    19, // size of the face array. 

    gripper_red_small_vert_array, // array of vertices
    nullptr, // array of texture coords
    nullptr, // array of normal vectors        
    gripper_red_small_face, // array of face vertex indexes   
    
    nullptr, // pointer to texture image 
    
    { 0.75f , 0.75f, 0.75f }, // default color
    
    0.1f, // ambiant light strength 
    0.7f, // diffuse light strength
    0.6f, // specular light strength
    32, // specular exponent
    
    nullptr, // next mesh to draw after this one    
    
    { // mesh bounding box
    -7.079016f, -0.379016f, 
    -7.810775f, 7.791507f, 
    10.352888f, 14.152888f
    },
    
    "gripper_red_small" // model name    
    };
    
                
/** end of gripper_red_small.h */
    
    
    