/**
 * @file Renderer3DFlash.h
 * Optional Teensy 4.x helpers for placing selected Renderer3D template
 * instantiations in flash memory.
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

#ifndef _TGX_RENDERER3D_FLASH_H_
#define _TGX_RENDERER3D_FLASH_H_

#include "tgx_config.h"


#ifdef __cplusplus

#if defined(TGX_RUN_ON_TEENSY4) && defined(TEENSYDUINO)

namespace tgx
{
namespace Renderer3D_detail
{

    template<typename T> struct RemoveReference { typedef T type; };
    template<typename T> struct RemoveReference<T&> { typedef T type; };
    template<typename T> struct RemoveReference<T&&> { typedef T type; };

    template<typename T> struct Renderer3DFlashTraits;

    template<typename color_t, Shader LOADED_SHADERS, typename ZBUFFER_t, int MAX_DIRECTIONAL_LIGHTS, int MAX_SPOT_LIGHTS>
    struct Renderer3DFlashTraits<Renderer3D<color_t, LOADED_SHADERS, ZBUFFER_t, MAX_DIRECTIONAL_LIGHTS, MAX_SPOT_LIGHTS> >
        {
        typedef color_t color_type;
        };

}
}

    #define TGX_RENDERER3D_FLASH_SECTION(NAME) __attribute__((section(".flashmem.tgx.renderer3d." NAME)))

    #define TGX_RENDERER3D_FLASH_CAT_(A, B) A##B
    #define TGX_RENDERER3D_FLASH_CAT(A, B) TGX_RENDERER3D_FLASH_CAT_(A, B)

    /**
     * Explicitly instantiate the solid sphere drawing path in Teensy 4.x flash.
     *
     * Use this macro once at namespace/global scope, after declaring the
     * Renderer3D object. It is a no-op on non-Teensy4 targets.
     */
    #define TGX_RENDERER3D_FLASH_SPHERE_CORE_PATH(RENDERER3D_OBJECT) \
        TGX_RENDERER3D_FLASH_SPHERE_CORE_PATH_(RENDERER3D_OBJECT, TGX_RENDERER3D_FLASH_CAT(_tgx_renderer3d_flash_sphere_, __LINE__))

    #define TGX_RENDERER3D_FLASH_SPHERE_CORE_PATH_(RENDERER3D_OBJECT, RENDERER_T) \
        namespace tgx { \
        typedef typename ::tgx::Renderer3D_detail::RemoveReference<decltype(RENDERER3D_OBJECT)>::type RENDERER_T; \
        typedef typename ::tgx::Renderer3D_detail::Renderer3DFlashTraits<RENDERER_T>::color_type TGX_RENDERER3D_FLASH_CAT(RENDERER_T, _color_t); \
        template TGX_RENDERER3D_FLASH_SECTION("sphere") void RENDERER_T::drawSphere(int, int); \
        template TGX_RENDERER3D_FLASH_SECTION("sphere") void RENDERER_T::drawSphere(int, int, const Image<TGX_RENDERER3D_FLASH_CAT(RENDERER_T, _color_t)>*); \
        template TGX_RENDERER3D_FLASH_SECTION("sphere") void RENDERER_T::drawAdaptativeSphere(float); \
        template TGX_RENDERER3D_FLASH_SECTION("sphere") void RENDERER_T::drawAdaptativeSphere(const Image<TGX_RENDERER3D_FLASH_CAT(RENDERER_T, _color_t)>*, float); \
        template TGX_RENDERER3D_FLASH_SECTION("sphere") void RENDERER_T::_drawSphere<false, Renderer3D_detail::WIREFRAME_FAST>(int, int, const Image<TGX_RENDERER3D_FLASH_CAT(RENDERER_T, _color_t)>*, float, TGX_RENDERER3D_FLASH_CAT(RENDERER_T, _color_t), float); \
        }

    #if TGX_DRAWSPHERE_USE_STRIP_BANDS
        #define TGX_RENDERER3D_FLASH_SPHERE_STRIP_PATH(RENDERER3D_OBJECT) \
            TGX_RENDERER3D_FLASH_SPHERE_STRIP_PATH_(RENDERER3D_OBJECT, TGX_RENDERER3D_FLASH_CAT(_tgx_renderer3d_flash_sphere_strip_, __LINE__))

        #define TGX_RENDERER3D_FLASH_SPHERE_STRIP_PATH_(RENDERER3D_OBJECT, RENDERER_T) \
            namespace tgx { \
            typedef typename ::tgx::Renderer3D_detail::RemoveReference<decltype(RENDERER3D_OBJECT)>::type RENDERER_T; \
            template TGX_RENDERER3D_FLASH_SECTION("sphere") void RENDERER_T::_drawSphereGouraudStripBand(const int, int, const float*, const float*, float, float, float, float, float, float, float, float, bool); \
            template TGX_RENDERER3D_FLASH_SECTION("sphere") void RENDERER_T::_drawSphereGouraudCap(const int, int, bool, const float*, const float*, float, float, float, float, float, bool); \
            }

        #define TGX_PLACE_RENDERER3D_SPHERE_PATH_IN_FLASH(RENDERER3D_OBJECT) \
            TGX_RENDERER3D_FLASH_SPHERE_CORE_PATH(RENDERER3D_OBJECT) \
            TGX_RENDERER3D_FLASH_SPHERE_STRIP_PATH(RENDERER3D_OBJECT)
    #else
        #define TGX_PLACE_RENDERER3D_SPHERE_PATH_IN_FLASH(RENDERER3D_OBJECT) \
            TGX_RENDERER3D_FLASH_SPHERE_CORE_PATH(RENDERER3D_OBJECT)
    #endif

    /**
     * Explicitly instantiate the cylinder/cone/truncated-cone path in Teensy
     * 4.x flash.
     *
     * The shared triangle, clipping and rasterizer helpers are deliberately
     * left in their normal sections because they are also used by other hot
     * rendering paths.
     */
    #define TGX_PLACE_RENDERER3D_TRUNCATED_CONE_PATH_IN_FLASH(RENDERER3D_OBJECT) \
        TGX_PLACE_RENDERER3D_TRUNCATED_CONE_PATH_IN_FLASH_(RENDERER3D_OBJECT, TGX_RENDERER3D_FLASH_CAT(_tgx_renderer3d_flash_cone_, __LINE__))

    #define TGX_PLACE_RENDERER3D_TRUNCATED_CONE_PATH_IN_FLASH_(RENDERER3D_OBJECT, RENDERER_T) \
        namespace tgx { \
        typedef typename ::tgx::Renderer3D_detail::RemoveReference<decltype(RENDERER3D_OBJECT)>::type RENDERER_T; \
        typedef typename ::tgx::Renderer3D_detail::Renderer3DFlashTraits<RENDERER_T>::color_type TGX_RENDERER3D_FLASH_CAT(RENDERER_T, _color_t); \
        template TGX_RENDERER3D_FLASH_SECTION("truncatedcone") void RENDERER_T::drawCylinder(int, bool, bool); \
        template TGX_RENDERER3D_FLASH_SECTION("truncatedcone") void RENDERER_T::drawCylinder(int, const Image<TGX_RENDERER3D_FLASH_CAT(RENDERER_T, _color_t)>*, const Image<TGX_RENDERER3D_FLASH_CAT(RENDERER_T, _color_t)>*, const Image<TGX_RENDERER3D_FLASH_CAT(RENDERER_T, _color_t)>*, bool, bool); \
        template TGX_RENDERER3D_FLASH_SECTION("truncatedcone") void RENDERER_T::drawCone(int, bool); \
        template TGX_RENDERER3D_FLASH_SECTION("truncatedcone") void RENDERER_T::drawCone(int, const Image<TGX_RENDERER3D_FLASH_CAT(RENDERER_T, _color_t)>*, const Image<TGX_RENDERER3D_FLASH_CAT(RENDERER_T, _color_t)>*, bool); \
        template TGX_RENDERER3D_FLASH_SECTION("truncatedcone") void RENDERER_T::drawTruncatedCone(int, float, float, bool, bool); \
        template TGX_RENDERER3D_FLASH_SECTION("truncatedcone") void RENDERER_T::drawTruncatedCone(int, float, float, const Image<TGX_RENDERER3D_FLASH_CAT(RENDERER_T, _color_t)>*, const Image<TGX_RENDERER3D_FLASH_CAT(RENDERER_T, _color_t)>*, const Image<TGX_RENDERER3D_FLASH_CAT(RENDERER_T, _color_t)>*, bool, bool); \
        template TGX_RENDERER3D_FLASH_SECTION("truncatedcone") void RENDERER_T::_drawTruncatedCone(int, float, float, const Image<TGX_RENDERER3D_FLASH_CAT(RENDERER_T, _color_t)>*, const Image<TGX_RENDERER3D_FLASH_CAT(RENDERER_T, _color_t)>*, const Image<TGX_RENDERER3D_FLASH_CAT(RENDERER_T, _color_t)>*, bool, bool); \
        template TGX_RENDERER3D_FLASH_SECTION("truncatedcone") void RENDERER_T::_drawTruncatedConeGouraudCachedTriangle(const int, typename RENDERER_T::ExtVec4&, fVec3&, typename RENDERER_T::ExtVec4&, fVec3&, typename RENDERER_T::ExtVec4&, fVec3&, bool, bool, float, bool); \
        template TGX_RENDERER3D_FLASH_SECTION("truncatedcone") void RENDERER_T::_drawTruncatedConeGouraudSideStrip(const int, int, float, float, const float*, const float*, float, float, float, float, bool); \
        template TGX_RENDERER3D_FLASH_SECTION("truncatedcone") void RENDERER_T::_drawTruncatedConeGouraudConeFan(const int, int, bool, float, const float*, const float*, float, float, float, float, bool); \
        template TGX_RENDERER3D_FLASH_SECTION("truncatedcone") void RENDERER_T::_drawTruncatedConeGouraudCapFan(const int, int, bool, float, const float*, const float*, float, bool); \
        }

    /**
     * Explicitly instantiate the default legacy Mesh3D path in Teensy 4.x flash.
     *
     * This covers the regular drawMesh(mesh, ...) overload. Advanced
     * drawMesh<SHADERS>(mesh, ...) calls are intentionally not instantiated by
     * this macro.
     */
    #define TGX_PLACE_RENDERER3D_MESH3D_PATH_IN_FLASH(RENDERER3D_OBJECT) \
        TGX_PLACE_RENDERER3D_MESH3D_PATH_IN_FLASH_(RENDERER3D_OBJECT, TGX_RENDERER3D_FLASH_CAT(_tgx_renderer3d_flash_mesh3d_, __LINE__))

    #define TGX_PLACE_RENDERER3D_MESH3D_PATH_IN_FLASH_(RENDERER3D_OBJECT, RENDERER_T) \
        namespace tgx { \
        typedef typename ::tgx::Renderer3D_detail::RemoveReference<decltype(RENDERER3D_OBJECT)>::type RENDERER_T; \
        typedef typename ::tgx::Renderer3D_detail::Renderer3DFlashTraits<RENDERER_T>::color_type TGX_RENDERER3D_FLASH_CAT(RENDERER_T, _color_t); \
        template TGX_RENDERER3D_FLASH_SECTION("mesh3d") void RENDERER_T::drawMesh(const Mesh3D<TGX_RENDERER3D_FLASH_CAT(RENDERER_T, _color_t)>*, bool, bool); \
        template TGX_RENDERER3D_FLASH_SECTION("mesh3d") void RENDERER_T::_drawMesh(const int, const Mesh3D<TGX_RENDERER3D_FLASH_CAT(RENDERER_T, _color_t)>*); \
        }

    /**
     * Explicitly instantiate the default Mesh3Dv2 helper path in Teensy 4.x
     * flash.
     *
     * The public non-template wrapper is inline and tiny; the heavy helper is
     * the part that is placed in flash. Advanced drawMesh<SHADERS>(mesh, ...)
     * calls are intentionally not instantiated by this macro.
     */
    #define TGX_PLACE_RENDERER3D_MESH3DV2_PATH_IN_FLASH(RENDERER3D_OBJECT) \
        TGX_PLACE_RENDERER3D_MESH3DV2_PATH_IN_FLASH_(RENDERER3D_OBJECT, TGX_RENDERER3D_FLASH_CAT(_tgx_renderer3d_flash_mesh3dv2_, __LINE__))

    #define TGX_PLACE_RENDERER3D_MESH3DV2_PATH_IN_FLASH_(RENDERER3D_OBJECT, RENDERER_T) \
        namespace tgx { \
        typedef typename ::tgx::Renderer3D_detail::RemoveReference<decltype(RENDERER3D_OBJECT)>::type RENDERER_T; \
        typedef typename ::tgx::Renderer3D_detail::Renderer3DFlashTraits<RENDERER_T>::color_type TGX_RENDERER3D_FLASH_CAT(RENDERER_T, _color_t); \
        template TGX_RENDERER3D_FLASH_SECTION("mesh3dv2") void RENDERER_T::_drawMesh(const int, const Mesh3Dv2<TGX_RENDERER3D_FLASH_CAT(RENDERER_T, _color_t)>*, bool); \
        }

#else

    #define TGX_PLACE_RENDERER3D_SPHERE_PATH_IN_FLASH(RENDERER3D_OBJECT)
    #define TGX_PLACE_RENDERER3D_TRUNCATED_CONE_PATH_IN_FLASH(RENDERER3D_OBJECT)
    #define TGX_PLACE_RENDERER3D_MESH3D_PATH_IN_FLASH(RENDERER3D_OBJECT)
    #define TGX_PLACE_RENDERER3D_MESH3DV2_PATH_IN_FLASH(RENDERER3D_OBJECT)

#endif


#endif

#endif

/** end of file */
