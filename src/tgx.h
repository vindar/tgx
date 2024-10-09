/**
 * @file tgx.h 
 * Main header file. 
 * Includes all the other headers needed to use the library inside a project. 
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
#ifndef _TGX_H_
#define _TGX_H_

/** Main library header. Includes all other headers*/

#include "Fonts.h" // include this when compiled as a C file

// and now only C++, no more plain C
#ifdef __cplusplus

#include "Misc.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"
#include "Mat4.h"
#include "Box2.h"
#include "Box3.h"
#include "Color.h"
#include "Image.h"
#include "Mesh3D.h"
#include "Renderer3D.h"

#endif

#endif // _TGX_H_

/* end of file */


