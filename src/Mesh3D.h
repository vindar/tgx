/** @file Mesh3D.h */
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
    * Structure containing the information about a 3D mesh
    *
    * FORMAT:
    *
    * vertice   the array of vertices in (x,y,z) fVec3 format.
    *           a vertex is refered by its index in this array
    *           -> maximum number of vertice: 32767
    *
    * texcoord  the array of texture coordinate in (u,v) fVec2 format
    *           a texture coord is refered by its index in this array.
    *           Set to nullptr (and nb_texcoords=0) if no texture are 
    *           used with this model.
    *           -> maximum number of texture coords: 65535
    *
    * normal   array of normal vectors in (x,y,z) fVec3 format
    *          a normal vector is refered by its index in this array.
    *          Set to nullptr if no normal vectors are defined for this 
    *          model (so only flat shading will be available). 
    *          -> maximum number of normal vectors: 65535
    *          THE NORMALS MUST HAVE UNIT NORM !
    *
    * face     The array of triangular face.
    *          The array is composed of uint16_t and is divided in
    *          "chains of triangles". See below for details
    *
    * texture  the texture image associated with the model (if any).
    *          nullptr if texture is not present/used currently (ok
    *          even if the texcoord array exists).
    *
    * color    the object color when texturing is not used
    * 
    * ambiant_strength      reflection factor for ambiant lightning.
    .
    * diffuse_strength      reflection factor for diffuse lightning.
    * 
    * specular_strength     reflection factor for specular lightning.
    * 
    * specular_exponent     specular lightning component.
    *
    * next      pointer to the next mesh to draw. nullptr if none.
    * 
    * bounding_box  model bounding box.
    * 
    * name  name of the model (optional, nullptr if none). 
    * 
    * DESCRIPTION OF THE FACE ARRAY
    * 
    * the array is composed of "chains" of triangles. Each chain starts with a 
    * uint16_t that specifies its length and is followed by a sequence of elements
    * where each element occupies 1, 2 or 3 uint16_t. So the array takes the following 
    * general form:
    * 
    * [chain 1 length = n] [elem 1] [elem 2] [elem 3]  ... [elem n+2]
    * [chain 2 length = m] [elem 1] [elem 2] [elem 3]  ... [elem m+2]
    *       ...
    * [chain k length = p] [elem 1] [elem 2] [elem 3]  ... [elem p+2]
    * [endtag = 0]
    *
    * - An endtag (int16_t)0 is used as a sentinel to mark the end of the array.
    *
    * - A chain starts with a single uint16_t [chain length] that specify
    * the number of triangles in the chain. It is followed  by [chain length]+2
    * "elements".
    *
    * A "element" represent a vertex of a triangle and occupies 1, 2 or 3 uint16_t
    * depending on wether the  textcoord and normal arrays exist. The first uint16_t
    * of an element is the vertex index (with the direction bit), possibly followed
    * by the texture index (if texture array is present) and then finally followed by
    * the normal index (if a normal array is present) i.e an element as the form:
    *
    *                         (if texture set) (if normal set)
    *    1bit     15bits          (16bits)        (16bits)
    *   [DBIT| VERTEX INDEX]  [TEXTURE INDEX]  [NORMAL INDEX]
    *
    * the highest bit of the first uint16_t is called the direction bit DBIT.
    * Thus, a vertex index is stored on only 15 bit which is why the vertex array
    * cannot contain more than 32767 indices.
    *
    * The first 3 elements of a chain represent the 3 vertices or the starting
    * triangle of the chain. Their DBIT is always 0. Each subsequent element
    * is combined with the current triangle to deduce the next triangle in the
    * following way.
    *
    * Say the current triangle is [V1, V2, V3] and the next element is DBIT|V4:
    *
    * if DBIT = 0, then the next triangle is [V1, V3, V4]
    * if DBIT = 1, then the next triangle is [V3, V2, V4]
    *
    * REMARK: the winding order of the triangle matters (for face culling)  and 
    * that is what the DBIT is for.
    * 
    * EXAMPLE: We assume that texcoord = nullptr but normal != nullptr so that
    * each element is coded on 2 uint16_t: the DBIT and vertex index on the first
    * uint16_t and the normal index on the second uint16_t.
    *
    * Let us consider the following face array composed of 19 uint16_t:
    *
    * face = {3,0,1,2,2,4,6,32773,8,7,7,1,8,7,9,4,5,5,0}
    *
    * It should be read as
    *
    * 3               (the first chain has 3 triangles)
    * 0/1   2/2  4/6  (first triangle with vertex 0,2,4 and normals 1,2,6)
    * 5/8             (32773 = 32768 + 5 so DBIT = 1, second triangle)
    * 7/7             (third triangle DBIT = 0)
    * 1               (the second chain has a single triangle)
    * 8/7   9/4  5/5  (the triangle of the second chain)
    * 0               (end tag)
    *
    * after decoding, this gives a list of 4 triangles
    * (with normals associated with each vertex index).
    *
    * 0/1   2/2  4/6
    * 4/6   2/2  5/8
    * 4/6   5/8  7/7
    * 8/7   9/4  5/5
    *
    **/
    template<typename color_t> 
    struct Mesh3D
        {
        // make sure right away that the template parameter is admissible to prevent cryptic error message later.  
        static_assert(is_color<color_t>::value, "color_t must be one of the color types defined in color.h");

        int32_t id;                         // just in case (set to 1 for the time being). 

        uint16_t nb_vertices;               // number of vertices in the vertex array
        uint16_t nb_texcoords;              // number of texture coordinates in the texcoord array
        uint16_t nb_normals;                // number of normal vectors in the normal array
        uint16_t nb_faces;                  // number of triangular faces in the mesh
        uint16_t len_face;                  // number of elements (uint16_t) in the face array

        const fVec3* vertice;               // vertex array pointer
        const fVec2* texcoord;              // texture coord array pointer (nullptr if none)
        const fVec3* normal;                // normal vector array pointer (nullptr if none). THE VECTOR MUST HAVE UNIT NORM !
        const uint16_t* face;               // array of triangles pointer. format described in docstring above.

        const Image<color_t>* texture;      // the texture image (or nullptr if none)

        RGBf color;                         // default color used when texturing is disabled.

        float ambiant_strength;             // the object ambiant factor (how much it reflects the ambiant light component) typ. value: 0.2sf 
        float diffuse_strength;             // the object diffuse factor (how much it reflects the diffuse light component) typ. value  0.7f 
        float specular_strength;            // the object ambiant factor (how much it reflects the specular light component) typ value 0.5f
        int specular_exponent;              // specular exponent. 0 to disable specular lightning, typically between 4 and 64.

        const Mesh3D * next;                // next object to draw when chaining is enabled. nullptr at end of chain. 
        
        fBox3 bounding_box;                 // object bounding box.
        
        const char* name;                   // mesh name, or nullptr
        };




    /**
     * Helper method to create a "cache version" of a mesh by copying part of its data into fast
     * memory buffers. A typical scenario for using this method is when when using a MCU with
     * limited RAM and slow FLASH memory.
     * 
     * The method copy as much as it can into the given RAM1/RAM2 buffers but will leave the arrays
     * that are to big in their current place. The method never 'fails' but it may return the
     * original mesh if not caching is possible.
     * 
     * A C-string describe which arrays should be copied and in which order:
     * 
     * "V" = vertex array. 
     * "N" = normal array. 
     * "T" = texture array. 
     * "I" = texture image  
     * "F" = face arrays.
     * 
     * For example "VIT" means copy vertex arrays first, then image texture and finally texture
     * coord (if there is still room).
     * 
     * REMARKS: 
     *   1. the memory buffers supplied do NOT have to be be aligned, the method takes care of it.
     * 
     *   2. The method also caches the sub-meshes linked after this one.
     * 
     * @param           mesh        Pointer to the mesh to cache.
     * @param [in,out]  ram1_buffer pointer to the first memory buffer (should have the fastest access)
     * @param           ram1_size   size in bytes of the ram1 buffer.
     * @param [in,out]  ram2_buffer pointer to a second memory buffer, may be nullptr.
     * @param           ram2_size   size in bytes of the ram2 buffer or 0 if not supplied.
     * @param           copy_order  c string that describe which array should be copied and in which order
     * @param [in,out]  ram1_used   If non-null, the number of bytes ultimately used in ram1_buffer is put here. 
     * @param [in,out]  ram2_used   If non-null, the number of bytes ultimately used in ram2_buffer is put here. 
     *
     * @returns The cached mesh (or the initial mesh if nothing was cached).
    **/
    template<typename color_t> const Mesh3D<color_t> * cacheMesh(const Mesh3D<color_t> * mesh,
                                                           void * ram1_buffer, size_t ram1_size,
                                                           void * ram2_buffer, size_t ram2_size,
                                                           const char * copy_order = "VNTIF",
                                                           size_t * ram1_used = nullptr,
                                                           size_t * ram2_used = nullptr);







#if defined(ARDUINO_TEENSY41)

    /******************************************************************************************
    *
    * TEENSY 4 SPECIFIC METHODS
    *
    *******************************************************************************************/


    /**
    * Create a copy of a mesh where specified arrays in PROGMEM are copied to EXTMEM.
    * 
    * Of course, external ram must be present...
    *
    * - Only arrays in PROGMEM are copied to EXTMEM. Arrays located elsewhere are never copied. 
    * - The source mesh must not have any part located in extmem already or the method will fail.
    * - In practice, it is most helpful to copy textures to extmem (but not the other arrays). 
    * 
    * Return a pointer to the new mesh or nullptr on error (nothing is allocated in that case). 
    * the methods also copies the sub-meshes linked to this one (via the ->next pointer).  
    **/
    template<typename color_t> Mesh3D<color_t>* copyMeshEXTMEM(const Mesh3D<color_t>* mesh,
                                                                bool copy_vertices = false,
                                                                bool copy_normals = false,
                                                                bool copy_texcoords = false,
                                                                bool copy_textures = true,
                                                                bool copy_faces = false);


    /**
    * Delete a mesh allocated with copyMeshEXTMEM().
    **/
    template<typename color_t> void freeMeshEXTMEM(Mesh3D<color_t>* mesh);




    /***************************************************************
    * NOT YET IMPLEMENTED : TODO...
    ****************************************************************/

    /**
    * Copy a mesh from SD to external memory, together with all its dependencies
    * (arrays, texture images and linked meshes).
    **/
    //template<typename color_t> Mesh3D<color_t>* copyMeshFromSDToEXTMEM(const char * filename);


#endif 


}



#include "Mesh3D.inl"


#endif

#endif

/** end of file */

