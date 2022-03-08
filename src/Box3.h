/** @file Box3.h */
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


#ifndef _TGX_BOX3_H_
#define _TGX_BOX3_H_

// only C++, no plain C
#ifdef __cplusplus

#include "Misc.h"
#include "Vec3.h"

#include <type_traits>
#include <stdint.h>

namespace tgx
{


    /** Forward declaration */

    template<typename T> struct Box3;


    /** Specializations */

    typedef Box3<int> iBox3;            // integer valued box with platform int

    typedef Box3<int16_t> iBox3_s16;    // integer box with 16-bit int

    typedef Box3<int32_t> iBox3_s32;    // integer box with 32 bit int

    typedef Box3<float>   fBox3;        // floating-point valued box with float precision

    typedef Box3<double>  dBox3;        // floating-point valued box with double precision




    /***************************************************************
    * Template class for a 3D box.
    *
    * Consist only of 4 public variables: minX, maxX, minY, maxY, minZ, maxZ
    * which represent the 3-dimensional closed box:
    *              [minX, maxX] x [minY, maxY] x [minZ, maxZ]
    *
    * - the box is empty if maxX < minX or if maxY < minY or if maxZ < minZ.
    *
    * - Caution ! Some methods compute things differently  depending
    * on whether T is an integral or a floating point  value type.
    ***************************************************************/
    template<typename T> struct Box3
    {
        
        // mtools extension (if available).   
        #if (MTOOLS_TGX_EXTENSIONS)
        #include <mtools/extensions/tgx/tgx_ext_Box3.inl>
        #endif        

        // box dimension: [minX, maxX] x [minY, maxY] x [minZ, maxZ]
        // the box is closed !
        T minX, maxX, minY, maxY, minZ, maxZ;


        /** 
        * default ctor: an uninitialized box. Undefined contents for max performance.  
        **/
        constexpr Box3()
            {
            }


        /** 
        * ctor with explicit dimensions. 
        **/
        constexpr Box3(const T minx, const T maxx, const T miny, const T maxy, const T minz, const T maxz) : minX(minx), maxX(maxx), minY(miny), maxY(maxy), minZ(minz), maxZ(maxz)
            {
            }


        /** 
        * ctor: box containing a single point
        **/
        constexpr Box3(const Vec3<T>& v) : minX(v.x), maxX(v.x), minY(v.y), maxY(v.y), minZ(v.z), maxZ(v.z)
            {
            }


        /** 
        * default copy ctor.
        **/
        Box3(const Box3<T>& B) = default;


        /** 
        * default assignement operator. 
        **/
        Box3<T>& operator=(const Box3<T>& B) = default;


        /**
        * Explicit conversion to another box type
        **/
        template<typename U>
        explicit operator Box3<U>() { return Box3<U>((U)minX, (U)maxX, (U)minY, (U)maxY, (U)minZ, (U)maxZ); }


        /**
        * Implicit conversion to floating point type.
        **/
        operator Box3<typename DefaultFPType<T>::fptype>() { return Box3<typename DefaultFPType<T>::fptype>((typename DefaultFPType<T>::fptype)minX, (typename DefaultFPType<T>::fptype)maxX, (typename DefaultFPType<T>::fptype)minY, (typename DefaultFPType<T>::fptype)maxY, (typename DefaultFPType<T>::fptype)minZ, (typename DefaultFPType<T>::fptype)maxZ); }


        /** 
        * Return true if the box is empty. 
        **/
        constexpr inline bool isEmpty() const { return ((maxX < minX) || (maxY < minY) || (maxZ < minZ)); }


        /** 
        * Make the box empty.
        **/
        void empty()
            {
            minX = (T)1; 
            maxX = (T)0;
            minY = (T)1;
            maxY = (T)0;
            minZ = (T)1;
            maxZ = (T)0;
            }


        /** 
         * Return the box width. Result differs depending wether T is integer or floating point type:
         * - if T is floating point: return maxX - minX.
         * - if T is integral: return maxX - minX + 1 (number of horizontal points in the closed box).
         **/
        inline T lx() const 
            {
            if (std::is_integral<T>::value) // compiler optimize this away. 
                {
                return (maxX - minX + 1); // for integer, return the number of points
                }
            else    
                {
                return (maxX - minX); // for floating point type, return maxX - minX. 
                }                   
            }


        /** 
        * Return the box height. Result differs depending wether T is integer or floating point type 
        * -if T is floating point : return maxY - minY.
        * -if T is integral : return maxY - minY + 1 (number of vertical points in the closed box).
        **/
        inline T ly() const
            {
            if (std::is_integral<T>::value) // compiler optimize this away. 
                {
                return (maxY - minY + 1); // for integer, return the number of points
                }
            else    
                {
                return (maxY - minY); // for floating point type, return maxX - minX. 
                }                   
            }


        /** 
        * Return the box depth. Result differs depending wether T is integer or floating point type 
        * -if T is floating point : return maxZ - minZ.
        * -if T is integral : return maxZ - minZ + 1 (number of depth points in the closed box).
        **/
        inline T lz() const
            {
            if (std::is_integral<T>::value) // compiler optimize this away. 
                {
                return (maxZ - minZ + 1); // for integer, return the number of points
                }
            else    
                {
                return (maxZ - minZ); // for floating point type, return maxX - minX. 
                }                   
            }


        /** 
        * returns true if the box contains point v 
        **/
        inline bool contains(const Vec3<T>& v) const
            {
            return(((minX <= v.x) && (v.x <= maxX)) && ((minY <= v.y) && (v.y <= maxY)) && ((minZ <= v.z) && (v.z <= maxZ)));
            }


        /**
         * returns true if B is included in this box.
         * rule 1) An empty box contains nothing.
         * rule 2) a non-empty box contains any empty box.
         **/
        inline bool contains(const Box3<T>& B) const
            {
            if (isEmpty()) return false;
            if (B.isEmpty()) return true;
            return ((minX <= B.minX) && (maxX >= B.maxX) && (minY <= B.minY) && (maxY >= B.maxY) && (minZ <= B.minZ) && (maxZ >= B.maxZ));
            }


        /**
        * Return true if the boxes are equal.
        * rule 1) two empty boxes always compare equal.
        **/
        inline bool equals(const Box3<T>& B) const
            {
            if (isEmpty()) { return B.isEmpty(); }
            return ((minX == B.minX) && (maxX == B.maxX) && (minY == B.minY) && (maxY == B.maxY) && (minZ == B.minZ) && (maxZ == B.maxZ));
            }


        /**
        * same as equals(B)
        **/
        inline bool operator==(const Box3<T>& B) const
            {
            return equals(B);
            }


        /**
        * same as contains(B)
        **/
        inline bool operator>=(const Box3<T>& B) const
            {
            return contains(B);
            }


        /**
        * same as B.contains(*this)
        **/
        inline bool operator<=(const Box3<T>& B) const
            {
            return B.contains(*this);
            }


        /**
        * check if this box strictly contains B (ie contains it but is not equal).
        **/
        inline bool operator>(const Box3<T>& B) const
            {
            return ((contains(B)) && (!equals(B)));
            }


        /**
        * check if B strictly contains this box (ie contains it but is not equal).
        **/
        inline bool operator<(const Box3<T>& B) const
            {
            return ((B.contains(*this)) && (!B.equals(*this)));
            }


        /**
         * Return the intersection of 2 boxes.
         **/
        inline Box3<T> operator&(const Box3<T>& B) const
            {
            Box3<T> R;
            if (isEmpty()) 
                {
                R = *this;
                }
            else if (B.isEmpty())
                {
                R = B;
                }
            else
                {
                R.minX = max(minX, B.minX);
                R.maxX = min(maxX, B.maxX);
                R.minY = max(minY, B.minY);
                R.maxY = min(maxY, B.maxY);
                R.minZ = max(minZ, B.minZ);
                R.maxZ = min(maxZ, B.maxZ);
                }
            return R;
            }


        /**
        * Intersect this box with box B.
        * May yield an empty box.
        **/
        inline void operator&=(const Box3<T>& B)
            {
            (*this) = ((*this) & B);
            }


        /**
        * Return the smallest box containing both boxes
        **/
        inline Box3<T> operator|(const Box3<T>& B) const
            {
            Box3<T> R;
            if (isEmpty())
                {
                R = B;
                }
            else if (B.isEmpty())
                {
                R = (*this);
                }
            else
                {
                R.minX = min(minX, B.minX);
                R.maxX = max(maxX, B.maxX);
                R.minY = min(minY, B.minY);
                R.maxY = max(maxY, B.maxY);
                R.minZ = min(minZ, B.minZ);
                R.maxZ = max(maxZ, B.maxZ);
                }
            return R;
            }


        /**
        * Enlarge this box in order to contain B.
        **/
        inline void operator|=(const Box3<T>& B)
            {
            *this = (*this) | B;
            }


        /**
        * Return the smallest box containing this box and point v.
        **/
        inline Box3<T> operator|(const Vec3<T>& v) const
            {
            Box3<T> R;
            if (isEmpty())
                {
                R.minX = R.maxX = v.x;
                R.minY = R.maxY = v.y;
                R.minZ = R.maxZ = v.z;
                }
            else
                {
                R.minX = min(minX, v.x);
                R.maxX = max(maxX, v.x);
                R.minY = min(minY, v.y);
                R.maxY = max(maxY, v.y);
                R.minZ = min(minZ, v.z);
                R.maxZ = max(maxZ, v.z);
                }
            return R;
            }


        /**
        * Enlarge this box in order to contain point v.
        **/
        inline void operator|=(const Vec3<T>& v)
            {
            (*this) = (*this) | v;
            }


        /**
        * Translate the box by a given vector
        **/
        inline void operator+=(Vec3<T>  V)
            {
            minX += V.x;
            maxX += V.x;
            minY += V.y;
            maxY += V.y;
            minZ += V.z;
            maxZ += V.z;
            }


        /**
        * Return this box translated by v.
        **/
        inline Box3<T> operator+(Vec3<T>  V) const
            {
            return Box3<T>(minX + V.x, maxX + V.x, minY + V.y, maxY + V.y, minZ + V.z, maxZ + V.z);
            }


        /**
        * Translate the box by a given vector
        **/
        inline void operator-=(Vec3<T> V)
            {
            minX -= V.x;
            maxX -= V.x;
            minY -= V.y;
            maxY -= V.y;
            minZ -= V.z;
            maxZ -= V.z;
            }


        /**
        * Return this box translated by v.
        **/
        inline Box3<T> operator-(Vec3<T> V) const
            {
            return Box3<T>(minX - V.x, maxX - V.x, minY - V.y, maxY - V.y, minZ - V.z, maxZ - V.z);
            }


        /**
        * Return the center of the box. 
        **/
        Vec3<T> center() const
            {
            return Vec3<T>((minX + maxX) / 2, (minY + maxY) / 2, (minZ + maxZ) / 2);
            }


        /**
        * Zoom outside the box (i.e. increase radius by 1/10th).
        **/
        void zoomOut()
            {
            const T u = lx() / 10;
            minX -= u;
            maxX += u;
            const T v = ly() / 10;
            minY -= v;
            maxY += v;
            const T w = lz() / 10;
            minZ -= w;
            maxZ += w;
            }


        /**
        * Zoom inside the box (i.e. decrease radius by 1/8th).
        **/
        void zoomIn()
            {
            const T u = lx() / 8;
            minX += u;
            maxX -= u;
            const T v = ly() / 8;
            minY += v;
            maxY -= v;
            const T w = lz() / 8;
            minZ += w;
            maxZ -= w;
            }


        /**
        * Move the box left by 1/10th of its width.
        **/
        void left()
            {
            const T u = lx() / 10;
            minX -= u;
            maxX -= u;
            }


        /**
        * Move the box right by 1/10th of its width.
        **/
        void right()
            {
            const T u = lx() / 10;
            minX += u;
            maxX += u;
            }


        /**
        * Move the rectangle up by 1/10th of its height.
        **/
        void up()
            {
            const T v = ly() / 10;
            minY -= v;
            maxY -= v;
            }


        /**
        * Move the rectangle down by 1/10th of its height.
        **/
        void down()
            {
            const T v = ly() / 10;
            minY += v;
            maxY += v;
            }


        /**
        * Move the rectangle front by 1/10th of its height.
        **/
        void front()
            {
            const T v = lz() / 10;
            minZ -= v;
            maxZ -= v;
            }


        /**
        * Move the rectangle back by 1/10th of its height.
        **/
        void back()
            {
            const T v = lz() / 10;
            minZ += v;
            maxZ += v;
            }


    };




}

#endif

#endif

/** end of file */

