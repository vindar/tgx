/**   
 * @file Mesh3D.h 
 * 3D model mesh class. 
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


#ifndef _TGX_MESH3D_H_
#define _TGX_MESH3D_H_

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
     * 3D mesh data stucture.
     * 
     * A Mesh3D structure contain all the informations about an object geometry, color, material
     * property and textures. A mesh can be rendered onto an `Image` object using the
     * `Renderer3D::drawMesh()` method.
     * 
     * The mesh data format is designed to be compact and favors linear access to elements in order
     * to improve cache coherency so that meshes can be stored and rendered directly from "slow" 
     * memory such as FLASH on a MCU.
     * 
     * The Python scripts `obj_to_h` located in the `\tools` directory of the library can be used to
     * create a `Mesh3D` object directly from an .obj file.  The associated image textures (if any)
     * can be converted using the Python script `texture_2_h`.
     *
     * **MESH FORMAT**
     *
     * @param vertice   Array of all the vertices of the mesh in `(x,y,z)` `fVec3` format. A vertex is
     *                  refered by its index in this array. **Maximum number of vertices per mesh: 32767.**
     *
     * @param texcoord  Array of texture coordinate in `(u,v)` `fVec2` format. A texture coord is
     *                  refered by its index in this array. Set to nullptr (and nb_texcoords=0) if
     *                  the mesh has no texture. **Maximum number of texture coords: 65535**
     *
     * @param normal    Array of normal vectors in (x,y,z) fVec3 format. A normal vector is refered
     *                  by its index in this array. Set to nullptr if no normal vectors are defined.
     *                  **Normal vectors must have unit norm**. **Maximum number of normal vectors: 65535**
     *
     * @param face      Array of triangular faces. The array is composed of `uint16_t` and is divided into
     *                  *chains of triangles*. See below for details.
     *
     * @param texture   image texture object associated with the mesh (or nullptr is none).
     *
     * @param next      pointer to the next mesh (or nullptr if none). Chaining mesh enalbe to draw complex 
     *                  models with multiple textures images and more than 32767 vertices...
     *
     * 
     * **Structure of the `face` array**.
     * 
     * the array is composed of "chains of triangles". Each chain starts with a `uint16_t` that
     * specifies its length and is followed by a sequence of elements where each element occupies 1,
     * 2 or 3 `uint16_t`. The array takes the following general form:
     *
     * ```
     * [chain 1 length = n] [elem 1] [elem 2] [elem 3]  ... [elem n+2]
     * [chain 2 length = m] [elem 1] [elem 2] [elem 3]  ... [elem m+2]
     *       ...
     * [chain k length = p] [elem 1] [elem 2] [elem 3]  ... [elem p+2] [endtag = 0]
     * ```
     *
     * - An endtag `(int16_t)0` is used as a sentinel to mark the end of the `face` array.
     *
     * - Each chain starts with a single uint16_t [chain length] that specify the number of
     *   triangles in the chain. It is followed by [chain length]+2 elements. 
     *
     * A "element" represent a vertex of a triangle and occupies 1, 2 or 3 `uint16_t`
     * depending on whether the textcoord and normal arrays exist in the mesh. The first 
     * `uint16_t` of an element is the vertex index (with a *direction bit*), then is 
     * followed by the texture index if a texture array is present and then finally followed by
     * the normal index if a normal array is present. 
     * 
     * Schematiccally:
     *
     *```
     *    1bit     15bits      (16bits, opt.)   (16bits, opt.)
     *   [DBIT| VERTEX INDEX]  [TEXTURE INDEX]  [NORMAL INDEX]
     *```
     * 
     * The first 3 elements of a chain represent the 3 vertices that compose the first triangle 
     * of the chain. Their DBIT is always 0. Each subsequent element is combined with the current 
     * triangle to deduce the next triangle in the following way: suppose the current triangle is 
     * `[V1, V2, V3]` and the next element is `DBIT|V4`:
     *
     * - if `DBIT = 0`, then the next triangle is `[V1, V3, V4]`
     * - if `DBIT = 1`, then the next triangle is `[V3, V2, V4]`.
     *
     * **Example**
     *
     * We assume here `texcoord = nullptr` but `normal != nullptr` so that
     * each element is coded on 2 `uint16_t`.
     *
     * Let us consider the following face array composed of 19 `uint16_t`:
     *
     * ```
     * face = {3,0,1,2,2,4,6,32773,8,7,7,1,8,7,9,4,5,5,0}
     *
     * ```
     * 
     * It should be read as
     *
     * ```
     * 3                the first chain has 3 triangles
     * 0/1   2/2  4/6   first triangle with vertex 0,2,4 and normals 1,2,6
     * 5/8              32773 = 32768 + 5 so DBIT = 1, second triangle
     * 7/7              third triangle DBIT = 0
     * 1                the second chain has a single triangle
     * 8/7   9/4  5/5   the triangle of the second chain
     * 0                end tag
     * ```
     * 
     * after decoding, this gives a list of 4 triangles (with normals associated 
     * with each vertex index).
     *
     * ```
     * 0/1   2/2  4/6
     * 4/6   2/2  5/8
     * 4/6   5/8  7/7
     * 8/7   9/4  5/5
     * ```
     */
    template<typename color_t> 
    struct Mesh3D
        {
        // make sure right away that the template parameter is admissible to prevent cryptic error message later.  
        static_assert(is_color<color_t>::value, "color_t must be one of the color types defined in color.h");

        int32_t id;                     ///< Set to 1 (may change in future version).

        uint16_t nb_vertices;           ///< Number of vertices in the vertex array.
        uint16_t nb_texcoords;          ///< Number of texture coordinates in the texcoord array.
        uint16_t nb_normals;            ///< Number of normal vectors in the normal array.
        uint16_t nb_faces;              ///< Number of triangular faces in the mesh.
        uint16_t len_face;              ///< Number of elements (uint16_t) in the face array.

        const fVec3* vertice;           ///< Vertex array.
        const fVec2* texcoord;          ///< Texture coord array (nullptr if none).
        const fVec3* normal;            ///< Normal vector array (nullptr if none). **all vectors must have unit norm**.
        const uint16_t* face;           ///< Array of triangles (format described above).

        const Image<color_t>* texture;  ///< Texture image (or nullptr if none).
        
        RGBf color;                     ///< Default color to use when texturing is disabled.

        float ambiant_strength;         ///< Object ambiant coefficient (how much it reflects the ambiant light component). Typical value: 0.2f.
        float diffuse_strength;         ///< Object diffuse coefficient (how much it reflects the diffuse light component). Typical value:  0.7f. 
        float specular_strength;        ///< Object ambiant coefficient (how much it reflects the specular light component). Typical value:  0.5f.
        int specular_exponent;          ///< Specular exponent. 0 to disable specular lightning. Typical value between 4 and 64.

        const Mesh3D * next;            ///< Next object to draw when chaining is enabled. nullptr at end of chain. 
        
        fBox3 bounding_box;             ///< Object bounding box.
        
        const char* name;               ///< Mesh name, or nullptr
        };




    /**
     * Creates a "cache version" of a mesh by copying part of its data into fast memory buffers.
     * 
     * A typical scenario for using this method is when when using a MCU (Teesny, ESP32..) with
     * limited RAM and slow FLASH memory.
     * 
     * The method copy as much as it can into the given RAM1/RAM2 buffers but will leave the arrays
     * that are to big in their current place. The method never 'fails' but it may return the
     * original mesh if caching is not possible.
     * 
     * A C-string describes which arrays should be copied and in which order:
     * 
     * - "V" = vertex array.   
     * - "N" = normal array.   
     * - "T" = texture array.   
     * - "I" = texture image  
     * - "F" = face arrays.
     * 
     * For example "VIT" means: "copy vertex arrays first, then image texture and finally texture
     * coord (if there is still room)".
     * 
     * @remark
     * 1. The memory buffers supplied do **not** need to be be aligned, the method takes care of it.
     * 2. The method also caches the sub-meshes linked after this one.
     *
     * @param  mesh        Pointer to the mesh to cache.
     * @param  ram1_buffer pointer to the first memory buffer (should have the fastest access).
     * @param  ram1_size   size in bytes of the ram1 buffer.
     * @param  ram2_buffer pointer to a second memory buffer, may be nullptr.
     * @param  ram2_size   size in bytes of the ram2 buffer or 0 if not supplied.
     * @param  copy_order  (Optional) c string that describe which array should be copied and in which order.
     * @param  ram1_used   If non-null, the number of bytes consumed in ram1_buffer is put here.
     * @param  ram2_used   If non-null, the number of bytes consumed in ram2_buffer is put here.
     *
     * @returns The cached mesh (or the initial mesh if nothing was cached).
     */
    template<typename color_t> const Mesh3D<color_t> * cacheMesh(const Mesh3D<color_t> * mesh,
                                                           void * ram1_buffer, size_t ram1_size,
                                                           void * ram2_buffer, size_t ram2_size,
                                                           const char * copy_order = "VNTIF",
                                                           size_t * ram1_used = nullptr,
                                                           size_t * ram2_used = nullptr);







#if defined(ARDUINO_TEENSY41)

//******************************************************************************************
//
// TEENSY 4 SPECIFIC METHODS
//
//******************************************************************************************


    /**
     * Create a copy of a mesh where specified arrays in PROGMEM are copied to EXTMEM.
     * 
     * @attention This method is defined only for Teensy 4.1
     * 
     * @remark
     * 1. Obviously, external RAM must be present...
     * 2. Only arrays in PROGMEM are copied to EXTMEM. Arrays located elsewhere are not copied.
     * 3. The source mesh must not have any part located in EXTMEM already or the method will fail.
     * 4. In practice, it is most efficient to copy textures to EXTMEM (the other arrays provide
     * less improvments).
     *
     * @returns a pointer to the new mesh or nullptr on error (nothing is allocated in that case). the
     * methods also copies the sub-meshes linked to this one (via the ->next pointer).  
     */
    template<typename color_t> Mesh3D<color_t>* copyMeshEXTMEM(const Mesh3D<color_t>* mesh,
                                                                bool copy_vertices = false,
                                                                bool copy_normals = false,
                                                                bool copy_texcoords = false,
                                                                bool copy_textures = true,
                                                                bool copy_faces = false);


    /**
     * Delete a mesh allocated with copyMeshEXTMEM().
     * 
     * @attention This method is defined only for Teensy 4.1
     * 
     * Deletes also the linked sub-meshes is any.
     */
    template<typename color_t> void freeMeshEXTMEM(Mesh3D<color_t>* mesh);




//***************************************************************
// NOT YET IMPLEMENTED : TODO...
//***************************************************************


    /**
    * Copy a mesh from the SD to external memory, together with all its dependencies
    * (arrays, texture images and linked meshes).
    */
    //template<typename color_t> Mesh3D<color_t>* copyMeshFromSDToEXTMEM(const char * filename);


#endif 


}



#include "Mesh3D.inl"


#endif

#endif

/** end of file */

