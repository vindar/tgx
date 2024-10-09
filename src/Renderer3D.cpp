/** @file Renderer3D.cpp */
//
// Copyright 2020 Arvind Singh
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; If not, see <http://www.gnu.org/licenses/>.
#include "Renderer3D.h"



namespace tgx
{




    /**
    * vertex list and face list for the unit cube [-1,1]^3. 
    *
    *
    *                                                                   H--------E
    *                                                                   |        |
    *                                                                   |  top   |
    *                          H-------------E                          |        |
    *                         /.            /|                 H--------A--------D--------E
    *                        / .   top     / |                 |        |        |        |
    *                       /  .          /  |                 |  left  | front  |  right |
    *                      A------------D    |  right          |        |        |        |
    *                      |   .        |    |                 G--------B--------C--------F
    *                      |   G .......|....F                          |        |
    *                      |  .         |   /                           | bottom |
    *                      | .  front   |  /                            |        |
    *                      |.           | /                             G--------F
    *                      B------------C                               |        |
    *                                                                   |  back  |
    *                                                                   |        |
    *                                                                   H--------E
    **/

    const tgx::fVec3 UNIT_CUBE_VERTICES[8] =
        {
        {-1,1,1},     //A = 0
        {-1,-1,1},    //B = 1
        {1,-1,1},     //C = 2
        {1,1,1},      //D = 3
        {1,1,-1},     //E = 4
        {1,-1,-1},    //F = 5
        {-1,-1,-1},   //G = 6
        {-1,1,-1}     //H = 7
        };

    const tgx::fVec3 UNIT_CUBE_NORMALS[6] =
        {
        {0,0,1},
        {0,0,-1},
        {0,1,0},
        {0,-1,0},
        {-1,0,0},        
        {1,0,0}        
        };


    const uint16_t UNIT_CUBE_FACES[6 * 4] =
        {
        0,1,2,3, // front   ABCD
        4,5,6,7, // back    EFGH
        7,0,3,4, // top     HADE
        1,6,5,2, // bottom  BGFC
        7,6,1,0, // left    HGBA
        3,2,5,4  // right   DCFE
        };

    const uint16_t UNIT_CUBE_FACES_NORMALS[6 * 4] =
        {
        0,0,0,0, // front   ABCD
        1,1,1,1, // back    EFGH
        2,2,2,2, // top     HADE
        3,3,3,3, // bottom  BGFC
        4,4,4,4, // left    HGBA
        5,5,5,5  // right   DCFE
        };



}

/** end of file */

