/** @file Box2.h */
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

#ifndef _TGX_BOX2_H_
#define _TGX_BOX2_H_

// only C++, no plain C
#ifdef __cplusplus


#include "Misc.h"
#include "Vec2.h"

#include <type_traits>
#include <stdint.h>

namespace tgx
{


    /** Forward declaration */

    template<typename T> struct Box2;


    /** Specializations */

    typedef Box2<int> iBox2;            // integer valued box with platform int

    typedef Box2<int16_t> iBox2_s16;    // integer box with 16-bit int

    typedef Box2<int32_t> iBox2_s32;    // integer box with 32 bit int

    typedef Box2<float>   fBox2;        // floating-point valued box with float precision

    typedef Box2<double>  dBox2;        // floating-point valued box with double precision


                                        
/** positions for anchor points in a 2D box. */
#define TGX_BOX2_ANCHOR_XCENTER (0)
#define TGX_BOX2_ANCHOR_LEFT (1)
#define TGX_BOX2_ANCHOR_RIGHT (2)
#define TGX_BOX2_ANCHOR_YCENTER (0)
#define TGX_BOX2_ANCHOR_TOP (4)
#define TGX_BOX2_ANCHOR_BOTTOM (8)
#define TGX_BOX2_ANCHOR_TOPLEFT  (TGX_BOX2_ANCHOR_TOP | TGX_BOX2_ANCHOR_LEFT)
#define TGX_BOX2_ANCHOR_TOPRIGHT  (TGX_BOX2_ANCHOR_TOP | TGX_BOX2_ANCHOR_RIGHT)
#define TGX_BOX2_ANCHOR_BOTTOMLEFT  (TGX_BOX2_ANCHOR_BOTTOM | TGX_BOX2_ANCHOR_LEFT)
#define TGX_BOX2_ANCHOR_BOTTOMRIGHT  (TGX_BOX2_ANCHOR_BOTTOM | TGX_BOX2_ANCHOR_RIGHT)
#define TGX_BOX2_ANCHOR_CENTER  (TGX_BOX2_ANCHOR_XCENTER | TGX_BOX2_ANCHOR_YCENTER)
#define TGX_BOX2_ANCHOR_CENTERLEFT  (TGX_BOX2_ANCHOR_YCENTER | TGX_BOX2_ANCHOR_LEFT)
#define TGX_BOX2_ANCHOR_CENTERRIGHT  (TGX_BOX2_ANCHOR_YCENTER | TGX_BOX2_ANCHOR_RIGHT)
#define TGX_BOX2_ANCHOR_CENTERTOP  (TGX_BOX2_ANCHOR_XCENTER | TGX_BOX2_ANCHOR_TOP)
#define TGX_BOX2_ANCHOR_CENTERBOTTOM  (TGX_BOX2_ANCHOR_XCENTER | TGX_BOX2_ANCHOR_BOTTOM)


/* Splitting a 2D box in half */
#define TGX_BOX2_SPLIT_UP (1)
#define TGX_BOX2_SPLIT_DOWN (3)
#define TGX_BOX2_SPLIT_LEFT (4)
#define TGX_BOX2_SPLIT_RIGHT (12)

/* Splitting a 2D box in quarter*/
#define TGX_BOX2_SPLIT_UP_LEFT  (TGX_BOX2_SPLIT_UP | TGX_BOX2_SPLIT_LEFT)
#define TGX_BOX2_SPLIT_UP_RIGHT  (TGX_BOX2_SPLIT_UP | TGX_BOX2_SPLIT_RIGHT)
#define TGX_BOX2_SPLIT_DOWN_LEFT  (TGX_BOX2_SPLIT_DOWN | TGX_BOX2_SPLIT_LEFT)
#define TGX_BOX2_SPLIT_DOWN_RIGHT  (TGX_BOX2_SPLIT_DOWN | TGX_BOX2_SPLIT_RIGHT)



    /***************************************************************
    * Template class for a 2D box. 
    * 
    * Consist only of 4 public variables: minX, maxX, minY, maxY
    * which represent the 2-dimensional closed box: 
    *              [minX, maxX] x [minY, maxY]
    *
    * - the box is empty if maxX < minX or if maxY < minY.  
    * 
    * - Caution ! Some methods compute things differently  depending 
    * on whether T is an integral or a floating point  value type. 
    ***************************************************************/
    template<typename T> struct Box2
    {

        // mtools extension (if available).  
        #if (MTOOLS_TGX_EXTENSIONS)
        #include <mtools/extensions/tgx/tgx_ext_Box2.inl>
        #endif


        // box dimension: [minX, maxX] x [minY, maxY]
        // the box is closed !
        T minX, maxX, minY, maxY;


        /** 
        * default ctor: an uninitialized box. Undefined contents for max performance.  
        **/
        constexpr Box2()
            {
            }


        /** 
        * ctor with explicit dimensions. 
        **/
        constexpr Box2(const T minx, const T maxx, const T miny, const T maxy) : minX(minx), maxX(maxx), minY(miny), maxY(maxy)
            {
            }


        /** 
        * ctor: box containing a single point
        **/
        constexpr Box2(const Vec2<T>& v) : minX(v.x), maxX(v.x), minY(v.y), maxY(v.y)
            {
            }


        /** 
        * default copy ctor.
        **/
        Box2(const Box2<T>& B) = default;


        /** 
        * default assignement operator. 
        **/
        Box2<T>& operator=(const Box2<T>& B) = default;


        /**
        * Explicit conversion to another box type
        **/
        template<typename U>
        explicit operator Box2<U>() { return Box2<U>((U)minX, (U)maxX, (U)minY, (U)maxY); }


        /**
        * Implicit conversion to floating point type.
        **/
        operator Box2<typename DefaultFPType<T>::fptype>() { return Box2<typename DefaultFPType<T>::fptype>((typename DefaultFPType<T>::fptype)minX, (typename DefaultFPType<T>::fptype)maxX, (typename DefaultFPType<T>::fptype)minY, (typename DefaultFPType<T>::fptype)maxY); }


        /** 
        * Return true if the box is empty. 
        **/
        constexpr inline bool isEmpty() const { return ((maxX < minX) || (maxY < minY)); }


        /** 
        * Make the box empty.
        **/
        void empty()
            {
            minX = (T)1; 
            maxX = (T)0;
            minY = (T)1;
            maxY = (T)0;
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
        * returns true if the box contains point v 
        **/
        inline bool contains(const Vec2<T>& v) const
            {
            return(((minX <= v.x) && (v.x <= maxX)) && ((minY <= v.y) && (v.y <= maxY)));
            }


        /**
         * returns true if B is included in this box.
         * rule 1) An empty box contains nothing.
         * rule 2) a non-empty box contains any empty box.
         **/
        inline bool contains(const Box2<T>& B) const
            {
            if (isEmpty()) return false;
            if (B.isEmpty()) return true;
            return ((minX <= B.minX) && (maxX >= B.maxX) && (minY <= B.minY) && (maxY >= B.maxY));
            }


        /**
        * Return true if the boxes are equal.
        * rule 1) two empty boxes always compare equal.
        **/
        inline bool equals(const Box2<T>& B) const
            {
            if (isEmpty()) { return B.isEmpty(); }
            return ((minX == B.minX) && (maxX == B.maxX) && (minY == B.minY) && (maxY == B.maxY));
            }


        /**
        * same as equals(B)
        **/
        inline bool operator==(const Box2<T>& B) const
            {
            return equals(B);
            }


        /**
        * same as contains(B)
        **/
        inline bool operator>=(const Box2<T>& B) const
            {
            return contains(B);
            }


        /**
        * same as B.contains(*this)
        **/
        inline bool operator<=(const Box2<T>& B) const
            {
            return B.contains(*this);
            }


        /**
        * check if this box strictly contains B (ie contains it but is not equal).
        **/
        inline bool operator>(const Box2<T>& B) const
            {
            return ((contains(B)) && (!equals(B)));
            }


        /**
        * check if B strictly contains this box (ie contains it but is not equal).
        **/
        inline bool operator<(const Box2<T>& B) const
            {
            return ((B.contains(*this)) && (!B.equals(*this)));
            }


        /**
         * Return the intersection of 2 boxes.
         **/
        inline Box2<T> operator&(const Box2<T>& B) const
            {
            Box2<T> R;
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
                }
            return R;
            }


        /**
        * Intersect this box with box B.
        * May yield an empty box.
        **/
        inline void operator&=(const Box2<T>& B)
            {
            (*this) = ((*this) & B);
            }


        /**
        * Return the smallest box containing both boxes
        **/
        inline Box2<T> operator|(const Box2<T>& B) const
            {
            Box2<T> R;
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
                }
            return R;
            }


        /**
        * Enlarge this box in order to contain B.
        **/
        inline void operator|=(const Box2<T>& B)
            {
            *this = (*this) | B;
            }


        /**
        * Return the smallest box containing this box and point v.
        **/
        inline Box2<T> operator|(const Vec2<T>& v) const
            {
            Box2<T> R;
            if (isEmpty())
                {
                R.minX = R.maxX = v.x;
                R.minY = R.maxY = v.y;
                }
            else
                {
                R.minX = min(minX, v.x);
                R.maxX = max(maxX, v.x);
                R.minY = min(minY, v.y);
                R.maxY = max(maxY, v.y);
                }
            return R;
            }


        /**
        * Enlarge this box in order to contain point v.
        **/
        inline void operator|=(const Vec2<T>& v)
            {
            (*this) = (*this) | v;
            }


        /**
        * Translate the box by a given vector
        **/
        inline void operator+=(Vec2<T>  V)
            {
            minX += V.x;
            maxX += V.x;
            minY += V.y;
            maxY += V.y;
            }


        /**
        * Return this box translated by v.
        **/
        inline Box2<T> operator+(Vec2<T>  V) const
            {
            return Box2<T>(minX + V.x, maxX + V.x, minY + V.y, maxY + V.y);
            }


        /**
        * Translate the box by a given vector
        **/
        inline void operator-=(Vec2<T> V)
            {
            minX -= V.x;
            maxX -= V.x;
            minY -= V.y;
            maxY -= V.y;
            }


        /**
        * Return this box translated by v.
        **/
        inline Box2<T> operator-(Vec2<T> V) const
            {
            return Box2<T>(minX - V.x, maxX - V.x, minY - V.y, maxY - V.y);
            }



        /**
        * Split the box in half or in quarter.
        * Part should be one of TGX_BOX2_SPLIT_UP, TGX_BOX2_SPLIT_DOWN, TGX_BOX2_SPLIT_LEFT, TGX_BOX2_SPLIT_RIGHT, 
        * TGX_BOX2_SPLIT_UP_LEFT, TGX_BOX2_SPLIT_UP_RIGHT, TGX_BOX2_SPLIT_DOWN_LEFT, TGX_BOX2_SPLIT_DOWN_RIGHT
        **/
        inline void split(int part)
            {
            *this = getSplit(part);
            }


        /**
        * Return this box splitted in half or quarter.
        * Part should be one of TGX_BOX2_SPLIT_UP, TGX_BOX2_SPLIT_DOWN, TGX_BOX2_SPLIT_LEFT, TGX_BOX2_SPLIT_RIGHT, 
        * TGX_BOX2_SPLIT_UP_LEFT, TGX_BOX2_SPLIT_UP_RIGHT, TGX_BOX2_SPLIT_DOWN_LEFT, TGX_BOX2_SPLIT_DOWN_RIGHT
        **/
        Box2<T> getSplit(int part) const
            {
            const T midX = (minX + maxX)/2;
            const T midY = (minY + maxY)/2;
            switch (part)
                {
            case TGX_BOX2_SPLIT_UP: { return Box2<T>(minX, maxX, midY, maxY); }
            case TGX_BOX2_SPLIT_DOWN: { return Box2<T>(minX, maxX, minY, midY); }
            case TGX_BOX2_SPLIT_LEFT: { return Box2<T>(minX, midX, minY, maxY); }
            case TGX_BOX2_SPLIT_RIGHT: { return Box2<T>(midX, maxX, minY, maxY); }
            case TGX_BOX2_SPLIT_UP_LEFT: { return Box2<T>(minX, midX, midY, maxY); }
            case TGX_BOX2_SPLIT_UP_RIGHT: { return Box2<T>(midX, maxX, midY, maxY); }
            case TGX_BOX2_SPLIT_DOWN_LEFT: { return Box2<T>(minX, midX, minY, midY); }
            case TGX_BOX2_SPLIT_DOWN_RIGHT: { return Box2<T>(midX, maxX, minY, midY); }
                }
            return *this;
            }


        /**
         * Return the border point on the box corresponding to one of the 9 possible anchor points.
         * anchor_pos should be one of BOX2_ANCHOR_XXX constant (possibly combined with |).
         **/
        Vec2<T> getAnchor(int anchor_pos) const
            {
            switch (anchor_pos)
                {
            case TGX_BOX2_ANCHOR_TOPLEFT:       return Vec2<T>(minX, maxY);
            case TGX_BOX2_ANCHOR_TOPRIGHT:      return Vec2<T>(maxX, maxY);
            case TGX_BOX2_ANCHOR_BOTTOMLEFT:    return Vec2<T>(minX, minY);
            case TGX_BOX2_ANCHOR_BOTTOMRIGHT:   return Vec2<T>(maxX, minY);
            case TGX_BOX2_ANCHOR_CENTER:        return Vec2<T>((minX + maxX) / 2, (minY + maxY) / 2);
            case TGX_BOX2_ANCHOR_CENTERLEFT:    return Vec2<T>(minX, (minY + maxY) / 2);
            case TGX_BOX2_ANCHOR_CENTERRIGHT:   return Vec2<T>(maxX, (minY + maxY) / 2);
            case TGX_BOX2_ANCHOR_CENTERTOP:     return Vec2<T>((minX + maxX) / 2, maxY);
            case TGX_BOX2_ANCHOR_CENTERBOTTOM:  return Vec2<T>((minX + maxX) / 2, minY);
                }
            return Vec2<T>((minX + maxX) / 2, (minY + maxY) / 2); // unknown pos returns center.
            }


        /**
        * Return the center of the box. 
        **/
        Vec2<T> center() const
            {
            return Vec2<T>((minX + maxX) / 2, (minY + maxY) / 2);
            }
            

        /**
        * return the aspect ratio of the box lx()/(ly().
        * return -1 for an empty box.
        * Remark: lx() and ly() are computed differently depending on 
        * wether T is an integer or a floating point type ! 
        **/
        template<typename Tfloat = DefaultFPType<T> > inline  Tfloat ratio() const
            {
            if (isEmpty()) return (Tfloat)(-1);
            return ((Tfloat)(lx())) / ((Tfloat)(ly()));
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
        * Return the largest box with the same ratio() as box B that is centered 
        * and enclosed in this box. 
        **/
        Box2<T> getEnclosedWithSameRatioAs(const Box2<T> & B) const;


        /**
        * Return the smallest box with the same ratio() as box B that contain
        * this box in its center.
        **/
        Box2<T> getEnclosingWithSameRatioAs(const Box2<T> & B) const;


    };





    template<typename T> Box2<T> Box2<T>::getEnclosedWithSameRatioAs(const Box2<T> & B) const
        {
        Box2<T> C;
        if (ratio() < B.ratio())
            {
            C.minX = 0;
            C.maxX = maxX - minX;
            T ll = (T)(lx() / B.ratio());
            C.minY = (ly() - ll) / 2;
            C.maxY = C.minY + ll - (std::is_integral<T>::value ? 1 : 0);
            }
        else
            {
            C.minY = 0;
            C.maxY = maxY - minY;
            T ll = (T)(ly() * B.ratio());
            C.minX = (lx() - ll) / 2;
            C.maxX = C.minX + ll - (std::is_integral<T>::value ? 1 : 0);
            }
        return C;
        }


    template<typename T> Box2<T> Box2<T>::getEnclosingWithSameRatioAs(const Box2<T> & B) const
        {
        Box2<T> C;
        if (ratio() > B.ratio())
            {
            C.minX = 0;
            C.maxX = maxX - minX;
            T ll = (T)(lx() / B.ratio());
            C.minY = (ly() - ll) / 2;
            C.maxY = C.minY + ll - (std::is_integral<T>::value ? 1 : 0);
            }
        else
            {
            C.minY = 0;
            C.maxY = maxY - minY;
            T ll = (T)(ly() * B.ratio());
            C.minX = (lx() - ll) / 2;
            C.maxX = C.minX + ll - (std::is_integral<T>::value ? 1 : 0);
            }
        return C;
        }


}


#endif

#endif

/** end of file **/

