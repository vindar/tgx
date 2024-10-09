/**   
 * @file Vec4.h 
 * 4D vector.
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


#ifndef _TGX_VEC4_H_
#define _TGX_VEC4_H_

// only C++, no plain C
#ifdef __cplusplus



#include <stdint.h>

#include "Misc.h"
#include "Vec2.h"
#include "Vec3.h"


namespace tgx
{


    // forward declarations

    template<typename T> struct Vec2;    

    template<typename T> struct Vec3;
   
    template<typename T> struct Vec4;


    // Specializations

    typedef Vec4<int> iVec4; ///< Integer-valued 4D vector

    typedef Vec4<float> fVec4; ///< Floating point valued 4D vector with single (float) precision

    typedef Vec4<double> dVec4; ///< Floating point valued 4D vector with double precision




    /**
     * Geenric 4D vector [specializations #iVec4, #fVec4, #dVec4].
     *
     * The class contains four public member variables `x`, `y`, `z` and `w` which define the 3D vector `(x,y,z,w)`.
     *
     * @tparam  `T` arithmetic type of the vector (`int`, `float`...)
     *
     * @sa `Vec2`,`Vec3`
     */
    template<typename T> struct Vec4 : public Vec3<T>
        {

        // mtools extension (if available).  
        #if (MTOOLS_TGX_EXTENSIONS)
        #include <mtools/extensions/tgx/tgx_ext_Vec4.inl>
        #endif


        // first three coordinates from base classes
        using Vec2<T>::x;
        using Vec2<T>::y;
        using Vec3<T>::z;

        // and the new fourth coordinates
        T w; ///< 'w' coordinate (fourth dimension)


        /**
         * default constructor: **the vector content is undefined**.
         */
        Vec4() {}


        /**
         * Constructor with explicit initialization.
         */
        constexpr Vec4(T X, T Y, T Z, T W) : Vec3<T>(X,Y,Z), w(W)
            {
            }


        /**
         * Default copy constructor.
         */
        Vec4(const Vec4 & V) = default;


        /**
        * Constructor from a Vec2.
        **/
        constexpr Vec4(Vec2<T> V, T Z, T W) : Vec3<T>(V, Z), w(W)
            {
            }


        /**
        * Constructor from a Vec3.
        **/
        constexpr Vec4(Vec3<T> V, T W) : Vec3<T>(V), w(W)
            {
            }


        /**
        * Default assignment operator.
        **/
        Vec4 & operator=(const Vec4 & V) = default;


        /**
         * Explicit conversion to another vector with different integral type.
         * 
         * @warning The conversion of the vectors components values from type `T` to type `U` is
         * performed with a simple C-style cast.
         */
        template<typename U>
        explicit operator Vec4<U>() { return Vec4<U>((U)x, (U)y, (U)z, (U)w); }


        /**
        * Implicit conversion to floating point type vector.
        **/
        operator Vec4<typename DefaultFPType<T>::fptype>() { return Vec4<typename DefaultFPType<T>::fptype>((typename DefaultFPType<T>::fptype)x, (typename DefaultFPType<T>::fptype)y, (typename DefaultFPType<T>::fptype)z, (typename DefaultFPType<T>::fptype)w); }


        /**
         * Equality comparator. Return true if all components compare equal.
         **/
        inline bool operator==(const Vec4 V) const
            {
            return ((x == V.x) && (y == V.y) && (z == V.z) && (w == V.w));
            }


        /**
         * Inequality operator.
         **/
        inline bool operator!=(const Vec4 V) const
            { 
            return(!(operator==(V))); 
            }


        /**
         * Less-than comparison operator. Use lexicographical order.
         **/
        inline bool operator<(const Vec4 V) const
            { 
            if (x < V.x) return true;
            if (x == V.x)
                {
                if (y < V.y) return true;
                if (y == V.y)
                    {
                    if (z < V.z) return true;
                    if (z == V.z) return true;
                        {
                        if (w < V.w) return true;
                        }
                    }
                }
            return false;
            }


        /**
         * Less-than-or-equal comparison operator. Use lexicographical order.
         **/
        inline bool operator<=(const Vec4 V) const
            {
            if (x < V.x) return true;
            if (x == V.x)
                {
                if (y < V.y) return true;
                if (y == V.y)
                    {
                    if (z < V.z) return true;
                    if (z == V.z) return true;
                        {
                        if (w <= V.w) return true;
                        }
                    }
                }
            return false;
            }
            

        /**
         * Greater-than comparison operator. Use lexicographical order.
         **/
        inline bool operator>(const Vec4 V) const 
            { 
            return(!(operator<=(V))); 
            }


        /**
         * Greater-than-or-equal comparison operator. Use lexicographical order.
         **/
        inline bool operator>=(const Vec4 V) const
            { 
            return(!(operator<(V))); 
            }


        /**
        * Add another vector to this one.
        **/
        inline void operator+=(const Vec4 V)
            { 
            x += V.x;
            y += V.y;
            z += V.z;
            w += V.w;
            }


        /**
        * Substract another vector from this one.
        **/
        inline void operator-=(const Vec4 V) 
            {
            x -= V.x;
            y -= V.y;
            z -= V.z;
            w -= V.w;
            }


        /**
         * Multiply this vector by another one (coordinate by coordinate multiplication).
         **/
        inline void operator*=(const Vec4 V) 
            {
            x *= V.x;
            y *= V.y;
            z *= V.z;
            w *= V.w;
            }


        /**
         * Divide this vector by another one (coordinate by coordinate division).
         **/
        inline void operator/=(const Vec4 V)
            {
            x /= V.x;
            y /= V.y;
            z /= V.z;
            w /= V.w;
            }


        /**
         * Scalar addition. Add v to each of the vector components. 
         **/
        inline void operator+=(const T v) 
            {
            x += v;
            y += v;
            z += v;
            w += v;
            }


        /**
         * Scalar substraction. Add v to each of the vector components.
         **/
        inline void operator-=(const T v) 
            {
            x -= v;
            y -= v;
            z -= v;
            w -= v;
            }

        /**
         * Scalar multiplication. Multiply each of the vector components by v.
         **/
        inline void operator*=(const T v) 
            {
            x *= v;
            y *= v;
            z *= v;
            w *= v;
            }

        /**
         * Scalar division. Divde each of the vector components by v.
         **/
        inline void operator/=(const T v) 
            {
            x /= v;
            y /= v;
            z /= v;
            w /= v;
            }


       
      //  DO NOT DEFINE FOR PROJECTIVE VECTORS 
      //  TO PREVENT SPUPID MISTAKES WHEN CASTING...       
      //  inline Vec4 operator-() const
      //      {
      //      return Vec4{ -x, -y, -z, w };
      //      }


        /**
         * Compute the squared euclidian norm of the vector.
         * 
         * @sa norm(), norm_fast()
        **/
        inline T norm2() const 
            { 
            return x*x + y*y +z*z +w*w; 
            }


        /**
         * Compute the euclidian norm of the vector.
         *
         * @tparam  Tfloat Return type also used by computation. If left unspecified, the default
         *                 floating type is used.
         *
         * @sa norm_fast()
         */
        template<typename Tfloat = typename DefaultFPType<T>::fptype > inline Tfloat norm() const
            { 
            return (Tfloat)tgx::precise_sqrt((Tfloat)(x*x + y*y + z*z + w*w));
            }


        /**
         * Compute the euclidian norm of the vector. Use the #tgx::fast_sqrt() approximation to speedup
         * computations.
         *
         * @tparam  Tfloat Return type also used by computation. If left unspecified, the default
         *                 floating type is used.
         *
         * @sa norm()
         */
        template<typename Tfloat = typename DefaultFPType<T>::fptype > inline Tfloat norm_fast() const
            { 
            return (Tfloat)tgx::fast_sqrt((Tfloat)(x*x + y*y + z*z + w*w));
            }


        /**
         * Compute the inverse of the euclidian norm of the vector.
         *
         * @tparam  Tfloat Return type also used by computation. If left unspecified, the default
         *                 floating type is used.
         *
         * @sa invnorm_fast()         
         */
        template<typename Tfloat = typename DefaultFPType<T>::fptype > inline Tfloat invnorm() const
            { 
            return (Tfloat)tgx::precise_invsqrt((Tfloat)(x*x + y*y + z*z + w*w));
            }


        /**
         * Compute the inverse of the euclidian norm of the vector. Use the #tgx::fast_invsqrt()
         * approximation to speedup computations.
         *
         * @tparam  Tfloat Return type also used by computation. If left unspecified, the default
         *                 floating type is used.
         *
         * @sa invnorm()
         */
        template<typename Tfloat = typename DefaultFPType<T>::fptype > inline Tfloat invnorm_fast() const
            { 
            return (Tfloat)tgx::fast_invsqrt((Tfloat)(x*x + y*y + z*z + w*w));
            }


        /**
         * Normalise the vector so that its norm is 1 (do nothing if the vector is 0).
         *
         * @tparam  Tfloat Floating point type used for computation (use default floating point type if
         *                 unspecified).
         *                 
         * @sa normalize_fast()
         */
        template<typename Tfloat = typename DefaultFPType<T>::fptype > inline void normalize()
            { 
            Tfloat a = invnorm<Tfloat>(); 
            x = (T)(x * a);
            y = (T)(y * a);
            z = (T)(z * a);
            w = (T)(w * a);
            }


        /**
         * Normalise the vector so that its norm is 1 (do nothing if the vector is 0). Use
         * #fast_invsqrt() approxiamtion to speedup computations.
         *
         * @tparam  Tfloat Floating point type used for computation (use default floating point type if
         *                 unspecified).
         *
         * @sa normalize()
         */
        template<typename Tfloat = typename DefaultFPType<T>::fptype > inline void normalize_fast()
            { 
            Tfloat a = invnorm_fast<Tfloat>(); 
            x = (T)(x * a);
            y = (T)(y * a);
            z = (T)(z * a);
            w = (T)(w * a);
            }


        /**
         * Return the vector normalized to have unit norm (do nothing if the vector is 0).
         *
         * @tparam  Tfloat Floating point type used for computation (use default floating point type if
         *                 unspecified).
         *
         * @sa getNormalize_fast()
         */
        template<typename Tfloat = typename DefaultFPType<T>::fptype > inline Vec4<T> getNormalize() const
            { 
            Vec4<T> V(*this);
            V.normalize();
            return V;
            }


        /**
         * Return the vector normalized to have unit norm (do nothing if the vector is 0). Use
         * #fast_invsqrt() approxiamtion to speedup computations.
         *
         * @tparam  Tfloat Floating point type used for computation (use default floating point type if
         *                 unspecified).
         *
         * @sa getNormalize()
         */
        template<typename Tfloat = typename DefaultFPType<T>::fptype > inline Vec4<T> getNormalize_fast() const
            { 
            Vec4<T> V(*this);
            V.normalize_fast();
            return V;
            }


        /**
        * Performs the 'z-divide' operation. 
        * 
        * Perform the following operation on the components using  #tgx::fast_inv to speed-up computation:
        * 
        * - x = x/w
        * - y = x/w
        * - z = z/w
        * - w = 1/w
        **/
        template<typename Tfloat = typename DefaultFPType<T>::fptype > inline void zdivide() 
            {
            const float iw = tgx::fast_inv(w);
            x = iw*x;
            y = iw*y;
            z = iw*z;
            w = iw;
            }


#ifdef TGX_ON_ARDUINO

        /**
         * Print a representation of the vector using a given stream object.
         * 
         * @attention Defined only in the Arduino environment.
         */
        inline void print(Stream & outputStream = Serial) const
            {
            outputStream.printf("[%.3f \t %.3f \t %.3f \t %.3f]\n", x, y, z, w);
            }

#endif 

    };



    /**
     * Return the vector normalized to have unit norm (do nothing if the vector is 0).
     *
     * @tparam Tfloat Floating point type used for computation (use default floating point type if
     *                unspecified).
     */
    template<typename T, typename Tfloat = typename DefaultFPType<T>::fptype > inline  Vec4<T> normalize(Vec4<T> V)
        {
        V.template normalize<Tfloat>();
        return V;
        }


    /**
     * Return the vector normalized to have unit norm (do nothing if the vector is 0). Use
     * fast_invsqrt() approximation to speedup computations.
     *
     * @tparam  Tfloat Floating point type used for computation (use default floating point type if
     *                 unspecified).
     */
    template<typename T, typename Tfloat = typename DefaultFPType<T>::fptype > inline  Vec4<T> normalize_fast(Vec4<T> V)
        {
        V.template normalize_fast<Tfloat>();
        return V;
        }


    /**
     * Compute the squared euclidian distance between two vectors.
     */
    template<typename T> inline T dist2(const Vec4<T>  V1, const Vec4<T>  V2)
        {
        const T xx = V1.x - V2.y;
        const T yy = V1.y - V2.y;
        const T zz = V1.z - V2.z;
        const T ww = V1.w - V2.w;
        return xx * xx + yy * yy + zz * zz + ww * ww;
        }


    /**
     * Compute the euclidian distance between two vectors.
     *
     * @tparam  Tfloat Floating point type used for computation (use default floating point type if
     *                 unspecified).
     */
    template<typename T, typename Tfloat = typename DefaultFPType<T>::fptype > Tfloat dist(Vec4<T> V1, const Vec4<T> V2)
        {
        const T xx = V1.x - V2.y;
        const T yy = V1.y - V2.y;
        const T zz = V1.z - V2.z;
        const T ww = V1.w - V2.w;
        return (Tfloat)tgx::precise_sqrt((Tfloat)(xx * xx + yy * yy + zz * zz + ww * ww));
        }


    /**
     * Compute the euclidian distance between two vectors. Use fast_sqrt() approximation to speedup
     * computations.
     *
     * @tparam  Tfloat Floating point type used for computation (use default floating point type if
     *                 unspecified).
     */
    template<typename T, typename Tfloat = typename DefaultFPType<T>::fptype > Tfloat dist_fast(Vec4<T> V1, const Vec4<T> V2)
        {
        const T xx = V1.x - V2.y;
        const T yy = V1.y - V2.y;
        const T zz = V1.z - V2.z;
        const T ww = V1.w - V2.w;
        return (Tfloat)tgx::fast_sqrt((Tfloat)(xx * xx + yy * yy + zz * zz + ww * ww));
        }


    /** Addition operator. Coordinates by coordinates */
    template<typename T> inline Vec4<T> operator+(Vec4<T> V1, const Vec4<T> V2) { V1 += V2; return V1; }

    /** Substraction operator. Coordinates by coordinates */
    template<typename T> inline Vec4<T> operator-(Vec4<T> V1, const Vec4<T> V2) { V1 -= V2; return V1; }

    /** Multiplication operator. Coordinates by coordinates */
    template<typename T> inline Vec4<T> operator*(Vec4<T> V1, const Vec4<T> V2) { V1 *= V2; return V1; }

    /** Division operator. Coordinates by coordinates */
    template<typename T> inline Vec4<T> operator/(Vec4<T> V1, const Vec4<T> V2) { V1 /= V2; return V1; }

    /** Scalar addition operator */
    template<typename T> inline Vec4<T> operator+(const T a, Vec4<T> V) { V += a; return V; }

    /** Scalar addition operator */
    template<typename T> inline Vec4<T> operator+(Vec4<T> V, const T a) { V += a; return V; }

    /** Scalar substraction operator */
    template<typename T> inline Vec4<T> operator-(const T a, Vec4<T> V) { V -= a; return V; }

    /** Scalar substraction operator */
    template<typename T> inline Vec4<T> operator-(Vec4<T> V, const T a) { V -= a; return V; }

    /** Scalar multiplication operator */
    template<typename T> inline Vec4<T> operator*(const T a, Vec4<T> V) { V *= a; return V; }

    /** Scalar multiplication operator */
    template<typename T> inline Vec4<T> operator*(Vec4<T> V, const T a) { V *= a; return V; }

    /** Scalar division operator */
    template<typename T> inline Vec4<T> operator/(const T a, Vec4<T> V) { V /= a; return V; }

    /** Scalar division operator */
    template<typename T> inline Vec4<T> operator/(Vec4<T> V, const T a) { V /= a; return V; }


    /**
     * Return the dot product U.V between two vectors.
     */
    template<typename T> inline T dotProduct(const Vec4<T> U, const Vec4<T> V)
        {
        return U.x * V.x + U.y * V.y + U.z * V.z + U.w * V.w;
        }


    /**
     * Return the cross product of u x v **as 3-dim vector (and set w = 0)**.
     */
    template<typename T> inline Vec4<T> crossProduct(const Vec4<T>& U, const Vec4<T>& V)
        {
        return Vec4<T>{ U.y* V.z - U.z * V.y,
                        U.z* V.x - U.x * V.z,
                        U.x* V.y - U.y * V.x,
                        0 };
        }



    /**
     * Linear interpolation: V1 + alpha(V2 - V1).
     */
    template<typename T, typename Tfloat = typename DefaultFPType<T>::fptype > inline Vec4<T> lerp(Tfloat alpha, Vec4<T> V1, Vec4<T> V2)
        {
        return Vec4<T>{ (T)(V1.x + alpha * (V2.x - V1.x)),
                        (T)(V1.y + alpha * (V2.y - V1.y)),
                        (T)(V1.z + alpha * (V2.z - V1.z)),
                        (T)(V1.w + alpha * (V2.w - V1.w)) };
        }


}

#endif

#endif

/** end of file */

