/**
 * @file Mesh3D2_16b.h
 * Experimental Mesh3D2_16 variant with quantized meshlet headers.
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


#ifndef _TGX_MESH3D2_16B_H_
#define _TGX_MESH3D2_16B_H_

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


    namespace Mesh3D2_16b_detail
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
     * Material definition for a Mesh3D2_16b object.
     */
    template<typename color_t>
    struct MeshMaterial3D2_16b
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
     * Header for a single meshlet inside a Mesh3D2_16b object.
     *
     * The payload is identical to Mesh3D2_16. The difference is that the
     * meshlet bounding sphere and visibility cone fields are quantized to
     * 16 bits relative to the parent mesh bounding box.
     */
    struct Meshlet3D2_16b
        {
        int16_t sphere_center[3];  ///< Quantized bounding sphere center around the global bbox center.
        int16_t cone_dir[3];       ///< Signed-normalized quantized visibility cone direction.

        uint16_t sphere_radius;    ///< Quantized bounding sphere radius.
        int16_t cone_cos;          ///< Signed-normalized cone cosine. Values <= -32767 disable meshlet cone culling.

        uint32_t payload_offset32; ///< Offset of this meshlet payload in Mesh3D2_16b::payload, in 32-bit words.

        uint8_t nb_vertices;       ///< Number of local vertices. With the current uint8_t face stream format, this must be <= 128.
        uint8_t nb_normals;        ///< Number of local normals.
        uint8_t nb_texcoords;      ///< Number of local texture coordinates.
        uint8_t material_index;    ///< Index in Mesh3D2_16b::materials.
        };


    /**
     * Experimental Mesh3D2_16 variant with quantized meshlet headers.
     *
     * Mesh3D2_16b keeps the Mesh3D2_16 payload format unchanged, but stores
     * per-meshlet sphere center, sphere radius, cone direction and cone cosine
     * as 16-bit values. The renderer reconstructs these values from the global
     * bounding box once per meshlet before culling and payload decoding.
     */
    template<typename color_t>
    struct Mesh3D2_16b
        {
        static_assert(is_color<color_t>::value, "color_t must be one of the color types defined in Color.h");

        int32_t id;                                         ///< Version/id. Expected to be 2162 for Mesh3D2_16b.

        uint32_t payload_size32;                            ///< Total size of the payload array, in 32-bit words.

        uint16_t nb_meshlets;                               ///< Number of meshlets in the mesh.
        uint16_t nb_faces;                                  ///< Total number of triangular faces in the mesh.
        uint16_t nb_materials;                              ///< Number of materials.

        const MeshMaterial3D2_16b<color_t>* materials;      ///< Material array.
        const Meshlet3D2_16b* meshlets;                     ///< Meshlet header array.
        const uint32_t* payload;                            ///< 32-bit aligned meshlet payload blob.

        fBox3 bounding_box;                                 ///< Global object bounding box.

        const char* name;                                   ///< Mesh name, or nullptr.
        };


}


#endif

#endif

/** end of file */
