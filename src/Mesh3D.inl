/** @file Mesh3D.inl */
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
#ifndef _TGX_MESH3D_INL_
#define _TGX_MESH3D_INL_


namespace tgx
    {

    /*******************************************************************************************
   *
   * Implementation details
   *
   ********************************************************************************************/


    namespace tgx_internals
    {

        class CacheMesh
        {




        public:

            /** ctor. */
            CacheMesh(void* ram1_ptr, size_t ram1_size, char* ram2_ptr, size_t ram2_size)
                : _ram1_size(ram1_size), _ram2_size(ram2_size), _ram1_ptr((char*)ram1_ptr), _ram2_ptr(ram2_ptr), _nb_entries(0)
            {
                align(_ram1_ptr, _ram1_size);
                align(_ram2_ptr, _ram2_size);
            }

            size_t remaining_ram1() const { return _ram1_size; }

            size_t remaining_ram2() const { return _ram2_size; }   

            const fVec3* cache_vertice(size_t size, const fVec3* va) { return (const fVec3*)cache_array(size * sizeof(fVec3), (const char*)va); }

            const fVec3* cache_normal(size_t size, const fVec3* vn) { return (const fVec3*)cache_array(size * sizeof(fVec3), (const char*)vn); }

            const fVec2* cache_texcoord(size_t size, const fVec2* vt) { return (const fVec2*)cache_array(size * sizeof(fVec2), (const char*)vt); }

            const uint16_t* cache_face(size_t size, const uint16_t* f) { return (const uint16_t*)cache_array(size * sizeof(uint16_t), (const char*)f); }

            template<typename color_t> const Image<color_t>* cache_image(const Image<color_t>* im)
                {
                if (im == nullptr) return nullptr;
                char* p = find_in_cache((const char*)im);
                if (p != nullptr) return ((const Image<color_t>*)p);

                const size_t image_size = sizeof(Image<color_t>);
                const size_t data_size = (im->stride() * im->ly() * sizeof(color_t));

                if (!can_alloc(image_size + data_size)) return im;

                Image<color_t>* new_im = (Image<color_t>*)alloc(image_size);
                memcpy(new_im, im, image_size);

                color_t * data = (color_t*)alloc(data_size);
                memcpy(data, im->data(), data_size);

                new_im->set((color_t*)data, im->lx(), im->ly(), im->stride());

                add_in_cache((const char*)im, (char*)new_im);
                return new_im;
                }


            template<typename color_t> Mesh3D<color_t>* cash_mesh_recurse(const Mesh3D<color_t>* mesh)
                {
                if (mesh == nullptr) return nullptr;
                if (!can_alloc(sizeof(Mesh3D<color_t>))) return nullptr;

                Mesh3D<color_t>* new_mesh = (Mesh3D<color_t>*)alloc(sizeof(Mesh3D<color_t>));
                memcpy(new_mesh, mesh, sizeof(Mesh3D<color_t>));

                Mesh3D<color_t>* m = new_mesh;
                while (m->next != nullptr)
                    {
                    if (!can_alloc(sizeof(Mesh3D<color_t>))) return nullptr;
                    Mesh3D<color_t>* mm = (Mesh3D<color_t>*)alloc(sizeof(Mesh3D<color_t>));
                    memcpy(mm, m->next, sizeof(Mesh3D<color_t>));
                    m->next = mm;
                    m = mm;
                    }
                return new_mesh;
                }

        private:




            /** align pointer on 4 bytes boundary and adjust remaining size */
            void align(char*& ptr, size_t& space)
            {
                if (space < 4) return; // cannot do nothing. 
                // ugly hack, should change this to use std::align at some point...
                const size_t adr = (size_t)(ptr);
                const size_t r = 4 - (adr % 4);
                if (r != 4)
                {
                    ptr += r;
                    space -= r;
                }
            }

            /** return true if it is possible to allocate size consecutive bytes */
            bool can_alloc(size_t size)
            {
                return ((size + 8 < _ram1_size) || (size + 8 < _ram2_size));
            }


            /** Allocates size bytes, try ram1 first and then ram2 */
            char* alloc(size_t size)
            {
                if (size < _ram1_size)
                {
                    char* ptr = _ram1_ptr;
                    _ram1_ptr += size;
                    _ram1_size -= size;
                    align(_ram1_ptr, _ram1_size);
                    return ptr;
                }
                if (size < _ram2_size)
                {
                    char* ptr = _ram2_ptr;
                    _ram2_ptr += size;
                    _ram2_size -= size;
                    align(_ram2_ptr, _ram2_size);
                    return ptr;
                }
                return nullptr;
            }


            /** return the cache version of an array if present or nullptr otherwise */
            char* find_in_cache(const char* ptr)
            {
                for (int i = 0; i < _nb_entries; i++)
                {
                    if (_keys[i] == ptr) return _values[i];
                }
                return nullptr;
            }


            /** register a cache version of an array */
            bool add_in_cache(const char* ptr_src, char* ptr_cached)
            {
                if (_nb_entries == MAX_NB_KEYS) return false;
                _keys[_nb_entries] = ptr_src;
                _values[_nb_entries] = ptr_cached;
                _nb_entries++;
                return true;
            }

            /** perform caching of an array if possible */
            const char* cache_array(size_t size, const char* src_ptr)
            {
                if ((src_ptr == nullptr) || (size == 0)) return nullptr;
                char* p = find_in_cache(src_ptr);
                if (p != nullptr) return p;
                if (!can_alloc(size)) return src_ptr;
                p = alloc(size);
                memcpy(p, src_ptr, size);
                add_in_cache(src_ptr, p);
                return p;
            }


            size_t          _ram1_size;
            size_t          _ram2_size;
            char*           _ram1_ptr;
            char*           _ram2_ptr;

            static  const int MAX_NB_KEYS = 256;

            int             _nb_entries;
            const char*     _keys[MAX_NB_KEYS];
            char*           _values[MAX_NB_KEYS];


        };


    }


    template<typename color_t> const Mesh3D<color_t> * cacheMesh(const Mesh3D<color_t>* mesh,
                                                           void* ram1_buffer, size_t ram1_size,
                                                           void* ram2_buffer, size_t ram2_size,
                                                           const char* copy_order,
                                                           size_t* ram1_used,
                                                           size_t* ram2_used)
        {
        tgx_internals::CacheMesh CM((char*)ram1_buffer, ram1_size, (char*)ram2_buffer, ram2_size);

        Mesh3D<color_t> * new_mesh = CM.cash_mesh_recurse(mesh);
        if (new_mesh == nullptr)
            {
            if (ram1_used) { *ram1_used = 0; }
            if (ram1_used) { *ram2_used = 0; }
            return mesh;
            }

        for (int k= 0; copy_order[k] != 0; k++)
            {
            const char c = copy_order[k]; 
            Mesh3D<color_t> * m = new_mesh;
            while (m != nullptr)
                {
                switch (c)
                    {
                case 'V':
                case 'v':
                    m->vertice = CM.cache_vertice(m->nb_vertices, m->vertice);
                    break;

                case 'N':
                case 'n':
                    m->normal = CM.cache_normal(m->nb_normals, m->normal);
                    break;

                case 'T':
                case 't':
                    m->texcoord = CM.cache_texcoord(m->nb_texcoords, m->texcoord);
                    break;

                case 'I':
                case 'i':
                    m->texture = CM.cache_image(m->texture);
                    break;

                case 'F':
                case 'f':
                    m->face = CM.cache_face(m->len_face, m->face);
                    break;
                    }
                m = const_cast<tgx::Mesh3D<color_t>*>(m->next); // valid because we know this mesh is in a ram buffer. 
                }
            }

        if (ram1_used) { *ram1_used = ram1_size - CM.remaining_ram1(); }
        if (ram2_used) { *ram2_used = ram2_size - CM.remaining_ram2(); }

        return new_mesh;
        }





#if defined(ARDUINO_TEENSY41)



   


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






#endif











}


#endif

/* end of file */

