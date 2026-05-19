/**
 * @file Mesh3D2_16.h
 * Experimental meshlet variant with a 16-bit quantized payload.
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


#ifndef _TGX_MESH3D2_16_H_
#define _TGX_MESH3D2_16_H_

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


    namespace Mesh3D2_16_detail
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
     * Material definition for a Mesh3D2_16 object.
     *
     * This is kept as a separate type so the Mesh3D2_16 experiment can evolve
     * independently from the other meshlet formats.
     */
    template<typename color_t>
    struct MeshMaterial3D2_16
        {
        // make sure right away that the template parameter is admissible to prevent cryptic error message later.
        static_assert(is_color<color_t>::value, "color_t must be one of the color types defined in Color.h");

        const Image<color_t>* texture;  ///< Optional texture image, or nullptr if the material is not textured.

        RGBf color;                     ///< Default color to use when texturing is disabled.

        float ambiant_strength;         ///< Object ambiant coefficient (how much it reflects the ambiant light component). Typical value: 0.2f.
        float diffuse_strength;         ///< Object diffuse coefficient (how much it reflects the diffuse light component). Typical value: 0.7f.
        float specular_strength;        ///< Object specular coefficient (how much it reflects the specular light component). Typical value: 0.5f.
        int specular_exponent;          ///< Specular exponent. 0 to disable specular lighting. Typical value between 4 and 64.
        };




    /**
     * Header for a single meshlet inside a Mesh3D2_16 object.
     *
     * This header stores meshlet bounds and the payload location. The payload arrays
     * are stored with signed 16-bit quantization to reduce mesh memory traffic.
     */
    struct Meshlet3D2_16
        {
        fVec3 sphere_center;       ///< Bounding sphere center, in object space. Also used as the vertex dequantization base.
        fVec3 cone_dir;            ///< Normalized visibility cone direction, in object space, pointing from the meshlet toward visible cameras.

        float sphere_radius;       ///< Bounding sphere radius. Also used as the vertex dequantization scale.
        float cone_cos;            ///< Cosine threshold for the visibility cone test.

        uint32_t payload_offset32; ///< Offset of this meshlet payload in Mesh3D2_16::payload, in 32-bit words.

        uint8_t nb_vertices;       ///< Number of local vertices. With the current uint8_t face stream format, this must be <= 128.
        uint8_t nb_normals;        ///< Number of local normals.
        uint8_t nb_texcoords;      ///< Number of local texture coordinates.
        uint8_t material_index;    ///< Index in Mesh3D2_16::materials.
        };




    /**
     * Experimental meshlet variant with 16-bit quantized local arrays.
     *
     * The Mesh3D2_16 payload layout is:
     *
     * ```
     * [int16_t vertices[meshlet.nb_vertices][3]]
     * [int16_t normals[meshlet.nb_normals][3]]
     * [int16_t texcoords[meshlet.nb_texcoords][2]]
     * [uint8_t face stream]
     * [padding bytes to reach the next 4-byte boundary]
     * ```
     *
     * Vertices are decoded as `meshlet.sphere_center + q * (meshlet.sphere_radius / 32767)`.
     * Normals are decoded as `q / 32767`. Texture coordinates are decoded on the fixed
     * range [-4, 4] as `q * (4 / 32767)`.
     *
     * Keeping this as a separate type lets benchmarks compare different meshlet decoders side by side.
     */
    template<typename color_t>
    struct Mesh3D2_16
        {
        // make sure right away that the template parameter is admissible to prevent cryptic error message later.
        static_assert(is_color<color_t>::value, "color_t must be one of the color types defined in Color.h");

        int32_t id;                                        ///< Version/id. Expected to be 216 for Mesh3D2_16.

        uint32_t payload_size32;                           ///< Total size of the payload array, in 32-bit words.

        uint16_t nb_meshlets;                              ///< Number of meshlets in the mesh.
        uint16_t nb_faces;                                 ///< Total number of triangular faces in the mesh.
        uint16_t nb_materials;                             ///< Number of materials. Must be <= 256 because meshlet material indices are uint8_t.

        const MeshMaterial3D2_16<color_t>* materials;      ///< Material array.
        const Meshlet3D2_16* meshlets;                     ///< Meshlet header array.
        const uint32_t* payload;                           ///< 32-bit aligned meshlet payload blob.

        fBox3 bounding_box;                                ///< Global object bounding box.

        const char* name;                                  ///< Mesh name, or nullptr.
        };


}


#endif

#endif

/** end of file */
