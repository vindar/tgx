/** @file Vec2.h */
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


#ifndef _TGX_VEC2_H_
#define _TGX_VEC2_H_

// only C++, no plain C
#ifdef __cplusplus


#include <stdint.h>
#include "Misc.h"

namespace tgx
{


    /** forward declarations */

    template<typename T> struct Vec2;

    template<typename T> struct Vec3;

    template<typename T> struct Vec4;



    /** Specializations */

    typedef Vec2<int> iVec2;            // integer valued 2-D vector with platform int

    typedef Vec2<int16_t> iVec2_s16;    // integer valued 2-D vector with 16 bit int

    typedef Vec2<int32_t> iVec2_s32;    // integer valued 2-D vector with 32 bit int

    typedef Vec2<float>   fVec2;        // floating point value 2-D vector with float precision

    typedef Vec2<double>  dVec2;        // floating point value 2-D vector with double precision




    /********************************************
    * template class for a 2-d vector
    *********************************************/
    template<typename T> struct Vec2
        {

        // mtools extension (if available).  
        #if (MTOOLS_TGX_EXTENSIONS)
        #include <mtools/extensions/tgx/tgx_ext_Vec2.inl>
        #endif
        
        // coordinates
        T x, y;


        /**
        * Default ctor. Uninitialized components. 
        **/
        Vec2() {}


        /**
        * ctor. Explicit initialization
        **/
        constexpr Vec2(T X, T Y) : x(X), y(Y) {}



        /**
        * default copy ctor.
        **/
        Vec2(const Vec2 & v) = default;



        /**
        * Default assignment operator.
        **/
        Vec2 & operator=(const Vec2 & V) = default;


        /**
        * Explicit conversion to another vector
        **/
        template<typename U>
        explicit operator Vec2<U>() { return Vec2<U>((U)x, (U)y); }


        /**
        * Implicit conversion to floating point type. 
        **/
        operator Vec2<typename DefaultFPType<T>::fptype>() { return Vec2<typename DefaultFPType<T>::fptype>((typename DefaultFPType<T>::fptype)x, (typename DefaultFPType<T>::fptype)y); }



        /**
         * Equality comparator. True if both component are equal. 
         **/
        inline bool operator==(const Vec2 V) const 
            {
            return ((x == V.x) && (y == V.y));
            }


        /**
         * Inequality operator.
         **/
        inline bool operator!=(const Vec2 V) const
            { 
            return(!(operator==(V))); 
            }


        /**
         * Less-than comparison operator. Lexicographical order.
         **/
        inline bool operator<(const Vec2 V) const 
            { 
            return ((x < V.x) || ((x == V.x) && (y < V.y)));
            }


        /**
         * Less-than-or-equal comparison operator. Lexicographical order.
         **/
        inline bool operator<=(const Vec2 V) const
            {
            return ((x <= V.x) || ((x == V.x) && (y <= V.y)));
            }
            

        /**
         * Greater-than comparison operator. Lexicographical order.
         **/
        inline bool operator>(const Vec2 V) const 
            { 
            return(!(operator<=(V))); 
            }


        /**
         * Greater-than-or-equal comparison operator. Lexicographical order.
         * @return true if the first parameter is greater than or equal to the second.
         **/
        inline bool operator>=(const Vec2 V) const
            { 
            return(!(operator<(V))); 
            }


        /**
        * Add another vector to this one.
        **/
        inline void operator+=(const Vec2 V)
            { 
            x += V.x;
            y += V.y;
            }


        /**
        * Substract another vector from this one.
        **/
        inline void operator-=(const Vec2 V) 
            {
            x -= V.x;
            y -= V.y;
            }


        /**
         * Multiply this vector by another one. 
         * Coordinate by coordinate multiplication.
         **/
        inline void operator*=(const Vec2 V)
            {
            x *= V.x;
            y *= V.y;
            }


        /**
         * Divide this vector by another one.
         * Coordinate by coordinate division.
         **/
        inline void operator/=(const Vec2 V) 
            {
            x /= V.x;
            y /= V.y;
            }


        /**
         * scalar addition. Add v to each of the vector components. 
         **/
        inline void operator+=(const T & v) 
            {
            x += v;
            y += v;
            }


        /**
         * scalar substraction. Add v to each of the vector components.
         **/
        inline void operator-=(const T & v) 
            {
            x -= v;
            y -= v;
            }

        /**
         * scalar multiplication. Multiply each of the vector components by v.
         **/
        inline void operator*=(const T & v)
            {
            x *= v;
            y *= v;
            }

        /**
         * scalar division. Divde each of the vector components by v.
         **/
        inline void operator/=(const T & v)
            {
            x /= v;
            y /= v;
            }


        /**
         * unary negation operator
         **/
        inline Vec2 operator-() const 
            {
            return Vec2{ -x, -y };
            }


        /**
         * Compute the squared euclidian norm of the vector (as type T).
        **/
        inline T norm2() const
            { 
            return x*x + y*y; 
            }


        /**
         * Compute the euclidian norm of the vector (return a Tfloat).
        * Tfloat selects the floating point type used for computation.
         **/
        template<typename Tfloat = typename DefaultFPType<T>::fptype > inline Tfloat norm() const
            { 
            return (Tfloat)tgx::precise_sqrt((Tfloat)(x*x + y*y));
            }


        /**
         * Compute the euclidian norm of the vector (return a Tfloat). Use fast (approx) computation.
        * Tfloat selects the floating point type used for computation.
         **/
        template<typename Tfloat = typename DefaultFPType<T>::fptype > inline Tfloat norm_fast() const
            { 
            return (Tfloat)tgx::fast_sqrt((Tfloat)(x*x + y*y));
            }


        /**
         * Compute the inverse euclidian norm of the vector (return a Tfloat).
        * Tfloat selects the floating point type used for computation.
         **/
        template<typename Tfloat = typename DefaultFPType<T>::fptype > inline Tfloat invnorm() const
            { 
            return (Tfloat)tgx::precise_invsqrt((Tfloat)(x*x + y*y));
            }


        /**
         * Compute the inverse euclidian norm of the vector (return a Tfloat). Use fast (approx) computation.
        * Tfloat selects the floating point type used for computation.
         **/
        template<typename Tfloat = typename DefaultFPType<T>::fptype > inline Tfloat invnorm_fast() const
            { 
            return (Tfloat)tgx::fast_invsqrt((Tfloat)(x*x + y*y));
            }



        /**
        * Normalise the vector so that its norm is 1, does nothing if the vector is 0.
        * Tfloat selects the floating point type used for computation.
        **/
        template<typename Tfloat = typename DefaultFPType<T>::fptype > inline void normalize()
            { 
            Tfloat a = invnorm<Tfloat>(); 
            x = (T)(x * a);
            y = (T)(y * a);
            }


        /**
        * Normalise the vector so that its norm is 1, does nothing if the vector is 0. Use fast (approx) computation.
        * Tfloat selects the floating point type used for computation.
        **/
        template<typename Tfloat = typename DefaultFPType<T>::fptype > inline void normalize_fast()
            { 
            Tfloat a = invnorm_fast<Tfloat>(); 
            x = (T)(x * a);
            y = (T)(y * a);
            }


        /**
        * Return the normalize vector, return the same vector if it is 0.
        * Tfloat selects the floating point type used for computation.
        **/
        template<typename Tfloat = typename DefaultFPType<T>::fptype > inline Vec2<T> getNormalize() const
            { 
            Vec2<T> V(*this);
            V.normalize();
            return V;
            }


        /**
        * Return the normalize vector, return the same vector if it is 0. Use fast (approx) computation.
        * Tfloat selects the floating point type used for computation.
        **/
        template<typename Tfloat = typename DefaultFPType<T>::fptype > inline Vec2<T> getNormalize_fast() const
            { 
            Vec2<T> V(*this);
            V.normalize_fast();
            return V;
            }



        /**
         * Rotate a the vector by +90 degree (anticlockise).
         **/
        inline void rotate90()
            { 
            *this = getRotate90(); 
            }


        /**
         * get the vector rotated by +90 degree (anticlockise).
         **/
        inline Vec2<T> getRotate90() const
            { 
            return Vec2<T>(-y, x); 
            }


        /** 
        * Test on which side of the oriented line (LA -> LB) does this point belong. 
        * Return 1 if this point is on the left of the line (LA -> LB).
        * Return 0 if this point is exactly on the line (LA -> LB).
        * Reurn -1 if this point is on the right of the line (LA -> LB). 
        **/
        inline int leftOf(Vec2<T> LA, Vec2<T> LB) const;


        /**
        * Set the value of this vector as the intersection points of the two lines
        * (LA1, LA2) and (LB1, LB2). If the lines are parallel, do not change the
        * value if *this.
        * Return true if the intersection point was computed and false if the lines
        * are parallel.
        **/
        inline bool setAsIntersection(Vec2<T> LA1, Vec2<T> LA2, Vec2<T> LB1, Vec2<T> LB2);





#ifdef TGX_ON_ARDUINO

        /***
        * Print the vector using a given stream object.
        **/
        inline void print(Stream & outputStream = Serial) const
            {
            outputStream.printf("[%.6f \t %.6f]\n", x, y);
            }

#endif

    };





        /**
        * Compute the squared euclidian distance between two vectors. Return as type T.
        **/
        template<typename T> inline T dist2(const Vec2<T>  V1, const Vec2<T>  V2)
            {
            const T xx = V1.x - V2.y;
            const T yy = V1.y - V2.y;
            return xx * xx + yy * yy;
            }


        /**
         * Compute the euclidian distance between two vectors, return as Tfloat.
         **/
        template<typename T, typename Tfloat = typename DefaultFPType<T>::fptype > Tfloat dist(Vec2<T> V1, const Vec2<T> V2)
            {
            const T xx = V1.x - V2.y;
            const T yy = V1.y - V2.y;
            return (Tfloat)tgx::precise_sqrt((Tfloat)(xx * xx + yy * yy));
            }


        /**
         * Compute the euclidian distance between two vectors, return as Tfloat. Use fast (approx) computations.
         **/
        template<typename T, typename Tfloat = typename DefaultFPType<T>::fptype > Tfloat dist_fast(Vec2<T> V1, const Vec2<T> V2)
            {
            const T xx = V1.x - V2.y;
            const T yy = V1.y - V2.y;
            return (Tfloat)tgx::fast_sqrt((Tfloat)(xx * xx + yy * yy));
            }



        /**
         * Addition operator. Coordinates by coordinates
         **/
        template<typename T> inline Vec2<T> operator+(Vec2<T> V1, const Vec2<T> V2) { V1 += V2; return V1; }


        /**
         * Substraction operator. Coordinates by coordinates
         **/
        template<typename T> inline Vec2<T> operator-(Vec2<T> V1, const Vec2<T> V2) { V1 -= V2; return V1; }


        /**
         * Multiplication operator. Coordinates by coordinates
         **/
        template<typename T> inline Vec2<T> operator*(Vec2<T> V1, const Vec2<T> V2) { V1 *= V2; return V1; }


        /**
         * Division operator. Coordinates by coordinates
         **/
        template<typename T> inline Vec2<T> operator/(Vec2<T> V1, const Vec2<T> V2) { V1 /= V2; return V1; }


        /**
         * Scalar addition operator.
         **/
        template<typename T> inline Vec2<T> operator+(const T a, Vec2<T> V) { V += a; return V; }
        template<typename T> inline Vec2<T> operator+(Vec2<T> V, const T a) { V += a; return V; }


        /**
         * Scalar substraction operator.
         **/
        template<typename T> inline Vec2<T> operator-(const T a, Vec2<T> V) { V -= a; return V; }
        template<typename T> inline Vec2<T> operator-(Vec2<T> V, const T a) { V -= a; return V; }


        /**
         * Scalar multiplication operator.
         **/
        template<typename T> inline Vec2<T> operator*(const T a, Vec2<T> V) { V *= a; return V; }
        template<typename T> inline Vec2<T> operator*(Vec2<T> V, const T a) { V *= a; return V; }


        /**
         * Scalar division operator.
         **/
        template<typename T> inline Vec2<T> operator/(const T a, Vec2<T> V) { V /= a; return V; }
        template<typename T> inline Vec2<T> operator/(Vec2<T> V, const T a) { V /= a; return V; }


        /**
         * Return the dot product U.V between two vectors.
         **/
        template<typename T> inline T dotProduct(const Vec2<T> U, const Vec2<T> V) 
            { 
            return U.x * V.x + U.y * V.y;
            }


        /**
        * Cross product UxV (i.e. the determinant of the two row vector matrix [U V]).
        **/
        template<typename T> inline T crossProduct(const Vec2<T> & U, const Vec2<T> & V) 
            { 
            return (U.x * V.y - U.y * V.x); 
            }



        template<typename T> inline int Vec2<T>::leftOf(Vec2<T> LA, Vec2<T> LB) const
            {
            T x = crossProduct(LB - LA, (*this) - LB);
            return ((x < 0) ? -1 : ((x > 0) ? 1 : 0));
            }


        template<typename T> inline bool Vec2<T>::setAsIntersection(Vec2<T> LA1, Vec2<T> LA2, Vec2<T> LB1, Vec2<T> LB2)
            {
            const T a1 = LA2.Y() - LA1.Y();
            const T b1 = LA1.X() - LA2.X();
            const T a2 = LB2.Y() - LB1.Y();
            const T b2 = LB1.X() - LB2.X();
            const T delta = a1 * b2 - a2 * b1;
            if (delta == 0) { return false; }
            const T c1 = LA1.X() * a1 + LA1.Y() * b1;
            const T c2 = LB1.X() * a2 + LB1.Y() * b2;
            x = ((b1 == 0) ? LA1.X() : ((b2 == 0) ? LB1.X() : (T)((b2 * c1 - b1 * c2) / delta)));   // complicated but insures perfect clipping
            y = ((a1 == 0) ? LA1.Y() : ((a2 == 0) ? LB1.Y() : (T)((a1 * c2 - a2 * c1) / delta)));   // for horizontal and vertical lines
            return true;
            }



        /**
        * Return the linear interpolation: V1 + alpha(V2 - V1).
        **/
        template<typename T, typename Tfloat = typename DefaultFPType<T>::fptype > inline Vec2<T> lerp(Tfloat alpha, Vec2<T> V1, Vec2<T> V2)
            {
            return Vec2<T>{ (T)(V1.x + alpha * (V2.x - V1.x)),
                            (T)(V1.y + alpha * (V2.y - V1.y)) };
            }
        

}

#endif

#endif

/** end of file */

