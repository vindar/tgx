/** @file Mesh3Dv2.inl */
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
#ifndef _TGX_MESH3DV2_INL_
#define _TGX_MESH3DV2_INL_


namespace tgx
    {


    namespace Mesh3Dv2_detail
        {

        /** Decode one local quantized vertex from a meshlet payload. */
        TGX_FORCED_INLINE fVec3 load_vertex(const int16_t* table, uint8_t index, float base_x, float base_y, float base_z, float scale)
            {
            const int16_t* const q = table + ((size_t)index) * 3;
#if TGX_USE_FMA_MATH
            return fVec3(fmaf((float)q[0], scale, base_x),
                         fmaf((float)q[1], scale, base_y),
                         fmaf((float)q[2], scale, base_z));
#else
            return fVec3(base_x + ((float)q[0]) * scale,
                         base_y + ((float)q[1]) * scale,
                         base_z + ((float)q[2]) * scale);
#endif
            }


        /** Decode one local quantized vertex directly in view coordinates. */
        TGX_FORCED_INLINE fVec4 load_vertex_transformed(const int16_t* table, uint8_t index,
                                                        const fVec4& base,
                                                        const fVec4& sx,
                                                        const fVec4& sy,
                                                        const fVec4& sz)
            {
            const int16_t* const q = table + ((size_t)index) * 3;
            const float qx = (float)q[0];
            const float qy = (float)q[1];
            const float qz = (float)q[2];
#if TGX_USE_FMA_MATH
            return fVec4(fmaf(qx, sx.x, fmaf(qy, sy.x, fmaf(qz, sz.x, base.x))),
                         fmaf(qx, sx.y, fmaf(qy, sy.y, fmaf(qz, sz.y, base.y))),
                         fmaf(qx, sx.z, fmaf(qy, sy.z, fmaf(qz, sz.z, base.z))),
                         fmaf(qx, sx.w, fmaf(qy, sy.w, fmaf(qz, sz.w, base.w))));
#else
            return fVec4(base.x + qx * sx.x + qy * sy.x + qz * sz.x,
                         base.y + qx * sx.y + qy * sy.y + qz * sz.y,
                         base.z + qx * sx.z + qy * sy.z + qz * sz.z,
                         base.w + qx * sx.w + qy * sy.w + qz * sz.w);
#endif
            }


        /** Decode one local signed-normalized normal from a meshlet payload. */
        TGX_FORCED_INLINE fVec3 load_normal(const int16_t* table, uint8_t index, float scale)
            {
            const int16_t* const q = table + ((size_t)index) * 3;
            return fVec3(((float)q[0]) * scale,
                         ((float)q[1]) * scale,
                         ((float)q[2]) * scale);
            }


        /** Decode one local signed-normalized normal directly in view coordinates. */
        TGX_FORCED_INLINE fVec4 load_normal_transformed(const int16_t* table, uint8_t index,
                                                        const fVec4& sx,
                                                        const fVec4& sy,
                                                        const fVec4& sz)
            {
            const int16_t* const q = table + ((size_t)index) * 3;
            const float qx = (float)q[0];
            const float qy = (float)q[1];
            const float qz = (float)q[2];
#if TGX_USE_FMA_MATH
            return fVec4(fmaf(qx, sx.x, fmaf(qy, sy.x, qz * sz.x)),
                         fmaf(qx, sx.y, fmaf(qy, sy.y, qz * sz.y)),
                         fmaf(qx, sx.z, fmaf(qy, sy.z, qz * sz.z)),
                         fmaf(qx, sx.w, fmaf(qy, sy.w, qz * sz.w)));
#else
            return fVec4(qx * sx.x + qy * sy.x + qz * sz.x,
                         qx * sx.y + qy * sy.y + qz * sz.y,
                         qx * sx.z + qy * sy.z + qz * sz.z,
                         qx * sx.w + qy * sy.w + qz * sz.w);
#endif
            }


        /** Decode one local texture coordinate from a meshlet payload. */
        TGX_FORCED_INLINE fVec2 load_texcoord(const int16_t* table, uint8_t index, float scale)
            {
            const int16_t* const q = table + ((size_t)index) * 2;
            return fVec2(((float)q[0]) * scale,
                         ((float)q[1]) * scale);
            }


        /** Align a byte pointer on a 4-byte boundary and adjust the remaining space. */
        inline void cache_align4(char*& ptr, size_t& space)
            {
            if (space < 4) return;
            const size_t adr = (size_t)(ptr);
            const size_t r = 4 - (adr % 4);
            if (r != 4)
                {
                ptr += r;
                space -= r;
                }
            }


        template<typename color_t>
        class CacheMesh3Dv2
            {
        public:

            CacheMesh3Dv2(void* ram1_ptr, size_t ram1_size, void* ram2_ptr, size_t ram2_size)
                : _ram1_size(ram1_size), _ram2_size(ram2_size), _ram1_ptr((char*)ram1_ptr), _ram2_ptr((char*)ram2_ptr), _nb_entries(0)
                {
                if (!_ram1_ptr) _ram1_size = 0;
                cache_align4(_ram1_ptr, _ram1_size);
                if (!_ram2_ptr) _ram2_size = 0;
                cache_align4(_ram2_ptr, _ram2_size);
                }

            size_t remaining_ram1() const { return _ram1_size; }

            size_t remaining_ram2() const { return _ram2_size; }

            Mesh3Dv2<color_t>* cache_mesh(const Mesh3Dv2<color_t>* mesh)
                {
                if (mesh == nullptr) return nullptr;
                if (!can_alloc(sizeof(Mesh3Dv2<color_t>))) return nullptr;

                Mesh3Dv2<color_t>* new_mesh = (Mesh3Dv2<color_t>*)alloc(sizeof(Mesh3Dv2<color_t>));
                memcpy(new_mesh, mesh, sizeof(Mesh3Dv2<color_t>));
                return new_mesh;
                }

            const uint32_t* cache_payload(size_t size_bytes, const uint32_t* payload)
                {
                return (const uint32_t*)cache_array(size_bytes, (const char*)payload);
                }

            const Meshlet3Dv2* cache_meshlets(uint16_t nb_meshlets, const Meshlet3Dv2* meshlets)
                {
                return (const Meshlet3Dv2*)cache_array(((size_t)nb_meshlets) * sizeof(Meshlet3Dv2), (const char*)meshlets);
                }

            const MeshMaterial3Dv2<color_t>* cache_materials(uint16_t nb_materials, const MeshMaterial3Dv2<color_t>* materials)
                {
                return (const MeshMaterial3Dv2<color_t>*)cache_array(((size_t)nb_materials) * sizeof(MeshMaterial3Dv2<color_t>), (const char*)materials);
                }

            const MeshMaterialExtra3Dv2<color_t>* cache_material_extras(uint16_t nb_materials, const MeshMaterialExtra3Dv2<color_t>* material_extras)
                {
                return (const MeshMaterialExtra3Dv2<color_t>*)cache_array(((size_t)nb_materials) * sizeof(MeshMaterialExtra3Dv2<color_t>), (const char*)material_extras);
                }

            const Image<color_t>* cache_image(const Image<color_t>* im)
                {
                if (im == nullptr) return nullptr;
                char* p = find_in_cache((const char*)im);
                if (p != nullptr) return ((const Image<color_t>*)p);

                const size_t image_size = sizeof(Image<color_t>);
                const size_t data_size = ((size_t)im->stride()) * ((size_t)im->ly()) * sizeof(color_t);

                if (!can_alloc(image_size + data_size)) return im;

                Image<color_t>* new_im = (Image<color_t>*)alloc(image_size);
                memcpy(new_im, im, image_size);

                color_t* data = (color_t*)alloc(data_size);
                memcpy(data, im->data(), data_size);

                new_im->set(data, im->lx(), im->ly(), im->stride());

                add_in_cache((const char*)im, (char*)new_im);
                return new_im;
                }

        private:

            bool can_alloc(size_t size) const
                {
                return ((size + 8 < _ram1_size) || (size + 8 < _ram2_size));
                }

            char* alloc(size_t size)
                {
                if (size < _ram1_size)
                    {
                    char* ptr = _ram1_ptr;
                    _ram1_ptr += size;
                    _ram1_size -= size;
                    cache_align4(_ram1_ptr, _ram1_size);
                    return ptr;
                    }
                if (size < _ram2_size)
                    {
                    char* ptr = _ram2_ptr;
                    _ram2_ptr += size;
                    _ram2_size -= size;
                    cache_align4(_ram2_ptr, _ram2_size);
                    return ptr;
                    }
                return nullptr;
                }

            char* find_in_cache(const char* ptr) const
                {
                for (int i = 0; i < _nb_entries; i++)
                    {
                    if (_keys[i] == ptr) return _values[i];
                    }
                return nullptr;
                }

            bool add_in_cache(const char* ptr_src, char* ptr_cached)
                {
                if (_nb_entries == MAX_NB_KEYS) return false;
                _keys[_nb_entries] = ptr_src;
                _values[_nb_entries] = ptr_cached;
                _nb_entries++;
                return true;
                }

            const char* cache_array(size_t size, const char* src_ptr)
                {
                if (src_ptr == nullptr) return nullptr;
                if (size == 0) return src_ptr;
                char* p = find_in_cache(src_ptr);
                if (p != nullptr) return p;
                if (!can_alloc(size)) return src_ptr;
                p = alloc(size);
                memcpy(p, src_ptr, size);
                add_in_cache(src_ptr, p);
                return p;
                }

            size_t      _ram1_size;
            size_t      _ram2_size;
            char*       _ram1_ptr;
            char*       _ram2_ptr;

            // payload + meshlet array + material arrays + diffuse/emissive texture objects.
            static const int MAX_NB_KEYS = 640;

            int         _nb_entries;
            const char* _keys[MAX_NB_KEYS];
            char*       _values[MAX_NB_KEYS];
            };


        inline size_t round_up4(size_t value)
            {
            return (value + 3) & ~(size_t)3;
            }


        inline size_t face_stream_size_bytes(const uint8_t* face, bool has_texcoords, bool has_normals)
            {
            const uint8_t* pos = face;
            const size_t elem_size = 1 + (has_texcoords ? 1 : 0) + (has_normals ? 1 : 0);
            uint8_t nbt;
            while ((nbt = *(pos++)) > 0)
                {
                pos += (((size_t)nbt) + 2) * elem_size;
                }
            return (size_t)(pos - face);
            }


        inline size_t meshlet_payload_size_bytes(const Meshlet3Dv2& meshlet, const uint8_t* base)
            {
            const uint8_t* pos = base;
            pos += ((size_t)meshlet.nb_vertices) * 3 * sizeof(int16_t);
            pos += ((size_t)meshlet.nb_normals) * 3 * sizeof(int16_t);
            pos += ((size_t)meshlet.nb_texcoords) * 2 * sizeof(int16_t);
            pos += face_stream_size_bytes(pos, meshlet.nb_texcoords != 0, meshlet.nb_normals != 0);
            return round_up4((size_t)(pos - base));
            }


        template<typename color_t>
        size_t payload_size_bytes(const Mesh3Dv2<color_t>* mesh)
            {
            if ((mesh == nullptr) || (mesh->payload == nullptr) || (mesh->meshlets == nullptr) || (mesh->nb_meshlets == 0)) return 0;

            const Meshlet3Dv2& last = mesh->meshlets[mesh->nb_meshlets - 1];
            const size_t last_offset = ((size_t)last.payload_offset32) * sizeof(uint32_t);
            const uint8_t* const payload = (const uint8_t*)mesh->payload;
            return last_offset + meshlet_payload_size_bytes(last, payload + last_offset);
            }


        template<typename color_t>
        bool has_emissive_textures(const Mesh3Dv2<color_t>* mesh)
            {
            if ((mesh == nullptr) || (mesh->material_extras == nullptr)) return false;
            for (uint16_t i = 0; i < mesh->nb_materials; i++)
                {
                if (mesh->material_extras[i].emissive_texture != nullptr) return true;
                }
            return false;
            }


#if defined(ARDUINO_TEENSY41)

        class ExtMemMap
            {
        public:

            ExtMemMap() : _nb(0), _used(0) {}

            size_t used() const { return _used; }

            bool contains(const void* p) const
                {
                return (p != nullptr) && (_find(p) != nullptr);
                }

            void free(const void* p)
                {
                if ((p != nullptr) && (_find(p) == nullptr))
                    {
                    if (_nb >= MAXPTR) return;
                    _add(p, (void*)p, 0);
                    extmem_free((void*)p);
                    }
                }

            void freeAll()
                {
                for (int k = 0; k < _nb; k++)
                    {
                    extmem_free(_vals[k]);
                    }
                _nb = 0;
                _used = 0;
                }

            void* malloc(const void* src, size_t size)
                {
                if ((src == nullptr) || (size == 0) || (_nb >= MAXPTR))
                    {
                    freeAll();
                    return nullptr;
                    }

                void* adr = _find(src);
                if (adr == nullptr)
                    {
                    adr = extmem_malloc(size);
                    if (!TGX_IS_EXTMEM(adr))
                        {
                        freeAll();
                        return nullptr;
                        }
                    _add(src, adr, size);
                    memcpy(adr, src, size);
                    _used += size;
                    }
                return adr;
                }

        private:

            void _add(const void* key, void* val, size_t)
                {
                _keys[_nb] = key;
                _vals[_nb] = val;
                _nb++;
                }

            void* _find(const void* key) const
                {
                for (int k = 0; k < _nb; k++)
                    {
                    if (_keys[k] == key) return _vals[k];
                    }
                return nullptr;
                }

            static const int MAXPTR = 1100;

            int         _nb;
            size_t      _used;
            const void* _keys[MAXPTR];
            void*       _vals[MAXPTR];
            };


        template<typename color_t>
        const Image<color_t>* copy_image_extmem(ExtMemMap& map, const Image<color_t>* im)
            {
            if (im == nullptr) return nullptr;
            if (im->isValid() && TGX_IS_PROGMEM(im->data()))
                {
                const size_t data_size = ((size_t)im->stride()) * ((size_t)im->ly()) * sizeof(color_t);
                color_t* imdata = (color_t*)map.malloc(im->data(), data_size);
                if (imdata == nullptr) return nullptr;

                Image<color_t>* imp = (Image<color_t>*)map.malloc(im, sizeof(Image<color_t>));
                if (imp == nullptr) return nullptr;
                imp->set(imdata, im->lx(), im->ly(), im->stride());
                return imp;
                }
            return im;
            }


        template<typename color_t>
        void free_image_extmem(ExtMemMap& map, const Image<color_t>* im)
            {
            if ((im == nullptr) || (!TGX_IS_EXTMEM(im)) || map.contains(im)) return;
            if (TGX_IS_EXTMEM(im->data())) map.free(im->data());
            map.free(im);
            }

#endif

        }


    template<typename color_t> const Mesh3Dv2<color_t>* cacheMesh(const Mesh3Dv2<color_t>* mesh,
                                                                  void* ram1_buffer, size_t ram1_size,
                                                                  void* ram2_buffer, size_t ram2_size,
                                                                  const char* copy_order,
                                                                  size_t* ram1_used,
                                                                  size_t* ram2_used)
        {
        Mesh3Dv2_detail::CacheMesh3Dv2<color_t> CM(ram1_buffer, ram1_size, ram2_buffer, ram2_size);

        Mesh3Dv2<color_t>* new_mesh = CM.cache_mesh(mesh);
        if (new_mesh == nullptr)
            {
            if (ram1_used) { *ram1_used = 0; }
            if (ram2_used) { *ram2_used = 0; }
            return mesh;
            }

        if (copy_order == nullptr) copy_order = "";

        const size_t payload_size = Mesh3Dv2_detail::payload_size_bytes(mesh);
        const bool has_emissive_textures = Mesh3Dv2_detail::has_emissive_textures(mesh);
        bool material_writable = (new_mesh->materials != mesh->materials);
        bool material_extras_writable = (new_mesh->material_extras == nullptr) || (new_mesh->material_extras != mesh->material_extras);

        for (int k = 0; copy_order[k] != 0; k++)
            {
            const char c = copy_order[k];
            switch (c)
                {
            case 'P':
            case 'p':
                if (new_mesh->payload == mesh->payload)
                    {
                    new_mesh->payload = CM.cache_payload(payload_size, new_mesh->payload);
                    }
                break;

            case 'L':
            case 'l':
                if (new_mesh->meshlets == mesh->meshlets)
                    {
                    new_mesh->meshlets = CM.cache_meshlets(new_mesh->nb_meshlets, new_mesh->meshlets);
                    }
                break;

            case 'M':
            case 'm':
                if (!material_writable)
                    {
                    new_mesh->materials = CM.cache_materials(new_mesh->nb_materials, new_mesh->materials);
                    material_writable = (new_mesh->materials != mesh->materials);
                    }
                if ((new_mesh->material_extras != nullptr) && (!material_extras_writable))
                    {
                    new_mesh->material_extras = CM.cache_material_extras(new_mesh->nb_materials, new_mesh->material_extras);
                    material_extras_writable = (new_mesh->material_extras != mesh->material_extras);
                    }
                break;

            case 'I':
            case 'i':
                if (!material_writable)
                    {
                    new_mesh->materials = CM.cache_materials(new_mesh->nb_materials, new_mesh->materials);
                    material_writable = (new_mesh->materials != mesh->materials);
                    }
                if (material_writable)
                    {
                    MeshMaterial3Dv2<color_t>* const materials = const_cast<MeshMaterial3Dv2<color_t>*>(new_mesh->materials);
                    for (uint16_t i = 0; i < new_mesh->nb_materials; i++)
                        {
                        materials[i].texture = CM.cache_image(materials[i].texture);
                        }
                    }
                if (has_emissive_textures && (new_mesh->material_extras != nullptr) && (!material_extras_writable))
                    {
                    new_mesh->material_extras = CM.cache_material_extras(new_mesh->nb_materials, new_mesh->material_extras);
                    material_extras_writable = (new_mesh->material_extras != mesh->material_extras);
                    }
                if (has_emissive_textures && material_extras_writable && (new_mesh->material_extras != nullptr))
                    {
                    MeshMaterialExtra3Dv2<color_t>* const material_extras = const_cast<MeshMaterialExtra3Dv2<color_t>*>(new_mesh->material_extras);
                    for (uint16_t i = 0; i < new_mesh->nb_materials; i++)
                        {
                        material_extras[i].emissive_texture = CM.cache_image(material_extras[i].emissive_texture);
                        }
                    }
                break;
                }
            }

        if (ram1_used) { *ram1_used = ram1_size - CM.remaining_ram1(); }
        if (ram2_used) { *ram2_used = ram2_size - CM.remaining_ram2(); }

        return new_mesh;
        }


    template<typename color_t> const Mesh3Dv2<color_t>* cacheMesh(const Mesh3Dv2<color_t>* mesh,
                                                                  void* ram_buffer, size_t ram_size,
                                                                  const char* copy_order,
                                                                  size_t* ram_used)
        {
        return cacheMesh(mesh, ram_buffer, ram_size, nullptr, 0, copy_order, ram_used, nullptr);
        }


#if defined(ARDUINO_TEENSY41)

    template<typename color_t> void freeMeshEXTMEM(Mesh3Dv2<color_t>* mesh)
        {
        if ((mesh == nullptr) || (!TGX_IS_EXTMEM(mesh))) return;

        Mesh3Dv2_detail::ExtMemMap map;
        if (TGX_IS_EXTMEM(mesh->payload)) map.free(mesh->payload);
        if (TGX_IS_EXTMEM(mesh->meshlets)) map.free(mesh->meshlets);
        if (TGX_IS_EXTMEM(mesh->materials))
            {
            MeshMaterial3Dv2<color_t>* materials = const_cast<MeshMaterial3Dv2<color_t>*>(mesh->materials);
            for (uint16_t i = 0; i < mesh->nb_materials; i++)
                {
                const Image<color_t>* im = materials[i].texture;
                Mesh3Dv2_detail::free_image_extmem<color_t>(map, im);
                }
            map.free(mesh->materials);
            }
        if (TGX_IS_EXTMEM(mesh->material_extras))
            {
            MeshMaterialExtra3Dv2<color_t>* material_extras = const_cast<MeshMaterialExtra3Dv2<color_t>*>(mesh->material_extras);
            for (uint16_t i = 0; i < mesh->nb_materials; i++)
                {
                const Image<color_t>* im = material_extras[i].emissive_texture;
                Mesh3Dv2_detail::free_image_extmem<color_t>(map, im);
                }
            map.free(mesh->material_extras);
            }
        map.free(mesh);
        }


    template<typename color_t> Mesh3Dv2<color_t>* copyMeshEXTMEM(const Mesh3Dv2<color_t>* mesh,
                                                                 bool copy_payload,
                                                                 bool copy_meshlets,
                                                                 bool copy_materials,
                                                                 bool copy_textures,
                                                                 size_t* extmem_used)
        {
        if (extmem_used) { *extmem_used = 0; }
        if (external_psram_size <= 0) return nullptr;
        if ((mesh == nullptr) || (TGX_IS_EXTMEM(mesh))) return nullptr;
        if (TGX_IS_EXTMEM(mesh->payload) || TGX_IS_EXTMEM(mesh->meshlets) || TGX_IS_EXTMEM(mesh->materials) || TGX_IS_EXTMEM(mesh->material_extras)) return nullptr;
        if (mesh->materials != nullptr)
            {
            for (uint16_t i = 0; i < mesh->nb_materials; i++)
                {
                const Image<color_t>* im = mesh->materials[i].texture;
                if (im != nullptr)
                    {
                    if (TGX_IS_EXTMEM(im) || TGX_IS_EXTMEM(im->data())) return nullptr;
                    }
                }
            }
        if (mesh->material_extras != nullptr)
            {
            for (uint16_t i = 0; i < mesh->nb_materials; i++)
                {
                const Image<color_t>* im = mesh->material_extras[i].emissive_texture;
                if (im != nullptr)
                    {
                    if (TGX_IS_EXTMEM(im) || TGX_IS_EXTMEM(im->data())) return nullptr;
                    }
                }
            }

        Mesh3Dv2_detail::ExtMemMap map;
        Mesh3Dv2<color_t>* new_mesh = (Mesh3Dv2<color_t>*)map.malloc(mesh, sizeof(Mesh3Dv2<color_t>));
        if (new_mesh == nullptr) return nullptr;

        const size_t payload_size = Mesh3Dv2_detail::payload_size_bytes(mesh);
        const bool has_emissive_textures = Mesh3Dv2_detail::has_emissive_textures(mesh);

        if ((copy_materials || copy_textures) && (mesh->nb_materials > 0))
            {
            const MeshMaterial3Dv2<color_t>* p = (const MeshMaterial3Dv2<color_t>*)map.malloc(mesh->materials, ((size_t)mesh->nb_materials) * sizeof(MeshMaterial3Dv2<color_t>));
            if (p == nullptr) return nullptr;
            new_mesh->materials = p;
            }

        if ((copy_materials || (copy_textures && has_emissive_textures)) && (mesh->material_extras != nullptr) && (mesh->nb_materials > 0))
            {
            const MeshMaterialExtra3Dv2<color_t>* p = (const MeshMaterialExtra3Dv2<color_t>*)map.malloc(mesh->material_extras, ((size_t)mesh->nb_materials) * sizeof(MeshMaterialExtra3Dv2<color_t>));
            if (p == nullptr) return nullptr;
            new_mesh->material_extras = p;
            }

        if (copy_textures && (new_mesh->materials != mesh->materials))
            {
            MeshMaterial3Dv2<color_t>* materials = const_cast<MeshMaterial3Dv2<color_t>*>(new_mesh->materials);
            for (uint16_t i = 0; i < new_mesh->nb_materials; i++)
                {
                const Image<color_t>* im = materials[i].texture;
                if (im == nullptr) continue;
                const Image<color_t>* imp = Mesh3Dv2_detail::copy_image_extmem<color_t>(map, im);
                if (imp == nullptr) return nullptr;
                materials[i].texture = imp;
                }
            }

        if (copy_textures && (new_mesh->material_extras != nullptr) && (new_mesh->material_extras != mesh->material_extras))
            {
            MeshMaterialExtra3Dv2<color_t>* material_extras = const_cast<MeshMaterialExtra3Dv2<color_t>*>(new_mesh->material_extras);
            for (uint16_t i = 0; i < new_mesh->nb_materials; i++)
                {
                const Image<color_t>* im = material_extras[i].emissive_texture;
                if (im == nullptr) continue;
                const Image<color_t>* imp = Mesh3Dv2_detail::copy_image_extmem<color_t>(map, im);
                if (imp == nullptr) return nullptr;
                material_extras[i].emissive_texture = imp;
                }
            }

        if (copy_payload && (payload_size > 0) && (TGX_IS_PROGMEM(mesh->payload)))
            {
            const uint32_t* p = (const uint32_t*)map.malloc(mesh->payload, payload_size);
            if (p == nullptr) return nullptr;
            new_mesh->payload = p;
            }

        if (copy_meshlets && (mesh->nb_meshlets > 0) && (TGX_IS_PROGMEM(mesh->meshlets)))
            {
            const Meshlet3Dv2* p = (const Meshlet3Dv2*)map.malloc(mesh->meshlets, ((size_t)mesh->nb_meshlets) * sizeof(Meshlet3Dv2));
            if (p == nullptr) return nullptr;
            new_mesh->meshlets = p;
            }

        if (extmem_used) { *extmem_used = map.used(); }
        return new_mesh;
        }

#endif


    }


#endif

/* end of file */
