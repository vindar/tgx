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


/*
* A 'minicube' is a 3D cube with rounded edges and 6 faces, each displaying a texture sticker.
* 
*             +--------+
*             |*       |
*             |  U[0]  |
*             |        |
*    +--------+--------+--------+--------+
*    |*       |*       |*       |*       |
*    |  L[4]  |  F[2]  |  R[1]  |  B[5]  |
*    |        |        |        |        |
*    +--------+--------+--------+--------+
*             |*       |
*             |  D[3]  |
*             |        |
*             +--------+
*
* Cube layout: U=0=up  R=1=right  F=2=front  D=3=down  L=4=left  B=5=back, 
*            : the top left corner of each sticker is at *
* 
* the front face of the cube is showing and facing up when the cube is a default 
* position and the camera is at (0, 0, +10) looking toward (0, 0, 0) with up vector (0, 1, 0).
*/
class Minicube
{



public:


    Minicube() = default;


    /**
    * Set the minicube texture image and the number of stickers in the images. 
    * 
    * The stickers in the image must be laid vertically (single row) i.e.
    * for i = 0 .. nbstickers-1, the sticker image is:
    * 
    * sticker[i] = subtexture  [0,1] x [i/nbstickers, (i+1)/nbstickers]
    * 
    * !!! recall that the texture size should be a power of 2 in each dimension 
    *     (but the stickers themselves do not have to be) !!!
    **/
    void setTexture(const tgx::Image<tgx::RGB565> & texture, int nbstickers)
        {
        _texture_image = texture;
        _nbstickers = ((nbstickers < 1) ? 1 : nbstickers); // ensure at least one sticker
        }


    /**
    * Set the cube mesh color, this is the cube of the boundary of the minicube.
    **/
    void setMeshColor(tgx::RGB565 meshcolor)
        {
        _meshcolor = meshcolor;
        }


    /**
    * Set the sticker associated with a given face:
    * 
    * - face:       0=U, 1=R, 2=F, 3=D, 4=L, 5=B  
    * - sticker:    from 0 to nbstickers-1
    **/
    void setFaceSticker(int face, int sticker)
        {
        const int mapi[6] = { 0, 5, 2, 1, 4, 3 };
        _face_sticker[mapi[face]] = sticker;
        }





    /**
    * Draw only the 'frame' of the minicube but not the textured sticker faces. 
    **/
    template<class RENDERER> void drawMinicubeFrame(const tgx::fMat4& M, RENDERER& renderer3D)
        {
        renderer3D.setCulling(0);
        renderer3D.setModelMatrix(M); // set the model-view matrix      
        renderer3D.setShaders(tgx::SHADER_GOURAUD);
        _mesh_boundary.color = _meshcolor; // set the boundary mesh color
        renderer3D.drawMesh(&_mesh_boundary);   // draw the boundary
        renderer3D.setCulling(1);
        }



    /**
    * Draw the minicube at location specified by a modelview matrix M.
    **/
    template<class RENDERER> void drawMinicube(const tgx::fMat4 & M, RENDERER& renderer3D)
        {   
        renderer3D.setModelMatrix(M); // set the model-view matrix      
        renderer3D.setShaders(tgx::SHADER_GOURAUD);
        _mesh_boundary.color = _meshcolor; // set the boundary mesh color
        renderer3D.drawMesh(&_mesh_boundary);   // draw the boundary

        _mesh_main.texcoord = _texture_array; // set the texture coordinates for the mesh
        _mesh_main.texture = &_texture_image; // set the texture image for the mesh
        _mesh_main.color = _meshcolor;        // set the mesh color

        const float inbstickers = 1.0f / _nbstickers; // inverse of the number of stickers
        const int sh[6] = { 3, 3, 3, 3, 1, 1 }; // sticker index for each face
        for (int index = 0; index < 6; index++)
            { // set the texture for each face
            const float xmin = 0.0f;
            const float xmax = 1.0f;
            const float ymin = ((_face_sticker[index] + 0.01f) * inbstickers);
            const float ymax = ((_face_sticker[index] + 0.99f) * inbstickers);
            _texture_array[4 * index + (0 + sh[index]) % 4] = {xmin , ymin};
            _texture_array[4 * index + (1 + sh[index]) % 4] = { xmin , ymax };
            _texture_array[4 * index + (2 + sh[index]) % 4] = { xmax , ymax };
            _texture_array[4 * index + (3 + sh[index]) % 4] = { xmax , ymin };
            }
        renderer3D.setShaders(tgx::SHADER_FLAT | tgx::SHADER_TEXTURE);
        renderer3D.drawMesh(&_mesh_main); // draw the colored faces
        }






private:
    
    int _nbstickers;
    tgx::Image<tgx::RGB565> _texture_image; 
    tgx::RGB565 _meshcolor; 
    int8_t _face_sticker[6]; // index of the sticker associated with each face.

    static tgx::fVec2 _texture_array[24];
    static tgx::Mesh3D<tgx::RGB565> _mesh_main;
    static tgx::Mesh3D<tgx::RGB565> _mesh_boundary;
    static const tgx::fVec3 _vert_array[24];
    static const tgx::fVec3 _norm_array[24];
    static const uint16_t _boundary_faces[95];
    static const uint16_t _main_faces[79];

};




/** end of file */

