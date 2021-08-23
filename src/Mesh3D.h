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
    *          "chains of triangles". See below to details
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
    * next      pointer to the next mesh to draw.
    * 
    * 
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
    * REMARK: the winding order of the trinagle matters (for face culling)  and 
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
        
        const char* name;                   // mesh name
        };






#if defined(ARDUINO_TEENSY41)


    /**
    * Create a copy of a mesh where (specified) arrays in PROGMEM are copied to EXTMEM.
    * 
    * Of course, external ram must be present...
    *
    * - Only arrays in PROGMEM are copied to EXTMEM. Arrays located elsewhere are never copied. 
    * - The source mesh must not have any part located in extmem already or the method will fail.
    * - In general, it is most helpful to copy textures to extmem but not the other arrays. 
    * 
    * Return a pointer to the new mesh or nullptr on error (nothing is allocated in that case). 
    * the methods also copies the sub-meshes linked to this one (via the ->next pointer).  
    **/
    template<typename color_t> Mesh3D<color_t>* copyMeshEXTMEM(const Mesh3D<color_t>* mesh,
                                                                bool copy_textures = true,
                                                                bool copy_vertices = false,
                                                                bool copy_normals = false,
                                                                bool copy_texcoords = false,
                                                                bool copy_faces = false);


    /**
    * Delete a mesh allocated with copyMeshEXTMEM().
    **/
    template<typename color_t> void freeMeshEXTMEM(Mesh3D<color_t>* mesh);









    /*******************************************************************************************
    *
    * Implementation details
    * 
    ********************************************************************************************/



    /** 
     * Very stupid, unoptimized "map" container to make sure memory is 
     * allocated/freed only once for each object.
     * Algo. inefficient but we dont care here... 
     **/
    class _MapPtr
        {
        public:

            /** ctor, empty container */
            _MapPtr() : _nb(0) {}


            /** 
             * free p if p is not a key in the map 
             * and then add it to the map (with itself as value)
             **/
            inline void free(const void* p)
                {
                if ((p) && (_find(p) == nullptr))
                    {
                    _add(p, (void *)p);
                    extmem_free((void *)p);
                    }
                }

            /** free all value pointers in the map and then clear the map */
            inline void freeAll()
                {
                for (int k = 0; k < _nb; k++)
                    {
                    extmem_free(_vals[k]);
                    }
                _nb = 0; 
                }

            /** 
             * Check if src is a key in the map. 
             * If true, return the mapped pointer.
             * If false, allocate size bytes and add to the map src as key
             * and the allocated adress as value and return the allocated adress. 
             * 
             * if allocation fails, free all cvalue pointer in the map and return nulptr
             **/
            inline void* malloc(const void* src, size_t size)
                {
                if ((src == nullptr) || (size == 0) || (_nb >= MAXPTR))
                    { // error or map full
                    freeAll();
                    return nullptr;
                    }
                void* adr = _find(src);
                if (adr == nullptr)
                    { // not yet in the map, 
                    adr = extmem_malloc(size);
                    if (!TGX_IS_EXTMEM(adr))
                        { // out of memory
                        freeAll();
                        return nullptr;
                        }
                    _add(src, adr);
                    memcpy(adr, src, size);
                    }
                return adr;
                }

        private: 

            /** push a new element, no overflow check ! */
            void _add(const void* key, void * val) 
                { 
                _keys[_nb] = key; 
                _vals[_nb] = val;
                _nb++;
                }

            /** check if a pointer is already registered, return its value or nullptr if not found */
            void * _find(const void* key) const
                {
                for(int k=0; k< _nb; k++)
                    {
                    if (_keys[k] == key) return _vals[k];
                    }
                return nullptr;
                }

        private:

            static const int MAXPTR = 256; // max number of entries

            int         _nb;            // number of entries
            const void* _keys[MAXPTR];  // list of keys
            void*       _vals[MAXPTR];  // list of values
        };





    template<typename color_t> void freeMeshEXTMEM(Mesh3D<color_t>* mesh)
        {
        _MapPtr map;
        while (TGX_IS_EXTMEM(mesh))
            {
            if (TGX_IS_EXTMEM(mesh->vertice)) map.free(mesh->vertice);
            if (TGX_IS_EXTMEM(mesh->texcoord)) map.free(mesh->texcoord);
            if (TGX_IS_EXTMEM(mesh->normal)) map.free(mesh->normal);
            if (TGX_IS_EXTMEM(mesh->face)) map.free(mesh->face);
            if (TGX_IS_EXTMEM(mesh->texture))
                {
                map.free(mesh->texture->data());
                map.free(mesh->texture);
                }       
            Mesh3D<color_t>* m = mesh;
            mesh = (Mesh3D<color_t> * )mesh->next;
            map.free(m);
            }
        }
    


    template<typename color_t> Mesh3D<color_t>* copyMeshEXTMEM(const Mesh3D<color_t>* mesh,
                                                                bool copy_textures,
                                                                bool copy_vertices,
                                                                bool copy_normals,
                                                                bool copy_texcoords,
                                                                bool copy_faces)    
        {
        if (external_psram_size <= 0) return nullptr; // no extram present. 
        if ((mesh == nullptr) || (TGX_IS_EXTMEM(mesh))) return nullptr; // not a valid mesh
        _MapPtr map;
        
        Mesh3D<color_t>* new_mesh = (Mesh3D<color_t> * )map.malloc(mesh, sizeof(Mesh3D<color_t>));
        if (new_mesh == nullptr) return nullptr;
        Mesh3D<color_t>* cur_mesh = new_mesh;   
        while (1)
            {
            if (TGX_IS_EXTMEM(mesh->vertice)) { map.freeAll(); return nullptr; }
            if ((copy_vertices) && (mesh->nb_vertices > 0) && (TGX_IS_PROGMEM(mesh->vertice)))
                {
                fVec3 *p = (fVec3 *)map.malloc(mesh->vertice, sizeof(fVec3) * mesh->nb_vertices);
                if (p == nullptr) return nullptr;
                cur_mesh->vertice = p;
                }
            
            if (TGX_IS_EXTMEM(mesh->texcoord)) { map.freeAll(); return nullptr; }
            if ((copy_texcoords) && (mesh->nb_texcoords > 0) && (TGX_IS_PROGMEM(mesh->texcoord)))
                {
                fVec2 * p = (fVec2 *)map.malloc(mesh->texcoord, sizeof(fVec2) * mesh->nb_texcoords);
                if (p == nullptr) return nullptr;
                cur_mesh->texcoord = p;
                }
            
            if (TGX_IS_EXTMEM(mesh->normal)) { map.freeAll(); return nullptr; }
            if ((copy_normals) && (mesh->nb_normals > 0) && (TGX_IS_PROGMEM(mesh->normal)))
                {
                fVec3* p = (fVec3 *)map.malloc(mesh->normal, sizeof(fVec3) * mesh->nb_normals);
                if (p == nullptr) return nullptr;
                cur_mesh->normal = p;
                }
                    
            if (TGX_IS_EXTMEM(mesh->face)) { map.freeAll(); return nullptr; }
            if ((copy_faces) && (mesh->nb_faces > 0) && (TGX_IS_PROGMEM(mesh->face)))
                {
                uint16_t * p = (uint16_t *)map.malloc(mesh->face, mesh->len_face*sizeof(uint16_t));
                if (p == nullptr) return nullptr;
                cur_mesh->face = p;
                }
            
            if (TGX_IS_EXTMEM(mesh->texture)) { map.freeAll(); return nullptr; }
            if ((copy_textures) && (mesh->texture))
                {
                if (mesh->texture->isValid())
                    {
                    if (TGX_IS_EXTMEM(mesh->texture->data())) { map.freeAll(); return nullptr; }
                    if (TGX_IS_PROGMEM(mesh->texture->data()))
                        {
                        int imlen = mesh->texture->stride() * mesh->texture->height();
                        color_t* imdata = (color_t*)map.malloc(mesh->texture->data(), imlen * sizeof(color_t));
                        if (imdata == nullptr) return nullptr;
                        Image<color_t>* imp = (Image<color_t>*)map.malloc(mesh->texture, sizeof(Image<color_t>)); // dirty, should use placement new...
                        if (imp == nullptr) return nullptr;
                        imp->set(imdata, { mesh->texture->width() , mesh->texture->height() }, mesh->texture->stride());
                        cur_mesh->texture = imp;
                        }
                    }
                }

            if (mesh->next == nullptr) return new_mesh;
            if (TGX_IS_EXTMEM(mesh->next)) { map.freeAll(); return nullptr; }
            Mesh3D<color_t>* next_cur_mesh = (Mesh3D<color_t>*)map.malloc(mesh->next, sizeof(Mesh3D<color_t>));
            if (next_cur_mesh == nullptr) return nullptr;
            cur_mesh->next = next_cur_mesh;
            cur_mesh = next_cur_mesh;
            mesh = mesh->next;
            }
        return new_mesh; 
        }




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


#endif

#endif

/** end of file */

