/**   
 * @file Image.h 
 * Main image class. 
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
#ifndef _TGX_IMAGE_H_
#define _TGX_IMAGE_H_

#include "Fonts.h" // include this also when compiled as a C file

// and now only C++, no more plain C
#ifdef __cplusplus

#include "Misc.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"
#include "Box2.h"
#include "Color.h"
#include "ShaderParams.h"
#include "Shaders.h"
#include "Rasterizer.h"
#include "bseg.h"

#include <stdint.h>


namespace tgx
{
    /**
     * Enumeration of end path shapes.
     * 
     * List the shapes that can be drawn at the end of a path. Used by methods such as
     * `Image::drawThickLineAA()`, `Image::drawThickPolylineAA()`, `Image::drawThickQuadSplineAA()`...
     */
    enum EndPath
        {
        END_STRAIGHT = -1,          ///< straight end. 
        END_ROUNDED = 0,            ///< rounded end.
        END_ARROW_1 = 1,            ///< tiny arrow head [extends = line thickness]
        END_ARROW_2 = 2,            ///< small arrow head [extends = 2 x line thickness]
        END_ARROW_3 = 3,            ///< medium arrow head [extends = 3 x line thickness]
        END_ARROW_4 = 4,            ///< large arrow head [extends = 4 x line thickness]
        END_ARROW_5 = 5,            ///< huge arrow head [extends = 5 x line thickness]
        END_ARROW_SKEWED_1 = 101,   ///< tiny skewed arrow head [extends = line thickness]
        END_ARROW_SKEWED_2 = 102,   ///< small skewed arrow head [extends = 2 x line thickness]
        END_ARROW_SKEWED_3 = 103,   ///< medium skewed arrow head [extends = 3 x line thickness]
        END_ARROW_SKEWED_4 = 104,   ///< large skewed arrow head [extends = 4 x line thickness]
        END_ARROW_SKEWED_5 = 105,   ///> huge skewed arrow head [extends = 5 x line thickness]
        };
        


     
    /**
     * Image class [**MAIN CLASS FOR THE 2D API**].
     *
     * **This is the main image class for the TGX library**.
     * 
     * An image object is a thin wrapper (only 16 bytes) around a memory buffer which defines the type and
     * dimension of the image.
     * 
     * @tparam  color_t Color type of the image. Must be one of the color type defined in Color.h. Typical choice:
     *                  - `RGB32` when running on a desktop computer.
     *                  - `RGB565` when running on an MCU with limited memory (Teensy 4, ESP32..)
     *
     * **Image memory layout**
     * 
     * The image in the memory buffer in made of consecutive pixel of type color_t in row major order but with a
     * stride which may be larger than the image width (ie row lenght).
     *
     * Pixels are ordered from the top left `(0,0)` to the bottom right `(_lx-1, _ly-1)` and the position of pixel
     * `(x,y)` in the buffer is given by the formula:
     * ```
     * pixel(x,y) = buffer[x + y * _stride].
     * ```
     * 
     * @note **No memory allocation is ever perfomed by the Image class. It is the sole user repsonsability to
     *       allocated et free the pixel buffer**
     * 
     * @remark
     *
     * 1. When creating a new image, one can (and usually should) choose `_stride = lx`. However, allowing
     *    other strides in the class definition allows to efficiently represent sub-images without making
     *    a copy and thus enables fast clipping of all drawing operations (sub-images are similar to Numpy
     *    view for ndarray).
     *
     * 2. Avoid using `HSV` as color type for an image: it has not been tested to work and will be very slow.
     *
     * 3. The library defines many drawing primitives with naming convention:
     *    ```
     *    [draw/fill][Thick/-]MethodName[AA/-]
     *    ```
     *    where
     *    - `[draw]`  means that the method draw a path whereas [fill] means that the method create a solid shape.
     *    - `[thick]` means that the outline of the shape has selectable thickness (not just a single pixel
     *                wide). The tickness is usually a float and should be at least 1.
     *    - `[AA]`    means that the method is *high quality*: it is drawn with anti-aliasing and sub-pixel
     *                precision. Method with this suffix take fVec2 parameters for coordinates inputs whereas
     *                regular 'low quality' methods (without the AA suffix) take iVec2 parameters.
     *                
     *    For example:
     *    - `drawThickCircleAA()` :  draw a high quality circle with user defined outline thickness, antialiasing and sub-pixel precison.
     *    - `fillTriangle()`      :  draw a low quality filled triangle (no antialiasing or subpixel precision).
     *
     * 4. All drawing methods that take a `color` parameter also take an `opacity` float parameter which allows to
     *    add transparency effects even when color type used do not have an alpha channel:
     *    - Passing `TGX_DEFAULT_NO_BLENDING` or a negative value for the opacity will disable blending so the method
     *      will simply use overwritting of the destination pixels.
     *    - Passing `0.0f < opacity <= 1.0f` will enable blending and the alpha channel of the colors will be used if present.
     *      
     * 5. As indicated in color.h. **All colors with an alpha channel are assumed to have pre-multiplied
     *    alpha** and treated as such for all blending operations cf: https://en.wikipedia.org/wiki/Alpha_compositing
     *
     * 6. Most methods take `iVec2`/`fVec2` parameters to specify coordinates in the image. Using initializer
     *    list, it is very easy to call such method without explicitly mentioning the `iVec2`/`fVec2` type. For
     *    example, a function with signature `f(iVec2 pos, int r)` can be simply called `f({x,y},z)` which
     *    is equivalent to the expression `f(iVec2(x,y),z)`. 
     *    
     *    @note `iVec2` vectors are automatically promoted to `fVec2` vectors so calling a method with sub-pixel
     *          precision is transparent when working with integer vectors `iVec2`. On the other hand, downgrading
     *          from `fVec2` to `iVec2` requires an explicit cast.
     *
     * 7. Creating sub-image is cheap and convenient to restrict a drawing operation to a rectangular region of an
     *    image. Do not refrain from creating sub-images!
     */
    template<typename color_t> 
    class Image
    {

        // make sure right away that the template parameter is admissible to prevent cryptic error message later.  
        static_assert(is_color<color_t>::value, "color_t must be one of the color types defined in Color.h");

        // befriend all sister Image classes
        template<typename> friend class Image;


    public:

        static const int DEFAULT_STRIDE = -1;   ///< If not specified, the stride of an image is equal to its width. 


        #if (MTOOLS_TGX_EXTENSIONS)
        #include <mtools/extensions/tgx/tgx_ext_Image.inl> 
        #endif



    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************
    /**
    * @name Creation of image and sub-images. 
    *
    * If the memory buffer is not supplied at creation, the image is marked as invalid until a buffer is set.
    *
    * @remark 
    * 1. The image object does not manage the buffer memory: **memory allocation/deallocation is the user's respnsability.**  
    * 2. The image class is lightweight: creating image and sub-image is very fast and requires no additional memory.  
    * 
    */
    ///@{
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************

   

        /** 
        * Default constructor.
        *  
        * Create a empty (invalid) image. 
        */
        Image();


        /**
         * Constructor. Creates an image with a given size and a given buffer.
         *
         * @param   buffer  the image buffer.
         * @param   lx      the image width.
         * @param   ly      the image height.
         * @param   stride  (Optional) the stride to use (equal to the image width if not specified).
         */
        template<typename T> Image(T* buffer, int lx, int ly, int stride = DEFAULT_STRIDE);


        /**
         * Constructor. Creates an image with a given size and a given buffer.
         *
         * @param   buffer  the image buffer.
         * @param   dim     the image dimension (width, height) in pixels.
         * @param   stride  the stride to use (equal to the image width if not specified).
         */
        template<typename T> Image(T* buffer, iVec2 dim, int stride = DEFAULT_STRIDE);


        /**
         * Constructor. Create a sub-image of a given image sharing the same buffer.
         *
         * @param   im      The source image.
         * @param   subbox  The region of the source image to use for this image.
         */
        Image(const Image<color_t>& im, iBox2 subbox);


        /**
         * Default copy constructor.
         * 
         * **Copy is shallow**: both images share the same memory buffer.
         *
         * @param   im  The source image.
         */
        Image(const Image<color_t> & im) = default;


        /**
         * Default assignement operator.
         *
         * **Copy is shallow**: both images share the same memory buffer.
         *
         * @param   im  The source image.
         *
         * @returns itself.
         */
        Image& operator=(const Image<color_t> & im) = default;


        /**
         * Set/update the image parameters.
         *
         * @param   buffer  the pixel buffer.
         * @param   lx      image width.
         * @param   ly      image height.
         * @param   stride  (Optional) The stride. If not specified, the stride is set equal to the image width.
         */
        template<typename T> void set(T* buffer, int lx, int ly, int stride = DEFAULT_STRIDE);


        /**
         * Set/update the image parameters.
         *
         * @param   buffer  the pixel buffer.
         * @param   dim     the image dimensions.
         * @param   stride  (Optional) The stride. If not specified, the stride is set equal to the image width.
         */
        template<typename T> void set(T* buffer, iVec2 dim, int stride = DEFAULT_STRIDE);


        /**
         * Crop the image.
         * 
         * Crop this image keeping only the region represented by subbox intersected with the image box.
         * 
         * This operation does not change the underlying pixel buffer: it simply replaces this image by
         * a sub-image of itself (with different dimension/stride).
         *
         * @param   subbox  The region to to keep.
         */
        void crop(const iBox2& subbox);


        /**
         * Return a sub-image of this image (sharing the same pixel buffer).
         *
         * See also `operator()` below.
         * 
         * @param   subbox  The region to to keep.
         *
         * @returns the cropped image.
         */
        Image<color_t> getCrop(const iBox2& subbox) const;


        /**
         * Return a sub-image of this image (sharing the same pixel buffer).
         * 
         * This is the same as `getCrop(B)`
         *
         * @param   B  box that delimit the sub-image inside this image. 
         *
         * @returns the sub-image delimited by B and sharing the same memory buffer. 
         */
        Image<color_t> operator()(const iBox2& B) const;


        /**
         * Return a sub-image of this image (sharing the same pixel buffer).
         *
         * This is the same as `getCrop(tgx::iBox2(min_x, max_x, min_y, max_x))` or `operator()(tgx::iBox2(min_x, max_x, min_y, max_x))`
         *
         * @param   min_x   left boundary (inclusive)
         * @param   max_x   right boundary (inclusive)
         * @param   min_y   top boundary (inclusive)
         * @param   max_y   bottom boundary (inclusive)
         *
         * @returns the sub-image delimited the closed box iBox(min_x, max_x, min_y, max_x) and sharing the same memory buffer. 
         */
        Image<color_t> operator()(int min_x, int max_x, int min_y, int max_y) const;



    ///@}
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************
    /**
    * @name Image attributes.
    *       
    * Query the state (dimension, buffer...) of the image.  
    */
    ///@{
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************


        /**
         * Query if the image is valid.
         *
         * @returns True if the image if valid, false otherwise.
         */
        inline TGX_INLINE bool isValid() const { return (_buffer != nullptr); }


        /**
        * Set the image as invalid.
        *
        * @note It is the user responsibility to manage the memory allocated for the pixel buffer (if required). No deallocation is performed here.
        */
        void setInvalid();


        /**
         * Return the image width. Same as lx().
         *
         * @returns The image width (0 for an invalid image).
         */
        inline TGX_INLINE int width() const { return _lx; }


        /**
         * Return the image width. Same as width().
         *
         * @returns The image width (0 for an invalid image).
         */
        inline TGX_INLINE int lx() const { return _lx; }


        /** 
         * Return the image height. Same as ly().
         *
         * @returns The image height (0 for an invalid image).
         */
        inline TGX_INLINE int height() const { return _ly; }


        /**
         * Return the image height. Same as height().
         *
         * @returns The image height (0 for an invalid image).
         */
        inline TGX_INLINE int ly() const { return _ly; }


        /**
         * Return the image stride.
         *
         * @returns The stride  (0 for an invalid image).
         */
        inline TGX_INLINE int stride() const { return _stride; }


        /**
         * Return the image dimensions as an iVec2
         *
         * @returns The image dimension (returns {0,0} for an invalid image).
         */
        inline TGX_INLINE iVec2 dim() const { return iVec2{ _lx, _ly }; }


        /**
         * Return the image dimension as a box.
         *
         * @returns a box of the form {0, width-1, 0 height-1 } or an empty box if the image is invalid.
         */
        inline TGX_INLINE iBox2 imageBox() const { return iBox2(0, _lx - 1, 0, _ly - 1); }


        /**
         * Return a pointer to the pixel buffer.
         * 
         * const overload.
         *
         * @returns A const pointer to the start of the pixel buffer associated with this image (or nullptr if the image is invalid).
         */
        inline TGX_INLINE const color_t * data() const { return _buffer; } 


        /**
         * Return a pointer to the pixel buffer.
         *
         * non-const overload. 
         * 
         * @returns A pointer to the start of the pixel buffer associated with this image (or nullptr if the image is invalid).
         */
        inline TGX_INLINE color_t * data() { return _buffer; }



       


    ///@}
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************
    /**
    * @name Direct pixel access.
    *       
    * Methods to read/write pixels directly on the image buffer. 
    */
    ///@{
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************




        /**
         * Set a pixel at a given position.
         *
         * @tparam  CHECKRANGE  set to false to disable range checking (danger!)
         * @param   pos         position.
         * @param   color       color to set.
         */
        template<bool CHECKRANGE = true> TGX_INLINE inline void drawPixel(iVec2 pos, color_t color) { if ((CHECKRANGE) && (!isValid())) return; _drawPixel<CHECKRANGE>(pos, color); }


        /**
         * Set a pixel at a given position (floating point coord).
         * 
         * @remark Version for floating point value coordinates. Recall that the image range is `[-0.5f, lx-0.5f]x[-0.5f,lx-0.5f]` and center of pixels are on the interger lattice. 
         * 
         * @tparam  CHECKRANGE  set to false to disable range checking (danger!)
         * @param   pos     position given as a floating point value vector.  
         * @param   color   color to set.
         */
        template<bool CHECKRANGE = true> TGX_INLINE inline void drawPixelf(fVec2 pos, color_t color) { if ((CHECKRANGE) && (!isValid())) return; _drawPixel<CHECKRANGE>({ (int32_t)roundf(pos.x) , (int32_t)roundf(pos.y) }, color); }


        /**
         * Blend a pixel with the current pixel color.
         * 
         * @remark If type color_t has an alpha channel, then it is used for alpha blending.
         *
         * @tparam  CHECKRANGE  set to false to disable range checking (danger!)
         * @param   pos         The position.
         * @param   color       The color to blend.
         * @param   opacity     opacity multiplier from 0.0f (fully transparent) to 1.0f fully transparent.
         *                      if negative, then simple overwriting of color is used instead of blending.
         */
        template<bool CHECKRANGE = true> TGX_INLINE inline void drawPixel(iVec2 pos, color_t color, float opacity) { if ((CHECKRANGE) && (!isValid())) return; _drawPixel<CHECKRANGE,true>(pos, color, opacity); }
        
        
        /**
         * Blend a pixel with the current pixel color (floating point coord).
         * 
         * @remark 
         * - Version for floating point value coordinates. Recall that the image range is `[-0.5f, lx-0.5f]x[-0.5f,lx-0.5f]` and center of pixels are on the interger lattice.
         * - If type color_t has an alpha channel, then it is used for alpha blending.
         *
         * @tparam  CHECKRANGE  set to false to disable range checking (danger!)
         * @param   pos         The position given as a floating point value vector.
         * @param   color       The color to blend.
         * @param   opacity     opacity multiplier from 0.0f (fully transparent) to 1.0f fully transparent.
         *                      if negative, then simple overwriting of color is used instead of blending.
         */
        template<bool CHECKRANGE = true> TGX_INLINE inline void drawPixelf(fVec2 pos, color_t color, float opacity) { if ((CHECKRANGE) && (!isValid())) return; _drawPixel<CHECKRANGE, true>({ (int32_t)roundf(pos.x) , (int32_t)roundf(pos.y) }, color, opacity); }


        /**
         * Return the color of a pixel at a given position.
         *
         * @tparam  CHECKRANGE      If set to true, outside_color is returned when querying outside of the
         *                          image range. Otherwise nor range checking is performed (danger!)
         * @param   pos             The position to read. 
         * @param   outside_color   (Optional) color to return when querying outside the range.
         *
         * @returns The pixel color.
         */
        template<bool CHECKRANGE = true> TGX_INLINE inline color_t readPixel(iVec2 pos, color_t outside_color = color_t()) const { if ((CHECKRANGE) && (!isValid())) return outside_color; return _readPixel(pos, outside_color); }


        /**
         * Return the color of a pixel at a given position (floating point coord).
         *
         * @remark Version for floating point value coordinates. Recall that the image range is `[-0.5f, lx-0.5f]x[-0.5f,lx-0.5f]` and center of pixels are on the interger lattice.
         *         
         * @tparam  CHECKRANGE      If set to true, outside_color is returned when querying outside of the
         *                          image range. Otherwise nor range checking is performed (danger!)
         * @param   pos             The position to read given as a floating point value vector.
         * @param   outside_color   (Optional) color to return when querying outside the range.
         *
         * @returns The pixel color.
         */
        template<bool CHECKRANGE = true> TGX_INLINE inline color_t readPixelf(fVec2 pos, color_t outside_color = color_t()) const { if ((CHECKRANGE) && (!isValid())) return outside_color; return _readPixel({ (int32_t)roundf(pos.x) , (int32_t)roundf(pos.y) }, outside_color); }


        /**
         * Get a reference to a pixel (no range check!)
         *
         * const overload.
         * 
         * @param   pos The position.
         *
         * @returns a const reference to the pixel color.
         */
        TGX_INLINE inline const color_t& operator()(iVec2 pos) const { return _buffer[TGX_CAST32(pos.x) + TGX_CAST32(_stride) * TGX_CAST32(pos.y)]; }
        
        /**
         * Get a reference to a pixel (no range check!)
         *
         * non-const overload.
         * 
         * @param   pos The position.
         *
         * @returns a reference to the pixel color.
         */
        TGX_INLINE inline color_t& operator()(iVec2 pos) { return _buffer[TGX_CAST32(pos.x) + TGX_CAST32(_stride) * TGX_CAST32(pos.y)]; }


        /**
         * Get a reference to a pixel (no range check!)
         *
         * const overload.
         * 
         * @param   x   x-coordinate.
         * @param   y   y-coordinate.
         *
         * @returns a const reference to the pixel color.
         */
        TGX_INLINE inline const color_t& operator()(int x, int y) const { return _buffer[TGX_CAST32(x) + TGX_CAST32(_stride) * TGX_CAST32(y)]; }


        /**
         * Get a reference to a pixel (no range check!)
         *
         * non-const overload.
         * 
         * @param   x   x-coordinate.
         * @param   y   y-coordinate.
         *
         * @returns a reference to the pixel color.
         */
        TGX_INLINE inline color_t& operator()(int x, int y) { return _buffer[TGX_CAST32(x) + TGX_CAST32(_stride) * TGX_CAST32(y)];}


        /**
         * Iterate over all the pixels of the image.
         * 
         * non-const overload.
         * 
         * Iteration is performed from left to right (inner loop) and top to bottom (outer loop). The
         * callback function cb_fun() is called for each pixel and must have a signature compatible with:
         * 
         * `bool cb_fun(tgx::iVec2 pos, color_t & color)` 
         * 
         * where:
         * 
         * - `pos` is the position of the current pixel in the image
         * - `color` is a reference to the current pixel color.
         * - the callback must return true to continue the iteration and false to abort iteration.
         * 
         * @remark this method is particularly useful with lambdas, for example, to paint all black pixels
         * to red in a tgx::Image<tgx::RGB565> image `im`:
         * ```
         * im.iterate( [](tgx::iVec2 pos, tgx::RGB565 & color)
         *               { if (color == tgx::RGB565_Black) color = tgx::RGB565_Red; return true; } );
         * ```
         * 
         * @param   cb_fun  The callback function.
         */
        template<typename ITERFUN> void iterate(ITERFUN cb_fun);


        /**
         * Iterate over all the pixels of the image.
         *
         * const overload.
         * 
         * Iteration is performed from left to right (inner loop) and top to bottom (outer loop). The
         * callback function cb_fun() is called for each pixel and must have a signature compatible with:
         *
         * `bool cb_fun(tgx::iVec2 pos, const color_t & color)`
         * 
         * see void iterate(ITERFUN cb_fun) for details.
         *
         * @param   cb_fun  The callback function.
         */
        template<typename ITERFUN> void iterate(ITERFUN cb_fun) const;


        /**
         * Iterate over the pixel of the image inside a given region.
         * 
         * non-const overload.
         * 
         * The method calls the callback function for each pixel inside `B` (intersected with `imageBox()`). 
         * 
         * See void iterate(ITERFUN cb_fun) for details.
         *
         * @param   cb_fun  The callback function.
         * @param   B       The sub-box to iterate inside.
         */
        template<typename ITERFUN> void iterate(ITERFUN cb_fun, tgx::iBox2 B);


        /**
         * Iterate over the pixel of the image inside a given region.
         * 
         * const overload.
         * 
         * The method calls the callback function for each pixel inside `B` (intersected with `imageBox()`). 
         * 
         * See void iterate(ITERFUN cb_fun) for details.
         *
         * @param   cb_fun  The callback function.
         * @param   B       The sub-box to iterate inside.
         */
        template<typename ITERFUN> void iterate(ITERFUN cb_fun, tgx::iBox2 B) const;







    ///@}
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************
    /**
    * @name Image manipulation. 
    *       
    * Methods for blitting, converting, resizing and rotating images.
    */
    ///@{
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************


        /**
         * Blit/blend a sprite over this image at a given position.
         * 
         * Remark: If only part of the sprite should be blended onto this image, simply blit a sub-image
         * of the sprite like so: 'blend(sprite(subbox),upperleftpos, blend_op)'. This is very efficient
         * because no copy is performed when creating (shallow) sub-image.
         *
         * @param   sprite          The sprite image to blit.
         * @param   upperleftpos    Position of the upper left corner of the sprite in the image.
         * @param   opacity         (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or
         *                          negative to disable blending and simply overwrite the spite over the
         *                          image.
         */
        void blit(const Image<color_t>& sprite, iVec2 upperleftpos, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Blend a sprite at a given position on the image using a custom blending operator.
         * 
         * The blending operator 'blend_op' can be a function/functor/lambda. It takes as input the
         * color of the source (sprite) pixel and the color of the destination pixel and returns the
         * blended color. It must have a signature compatible with
         * 
         *                    color_t blend_op(color_t_src src, color_t dst)
         *
         * (the method can, in fact, return a color of any type but returning type color_t will improve
         * performance).
         * 
         * @remark
         * 1. The sprite can have a different color type from the image. 
         * 2. To perform "classical alpha blending", use the blit() method with an opacity parameter
         *    instead of this method as it will be faster.
         *
         * @param   sprite          The sprite image to blend.
         * @param   upperleftpos    Position of the upper left corner of the sprite in the image.
         * @param   blend_op        The blending operator.
         */
        template<typename color_t_src, typename BLEND_OPERATOR> 
        void blit(const Image<color_t_src>& sprite, iVec2 upperleftpos, const BLEND_OPERATOR& blend_op);


        /**
         * Blit/blend a rotated sprite over this image at a given position.
         * 
         * The rotation must be only quarter turns (0, 90, 180 or 270) degree. For blitting with arbitrary
         * rotation, use the blitScaledRotated() method instead.
         *
         * @param   sprite          The sprite image to blit.
         * @param   upperleftpos    Position inside this image of the upper left corner of the sprite (after rotation).
         * @param   angle           Counter-clockwise angle to rotate the sprite prior to blitting. Must be either 0, 90, 180 or 270.
         * @param   opacity         (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or
         *                          negative to disable blending and simply overwrite the spite over the
         *                          image.
         */
        void blitRotated(const Image<color_t> & sprite, iVec2 upperleftpos, int angle, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Blend a rotated sprite over this image at a given position using a custom blending operator.
         * 
         * The rotation must be only quarter turns (0, 90, 180 or 270) degree. For blitting with arbitrary
         * rotation, use the blitScaledRotated() method instead.
         * 
         * The blending operator 'blend_op' can be a function/functor/lambda. It takes as input the
         * color of the source (sprite) pixel and the color of the destination pixel and returns the
         * blended color. It must have a signature compatible with
         * 
         *                    color_t blend_op(color_t_src src, color_t dst)
         * 
         * (the method can, in fact, return a color of any type but returning type color_t will improve
         * performance).
         * 
         * @remark
         * 1. The sprite can have a different color type from the image.
         * 2. To perform "classical alpha blending", use the blit() method with an opacity parameter
         *    instead of this method as it will be faster.
         *
         * @param   sprite          The sprite image to blit.
         * @param   upperleftpos    Position inside this image of the upper left corner of the sprite (after rotation).
         * @param   angle           Counter-clockwise angle to rotate the sprite prior to blitting. Must be either 0, 90, 180 or 270.
         * @param   blend_op        The blending operator.
         */
        template<typename color_t_src, typename BLEND_OPERATOR>
        void blitRotated(const Image<color_t_src>& sprite, iVec2 upperleftpos, int angle, const BLEND_OPERATOR& blend_op);


        /**
         * Blend a sprite at a given position on this image with a given mask.
         * 
         * Sprite pixels with color `transparent_color` are treated as transparent hence are not copied
         * on the image. Other pixels are copied/blended with the destination image (after being
         * multiplied by the opacity factor).
         * 
         * @remark This method is especially useful when color_t does not have an alpha channel.
         *
         * @param   sprite              The sprite image to blit.
         * @param   transparent_color   The sprite color considered transparent.
         * @param   upperleftpos        Position of the upper left corner of the sprite in the image.
         * @param   opacity             (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or
         *                              negative to disable blending and simply draw the sprite over
         *                              the image.
         */
        void blitMasked(const Image<color_t>& sprite, color_t transparent_color, iVec2 upperleftpos, float opacity = 1.0f);


        /**
         * Reverse blitting. Copy part of the image into the sprite This is the inverse of the blit
         * operation.
         * 
         * @remark This method can be useful to 'save' a portion of the image into a sprite before
         * drawing/blitting something on it...
         *
         * @param [in,out]  dst_sprite      The sprite to save part of this image into.
         * @param           upperleftpos    Position of the upper left corner of the sprite in the image.
         */
        void blitBackward(Image<color_t>& dst_sprite, iVec2 upperleftpos) const;


        /**
         * Blit/blend a sprite onto this image after rescaling and rotation.
         * 
         * The anchor point 'anchor_src' in the sprite is mapped to 'anchor_im' in this image. The
         * rotation is performed in counter-clockwise direction around the anchor point.
         * 
         * @remark
         *   1. Positions are given using floating point values to allow for sub-pixel precision for
         *      smoother animation.
         *   2. The method uses bilinear interpolation for high quality rendering.
         *   3. The sprite image can have a different color type from this image.
         * 
         * @note When rotated, access to the sprite pixels colors is not linear anymore. For certain
         * orientations, this will yield very 'irregular' access to the sprite memory locations.
         * The method will try to improve this by splitting the sprite in smaller part to improve
         * cache access. If this does not help, try moving the sprite to a faster memory location
         *
         * @tparam  CACHE_SIZE  Size of the MCU cache when reading from flash. This value is indicative
         *                      and used to optimize cache access to flash memory. You may try changing
         *                      the default value it if drawing takes a long time...
         * @param   src_im          The sprite image to draw.
         * @param   anchor_src      Position of the anchor point in the sprite image.
         * @param   anchor_dst      Position of the anchor point in this image.
         * @param   scale           (Optional) Scaling factor (default 1.0f for no rescaling).
         * @param   angle_degrees   (Optional) The rotation angle in degrees (counter-clockwise, default 0 for no rotation).
         * @param   opacity         (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or
         *                          negative to disable blending and simply use overwrite.
         */
        template<typename color_t_src, int CACHE_SIZE = TGX_PROGMEM_DEFAULT_CACHE_SIZE>
        void blitScaledRotated(const Image<color_t_src> src_im, fVec2 anchor_src, fVec2 anchor_dst, float scale = 1.0f, float angle_degrees = 0.0f, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Blend a sprite onto this image after rescaling and rotation using a custom blending operator.
         * 
         * This is the same as the method above excpet that it uses a user defined blending operator
         * which takes as input the color of the source (sprite) pixel and the color of the destination
         * pixel and returns the blended color. It must have a signature compatible with
         * 
         *                    color_t blend_op(color_t_src src, color_t dst)
         * 
         * (the method can, in fact, return a color of any type but returning type color_t will improve
         * performance).  
         * 
         * See the method above for more details.
         *
         * @tparam  CACHE_SIZE      Size of the MCU cache when reading from flash. This value is
         *                          indicative and used to optimize cache access to flash memory. You may
         *                          try changing the default value it if drawing takes a long time...
         * @param   src_im          The sprite image to draw.
         * @param   anchor_src      Position of the anchor point in the sprite image.
         * @param   anchor_dst      Position of the anchor point in this image.
         * @param   scale           Scaling factor (counter-clockwise, default 1.0f for no rescaling).
         * @param   angle_degrees   The rotation angle in degrees (default 0 for no rotation).
         * @param   blend_op        blending operator.
         */
        template<typename color_t_src, typename BLEND_OPERATOR, int CACHE_SIZE = TGX_PROGMEM_DEFAULT_CACHE_SIZE>
        void blitScaledRotated(const Image<color_t_src>& src_im, fVec2 anchor_src, fVec2 anchor_dst, float scale, float angle_degrees, const BLEND_OPERATOR& blend_op);


        /**
         * Blend a sprite onto this image after rescaling and rotation and use a given color which is
         * treated as fully transparent.
         * 
         * The anchor point 'anchor_src' in the sprite is mapped to 'anchor_im' in this image. The
         * rotation is performed in counter-clockwise direction around the anchor point.
         * 
         * @remark
         *   1. Positions are given using floating point values to allow for sub-pixel precision for
         *      smoother animation.
         *   2. The method uses bilinear interpolation for high quality rendering.
         *   3. The sprite image can have a different color type from this image.
         *   4. This method is useful when the sprite color type does not have an alpha channel.
         * 
         * @note When rotated, access to the sprite pixels colors is not linear anymore. For certain
         * orientations, this will yield very 'irregular' access to the sprite memory locations.
         * The method will try to improve this by splitting the sprite in smaller part to improve
         * cache access. If this does not help, try moving the sprite to a faster memory location
         *
         * @tparam  CACHE_SIZE  Size of the MCU cache when reading from flash. This value is indicative
         *                      and used to optimize cache access to flash memory. You may try changing
         *                      the default value it if drawing takes a long time...
         * @param   src_im              The sprite image to draw.
         * @param   transparent_color   The sprite color considered transparent.
         * @param   anchor_src          Position of the anchor point in the sprite image.
         * @param   anchor_dst          Position of the anchor point in this image.
         * @param   scale               Scaling factor (default 1.0f for no rescaling).
         * @param   angle_degrees       The rotation angle in degrees (default 0 for no rotation).
         * @param   opacity             (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or
         *                              negative to disable blending and simply use overwrite.
         */
        template<typename color_t_src, int CACHE_SIZE = TGX_PROGMEM_DEFAULT_CACHE_SIZE>
        void blitScaledRotatedMasked(const Image<color_t_src>& src_im, color_t_src transparent_color, fVec2 anchor_src, fVec2 anchor_dst, float scale, float angle_degrees, float opacity = 1.0f);


        /**
         * Copy (or blend) the src image onto the destination image with resizing and color conversion.
         * 
         * Very useful for converting image of different size and color type !
         * 
         * @remark
         * 1. the source image is resized to match this image size. Bilinear interpolation is used to
         *    improve quality.  
         * 2. The source and destination image may have different color type. Conversion is automatic.
         *
         * @param   src_im  The image to copy into this image.
         * @param   opacity (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative to
         *                  disable blending and simply use overwrite. 
         */
        template<typename src_color_t> void copyFrom(const Image<src_color_t>& src_im, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Blend the src image onto the destination image with resizing and color conversion.
         * 
         * Same as above but uses a user-defined blending operator to combine src over the existing
         * image.
         * 
         * The blending operator 'blend_op' can be a function/functor/lambda. It takes as input the
         * color of the source (sprite) pixel and the color of the destination pixel and returns the
         * blended color. It must have a signature compatible with
         * 
         *                    color_t blend_op(color_t_src src, color_t dst)
         * 
         * (the method can, in fact, return a color of any type but returning type color_t will improve
         * performance).  
         * 
         * @remark
         * 1. the source image is resized to match this image size. Bilinear interpolation is used to
         *    improve quality.
         * 2. The source and destination image may have different color type. Conversion is automatic.
         * 3. To perform simple copy or classical alpha blending, use the copyFrom() method with an
         *    opacity parameter instead as it will be faster.
         *    
         * @param   src_im      Source image to copy onto this image.
         * @param   blend_op    The blending operator.
         */
        template<typename src_color_t, typename BLEND_OPERATOR> void copyFrom(const Image<src_color_t>& src_im, const BLEND_OPERATOR& blend_op);


        /**
         * Copy the source image pixels into this image, reducing it by half in the process.
         * 
         * The method ignores the last row/column for odd dimensions larger than 1. Resizing is done by
         * averaging the color of the 4 neighbour pixels.
         * 
         * This image must be large enough to accomodate the reduced image otherwise the method returns
         * without doing anything. The reduced image is copied from the top-left corner of this image.
         * 
         * @note This is an old method. Use blitScaledRotated() instead.
         *
         * @param   src_image   the source image.
         *
         * @returns a subimage of this image that contain the reduced image.
         */
        Image<color_t> copyReduceHalf(const Image<color_t> & src_image);


        /**
         * Reduce this image by half.
         * 
         * Use the same memory buffer and keep the same stride. Resizing is done by averaging the color
         * of the 4 neighbour pixels.
         *
         * @returns a sub-image of this image that contains the reduce image.
         */
        Image<color_t> reduceHalf();


        /**
         * Convert this image to another type.
         * 
         * Performs 'in place' conversion of the image using the same memory buffer.
         * 
         * @note The image returned may have a different stride from this image !
         * 
         * @warning 
         * 1. In place conversion only possible if `sizeof(color_dst)` divides `sizeof(color_t)`. Otherwise
         * a compile time error is raised. For conversion between any two color types, one can use `copyFrom()`.
         * 2. When the method returns, this image dimensions/stride are not unchange but the content of its memory
         * buffer has been modified. 
         * 
         * @tparam  color_dst   Color type to convert the image into.
         *
         * @returns A new image with the same content and using the same memory framebuffer but converted to type `color_dest`.
         */
        template<typename color_dst>
        Image<color_dst> convert();



    ///@}
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************
    /**
    * @name Drawing primitives: filling a region.
    *       
    * Method for filling the whole (or just a region of the) image with a given color.  
    */
    ///@{
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************



        /**
         * Fill the whole image with a single color.
         * 
         * same as `clear(color)`
         *  
         * @param   color   The color to use.
         */
        void fillScreen(color_t color);

        /**
         * Fill the whole image with a single color
         * 
         * Same as `fillScreen(color)`.
         *
         * @param   color   The color to use.
         */
        void clear(color_t color);


        /**
         * Fill the whole image with a vertical color gradient between two colors.
         * 
         * Interpolation takes place in RGB color space (even if color_t is HSV).
         *
         * @param   top_color       color at the top of the image.
         * @param   bottom_color    color at the bottom of the image.
         */
        void fillScreenVGradient(color_t top_color, color_t bottom_color);


        /**
         * Fill the whole screen with an horizontal color gradient between two colors.
         * 
         * Interpolation takes place in RGB color space (even if color_t is HSV).
         *
         * @param   left_color  color on the left side of the image.
         * @param   right_color color on the right side of the image.
         */
        void fillScreenHGradient(color_t left_color, color_t right_color);


        /**
         * 'Flood fill' a 4-connected region of the image.
         * 
         * Recolor the unicolor component containing position `start_pos` with the color `new_color`.
         * 
         * The template parameter can be adjusted to specify the size (in bytes) allocated on the stack.
         * If the algorithm runs out of space, it stops without completing the filling (and return -1 to
         * indicate failure). Otherwise, the method returns the max number of bytes used on the stack
         * during the filling.
         *
         * @tparam  STACK_SIZE  size allocated on the stack (in bytes).
         * @param   start_pos   Start position. The color to replace is the color at that position.
         * @param   new_color   New color to use.
         *
         * @returns return the max stack used (in bytes) during the algorithm. Return -1 if we run out of memory (in
         *          this case the method returns early without completing the full filling.
         */
        template<int STACK_SIZE = 1024> int fill(iVec2 start_pos, color_t new_color);


        /**
         * 'Flood fill' a 4-connected region of the image.
         * 
         * Recolor the connected component containing position `startpos` whose boundary is delimited by
         * `border_color`.
         * 
         * The template parameter can be adjusted to specify the size (in bytes) allocated on the stack.
         * If the algorithm runs out of space, it stops without completing the filling (and return -1 to
         * indicate failure). Otherwise, the method returns the max number of bytes used on the stack
         * during the filling.
         * 
         * @note During the algorithm, `new_color` is treated the same as `border_color` and will also
         * block the filling procedure when encountered.
         *
         * @tparam  STACK_SIZE  size allocated on the stack (in bytes).
         * @param   start_pos       Start position.
         * @param   border_color    border color that delimits the connected component to fill.
         * @param   new_color       New color to use.
         *
         * @returns return the max stack used (in bytes) during the algorithm. Return -1 if we run out of memory (in
         *          this case the method returns early without completing the full filling.
         */
        template<int STACK_SIZE = 1024> int fill(iVec2 start_pos, color_t border_color, color_t new_color);



    ///@}
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************
    /**
    * @name Drawing primitives: lines.
    *       
    * FAST/REGULAR METHODS
    */
    ///@{
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************



        /**
         * Draw an vertical segment of `h` pixels starting at `pos`.
         *
         * @param   pos     position of the top endpoint of the segment.
         * @param   h       number of pixels in the segment.
         * @param   color   color to use.
         * @param   opacity opacity multiplier when blending (in [0.0f, 1.0f]) or negative to disable
         *                  blending and simply use overwritting.
         */
        void drawFastVLine(iVec2 pos, int h, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Draw an horizontal segment of `w` pixels starting at pos.
         *
         * @param   pos     position of the left endpoint of the segment.
         * @param   w       number of pixels in the segment.
         * @param   color   color to use.
         * @param   opacity opacity multiplier when blending (in [0.0f, 1.0f]) or negative to disable
         *                  blending and simply use overwritting.
         */
        void drawFastHLine(iVec2 pos, int w, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Draw a line segment between two points (uses Bresenham's algorithm).
         *
         * @param   P1      The first point.
         * @param   P2      The second point.
         * @param   color   color to use.
         * @param   opacity (Optional) opacity multiplier when blending (in [0.0f, 1.0f]) or negative to
         *                  disable blending and simply use overwritting.
         */
        void drawLine(iVec2 P1, iVec2 P2, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Draw a line segment between two points (using Bresenham's algorithm).
         * Same as drawLine() but specify also which endpoints should be drawn. 
         *
         * @param   P1      The first point.
         * @param   drawP1  True to draw the pixel at endpoint P1.
         * @param   P2      The second point.
         * @param   drawP2  True to draw the pixel at endpoint P2.
         * @param   color   color to use.
         * @param   opacity (Optional) opacity multiplier when blending (in [0.0f, 1.0f]) or negative to
         *                  disable blending and simply use overwritting.
         */
        void drawSegment(iVec2 P1, bool drawP1, iVec2 P2, bool drawP2, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


    ///@}
    //*************************************************************************************************************
    /**
    * @name Drawing primitives: lines (AA).
    *       
    * HIGH QUALITY DRAWING
    */
    ///@{
    //*************************************************************************************************************



        /**
         * Draw a line segment between two points [**High quality**]
         * 
         * @note High quality drawing with anti-aliasing and sub-pixel precision. 
         * 
         * @param   P1      The first point.
         * @param   P2      The second point.
         * @param   color   color to use.
         * @param   opacity opacity multiplier between 0.0f and 1.0f (default).
         */
        void drawLineAA(fVec2 P1, fVec2 P2, color_t color, float opacity = 1.0f);


        /**
         * Draw a thick line segment between two points [**High quality**].
         * 
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @attention this method superseeds the old `drawWideLine()` method.
         *
         * @param   P1          The first point.
         * @param   P2          The second point.
         * @param   line_width  Width of the line.
         * @param   end_P1      specify how the line extremity at P1 should be drawn (c.f. `tgx::EndPath`)
         * @param   end_P2      specify how the line extremity at P2 should be drawn (c.f. `tgx::EndPath`)
         * @param   color       color to use.
         * @param   opacity     (Optional) opacity multiplier between 0.0f and 1.0f (default).
         */
        void drawThickLineAA(fVec2 P1, fVec2 P2, float line_width, EndPath end_P1, EndPath end_P2, color_t color, float opacity = 1.0f);


        /**
         * Draw a from `P1` to `P2` with with respective wideness `line_width_P1` and `line_width_P2` at both ends [**High quality**].
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @param   P1              first end point.
         * @param   P2              second end point.
         * @param   line_width_P1   Width of the wedge at P1.
         * @param   end_P1          specify how the line extremity at P1 should be drawn (c.f. `tgx::EndPath`)
         * @param   line_width_P2   Width of the wedge at P2.
         * @param   end_P2          specify how the line extremity at P2 should be drawn (c.f. `tgx::EndPath`)
         * @param   color           color to use.
         * @param   opacity         (Optional) opacity multiplier between 0.0f and 1.0f (default).
         */
        void drawWedgeLineAA(fVec2 P1, fVec2 P2, float line_width_P1, EndPath end_P1, float line_width_P2, EndPath end_P2, color_t color, float opacity = 1.0f);



#ifndef DOXYGEN_EXCLUDE



    ///@}
    /**
    * @name Drawing primitives: lines. (DEPRECATED METHODS)
    *
    * @warning These method are still available for compatibility but will be removed 'soon'...
    */
    ///@{


        /**
        * DEPRECATED: Use drawThickLineAA() instead.
        * 
        * @warning This method will be removed soon...
        */
        DEPRECATED("Use method drawThickLineAA() instead.") 
        void drawWideLine(fVec2 PA, fVec2 PB, float w, color_t color, float opacity)  { drawThickLineAA(PA, PB, w, END_ROUNDED, END_ROUNDED, color, opacity); }


        /**
        * DEPRECATED: Use drawWedgeLineAA() instead.
        *
        * @warning This method will be removed soon...
        */
        DEPRECATED("Use method drawWedgeLineAA() instead.")
        void drawWedgeLine(fVec2 PA, fVec2 PB, float aw, float bw, color_t color, float opacity) { drawWedgeLineAA(PA, PB, 2*aw, END_ROUNDED, 2*bw, END_ROUNDED, color, opacity); }


        /**
        * DEPRECATED: Use fillCircleAA() instead.
        *
        * @warning This method will be removed soon...
        */
        DEPRECATED("Use method fillCircleAA() instead")
        void drawSpot(fVec2 center, float r, color_t color, float opacity) { fillCircleAA(center, r, color, opacity); }


#endif




    ///@}
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************
    /**
    * @name Drawing primitives: rectangles.
    *
    * FAST/REGULAR METHODS
    */
    ///@{
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************



        /**
         * Draw a rectangle.
         *
         * @param   B       Box that delimits the rectangle to draw.
         * @param   color   rectangle outline color.
         * @param   opacity (Optional) opacity multiplier when blending (in [0.0f, 1.0f] or negative to
         *                  disable blending.
         */
        void drawRect(const iBox2 & B, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Draw a rectangle with a thick outline.
         *
         * @param   B           Box that delimits the rectangle to draw.
         * @param   thickness   thickness of the border (going 'inside' the rectangle)
         * @param   color       rectangle outline color.
         * @param   opacity     (Optional) opacity multiplier when blending (in [0.0f, 1.0f] or negative
         *                      to disable blending.
         */
        void drawThickRect(const iBox2& B, int thickness, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Draw a filled rectangle.
         *
         * @param   B       box that delimits the rectangle to draw.
         * @param   color   rectangle color.
         * @param   opacity (Optional) opacity multiplier when blending (in [0.0f, 1.0f]) or negative to
         *                  disable blending.
         */
        void fillRect(const iBox2 & B, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Draw a filled rectangle with a thick border of a possibly different color.
         *
         * @param   B               box that delimits the rectangle to draw.
         * @param   thickness       thickness of the border (going 'inside the rectangle)
         * @param   color_interior  color for the rectangle interior.
         * @param   color_border    color for the rectangle border.
         * @param   opacity         (Optional) opacity multiplier when blending (in [0.0f, 1.0f]) or
         *                          negative to disable blending.
         */
        void fillThickRect(const iBox2& B, int thickness, color_t color_interior, color_t color_border, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Draw a rectangle filled with an horizontal gradient of colors
         *
         * @param   B           box that delimits the rectangle to draw.
         * @param   color_left  color on the left side.
         * @param   color_right color on the right side.
         * @param   opacity     (Optional) opacity multiplier when blending (in [0.0f, 1.0f]) or negative
         *                      to disable blending.
         */
        void fillRectHGradient(iBox2 B, color_t color_left, color_t color_right, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Draw a rectangle filled with a vertical gradient of colors
         *
         * @param   B               box that delimits the rectangle to draw.
         * @param   color_top       color on the top side.
         * @param   color_bottom    color on the bottom side.
         * @param   opacity         (Optional) opacity multiplier when blending (in [0.0f, 1.0f]) or
         *                          negative to disable blending.
         */
        void fillRectVGradient(iBox2 B, color_t color_top, color_t color_bottom, float opacity = TGX_DEFAULT_NO_BLENDING);



    ///@}
    //*************************************************************************************************************
    /**
    * @name Drawing primitives: rectangles. (AA)
    *       
    * HIGH QUALITY DRAWING       
    *
    * @remark
    * 1. Sub-pixel precision and anti-aliasing is usually not desirable
    * when drawing rectangle. The method below should not be
    * needed in usual circonstance (but may be useful for creating
    * very smooth animations).
    *
    * 2. Recall that pixel centers are at integer values (and the full
    * image range is [-0.5, lx - 0.5] x [-0.5, lx-0.5]), therefore,
    * giving a fVec2 box with integer values to the method below will
    * create aliasing along the edges of the rectangle...
    *
    */
    ///@{
    //*************************************************************************************************************




        /**
         * Draw a filled rectangle with a thick border [**High quality**].
         * 
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @warning This is probably not the method you want to use: if you just want to draw a nice
         * rectangle with a thick border, try drawThickRect() instead.
         *
         * @param   B           Box representing the outer boundary of the rectangle.
         * @param   thickness   thickness of the boundary (going 'inside' the rectangle)
         * @param   color       color to use.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void drawThickRectAA(const fBox2& B, float thickness, color_t color, float opacity = 1.0f);


        /**
         * Draw a filled rectangle [**High quality**].
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @warning This is probably not the method you want to use: if you just want to draw a nice
         * filled rectangle, try fillRect() instead.
         *
         * @param   B       Box representing the rectngle to daw.
         * @param   color   color.
         * @param   opacity (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void fillRectAA(const fBox2& B, color_t color, float opacity = 1.0f);


        /**
         * Draw a filled rectangle with a thick border of a different color [**High quality**].
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @warning This is probably not the method you want to use: if you just want to draw a nice
         * filled rectangle with a thick border, try fillThickRect() instead.
         *
         * @param   B               Box representing the outer boundary of the rectangle
         * @param   thickness       thickness of the boundary (going 'inside' the rectangle)
         * @param   color_interior  color for the interior
         * @param   color_border    color for the boundary
         * @param   opacity         (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void fillThickRectAA(const fBox2& B, float thickness, color_t color_interior, color_t color_border, float opacity = 1.0f);
 






    ///@}
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************
    /**
    * @name Drawing primitives: rounded rectangles.
    *
    * FAST/REGULAR METHODS
    */
    ///@{
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************



        /**
         * Draw a rounded rectangle in box `B` with radius `corner_radius`.
         *
         * @param   B               box that delimits the rectangle to draw.
         * @param   corner_radius   corner radius.
         * @param   color           rectangle color.
         * @param   opacity         (Optional) opacity multiplier when blending (in [0.0f, 1.0f]) or
         *                          negative to disable blending.
         */
        void drawRoundRect(const iBox2& B, int corner_radius, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Draw a filled rounded rectangle in box `B` with radius `corner_radius`.
         *
         * @param   B               box that delimits the rectangle to draw.
         * @param   corner_radius   corner radius.
         * @param   color           rectangle color.
         * @param   opacity         (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or
         *                          negative to disable blending.
         */
        void fillRoundRect(const iBox2& B, int corner_radius, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


    ///@}
    //*************************************************************************************************************
    /**
    * @name Drawing primitives: rounded rectangles (AA).
    *       
    * HIGH QUALITY DRAWING
    */
    ///@{
    //*************************************************************************************************************


        /**
         * Draw a rounded rectangle [**High quality**].
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @param   B               box that delimits the rectangle to draw.
         * @param   corner_radius   corner radius.
         * @param   color           color.
         * @param   opacity         (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void drawRoundRectAA(const fBox2& B, float corner_radius, color_t color, float opacity = 1.0f);


        /**
         * Draw a rounded rectangle with a thick border [**High quality**].
         * 
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @param   B               box that delimits the rectangle to draw.
         * @param   corner_radius   corner radius.
         * @param   thickness       thickness going 'inside' the rounded rectangle (should be >1)
         * @param   color           color.
         * @param   opacity         (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void drawThickRoundRectAA(const fBox2& B, float corner_radius, float thickness, color_t color, float opacity = 1.0f);


        /**
         * Draw a filled rounded rectangle [**High quality**].
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @param   B               box that delimits the rectangle to draw.
         * @param   corner_radius   corner radius.
         * @param   color           color.
         * @param   opacity         (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void fillRoundRectAA(const fBox2& B, float corner_radius, color_t color, float opacity = 1.0f);


        /**
         * Draw a filled rounded rectangle with a thick border of another color [**High quality**].
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @param   B               box that delimits the rectangle to draw.
         * @param   corner_radius   corner radius.
         * @param   thickness       thickness going 'inside' the rounded rectangle (should be >1)
         * @param   color_interior  color.
         * @param   color_border    The color border.
         * @param   opacity         (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void fillThickRoundRectAA(const fBox2& B, float corner_radius, float thickness, color_t color_interior, color_t color_border, float opacity = 1.0f);







    ///@}
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************
    /**
    * @name Drawing primitives: triangles.
    *
    * FAST/REGULAR METHODS
    */
    ///@{
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************


        /**
         * Draw a triangle.
         *
         * @param   P1      first vertex.
         * @param   P2      second vertex.
         * @param   P3      third vertex.
         * @param   color   The color to use.
         * @param   opacity Opacity multiplier when blending (in [0.0f, 1.0f]) or negative to disable
         *                  blending and simply use overwrite.
         */
        void drawTriangle(const iVec2& P1, const iVec2& P2, const iVec2& P3, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);



        /**
         * Draw a filled triangle with different colors for the outline and the interior.
         *
         * @param   P1              first vertex.
         * @param   P2              second vertex.
         * @param   P3              third vertex.
         * @param   interior_color  color for the interior.
         * @param   outline_color   color for the outline.
         * @param   opacity         (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or
         *                          negative to disable blending and simply use overwrite.
         */
        void fillTriangle(const iVec2& P1, const iVec2& P2, const iVec2& P3, color_t interior_color, color_t outline_color, float opacity = TGX_DEFAULT_NO_BLENDING);




    ///@}
    //*************************************************************************************************************
    /**
    * @name Drawing primitives: triangles (AA).
    *       
    * HIGH QUALITY DRAWING
    */
    ///@{
    //*************************************************************************************************************



        /**
         * Draw a triangle [**High quality**].
         *
         * @param   P1      first vertex.
         * @param   P2      second vertex.
         * @param   P3      third vertex.
         * @param   color   The color.
         * @param   opacity (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void drawTriangleAA(fVec2 P1, fVec2 P2, fVec2 P3, color_t color, float opacity = 1.0f);


        /**
         * Draw a triangle with a thick border [**High quality**].
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @param   P1          first vertex.
         * @param   P2          second vertex.
         * @param   P3          third vertex.
         * @param   thickness   The thickness (going 'inside' the triangle).
         * @param   color       The color.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void drawThickTriangleAA(fVec2 P1, fVec2 P2, fVec2 P3, float thickness, color_t color, float opacity = 1.0f);


        /**
         * Draw a filled triangle [**High quality**].
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @param   P1      first vertex.
         * @param   P2      second vertex.
         * @param   P3      third vertex.
         * @param   color   The color.
         * @param   opacity (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void fillTriangleAA(fVec2 P1, fVec2 P2, fVec2 P3, color_t color, float opacity = 1.0f);


        /**
         * Draw a filled triangle with a thick border of a different color [**High quality**]. 
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @param   P1              first vertex.
         * @param   P2              second vertex.
         * @param   P3              third vertex.
         * @param   thickness       thickness of the border (going 'inside' the triangle). 
         * @param   color_interior  interior color.
         * @param   color_border    boundary color.
         * @param   opacity         (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void fillThickTriangleAA(fVec2 P1, fVec2 P2, fVec2 P3, float thickness, color_t color_interior, color_t color_border, float opacity = 1.0f);




    ///@}
    //*************************************************************************************************************
    /**
    * @name Drawing primitives: triangles (SPECIAL METHODS).
    *       
    * ADVANCED METHODS USING THE 3D RASTERIZER BACKEND. 
    *
    * @remark
    *   1. These method use sub-pixel precision (but not anti-aliasing).
    *   2. Sprite images and gradient colors can have different types from the destination (ie this) image.
    */
    ///@{
    //*************************************************************************************************************



        /**
         * Draw a triangle with gradient color specified by the colors on its vertices.
         * 
         * @param   P1      First triangle vertex.
         * @param   P2      Second triangle vertex.
         * @param   P3      Third triangle vertex.
         * @param   colorP1 color at the first vertex.
         * @param   colorP2 color at the second vertex.
         * @param   colorP3 color at the third vertex.
         * @param   opacity Opacity multiplier when blending (in [0.0f, 1.0f]) or negative to disable
         *                  blending and simply use overwrite.
         */
        template<typename color_alt>
        void drawGradientTriangle(fVec2 P1, fVec2 P2, fVec2 P3, color_alt colorP1, color_alt colorP2, color_alt colorP3, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Draw textured triangle onto the image.
         * 
         * The texture is mapped onto the triangle using bilinear filtering.
         *
         * @warning For particular orientations of triangles, access to texture pixels is
         * highly non-linear. For texture stored in slow memory (e.g. flash), this
         * can cause huge slowdown because the read cache becomes basically useless.
         * To overcome this problem:
         * 1. Move the texture to a faster memory location before drawing.
         * 2. Or tessellate the triangle into smaller triangles so that the
         *    memory data for each triangle fit into the cache.
         *
         * @param   src_im  the image texture to map onto the triangle.
         * @param   srcP1   coords of point 1 on the texture.
         * @param   srcP2   coords of point 2 on the texture.
         * @param   srcP3   coords of point 3 on the texture.
         * @param   dstP1   coords of point 1 on this image.
         * @param   dstP2   coords of point 2 on this image.
         * @param   dstP3   coords of point 3 on this image.
         * @param   opacity Opacity multiplier when blending (in [0.0f, 1.0f]) or negative to disable
         *                  blending and simply use overwrite.
         */
        template<typename color_t_tex>
        void drawTexturedTriangle(const Image<color_t_tex>& src_im, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Blend a textured triangle onto the image. 
         * 
         * The texture is mapped onto the triangle using bilinear filtering.
         * 
         * The method takes as input a user defined 'blending operator' with allow to create fancy
         * effects. The blending operator 'blend_op' can be a function/functor/lambda. It takes as 
         * input the color of the source (sprite) pixel and the color of the destination pixel and 
         * returns the blended color. It must have a signature compatible with
         *
         *                    color_t blend_op(color_t_src src, color_t dst)
         *
         * (the method can, in fact, return a color of any type but returning type color_t will 
         * improve performance).
         *
         * @warning For particular orientations of triangles, access to texture pixels is
         * highly non-linear. For texture stored in slow memory (e.g. flash), this
         * can cause huge slowdown because the read cache becomes basically useless.
         * To overcome this problem:
         * 1. Move the texture to a faster memory location before drawing.
         * 2. Or tessellate the triangle into smaller triangles so that the
         *    memory data for each triangle fit into the cache.
         * 
         * @param   src_im  the image texture to map onto the triangle.
         * @param   srcP1   coords of point 1 on the texture.
         * @param   srcP2   coords of point 2 on the texture.
         * @param   srcP3   coords of point 3 on the texture.
         * @param   dstP1   coords of point 1 on this image.
         * @param   dstP2   coords of point 2 on this image.
         * @param   dstP3   coords of point 3 on this image.
         * @param   blend_op    the blending operator
         */
        template<typename color_t_tex, typename BLEND_OPERATOR>
        void drawTexturedTriangle(const Image<color_t_tex>& src_im, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, const BLEND_OPERATOR& blend_op);


        /**
         * Blend textured triangle on the image while combining it with a color gradient.
         * 
         * The texture is mapped onto the triangle using bilinear filtering.
         *
         * @warning For particular orientations of triangles, access to texture pixels is
         * highly non-linear. For texture stored in slow memory (e.g. flash), this
         * can cause huge slowdown because the read cache becomes basically useless.
         * To overcome this problem:
         * 1. Move the texture to a faster memory location before drawing.
         * 2. Or tessellate the triangle into smaller triangles so that the
         *    memory data for each triangle fit into the cache.
         *
         * @param   src_im  the image/texture to map onto the triangle.
         * @param   srcP1   coords of point 1 on the texture.
         * @param   srcP2   coords of point 2 on the texture.
         * @param   srcP3   coords of point 3 on the texture.
         * @param   dstP1   coords of point 1 on this image.
         * @param   dstP2   coords of point 2 on this image.
         * @param   dstP3   coords of point 3 on this image.
         * @param   C1      color gradient multiplier at point 1.
         * @param   C2      color gradient multiplier at point 2.
         * @param   C3      color gradient multiplier at point 3.
         * @param   opacity (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative to
         *                  disable blending and simply use overwrite.
         */
        template<typename color_t_tex>
        void drawTexturedGradientTriangle(const Image<color_t_tex>& src_im, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, color_t_tex C1, color_t_tex C2, color_t_tex C3, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Blend textured triangle with a transparency mask (ie a specific color is treated as fully transparent)
         *
         * @warning For particular orientations of triangles, access to texture pixels is
         * highly non-linear. For texture stored in slow memory (e.g. flash), this
         * can cause huge slowdown because the read cache becomes basically useless.
         * To overcome this problem:
         * 1. Move the texture to a faster memory location before drawing.
         * 2. Or tessellate the triangle into smaller triangles so that the
         *    memory data for each triangle fit into the cache.
         *
         * @param   src_im              the image/texture to map onto the triangle.
         * @param   transparent_color   the color considered transparent in the source texture.
         * @param   srcP1               coords of point 1 on the texture.
         * @param   srcP2               coords of point 2 on the texture.
         * @param   srcP3               coords of point 3 on the texture.
         * @param   dstP1               coords of point 1 on this image.
         * @param   dstP2               coords of point 2 on this image.
         * @param   dstP3               coords of point 3 on this image.
         * @param   opacity             The opacity multiplier between 0.0f (transparent) and 1.0f (opaque).
         */
        template<typename color_t_tex>
        void drawTexturedMaskedTriangle(const Image<color_t_tex>& src_im, color_t_tex transparent_color, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, float opacity = 1.0f);


        /**
         * Blend textured triangle with a transparency mask (ie a specific color is treated as fully
         * transparent) and blend it with a color gradient...
         *
         * Ok, this one is really overkill :p 
         *
         * @warning For particular orientations of triangles, access to texture pixels is
         * highly non-linear. For texture stored in slow memory (e.g. flash), this
         * can cause huge slowdown because the read cache becomes basically useless.
         * To overcome this problem:
         * 1. Move the texture to a faster memory location before drawing.
         * 2. Or tessellate the triangle into smaller triangles so that the
         *    memory data for each triangle fit into the cache.
         *
         * @param   src_im              the image/texture to map onto the triangle.
         * @param   transparent_color   the color considered transparent in the source texture.
         * @param   srcP1               coords of point 1 on the texture.
         * @param   srcP2               coords of point 2 on the texture.
         * @param   srcP3               coords of point 3 on the texture.
         * @param   dstP1               coords of point 1 on this image.
         * @param   dstP2               coords of point 2 on this image.
         * @param   dstP3               coords of point 3 on this image.
         * @param   C1                  color gradient multiplier at point 1.
         * @param   C2                  color gradient multiplier at point 2.
         * @param   C3                  color gradient multiplier at point 3.
         * @param   opacity             The opacity multiplier between 0.0f (transparent) and 1.0f (opaque).
         */
        template<typename color_t_tex>
        void drawTexturedGradientMaskedTriangle(const Image<color_t_tex>& src_im, color_t_tex transparent_color, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, color_t_tex C1, color_t_tex C2, color_t_tex C3, float opacity = 1.0f);







    ///@}
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************
    /**
    * @name Drawing primitives: quads.
    *
    * FAST/REGULAR METHODS
    */
    ///@{
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************


        /**
         * Draw a  quad.
         *
         * @param   P1      The first vertex.
         * @param   P2      The second vertex.
         * @param   P3      The third vertex.
         * @param   P4      The fourth vertex.
         * @param   color   The color to use.
         * @param   opacity (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative to
         *                  disable blending and simply use overwrite.
         */
        void drawQuad(iVec2 P1, iVec2 P2, iVec2 P3, iVec2 P4, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * draw a filled quad.
         * 
         * @warning The quad must be **convex**.
         *
         * @param   P1      first vertex.
         * @param   P2      second vertex.
         * @param   P3      third vertex.
         * @param   P4      fourth vertex.
         * @param   color   color.
         * @param   opacity (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative to
         *                  disable blending and simply use overwrite.
         */
        void fillQuad(iVec2 P1, iVec2 P2, iVec2 P3, iVec2 P4, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);




    ///@}
    //*************************************************************************************************************
    /**
    * @name Drawing primitives: quads (AA).
    *
    * HIGH QUALITY DRAWING
    */
    ///@{
    //*************************************************************************************************************


        /**
         * Draw a quad [**High quality**].
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @param   P1      The first vertex.
         * @param   P2      The second vertex.
         * @param   P3      The third vertex.
         * @param   P4      The fourth vertex.
         * @param   color   The color to use.
         * @param   opacity (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void drawQuadAA(fVec2 P1, fVec2 P2, fVec2 P3, fVec2 P4, color_t color, float opacity = 1.0f);


        /**
         * Draw a quad with a thick border [**High quality**].
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @param   P1          The first vertex.
         * @param   P2          The second vertex.
         * @param   P3          The third vertex.
         * @param   P4          The fourth vertex.
         * @param   thickness   The thickness (going 'inside' the quad)
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void drawThickQuadAA(fVec2 P1, fVec2 P2, fVec2 P3, fVec2 P4, float thickness, color_t color, float opacity = 1.0f);


        /**
         * Draw a filled quad [**High quality**].
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @warning: the quad must be **convex**.
         *
         * @param   P1      first vertex.
         * @param   P2      second vertex.
         * @param   P3      third vertex.
         * @param   P4      fourth vertex.
         * @param   color   color.
         * @param   opacity (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void fillQuadAA(fVec2 P1, fVec2 P2, fVec2 P3, fVec2 P4, color_t color, float opacity = 1.0f);


        /**
         * Draw a filled quad with a thick border of a different color [**High quality**].
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @warning: the quad must be **convex**.
         *
         * @param   P1              first vertex.
         * @param   P2              second vertex.
         * @param   P3              third vertex.
         * @param   P4              fourth vertex.
         * @param   thickness       The thickness (going 'inside' the quad)
         * @param   color_interior  interior color.
         * @param   color_border    boundary color.
         * @param   opacity         (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void fillThickQuadAA(fVec2 P1, fVec2 P2, fVec2 P3, fVec2 P4, float thickness, color_t color_interior, color_t color_border, float opacity = 1.0f);



    ///@}
    //*************************************************************************************************************
    /**
    * @name Drawing primitives: quads (SPECIAL METHODS).
    *       
    * ADVANCED METHODS USING THE 3D RASTERIZER BACKEND. 
    *
    * @remark
    *   1. These method use sub-pixel precision (but not anti-aliasing).
    *   2. Sprite images and gradient colors can have different types from the destination (ie this) image.
    */
    ///@{
    //*************************************************************************************************************



        /**
        * Draw a quad with a gradient color specified by the color at the four vertices.
        *
        * See drawGradientTriangle() for details.
        */
        template<typename color_alt>
        void drawGradientQuad(fVec2 P1, fVec2 P2, fVec2 P3, fVec2 P4, color_alt colorP1, color_alt colorP2, color_alt colorP3, color_alt colorP4, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
        * Draw a textured quad using bilinear filtering.
        *
        * See drawTexturedTriangle() for details.
        */
        template<typename color_t_tex>
        void drawTexturedQuad(const Image<color_t_tex>& src_im, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 srcP4, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, fVec2 dstP4, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
        * Draw a textured quad with bilinear filtering and a custom blending operator.
        *
        * See drawTexturedTriangle() for more details.
        */
        template<typename color_t_tex, typename BLEND_OPERATOR>
        void drawTexturedQuad(const Image<color_t_tex>& src_im, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 srcP4, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, fVec2 dstP4, const BLEND_OPERATOR& blend_op);


        /**
        * Draw a textured quad using bilinear filtering combined with a color gradient.
        *
        * See drawTexturedGradientTriangle() for details.
        */
        template<typename color_t_tex>
        void drawTexturedGradientQuad(const Image<color_t_tex>& src_im, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 srcP4, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, fVec2 dstP4, color_t_tex C1, color_t_tex C2, color_t_tex C3, color_t_tex C4, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
        * Draw a textured quad using bilinear filtering and with a mask (ie a fixed transparent color).
        *
        * See drawTexturedMaskedTriangle() for details.
        */
        template<typename color_t_tex>
        void drawTexturedMaskedQuad(const Image<color_t_tex>& src_im, color_t_tex transparent_color, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 srcP4, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, fVec2 dstP4, float opacity = 1.0f);


        /**
        * Draw a textured quad using bilinear filtering and with a mask (ie a fixed transparent color) and combined with a color gradient.
        *
        * See drawTexturedGradientMaskedTriangle() for details.
        */
        template<typename color_t_tex>
        void drawTexturedGradientMaskedQuad(const Image<color_t_tex>& src_im, color_t_tex transparent_color, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 srcP4, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, fVec2 dstP4, color_t_tex C1, color_t_tex C2, color_t_tex C3, color_t_tex C4, float opacity = 1.0f);









    ///@}
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************
    /**
    * @name Drawing primitives: polylines.
    *
    * FAST/REGULAR METHODS
    */
    ///@{
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************



        /**
         * Draw a polyline ie a sequence of consecutif segments [P0,P1] , [P1,P2],,, [Pn-1,Pn].
         *
         * @param   nbpoints    number of points in tabPoints.
         * @param   tabPoints   array of points.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative
         *                      to disable blending and simply use overwrite.
         */
        void drawPolyline(int nbpoints, const iVec2 tabPoints[], color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Draw a polyline ie a sequence of consecutif segments [P0,P1] , [P1,P2],,, [Pn-1,Pn].
         * 
         * Points are queried in order P0, P1,... using a functor callback which must have a signature
         * compatible with: 
         * ```
         *                    bool next_point(iVec2 & P)
         * ```         
         * The callback must store the next point in the reference `P` and return:
         * - `true`: if there are additional points to plot after this one.
         * - `false`: if this is the last point.
         *
         * @param   next_point  callback functor that provides the list of points.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative
         *                      to disable blending and simply use overwrite.
         */
        template<typename FUNCTOR_NEXT>
        void drawPolyline(FUNCTOR_NEXT next_point, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);





    ///@}
    //*************************************************************************************************************
    /**
    * @name Drawing primitives: polylines (AA).
    *
    * HIGH QUALITY DRAWING
    */
    ///@{
    //*************************************************************************************************************




        /**
         * Draw a polyline ie a sequence of consecutif segments [P0,P1] , [P1,P2],,, [Pn-1,Pn] (**High quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @param   nbpoints    number of points in tabPoints.
         * @param   tabPoints   array of points.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void drawPolylineAA(int nbpoints, const fVec2 tabPoints[], color_t color, float opacity = 1.0f);


        /**
         * Draw a polyline ie a sequence of consecutif segments [P0,P1] , [P1,P2],,, [Pn-1,Pn] (**High quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * Points are queried in order P0, P1,... using a functor callback which must have a signature
         * compatible with:
         * ```
         *                    bool next_point(fVec2 & P)
         * ```
         * The callback must store the next point in the reference `P` and return:
         * - `true`: if there are additional points to plot after this one.
         * - `false`: if this is the last point.
         *
         * @param   next_point  callback functior that privdes the list of points.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        template<typename FUNCTOR_NEXT>
        void drawPolylineAA(FUNCTOR_NEXT next_point, color_t color, float opacity = 1.0f);


        /**
         * Draw a thick polyline ie a sequence of consecutif thick segments [P0,P1] , [P1,P2],,, [Pn-1,Pn] (**High quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @param   nbpoints    number of points in tabPoints.
         * @param   tabPoints   array of points.
         * @param   thickness   thickness of the polyline.
         * @param   end_P0      specify how the extremity P0 should be drawn (c.f. tgx::EndPath enum)
         * @param   end_Pn      specify how the extremity Pn should be drawn (c.f. tgx::EndPath enum)
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void drawThickPolylineAA(int nbpoints, const fVec2 tabPoints[], float thickness, EndPath end_P0, EndPath end_Pn, color_t color, float opacity = 1.0f);



        /**
         * Draw a thick polyline ie a sequence of consecutif thick segments [P0,P1] , [P1,P2],,, [Pn-1,Pn] (**High quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * Points are queried in order P0, P1,... using a functor callback which must have a signature
         * compatible with: 
         * ```
         *                    bool next_point(fVec2 & P)
         * ```         
         * The callback must store the next point in the reference `P` and return:
         * - `true`: if there are additional points to plot after this one.
         * - `false`: if this is the last point.
         *
         * @param   next_point  callback functior that provides the list of points.
         * @param   thickness   thickness of the polyline.
         * @param   end_P0      specify how the extremity P0 should be drawn (c.f. tgx::EndPath enum)
         * @param   end_Pn      specify how the extremity Pn should be drawn (c.f. tgx::EndPath enum)
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        template<typename FUNCTOR_NEXT>
        void drawThickPolylineAA(FUNCTOR_NEXT next_point, float thickness, EndPath end_P0, EndPath end_Pn, color_t color, float opacity = 1.0f);






    ///@}
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************
    /**
    * @name Drawing primitives: polygons.
    *
    * FAST/REGULAR METHODS
    */
    ///@{
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************



        /**
         * Draw a closed polygon with vertices [P0,P2,,, PN].
         *
         * @param   nbpoints    number of points in tabPoints.
         * @param   tabPoints   array of points of the polygon.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative
         *                      to disable blending and simply use overwrite.
         */
        void drawPolygon(int nbpoints, const iVec2 tabPoints[], color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Draw a closed polygon with vertices [P0,P2,,, PN].
         *
         * Points are queried in order P0, P1,... using a functor callback which must have a signature
         * compatible with:
         * ```
         *                    bool next_point(iVec2 & P)
         * ```
         * The callback must store the next point in the reference `P` and return:
         * - `true`: if there are additional points to plot after this one.
         * - `false`: if this is the last point.
         *
         * @param   next_point  callback functor that provides the list of points.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative
         *                      to disable blending and simply use overwrite.
         */
        template<typename FUNCTOR_NEXT>
        void drawPolygon(FUNCTOR_NEXT next_point, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Draw a filled polygon with vertices [P0,P2,,, PN]
         * 
         * @warning The polygon must be **convex** (or at least star-shaped from its center of mass).
         *
         * @param   nbpoints    number of points in tabPoints.
         * @param   tabPoints   array of points of the polygon.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative
         *                      to disable blending and simply use overwrite.
         */
        void fillPolygon(int nbpoints, const iVec2 tabPoints[], color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Draw a filled polygon with vertices [P0,P2,,, PN]
         *
         * Points are queried in order P0, P1,... using a functor callback which must have a signature
         * compatible with:
         * ```
         *                    bool next_point(iVec2 & P)
         * ```
         * The callback must store the next point in the reference `P` and return:
         * - `true`: if there are additional points to plot after this one.
         * - `false`: if this is the last point **AND THEN THE FUNCTOR MUST RESET BACK THE FIRST POINT !**
         *
         * @warning In order to draw the polygon correctly, all points must be queried TWICE so
         * the callback must reset to the first point after returning false. 
         *           
         * @warning The polygon must be **convex** (or at least star-shaped from its center of mass).
         *
         * @param   next_point  callback functor that provides the list of points.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative
         *                      to disable blending and simply use overwrite.
         */
        template<typename FUNCTOR_NEXT>
        void fillPolygon(FUNCTOR_NEXT next_point, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);





    ///@}
    //*************************************************************************************************************
    /**
    * @name Drawing primitives: polygons (AA).
    *
    * HIGH QUALITY DRAWING
    */
    ///@{
    //*************************************************************************************************************



        /**
         * Draw a closed polygon with vertices [P0,P2,,, PN] (**High quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @param   nbpoints    number of points in tabPoints.
         * @param   tabPoints   array of points of the polygon.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative
         *                      to disable blending and simply use overwrite.
         */
        void drawPolygonAA(int nbpoints, const fVec2 tabPoints[], color_t color, float opacity = 1.0f);


        /**
         * Draw a closed polygon with vertices [P0,P2,,, PN] (**High quality**).
         * 
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * Points are queried in order P0, P1,... using a functor callback which must have a signature
         * compatible with:
         * ```
         *                    bool next_point(fVec2 & P)
         * ```
         * The callback must store the next point in the reference `P` and return:
         * - `true`: if there are additional points to plot after this one.
         * - `false`: if this is the last point.
         *
         * @param   next_point  callback functor that provides the list of points.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative
         *                      to disable blending and simply use overwrite.
         */
        template<typename FUNCTOR_NEXT>
        void drawPolygonAA(FUNCTOR_NEXT next_point, color_t color, float opacity = 1.0f);


        /**
         * Draw a polygon with thick lines (**High quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @param   nbpoints    number of points in tabPoints.
         * @param   tabPoints   array of points that delimit the polygon outer boundary.
         * @param   thickness   The thickness of the polygon boundary (going inside the polygon).
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void drawThickPolygonAA(int nbpoints, const fVec2 tabPoints[], float thickness, color_t color, float opacity = 1.0f);


        /**
         * Draw a polygon with thick lines  (**High quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * Points are queried in order P0, P1,... using a functor callback which must have a signature
         * compatible with:
         * ```
         *                    bool next_point(fVec2 & P)
         * ```
         * The callback must store the next point in the reference `P` and return:
         * - `true`: if there are additional points to plot after this one.
         * - `false`: if this is the last point **AND THEN THE FUNCTOR MUST RESET BACK THE FIRST POINT !**
         *
         * @warning In order to draw the polygon correctly, all points must be queried TWICE so
         * the callback must reset to the first point after returning false. 
         *
         * @param   next_point  callback functor that provides the list of points delimiting the outer
         *                      boundary of the polygon.
         * @param   thickness   The thickness of the polygon boundary (going inside the polygon).
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        template<typename FUNCTOR_NEXT>
        void drawThickPolygonAA(FUNCTOR_NEXT next_point, float thickness, color_t color, float opacity = 1.0f);


        /**
         * Draw a filled polygon  (**High quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @warning The polygon must be **convex** (or at least star-shaped from its center of mass).
         *
         * @param   nbpoints    number of points in tabPoints.
         * @param   tabPoints   array of points of the polygon.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void fillPolygonAA(int nbpoints, const fVec2 tabPoints[], color_t color, float opacity = 1.0f);


        /**
         * Draw a filled polygon (**High quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * Points are queried in order P0, P1,... using a functor callback which must have a signature
         * compatible with:
         * ```
         *                    bool next_point(fVec2 & P)
         * ```
         * The callback must store the next point in the reference `P` and return:
         * - `true`: if there are additional points to plot after this one.
         * - `false`: if this is the last point **AND THEN THE FUNCTOR MUST RESET BACK THE FIRST POINT !**
         *
         * @warning In order to draw the polygon correctly, all points must be queried TWICE so
         * the callback must reset to the first point after returning false.
         *
         * @warning The polygon must be **convex** (or at least star-shaped from its center of mass).
         *
         * @param   next_point  callback functor that provides the list of points delimiting the polygon.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        template<typename FUNCTOR_NEXT>
        void fillPolygonAA(FUNCTOR_NEXT next_point,  color_t color, float opacity = 1.0f);


        /**
         * Draw a filled polygon with a thick border of a different color (**High quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @param   nbpoints        number of points in tabPoints.
         * @param   tabPoints       array of points of the polygon
         * @param   thickness       The thickness of the polygon boundary (going 'inside' the polygon).
         * @param   interior_color  color for the interior
         * @param   border_color    color for the thick boundary
         * @param   opacity         (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void fillThickPolygonAA(int nbpoints, const fVec2 tabPoints[], float thickness, color_t interior_color, color_t border_color, float opacity = 1.0f);


        /**
         * Draw a filled polygon with a thick border of a different color (**High quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * Points are queried in order P0, P1,... using a functor callback which must have a signature
         * compatible with:
         * ```
         *                    bool next_point(fVec2 & P)
         * ```
         * The callback must store the next point in the reference `P` and return:
         * - `true`: if there are additional points to plot after this one.
         * - `false`: if this is the last point **AND THEN THE FUNCTOR MUST RESET BACK THE FIRST POINT !**
         *
         * @warning In order to draw the polygon correctly, all points must be queried TWICE so
         * the callback must reset to the first point after returning false.
         *
         * @warning The polygon must be **convex** (or at least star-shaped from its center of mass).
         *
         * @param   next_point      callback functor that provides the list of points delimiting the polygon outer boundary.
         * @param   thickness       The thickness of the polygon boundary (going 'inside' the polygon)
         * @param   interior_color  color for the interior
         * @param   border_color    color for the thick boundary
         * @param   opacity         (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        template<typename FUNCTOR_NEXT>
        void fillThickPolygonAA(FUNCTOR_NEXT next_point, float thickness, color_t interior_color, color_t border_color, float opacity = 1.0f);










    ///@}
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************
    /**
    * @name Drawing primitives: circles.
    *
    * FAST/REGULAR METHODS
    */
    ///@{
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************




        /**
         * Draw a circle.
         *
         * @param   center  Circle center.
         * @param   r       radius.
         * @param   color   color.
         * @param   opacity (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative to
         *                  disable blending.
         */
        void drawCircle(iVec2 center, int r, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Draw a filled circle with different colors for outline and interior.
         *
         * @param   center          Circle center.
         * @param   r               radius.
         * @param   interior_color  color for the interior.
         * @param   outline_color   color for the outline.
         * @param   opacity         (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or
         *                          negative to disable blending.
         */
        void fillCircle(iVec2 center, int r, color_t interior_color, color_t outline_color, float opacity = TGX_DEFAULT_NO_BLENDING);



    ///@}
    //*************************************************************************************************************
    /**
    * @name Drawing primitives: circles (AA).
    *
    * HIGH QUALITY DRAWING
    */
    ///@{
    //*************************************************************************************************************



        /**
         * Draw a circle (**high quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @param   center  Circle center.
         * @param   r       radius.
         * @param   color   color
         * @param   opacity (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void drawCircleAA(fVec2 center, float r, color_t color, float opacity = 1.0f);


        /**
         * Draw a circle with a thick border (**high quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @param   center      Circle center.
         * @param   r           external radius (the internal radius is r - thickness).
         * @param   thickness   thickness (going inside the circle).
         * @param   color       color.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void drawThickCircleAA(fVec2 center, float r, float thickness, color_t color, float opacity = 1.0f);


        /**
         * Draw a filled circle (**high quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @param   center  Circle center.
         * @param   r       radius.
         * @param   color   color.
         * @param   opacity (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void fillCircleAA(fVec2 center, float r, color_t color, float opacity = 1.0f);


        /**
         * Draw a filled circle with a thick border of a different color (**high quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @param   center          Circle center.
         * @param   r               external radius (the internal radius is r - thickness).
         * @param   thickness       thickness 'going inside the circle'
         * @param   color_interior  color of the interior
         * @param   color_border    color of the boundary
         * @param   opacity         (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void fillThickCircleAA(fVec2 center, float r, float thickness, color_t color_interior, color_t color_border, float opacity = 1.0f);





    ///@}
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************
    /**
    * @name Drawing primitives: arcs and pies (AA).
    *
    * HIGH QUALITY DRAWING
    */
    ///@{
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************


        /**
         * Draw a circle arc (**high quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * the arc is drawn moving clockwise from angle_start until reaching angle_end.
         * | angle (in degrees)  | position |
         * |---------------------|----------|
         * |          0          |  12AM    |
         * |         90          |   3AM    |
         * |        180          |   6AM    |
         * |        270          |   9AM    |
         *
         * @param   center      circle center position.
         * @param   r           circle radius.
         * @param   angle_start angle in degrees of the begigining of the arc.
         * @param   angle_end   angle in degrees of the end  of the arc.
         * @param   color       color to use.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void drawCircleArcAA(fVec2 center, float r, float angle_start, float angle_end, color_t color, float opacity = 1.0f);


        /**
         * Draw a circle arc with a thick border (**high quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * the arc is drawn moving clockwise from angle_start until reaching angle_end.
         * | angle (in degrees)  | position |
         * |---------------------|----------|
         * |          0          |  12AM    |
         * |         90          |   3AM    |
         * |        180          |   6AM    |
         * |        270          |   9AM    |
         * 
         * @param   center      circle center position.
         * @param   r           circle radius.
         * @param   angle_start angle in degrees of the begigining of the arc.
         * @param   angle_end   angle in degrees of the end  of the arc.
         * @param   thickness   The thickness of the arc, going 'inside' the circle.
         * @param   color       color to use.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void drawThickCircleArcAA(fVec2 center, float r, float angle_start, float angle_end, float thickness, color_t color, float opacity = 1.0f);


        /**
         * Draw a filled circle sector/slice/pie (**high quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * the arc is drawn moving clockwise from angle_start until reaching angle_end.
         * | angle (in degrees)  | position |
         * |---------------------|----------|
         * |          0          |  12AM    |
         * |         90          |   3AM    |
         * |        180          |   6AM    |
         * |        270          |   9AM    |
         *
         * @param   center      circle center position.
         * @param   r           circle radius.
         * @param   angle_start angle in degrees of the begigining of the arc.
         * @param   angle_end   angle in degrees of the end  of the arc.
         * @param   color       color to use.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void fillCircleSectorAA(fVec2 center, float r, float angle_start, float angle_end, color_t color, float opacity = 1.0f);


        /**
         * Draw a filled circle sector/slice/pie with a thick border of a different color (**high quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * the arc is drawn moving clockwise from angle_start until reaching angle_end.
         * | angle (in degrees)  | position |
         * |---------------------|----------|
         * |          0          |  12AM    |
         * |         90          |   3AM    |
         * |        180          |   6AM    |
         * |        270          |   9AM    |
         *
         * @param   center          circle center position.
         * @param   r               circle radius.
         * @param   angle_start     angle in degrees of the begigining of the arc.
         * @param   angle_end       angle in degrees of the end  of the arc.
         * @param   thickness       border thickness going 'inside' the circle.
         * @param   color_interior  color for the interior.
         * @param   color_border    color for the boundary.
         * @param   opacity         (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void fillThickCircleSectorAA(fVec2 center, float r, float angle_start, float angle_end, float thickness, color_t color_interior, color_t color_border, float opacity = 1.0f);






    ///@}
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************
    /**
    * @name Drawing primitives: ellipses
    *
    * FAST/REGULAR METHODS
    */
    ///@{
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************



        /**
         * Draw an ellipse.
         *
         * @param   center      Circle center.
         * @param   radiuses    radiuses along the x and y axis.
         * @param   color       color.
         * @param   opacity     (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative
         *                      to disable blending.
         */
        void drawEllipse(iVec2 center, iVec2 radiuses, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Draw a filled ellipse with different colors for the outline and the interior.
         *
         * @param   center          Circle center.
         * @param   radiuses        radiuses along the x and y axis.
         * @param   interior_color  color for the interior.
         * @param   outline_color   color for the outline.
         * @param   opacity         (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or
         *                          negative to disable blending.
         */
        void fillEllipse(iVec2 center, iVec2 radiuses, color_t interior_color, color_t outline_color, float opacity = TGX_DEFAULT_NO_BLENDING);




    ///@}
    //*************************************************************************************************************
    /**
    * @name Drawing primitives: ellipses (AA).
    *
    * HIGH QUALITY DRAWING
    */
    ///@{
    //*************************************************************************************************************



        /**
         * Draw an ellipse (**high quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @param   center      Ellipse center.
         * @param   radiuses    radiuses along the x and y axis.
         * @param   color       color.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void drawEllipseAA(fVec2 center, fVec2 radiuses, color_t color, float opacity = 1.0f);


        /**
         * Draw an ellipse with a thick border (**high quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @param   center      Ellipse center.
         * @param   radiuses    external radiuses along the x and y axis.
         * @param   thickness   thickness going 'inside' the ellipse
         * @param   color       color.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void drawThickEllipseAA(fVec2 center, fVec2 radiuses, float thickness, color_t color, float opacity = 1.0f);


        /**
         * Draw a filled ellipse (**high quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @param   center      Ellipse center.
         * @param   radiuses    radiuses along the x and y axis.
         * @param   color       color.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void fillEllipseAA(fVec2 center, fVec2 radiuses, color_t color, float opacity = 1.0f);


        /**
         * Draw a filled ellipse with a thick border of a different color (**high quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @param   center          ellipse center.
         * @param   radiuses        radiuses along the x and y axis.
         * @param   thickness       border thickness going 'inside' the ellipse
         * @param   color_interior  color of the interior.
         * @param   color_border    color of the boundary.
         * @param   opacity         (Optional) Opacity multiplier in [0.0f, 1.0f].
         */
        void fillThickEllipseAA(fVec2 center, fVec2 radiuses, float thickness, color_t color_interior, color_t color_border, float opacity = 1.0f);





    ///@}
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************
    /**
    * @name Drawing primitives: Bezier curves.
    *
    * FAST/REGULAR METHODS
    */
    ///@{
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************




        /**
         * Draw a quadratic (rational) Bezier curve.
         *
         * @param   P1      Start point.
         * @param   P2      End point.
         * @param   PC      Control point.
         * @param   wc      Control point weight (must be > 0). Fastest for wc=1.
         * @param   drawP2  true to draw the endpoint P2.
         * @param   color   The color to use.
         * @param   opacity Opacity multiplier when blending (in [0.0f, 1.0f]) or negative to disable
         *                  blending and simply use overwrite.
         */
        void drawQuadBezier(iVec2 P1, iVec2 P2, iVec2 PC, float wc, bool drawP2, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Draw a cubic Bezier curve.
         *
         * @param   P1      Start point.
         * @param   P2      End point.
         * @param   PA      first control point.
         * @param   PB      second control point.
         * @param   drawP2  true to draw the endpoint P2.
         * @param   color   The color to use.
         * @param   opacity Opacity multiplier when blending (in [0.0f, 1.0f]) or negative to disable
         *                  blending and simply use overwrite.
        **/
        void drawCubicBezier(iVec2 P1, iVec2 P2, iVec2 PA, iVec2 PB, bool drawP2, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Draw a quadratic spline interpolating between a given set of points.
         * 
         * The template parameter SPLINE_MAX_POINTS defines the maximum number of points that a spline
         * can have. Can be increased if needed (but increase memory allocated on stack when the method
         * is called).
         *
         * @tparam  SPLINE_MAX_POINTS   Max number of point interpolation point in the sline. Adjust if
         *                              needed (but uses more memory on stack)
         * @param   nbpoints        number of points in the spline Must be smaller or equal to
         *                          SPLINE_MAX_POINTS.
         * @param   tabPoints       the array of points to interpolate.
         * @param   draw_last_point true to draw the last point.
         * @param   color           The color to use.
         * @param   opacity         (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or
         *                          negative to disable blending and simply use overwrite.
         */
        template<int SPLINE_MAX_POINTS = 32>
        void drawQuadSpline(int nbpoints, const iVec2 tabPoints[], bool draw_last_point, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Draw a cubic spline interpolating between a given set of points. 
         * 
         * The template parameter SPLINE_MAX_POINTS defines the maximum number of points that a spline
         * can have. Can be increased if needed (but increase memory allocated on stack when the method
         * is called).
         *
         * @tparam  SPLINE_MAX_POINTS   Max number of point interpolation point in the sline. Adjust if
         *                              needed (but uses more memory on stack)
         * @param   nbpoints        number of points in the spline Must be smaller or equal to
         *                          SPLINE_MAX_POINTS.
         * @param   tabPoints       the array of points to interpolate.
         * @param   draw_last_point true to draw the last point.
         * @param   color           The color to use.
         * @param   opacity         (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or
         *                          negative to disable blending and simply use overwrite.
         */
        template<int SPLINE_MAX_POINTS = 32>
        void drawCubicSpline(int nbpoints, const iVec2 tabPoints[], bool draw_last_point, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Draw a closed quadratic spline interpolating between a given set of points.
         * 
         * The template parameter SPLINE_MAX_POINTS defines the maximum number of points that a spline
         * can have. Can be increased if needed (but increase memory allocated on stack when the method
         * is called).
         *
         * @tparam  SPLINE_MAX_POINTS   Max number of point interpolation point in the sline. Adjust if
         *                              needed (but uses more memory on stack)
         * @param   nbpoints    number of points in the spline Must be smaller or equal to
         *                      SPLINE_MAX_POINTS.
         * @param   tabPoints   the array of points to interpolate.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative
         *                      to disable blending and simply use overwrite.
         */
        template<int SPLINE_MAX_POINTS = 32>
        void drawClosedSpline(int nbpoints, const iVec2 tabPoints[], color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);





    ///@}
    //*************************************************************************************************************
    /**
    * @name Drawing primitives: Bezier curves (AA).
    *
    * HIGH QUALITY DRAWING
    */
    ///@{
    //*************************************************************************************************************


        /**
         * Draw a thick quadratic (rational) Bezier curve (**high quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @param   P1          Start point.
         * @param   P2          End point.
         * @param   PC          Control point.
         * @param   wc          Control point weight (must be >0).
         * @param   thickness   thickness of the curve.
         * @param   end_P1      specify how the extremity P0 should be drawn (c.f. tgx::EndPath enum)
         * @param   end_P2      specify how the extremity P2 should be drawn (c.f. tgx::EndPath enum)
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative
         *                      to disable blending and simply use overwrite.
         */
        void drawThickQuadBezierAA(fVec2 P1, fVec2 P2, fVec2 PC, float wc, float thickness, EndPath end_P1, EndPath end_P2, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Draw a thick cubic Bezier curve (**high quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * @param   P1          Start point.
         * @param   P2          End point.
         * @param   PA          first control point.
         * @param   PB          second control point.
         * @param   thickness   thickness of the curve.
         * @param   end_P1      specify how the extremity P0 should be drawn (c.f. tgx::EndPath enum)
         * @param   end_P2      specify how the extremity P2 should be drawn (c.f. tgx::EndPath enum)
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative
         *                      to disable blending and simply use overwrite.
         */
        void drawThickCubicBezierAA(fVec2 P1, fVec2 P2, fVec2 PA, fVec2 PB, float thickness, EndPath end_P1, EndPath end_P2, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Draw a thick quadratic spline interpolating between a given set of points (**high quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * The template parameter SPLINE_MAX_POINTS defines the maximum number of points that a spline
         * can have. Can be increased if needed (but increase memory allocated on stack when the method
         * is called).
         *
         * @tparam  SPLINE_MAX_POINTS   Max number of point interpolation point in the sline. Adjust if
         *                              needed (but uses more memory on stack)
         * @param   nbpoints    number of points in the spline Must be smaller or equal to
         *                      SPLINE_MAX_POINTS.
         * @param   tabPoints   the array of points to interpolate.
         * @param   thickness   thickness of the curve.
         * @param   end_P0      specify how the extremity P0 should be drawn (c.f. tgx::EndPath enum)
         * @param   end_Pn      specify how the extremity Pn should be drawn (c.f. tgx::EndPath enum)
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative
         *                      to disable blending and simply use overwrite.
         */
        template<int SPLINE_MAX_POINTS = 32>
        void drawThickQuadSplineAA(int nbpoints, const fVec2 tabPoints[], float thickness, EndPath end_P0, EndPath end_Pn, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Draw a thick cubic spline interpolating between a given set of points (**high quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * The template parameter SPLINE_MAX_POINTS defines the maximum number of points that a spline
         * can have. Can be increased if needed (but increase memory allocated on stack when the method
         * is called).
         *
         * @tparam  SPLINE_MAX_POINTS   Max number of point interpolation point in the sline. Adjust if
         *                              needed (but uses more memory on stack)
         * @param   nbpoints    number of points in the spline Must be smaller or equal to
         *                      SPLINE_MAX_POINTS.
         * @param   tabPoints   the array of points to interpolate.
         * @param   thickness   thickness of the curve.
         * @param   end_P0      specify how the extremity P0 should be drawn (c.f. tgx::EndPath enum)
         * @param   end_Pn      specify how the extremity Pn should be drawn (c.f. tgx::EndPath enum)
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative
         *                      to disable blending and simply use overwrite.
         */
        template<int SPLINE_MAX_POINTS = 32>
        void drawThickCubicSplineAA(int nbpoints, const fVec2 tabPoints[], float thickness, EndPath end_P0, EndPath end_Pn, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Draw a thick closed quadratic spline interpolating between a given set of points (**high quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * The template parameter SPLINE_MAX_POINTS defines the maximum number of points that a spline
         * can have. Can be increased if needed (but increase memory allocated on stack when the method
         * is called).
         *
         * @tparam  SPLINE_MAX_POINTS   Max number of point interpolation point in the sline. Adjust if
         *                              needed (but uses more memory on stack)
         * @param   nbpoints    number of points in the spline Must be smaller or equal to
         *                      SPLINE_MAX_POINTS.
         * @param   tabPoints   the array of points to interpolate.
         * @param   thickness   thickness of the curve.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative
         *                      to disable blending and simply use overwrite.
         */
        template<int SPLINE_MAX_POINTS = 32>
        void drawThickClosedSplineAA(int nbpoints, const fVec2 tabPoints[], float thickness, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Fill a region delimited by a closed quadratic spline (**high quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * The template parameter SPLINE_MAX_POINTS defines the maximum number of points that a spline
         * can have. Can be increased if needed (but increase memory allocated on stack when the method
         * is called).
         *
         * @warning The region should be convex (or at least star-shape around center of mass). Even 
         * for a convex region, the drawing may not be perfect if there are sharp turns.
         *
         * @tparam  SPLINE_MAX_POINTS   Max number of point interpolation point in the sline. Adjust if
         *                              needed (but uses more memory on stack)
         * @param   nbpoints    number of points in the spline Must be smaller or equal to
         *                      SPLINE_MAX_POINTS.
         * @param   tabPoints   the array of points to interpolate.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative
         *                      to disable blending and simply use overwrite.
         */
        template<int SPLINE_MAX_POINTS = 32>
        void fillClosedSplineAA(int nbpoints, const fVec2 tabPoints[], color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Fill a region delimited by a closed thick quadratic spline where the interior and boundary can have different colors (**high quality**).
         *
         * @note High quality drawing with anti-aliasing and sub-pixel precision.
         *
         * The template parameter SPLINE_MAX_POINTS defines the maximum number of points that a spline
         * can have. Can be increased if needed (but increase memory allocated on stack when the method
         * is called).
         *
         * @warning 
         * 1. The region should be convex (or at least star-shape around center of mass). Even
         *    for a convex region, the drawing may not be perfect if there are sharp turns.
         * 2. If the shape is 'very irregular'. it might be better to draw a thick curve with
         *    drawThickClosedSplineAA() and with thickness (at least 3) and then use the fill()
         *    method to color the interior.
         *
         * @tparam  SPLINE_MAX_POINTS   Max number of point interpolation point in the sline. Adjust if
         *                              needed (but uses more memory on stack)
         * @param   nbpoints        number of points in the spline Must be smaller or equal to
         *                          SPLINE_MAX_POINTS.
         * @param   tabPoints       the array of points to interpolate.
         * @param   thickness       thickness of the curve.
         * @param   color_interior  The color to use.
         * @param   color_border    The color border.
         * @param   opacity         (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or
         *                          negative to disable blending and simply use overwrite.
         */
        template<int SPLINE_MAX_POINTS = 32>
        void fillThickClosedSplineAA(int nbpoints, const fVec2 tabPoints[], float thickness, color_t color_interior, color_t color_border, float opacity = TGX_DEFAULT_NO_BLENDING);




     

    ///@}
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************
    /**
    * @name Drawing primitives: Text
    *
    * The following font formats are supported:
    *
    * - AdafruitGFX                     (https://glenviewsoftware.com/projects/products/adafonteditor/adafruit-gfx-font-format)
    * - ILI9341_t3 v1                   (https://forum.pjrc.com/threads/54316-ILI9341_t-font-structure-format)
    * - ILI9341_t3 v23 (antialiased)    (https://github.com/projectitis/packedbdf/blob/master/packedbdf.md)
    *
    * @note tgx-font (https://github.com/vindar/tgx-font) contains a collection ILI9341_t3 v1 and v2 (antialiased) fonts
    *       that can be used directly with the methods below (and the instruction on how to convert a ttf font to  this format).
    */
    ///@{
    //*************************************************************************************************************
    //*************************************************************************************************************
    //*************************************************************************************************************



        /**
         * Query the height of a font.
         * 
         * overload for `GFXfont`.
         * 
         * @remark this method simply forwards the calls `tgx::fontHeight(const GFXfont & font)`.
         *
         * @param   font    The font.
         *
         * @returns the number of vertical pixels between two lines of text with this font.
         */
        int fontHeight(const GFXfont & font) const;


        /**
         * Query the height of a font
         *
         * overload for `ILI9341_t3_font_t`.
         *
         * @remark this method simply forwards the calls `tgx::fontHeight(const ILI9341_t3_font_t& font)`.
         *
         * @param   font    The font.
         *
         * @returns the number of vertical pixels between two lines of text with this font.
         */
        int fontHeight(const ILI9341_t3_font_t& font) const;


        /**
         * Compute the bounding box of a character.
         * 
         * overload for `ILI9341_t3_font_t`.
         *
         * @remark this method simply forwards the calls `tgx::measureChar(char c, iVec2 pos, const GFXfont& font, Anchor anchor, int* xadvance)`.
         *
         * @param           c           The character.
         * @param           pos         position of the anchor point in this image.
         * @param           font        The font to use.
         * @param           anchor      (Optional) location of the anchor with respect to the char
         *                              bounding box. (by default, this is the BASELINE|LEFT).
         * @param [in,out]  xadvance    If non-null, the number of pixel to advance horizontally after
         *                              drawing the char is stored here.
         *
         * @returns the bounding box of pixels occupied by the char draw with `font `when its chosen anchor is at `pos`.
         */
        iBox2 measureChar(char c, iVec2 pos, const GFXfont& font, Anchor anchor = DEFAULT_TEXT_ANCHOR, int* xadvance = nullptr) const;


        /**
         * Compute the bounding box of a character.
         * 
         * overload for `ILI9341_t3_font_t`.
         *
         * @remark this method simply forwards the calls `tgx::measureChar(char c, iVec2 pos, const ILI9341_t3_font_t& font, Anchor anchor, int* xadvance)`.
         *
         * @param           c           The character.
         * @param           pos         position of the anchor point in this image.
         * @param           font        The font to use.
         * @param           anchor      (Optional) location of the anchor with respect to the char
         *                              bounding box. (by default, this is the BASELINE|LEFT).
         * @param [in,out]  xadvance    If non-null, the number of pixel to advance horizontally after
         *                              drawing the char is stored here.
         *
         * @returns the bounding box of pixels occupied by the char draw with `font `when its chosen anchor is at `pos`.
         */
        iBox2 measureChar(char c, iVec2 pos, const ILI9341_t3_font_t& font, Anchor anchor = DEFAULT_TEXT_ANCHOR, int* xadvance = nullptr) const;


        /**
         * Compute the bounding box of a text.
         *
         * overload for `GFXfont`.
         *
         * @param   text                The text.
         * @param   pos                 position of the anchor point in the image.
         * @param   font                The font to use.
         * @param   anchor              (Optional) location of the anchor with respect to the text
         *                              bounding box. (by default, this is the BASELINE|LEFT).
         * @param   wrap_text           (Optional) True to wrap wrap text at the end of image.
         * @param   start_newline_at_0  (Optional) True to start a new line of text at x=0 and false to
         *                              start at x=pos.x.
         *
         * @returns the bounding box of pixels occupied by the `text` draw with `font` when its chosen anchor is at `pos`.
         */
        iBox2 measureText(const char * text, iVec2 pos, const GFXfont& font, Anchor anchor = DEFAULT_TEXT_ANCHOR, bool wrap_text = false, bool start_newline_at_0 = false) const;


        /**
         * Compute the bounding box of a text.
         *
         * overload for `ILI9341_t3_font_t`.
         *
         * @param   text                The text.
         * @param   pos                 position of the anchor point in the image.
         * @param   font                The font to use.
         * @param   anchor              (Optional) location of the anchor with respect to the text
         *                              bounding box. (by default, this is the BASELINE|LEFT).
         * @param   wrap_text           (Optional) True to wrap wrap text at the end of image.
         * @param   start_newline_at_0  (Optional) True to start a new line of text at x=0 and false to
         *                              start at x=pos.x.
         *
         * @returns the bounding box of pixels occupied by the `text` draw with `font` when its chosen anchor is at `pos`.
         */
        iBox2 measureText(const char * text, iVec2 pos, const ILI9341_t3_font_t& font, Anchor anchor = DEFAULT_TEXT_ANCHOR, bool wrap_text = false, bool start_newline_at_0 = false) const;


        /**
         * Draw a single character at position pos on the image and return the position for the next
         * character.
         *
         * overload for `GFXfont`.
         * 
         * @param   c       The character to draw.
         * @param   pos     Location of the anchor with respect to the char bounding box. (by default,
         *                  this is the BASELINE|LEFT).
         * @param   font    The font to use.
         * @param   color   The color.
         * @param   opacity (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative to
         *                  disable blending and simply use overwrite.
         *
         * @returns the position to draw the next char.
         */
        iVec2 drawChar(char c, iVec2 pos, const GFXfont& font, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Draw a single character at position pos on the image and return the position for the next
         * character.
         *
         * overload for `ILI9341_t3_font_t`.
         * 
         * @param   c       The character to draw.
         * @param   pos     Location of the anchor with respect to the char bounding box. (by default,
         *                  this is the BASELINE|LEFT).
         * @param   font    The font to use.
         * @param   color   The color.
         * @param   opacity (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative to
         *                  disable blending and simply use overwrite.
         *
         * @returns the position to draw the next char.
         */
        iVec2 drawChar(char c, iVec2 pos, const ILI9341_t3_font_t& font, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Draw a text at a given position with a given font.
         *
         * overload for `GFXFont`.
         *
         * @note use char '\\n' to changes line.
         *
         * @param   text    The text to draw.
         * @param   pos     Location of the anchor with respect to the char bounding box. (by default,
         *                  this is the BASELINE|LEFT).
         * @param   font    The font to use.
         * @param   color   The color.
         * @param   opacity (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative to
         *                  disable blending and simply use overwrite.
         *
         * @returns the position to use draw the next char after the text.
         */
        iVec2 drawText(const char* text, iVec2 pos, const GFXfont& font, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Draw a text at a given position with a given font.
         *
         * overload for `ILI9341_t3_font_t`.
         *
         * @note use char '\\n' to changes line.
         *
         * @param   text    The text to draw.
         * @param   pos     Location of the anchor with respect to the char bounding box. (by default,
         *                  this is the BASELINE|LEFT).
         * @param   font    The font to use.
         * @param   color   The color.
         * @param   opacity (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative to
         *                  disable blending and simply use overwrite.
         *
         * @returns the position to use draw the next char after the text.
         */
        iVec2 drawText(const char* text, iVec2 pos, const ILI9341_t3_font_t& font, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);



        /**
         * Advanced drawText method. Draw a text with a given font at a given position relative to a
         * given anchor point.
         *
         * overload for `GFXFont`.
         *
         * @note Use char '\\n' to changes line.
         *
         * @param   text                The text to draw.
         * @param   pos                 position to draw the text. This is the position to which will be
         *                              mapped the selected anchor point of the text.
         * @param   anchor              Select the anchor point of the text c.f. enum tgx::Anchor (the `drawText()` methods uses `BASELINE|LEFT`). 
         * @param   font                The font to use.
         * @param   wrap_text           True to wrap wrap text at the end of image. Wrapping occur per
         *                              character (not per word).
         * @param   start_newline_at_0  True to start a new line of text at position x=0 and false to
         *                              start at x=pos.x.
         * @param   color               The color.
         * @param   opacity             (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or
         *                              negative to disable blending and simply use overwrite.
         *
         * @returns the position to draw the next char (when using the same anchor location).
         */
        iVec2 drawTextEx(const char* text, iVec2 pos, Anchor anchor, const GFXfont& font, bool wrap_text, bool start_newline_at_0, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);


        /**
         * Advanced drawText method. Draw a text with a given font at a given position relative to a
         * given anchor point.
         *
         * overload for `ILI9341_t3_font_t`.
         *
         * @note Use char '\\n' to changes line.
         *
         * @param   text                The text to draw.
         * @param   pos                 position to draw the text. This is the position to which will be
         *                              mapped the selected anchor point of the text.
         * @param   anchor              Select the anchor point of the text c.f. enum tgx::Anchor (the `drawText()` methods uses `BASELINE|LEFT`). 
         * @param   font                The font to use.
         * @param   wrap_text           True to wrap wrap text at the end of image. Wrapping occur per
         *                              character (not per word).
         * @param   start_newline_at_0  True to start a new line of text at position x=0 and false to
         *                              start at x=pos.x.
         * @param   color               The color.
         * @param   opacity             (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or
         *                              negative to disable blending and simply use overwrite.
         *
         * @returns the position to draw the next char (when using the same anchor location).
         */
        iVec2 drawTextEx(const char* text, iVec2 pos, Anchor anchor, const ILI9341_t3_font_t& font, bool wrap_text, bool start_newline_at_0, color_t color, float opacity = TGX_DEFAULT_NO_BLENDING);





#ifndef DOXYGEN_EXCLUDE



    ///@}
    //*************************************************************************************************************
    /**
    * @name Drawing primitives: Text DEPRECATED METHODS.
    *
    * @warning These method are still available for compatibility but will be removed 'soon'...
    */
    //*************************************************************************************************************
    ///@{


        /**
        * DEPRECATED: Use the new signature drawText(text, pos, font, color opacity) or the new method drawTextEx() instead.
        */
        DEPRECATED("Use new signature: drawText(text, pos, font, color, opacity) or the new method drawTextEx()")
        iVec2 drawText(const char* text, iVec2 pos, color_t color, const ILI9341_t3_font_t& font, bool start_newline_at_0, float opacity = 1.0f) { return drawTextEx(text, pos, DEFAULT_TEXT_ANCHOR, font, false, start_newline_at_0, color, opacity); }


        /**
        * DEPRECATED: Use the new signature drawText(text, pos, font, color opacity) or the new method drawTextEx() instead.
        */
        DEPRECATED("Use new signature: drawText(text, pos, font, color, opacity) or the new method drawTextEx()")
        iVec2 drawText(const char* text, iVec2 pos, color_t color, const GFXfont& font, bool start_newline_at_0, float opacity = 1.0f) { return drawTextEx(text, pos, DEFAULT_TEXT_ANCHOR, font, false, start_newline_at_0, color, opacity); }


        /**
        * DEPRECATED: Use the new signature measureText(text,pos,font,anchor, wrap, newline_at_0) instead.
        */
        DEPRECATED("Use new signature measureText(text,pos,font,anchor, wrap, newline_at_0)")
        iBox2 measureText(const char* text, iVec2 pos, const GFXfont& font, bool start_newline_at_0) { return measureText(text, pos, font, DEFAULT_TEXT_ANCHOR, false, start_newline_at_0); }


        /**
        * DEPRECATED: Use the new signature measureText(text,pos,font,anchor, wrap, newline_at_0) instead.
        */
        DEPRECATED("Use new signature measureText(text,pos,font,anchor, wrap, newline_at_0)")
        iBox2 measureText(const char* text, iVec2 pos, const ILI9341_t3_font_t& font, bool start_newline_at_0) { return measureText(text, pos, font, DEFAULT_TEXT_ANCHOR, false, start_newline_at_0); }


#endif











    ///@}


    //**************************************************************************************************************************************************
    //**************************************************************************************************************************************************
    //**************************************************************************************************************************************************
    //
    //                                            IMPLEMENTATION
    //
    //
    // Don't you dare look below... this is private :-p
    //
    //***************************************************************************************************************************************************
    //***************************************************************************************************************************************************
    //***************************************************************************************************************************************************



private:

        bool _collision(); // For debug 

private:

        inline void _checkvalid() { if ((_lx <= 0) || (_ly <= 0) || (_stride < _lx) || (_buffer == nullptr)) setInvalid(); } // Make sure the image parameters are ok, else mark the image as invalid.



        /***************************************
        * COPY / RESIZE / BLIT
        ****************************************/

        static inline void _fast_memset(color_t* p_dest, color_t color, int32_t len);

        bool _blitClip(const Image& sprite, int& dest_x, int& dest_y, int& sprite_x, int& sprite_y, int& sx, int& sy);
        bool _blitClip(int sprite_lx, int sprite_ly, int& dest_x, int& dest_y, int& sprite_x, int& sprite_y, int& sx, int& sy);

        void _blit(const Image& sprite, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy);
        static void _blitRegion(color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy) { if ((size_t)pdest <= (size_t)psrc) _blitRegionUp(pdest, dest_stride, psrc, src_stride, sx, sy); else _blitRegionDown(pdest, dest_stride, psrc, src_stride, sx, sy); }
        static void _blitRegionUp(color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy);
        static void _blitRegionDown(color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy);

        void _blit(const Image& sprite, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy, float opacity);
        static void _blitRegion(color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy, float opacity) { if ((size_t)pdest <= (size_t)psrc) _blitRegionUp(pdest, dest_stride, psrc, src_stride, sx, sy, opacity); else _blitRegionDown(pdest, dest_stride, psrc, src_stride, sx, sy, opacity); }
        static void _blitRegionUp(color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy, float opacity);
        static void _blitRegionDown(color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy, float opacity);

        template<typename color_t_src, typename BLEND_OPERATOR> void _blit(const Image<color_t_src>& sprite, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy, const  BLEND_OPERATOR& blend_op);
        template<typename color_t_src, typename BLEND_OPERATOR> static void _blitRegion(color_t* pdest, int dest_stride, color_t_src * psrc, int src_stride, int sx, int sy, const  BLEND_OPERATOR& blend_op) { if ((size_t)pdest <= (size_t)psrc) _blitRegionUp(pdest, dest_stride, psrc, src_stride, sx, sy, blend_op); else _blitRegionDown(pdest, dest_stride, psrc, src_stride, sx, sy, blend_op); }
        template<typename color_t_src, typename BLEND_OPERATOR> static void _blitRegionUp(color_t* pdest, int dest_stride, color_t_src * psrc, int src_stride, int sx, int sy, const  BLEND_OPERATOR& blend_op);
        template<typename color_t_src, typename BLEND_OPERATOR> static void _blitRegionDown(color_t* pdest, int dest_stride, color_t_src* psrc, int src_stride, int sx, int sy, const  BLEND_OPERATOR& blend_op);

        void _blitMasked(const Image& sprite, color_t transparent_color, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy, float opacity);
        static void _maskRegion(color_t transparent_color, color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy, float opacity) { if ((size_t)pdest <= (size_t)psrc) _maskRegionUp(transparent_color, pdest, dest_stride, psrc, src_stride, sx, sy, opacity); else _maskRegionDown(transparent_color, pdest, dest_stride, psrc, src_stride, sx, sy, opacity); }
        static void _maskRegionUp(color_t transparent_color, color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy, float opacity);
        static void _maskRegionDown(color_t transparent_color, color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy, float opacity);

        template<typename color_t_src, int CACHE_SIZE, bool USE_BLENDING, bool USE_MASK, bool USE_CUSTOM_OPERATOR, typename BLEND_OPERATOR>
        void _blitScaledRotated(const Image<color_t_src>& src_im, color_t_src transparent_color, fVec2 anchor_src, fVec2 anchor_dst, float scale, float angle_degrees, float opacity, const BLEND_OPERATOR& blend_op);

        void _blitRotated90(const Image& sprite, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy, float opacity);
        void _blitRotated180(const Image& sprite, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy, float opacity);
        void _blitRotated270(const Image& sprite, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy, float opacity);

        template<typename color_t_src, typename BLEND_OPERATOR> void _blitRotated90(const Image& sprite, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy, const BLEND_OPERATOR& blend_op);
        template<typename color_t_src, typename BLEND_OPERATOR> void _blitRotated180(const Image& sprite, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy, const BLEND_OPERATOR& blend_op);
        template<typename color_t_src, typename BLEND_OPERATOR> void _blitRotated270(const Image& sprite, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy, const BLEND_OPERATOR& blend_op);


        /***************************************
        * FLOOD FILLING
        ****************************************/

        template<bool UNICOLOR_COMP, int STACK_SIZE_BYTES> int _scanfill(int x, int y, color_t border_color, color_t new_color);



        /***************************************
        * BRESENHAM
        ****************************************/
     
        template<bool X_MAJOR, bool BLEND, int SIDE, bool CHECKRANGE = false> inline TGX_INLINE void _bseg_update_pixel(const BSeg & seg, color_t color, int32_t op)
            { 
            const int x = seg.X(); const int y = seg.Y();
            if (CHECKRANGE) { if ((x < 0) || (y < 0) || (x >= _lx) || (y >= _ly)) return; }
            if (SIDE != 0) { const int32_t o = ((seg.AA<SIDE, X_MAJOR>()) * op) >> 8; _buffer[TGX_CAST32(x) + TGX_CAST32(_stride) * TGX_CAST32(y)].blend256(color, (uint32_t)o); }
            else if (BLEND) { _buffer[TGX_CAST32(x) + TGX_CAST32(_stride) * TGX_CAST32(y)].blend256(color, (uint32_t)op); }
            else { _buffer[TGX_CAST32(x) + TGX_CAST32(_stride) * TGX_CAST32(y)] = color; }
            }

        template<bool CHECKRANGE = false> inline TGX_INLINE void _bseg_update_pixel(const BSeg & seg, color_t color, int32_t op, int32_t aa)
            {
            const int x = seg.X(); const int y = seg.Y();
            if (CHECKRANGE) { if ((x < 0) || (y < 0) || (x >= _lx) || (y >= _ly)) return; }
            _buffer[TGX_CAST32(x) + TGX_CAST32(_stride) * TGX_CAST32(y)].blend256(color, (uint32_t)((op * aa)>>8));
            }


        template<int SIDE> void _bseg_draw_template(BSeg& seg, bool draw_first, bool draw_last, color_t color, int32_t op, bool checkrange);
        void _bseg_draw(BSeg & seg, bool draw_first, bool draw_last, color_t color, int side, int32_t op, bool checkrange);
        void _bseg_draw_AA(BSeg & seg, bool draw_first, bool draw_last, color_t color, int32_t op, bool checkrange);

        template<int SIDE> void _bseg_avoid1_template(BSeg& segA, bool lastA, BSeg& segB, bool lastB, color_t color, int32_t op, bool checkrange);
        void _bseg_avoid1(BSeg& PQ, BSeg& PA, bool drawP, bool drawQ, bool closedPA, color_t color, int side, int32_t op, bool checkrange);
        template<int SIDE> void _bseg_avoid2_template(BSeg& segA, bool lastA, BSeg& segB, bool lastB, BSeg& segC, bool lastC, color_t color, int32_t op, bool checkrange);
        void _bseg_avoid2(BSeg & PQ, BSeg & PA, BSeg & PB, bool drawQ, bool closedPA, bool closedPB, color_t color, int side, int32_t op, bool checkrange);
        template<int SIDE> void _bseg_avoid11_template(BSeg& segA, BSeg& segB, bool lastB, BSeg& segD, bool lastD, color_t color, int32_t op, bool checkrange);
        void _bseg_avoid11(BSeg & PQ, BSeg& PA, BSeg & QB, bool drawP, bool drawQ, bool closedPA, bool closedQB, color_t color, int side, int32_t op, bool checkrange);
        template<int SIDE> void _bseg_avoid21_template(BSeg& segA, BSeg& segB, bool lastB, BSeg& segC, bool lastC, BSeg& segD, bool lastD, color_t color, int32_t op, bool checkrange);
        void _bseg_avoid21(BSeg& PQ, BSeg& PA, BSeg& PB, BSeg& QC, bool closedPA, bool closedPB, bool closedQC, color_t color, int side, int32_t op, bool checkrange);
        template<int SIDE> void _bseg_avoid22_template(BSeg& segA, BSeg& segB, bool lastB, BSeg& segC, bool lastC, BSeg& segD, bool lastD, BSeg& segE, bool lastE, color_t color, int32_t op, bool checkrange);
        void _bseg_avoid22(BSeg& PQ, BSeg& PA, BSeg& PB, BSeg& QC, BSeg& QD, bool closedPA, bool closedPB, bool closedQC, bool closedQD, color_t color, int side, int32_t op, bool checkrange);


        // filling a triangle 

        //void _bseg_fill_triangle(iVec2 P1, iVec2 P2, iVec2 P3, color_t fillcolor, float opacity);
        void _bseg_fill_triangle(fVec2 fP1, fVec2 fP2, fVec2 fP3, color_t fillcolor, float opacity);
        void _bseg_fill_triangle_precomputed(fVec2 fP1, fVec2 fP2, fVec2 fP3, BSeg & seg12, BSeg & seg21, BSeg & seg23, BSeg & seg32, BSeg & seg31, BSeg & seg13, color_t fillcolor, float opacity);
        void _bseg_fill_triangle_precomputed_sub(fVec2 fP1, fVec2 fP2, fVec2 fP3, BSeg & seg12, BSeg & seg21, BSeg & seg23, BSeg & seg32, BSeg & seg31, BSeg & seg13, color_t fillcolor, float opacity);
        void _bseg_fill_interior_angle(iVec2 P, iVec2 Q1, iVec2 Q2, BSeg& seg1, BSeg& seg2, color_t color, bool fill_last, float opacity);
        void _bseg_fill_interior_angle_sub(int dir, int y, int ytarget, BSeg& sega, BSeg& segb, color_t color, float opacity);

        void _triangle_hline(int x1, int x2, const int y, color_t color, float opacity)
            { // like drawFasthLine but used by _bseg_fill_interior_angle_sub
            x1 = tgx::max(0, x1); x2 = tgx::min(_lx - 1, x2);
            color_t* p = _buffer + TGX_CAST32(x1) + TGX_CAST32(y) * TGX_CAST32(_stride);
            if (opacity < 0) { while (x1++ <= x2) { (*p++) = color; } } else { while (x1++ <= x2) { (*p++).blend(color,opacity); } }
            }
        
        template<typename T> static inline T _triangleAera(Vec2<T> P1, Vec2<T> P2, Vec2<T> P3) { return P1.x * (P2.y - P3.y) + P2.x * (P3.y - P1.y) + P3.x * (P1.y - P2.y); } // return twice the aera of the triangle.



        /***************************************
        * DRAWING LINES
        ****************************************/

        template<bool CHECKRANGE, bool CHECK_VALID_BLEND = true> TGX_INLINE inline void _drawPixel(iVec2 pos, color_t color, float opacity)
            {
            if (CHECKRANGE) { if ((pos.x < 0) || (pos.y < 0) || (pos.x >= _lx) || (pos.y >= _ly)) return; }
            if ((CHECK_VALID_BLEND) && ((opacity < 0)||(opacity>1))) _buffer[TGX_CAST32(pos.x) + TGX_CAST32(_stride) * TGX_CAST32(pos.y)] = color; else _buffer[TGX_CAST32(pos.x) + TGX_CAST32(_stride) * TGX_CAST32(pos.y)].blend(color, opacity);
            }

        template<bool CHECKRANGE> TGX_INLINE inline void _drawPixel(iVec2 pos, color_t color)
            {
            if (CHECKRANGE) { if ((pos.x < 0) || (pos.y < 0) || (pos.x >= _lx) || (pos.y >= _ly)) return; }
            _buffer[TGX_CAST32(pos.x) + TGX_CAST32(_stride) * TGX_CAST32(pos.y)] = color;
            }

        TGX_INLINE inline void _drawPixel(bool checkrange, iVec2 pos, color_t color, float opacity)
            {
            if (checkrange) { if ((pos.x < 0) || (pos.y < 0) || (pos.x >= _lx) || (pos.y >= _ly)) return; }
            if ((opacity < 0)||(opacity>1)) _buffer[TGX_CAST32(pos.x) + TGX_CAST32(_stride) * TGX_CAST32(pos.y)] = color; else _buffer[TGX_CAST32(pos.x) + TGX_CAST32(_stride) * TGX_CAST32(pos.y)].blend(color, opacity);
            }

        TGX_INLINE inline void _drawPixel(bool checkrange, iVec2 pos, color_t color)
            {
            if (checkrange) { if ((pos.x < 0) || (pos.y < 0) || (pos.x >= _lx) || (pos.y >= _ly)) return; }
            _buffer[TGX_CAST32(pos.x) + TGX_CAST32(_stride) * TGX_CAST32(pos.y)] = color;
            }

        template<bool CHECKRANGE = true> TGX_INLINE inline color_t _readPixel(iVec2 pos, color_t outside_color) const
            {
            if (CHECKRANGE) { if ((pos.x < 0) || (pos.y < 0) || (pos.x >= _lx) || (pos.y >= _ly)) return outside_color; }
            return _buffer[TGX_CAST32(pos.x) + TGX_CAST32(_stride) * TGX_CAST32(pos.y)];
            }

        template<bool CHECKRANGE> void _drawFastVLine(iVec2 pos, int h, color_t color, float opacity);
        template<bool CHECKRANGE> void _drawFastHLine(iVec2 pos, int w, color_t color, float opacity);
        template<bool CHECKRANGE> void _drawFastVLine(iVec2 pos, int h, color_t color);
        template<bool CHECKRANGE> void _drawFastHLine(iVec2 pos, int w, color_t color);

        inline TGX_INLINE void _drawFastVLine(bool checkrange, iVec2 pos, int h, color_t color, float opacity) { if (checkrange) _drawFastVLine<true>(pos, h, color, opacity); else _drawFastVLine<false>(pos, h, color, opacity); }
        inline TGX_INLINE void _drawFastHLine(bool checkrange, iVec2 pos, int w, color_t color, float opacity) { if (checkrange) _drawFastHLine<true>(pos, w, color, opacity); else _drawFastHLine<false>(pos, w, color, opacity); }
        inline TGX_INLINE void _drawFastVLine(bool checkrange, iVec2 pos, int h, color_t color) { if (checkrange) _drawFastVLine<true>(pos, h, color); else _drawFastVLine<false>(pos, h, color); }
        inline TGX_INLINE void _drawFastHLine(bool checkrange, iVec2 pos, int w, color_t color) { if (checkrange) _drawFastHLine<true>(pos, w, color); else _drawFastHLine<false>(pos, w, color); }
        inline TGX_INLINE void _drawSeg(iVec2 P1, bool drawP1, iVec2 P2, bool drawP2, color_t color, float opacity) { BSeg seg(P1, P2); _bseg_draw(seg, drawP1, drawP2, color, 0, (int32_t)(opacity * 256), true); }


        void _drawEnd(float distAB, fVec2 A, fVec2 B, BSeg& segAB, BSeg& segBA, BSeg& segAP, BSeg& segBQ, EndPath end, int w, color_t color, float opacity);

        // legacy (not used anymore)
        void _drawWedgeLine(float ax, float ay, float bx, float by, float aw, float bw, color_t color, float opacity);
        float _wedgeLineDistance(float pax, float pay, float bax, float bay, float dr);




        /***************************************
        * RECT AND ROUNDED RECT
        ****************************************/


        // low quality drawing 
        void _fillRect(iBox2 B, color_t color, float opacity);
        template<bool CHECKRANGE> void _drawRoundRect(int x, int y, int w, int h, int r, color_t color);
        template<bool CHECKRANGE> void _drawRoundRect(int x, int y, int w, int h, int r, color_t color, float opacity);        
        template<bool CHECKRANGE> void _fillRoundRect(int x, int y, int w, int h, int r, color_t color);        
        template<bool CHECKRANGE> void _fillRoundRect(int x, int y, int w, int h, int r, color_t color, float opacity);


        // high quality drawing
        void _fillSmoothRect(const fBox2& B, color_t color, float opacity);
        void _fillSmoothRoundedRect(const tgx::iBox2& B, float corner_radius, color_t color, float opacity);
        void _drawSmoothRoundRect(const tgx::iBox2& B, float corner_radius, color_t color, float opacity);
        void _drawSmoothWideRoundRect(const tgx::iBox2& B, float corner_radius, float thickness, color_t color, float opacity);



        /***************************************
        * TRIANGLES
        ****************************************/


        // methods using the 3D triangle rasterizer        
        inline TGX_INLINE tgx::fVec2 _coord_texture(tgx::fVec2 pos, tgx::iVec2 size) { return tgx::fVec2(pos.x / ((float)size.x), pos.y / ((float)size.y)); } // Convert to texture coordinates
        inline TGX_INLINE tgx::fVec2 _coord_viewport(tgx::fVec2 pos, tgx::iVec2 size) { return tgx::fVec2((2.0f / ((float)size.x)) * (pos.x) - 1.0f, (2.0f / ((float)size.y)) * (pos.y) - 1.0f); } // Convert to viewport coordinates

        template<typename color_alt, bool USE_BLENDING> void _drawGradientTriangle(fVec2 P1, fVec2 P2, fVec2 P3, color_alt colorP1, color_alt colorP2, color_alt colorP3, float opacity);
        template<typename color_t_tex, bool GRADIENT, bool USE_BLENDING, bool MASKED> void _drawTexturedTriangle(const Image<color_t_tex>& src_im, color_t_tex transparent_color, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, color_t_tex C1, color_t_tex C2, color_t_tex C3, float opacity);



        /***************************************
        * QUADS
        ****************************************/



        /***************************************
        * POLYGONS AND POLYLINES
        ****************************************/



        /***************************************
        * CIRCLES, ARC AND PIES
        ****************************************/


        // low quality drawing for circles
        template<bool CHECKRANGE>  void _drawCircleHelper(int x0, int y0, int r, int cornername, color_t color, float opacity); // adapted from Adafruit GFX library
        template<bool CHECKRANGE>  void _fillCircleHelper(int x0, int y0, int r, int corners, int delta, color_t color, float opacity); // adapted from Adafruit GFX library
        template<bool OUTLINE, bool FILL, bool CHECKRANGE> void _drawFilledCircle(int xm, int ym, int r, color_t color, color_t fillcolor, float opacity); // filled circle drawing method


        // high quality drawing for circle, arc and pie
        static float _rectifyAngle(float a); 
        static void _defaultQuarterVH(int quarter, int& v, int& h);
      
        void _fillSmoothQuarterCircleInterHPsub(tgx::fVec2 C, float R, int quarter, bool vertical_center_line, bool horizontal_center_line, color_t color, float opacity, int nb_planes, int32_t kx1 = 0, int32_t ky1 = 0, int32_t off1 = 0, int32_t off1_full = 0, int32_t kx2 = 0, int32_t ky2 = 0, int32_t off2 = 0, int32_t off2_full = 0);        
        void _fillSmoothQuarterCircleInterHP0(int quarter, tgx::fVec2 C, float R, color_t color, float opacity);
        void _fillSmoothQuarterCircleInterHP1(int quarter, tgx::fVec2 C, float R, color_t color, float opacity, const BSeg& seg1, int side1);
        void _fillSmoothQuarterCircleInterHP2(int quarter, tgx::fVec2 C, float R, color_t color, float opacity, const BSeg& seg1, int side1, const BSeg& seg2, int side2);
        void _fillSmoothCircleInterHP(tgx::fVec2 C, float R, color_t color, float opacity, const BSeg& seg, int side); // used to draw rounded ends

        void _drawSmoothQuarterCircleInterHPsub(tgx::fVec2 C, float R, int quarter, bool vertical_center_line, bool horizontal_center_line, color_t color, float opacity, int nb_planes, int32_t kx1 = 0, int32_t ky1 = 0, int32_t off1 = 0, int32_t off1_full = 0, int32_t kx2 = 0, int32_t ky2 = 0, int32_t off2 = 0, int32_t off2_full = 0);
        void _drawSmoothQuarterCircleInterHP0(int quarter, tgx::fVec2 C, float R, color_t color, float opacity);
        void _drawSmoothQuarterCircleInterHP1(int quarter, tgx::fVec2 C, float R, color_t color, float opacity, const BSeg& seg1, int side1);
        void _drawSmoothQuarterCircleInterHP2(int quarter, tgx::fVec2 C, float R, color_t color, float opacity, const BSeg& seg1, int side1, const BSeg& seg2, int side2);

        void _drawSmoothThickQuarterCircleInterHPsub(tgx::fVec2 C, float R, float thickness, int quarter, bool vertical_center_line, bool horizontal_center_line, color_t color, float opacity, int nb_planes, int32_t kx1 = 0, int32_t ky1 = 0, int32_t off1 = 0, int32_t off1_full = 0, int32_t kx2 = 0, int32_t ky2 = 0, int32_t off2 = 0, int32_t off2_full = 0);
        void _drawSmoothThickQuarterCircleInterHP0(int quarter, tgx::fVec2 C, float R, float thickness, color_t color, float opacity);
        void _drawSmoothThickQuarterCircleInterHP1(int quarter, tgx::fVec2 C, float R, float thickness, color_t color, float opacity, const BSeg& seg1, int side1);
        void _drawSmoothThickQuarterCircleInterHP2(int quarter, tgx::fVec2 C, float R, float thickness, color_t color, float opacity, const BSeg& seg1, int side1, const BSeg& seg2, int side2);

        void _fillSmoothThickQuarterCircleInterHPsub(tgx::fVec2 C, float R, float thickness, int quarter, bool vertical_center_line, bool horizontal_center_line, color_t color_interior, color_t color_border, float opacity, int nb_planes, int32_t kx1 = 0, int32_t ky1 = 0, int32_t off1 = 0, int32_t off1_full = 0, int32_t kx2 = 0, int32_t ky2 = 0, int32_t off2 = 0, int32_t off2_full = 0);
        void _fillSmoothThickQuarterCircleInterHP0(int quarter, tgx::fVec2 C, float R, float thickness, color_t color_interior, color_t color_border, float opacity);
        void _fillSmoothThickQuarterCircleInterHP1(int quarter, tgx::fVec2 C, float R, float thickness, color_t color_interior, color_t color_border, float opacity, const BSeg& seg1, int side1);
        void _fillSmoothThickQuarterCircleInterHP2(int quarter, tgx::fVec2 C, float R, float thickness, color_t color_interior, color_t color_border, float opacity, const BSeg& seg1, int side1, const BSeg& seg2, int side2);



        /***************************************
        * ELLIPSES
        ****************************************/

        
        // low quality drawing for ellipses
        template<bool OUTLINE, bool FILL, bool CHECKRANGE> void _drawEllipse(int x0, int y0, int rx, int ry, color_t outline_color, color_t interior_color, float opacity); // adapted from bodmer e_tft library

        // high quality drawing for ellipses
        void _drawSmoothQuarterEllipse(tgx::fVec2 C, float rx, float ry, int quarter, bool vertical_center_line, bool horizontal_center_line, color_t color, float opacity);
        void _drawSmoothThickQuarterEllipse(tgx::fVec2 C, float rx, float ry, float thickness, int quarter, bool vertical_center_line, bool horizontal_center_line, color_t color, float opacity);        
        void _fillSmoothQuarterEllipse(tgx::fVec2 C, float rx, float ry, int quarter, bool vertical_center_line, bool horizontal_center_line, color_t color, float opacity);
        void _fillSmoothThickQuarterEllipse(tgx::fVec2 C, float rx, float ry, float thickness, int quarter, bool vertical_center_line, bool horizontal_center_line, color_t color_interior, color_t color_border, float opacity);
  


        /***************************************
        * DRAWING BEZIER
        ****************************************/


        // low quality Bezier curves
        void _plotQuadRationalBezierSeg(const bool checkrange, int x0, int y0, int x1, int y1, int x2, int y2, float w, const color_t color, const float opacity);
        void _plotQuadRationalBezier(const bool checkrange, int x0, int y0, int x1, int y1, int x2, int y2, float w, const bool draw_P2, const color_t color, const float opacity);
        void _drawQuadBezier(iVec2 P1, iVec2 P2, iVec2 PC, float wc, bool drawP2, color_t color, float opacity);

        void _plotCubicBezierSeg(const bool checkrange, int x0, int y0, float x1, float y1, float x2, float y2, int x3, int y3, const color_t color, const float opacity);
        void _plotCubicBezier(const bool checkrange, int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, bool draw_P2, const color_t color, const float opacity);
        void _drawCubicBezier(iVec2 P1, iVec2 P2, iVec2 PA, iVec2 PB, bool drawP2, color_t color, float opacity);

        void _plotQuadSpline(int n, int x[], int y[], bool draw_last, const color_t color, const float opacity);
        template<int SPLINE_MAX_POINTS> void _drawQuadSpline(int nbpoints, const iVec2* tabPoints, bool draw_last_point, color_t color, float opacity);

        void _plotClosedSpline(int n, int x[], int y[], const color_t color, const float opacity);
        template<int SPLINE_MAX_POINTS> void _drawClosedSpline(int nbpoints, const iVec2* tabPoints, color_t color, float opacity);

        void _plotCubicSpline(int n, int x[], int y[], bool draw_last, const color_t color, const float opacity);
        template<int SPLINE_MAX_POINTS> void _drawCubicSpline(int nbpoints, const iVec2* tabPoints, bool draw_last_point, color_t color, float opacity);



        // high quality Bezier curves
        static bool _splitRationalQuadBezier(fVec2 P1, fVec2 P2, fVec2 PC, float w, fVec2& Q, fVec2& PB, float& wb);  
        static bool _splitCubicBezier(fVec2 P1, fVec2 P2, fVec2 PC1, fVec2 PC2, fVec2& Q, fVec2& C, fVec2 & D);



        /***************************************
        * DRAWING TEXT
        ****************************************/
        
        bool _clipit(int& x, int& y, int& sx, int& sy, int& b_left, int& b_up);

        template<bool BLEND> iVec2 _drawCharGFX(char c, iVec2 pos, color_t col, const GFXfont& font, float opacity);
        template<bool BLEND> iVec2 _drawCharILI(char c, iVec2 pos, color_t col, const ILI9341_t3_font_t& font, float opacity);

        template<bool BLEND> iVec2 _drawTextGFX(const char* text, iVec2 pos, const GFXfont& font, color_t col, float opacity, bool wrap, bool start_newline_at_0);
        template<bool BLEND> iVec2 _drawTextILI(const char* text, iVec2 pos, const ILI9341_t3_font_t& font, color_t col, float opacity, bool wrap, bool start_newline_at_0);

        template<bool BLEND> void _drawCharILI9341_t3(const uint8_t* bitmap, int32_t off, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col, float opacity);        
        template<bool BLEND> static void _drawcharline(const uint8_t* bitmap, int32_t off, color_t* p, int dx, color_t col, float opacity);
        template<bool BLEND> void _drawCharBitmap_1BPP(const uint8_t* bitmap, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col, float opacity);
        template<bool BLEND> void _drawCharBitmap_2BPP(const uint8_t* bitmap, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col, float opacity);
        template<bool BLEND> void _drawCharBitmap_4BPP(const uint8_t* bitmap, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col, float opacity);
        template<bool BLEND> void _drawCharBitmap_8BPP(const uint8_t* bitmap, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col, float opacity);




        /************************************************************************************************************
        *
        * Image members variables (16 bytes).
        *
        *************************************************************************************************************/

        color_t* _buffer;       // pointer to the pixel buffer  - nullptr if the image is invalid.
        int     _lx, _ly;       // image size  - (0,0) if the image is invalid
        int     _stride;        // image stride - 0 if the image is invalid


    };



}




#include "Image.inl"


#endif

#endif

/** end of file */


