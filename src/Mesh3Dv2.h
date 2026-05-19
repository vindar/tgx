/**
 * @file Mesh3Dv2.h
 * Compact meshlet-based 3D model mesh format with 16-bit quantization.
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


#ifndef _TGX_MESH3DV2_H_
#define _TGX_MESH3DV2_H_

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


    namespace Mesh3Dv2_detail
    {
        /** Decode one local quantized vertex from a meshlet payload. */
        TGX_INLINE inline fVec3 load_vertex(const int16_t* table, uint8_t index, float base_x, float base_y, float base_z, float scale)
            {
            const int16_t* const q = table + ((size_t)index) * 3;
            return fVec3(base_x + ((float)q[0]) * scale,
                         base_y + ((float)q[1]) * scale,
                         base_z + ((float)q[2]) * scale);
            }

        /** Decode one local signed-normalized normal from a meshlet payload. */
        TGX_INLINE inline fVec3 load_normal(const int16_t* table, uint8_t index, float scale)
            {
            const int16_t* const q = table + ((size_t)index) * 3;
            return fVec3(((float)q[0]) * scale,
                         ((float)q[1]) * scale,
                         ((float)q[2]) * scale);
            }

        /** Decode one local texture coordinate from a meshlet payload. */
        TGX_INLINE inline fVec2 load_texcoord(const int16_t* table, uint8_t index, float scale)
            {
            const int16_t* const q = table + ((size_t)index) * 2;
            return fVec2(((float)q[0]) * scale,
                         ((float)q[1]) * scale);
            }
    }


    /**
     * Material definition for a Mesh3Dv2 object.
     *
     * A Mesh3Dv2 object stores all its materials in a single array. Each
     * meshlet references one material by index. This replaces the legacy
     * Mesh3D linked-list convention that was mostly used to switch texture or
     * material between sub-meshes.
     */
    template<typename color_t>
    struct MeshMaterial3Dv2
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
     * Header for a single meshlet inside a Mesh3Dv2 object.
     *
     * Meshlet headers are stored in a compact linear array and are read before
     * touching the meshlet payload. This makes it possible to reject invisible
     * meshlets quickly using the bounding sphere and visibility cone.
     *
     * The sphere and cone metadata are quantized to reduce flash/RAM traffic
     * before a meshlet is accepted. The renderer reconstructs them as:
     *
     * ```
     * bbox_center = 0.5 * (mesh.bounding_box.min + mesh.bounding_box.max)
     * bbox_extent = max(mesh.bounding_box size on x/y/z)
     *
     * sphere_center = bbox_center + sphere_center[i] * (bbox_extent / 65534)
     * sphere_radius = sphere_radius * (bbox_extent / 65535)
     * cone_dir      = cone_dir[i] * (1 / 32767)
     * cone_cos      = cone_cos * (1 / 32767)
     * ```
     *
     * A `cone_cos` value below or equal to -1 disables meshlet cone culling.
     * The payload offset is expressed in 32-bit words into Mesh3Dv2::payload.
     * This guarantees that every meshlet payload starts on a 4-byte boundary
     * when Mesh3Dv2::payload points to a uint32_t array. The payload
     * generator must also pad each meshlet payload to a multiple of 4 bytes.
     */
    struct Meshlet3Dv2
        {
        int16_t sphere_center[3];  ///< Quantized bounding sphere center relative to the global bounding-box center.
        int16_t cone_dir[3];       ///< Signed-normalized quantized visibility cone direction.

        uint16_t sphere_radius;    ///< Quantized bounding sphere radius.
        int16_t cone_cos;          ///< Signed-normalized cone cosine. Values <= -32767 disable meshlet cone culling.

        uint32_t payload_offset32; ///< Offset of this meshlet payload in Mesh3Dv2::payload, in 32-bit words.

        uint8_t nb_vertices;       ///< Number of local vertices. With the uint8_t face stream format, this must be <= 128.
        uint8_t nb_normals;        ///< Number of local normals.
        uint8_t nb_texcoords;      ///< Number of local texture coordinates.
        uint8_t material_index;    ///< Index in Mesh3Dv2::materials.
        };


    /**
     * Compact meshlet-based 3D mesh data structure.
     *
     * Mesh3Dv2 is the compact static-mesh format used by the TGX meshlet
     * renderer. It is designed for microcontrollers where the limiting factor
     * is often flash/RAM bandwidth and cache locality rather than pure ALU
     * throughput.
     *
     * A Mesh3Dv2 object contains:
     *
     * - a global bounding box for fast rejection of the whole mesh,
     * - an array of materials, each with an optional texture and lighting coefficients,
     * - a linear array of compact meshlet headers used for culling,
     * - a 32-bit aligned payload blob containing local arrays and face streams.
     *
     * Unlike Mesh3D, Mesh3Dv2 does not use a linked list of sub-meshes.
     * Multi-texture models are represented by one mesh object with several
     * materials. Each meshlet stores a one-byte material index.
     *
     * **Meshlet payload layout**
     *
     * The payload for a meshlet starts at:
     *
     * ```
     * reinterpret_cast<const uint8_t*>(mesh.payload + meshlet.payload_offset32)
     * ```
     *
     * and is laid out as:
     *
     * ```
     * [int16_t vertices[meshlet.nb_vertices][3]]
     * [int16_t normals[meshlet.nb_normals][3]]
     * [int16_t texcoords[meshlet.nb_texcoords][2]]
     * [uint8_t face stream]
     * [padding bytes to reach the next 4-byte boundary]
     * ```
     *
     * Vertices are decoded as:
     *
     * ```
     * sphere_center + q * (sphere_radius / 32767)
     * ```
     *
     * Normals are decoded as signed normalized values `q / 32767`. Texture
     * coordinates are decoded on the fixed range [-4, 4] as `q * (4 / 32767)`.
     *
     * **Face stream**
     *
     * Each triangle chain starts with a uint8_t triangle count and is followed
     * by count+2 vertex elements. A zero chain length marks the end of the
     * meshlet face stream.
     *
     * An element occupies 1, 2 or 3 bytes depending on whether local texture
     * coordinates and normals are present:
     *
     * ```
     * [DBIT|LOCAL_VERTEX_INDEX] [LOCAL_TEXCOORD_INDEX, opt.] [LOCAL_NORMAL_INDEX, opt.]
     * ```
     *
     * The DBIT is the high bit of the vertex byte. The local vertex index is
     * stored in the low 7 bits, so each meshlet supports up to 128 local
     * vertices. The first three elements of a chain have DBIT=0 and define the
     * first triangle. Each subsequent element defines the next triangle using
     * the same convention as Mesh3D:
     *
     * - if DBIT=0, the next triangle is [V1, V3, V4],
     * - if DBIT=1, the next triangle is [V3, V2, V4].
     *
     * **Alignment and endianness**
     *
     * Mesh3Dv2::payload is a uint32_t array. Meshlet payload offsets are
     * expressed in 32-bit words, and each meshlet payload is padded to a
     * multiple of 4 bytes. The generated payload is intended for little-endian
     * targets, which covers the current TGX targets (x86/x64, Teensy 3/4,
     * ESP32, RP2040 and RP2350).
     */
    template<typename color_t>
    struct Mesh3Dv2
        {
        static_assert(is_color<color_t>::value, "color_t must be one of the color types defined in Color.h");

        int32_t id;                                         ///< Version/id. Expected to be 2162 for Mesh3Dv2.

        uint32_t payload_size32;                            ///< Total size of the payload array, in 32-bit words.

        uint16_t nb_meshlets;                               ///< Number of meshlets in the mesh.
        uint16_t nb_faces;                                  ///< Total number of triangular faces in the mesh.
        uint16_t nb_materials;                              ///< Number of materials. Must be <= 256 because meshlet material indices are uint8_t.

        const MeshMaterial3Dv2<color_t>* materials;      ///< Material array.
        const Meshlet3Dv2* meshlets;                     ///< Meshlet header array.
        const uint32_t* payload;                            ///< 32-bit aligned meshlet payload blob.

        fBox3 bounding_box;                                 ///< Global object bounding box.

        const char* name;                                   ///< Mesh name, or nullptr.
        };


}


#endif

#endif

/** end of file */
