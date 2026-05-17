/**
 * @file Mesh3D2.h
 * Experimental meshlet-based 3D model mesh format.
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


#ifndef _TGX_MESH3D2_H_
#define _TGX_MESH3D2_H_

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
     * Material definition for a Mesh3D2 object.
     *
     * A Mesh3D2 object stores all its materials in a single array. Each meshlet references one
     * material by index. This replaces the Mesh3D linked-list convention that was mostly used to
     * switch texture/material between sub-meshes.
     */
    template<typename color_t>
    struct MeshMaterial3D2
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
     * Header for a single meshlet inside a Mesh3D2 object.
     *
     * Meshlet headers are stored in a compact linear array and are read before touching the meshlet
     * payload. This makes it possible to reject invisible meshlets quickly using the bounding sphere
     * and visibility cone, without fetching vertices, normals, texture coordinates or faces.
     *
     * The payload offset is expressed in 32-bit words into Mesh3D2::payload. This guarantees that
     * every meshlet payload starts on a 4-byte boundary when Mesh3D2::payload points to a uint32_t
     * array. The payload generator must also pad each meshlet payload to a multiple of 4 bytes.
     *
     * The first face stream format currently considered for Mesh3D2 uses uint8_t entries. The high
     * bit of the vertex index is used as the triangle-chain direction bit, so this format supports
     * local vertex indices in the range 0..127 (128 local vertices per meshlet maximum).
     */
    struct Meshlet3D2
        {
        fVec3 sphere_center;       ///< Bounding sphere center, in object space.
        fVec3 cone_dir;            ///< Normalized visibility cone direction, in object space, pointing from the meshlet toward visible cameras.

        float sphere_radius;       ///< Bounding sphere radius.
        float cone_cos;            ///< Cosine threshold for the visibility cone test.

        uint32_t payload_offset32; ///< Offset of this meshlet payload in Mesh3D2::payload, in 32-bit words.

        uint8_t nb_vertices;       ///< Number of local vertices. With the first uint8_t face stream format, this must be <= 128.
        uint8_t nb_normals;        ///< Number of local normals.
        uint8_t nb_texcoords;      ///< Number of local texture coordinates.
        uint8_t material_index;    ///< Index in Mesh3D2::materials.
        };




    /**
     * Experimental meshlet-based 3D mesh data structure.
     *
     * Mesh3D2 is a proposed replacement for Mesh3D for static 3D models. It is designed around
     * meshlets: small local pieces of geometry that can be culled independently before their payload
     * is read and rasterized.
     *
     * A Mesh3D2 object contains:
     *
     * - a global bounding box for fast rejection of the whole mesh,
     * - an array of materials, each with an optional texture and lighting coefficients,
     * - a linear array of meshlet headers used for culling,
     * - a 32-bit aligned payload blob containing the local arrays and face stream for each meshlet.
     *
     * Unlike Mesh3D, Mesh3D2 does not use a linked list of sub-meshes. Multi-texture models are
     * represented by a single Mesh3D2 object with several materials. Each meshlet stores a one-byte
     * material index.
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
     * [fVec3 vertices[meshlet.nb_vertices]]
     * [fVec3 normals[meshlet.nb_normals]]
     * [fVec2 texcoords[meshlet.nb_texcoords]]
     * [uint8_t face stream]
     * [padding bytes to reach the next 4-byte boundary]
     * ```
     *
     * If normals or texture coordinates are absent, the corresponding count is zero and the array is
     * omitted. Vertices, normals, texture coordinates and the face stream all start on a 4-byte
     * boundary. Each meshlet payload is padded with zero bytes to a multiple of 4 bytes.
     *
     * **Face stream**
     *
     * The first Mesh3D2 face stream format is the 8-bit local-index equivalent of the Mesh3D triangle
     * chain format. Each chain starts with a uint8_t triangle count and is followed by count+2
     * elements. A zero chain length marks the end of the meshlet face stream.
     *
     * An element occupies 1, 2 or 3 bytes depending on whether local texture coordinates and normals
     * are present:
     *
     * ```
     * [DBIT|LOCAL_VERTEX_INDEX] [LOCAL_TEXCOORD_INDEX, opt.] [LOCAL_NORMAL_INDEX, opt.]
     * ```
     *
     * The DBIT is the high bit of the vertex byte. The local vertex index is stored in the low 7 bits.
     * The first three elements of a chain have DBIT=0 and define the first triangle. Each subsequent
     * element defines the next triangle using the same convention as Mesh3D:
     *
     * - if DBIT=0, the next triangle is [V1, V3, V4],
     * - if DBIT=1, the next triangle is [V3, V2, V4].
     *
     * **Alignment and endianness**
     *
     * Mesh3D2::payload is a uint32_t array. Meshlet payload offsets are expressed in 32-bit words,
     * and each meshlet payload is padded to a 4-byte boundary. The generated payload is intended for
     * little-endian targets, which covers the current TGX targets (x86/x64, Teensy 3/4, ESP32,
     * RP2040 and RP2350).
     *
     * No vertex, normal, texture-coordinate or meshlet metadata quantization is used in this first
     * version of the format.
     */
    template<typename color_t>
    struct Mesh3D2
        {
        // make sure right away that the template parameter is admissible to prevent cryptic error message later.
        static_assert(is_color<color_t>::value, "color_t must be one of the color types defined in Color.h");

        int32_t id;                                     ///< Version/id. Expected to be 2 for Mesh3D2.

        uint32_t payload_size32;                        ///< Total size of the payload array, in 32-bit words.

        uint16_t nb_meshlets;                           ///< Number of meshlets in the mesh.
        uint16_t nb_faces;                              ///< Total number of triangular faces in the mesh.
        uint16_t nb_materials;                          ///< Number of materials. Must be <= 256 because meshlet material indices are uint8_t.

        const MeshMaterial3D2<color_t>* materials;      ///< Material array.
        const Meshlet3D2* meshlets;                     ///< Meshlet header array.
        const uint32_t* payload;                        ///< 32-bit aligned meshlet payload blob.

        fBox3 bounding_box;                             ///< Global object bounding box.

        const char* name;                               ///< Mesh name, or nullptr.
        };


}


#endif

#endif

/** end of file */
