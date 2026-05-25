//
// Copyright 2021 Arvind Singh
//
// This file is part of the S.A.R.C.A.S.M. source code.
//
// This is a free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this code. If not, see <http://www.gnu.org/licenses/>.


#include "Minicube.h"
#include <Arduino.h>



// the vertices of a minicube. 
const tgx::fVec3 Minicube::_vert_array[24] PROGMEM = {
{1.0,0.9,-0.9},
{0.9,0.9,-1.0},
{0.9,1.0,-0.9},
{0.9,1.0,0.9},
{1.0,0.9,0.9},
{0.9,0.9,1.0},
{-0.9,1.0,0.9},
{-0.9,0.9,1.0},
{-1.0,0.9,0.9},
{-0.9,1.0,-0.9},
{-1.0,0.9,-0.9},
{-0.9,0.9,-1.0},
{0.9,-0.9,-1.0},
{1.0,-0.9,-0.9},
{0.9,-1.0,-0.9},
{-0.9,-1.0,-0.9},
{-0.9,-0.9,-1.0},
{-1.0,-0.9,-0.9},
{-0.9,-1.0,0.9},
{-1.0,-0.9,0.9},
{-0.9,-0.9,1.0},
{0.9,-1.0,0.9},
{0.9,-0.9,1.0},
{1.0,-0.9,0.9}
};


// the normal vectors associated with the vertices of a minicube
const tgx::fVec3 Minicube::_norm_array[24] PROGMEM = {
{0.9983367386403528,0.04076613963170857,-0.04076613963170857},
{0.04159451654038513,0.04159451654038513,-0.9982683969692435},
{0.021253348744365113,0.9989073909851608,-0.041621141291048346},
{0.07816135045932011,0.9961414664921865,0.0399121789579507},
{0.9982683969692435,0.04159451654038513,0.04159451654038513},
{0.04076613963170857,0.04076613963170857,0.9983367386403528},
{-0.021253348744365113,0.9989073909851608,0.041621141291048346},
{-0.04159451654038513,0.04159451654038513,0.9982683969692435},
{-0.9983367386403528,0.04076613963170857,0.04076613963170857},
{-0.07816135045932011,0.9961414664921865,-0.0399121789579507},
{-0.9982683969692435,0.04159451654038513,-0.04159451654038513},
{-0.04076613963170857,0.04076613963170857,-0.9983367386403528},
{0.04076613963170857,-0.04076613963170857,-0.9983367386403528},
{0.9982683969692435,-0.04159451654038513,-0.04159451654038513},
{0.07816135045932011,-0.9961414664921865,-0.0399121789579507},
{-0.021253348744365113,-0.9989073909851608,-0.041621141291048346},
{-0.04159451654038513,-0.04159451654038513,-0.9982683969692435},
{-0.9983367386403528,-0.04076613963170857,-0.04076613963170857},
{-0.07816135045932011,-0.9961414664921865,0.0399121789579507},
{-0.9982683969692435,-0.04159451654038513,0.04159451654038513},
{-0.04076613963170857,-0.04076613963170857,0.9983367386403528},
{0.021253348744365113,-0.9989073909851608,0.041621141291048346},
{0.04159451654038513,-0.04159451654038513,0.9982683969692435},
{0.9983367386403528,-0.04076613963170857,0.04076613963170857}
};


// list of faces that describe the boundary of the minicube (but not the 6 main 'colored' faces)
const uint16_t Minicube::_boundary_faces[95] PROGMEM = {
    12, // chain 0
    0,0, 1,1, 2,2, 3,3, 4,4, 32773,5, 32774,6, 7,7, 32776,8, 32777,9, 10,10, 32779,11, 32770,2, 1,1, 
    12, // chain 1
    12,12, 13,13, 14,14, 15,15, 16,16, 32785,17, 32786,18, 19,19, 32788,20, 32789,21, 22,22, 32791,23, 32782,14, 13,13, 
    2, // chain 2
    1,1, 0,0, 12,12, 32781,13, 
    2, // chain 3
    22,22, 23,23, 5,5, 32772,4, 
    2, // chain 4
    7,7, 8,8, 20,20, 32787,19, 
    2, // chain 5
    16,16, 17,17, 11,11, 32778,10, 
    0
    };

      
// list of faces that describe the main colored faces of the minicube (with texture indexes).
const uint16_t Minicube::_main_faces[79] PROGMEM = {
    2, // chain 4 (UP)
    6,0,6, 3,1,3, 2,2,2, 9,3,9,
    2, // chain 2 (DOWN)
    15,4,15, 14,5,14, 21,6,21, 18,7,18,
    2, // chain 5 (FRONT)
    20,8,20, 22,9,22, 5,10,5, 7,11,7,
    2, // chain 1 (BACK)
    12,12,12, 16,13,16, 11,14,11, 1,15,1, 
    2, // chain 3 (LEFT)
    8,16,8, 10,17,10, 17,18,17, 19,19,19,
    2, // chain 0 (RIGHT)
    0,20,0, 4,21,4, 23,22,23, 13,23,13,
    0
    };
    

// mesh that draws the minicube boundary 
tgx::Mesh3D<tgx::RGB565> Minicube::_mesh_boundary =
    {
    1, // version/id
    
    24, // number of vertices
    0, // number of texture coords
    24, // number of normal vectors
    32, // number of triangles
    95, // size of the face array. 

    Minicube::_vert_array, // array of vertices
    nullptr, // array of texture coords
    Minicube::_norm_array, // array of normal vectors        
    Minicube::_boundary_faces, // array of face vertex indexes   
    
    nullptr, // pointer to texture image 
    
    { 0.5f , 0.5f, 0.5f }, // default color
    
    0.1f, // ambiant light strength 
    0.9f, // diffuse light strength
    0.9f, // specular light strength
    32, // specular exponent
    
    nullptr,    
    
    { // mesh bounding box
    -1.0f, 1.0f, 
    -1.0f, 1.0f, 
    -1.0f, 1.0f
    },
    
    "minicube" // model name    
    };   


// the mesh with the 6 stickers
tgx::Mesh3D<tgx::RGB565> Minicube::_mesh_main = { 1, 24, 0, 24, 12, 79,
                                                Minicube::_vert_array,
                                                nullptr,
                                                Minicube::_norm_array,
                                                Minicube::_main_faces,
                                                nullptr,
                                                { 1.0f , 1.0f, 1.0f }, // default color
                                                0.2f, // ambiant light strength 
                                                0.7f, // diffuse light strength
                                                0.5f, // specular light strength
                                                64, // specular exponent
                                                nullptr, // next mesh to draw after this one    
                                                { -1.0f, 1.0f,-1.0f, 1.0f, -1.0f, 1.0f}, // bounding box
                                                "minicube" // model name    
                                                };

// array of texteure coords
tgx::fVec2 Minicube::_texture_array[24];



/** end of file */

