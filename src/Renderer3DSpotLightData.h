/**
 * @file Renderer3DSpotLightData.h
 * Internal storage for Renderer3D local spot lights.
 */
//
// Copyright 2020 Arvind Singh
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; If not, see <http://www.gnu.org/licenses/>.

#ifndef _TGX_RENDERER3D_SPOT_LIGHT_DATA_H_
#define _TGX_RENDERER3D_SPOT_LIGHT_DATA_H_

// only C++, no plain C
#ifdef __cplusplus

#include "tgx_config.h"
#include "Color.h"
#include "Vec3.h"

namespace tgx
{

    namespace Renderer3D_detail
    {

        enum SpotLightFlag
            {
            SPOT_LIGHT_CONE_ENABLED             = 1u << 0,
            SPOT_LIGHT_SOFT_CONE                = 1u << 1,
            SPOT_LIGHT_RUNTIME_SPECULAR_ENABLED = 1u << 2
            };


        /**
        * Internal storage for local spot lights.
        *
        * A point light is represented as a spot light without SPOT_LIGHT_CONE_ENABLED.
        * The arrays are split by field instead of grouped per light so the render loop may
        * touch only the values needed by each branch.
        */
        template<int MAX_SPOT_LIGHTS>
        struct SpotLightData
            {
            static_assert(MAX_SPOT_LIGHTS > 0, "MAX_SPOT_LIGHTS must be positive for this specialization");

            int count;                                      ///< Number of active spot-light slots.
            bool hasRuntimeSpecular;                        ///< True when at least one active light has local specular.

            // User-space scene parameters.
            fVec3 position[MAX_SPOT_LIGHTS];                ///< Light positions in world space.
            fVec3 direction[MAX_SPOT_LIGHTS];               ///< Light directions in world space.
            RGBf diffuseColor[MAX_SPOT_LIGHTS];             ///< User diffuse light colors.
            RGBf specularColor[MAX_SPOT_LIGHTS];            ///< User specular light colors.

            int flags[MAX_SPOT_LIGHTS];                     ///< Bitwise OR of SpotLightFlag values.

            // Precomputed range and cone constants.
            float range2[MAX_SPOT_LIGHTS];                  ///< Squared range, or a very large value for infinite range.
            float invRange2[MAX_SPOT_LIGHTS];               ///< Inverse squared range, or 0 for infinite range.
            float cosOuter[MAX_SPOT_LIGHTS];                ///< Cosine of the outer cone half-angle.
            float invCosWidth[MAX_SPOT_LIGHTS];             ///< 1 / (cos(inner) - cos(outer)), or 0 for hard cones.

            // Camera-space runtime geometry.
            fVec3 positionView[MAX_SPOT_LIGHTS];            ///< Light positions in view space.
            fVec3 directionView[MAX_SPOT_LIGHTS];           ///< Normalized light directions in view space.

            // Material-scaled runtime colors.
            RGBf runtimeDiffuseColor[MAX_SPOT_LIGHTS];      ///< diffuseColor multiplied by material diffuse strength.
            RGBf runtimeSpecularColor[MAX_SPOT_LIGHTS];     ///< specularColor multiplied by material specular strength.
            };


        /**
        * Empty specialization used when local spot lights are disabled.
        */
        template<>
        struct SpotLightData<0>
            {
            };

    }

}

#endif

#endif

/** end of file */
