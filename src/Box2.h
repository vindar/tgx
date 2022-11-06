/**   
 * @file Box2.h 
 * 2D box class
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


    // Forward declaration 
     
    template<typename T> struct Box2;


    // Specializations

    typedef Box2<int> iBox2; ///< Integer-valued 2D box.

    typedef Box2<float> fBox2; ///< Floating point valued 2D box with single (float) precision.

    typedef Box2<double> dBox2; ///< Floating point valued 2D box with double precision.



    /**
    * Define the placement of an anchor point inside a box.
    *
    * ```
    *        left       center      right
    *      top +-----------------------+
    *          |           .           |
    *          |           .           |
    *          |           .           |
    *   center | ..................... |
    *          |           .           |
    * baseline | ..................... |
    *          |           .           |
    *   bottom +-----------------------+
    * ```
    *
    * @remark
    * - The default value is `CENTER` which is used when either the horizontal or vertical placement is not specified.
    * - `BASELINE` only make sense for a text bounding and a given font. Otherwise, it is ignored (and implicitly replaced by `CENTER`).
    **/
    enum Anchor
        {
        CENTER = 0,                             ///< Center (vertical/horizontal alignement). This is the default placement if not specitified.
        LEFT = 1,                               ///< Left side (horizontal alignement)
        RIGHT = 2,                              ///< Right side (horizontal alignement)
        TOP = 4,                                ///< Top side (vertical alignement)
        BOTTOM = 8,                             ///< Bottom side (vertical alignement)
        BASELINE = 16,                          ///< Baseline height (vertical alignement). **only makes sense with a font (when drawing text), replaced by center otherwise**
        TOPLEFT = TOP | LEFT,                   ///< Top-left corner
        TOPRIGHT = TOP | RIGHT,                 ///< Top-right corner
        BOTTOMLEFT = BOTTOM | LEFT,             ///< bottom-left corner
        BOTTOMRIGHT = BOTTOM | RIGHT,           ///< Bottom-right corner
        CENTERLEFT = CENTER | LEFT,             ///< center point on the left side
        CENTERRIGHT = CENTER | RIGHT,           ///< center point on the right side 
        CENTERTOP = CENTER | TOP,               ///< center point on the top side
        CENTERBOTTOM = CENTER | BOTTOM,         ///< center point on the bottom side 
        DEFAULT_TEXT_ANCHOR = BASELINE | LEFT   ///< Default location for text anchoring. 
        };


    /** Enable bitwise `|` operator for enum Anchor. */
    inline constexpr Anchor operator|(Anchor a1, Anchor a2) { return ((Anchor)((int)a1 | (int)a2)); }

    /** Enable bitwise `|=` operator for enum Anchor. */
    inline Anchor& operator|=(Anchor& a1, Anchor a2) { a1 = a1 | a2; return a1; }

    /** Enable bitwise `&` operator for enum Anchor. */
    inline constexpr Anchor operator&(Anchor a1, Anchor a2) { return ((Anchor)((int)a1 & (int)a2)); }

    /** Enable bitwise `&=` operator for enum Anchor. */
    inline Anchor& operator&=(Anchor& a1, Anchor a2) { a1 = a1 & a2; return a1; }



    /** Splitting of a box in half and quarters. */
    enum BoxSplit
        {
        SPLIT_LEFT          = Anchor::LEFT,        ///< left half
        SPLIT_RIGHT         = Anchor::RIGHT,       ///< right half
        SPLIT_TOP           = Anchor::TOP,         ///< top half
        SPLIT_BOTTOM        = Anchor::BOTTOM,      ///< bottom half
        SPLIT_TOPLEFT       = Anchor::TOPLEFT,     ///< top left quarter
        SPLIT_TOPRIGHT      = Anchor::TOPRIGHT,    ///< top right quarter 
        SPLIT_BOTTOMLEFT    = Anchor::BOTTOMLEFT,  ///< bottom left quarter
        SPLIT_BOTTOMRIGHT   = Anchor::BOTTOMRIGHT  ///< bottom right quarter
        };


    /** Enable bitwise `|` operator for enum BoxSplit.*/
    inline constexpr BoxSplit operator|(BoxSplit a1, BoxSplit a2) { return ((BoxSplit)((int)a1 | (int)a2)); }

    /** Enable bitwise `|=` operator for enum BoxSplit.*/
    inline BoxSplit& operator|=(BoxSplit& a1, BoxSplit a2) { a1 = a1 | a2; return a1; }

    /** Enable bitwise `&` operator for enum BoxSplit.*/
    inline constexpr BoxSplit operator&(BoxSplit a1, BoxSplit a2) { return ((BoxSplit)((int)a1 & (int)a2)); }

    /** Enable bitwise `&=` operator for enum BoxSplit.*/
    inline BoxSplit& operator&=(BoxSplit& a1, BoxSplit a2) { a1 = a1 & a2; return a1; }




    /**
     * Generic 2D Box [specializations #iBox2 , #fBox2, #dBox2].
     * 
     * The class encapsulates of 4 public variables: `minX`, `maxX`, `minY`, `maxY` which delimit the 2
     * dimensional *closed* box: `[minX, maxX] x [minY, maxY]`
     * 
     * The box is empty if `maxX` < `minX` or if `maxY` < `minY`.
     * 
     * @warning Some methods compute things differently  depending whether T is an integral or a floating point  value type.
     * 
     * @tparam  `T` arithmetic type of the box (`int`, `float`...)
     *              
     * @sa `iBox2`,`fBox2`,`dBox2`
     */
    template<typename T> struct Box2
    {

        // mtools extension (if available).  
        #if (MTOOLS_TGX_EXTENSIONS)
        #include <mtools/extensions/tgx/tgx_ext_Box2.inl>
        #endif


        // box dimension: [minX, maxX] x [minY, maxY]
        
        T minX; ///< min horizontal (X) value (inclusive)
        T maxX; ///< max horizontal (X) value (inclusive)
        T minY; ///< min vertical (Y) value (inclusive)
        T maxY; ///< max vertical (Y) value (inclusive)


        /**
         * default constructor: **the box content is undefined**.
         */
        constexpr Box2()
            {
            }


        /**
         * Constructor with explicit dimensions.
         */
        constexpr Box2(const T minx, const T maxx, const T miny, const T maxy) : minX(minx), maxX(maxx), minY(miny), maxY(maxy)
            {
            }


        /**
         * Construct a box representing a single point.
         */
        constexpr Box2(const Vec2<T>& P) : minX(P.x), maxX(P.x), minY(P.y), maxY(P.y)
            {
            }


        /** 
        * default copy constructor.
        */
        Box2(const Box2<T>& B) = default;


        /** 
        * default assignement operator. 
        */
        Box2<T>& operator=(const Box2<T>& B) = default;


        /**
        * Explicit conversion to another box type.
        */
        template<typename U>
        explicit operator Box2<U>() { return Box2<U>((U)minX, (U)maxX, (U)minY, (U)maxY); }


        /**
        * Implicit conversion to floating-point type.
        */
        operator Box2<typename DefaultFPType<T>::fptype>() { return Box2<typename DefaultFPType<T>::fptype>((typename DefaultFPType<T>::fptype)minX, (typename DefaultFPType<T>::fptype)maxX, (typename DefaultFPType<T>::fptype)minY, (typename DefaultFPType<T>::fptype)maxY); }


        /** 
        * Return true if the box is empty. 
        */
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
         * Return the box width.
         * 
         * @warning The width is computed differently depending on whether `T` is of floating point
         *          or integral type.
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
         * @warning The height is computed differently depending on whether `T` is of floating point
         *          or integral type.
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
        * Return true if the boxes are equal.
        * 
        * @note Two empty boxes always compare equal. 
        * 
        * @see operator==()
        **/
        inline bool equals(const Box2<T>& B) const
            {
            if (isEmpty()) { return B.isEmpty(); }
            return ((minX == B.minX) && (maxX == B.maxX) && (minY == B.minY) && (maxY == B.maxY));
            }


        /**
        * Return true if the boxes are equal.
        *
        * @note Two empty boxes always compare equal.
        * 
        * @see equals()
        **/
        inline bool operator==(const Box2<T>& B) const
            {
            return equals(B);
            }


        /**
        * Return true if the box contains the point v.
        **/
        inline bool contains(const Vec2<T>& v) const
            {
            return(((minX <= v.x) && (v.x <= maxX)) && ((minY <= v.y) && (v.y <= maxY)));
            }


        /**
         * Return true if B is included in this box.
         *
         * @note
         * 1. An empty box contains nothing.
         * 2. A non-empty box contains any empty box.
         **/
        inline bool contains(const Box2<T>& B) const
            {
            if (isEmpty()) return false;
            if (B.isEmpty()) return true;
            return ((minX <= B.minX) && (maxX >= B.maxX) && (minY <= B.minY) && (maxY >= B.maxY));
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
        inline bool operator>=(const Box2<T>& B) const
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
        inline bool operator<=(const Box2<T>& B) const
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
        inline bool operator>(const Box2<T>& B) const
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
        inline bool operator<(const Box2<T>& B) const
            {
            return ((B.contains(*this)) && (!B.equals(*this)));
            }


        /**
         * Return the intersection of this box and B. This may return an empty box. 
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
        * Intersect this box with box B. May create an empty box.
        **/
        inline void operator&=(const Box2<T>& B)
            {
            (*this) = ((*this) & B);
            }


        /**
        * Return the smallest box containing both this box and B.
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
        * Translate this box by a given vector. 
        **/
        inline void operator+=(const Vec2<T> & V)
            {
            minX += V.x;
            maxX += V.x;
            minY += V.y;
            maxY += V.y;
            }


        /**
        * Return this box translated by vector v.
        **/
        inline Box2<T> operator+(const Vec2<T> & V) const
            {
            return Box2<T>(minX + V.x, maxX + V.x, minY + V.y, maxY + V.y);
            }


        /**
        * Translate the box by a given vector (substracted)
        **/
        inline void operator-=(const Vec2<T> & V)
            {
            minX -= V.x;
            maxX -= V.x;
            minY -= V.y;
            maxY -= V.y;
            }


        /**
        * Return this box translated by v (substracted).
        **/
        inline Box2<T> operator-(const Vec2<T> & V) const
            {
            return Box2<T>(minX - V.x, maxX - V.x, minY - V.y, maxY - V.y);
            }


        /**
         * Split the box in half or quarter.
         *
         * @param   part    The part or the box to keep. See tgx::BoxSplit. 
         *
         * @sa  getSplit()
         */
        inline void split(BoxSplit part)
            {
            *this = getSplit(part);
            }


        /**
         * Return the box splitted in half or quater.
         *
         * @param   part    The part or the box to keep. See tgx::BoxSplit.
         *
         * @sa  split()
         */
        Box2<T> getSplit(BoxSplit part) const
            {
            const T midX = (minX + maxX)/2;
            const T midY = (minY + maxY)/2;
            switch (part)
                {
            case SPLIT_TOP: { return Box2<T>(minX, maxX, midY, maxY); }
            case SPLIT_BOTTOM: { return Box2<T>(minX, maxX, minY, midY); }
            case SPLIT_LEFT: { return Box2<T>(minX, midX, minY, maxY); }
            case SPLIT_RIGHT: { return Box2<T>(midX, maxX, minY, maxY); }
            case SPLIT_TOPLEFT: { return Box2<T>(minX, midX, midY, maxY); }
            case SPLIT_TOPRIGHT: { return Box2<T>(midX, maxX, midY, maxY); }
            case SPLIT_BOTTOMLEFT: { return Box2<T>(minX, midX, minY, midY); }
            case SPLIT_BOTTOMRIGHT: { return Box2<T>(midX, maxX, minY, midY); }
                }
            return *this;
            }


        /**
         * Return the position of an anchor point inside this box.
         *
         * @param   anchor_pos Th anchor location, see 
         */
        Vec2<T> getAnchor(Anchor anchor_pos) const
            {
            anchor_pos &= (CENTER | LEFT | RIGHT | TOP | BOTTOM); // remove baseline flag
            switch (anchor_pos)
                {
            case TOPLEFT:       return Vec2<T>(minX, maxY);
            case TOPRIGHT:      return Vec2<T>(maxX, maxY);
            case BOTTOMLEFT:    return Vec2<T>(minX, minY);
            case BOTTOMRIGHT:   return Vec2<T>(maxX, minY);
            case CENTER:        return Vec2<T>((minX + maxX) / 2, (minY + maxY) / 2);
            case CENTERLEFT:    return Vec2<T>(minX, (minY + maxY) / 2);
            case CENTERRIGHT:   return Vec2<T>(maxX, (minY + maxY) / 2);
            case CENTERTOP:     return Vec2<T>((minX + maxX) / 2, maxY);
            case CENTERBOTTOM:  return Vec2<T>((minX + maxX) / 2, minY);
            default: break;
                }
            return Vec2<T>((minX + maxX) / 2, (minY + maxY) / 2); // unknown pos returns center.
            }


        /**
        * Return the position of the box center as a 2 dimensional vector.
        **/
        Vec2<T> center() const
            {
            return Vec2<T>((minX + maxX) / 2, (minY + maxY) / 2);
            }
            

        /**
        * Return the aspect ratio of the box `lx()/ly()`.
        * 
        * - Return -1 for an empty box.
        * - Beware that lx() and ly() are computed differently depending on wether T is an integral or a floating point type !   
        * 
        * @sa lx(),ly()
        **/
        template<typename Tfloat = DefaultFPType<T> > inline  Tfloat ratio() const
            {
            if (isEmpty()) return (Tfloat)(-1);
            return ((Tfloat)(lx())) / ((Tfloat)(ly()));
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
            }


        /**
        * Move the box to the left by 1/10th of its width.
        *
        * @sa up(),down(),right()
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
        * @sa up(),down(),left()
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
        * @sa down(),left(),right()
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
        * @sa up(),left(),right()
        **/
        void down()
            {
            const T v = ly() / 10;
            minY += v;
            maxY += v;
            }


        /**
        * Return the largest box with the same `ratio()` as box `B` that is centered and enclosed inside this box. 
        * 
        * @sa ratio(),getEnclosingWithSameRatioAs()
        **/
        Box2<T> getEnclosedWithSameRatioAs(const Box2<T> & B) const;


        /**
        * Return the smallest box with the same `ratio()` as box `B` that contains this box in its center.
        * 
        * @sa ratio(),getEnclosedWithSameRatioAs()
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

