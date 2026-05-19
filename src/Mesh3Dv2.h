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
     * The sphere and cone metadata are quantized relative to the mesh global
     * bounding box. The renderer reconstructs this metadata before it touches
     * the meshlet payload:
     *
     * ```
     * bbox_center = 0.5 * (mesh.bounding_box.min + mesh.bounding_box.max)
     * bbox_extent = max(mesh.bounding_box size on x/y/z)
     *
     * decoded_sphere_center = bbox_center + sphere_center[i] * (bbox_extent / 65534)
     * decoded_sphere_radius = sphere_radius * (bbox_extent / 65535)
     * decoded_cone_dir      = cone_dir[i] * (1 / 32767)
     * decoded_cone_cos      = cone_cos * (1 / 32767)
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
     * The payload stores local meshlet arrays. Vertex positions are quantized
     * relative to the decoded meshlet bounding sphere above, not relative to
     * the global mesh bounding box. They are decoded as:
     *
     * ```
     * vertex = decoded_sphere_center + q * (decoded_sphere_radius / 32767)
     * ```
     *
     * Normals use signed-normalized coordinates:
     *
     * ```
     * normal = q * (1 / 32767)
     * ```
     *
     * Texture coordinates use a fixed signed range large enough for ordinary
     * UVs and simple tiling:
     *
     * ```
     * texcoord = q * (4 / 32767)    // range approximately [-4, 4]
     * ```
     *
     * The renderer computes these scale factors once per meshlet and then only
     * performs multiply-adds when a vertex, normal or texture coordinate is
     * actually needed by the active shader path.
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

        uint16_t nb_meshlets;                               ///< Number of meshlets in the mesh.
        uint16_t nb_materials;                              ///< Number of materials. Must be <= 256 because meshlet material indices are uint8_t.

        const MeshMaterial3Dv2<color_t>* materials;      ///< Material array.
        const Meshlet3Dv2* meshlets;                     ///< Meshlet header array.
        const uint32_t* payload;                            ///< 32-bit aligned meshlet payload blob.

        fBox3 bounding_box;                                 ///< Global object bounding box.

        const char* name;                                   ///< Mesh name, or nullptr.
        };


    /**
     * Creates a cached version of a Mesh3Dv2 object by copying selected data arrays
     * into user-supplied RAM buffers.
     *
     * This is useful on MCUs where meshes are stored in slow flash/PROGMEM but some
     * RAM is available for faster rendering. The method first copies the small
     * Mesh3Dv2 structure itself, then tries to copy the requested arrays in the
     * order specified by copy_order. If an array does not fit in either buffer, its
     * original pointer is kept.
     *
     * Copy-order letters:
     *
     * - "P" = meshlet payload blob.
     * - "L" = meshlet header array.
     * - "M" = material array.
     * - "I" = texture images referenced by the material array.
     *
     * Texture caching requires a writable material array because texture pointers
     * inside materials must be remapped. Therefore, requesting "I" implicitly tries
     * to cache the material array first if it has not already been cached. If the
     * material array cannot be cached, textures are left untouched.
     *
     * @remark
     * 1. The memory buffers supplied do not need to be aligned; the method aligns
     *    allocations internally.
     * 2. Mesh3Dv2 has no linked sub-meshes. Multi-material models are represented
     *    by the material array and per-meshlet material indices.
     * 3. The payload size is computed once by scanning the final meshlet payload.
     *    This has no runtime rendering cost.
     * 4. The returned mesh points either to cached arrays or to the original arrays
     *    for data that did not fit in the supplied buffers.
     *
     * @param mesh        Pointer to the Mesh3Dv2 object to cache.
     * @param ram1_buffer Pointer to the first memory buffer, usually the fastest RAM.
     * @param ram1_size   Size in bytes of the first RAM buffer.
     * @param ram2_buffer Pointer to a second memory buffer, may be nullptr.
     * @param ram2_size   Size in bytes of the second RAM buffer, or 0 if not supplied.
     * @param copy_order  Optional C string describing which data should be copied
     *                    and in which order. Default is "PLMI".
     * @param ram1_used   If non-null, receives the number of bytes consumed in ram1_buffer.
     * @param ram2_used   If non-null, receives the number of bytes consumed in ram2_buffer.
     *
     * @returns The cached mesh object, or the original mesh if even the Mesh3Dv2
     *          structure itself could not be copied.
     */
    template<typename color_t> const Mesh3Dv2<color_t>* cacheMesh(const Mesh3Dv2<color_t>* mesh,
                                                                  void* ram1_buffer, size_t ram1_size,
                                                                  void* ram2_buffer, size_t ram2_size,
                                                                  const char* copy_order = "PLMI",
                                                                  size_t* ram1_used = nullptr,
                                                                  size_t* ram2_used = nullptr);


}


#include "Mesh3Dv2.inl"


#endif

#endif

/** end of file */
