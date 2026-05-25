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

#include "RubikCube3D.h"
#include "Minicube.h"
#include <Arduino.h>




// Liste of initial position for each minicube (index between 0 and 26)
static const tgx::fVec3 MINICUBE_DEFAULTPOS[26] PROGMEM = {
    {-2,2,2},   {0,2,2},   {2,2,2},
    {-2,0,2},   {0,0,2},   {2,0,2},
    {-2,-2,2},  {0,-2,2},  {2,-2,2},
    {-2,2,0},   {0,2,0},   {2,2,0},
    {-2,0,0},              {2,0,0},
    {-2,-2,0},  {0,-2,0},  {2,-2,0},
    {-2,2,-2},  {0,2,-2},  {2,2,-2},
    {-2,0,-2},  {0,0,-2},  {2,0,-2},
    {-2,-2,-2}, {0,-2,-2}, {2,-2,-2},
    };


// mapping from facets to minicube (indexed from 1)
static const int FACET_TO_MINICUBE_ORDER[54] PROGMEM = {
    17, 18, 19, 9, 10, 11, 0, 1, 2,       // U1...U9
    2, 11, 19, 5, 13, 22, 8, 16, 25,      // R1...R9
    0, 1, 2, 3, 4, 5, 6, 7, 8,            // F1...F9
    6, 7, 8, 14, 15, 16, 23, 24, 25,      // D1...D9
    17, 9, 0, 20, 12, 3, 23, 14, 6,       // L1../L9 
    19, 18, 17, 22, 21, 20, 25, 24, 23    // B1...B9
    };




void RubikCube3D::init(tgx::Image<tgx::RGB565> texture, int nbstickers, int default_sticker_value, tgx::RGB565 color)
    {
    for (int i = 0; i < 26; i++)
        {
        _mc[i].setMeshColor(color);
        _mc[i].setTexture(texture, nbstickers);
        for (int k = 0; k < 6; k++) _mc[i].setFaceSticker(k, default_sticker_value); // set all stickers to thier default value
        _mcm_cur[i].setTranslate(MINICUBE_DEFAULTPOS[i]);
        }
    }


void RubikCube3D::setStickers_int(const int* stickers)
    {
    if (!stickers) return;
    for (int n = 0; n < 54; n++)
        {
        const int i = FACET_TO_MINICUBE_ORDER[n]; // minicube index for this facet
        _mc[i].setFaceSticker(n / 9, stickers[n]);
        }
    }


void RubikCube3D::setStickers_char(const char* stickers)
    {
    if (!stickers) return;
    for (int n = 0; n < 54; n++)
        {
        const int i = FACET_TO_MINICUBE_ORDER[n]; // minicube index for this facet
        switch (stickers[n])
            {
            case 'u':
            case 'U': _mc[i].setFaceSticker(n / 9, 0); break; // U
            case 'r':
            case 'R': _mc[i].setFaceSticker(n / 9, 1); break; // R
            case 'f':
            case 'F': _mc[i].setFaceSticker(n / 9, 2); break; // F
            case 'd':
            case 'D': _mc[i].setFaceSticker(n / 9, 3); break; // D
            case 'l':
            case 'L': _mc[i].setFaceSticker(n / 9, 4); break; // L
            case 'b':
            case 'B': _mc[i].setFaceSticker(n / 9, 5); break; // B
            default: break; // no sticker
            }
        }
    }


void RubikCube3D::resetPosition()
    {
    for (int i = 0; i < 26; i++)
        {
        _mcm_cur[i].setTranslate(MINICUBE_DEFAULTPOS[i]);
        }    
    }


void RubikCube3D::tumble()
    {
    rotate(90, 0);
    }


void RubikCube3D::turn(int dir)
    {
    rotate((dir * 90), 1);
    }


void RubikCube3D::turn_bottom(int dir)
    {
    const float angle = (dir * 90);
    const tgx::fVec3 V = tgx::fVec3{ 0.0f, 1.0f, 0.0f };
    for (int i = 0; i < 26; i++)
        {
        const float o = _mcm_cur[i].M[12 + 1];
        if (o < 1.0f) { _mcm_cur[i].multRotate(angle, V); }
        }
    }



void RubikCube3D::rotateSlice(float angle, int axe, int slice)
    {
    const tgx::fVec3 V = ((axe == 0) ? tgx::fVec3{ 1.0f, 0.0f, 0.0f } : ((axe == 1) ? tgx::fVec3{ 0.0f, 1.0f, 0.0f } : tgx::fVec3{ 0.0f, 0.0f, 1.0f }));
    for (int i = 0; i < 26; i++)
        {
        float o = _mcm_cur[i].M[12 + axe];
        if (((slice > 0) && (o > 1.0f)) || ((slice < 0) && (o < -1.0f)) || ((slice == 0) && ((o < 1.0f) && (o > -1.0f)))) { _mcm_cur[i].multRotate(angle, V); }
        }
    }



void RubikCube3D::rotate(float angle, int axe)
    {
    const tgx::fVec3 V = ((axe == 0) ? tgx::fVec3{ 1.0f, 0.0f, 0.0f } : ((axe == 1) ? tgx::fVec3{ 0.0f, 1.0f, 0.0f } : tgx::fVec3{ 0.0f, 0.0f, 1.0f }));
    for (int i = 0; i < 26; i++)
        {
        _mcm_cur[i].multRotate(angle, V);
        }
    }



/** end of file */

