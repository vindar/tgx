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

#pragma once

#include <tgx.h>
#include "Minicube.h"

/*
A 3D Rubik Cube. 

             |************|
             |*U1**U2**U3*|
             |************|
             |*U4**U5**U6*|
             |************|
             |*U7**U8**U9*|
             |************|
 ************|************|************|************
 *L1**L2**L3*|*F1**F2**F3*|*R1**R2**R3*|*B1**B2**B3*
 ************|************|************|************
 *L4**L5**L6*|*F4**F5**F6*|*R4**R5**R6*|*B4**B5**B6*
 ************|************|************|************
 *L7**L8**L9*|*F7**F8**F9*|*R7**R8**R9*|*B7**B8**B9*
 ************|************|************|************
             |************|
             |*D1**D2**D3*|
             |************|
             |*D4**D5**D6*|
             |************|
             |*D7**D8**D9*|
             |************|

Order: U1...U9 R1...R9 F1...F9 D1...D9 L1...L9 B1...B9
*/
class RubikCube3D
{

public:



    RubikCube3D() = default;


    /**
    * Initialise the cube. Set the texture, the number of stickers in the texture and the default sticker index
    * 
    * The stickers in the image must be laid vertically (single row) i.e.
    * for i = 0 .. nbstickers-1, the sticker image is:
    *
    * sticker[i] = subtexture  [0,1] x [i/nbstickers, (i+1)/nbstickers]
    *
    * !!! recall that the texture size should be a power of 2 in each dimension 
    *     (but the stickers themselves do not have to be) !!!
    **/
    void init(tgx::Image<tgx::RGB565> texture, int nbstickers, int default_sticker_value, tgx::RGB565 color);



    /**
    * Set the stickers of the cube from an array of integers.
    * The facet are enumerated in kociemba order from 0 to 53: U1...U9 R1...R9 F1...F9 D1...D9 L1...L9 B1...B9
    * 
    * - stickers: an array of 54 integers, each integer is the sticker value for the corresponding facet.
    **/
    void setStickers_int(const int* stickers);


    /**
    * Set the stickers of the cube from an array of char.
    * The facet are enumerated in kociemba order from 0 to 53: U1...U9 R1...R9 F1...F9 D1...D9 L1...L9 B1...B9
    *
    * - stickers: an array of 54 chars, each char is one of 'U'=0, 'R'=1, 'F'=2, 'D'=3, 'L'=4, 'B'=5 
    **/
    void setStickers_char(const char* stickers);



    /**
    * Reset the cube to its initial state. 
    **/
    void resetPosition();


    /**
    * Rotate a given slice of a given axe by a given angle.
    * - axe : 0=x, 1=y, 2=z
    * - slice < 0 , 0 or > 0 
    **/
    void rotateSlice(float angle, int axe, int slice);


    /**
    * Rotate the rubik cube around a given axe by a given angle.
    **/
    void rotate(float angle, int axe);



    /**
    * Tumble the cube.
    **/
    void tumble();


    /**
    * Draw the cube during a tumbling animation.
    **/
    template<class RENDERER> void draw_tumble(const tgx::fMat4& M, RENDERER& renderer3D, float ratio)
        {
        drawCube_rotate(M, renderer3D, 0, 90 * ratio);
        }


    /**
    * Turn the cube in a given direction.
    **/
    void turn(int dir);


    /**
    * Draw the cube during a turn animation.
    **/
    template<class RENDERER> void draw_turn(const tgx::fMat4& M, RENDERER& renderer3D, int dir, float ratio)
        {
        drawCube_rotate(M, renderer3D, 1, (dir * 90) * ratio);
        }


    /**
    * Turn the two bottom slices while keeping the top one in place
    **/
    void turn_bottom(int dir);


    /**
    * Draw the cube during a topturn animation.
    **/
    template<class RENDERER> void draw_turn_bottom(const tgx::fMat4& M, RENDERER& renderer3D, int dir, float ratio)
        {
        const float angle = (dir * 90) * ratio;
        const tgx::fVec3 V = tgx::fVec3{ 0.0f, 1.0f, 0.0f };
        for (int i = 0; i < 26; i++)
            {
            tgx::fMat4 mi = _mcm_cur[i];
            const float o = mi.M[12 + 1];
            if (o < 1.0f) { mi.multRotate(angle, V); }
            _mc[i].drawMinicube(M * mi, renderer3D);
            }
        }



    /**
    * Draw the cube at location specified by a modelview matrix M.
    **/
    template<class RENDERER> void drawCube(const tgx::fMat4& M, RENDERER& renderer3D)
        {
        for (int i = 0; i < 26; i++)
            {
            _mc[i].drawMinicube(M * _mcm_cur[i], renderer3D);
            }
        }



    /**
    * Draw only some cubelets.
    * if r < 0, reshuffle the cubelets
    **/
    template<class RENDERER> void drawCubePartial(const tgx::fMat4& M, float r, RENDERER& renderer3D, bool frame = false)
        {
        static uint8_t perm[26] =  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25 }; // { 7, 2, 23, 14, 4, 1, 17, 11, 9, 6, 22, 0, 15, 19, 13, 10, 24, 5, 20, 16, 12, 25, 3, 21, 8, 18 };
        if (r < 0)
            { 
            if (r <= -2)
                { // reset
                for (int i = 0; i < 26; i++) { perm[i] = i; }
                }
            else
                { // reshuffle
                for (int i = 25; i > 0; i--)
                    {
                    int j = random(i + 1);
                    const uint8_t t = perm[i];
                    perm[i] = perm[j];
                    perm[j] = t;
                    }
                }
            return;
            }        
        for (int i = 0; i < 26; i++)
            {
            if (i / 25.001f < r)
                {
                _mc[perm[i]].drawMinicube(M * _mcm_cur[perm[i]], renderer3D);
                } 
            else if (frame)
                {
                _mc[perm[i]].drawMinicubeFrame(M * _mcm_cur[perm[i]], renderer3D);
                }
            }
        }






    /**
    * Draw the cube at location specified by a modelview matrix M, rotating it around a given axe by a given angle.
    **/
    template<class RENDERER> void drawCube_rotate(const tgx::fMat4& M, RENDERER& renderer3D, int axe, float angle)
        {
        const tgx::fVec3 V = ((axe == 0) ? tgx::fVec3{ 1.0f, 0.0f, 0.0f } : ((axe == 1) ? tgx::fVec3{ 0.0f, 1.0f, 0.0f } : tgx::fVec3{ 0.0f, 0.0f, 1.0f }));
        for (int i = 0; i < 26; i++)
            {
            tgx::fMat4 mi = _mcm_cur[i];
            mi.multRotate(angle, V);
            _mc[i].drawMinicube(M * mi, renderer3D);
            }
        }


    /**
    * Draw the cube at location specified by a modelview matrix M, rotating a given slice around a given axe by a given angle.
    **/
    template<class RENDERER> void drawCube_rotateSlice(const tgx::fMat4& M, RENDERER& renderer3D, int axe, int slice, float angle)
        {
        const tgx::fVec3 V = ((axe == 0) ? tgx::fVec3{ 1.0f, 0.0f, 0.0f } : ((axe == 1) ? tgx::fVec3{ 0.0f, 1.0f, 0.0f } : tgx::fVec3{ 0.0f, 0.0f, 1.0f }));
        for (int i = 0; i < 26; i++)
            {
            tgx::fMat4 mi = _mcm_cur[i];
            const float o = mi.M[12 + axe];
            if (((slice > 0) && (o > 1.0f)) || ((slice < 0) && (o < -1.0f)) || ((slice == 0) && ((o < 1.0f) && (o > -1.0f)))) { mi.multRotate(angle, V); }
            _mc[i].drawMinicube(M * mi, renderer3D);
            }
        }


private: 

    Minicube    _mc[26];        // the 26 minicubes
    tgx::fMat4  _mcm_cur[26];   // and their current positions. 

};


/** end of file */

