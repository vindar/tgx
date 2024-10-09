/**   
 * @file Box3.h 
 * 3D box class
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


    // Forward declaration

    template<typename T> struct Box3;


    // Specializations

    typedef Box3<int> iBox3; ///< Integer-valued 3D box

    typedef Box3<float> fBox3; ///< Floating point valued 3D box with single(float) precision

    typedef Box3<double> dBox3; ///< Floating point valued 3D box with double precision


    /**
     * Generic 3D Box [specializations #iBox3, #fBox3, #dBox3].
     * 
     * The class encapsulates of 6 public variables: `minX`, `maxX`, `minY`, `maxY`, `minZ`, `maxZ`
     * which delimit the 3 dimensional *closed* box: `[minX, maxX] x [minY, maxY] x [minZ, maxZ]`
     * 
     * The box is empty if `maxX` < `minX` or if `maxY` < `minY` or if `maxZ` < `minZ`.  
     * 
     * @warning Some methods compute things differently  depending whether T is an integral or a floating point  value type.
     *
     * @tparam  T arithmetic type of the box (`int`, `float`...)
     *
     * @sa  `iBox3`,`fBox3`,`dBox3`
     */
    template<typename T> struct Box3
    {
        
        // mtools extension (if available).   
        #if (MTOOLS_TGX_EXTENSIONS)
        #include <mtools/extensions/tgx/tgx_ext_Box3.inl>
        #endif        

        // box dimension: [minX, maxX] x [minY, maxY] x [minZ, maxZ]

        T minX; ///< min horizontal (X) value (inclusive)
        T maxX; ///< max horizontal (X) value (inclusive)
        T minY; ///< min vertical (Y) value (inclusive)
        T maxY; ///< max vertical (Y) value (inclusive)
        T minZ; ///< min depth (Z) value (inclusive)
        T maxZ; ///< max depth (Z) value (inclusive)


        /**
         * default constructor: **the box content is undefined**.
         */
        constexpr Box3()
            {
            }


        /**
         * Constructor with explicit dimensions.
         */
        constexpr Box3(const T minx, const T maxx, const T miny, const T maxy, const T minz, const T maxz) : minX(minx), maxX(maxx), minY(miny), maxY(maxy), minZ(minz), maxZ(maxz)
            {
            }


        /**
         * Construct a box representing a single point.
         */
        constexpr Box3(const Vec3<T>& v) : minX(v.x), maxX(v.x), minY(v.y), maxY(v.y), minZ(v.z), maxZ(v.z)
            {
            }


        /**
        * default copy constructor.
        */
        Box3(const Box3<T>& B) = default;


        /**
        * default assignement operator.
        */
        Box3<T>& operator=(const Box3<T>& B) = default;


        /**
        * Explicit conversion to another box type.
        */
        template<typename U>
        explicit operator Box3<U>() { return Box3<U>((U)minX, (U)maxX, (U)minY, (U)maxY, (U)minZ, (U)maxZ); }


        /**
        * Implicit conversion to floating-point type.
        */
        operator Box3<typename DefaultFPType<T>::fptype>() { return Box3<typename DefaultFPType<T>::fptype>((typename DefaultFPType<T>::fptype)minX, (typename DefaultFPType<T>::fptype)maxX, (typename DefaultFPType<T>::fptype)minY, (typename DefaultFPType<T>::fptype)maxY, (typename DefaultFPType<T>::fptype)minZ, (typename DefaultFPType<T>::fptype)maxZ); }


        /**
        * Return true if the box is empty.
        */
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
         * Return the box width.
         *
         * @warning The width is computed differently depending on whether `T` is of floating point or integral type.
         *          - If `T` is floating point, the method returns `maxX - minX`.
         *          - If `T` is integral, the method returns `maxX - minX + 1` (number of horizontal points in the closed box).
         */
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
         * Return the box height.
         *
         * @warning The height is computed differently depending on whether `T` is of floating point or integral type.
         *          - If `T` is floating point, the method returns `maxY - minY`.
         *          - If `T` is integral, the method returns `maxY - minY + 1` (number of vertical points in the closed box).
         */
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
         * Return the box depth.
         *
         * @warning The depth is computed differently depending on whether `T` is of floating point or integral type.
         *          - If `T` is floating point, the method returns `maxZ - minZ`.
         *          - If `T` is integral, the method returns `maxZ - minZ + 1` (number of vertical points in the closed box).
         */
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
        * Return true if the boxes are equal.
        *
        * @note Two empty boxes always compare equal.
        *
        * @see operator==()
        **/
        inline bool equals(const Box3<T>& B) const
            {
            if (isEmpty()) { return B.isEmpty(); }
            return ((minX == B.minX) && (maxX == B.maxX) && (minY == B.minY) && (maxY == B.maxY) && (minZ == B.minZ) && (maxZ == B.maxZ));
            }


        /**
        * Return true if the boxes are equal.
        *
        * @note Two empty boxes always compare equal.
        *
        * @see equals()
        **/
        inline bool operator==(const Box3<T>& B) const
            {
            return equals(B);
            }


        /**
        * Return true if the box contains the point v.
        **/
        inline bool contains(const Vec3<T>& v) const
            {
            return(((minX <= v.x) && (v.x <= maxX)) && ((minY <= v.y) && (v.y <= maxY)) && ((minZ <= v.z) && (v.z <= maxZ)));
            }


        /**
         * Return true if B is included in this box.
         *
         * @note
         * 1. An empty box contains nothing.
         * 2. A non-empty box contains any empty box.
         **/
        inline bool contains(const Box3<T>& B) const
            {
            if (isEmpty()) return false;
            if (B.isEmpty()) return true;
            return ((minX <= B.minX) && (maxX >= B.maxX) && (minY <= B.minY) && (maxY >= B.maxY) && (minZ <= B.minZ) && (maxZ >= B.maxZ));
            }


        /**
        * Return true if B is included in this box.
        *
        * @note
        * 1. An empty box contains nothing.
        * 2. A non-empty box contains any empty box.
        *
        * @see contains()
        **/
        inline bool operator>=(const Box3<T>& B) const
            {
            return contains(B);
            }


        /**
        * Return true if this box is included in B.
        *
        * @note
        * 1. An empty box contains nothing.
        * 2. A non-empty box contains any empty box.
        *
        * @see contains()
        **/
        inline bool operator<=(const Box3<T>& B) const
            {
            return B.contains(*this);
            }


        /**
        * Return true if B is *strictly* included in this box (i.e. contained but not equal).
        *
        * @note
        * 1. An empty box contains nothing.
        * 2. A non-empty box contains any empty box.
        **/
        inline bool operator>(const Box3<T>& B) const
            {
            return ((contains(B)) && (!equals(B)));
            }


        /**
        * Return true if this box is *strictly* included inside B (i.e. contained but not equal).
        *
        * @note
        * 1. An empty box contains nothing.
        * 2. A non-empty box contains any empty box.
        **/
        inline bool operator<(const Box3<T>& B) const
            {
            return ((B.contains(*this)) && (!B.equals(*this)));
            }


        /**
         * Return the intersection of this box and B. This may return an empty box.
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
        * Intersect this box with box B. May create an empty box.
        **/
        inline void operator&=(const Box3<T>& B)
            {
            (*this) = ((*this) & B);
            }


        /**
        * Return the smallest box containing both this box and B.
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
        * Translate this box by a given vector.
        **/
        inline void operator+=(const Vec3<T> & V)
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
        inline Box3<T> operator+(const Vec3<T> & V) const
            {
            return Box3<T>(minX + V.x, maxX + V.x, minY + V.y, maxY + V.y, minZ + V.z, maxZ + V.z);
            }


        /**
        * Translate the box by a given vector (substracted)
        **/
        inline void operator-=(const Vec3<T> & V)
            {
            minX -= V.x;
            maxX -= V.x;
            minY -= V.y;
            maxY -= V.y;
            minZ -= V.z;
            maxZ -= V.z;
            }


        /**
        * Return this box translated by v (substracted).
        **/
        inline Box3<T> operator-(const Vec3<T> & V) const
            {
            return Box3<T>(minX - V.x, maxX - V.x, minY - V.y, maxY - V.y, minZ - V.z, maxZ - V.z);
            }


        /**
        * Return the position of the box center as a 3 dimensional vector.
        **/
        Vec3<T> center() const
            {
            return Vec3<T>((minX + maxX) / 2, (minY + maxY) / 2, (minZ + maxZ) / 2);
            }


        /**
        * Zoom outside the box (ie increase its size by 1/10th).
        *
        * @sa zoomIn()
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
        * Zoom inside the box (ie decrease its size by 1/8th).
        *
        * @sa zoomOut()
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
        * Move the box to the left by 1/10th of its width.
        *
        * @sa up(),down(),right(),front(),back()
        **/
        void left()
            {
            const T u = lx() / 10;
            minX -= u;
            maxX -= u;
            }


        /**
        * Move the box to the right by 1/10th of its width.
        *
        * @sa up(),down(),left(),front(),back()
        **/
        void right()
            {
            const T u = lx() / 10;
            minX += u;
            maxX += u;
            }


        /**
        * Move the box up by 1/10th of its height.
        *
        * @sa down(),left(),right(),front(),back()
        **/
        void up()
            {
            const T v = ly() / 10;
            minY -= v;
            maxY -= v;
            }


        /**
        * Move the box down by 1/10th of its height.
        *
        * @sa up(),left(),right(),front(),back()
        **/
        void down()
            {
            const T v = ly() / 10;
            minY += v;
            maxY += v;
            }


        /**
        * Move the box front by 1/10th of its height.
        *
        * @sa up(),down(),left(),right(),back()
        **/
        void front()
            {
            const T v = lz() / 10;
            minZ -= v;
            maxZ -= v;
            }


        /**
        * Move the box back by 1/10th of its height.
        *
        * @sa up(),down(),left(),right(),front()
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

