/**
 * @file Mesh3D3_16.h
 * Experimental Mesh3D2_16 variant with per-meshlet visibility bitmasks.
 */
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


#ifndef _TGX_MESH3D3_16_H_
#define _TGX_MESH3D3_16_H_

// only C++, no plain C
#ifdef __cplusplus


#include "Misc.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Box3.h"
#include "Color.h"
#include "Image.h"

#include <stdint.h>


namespace tgx
{


    namespace Mesh3D3_16_detail
    {
        TGX_INLINE inline fVec3 load_vertex(const int16_t* table, uint8_t index, float base_x, float base_y, float base_z, float scale)
            {
            const int16_t* const q = table + ((size_t)index) * 3;
            return fVec3(base_x + ((float)q[0]) * scale,
                         base_y + ((float)q[1]) * scale,
                         base_z + ((float)q[2]) * scale);
            }

        TGX_INLINE inline fVec3 load_normal(const int16_t* table, uint8_t index, float scale)
            {
            const int16_t* const q = table + ((size_t)index) * 3;
            return fVec3(((float)q[0]) * scale,
                         ((float)q[1]) * scale,
                         ((float)q[2]) * scale);
            }

        TGX_INLINE inline fVec2 load_texcoord(const int16_t* table, uint8_t index, float scale)
            {
            const int16_t* const q = table + ((size_t)index) * 2;
            return fVec2(((float)q[0]) * scale,
                         ((float)q[1]) * scale);
            }
    }


    /**
     * Material definition for a Mesh3D3_16 object.
     *
     * This is intentionally separate from MeshMaterial3D2_16 so the experimental
     * visibility-mask format can evolve independently.
     */
    template<typename color_t>
    struct MeshMaterial3D3_16
        {
        static_assert(is_color<color_t>::value, "color_t must be one of the color types defined in Color.h");

        const Image<color_t>* texture;  ///< Optional texture image, or nullptr if the material is not textured.

        RGBf color;                     ///< Default color to use when texturing is disabled.

        float ambiant_strength;         ///< Object ambiant coefficient.
        float diffuse_strength;         ///< Object diffuse coefficient.
        float specular_strength;        ///< Object specular coefficient.
        int specular_exponent;          ///< Specular exponent. 0 to disable specular lighting.
        };


    /**
     * Header for a single meshlet inside a Mesh3D3_16 object.
     *
     * The payload is the same 16-bit quantized payload used by Mesh3D2_16. Meshlet
     * visibility is stored in Mesh3D3_16::visibility_masks instead of cone fields.
     */
    struct Meshlet3D3_16
        {
        fVec3 sphere_center;       ///< Bounding sphere center, in object space. Also used as the vertex dequantization base.

        float sphere_radius;       ///< Bounding sphere radius. Also used as the vertex dequantization scale.

        uint32_t payload_offset32; ///< Offset of this meshlet payload in Mesh3D3_16::payload, in 32-bit words.

        uint8_t nb_vertices;       ///< Number of local vertices. With the current uint8_t face stream format, this must be <= 128.
        uint8_t nb_normals;        ///< Number of local normals.
        uint8_t nb_texcoords;      ///< Number of local texture coordinates.
        uint8_t material_index;    ///< Index in Mesh3D3_16::materials.
        };


    /**
     * Experimental 16-bit meshlet format with a 1024-bit visibility mask per meshlet.
     *
     * The 1024 visibility bits are organized as a 32x32 octahedral direction map.
     * Each meshlet owns 32 consecutive uint32_t words in `visibility_masks`.
     * A bit set to 1 means that the meshlet may be visible from the associated
     * object-space direction. A bit set to 0 lets the renderer reject the meshlet.
     */
    template<typename color_t>
    struct Mesh3D3_16
        {
        static_assert(is_color<color_t>::value, "color_t must be one of the color types defined in Color.h");

        int32_t id;                                        ///< Version/id. Expected to be 316 for Mesh3D3_16.

        uint32_t payload_size32;                           ///< Total size of the payload array, in 32-bit words.

        uint16_t nb_meshlets;                              ///< Number of meshlets in the mesh.
        uint16_t nb_faces;                                 ///< Total number of triangular faces in the mesh.
        uint16_t nb_materials;                             ///< Number of materials.

        const MeshMaterial3D3_16<color_t>* materials;      ///< Material array.
        const Meshlet3D3_16* meshlets;                     ///< Meshlet header array.
        const uint32_t* visibility_masks;                  ///< `nb_meshlets * 32` words, one 1024-bit octahedral mask per meshlet.
        const uint32_t* payload;                           ///< 32-bit aligned meshlet payload blob.

        fBox3 bounding_box;                                ///< Global object bounding box.

        const char* name;                                  ///< Mesh name, or nullptr.
        };


}


#endif

#endif

/** end of file */
