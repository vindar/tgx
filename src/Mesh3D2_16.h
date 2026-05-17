/**
 * @file Mesh3D2_16.h
 * Experimental Mesh3D2 variant reserved for a future 16-bit quantized payload.
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


    /**
     * Material definition for a Mesh3D2_16 object.
     *
     * This is currently a copy of MeshMaterial3D2. It is kept as a separate type so the
     * Mesh3D2_16 experiment can evolve independently from the baseline Mesh3D2 format.
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
     * This first version is intentionally identical to Meshlet3D2. The name is separated now so
     * a future patch can change the payload interpretation to 16-bit quantized arrays while keeping
     * the current Mesh3D2 format available for direct performance comparisons.
     */
    struct Meshlet3D2_16
        {
        fVec3 sphere_center;       ///< Bounding sphere center, in object space.
        fVec3 cone_dir;            ///< Normalized visibility cone direction, in object space, pointing from the meshlet toward visible cameras.

        float sphere_radius;       ///< Bounding sphere radius.
        float cone_cos;            ///< Cosine threshold for the visibility cone test.

        uint32_t payload_offset32; ///< Offset of this meshlet payload in Mesh3D2_16::payload, in 32-bit words.

        uint8_t nb_vertices;       ///< Number of local vertices. With the current uint8_t face stream format, this must be <= 128.
        uint8_t nb_normals;        ///< Number of local normals.
        uint8_t nb_texcoords;      ///< Number of local texture coordinates.
        uint8_t material_index;    ///< Index in Mesh3D2_16::materials.
        };




    /**
     * Experimental Mesh3D2 copy reserved for a future 16-bit quantized payload.
     *
     * The current Mesh3D2_16 payload is deliberately identical to Mesh3D2:
     *
     * ```
     * [fVec3 vertices[meshlet.nb_vertices]]
     * [fVec3 normals[meshlet.nb_normals]]
     * [fVec2 texcoords[meshlet.nb_texcoords]]
     * [uint8_t face stream]
     * [padding bytes to reach the next 4-byte boundary]
     * ```
     *
     * The separate type lets benchmarks compare the baseline Mesh3D2 path with the upcoming
     * quantized decoder without changing the validated Mesh3D2 format.
     */
    template<typename color_t>
    struct Mesh3D2_16
        {
        // make sure right away that the template parameter is admissible to prevent cryptic error message later.
        static_assert(is_color<color_t>::value, "color_t must be one of the color types defined in Color.h");

        int32_t id;                                        ///< Version/id. Expected to be 216 for Mesh3D2_16 once quantization is enabled.

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
