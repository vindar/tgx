/** @file Image.h */
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

        

    /************************************************************************************
    * Template class for an "image object" that draws into a memory buffer.   
    *  
    * The class object itself is very small (16 bytes) thus image objects can easily be 
    * passed around/copied without much cost. Also, sub-images sharing the same memory 
    * buffer can be created and are useful for clipping drawing operation to a given 
    * region of the image. 
    *
    * No memory allocation is performed by the class: it is the user's responsability to
    * allocate memory and assign the buffer to the image.   
    * 
    * Image layout:
    * 
    * - for an image of type 'color_t' with size (_lx,_ly) and stride '_stride', the memory
    *   buffer should be large enough to contain (_ly * _stride) elements of type color_t.
    * 
    * - the color of the pixel at position (x,y) is given by the formula:
    *   color(x,y) = buffer[x + y*_stride].  
    * 
    * - for new image, one can always choose _stride = lx (but allowing larger stride in 
    *   the class definition allows to efficiently represent sub-image as well without 
    *   having to make a copy).  
    *
    * 
    * Template parameter:
    * 
    * - color_t: Color type to use with the image, the memory buffer supplied should have 
    *             type color_t*. It must be one of the color types defined in color.h
    *             
    *             Remark : avoid type HSV which has not been tested and will be very slow.
    * 
    * Remarks:
    * 
    * (1) Avoid using HSV as an Image color type:  it has not been tested to work and using
    *     HSV colors will be very slow and take a lot of memory anyway...
    * 
    * (2) Most drawing methods of the image class have two versions, one with a final 'opacity'
    *     parameter and one without this parameter. The difference between these version is
    *     how how source and destination pixels are combined. 
    *     
    *     a. Method without the 'opacity' parameter. The image is simply written over without   
    *        any fancy alpha-blending (irrespectively of whether or not the source pixel has an 
    *        alpha channel). 
    *        
    *     b. Method with the 'opacity' parameter. The color to write on the image is blended
    *        over the current pixel color on the image. The way this happens depends on whether
    *        the color to blend over has an alpha channel (RGB32, RGB64) or not (RGB565, RGB24, RGBf)
    *        - if the color has an alpha channel, then its opacity is multiplied by the opacity   
    *          parameter and then the color is blended over the current pixel color.
    *        - if the color does not have an alpha channel, then its opacity is emulated with the  
    *          'opacity' parameter and then it is blended over the current pixel color.
    * 
    *      In particular, calling a method without the opacity parameter is not the same as calling 
    *      the method with opacity=1.0f if the source color has an alpha channel and is not fully
    *      opaque !
    *           
    *  (3) As indicated in color.h. All colors with an alpha channel are assumed to have pre-
    *      multiplied  alpha and treated as such for all blending operations.
    *      
    * 
    *  (4) Many class  methods require iVec2 parameters to specify coordinates in the image. 
    *      Using initializer list, it is very easy to call such method without explicitly
    *      mentioning the iVec2 type. For example, assume a function with signature 
    *      f(iVec2 pos, int r)` then it can simply be called with f({x,y},z)' which is 
    *      equivalent to the expression f(iVec2(x,y),z)
    *      
    *      Also, some methods take an fVec2 instead of iVec2 for sub-pixel precision. In 
    *      that case, it is still valid to call them with an iVec2 since it will implicitly 
    *      be promoted to an fVec2. For example, assume a function with signature 
    *      `f(fVec2 pos, int r)`, then both call f({1,3},z) or  f({1.0f,3.0f},z) yield the
    *      same result. 
    * 
    *************************************************************************************/
    template<typename color_t> 
    class Image
    {

        // make sure right away that the template parameter is admissible to prevent cryptic error message later.  
        static_assert(is_color<color_t>::value, "color_t must be one of the color types defined in color.h");

        // befriend all sister Image classes
        template<typename> friend class Image;


    /************************************************************************************************************
    * 
    * Image members variables (16 bytes).
    * 
    *************************************************************************************************************/

        color_t* _buffer;       // pointer to the pixel buffer  - nullptr if the image is invalid.
        int     _lx, _ly;       // image size  - (0,0) if the image is invalid
        int     _stride;        // image stride - 0 if the image is invalid


    public:


        #if (MTOOLS_TGX_EXTENSIONS)
        #include <mtools/extensions/tgx/tgx_ext_Image.inl> 
        #endif







    /************************************************************************************************************
    *************************************************************************************************************
    *************************************************************************************************************
    * 
    *                        CREATION OF IMAGES AND SUB-IMAGES
    * 
    * The memory buffer should be supplied at creation. Otherwise, the image is set as 
    * invalid until a valid buffer is supplied. 
    * 
    * NOTE: The image class itself is lightweight as it does not manage the memory buffer.
    *       Creating image and sub-image is very fast and do not use additional memory.   
    *       
    *************************************************************************************************************
    *************************************************************************************************************
    ************************************************************************************************************/


        /** 
        * Create a default empty (invalid) image. 
        */
        Image();


        /**
         * Constructor. Creates an image with a given size and a given buffer.
         *
         * @param [in,out]  buffer  the image buffer
         * @param           lx      the image width.
         * @param           ly      the image height.
         * @param           stride  the stride to use (equal to the image width if not specified).
        **/
        template<typename T> Image(T* buffer, int lx, int ly, int stride = -1);


        /**
         * Constructor. Creates an image with a given size and a given buffer.
         *
         * @param [in,out]  buffer  the image buffer
         * @param           dim     the image dimension (width, height) in pixels.
         * @param           stride  the stride to use (equal to the image width if not specified).
        **/
        template<typename T> Image(T* buffer, iVec2 dim, int stride = -1);


        /**
         * Constructor. Create a sub-image of a given image sharing the same buffer.
         *
         * @param   im      The source image.
         * @param   subbox  The region of the source image to use for this image.
         * @param   clamp   (Optional) True to clamp the values of the `subbox` parameter. If set,
         *                  `subbox` is intersected with the source image box to create a valid region.
         *                  Otherwise, and if `subbox` does not fit inside the source image box, then
         *                  an empty image is created.
        **/
        Image(const Image<color_t>& im, iBox2 subbox, bool clamp = true);


        /**
         * Default copy constructor.
         * 
         * Copy is shallow: both images share the same memory buffer.
         *
         * @param   im  The source image.
        **/
        Image(const Image<color_t> & im) = default;


        /**
         * Default assignement operator.
         *
         * Copy is shallow: both images share the same memory buffer.
         *
         * @param   im  The source image.
         *
         * @returns itself.
        **/
        Image& operator=(const Image<color_t> & im) = default;


        /**
         * Set/update the image parameters.
         *
         * @param [in,out]  buffer  the pixel buffer.
         * @param           lx      image width.
         * @param           ly      image height.
         * @param           stride  (Optional) The stride. If not specified, the stride is set equal to
         *                          the image width.
        **/
        template<typename T> void set(T* buffer, int lx, int ly, int stride = -1);


        /**
         * Set/update the image parameters.
         *
         * @param [in,out]  buffer  the pixel buffer.
         * @param           dim     the image dimensions.
         * @param           stride  (Optional) The stride. If not specified, the stride is set equal to.
        **/
        template<typename T> void set(T* buffer, iVec2 dim, int stride = -1);


        /**
         * Crop this image by keeping only the region represented by subbox (intersected with the image
         * box).
         * 
         * This operation does not change the underlying pixel buffer: it simply replaces this image by
         * a sub-image of itself (with different dimension/stride).
         *
         * @param   subbox  The region to to keep.
        **/
        void crop(const iBox2& subbox);


        /**
         * Return a sub-image of this image (sharing the same pixel buffer).
         *
         * See also `operator()` below.
         * 
         * @param   subbox  The region to to keep.
         * @param   clamp   (Optional) True to clamp the values of the `subbox` parameter. If set,
         *                  `subbox` is intersected with this image box to create a valid region.
         *                  Otherwise, and if `subbox` does not fit inside the source image box, 
         *                  then an empty image is returned.
         *
         * @returns the cropped image.
        **/
        Image<color_t> getCrop(const iBox2& subbox, bool clamp = true) const;


        /**
         * Return a sub-image of this image (sharing the same pixel buffer).
         * 
         * This is the same as `getCrop(B, true)`
         *
         * @param   B  box that delimit the sub-image inside this image. 
         *
         * @returns the sub-image delimited by B and sharing the same memory buffer. 
        **/
        Image<color_t> operator()(const iBox2& B) const;


        /**
         * Return a sub-image of this image (sharing the same pixel buffer).
         *
         * This is the same as `getCrop(tgx::iBox2(min_x, max_x, min_y, max_x), true)` or `operator()(tgx::iBox2(min_x, max_x, min_y, max_x), true)`
         *
         * @param   min_x   left boundary
         * @param   max_x   right boundary
         * @param   min_y   top boundary
         * @param   max_y   bottom boundary
         *
         * @returns the sub-image delimited the closed box iBox(min_x, max_x, min_y, max_x) and sharing the same memory buffer. 
        **/
        Image<color_t> operator()(int min_x, int max_x, int min_y, int max_y) const;


        /**
         * Query if the image is valid.
         *
         * @returns True if the image if valid, false otherwise.
        **/
        inline TGX_INLINE bool isValid() const { return (_buffer != nullptr); }


        /**
        * Set the image as invalid.
        *
        * Note: It is the user responsibility to manage the memory allocated for
        * the pixel buffer (if required). No deallocation is performed here.
        */
        void setInvalid();







    /************************************************************************************************************
    *************************************************************************************************************
    *************************************************************************************************************
    * 
    *                                      QUERY OF IMAGE ATTRIBUTES
    *       
    *************************************************************************************************************
    *************************************************************************************************************
    ************************************************************************************************************/



        /**
         * Return the image width. Same as lx().
         *
         * @returns The image width (0 for an invalid image).
        **/
        inline TGX_INLINE int width() const { return _lx; }


        /**
         * Return the image width. Same as width().
         *
         * @returns The image width (0 for an invalid image).
        **/
        inline TGX_INLINE int lx() const { return _lx; }


        /** 
         * Return the image height. Same as ly().
         *
         * @returns The image height (0 for an invalid image).
        **/
        inline TGX_INLINE int height() const { return _ly; }


        /**
         * Return the image height. Same as height().
         *
         * @returns The image height (0 for an invalid image).
        **/
        inline TGX_INLINE int ly() const { return _ly; }


        /**
         * Return the image stride.
         *
         * @returns The stride  (0 for an invalid image).
        **/
        inline TGX_INLINE int stride() const { return _stride; }


        /**
         * Return the image dimensions as a vector.
         *
         * @returns The image dimension ({0,0} for an invalid image).
        **/
        inline TGX_INLINE iVec2 dim() const { return iVec2{ _lx, _ly }; }


        /**
         * Return the image dimension as a box.
         *
         * @returns a box of the form {0, width-1, 0 height-1 } or an empty box if the image is invalid.
        **/
        inline TGX_INLINE iBox2 imageBox() const { return iBox2(0, _lx - 1, 0, _ly - 1); }


        /**
         * Return a pointer to the pixel buffer (const version).
         *
         * @returns A pointer to the start of the pixel buffer associated with this image (or nullptr if
         *          the image is invalid).
        **/
        inline TGX_INLINE const color_t * data() const { return _buffer; }


        /**
         * Return a pointer to the pixel buffer.
         *
         * @returns A pointer to the start of the pixel buffer associated with this image (or nullptr if
         *          the image is invalid).
        **/
        inline TGX_INLINE color_t * data() { return _buffer; }



       



    /************************************************************************************************************
    *************************************************************************************************************
    *************************************************************************************************************
    * 
    *                                           DIRECT PIXEL ACCESS
    *       
    *************************************************************************************************************
    *************************************************************************************************************
    ************************************************************************************************************/



        /**
         * Set a pixel at a given position.
         * 
         * Use template parameter to disable range check for faster access (danger!)
         *
         * @tparam  CHECKRANGE  True to check the range and false to disable checking.
         * @param   pos     The position.
         * @param   color   The color.
        **/
        template<bool CHECKRANGE = true> TGX_INLINE inline void drawPixel(iVec2 pos, color_t color)
            {
            if ((CHECKRANGE) && (!isValid())) return;
            _drawPixel<CHECKRANGE>(pos, color);
            }


        /**
         * Blend a pixel with the current pixel color after multiplying its opacity with a given factor.
         * If type color_t has an alpha channel, then it is used for alpha blending.
         * 
         * Use template parameter to disable range check for faster access (danger!).
         *
         * @tparam  CHECKRANGE  True to check the range and false to disable checking.
         * @param   pos     The position.
         * @param   color   The color to blend.
         * @param   opacity opacity multiplier from 0.0f (fully transparent) to 1.0f fully transparent.
         *                  if negative, then simple overwrittening of color is used instead of blending.
        **/
        template<bool CHECKRANGE = true> TGX_INLINE inline void drawPixel(iVec2 pos, color_t color, float opacity)
            {
            if ((CHECKRANGE) && (!isValid())) return;
            _drawPixel<CHECKRANGE,true>(pos, color, opacity);
            }


        /**
         * Return the color of a pixel at a given position.
         * 
         * If CHECKRANGE is true, outside_color is returned when querying outside of the image.
         *
         * @tparam  CHECKRANGE      True to check the range and false to disable it (danger!).
         * @param   pos             The position.
         * @param   outside_color   (Optional) color to return when querying outside the range.
         *
         * @returns The pixel color
        **/
        template<bool CHECKRANGE = true> TGX_INLINE inline color_t readPixel(iVec2 pos, color_t outside_color = color_t()) const
            {
            if ((CHECKRANGE) && (!isValid())) return outside_color;
            return _readPixel(pos, outside_color);
            }


        /**
         * Get a reference to a pixel (no range check!) const version
         *
         * @param   pos The position.
         *
         * @returns a const reference to the pixel color.
        **/
        TGX_INLINE inline const color_t& operator()(iVec2 pos) const
            {
            return _buffer[TGX_CAST32(pos.x) + TGX_CAST32(_stride) * TGX_CAST32(pos.y)];
            }


        /**
         * Get a reference to a pixel (no range check!) const version
         *
         * @param   x   x-coordinate
         * @param   y   y-coordinate.
         *
         * @returns a const reference to the pixel color.
        **/
        TGX_INLINE inline const color_t& operator()(int x, int y) const
            {
            return _buffer[TGX_CAST32(x) + TGX_CAST32(_stride) * TGX_CAST32(y)];
            }


        /**
         * Get a reference to a pixel (no range check!) 
         *
         * @param   pos The position.
         *
         * @returns a reference to the pixel color.
        **/
        TGX_INLINE inline color_t & operator()(iVec2 pos)
            {
            return _buffer[TGX_CAST32(pos.x) + TGX_CAST32(_stride) * TGX_CAST32(pos.y)];
            }


        /**
         * Get a reference to a pixel (no range check!)
         *
         * @param   x   x-coordinate
         * @param   y   y-coordinate.
         *
         * @returns a reference to the pixel color.
        **/
        TGX_INLINE inline color_t& operator()(int x, int y)
            {
            return _buffer[TGX_CAST32(x) + TGX_CAST32(_stride) * TGX_CAST32(y)];
            }


        /**
         * Iterate over all the pixels of the image going from left to right (inner loop) and then from 
         * top to bottom (outer loop). The callback function cb_fun is called for each pixel and must 
         * have a signature compatible with:
         * 
         *  `bool cb_fun(tgx::iVec2 pos, color_t & color)`
         * 
         * where:
         * 
         * - `pos` is the position of the current pixel in the image
         * - `color` is a reference to the current pixel color.
         * - the callback must return true to continue the iteration and false to abort the iteration.
         * 
         * This method is particularly useful with lambdas, for example, to paint all black pixels to
         * red in a RGB565 image `im`:
         * 
         * im.iterate( [](tgx::iVec2 pos, tgx::RGB565 &amp; color) { if (color == tgx::RGB565_Black) color = tgx::RGB565_Red; return true;});
         *
         * @param   cb_fun  The callback function
        **/
        template<typename ITERFUN> void iterate(ITERFUN cb_fun);


        /**
         * Same as above but iterate only over the pixels inside the sub-box B (intersected with the
         * image box).
         *
         * @param   cb_fun  The callback function.
         * @param   B       The sub-box to iterate inside.
        **/
        template<typename ITERFUN> void iterate(ITERFUN cb_fun, tgx::iBox2 B);







    /************************************************************************************************************
    *************************************************************************************************************
    *************************************************************************************************************
    * 
    *                                 BLITTING, COPYING AND RESIZING IMAGES
    *       
    *************************************************************************************************************
    *************************************************************************************************************
    ************************************************************************************************************/



        /**
         * Blit/blend a sprite over this image at a given position.
         *
         * @param   sprite          The sprite image to blit over this image.
         * @param   upperleftpos    Position of the upper left corner of the sprite in the image.
         * @param   opacity         (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or
         *                          negative to disable blending and simply overwrite the spite over the
         *                          image.
        **/
        void blit(const Image<color_t>& sprite, iVec2 upperleftpos, float opacity = TGX_DEFAULT_BLENDING_MODE);


        /**
         * Blend a sprite at a given position on the image using a custom blending operator.
         * 
         * The blending operator 'blend_op' can be a function/functor/lambda. It takes as input the
         * color of the source (sprite) pixel and the color of the dest. pixel and return the blended
         * color.
         * 
         * Thus, it must be callable in the form:
         * 
         *       blend_op(col_src, col_dst)
         * 
         * where `col_src` and `col_dst` are colors of respective type color_t_src and color_t and it
         * must return a valid color (of any type but preferably of type color_t for best performance).
         * 
         * Remarks:
         * 
         * 1. To perform "classical alpha blending", use the blit() method with an opacity parameter
         *    instead of this method as it will be faster.
         * 2. If only part of the sprite should be blended onto this image, one simply has to create a
         *    temporary sub-image like so: 'blend(sprite.getCrop(subbox),upperleftpos, blend_op)'. No
         *    copy is performed when creating (shallow) sub-image so this does not incur any slowdown.
         *
         * @param   sprite          The sprite image to blend.
         * @param   upperleftpos    Position of the upper left corner of the sprite in the image.
         * @param   blend_op        The blending operator.
        **/
        template<typename color_t_src, typename BLEND_OPERATOR> 
        void blit(const Image<color_t_src>& sprite, iVec2 upperleftpos, const BLEND_OPERATOR& blend_op);


        /**
         * Blend a sprite at a given position on this image with a given mask. Sprite pixels with color
         * transparent_color` are treated as transparent hence are not copied on the image. Other pixels
         * are blended with the destination image after being multiplied by the opacity factor.
         * 
         * Remark: This method is especially useful when color_t does not have an alpha channel hence no
         * predefined transparent color.
         *
         * @param   sprite              The sprite image to blit.
         * @param   transparent_color   The sprite color considered transparent.
         * @param   upperleftpos        Position of the upper left corner of the sprite in the image.
         * @param   opacity             (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or
         *                              negative to disable blending and simply overwrite the spite over
         *                              the image.
        **/
        void blitMasked(const Image<color_t>& sprite, color_t transparent_color, iVec2 upperleftpos, float opacity = 1.0f);


        /**
         * Reverse blitting. Copy part of the image into the sprite This is the inverse of the blit
         * operation.
         * 
         * THis method can be useful to 'save' a potion of the image into a sprite before
         * drawing/blitiing something on it...
         *
         * @param [in,out]  dst_sprite      The sprite to save part of this image into.
         * @param           upperleftpos    Position of the upper left corner of the sprite in the image.
        **/
        void blitBackward(Image<color_t>& dst_sprite, iVec2 upperleftpos) const;


        /**
         * Blit/blend a sprite onto this image after rescaling and rotation. The anchor point
         * 'anchor_src' in the sprite is mapped to 'anchor_im' in this image. The rotation is performed
         * in counter-clockwise direction around the anchor point.
         * 
         * Remarks:
         * 
         * 1. The positions are given using floating point values to allow for sub-pixel precision for
         *    smoother animation.
         * 2. The method uses bilinear interpolation for high quality rendering.
         * 3. The sprite image can have a different color type from this image.
         * 
         * Note: When rotated, access to the sprite pixels colors is not linear anymore. For certain
         *       orientations, this will yield very 'irregular' access to the sprite memory locations.
         *       When using large sprites in PROGMEM, this can result in huge slowdown as caching cannot
         *       be performed efficiently by the MCU. If this is the case, try moving the sprite to RAM
         *       (or another faster memory) before blitting...
         *
         * @tparam  color_t_src Color type of the sprite image.
         * @tparam  CACHE_SIZE  Size of the MCU cache when reading from flash. This value is indicative
         *                      and used to optimize cache access to flash memory. You may try changing
         *                      the default value it if drawing takes a long time...
         * @param   src_im          The sprite image to draw.
         * @param   anchor_src      Position of the anchor point in the sprite image.
         * @param   anchor_dst      Position of the anchor point in this (destination) image.
         * @param   scale           Scaling factor (1.0f for no rescaling).
         * @param   angle_degrees   The rotation angle in degrees (0 for no rotation).
         * @param   opacity         Opacity multiplier when blending (in [0.0f, 1.0f]) or negative to
         *                          disable blending and simply use overwrite.
        **/
        template<typename color_t_src, int CACHE_SIZE = TGX_PROGMEM_DEFAULT_CACHE_SIZE>
        void blitScaledRotated(const Image<color_t_src> src_im, fVec2 anchor_src, fVec2 anchor_dst, float scale, float angle_degrees, float opacity = TGX_DEFAULT_BLENDING_MODE);



        /**
         * Blend a sprite onto this image after rescaling and rotation using a custom blending operator.
         * 
         * The anchor point 'anchor_src' in the sprite is mapped to 'anchor_im' in this image. The
         * rotation is performed in counter-clockwise direction around the anchor point.
         * 
         * The blending operator 'blend_op' can be a function/functor/lambda. It takes as input the
         * color of the source (sprite) pixel and the color of the dest. pixel and return the blended
         * color.
         * 
         * Thus, it must be callable in the form:
         * 
         *       blend_op(col_src, col_dst)
         * 
         * where `col_src` and `col_dst` are colors of respective type color_t_src and color_t and it
         * must return a valid color (of any type but preferably of type color_t for best performance).
         * 
         * Remarks:
         * 
         * 1. The positions are given using floating point values to allow for sub-pixel precision for
         *    smoother animation.
         * 2. The method use bilinear interpolation for high quality rendering.
         * 3. The sprite image can have a different color type from this image.  
         * 4. To perform "classical alpha blending", use the blitScaleRotated() method with an opacity
         *    parameter instead as it will be faster.
         * 
         * Note: When rotated, access to  the sprite pixels colors is not linear anymore. For certain
         *       orientations, this will yield very 'irregular' access to the sprite memory locations.
         *       When using large sprites in PROGMEM, this can result in huge slowdown as caching cannot
         *       be performed efficiently by the MCU. If this is the case, try moving the sprite to RAM
         *       (or another faster memory) before blitting...
         *
         * @tparam  color_t_src     Color type of the sprite image.
         * @tparam  BLEND_OPERATOR  Type of the blend operator.
         * @tparam  CACHE_SIZE      Size of the MCU cache when reading from flash. This value is
         *                          indicative and used to optimize cache access to flash memory. You may
         *                          try changing the default value it if drawing takes a long time...
         * @param   src_im          The sprite image to draw.
         * @param   anchor_src      Position of the anchor point in the sprite image.
         * @param   anchor_dst      Position of the anchor point in this (destination) image.
         * @param   scale           Scaling factor (1.0f for no scaling).
         * @param   angle_degrees   The rotation angle in degrees.
         * @param   blend_op        The blending operator.
        **/
        template<typename color_t_src, typename BLEND_OPERATOR, int CACHE_SIZE = TGX_PROGMEM_DEFAULT_CACHE_SIZE>
        void blitScaledRotated(const Image<color_t_src>& src_im, fVec2 anchor_src, fVec2 anchor_dst, float scale, float angle_degrees, const BLEND_OPERATOR& blend_op);


        /**
         * Blend a sprite onto this image after rescaling and rotation and use a given color which is
         * treated as fully transparent. This is especially useful when the sprite color type does not
         * have an alpha channel. The anchor point 'anchor_src' in the sprite is mapped to 'anchor_im'
         * in this image. The rotation is performed in counter-clockwise direction around the anchor
         * point.
         * 
         * Remarks:
         * 
         * 1. The positions are given using floating point values to allow for sub-pixel precision for
         *    smoother animation.
         * 2. The method use bilinear interpolation for high quality rendering.
         * 3. The sprite image can have a different color type from this image.
         * 
         * Note: When rotated, access to  the sprite pixels colors is not linear anymore. For certain
         *       orientations, this will yield very 'irregular' access to the sprite memory locations.
         *       When using large sprites in PROGMEM, this can result in huge slowdown as caching cannot
         *       be performed efficiently by the MCU. If this is the case, try moving the sprite to RAM
         *       (or another faster memory) before blitting...
         *
         * @tparam  color_t_src Color type of the sprite image.
         * @tparam  CACHE_SIZE  Size of the MCU cache when reading from flash. This value is indicative
         *                      and used to optimize cache access to flash memory. You may try changing
         *                      the default value it if drawing takes a long time...
         * @param   src_im              The sprite image to draw.
         * @param   transparent_color   The transparent color.
         * @param   anchor_src          Position of the anchor point in the sprite image.
         * @param   anchor_dst          Position of the anchor point in this (destination) image.
         * @param   scale               Scaling factor (1.0f for no scaling).
         * @param   angle_degrees       The rotation angle in degrees.
         * @param   opacity             Opacity multiplier when blending (in [0.0f, 1.0f]) 
        **/
        template<typename color_t_src, int CACHE_SIZE = TGX_PROGMEM_DEFAULT_CACHE_SIZE>
        void blitScaledRotatedMasked(const Image<color_t_src>& src_im, color_t_src transparent_color, fVec2 anchor_src, fVec2 anchor_dst, float scale, float angle_degrees, float opacity = 1.0f);


        /**
         * Copy (or blend) the src image onto the destination image with resizing and color conversion.
         * 
         * Remark:
         * 
         * 1. the source image is resized to match this image size. Bilinear interpolation is used to
         *    improve quality.  
         * 2. The source and destination image may have different color type. Conversion is automatic.
         * 
         * Beware: This method does not check for buffer overlap between source and destination !
         *
         * @tparam  src_color_t color type of the source image
         * @param   src_im  The image to copy into this image.
         * @param   opacity             Opacity multiplier when blending (in [0.0f, 1.0f]) or negative to
         *                              disable blending and simply use overwrite.
        **/
        template<typename src_color_t> void copyFrom(const Image<src_color_t>& src_im, float opacity = TGX_DEFAULT_BLENDING_MODE);



        /**
         * Blend the src image onto the destination image with resizing and color conversion.
         * 
         * This version uses a custom blending operator to combine src over the existing image.
         * 
         * The blending operator 'blend_op' can be a function/functor/lambda. It takes as input the
         * color of the source (sprite) pixel and the color of the dest. pixel and return the blended
         * color.
         *
         * Thus, it must be callable in the form:
         *
         *       blend_op(col_src, col_dst)
         *
         * where `col_src` and `col_dst` are colors of respective type color_t_src and color_t and it
         * must return a valid color (of any type but preferably of type color_t for best performance).
         * 
         * Remarks:
         * 
         * 1. the source image is resized to match this image size. Bilinear interpolation is used to
         *    improve quality.
         * 2. The source and destination image may have different color type. Conversion is automatic.  
         * 3. To perform "classical alpha blending", use the copyFrom() method with an opacity parameter   
         *    instead as it will be faster.
         * 
         * Beware: The method does not check for buffer overlap between source and destination !
         *
         * @tparam  src_color_t     color type of the source image
         * @param   src_im          Source image to copy onto this image.
         * @param   blend_op        The blending operator
        **/
        template<typename src_color_t, typename BLEND_OPERATOR> void copyFrom(const Image<src_color_t>& src_im, const BLEND_OPERATOR& blend_op);


        /**
         * Copy the source image pixels into this image, reducing it by half in the process. Ignore the
         * last row/column for odd dimensions larger than 1. Resizing is done by averaging the color of
         * the 4 neighbour pixels.
         * 
         * This image must be large enough to accomodate the reduced image otherwise do nothing. The
         * reduced image is copied on the top left corner of this image.
         * 
         * Return a sub-image of this image corresponding to the reduced image pixels (or an empty image
         * if nothing was done).
         * 
         * REMARK: This is an old method. Use blitScaledRotated() for a more powerful method.
         *
         * @param   src_image   the source image.
         *
         * @returns a subimage of this image that contain the reduced image.
        **/
        Image<color_t> copyReduceHalf(const Image<color_t> & src_image);


        /**
         * Reduce this image by half. Use the same memory buffer and keep the same stride. Resizing is
         * done by averaging the color of the 4 neighbour pixels. Same as copyReduceHalf(*this)
         * 
         * Return a sub-image of this image corresponding to the reduced image pixels which correspond
         * to the upper left half of this image.
         *
         * @returns a sub-image of this image that contains the reduce image. 
        **/
        Image<color_t> reduceHalf();






    /************************************************************************************************************
    *************************************************************************************************************
    *************************************************************************************************************
    * 
    *                                          DRAWING PRIMITIVES
    *       
    *************************************************************************************************************
    *************************************************************************************************************
    ************************************************************************************************************/



    /********************************************************************************************
    *********************************************************************************************
    *
    *                             FILLING (A REGION OF) AN IMAGE.
    *
    ********************************************************************************************* 
    ********************************************************************************************/


        /**
         * Fill the whole image with a single color
         *
         * @param   color   The color to use.
        **/
        void fillScreen(color_t color);


        /**
         * Fill the whole image with a vertical color gradient between two colors.
         * 
         * Interpolation takes place in RGB color space (even if color_t is HSV).
         *
         * @param   top_color       color at the top of the image.
         * @param   bottom_color    color at the bottom of the image.
        **/
        void fillScreenVGradient(color_t top_color, color_t bottom_color);


        /**
         * Fill the whole screen with an horizontal color gradient between two colors.
         * 
         * Interpolation takes place in RGB color space (even if color_t is HSV).
         *
         * @param   left_color  color on the left side of the image.
         * @param   right_color color on the right side of the image.
        **/
        void fillScreenHGradient(color_t left_color, color_t right_color);




        /**
        * 'Flood fill' a 4-connected region of the image.
        *
        * Recolor the unicolor component containing position 'start_pos' with the color 'new_color'.
        *
        * The template parameter can be adjusted to specify the size (in bytes) allocated on the stack.
        * If the algorithm runs out of space, it stops without completing the filling (and return -1 to indicate failure).
        * Otherwise, the method return the max number of bytes used on the stack during the filling.
        *
        * @param	start_pos   Start position. The color to replace is the color at that position.
        * @param	new_color   New color to use
        * @return   return the max stack used during the algorithm. Return -1 if we run out of memory (in this
        *           case the method returns early without completing the full filling.
        **/
        template<int STACK_SIZE = 1024> int fill(iVec2 start_pos, color_t new_color);



        /**
        * 'Flood fill' a 4-connected region of the image.
        *
        * Recolor the connected component containing position 'startpos' whose boundary is delimited by 'border_color'.
        *
        * The template parameter can be adjusted to specify the size (in bytes) allocated on the stack.
        * If the algorithm runs out of space, it stops without completing the filling (and return -1 to indicate failure).
        * Otherwise, the method return the max number of bytes used on the stack during the filling.
        *
        * NOTE: During the algorithm, 'new_color' is treated the same as 'border_color' and will also block the
        *       filling when encountered.
        *
        * @param	start_pos		Start position.
        * @param	border_color	border color that delimits the connected component to fill.
        * @param	new_color		New color to use
        * @return   return the max stack used during the algorithm. Return -1 if we run out of memory (in this
        *           case the method returns early without completing the full filling.
        **/
        template<int STACK_SIZE = 1024> int fill(iVec2 start_pos, color_t border_color, color_t new_color);






    /********************************************************************************************
    *********************************************************************************************
    *
    *                                       DRAWING LINES
    *
    ********************************************************************************************* 
    ********************************************************************************************/



    /**********************************************************************
    *                          DRAWING LINES
    *                          
    *                      'Low' quality methods
    ***********************************************************************/



        /**
         * Draw an vertical segment of h pixels starting at pos.
         *
         * @param   pos     position of the top endpoint of the segment.
         * @param   h       number of pixels in the segment.
         * @param   color   color to use.
         * @param   opacity opacity multiplier when blending (in [0.0f, 1.0f]) or negative to disable
         *                  blending.
        **/
        void drawFastVLine(iVec2 pos, int h, color_t color, float opacity = TGX_DEFAULT_BLENDING_MODE);


        /**
         * Draw an horizontal segment of h pixels starting at pos.
         *
         * @param   pos     position of the left endpoint of the segment.
         * @param   w       number of pixels in the segment.
         * @param   color   color to use.
         * @param   opacity opacity multiplier when blending (in [0.0f, 1.0f]) or negative to disable
         *                  blending and simply use overwritting.
        **/
        void drawFastHLine(iVec2 pos, int w, color_t color, float opacity = TGX_DEFAULT_BLENDING_MODE);


        /**
         * Draw a line segment between two points (using Bresenham's algorithm).
         *
         * @param   P1      The first point.
         * @param   P2      The second point.
         * @param   color   color to use.
         * @param   opacity (Optional) opacity multiplier when blending (in [0.0f, 1.0f]) or negative to
         *                  disable blending and simply use overwritting.
        **/
        void drawLine(iVec2 P1, iVec2 P2, color_t color, float opacity = TGX_DEFAULT_BLENDING_MODE);


        /**
         * Draw a line segment between two points (using Bresenham's algorithm) 
         * Same as drawLine() but possibly without drawing one or both endpoints.
         *
         * @param   P1      The first point.
         * @param   drawP1  True to draw the pixel at endpoint P1.
         * @param   P2      The second point.
         * @param   drawP2  True to draw the pixel at endpoint P2.
         * @param   color   color to use.
         * @param   opacity (Optional) opacity multiplier when blending (in [0.0f, 1.0f]) or negative to
         *                  disable blending and simply use overwritting.
        **/
        void drawSegment(iVec2 P1, bool drawP1, iVec2 P2, bool drawP2, color_t color, float opacity = TGX_DEFAULT_BLENDING_MODE);




    /**********************************************************************
    *                          DRAWING LINES
    *                          
    *                      'High' quality methods
    ***********************************************************************/



        /**
         * Draw a smooth (i.e. antialiased with sub-pixel precision) line segment between two points
         * (using Bresenham's algorithm)
         *
         * @param   P1      The first point.
         * @param   P2      The second point.
         * @param   color   color to use.
         * @param   opacity opacity multiplier between 0.0f and 1.0f (default).
        **/
        void drawSmoothLine(fVec2 P1, fVec2 P2, color_t color, float opacity = 1.0f);


        /**
         * Draw a smooth (i.e. antialiased with sub-pixel precision) thick line segment between two
         * points (using Bresenham's algorithm)
         *
         * @param   P1              The first point.
         * @param   P2              The second point.
         * @param   line_width      Width of the line.
         * @param   rounded_ends    true to draw rounded ends on the line extremities
         * @param   color           color to use.
         * @param   opacity opacity multiplier between 0.0f and 1.0f (default).
        **/
        void drawSmoothThickLine(fVec2 P1, fVec2 P2, float line_width, bool rounded_ends, color_t color, float opacity = 1.0f);


        /**
         * Draw a smooth (i.e. antialiased with sub-pixel precision) wedge line from P1 to P2 with
         * rounded ends and with respective wideness line_width_P1 and line_width_P2 at both ends.
         * 
         * CREDIT: code for wedge with rounded edges is adapted Bodmer TFT_eSPI library:
         * https://github.com/Bodmer/TFT_eSPI
         *
         * @param   P1              first end point.
         * @param   P2              second end point.
         * @param   line_width_P1   width of the wedge at P1.
         * @param   line_width_P2   width of the wedge at P2
         * @param   rounded_ends    true to draw rounded ends on the extremities
         * @param   color           color to use.
         * @param   opacity opacity multiplier between 0.0f and 1.0f (default).
        **/
        void drawSmoothWedgeLine(fVec2 P1, fVec2 P2, float line_width_P1, float line_width_P2, bool rounded_ends, color_t color, float opacity = 1.0f);






    /********************************************************************************************
    *********************************************************************************************
    *
    *                                    DRAWING RECTANGES
    *
    ********************************************************************************************* 
    ********************************************************************************************/



    /**********************************************************************
    *                        DRAWING RECTANGLES
    *
    *                       'Low' quality methods
    ***********************************************************************/


        /**
         * Draw a rectangle.
         *
         * @param   B       Box that delimits the rectangle to draw.
         * @param   color   rectangle outline color.
         * @param   opacity (Optional) opacity multiplier when blending (in [0.0f, 1.0f] or negative to
         *                  disable blending.
        **/        
        void drawRect(const iBox2 & B, color_t color, float opacity = TGX_DEFAULT_BLENDING_MODE);


        /**
         * Draw a rectangle with a thick border.
         *
         * @param   B           Box that delimits the rectangle to draw.
         * @param   thickness   thickness of the border (going 'inside the rectangle)
         * @param   color       rectangle outline color.
         * @param   opacity     (Optional) opacity multiplier when blending (in [0.0f, 1.0f] or negative
         *                      to disable blending.
        **/
        void drawThickRect(const iBox2& B, int thickness, color_t color, float opacity = TGX_DEFAULT_BLENDING_MODE);


        /**
         * Draw a filled rectangle.
         *
         * @param   B       box that delimits the rectangle to draw.
         * @param   color   rectangle color.
         * @param   opacity (Optional) opacity multiplier when blending (in [0.0f, 1.0f]) or negative to
         *                  disable blending.
        **/
        void fillRect(const iBox2 & B, color_t color, float opacity = TGX_DEFAULT_BLENDING_MODE);


        /**
         * Draw a filled rectangle with a thick border of a possibly different color.
         *
         * @param   B               box that delimits the rectangle to draw.
         * @param   thickness       thickness of the border (going 'inside the rectangle)
         * @param   color_interior  color for the rectangle interior.
         * @param   color_border    color for the rectangle border.
         * @param   opacity         (Optional) opacity multiplier when blending (in [0.0f, 1.0f]) or
         *                          negative to disable blending.
        **/
        void fillThickRect(const iBox2& B, int thickness, color_t color_interior, color_t color_border, float opacity = TGX_DEFAULT_BLENDING_MODE);


        /**
         * Draw a rectangle filled with an horizontal gradient of colors
         *
         * @param   B           box that delimits the rectangle to draw.
         * @param   color_left  color on the left side
         * @param   color_right color on the right side
         * @param   opacity     (Optional) opacity multiplier when blending (in [0.0f, 1.0f]) or negative
         *                      to disable blending.
        **/
        void fillRectHGradient(iBox2 B, color_t color_left, color_t color_right, float opacity = TGX_DEFAULT_BLENDING_MODE);


        /**
         * Draw a rectangle filled with a vertical gradient of colors
         *
         * @param   B       box that delimits the rectangle to draw.
         * @param   color1  color on the top side.
         * @param   color2  color on the bottom side.
         * @param   opacity (Optional) opacity multiplier when blending (in [0.0f, 1.0f]) or negative to
         *                  disable blending.
        **/
        void fillRectVGradient(iBox2 B, color_t color_top, color_t color_bottom, float opacity = TGX_DEFAULT_BLENDING_MODE);




    /**********************************************************************
    *                        DRAWING RECTANGLES
    *
    *                      'High' quality methods
    *                      
    * WARNING: These methods are only useful for creating smooth animations
    *          otherwise, use the 'low' quality method that will prevent 
    *          unwanted aliasing...
    ***********************************************************************/


        /**
         * Draw a filed rectangle wih sub-pixel precision and anti-aliasing and thick border.
         * 
         * WARNING: This is probably not the method you want to use... If you just want to draw
         *          a nice rectangle with a thick border, try drawThickRect() instead. This method 
         *          is useful for creating very smooths animations (when the rectangle moves by less 
         *          than a pixel).
         *          
         * NOTE: Recall that pixel centers are at integer values (and the full image range is
         *       [-0.5, lx - 0.5] x [-0.5, lx-0.5]), therefore, giving a box with integer values to this
         *       method will create aliasing along the edges of the rectangle...
         *
         * @param   B           Box representing the outer boundary of the rectangle.
         * @param   thickness   thickness of the boundary (going 'inside' the rectangle)
         * @param   color       color to use
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void drawSmoothThickRect(const fBox2& B, float thickness, color_t color, float opacity = 1.0f);


        /**
         * Draw a filed rectangle wih sub-pixel precision and anti-aliasing.
         *
         * WARNING: This is probably not the method you want to use... If you just want to draw
         *          a nice filled rectangle use try drawRect() instead. This method is only useful 
         *          for creating very smooths animations (when the rectangle moves by less than a pixel).
         *
         * NOTE: Recall that pixel centers are at integer values (and the full image range is
         *       [-0.5, lx - 0.5] x [-0.5, lx-0.5]), therefore, giving a box with integer values to this
         *       method will create aliasing along the edges of the rectangle...
         *
         * @param   B       Box representing the rectngle to daw.
         * @param   color   color.
         * @param   opacity (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void fillSmoothRect(const fBox2& B, color_t color, float opacity = 1.0f);


        /**
         * Draw a filed rectangle wih sub-pixel precision and anti-aliasing and thick border.
         * 
         * WARNING: This is probably not the method you want to use... If you just want to draw
         *          a nice filled rectangle with a thick border, try fillThickRect() instead. This method
         *          is only useful for creating very smooths animations (when the rectangle moves by less
         *          than a pixel).
         *
         * NOTE: Recall that pixel centers are at integer values (and the full image range is
         *       [-0.5, lx - 0.5] x [-0.5, lx-0.5]), therefore, giving a box with integer values to this
         *       method will create aliasing along the edges of the rectangle...
         *
         * @param   B               Box representing the outer boundary of the rectangle
         * @param   thickness       thickness of the boundary (going 'inside' the rectangle)
         * @param   color_interior  color for the interior
         * @param   color_border    color for the boundary
         * @param   opacity         (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void fillSmoothThickRect(const fBox2& B, float thickness, color_t color_interior, color_t color_border, float opacity = 1.0f);
 





    /********************************************************************************************
    *********************************************************************************************
    *
    *                                    DRAWING ROUNDED RECTANGES
    *
    ********************************************************************************************* 
    ********************************************************************************************/




    /**********************************************************************
    *                     DRAWING ROUNDED RECTANGLES
    *
    *                      'Low' quality methods
    ***********************************************************************/



        /**
         * Draw a rounded rectangle in box B with corner radius r.
         *
         * @param   B               box that delimits the rectangle to draw.
         * @param   corner_radius   corner radius.
         * @param   color           rectangle color.
         * @param   opacity         (Optional) opacity multiplier when blending (in [0.0f, 1.0f]) or
         *                          negative to disable blending.
        **/
        void drawRoundRect(const iBox2& B, int corner_radius, color_t color, float opacity = TGX_DEFAULT_BLENDING_MODE);


        /**
         * Draw a filled rounded rectangle in box B with corner radius r.
         *
         * @param   B               box that delimits the rectangle to draw.
         * @param   corner_radius   corner radius.
         * @param   color           rectangle color.
         * @param   opacity         (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or
         *                          negative to disable blending.
        **/
        void fillRoundRect(const iBox2& B, int corner_radius, color_t color, float opacity = TGX_DEFAULT_BLENDING_MODE);




    /**********************************************************************
    *                     DRAWING ROUNDED RECTANGLES
    *
    *                       'High' quality methods
    ***********************************************************************/



        /**
         * Draw a smooth (i.e. anti-aliased with subpixel precision) rounded rectangle.
         * 
         * @param   B               Box representing the rectngle to daw.
         * @param   corner_radius   corner radius.
         * @param   color           color.
         * @param   opacity         (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void drawSmoothRoundRect(const fBox2& B, float corner_radius, color_t color, float opacity = 1.0f);


        /**
         * Draw a smooth (i.e. anti-aliased with subpixel precision) thick rounded rectangle.
         * 
         * @param   B               Box representing the rectngle to daw.
         * @param   corner_radius   corner radius.
         * @param   thickness       thickness going 'inside' the rounded rectangle (should be >1)
         * @param   color           color.
         * @param   opacity         (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void drawSmoothThickRoundRect(const fBox2& B, float corner_radius, float thickness, color_t color, float opacity = 1.0f);


        /**
         * Draw a filled smooth (i.e. anti-aliased with subpixel precision) rounded rectangle.
         * 
         * @param   B               Box representing the rectngle to daw.
         * @param   corner_radius   corner radius.
         * @param   color           color.
         * @param   opacity         (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void fillSmoothRoundRect(const fBox2& B, float corner_radius, color_t color, float opacity = 1.0f);


        /**
         * Draw a smooth (i.e. anti-aliased with subpixel precision) filled rounded rectangle with thick
         * border of another color.
         *
         * @param   B               Box representing the rectngle to daw.
         * @param   corner_radius   corner radius.
         * @param   thickness       thickness going 'inside' the rounded rectangle (should be >1)
         * @param   color_interior  color.
         * @param   color_border    The color border.
         * @param   opacity         (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void fillSmoothThickRoundRect(const fBox2& B, float corner_radius, float thickness, color_t color_interior, color_t color_border, float opacity = 1.0f);





    /********************************************************************************************
    *********************************************************************************************
    *
    *                                    DRAWING TRIANGLES
    *
    *********************************************************************************************
    ********************************************************************************************/




    /**********************************************************************
    *                        DRAWING TRIANGLES
    *
    *                      'low' quality methods
    ***********************************************************************/



        /**
         * Draw a triangle.
         *
         * @param   P1      first vertex.
         * @param   P2      second vertex.
         * @param   P3      third vertex.
         * @param   color   The color to use.
         * @param   opacity Opacity multiplier when blending (in [0.0f, 1.0f]) or negative to disable
         *                  blending and simply use overwrite.
        **/
        void drawTriangle(const iVec2& P1, const iVec2& P2, const iVec2& P3, color_t color, float opacity = TGX_DEFAULT_BLENDING_MODE);



        /**
         * Draw a filled triangle with possibly different colors for the outline and the interior.
         *
         * @param   P1              first vertex.
         * @param   P2              second vertex.
         * @param   P3              third vertex.
         * @param   interior_color  color for the interior.
         * @param   outline_color   color for the outline.
         * @param   opacity         (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or
         *                          negative to disable blending and simply use overwrite.
        **/
        void fillTriangle(const iVec2& P1, const iVec2& P2, const iVec2& P3, color_t interior_color, color_t outline_color, float opacity = TGX_DEFAULT_BLENDING_MODE);




    /**********************************************************************
    *                        DRAWING TRIANGLES
    *
    *                      'high' quality methods
    ***********************************************************************/


        /**
         * Draw a smooth (with anti-aliasing and sub-pixel precision) triangle
         *
         * @param   P1      first vertex.
         * @param   P2      second vertex.
         * @param   P3      third vertex.
         * @param   color   The color.
         * @param   opacity (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void drawSmoothTriangle(fVec2 P1, fVec2 P2, fVec2 P3, color_t color, float opacity = 1.0f);


        /**
         * Draw smooth thick triangle. The vertex coordinates (P1, P2, P3) are those for the exterior
         * outline of the triangle and the thickness is 'inside' the triangle.
         *
         * @param   P1          first vertex.
         * @param   P2          second vertex.
         * @param   P3          third vertex.
         * @param   thickness   The thickness. Should be >=1 otherwise use drawSmoothTriangle() instead.
         * @param   color       The color.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void drawSmoothThickTriangle(fVec2 P1, fVec2 P2, fVec2 P3, float thickness, color_t color, float opacity = 1.0f);


        /**
         * Fill smooth (with anti-aliasing and sub-pixel precision) triangle
         *
         * @param   P1      first vertex.
         * @param   P2      second vertex.
         * @param   P3      third vertex.
         * @param   color   The color.
         * @param   opacity (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void fillSmoothTriangle(fVec2 P1, fVec2 P2, fVec2 P3, color_t color, float opacity = 1.0f);


        /**
         * Fill a smooth (with anti-aliasing and sub-pixel precision) triangle with a thick outline of a
         * possibly different color.
         *
         * @param   P1              first vertex.
         * @param   P2              second vertex.
         * @param   P3              third vertex.
         * @param   thickness       thickness of the border (going 'inside' the triangle). 
         * @param   color_interior  interior color.
         * @param   color_border    boundary color.
         * @param   opacity         (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void fillSmoothThickTriangle(fVec2 P1, fVec2 P2, fVec2 P3, float thickness, color_t color_interior, color_t color_border, float opacity = 1.0f);






    /**********************************************************************
    *                        DRAWING TRIANGLES
    *
    * ADVANCED METHODS FOR TEXTURING/GRADIENT USING THE 3D RASTERIZER BACKEND
    *
    * Remarks: 1. For all these methods, the positions are given using floating
    *             point values to allow for sub-pixel precision (improves animations)
    *          2. Sprite images and gradient colors can have different types from
    *             the destination (i.e. this) image.
    *
    * Warning: For particular orientations of triangles, access to texture pixels is
    *          highly non-linear. For texture stored in slow memory (e.g. flash), this
    *          can cause huge slowdown because the read cache becomes basically useless.
    *          To overcome this problem:
    *              a. Move the texture to a faster memory location before drawing.
    *              b. Tessellate the triangle into smaller triangles so that the
    *                 memory data for each triangle fit into the cache.
    ***********************************************************************/



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
        **/
        template<typename color_alt>
        void drawGradientTriangle(fVec2 P1, fVec2 P2, fVec2 P3, color_alt colorP1, color_alt colorP2, color_alt colorP3, float opacity = TGX_DEFAULT_BLENDING_MODE);


        /**
         * Draw textured triangle using bilinear filtering for high quality rendering.
         *
         * @param   src_im  the image/texture to map onto the triangle.
         * @param   srcP1   coords of point 1 on the texture.
         * @param   srcP2   coords of point 2 on the texture.
         * @param   srcP3   coords of point 3 on the texture.
         * @param   dstP1   coords of point 1 on this image.
         * @param   dstP2   coords of point 2 on this image.
         * @param   dstP3   coords of point 3 on this image.
         * @param   opacity Opacity multiplier when blending (in [0.0f, 1.0f]) or negative to disable
         *                  blending and simply use overwrite.
        **/
        template<typename color_t_tex>
        void drawTexturedTriangle(const Image<color_t_tex>& src_im, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, float opacity = TGX_DEFAULT_BLENDING_MODE);


        /**
         * Blend a textured triangle over this image using bilinear filtering and a custom blend operator.
         *
         * The blending operator 'blend_op' can be a function/functor/lambda. It takes as input the
         * color of the source (sprite) pixel and the color of the dest. pixel and return the blended
         * color. It must be callable in the form:
         *
         *       blend_op(col_src, col_dst)
         *
         * where `col_src` and `col_dst` are colors of respective type color_t_src and color_t and it
         * must return a valid color (of any type but preferably of type color_t for best performance).         *
         * 
         * @param   src_im  the image/texture to map onto the triangle.
         * @param   srcP1   coords of point 1 on the texture.
         * @param   srcP2   coords of point 2 on the texture.
         * @param   srcP3   coords of point 3 on the texture.
         * @param   dstP1   coords of point 1 on this image.
         * @param   dstP2   coords of point 2 on this image.
         * @param   dstP3   coords of point 3 on this image.
         * @param   blend_op    the blending operator
        **/
        template<typename color_t_tex, typename BLEND_OPERATOR>
        void drawTexturedTriangle(const Image<color_t_tex>& src_im, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, const BLEND_OPERATOR& blend_op);


        /**
         * Blend textured triangle using bilinear filtering combined with a color gradient.
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
        **/
        template<typename color_t_tex>
        void drawTexturedGradientTriangle(const Image<color_t_tex>& src_im, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, color_t_tex C1, color_t_tex C2, color_t_tex C3, float opacity = TGX_DEFAULT_BLENDING_MODE);


        /**
         * Blend textured triangle with a transparency mask (i.e. a specific color is treated as fully transparent)
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
        **/
        template<typename color_t_tex>
        void drawTexturedMaskedTriangle(const Image<color_t_tex>& src_im, color_t_tex transparent_color, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, float opacity = 1.0f);


        /**
         * Blend textured triangle with a transparency mask (i.e. a specific color is treated as fully
         * transparent) and blend it with a color gradient...
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
        **/
        template<typename color_t_tex>
        void drawTexturedGradientMaskedTriangle(const Image<color_t_tex>& src_im, color_t_tex transparent_color, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, color_t_tex C1, color_t_tex C2, color_t_tex C3, float opacity = 1.0f);







    /********************************************************************************************
    *********************************************************************************************
    *
    *                                     DRAWING QUADS
    *
    *********************************************************************************************
    ********************************************************************************************/



    /**********************************************************************
    *                           DRAWING QUADS
    *
    *                      'low' quality methods
    ***********************************************************************/



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
        **/
        void drawQuad(iVec2 P1, iVec2 P2, iVec2 P3, iVec2 P4, color_t color, float opacity = TGX_DEFAULT_BLENDING_MODE);


        /**
         * draw a filled quad.
         * 
         * WARNING : The quad must be CONVEX !
         *
         * @param   P1      first vertex.
         * @param   P2      second vertex.
         * @param   P3      third vertex.
         * @param   P4      fourth vertex.
         * @param   color   color.
         * @param   opacity (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative to
         *                  disable blending and simply use overwrite.
        **/
        void fillQuad(iVec2 P1, iVec2 P2, iVec2 P3, iVec2 P4, color_t color, float opacity = TGX_DEFAULT_BLENDING_MODE);




    /**********************************************************************
    *                           DRAWING QUADS
    *
    *                      'high' quality methods
    ***********************************************************************/


        /**
         * Draw a smooth (anti-aliased with sub-pixel precision) quad.
         *
         * @param   P1      The first vertex.
         * @param   P2      The second vertex.
         * @param   P3      The third vertex.
         * @param   P4      The fourth vertex.
         * @param   color   The color to use.
         * @param   opacity (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void drawSmoothQuad(fVec2 P1, fVec2 P2, fVec2 P3, fVec2 P4, color_t color, float opacity = 1.0f);


        /**
         * Draw a smooth (anti-aliased with sub-pixel precision) quad.
         * 
         * WARNING : The quad must be CONVEX !
         *
         * @param   P1          The first vertex.
         * @param   P2          The second vertex.
         * @param   P3          The third vertex.
         * @param   P4          The fourth vertex.
         * @param   thickness   The thickness (going inside the polygon delimited by P1,P2,P3,P4)
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void drawSmoothThickQuad(fVec2 P1, fVec2 P2, fVec2 P3, fVec2 P4, float thickness, color_t color, float opacity = 1.0f);


        /**
         * Fill (with anti-aliasing and sub-pixel precision) smooth quad.
         * 
         * WARNING : The quad must be CONVEX !
         *
         * @param   P1      first vertex.
         * @param   P2      second vertex.
         * @param   P3      third vertex.
         * @param   P4      fourth vertex.
         * @param   color   color.
         * @param   opacity (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void fillSmoothQuad(fVec2 P1, fVec2 P2, fVec2 P3, fVec2 P4, color_t color, float opacity = 1.0f);


        /**
         * Fill a smooth (with anti-aliasing and sub-pixel precision) quad with a thick outline of a
         * possibly different color.
         * 
         * WARNING : The quad must be CONVEX !
         *
         * @param   P1              first vertex.
         * @param   P2              second vertex.
         * @param   P3              third vertex.
         * @param   P4              fourth vertex.
         * @param   thickness       The thickness (going inside the polygon delimited by P1,P2,P3,P4)
         * @param   color_interior  interior color.
         * @param   color_border    boundary color.
         * @param   opacity         (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void fillSmoothThickQuad(fVec2 P1, fVec2 P2, fVec2 P3, fVec2 P4, float thickness, color_t color_interior, color_t color_border, float opacity = 1.0f);




    /**********************************************************************
    *                            DRAWING QUADS
    *
    * ADVANCED METHODS FOR TEXTURING/GRADIENT USING THE 3D RASTERIZER BACKEND
    *
    * Remarks: 1. For all these methods, the positions are given using floating
    *             point values to allow for sub-pixel precision (improves animations)
    *          2. Sprite images and gradient colors can have different types from
    *             the destination (i.e. this) image.
    *
    * Warning: For particular orientations of triangles, access to texture pixels is
    *          highly non-linear. For texture stored in slow memory (e.g. flash), this
    *          can cause huge slowdown because the read cache becomes basically useless.
    *          To overcome this problem:
    *              a. Move the texture to a faster memory location before drawing.
    *              b. Tessellate the triangle into smaller triangles so that the
    *                 memory data for each triangle fit into the cache.
    ***********************************************************************/





        /**
        * Draw a quad with a gradient color specified by the color at the four vertices.
        *
        * See drawGradientTriangle() for details.
        * NOTE: the vertices can be given in clockwise or counter-clockwise order.
        **/
        template<typename color_alt>
        void drawGradientQuad(fVec2 P1, fVec2 P2, fVec2 P3, fVec2 P4, color_alt colorP1, color_alt colorP2, color_alt colorP3, color_alt colorP4, float opacity = TGX_DEFAULT_BLENDING_MODE);


        /**
        * Draw a textured quad using bilinear filtering.
        *
        * See drawTexturedTriangle() for details.
        * NOTE: the vertices can be given either in clockwise or counter-clockwise order.
        **/
        template<typename color_t_tex>
        void drawTexturedQuad(const Image<color_t_tex>& src_im, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 srcP4, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, fVec2 dstP4, float opacity = TGX_DEFAULT_BLENDING_MODE);


        /**
        * Draw a textured quad with bilinear filtering and a custom blending operator.
        *
        * See drawTexturedTriangle() for more details.
        * NOTE: the vertices can be given either in clockwise or counter-clockwise order.
        **/
        template<typename color_t_tex, typename BLEND_OPERATOR>
        void drawTexturedQuad(const Image<color_t_tex>& src_im, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 srcP4, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, fVec2 dstP4, const BLEND_OPERATOR& blend_op);


        /**
        * Draw a textured quad using bilinear filtering and combined with a color gradient.
        *
        * See drawTexturedTriangleGradient() for details.
        * NOTE: the vertices can be given either in clockwise or counter-clockwise order.
        **/
        template<typename color_t_tex>
        void drawTexturedGradientQuad(const Image<color_t_tex>& src_im, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 srcP4, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, fVec2 dstP4, color_t_tex C1, color_t_tex C2, color_t_tex C3, color_t_tex C4, float opacity = TGX_DEFAULT_BLENDING_MODE);


        /**
        * Draw a textured quad using bilinear filtering and with a mask (i.e. a fixed transparent color).
        *
        * See drawTexturedTriangleMasked() for details.
        * NOTE: the vertices can be given either in clockwise or counter-clockwise order.
        **/
        template<typename color_t_tex>
        void drawTexturedMaskedQuad(const Image<color_t_tex>& src_im, color_t_tex transparent_color, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 srcP4, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, fVec2 dstP4, float opacity = 1.0f);


        /**
        * Draw a textured quad using bilinear filtering and with a mask (i.e. a fixed transparent color) 
        * combined with a color gradient.
        *
        * See drawTexturedGradientMaskedTriangle() for details.
        * NOTE: the vertices can be given either in clockwise or counter-clockwise order.
        **/
        template<typename color_t_tex>
        void drawTexturedGradientMaskedQuad(const Image<color_t_tex>& src_im, color_t_tex transparent_color, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 srcP4, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, fVec2 dstP4, color_t_tex C1, color_t_tex C2, color_t_tex C3, color_t_tex C4, float opacity = 1.0f);








    /********************************************************************************************
    *********************************************************************************************
    *
    *                                    DRAWING POLYLINES
    *
    *********************************************************************************************
    ********************************************************************************************/




    /**********************************************************************
    *                           DRAWING POLYLINES
    *
    *                        'low' quality methods
    ***********************************************************************/



        /**
         * Draw a polyline i.e. a sequence of consecutif segments [P0,P1] , [P1,P2], ... , [Pn-1,Pn]
         *
         * @param   nbpoints    number of points in tabPoints.
         * @param   tabPoints   array of points.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative
         *                      to disable blending and simply use overwrite.
        **/
        void drawPolyline(int nbpoints, const iVec2 tabPoints[], color_t color, float opacity = TGX_DEFAULT_BLENDING_MODE);


        /**
         * Draw a polyline i.e. a sequence of consecutif segments [P0,P1] , [P1,P2], ... , [Pn-1,Pn]
         * 
         * Points are queried in order P0, P1,... using a functor callback which must have a signature
         * compatible with: 
         *                 bool next_point(iVec2 & P)
         *                 
         * The callback must store the next point in the reference P and return:
         * - true  if there are more point to plot after this one.
         * - false if this is the last point.
         *
         * @param   next_point  callback functor that provides the list of points.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative
         *                      to disable blending and simply use overwrite.
        **/
        template<typename FUNCTOR_NEXT>
        void drawPolyline(FUNCTOR_NEXT next_point, color_t color, float opacity = TGX_DEFAULT_BLENDING_MODE);




    /**********************************************************************
    *                          DRAWING POLYLINES
    *
    *                       'high' quality methods
    ***********************************************************************/



        /**
         * Draw a smooth (antialised with subpixel precision) polyline i.e. a sequence of consecutif
         * segments [P0,P1] , [P1,P2], ... , [Pn-1,Pn]
         *
         * @param   nbpoints    number of points in tabPoints.
         * @param   tabPoints   array of points.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void drawSmoothPolyline(int nbpoints, const fVec2 tabPoints[], color_t color, float opacity = 1.0f);


        /**
         * Draw a smooth (antialised with subpixel precision) polyline i.e. a sequence of consecutif
         * segments [P0,P1] , [P1,P2], ... , [Pn-1,Pn]
         * 
         * Points are queried in order P0, P1,... using a functor callback which must have a signature
         * compatible with:
         *                 bool next_point(fVec2 & P)
         *
         * The callback must store the next point in the reference P and return:
         * - true  if there are more point to plot after this one.
         * - false if this is the last point.
         *
         * @param   next_point  callback functior that privdes the list of points.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        template<typename FUNCTOR_NEXT>
        void drawSmoothPolyline(FUNCTOR_NEXT next_point, color_t color, float opacity = 1.0f);


        /**
         * Draw a thick smooth (antialised with subpixel precision) polyline i.e. a sequence of
         * consecutif segments [P0,P1] , [P1,P2], ... , [Pn-1,Pn]
         *
         * @param   nbpoints        number of points in tabPoints.
         * @param   tabPoints       array of points.
         * @param   thickness       thickness of the polyline.
         * @param   rounded_ends    True to draw rounded ends on the polyline extremities and false to
         *                          draw straight ends.
         * @param   color           The color to use.
         * @param   opacity         (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void drawSmoothThickPolyline(int nbpoints, const fVec2 tabPoints[], float thickness, bool rounded_ends, color_t color, float opacity = 1.0f);


        /**
         * Draw a thick smooth (antialised with subpixel precision) polyline i.e. a sequence of
         * consecutif segments [P0,P1] , [P1,P2], ... , [Pn-1,Pn]
         * 
         * Points are queried in order P0, P1,... using a functor callback which must have a signature
         * compatible with:
         *                 bool next_point(fVec2 & P)
         * 
         * The callback must store the next point in the reference P and return:
         * - true  if there are more point to plot after this one.
         * - false if this is the last point.
         *
         * @param   next_point      callback functior that provides the list of points.
         * @param   thickness       thickness of the polyline.
         * @param   rounded_ends    True to draw rounded ends on the polyline extremities and false to
         *                          draw straight ends.
         * @param   color           The color to use.
         * @param   opacity         (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        template<typename FUNCTOR_NEXT>
        void drawSmoothThickPolyline(FUNCTOR_NEXT next_point, float thickness, bool rounded_ends, color_t color, float opacity = 1.0f);






    /********************************************************************************************
    *********************************************************************************************
    *
    *                                    DRAWING POLYGONS
    *
    *********************************************************************************************
    ********************************************************************************************/



    /**********************************************************************
    *                           DRAWING POLYGONS
    *
    *                        'low' quality methods
    ***********************************************************************/


        /**
         * Draw a closed polygon with vertices [P0,P2, .., PN]
         *
         * @param   nbpoints    number of points in tabPoints.
         * @param   tabPoints   array of points of the polygon.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative
         *                      to disable blending and simply use overwrite.
        **/
        void drawPolygon(int nbpoints, const iVec2 tabPoints[], color_t color, float opacity = TGX_DEFAULT_BLENDING_MODE);



        /**
         * Draw a closed polygon with vertices [P0,P2, .., PN]
         *
         * Points are queried in order P0, P1,... using a functor callback which must have a signature
         * compatible with:
         *                 bool next_point(iVec2 & P)
         *
         * The callback must store the next point in the reference P and return:
         * - true  if there are more point to plot after this one.
         * - false if this is the last point
         *
         * @param   next_point  callback functor that provides the list of points.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative
         *                      to disable blending and simply use overwrite.
        **/
        template<typename FUNCTOR_NEXT>
        void drawPolygon(FUNCTOR_NEXT next_point, color_t color, float opacity = TGX_DEFAULT_BLENDING_MODE);


        /**
         * Draw a filled polygon with vertices [P0,P2, .., PN]
         * 
         * WARNING : The polygon must be star-shaped w.r.t the barycenter of its points 
         *           (so it is ok for convex polygons...)
         *
         * @param   nbpoints    number of points in tabPoints.
         * @param   tabPoints   array of points of the polygon.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative
         *                      to disable blending and simply use overwrite.
        **/
        void fillPolygon(int nbpoints, const iVec2 tabPoints[], color_t color, float opacity = TGX_DEFAULT_BLENDING_MODE);


        /**
         * Draw a filled polygon with vertices [P0,P2, .., PN]
         *
         * Points are queried in order P0, P1,... using a functor callback which must have a signature
         * compatible with:
         *                 bool next_point(iVec2 & P)
         *
         * The callback must store the next point in the reference P and return:
         * - true  if there are more point to plot after this one.
         * - false if this is the last point AND THEN THE FUNCTOR MUST RESET BACK THE FIRST POINT !
         *
         * WARNING:  In order to draw the polygon correctly, all points must be queried twice so that after
         *           finishing the first iteration (by returning false), next_point() must reset to the
         *           first point (because and all points are queried a second time) !
         *           
         * WARNING : The polygon must be star-shaped w.r.t the barycenter of its points
         *           (so it is ok for convex polygons...)
         *
         * @param   next_point  callback functor that provides the list of points.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative
         *                      to disable blending and simply use overwrite.
        **/
        template<typename FUNCTOR_NEXT>
        void fillPolygon(FUNCTOR_NEXT next_point, color_t color, float opacity = TGX_DEFAULT_BLENDING_MODE);





        /**********************************************************************
        *                           DRAWING POLYGONS
        *
        *                        'high' quality methods
        ***********************************************************************/



        /**
         * Draw a smooth (anti-aliased with sub-pixel precision) polygon.
         *
         * @param   nbpoints    number of points in tabPoints.
         * @param   tabPoints   array of points of the polygon: these points delimit the exterior
         *                      boundary of the polygon and the thickness of the line goes 'inside' the
         *                      polygon.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void drawSmoothPolygon(int nbpoints, const fVec2 tabPoints[], color_t color, float opacity = 1.0f);


        /**
         * Draw a smooth (anti-aliased with sub-pixel precision) polygon.
         *
         * Points are queried in order P0, P1,... using a functor callback which must have a signature
         * compatible with:
         *                 bool next_point(fVec2 & P)
         *
         * The callback must store the next point in the reference P and return:
         * - true  if there are more point to plot after this one.
         * - false if this is the last point
         *
         * @param   next_point  callback functor that provides the list of points.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        template<typename FUNCTOR_NEXT>
        void drawSmoothPolygon(FUNCTOR_NEXT next_point, color_t color, float opacity = 1.0f);


        /**
         * Draw a thick smooth (anti-aliased with sub-pixel precision) polygon.
         * 
         * @param   nbpoints    number of points in tabPoints.
         * @param   tabPoints   array of points of the polygon: these points delimit the exterior
         *                      boundary of the polygon and the thickness of the line goes 'inside' the
         *                      polygon.
         * @param   thickness   The thickness of the polygon boundary
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void drawSmoothThickPolygon(int nbpoints, const fVec2 tabPoints[], float thickness, color_t color, float opacity = 1.0f);


        /**
         * Draw a thick smooth (anti-aliased with sub-pixel precision) polygon.
         * 
         * Points are queried in order P0, P1,... using a functor callback which must have a signature
         * compatible with:
         *                 bool next_point(fVec2 & P)
         *
         * The callback must store the next point in the reference P and return:
         * - true  if there are more point to plot after this one.
         * - false if this is the last point
         *
         * @param   next_point  callback functor that provides the list of points delimiting the outer
         *                      boundary of the polygon.
         * @param   thickness   The thickness of the polygon boundary.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        template<typename FUNCTOR_NEXT>
        void drawSmoothThickPolygon(FUNCTOR_NEXT next_point, float thickness, color_t color, float opacity = 1.0f);


        /**
         * Draw a smooth (anti-aliased with sub-pixel precision) filled polygon.
         *
         * WARNING : The polygon must be star-shaped w.r.t the barycenter of its points
         *           (so it is ok for convex polygons...)
         *           
         * @param   nbpoints    number of points in tabPoints.
         * @param   tabPoints   array of points of the polygon.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void fillSmoothPolygon(int nbpoints, const fVec2 tabPoints[], color_t color, float opacity = 1.0f);


        /**
         * Draw a smooth (anti-aliased with sub-pixel precision) filled polygon.
         *        
         * Points are queried in order P0, P1,... using a functor callback which must have a signature
         * compatible with:
         *                 bool next_point(fVec2 & P)
         *
         * The callback must store the next point in the reference P and return:
         * - true  if there are more point to plot after this one.
         * - false if this is the last point AND THEN THE FUNCTOR MUST RESET BACK THE FIRST POINT !
         *
         * WARNING:  In order to draw the polygon correctly, all points must be queried twice so that
         *           after finishing the first iteration (by returniong false), next_point() must reset
         *           to the first point (because and all point are queried a second time) !
         *
         * WARNING : The polygon must be star-shaped w.r.t the barycenter of its points
         *           (so it is ok for convex polygons...)
         *
         * @param   next_point  callback functor that provides the list of points delimiting the polygon.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        template<typename FUNCTOR_NEXT>
        void fillSmoothPolygon(FUNCTOR_NEXT next_point,  color_t color, float opacity = 1.0f);


        /**
         * Draw a smooth (anti-aliased with sub-pixel precision) filled polygon with thick boundary.
         *
         * @param   nbpoints        number of points in tabPoints.
         * @param   tabPoints       array of points of the polygon: these points delimit the exterior
         *                          boundary of the polygon and the thickness of the line goes 'inside'
         *                          the polygon.
         * @param   thickness       The thickness of the polygon boundary (going 'inside').
         * @param   interior_color  color for the interior
         * @param   border_color    color for the thick boundary
         * @param   opacity         (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void fillSmoothThickPolygon(int nbpoints, const fVec2 tabPoints[], float thickness, color_t interior_color, color_t border_color, float opacity = 1.0f);


        /**
         * Draw a smooth (anti-aliased with sub-pixel precision) filled polygon with thick boundary.
         * 
         * Points are queried in order P0, P1,... using a functor callback which must have a signature
         * compatible with:
         *                 bool next_point(fVec2 & P)
         * 
         * The callback must store the next point in the reference P and return:
         * - true  if there are more point to plot after this one.
         * - false if this is the last point AND THEN THE FUNCTOR MUST RESET BACK THE FIRST POINT !
         * 
         * WARNING:  In order to draw the polygon correctly, all points must be queried twice so that
         *           after finishing the first iteration (by returniong false), next_point() must reset 
         *           to the first point (because and all point are queried a second time) !
         * 
         * WARNING : The polygon must be star-shaped w.r.t the barycenter of its points
         *           (so it is ok for convex polygons...)
         *
         * @param   next_point      callback functor that provides the list of points delimiting the polygon outer boundary.
         * @param   thickness       The thickness of the polygon boundary (going 'inside')
         * @param   interior_color  color for the interior
         * @param   border_color    color for the thick boundary
         * @param   opacity         (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        template<typename FUNCTOR_NEXT>
        void fillSmoothThickPolygon(FUNCTOR_NEXT next_point, float thickness, color_t interior_color, color_t border_color, float opacity = 1.0f);










    /********************************************************************************************
    *********************************************************************************************
    *
    *                                     DRAWING CIRCLES
    *
    *********************************************************************************************
    ********************************************************************************************/




    /**********************************************************************
    *                           DRAWING CIRCLES
    *
    *                        'low' quality methods
    ***********************************************************************/


        /**
         * Draw a circle.
         *
         * @param   center  Circle center.
         * @param   r       radius.
         * @param   color   color.
         * @param   opacity (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative to
         *                  disable blending.
        **/
        void drawCircle(iVec2 center, int r, color_t color, float opacity = TGX_DEFAULT_BLENDING_MODE);


        /**
         * Draw a filled circle with possibly distinct colors for outline and interior.
         *
         * @param   center          Circle center.
         * @param   r               radius.
         * @param   interior_color  color for the interior.
         * @param   outline_color   color for the outline.
         * @param   opacity         (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or
         *                          negative to disable blending.
        **/
        void fillCircle(iVec2 center, int r, color_t interior_color, color_t outline_color, float opacity = TGX_DEFAULT_BLENDING_MODE);




    /**********************************************************************
    *                           DRAWING CIRCLES
    *
    *                        'high' quality methods
    ***********************************************************************/



        /**
         * Draw smooth (anti-aliased and with sub-pixel precision) circle
         *
         * @param   center  Circle center.
         * @param   r       radius.
         * @param   color   color
         * @param   opacity (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void drawSmoothCircle(fVec2 center, float r, color_t color, float opacity = 1.0f);


        /**
         * Draw a thick smooth (anti-aliased and with sub-pixel precision) circle
         *
         * @param   center      Circle center.
         * @param   r           external radius (the internal radius is r - thickness).
         * @param   thickness   circle thickness. Should be > 1 otherwise use drawSmoothCircle(() instead.
         * @param   color       color.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void drawSmoothThickCircle(fVec2 center, float r, float thickness, color_t color, float opacity = 1.0f);


        /**
         * Draw a filled smooth (anti-aliased and with sub-pixel precision) circle
         *
         * @param   center  Circle center.
         * @param   r       radius.
         * @param   color   color.
         * @param   opacity (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void fillSmoothCircle(fVec2 center, float r, color_t color, float opacity = 1.0f);


        /**
         * Draw a filled thick smooth (anti-aliased and with sub-pixel precision) circle
         * with different colors for the border and the interior. 
         *
         * @param   center          Circle center.
         * @param   r               external radius (the internal radius is r - thickness).
         * @param   thickness       thickness 'going inside the circle'
         * @param   color_interior  color of the interior
         * @param   color_border    color of the boundary
         * @param   opacity         (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void fillSmoothThickCircle(fVec2 center, float r, float thickness, color_t color_interior, color_t color_border, float opacity = 1.0f);




    /********************************************************************************************
    *********************************************************************************************
    *
    *                                     DRAWING ELLIPSES
    *
    *********************************************************************************************
    ********************************************************************************************/




    /**********************************************************************
    *                           DRAWING ELLIPSES
    *
    *                        'low' quality methods
    ***********************************************************************/



        /**
         * Draw an ellipse.
         *
         * @param   center      Circle center.
         * @param   radiuses    radiuses along the x and y axis.
         * @param   color       color.
         * @param   opacity     (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative
         *                      to disable blending.
        **/
        void drawEllipse(iVec2 center, iVec2 radiuses, color_t color, float opacity = TGX_DEFAULT_BLENDING_MODE);


        /**
         * Draw a filled ellipse with possibly different colors for the outline and the interior.
         *
         * @param   center          Circle center.
         * @param   radiuses        radiuses along the x and y axis.
         * @param   interior_color  color for the interior.
         * @param   outline_color   color for the outline.
         * @param   opacity         (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or
         *                          negative to disable blending.
        **/
        void fillEllipse(iVec2 center, iVec2 radiuses, color_t interior_color, color_t outline_color, float opacity = TGX_DEFAULT_BLENDING_MODE);




    /**********************************************************************
    *                           DRAWING ELLIPSES
    *
    *                        'high' quality methods
    ***********************************************************************/



        /**
         * Draw smooth (anti-aliased and with sub-pixel precision) ellipse.
         *
         * @param   center      Ellipse center.
         * @param   radiuses    radiuses along the x and y axis.
         * @param   color       color.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void drawSmoothEllipse(fVec2 center, fVec2 radiuses, color_t color, float opacity = 1.0f);


        /**
         * Draw a thick smooth (anti-aliased and with sub-pixel precision) ellipse. .
         *
         * @param   center      Ellipse center.
         * @param   radiuses    external radiuses along the x and y axis.
         * @param   thickness   thickness going 'inside' the ellipse
         * @param   color       color.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void drawSmoothThickEllipse(fVec2 center, fVec2 radiuses, float thickness, color_t color, float opacity = 1.0f);


        /**
         * Draw a filled smooth (anti-aliased and with sub-pixel precision) ellipse.
         *
         * @param   center      Ellipse center.
         * @param   radiuses    radiuses along the x and y axis.
         * @param   color       color.
         * @param   opacity     (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void fillSmoothEllipse(fVec2 center, fVec2 radiuses, color_t color, float opacity = 1.0f);


        /**
         * Draw a filled thick smooth (anti-aliased and with sub-pixel precision) ellipse with different
         * colors for the border and the interior.
         *
         * @param   center          ellipse center.
         * @param   radiuses        radiuses along the x and y axis.
         * @param   thickness       border thickness going 'inside' the ellipse
         * @param   color_interior  color of the interior.
         * @param   color_border    color of the boundary.
         * @param   opacity         (Optional) Opacity multiplier in [0.0f, 1.0f].
        **/
        void fillSmoothThickEllipse(fVec2 center, fVec2 radiuses, float thickness, color_t color_interior, color_t color_border, float opacity = 1.0f);





    /********************************************************************************************
    *********************************************************************************************
    *
    *                            DRAWING BEZIER CURVE AND SPLINES
    *
    *********************************************************************************************
    ********************************************************************************************/




        /*****************************************************
        * BEZIER CURVES AND SPLINES
        * LOW QUALITY (FAST) DRAWING METHODS
        ******************************************************/


        /**
         * Draw a quadratic (rational) Bezier curve.
         *
         * @param   P1      Start point.
         * @param   P2      End point.
         * @param   PC      Control point.
         * @param   wc      Control point weight (must be positive). Fastest for wc=1.
         * @param   drawP2  true to draw the endpoint P2.
         * @param   color   The color to use.
         * @param   opacity Opacity multiplier when blending (in [0.0f, 1.0f]) or negative to disable
         *                  blending and simply use overwrite.
        **/
        void drawQuadBezier(iVec2 P1, iVec2 P2, iVec2 PC, float wc, bool drawP2, color_t color, float opacity = TGX_DEFAULT_BLENDING_MODE);


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
        void drawCubicBezier(iVec2 P1, iVec2 P2, iVec2 PA, iVec2 PB, bool drawP2, color_t color, float opacity = TGX_DEFAULT_BLENDING_MODE);


        /**
         * Draw a quadratic spline interpolating between a given set of points.
         * 
         * The template parameter SPLINE_MAX_POINTS defines the maximum number of points that a spline
         * can have. Can be increased if needed but this will increase the memory used on the stack for
         * allocating the temporary point arrays...
         *
         * @param   nbpoints        number of points in the spline Must be smaller or equal to
         *                          SPLINE_MAX_POINTS.
         * @param   tabPoints       the array of points to interpolate.
         * @param   draw_last_point true to draw the last point.
         * @param   color           The color to use.
         * @param   opacity         (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or
         *                          negative to disable blending and simply use overwrite.
        **/
        template<int SPLINE_MAX_POINTS = 32>
        void drawQuadSpline(int nbpoints, const iVec2 tabPoints[], bool draw_last_point, color_t color, float opacity = TGX_DEFAULT_BLENDING_MODE);


        /**
         * Draw a cubic spline interpolating between a given set of points. 
         * 
         * The template parameter SPLINE_MAX_POINTS defines the maximum number of points that a spline
         * can have. Can be increased if needed but this will increase the memory used on the stack for
         * allocating the temporary point arrays...
         *
         * @tparam  SPLINE_MAX_POINTS   Type of the spline maximum points.
         * @param   nbpoints        number of points in the spline Must be smaller or equal to
         *                          SPLINE_MAX_POINTS.
         * @param   tabPoints       the array of points to interpolate.
         * @param   draw_last_point true to draw the last point.
         * @param   color           The color to use.
         * @param   opacity         (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or
         *                          negative to disable blending and simply use overwrite.
        **/
        template<int SPLINE_MAX_POINTS = 32>
        void drawCubicSpline(int nbpoints, const iVec2 tabPoints[], bool draw_last_point, color_t color, float opacity = TGX_DEFAULT_BLENDING_MODE);


        /**
         * Draw a closed quadratic spline interpolating between a given set of points.
         * 
         * The template parameter SPLINE_MAX_POINTS defines the maximum number of points that a spline
         * can have. Can be increased if needed but this will increase the memory used on the stack for
         * allocating the temporary point arrays...
         *
         * @param   nbpoints    number of points in the spline Must be smaller or equal to
         *                      SPLINE_MAX_POINTS.
         * @param   tabPoints   the array of points to interpolate.
         * @param   color       The color to use.
         * @param   opacity     (Optional) Opacity multiplier when blending (in [0.0f, 1.0f]) or negative
         *                      to disable blending and simply use overwrite.
        **/
        template<int SPLINE_MAX_POINTS = 32>
        void drawClosedSpline(int nbpoints, const iVec2 tabPoints[], color_t color, float opacity = TGX_DEFAULT_BLENDING_MODE);





        /*****************************************************
        * BEZIER CURVES AND SPLINES
        * HIGH QUALITY (VERY SLOW) DRAWING METHODS
        ******************************************************/







    /********************************************************************************************
    *********************************************************************************************
    *
    *                                       DRAWING TEXT
    *
    * supported font format:
    * 
    *     - AdafruitGFX            https ://glenviewsoftware.com/projects/products/adafonteditor/adafruit-gfx-font-format/
    *     - ILI9341_t3 v1          https://forum.pjrc.com/threads/54316-ILI9341_t-font-structure-format
    *       and v23 (antialiased). https://github.com/projectitis/packedbdf/blob/master/packedbdf.md
    *       
    *********************************************************************************************
    ********************************************************************************************/





        /**
        * Return the box of pixels occupied by a char 'c' when drawn with 'font' at position 'pos'
        * position is w.r.t. the baseline
        * if xadvance is non null, put in there the offset for printing the next char on the same line
        **/
        static iBox2 measureChar(char c, iVec2 pos, const GFXfont& font, int* xadvance = nullptr);


        /**
        * Return the box of pixels occupied by a char 'c' when drawn with 'font' at position 'pos'
        * position is w.r.t. the baseline
        * if xadvance is non null, put in there the offset for printing the next char on the same line
        **/
        static iBox2 measureChar(char c, iVec2 pos, const ILI9341_t3_font_t& font, int* xadvance = nullptr);


        /**
        * Return the box of pixels occupied by a text when drawn with 'font' starting at position 'pos' (w.r.t. the baseline). 
        * If start_newline_at_0 is true, cursor restart at x=0 at newline. Otherwise, it restart at pos.x
        **/
        static iBox2 measureText(const char * text, iVec2 pos, const GFXfont& font, bool start_newline_at_0 = false);


        /**
        * Return the box of pixels occupied by a text when drawn with 'font' starting at position 'pos' (w.r.t. the baseline).
        * If start_newline_at_0 is true, cursor restart at x=0 at newline. Otherwise, it restart at pos.x
        **/
        static iBox2 measureText(const char * text, iVec2 pos, const ILI9341_t3_font_t& font, bool start_newline_at_0 = false);


        /**
        * Draw a character from an Adafruit font at position pos on the image. 
        * Return the position for the next character (on the same line). 
        * position is w.r.t. the baseline
        **/
        iVec2 drawChar(char c, iVec2 pos, color_t col, const GFXfont& font)
            {
            return _drawCharGFX<false>(c, pos, col, font, 1.0f);
            }


        /**
        * Draw a character from an Adafruit font at position pos on the image.
        * Return the position for the next character (on the same line).
        * position is w.r.t. the baseline
        * 
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        iVec2 drawChar(char c, iVec2 pos, color_t col, const GFXfont& font, float opacity)
            {
            return _drawCharGFX<true>(c, pos, col, font, opacity);
            }


        /**
        * Draw a character from an ili9341_t3 font at position (x,y) on the image.
        * Return the position for the next character (on the same line).
        * position is w.r.t. the baseline
        **/
        iVec2 drawChar(char c, iVec2 pos, color_t col, const ILI9341_t3_font_t& font)
            {
            return _drawCharILI<false>(c, pos, col, font, 1.0f);
            }


        /**
        * Draw a character from an ili9341_t3 font at position (x,y) on the image.
        * Return the position for the next character (on the same line).
        * position is w.r.t. the baseline
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        iVec2 drawChar(char c, iVec2 pos, color_t col, const ILI9341_t3_font_t& font, float opacity)
            {
            return _drawCharILI<true>(c, pos, col, font, opacity);
            }


        /**
        * Draw a  text  starting at position (x,y) on the image using an adafruit font
        * Return the position after the last character (ie the position for the next char).
        * All position are w.r.t. the baseline
        **/
        iVec2 drawText(const char* text, iVec2 pos, color_t col, const GFXfont& font, bool start_newline_at_0)
            {
            return _drawTextGFX<false>(text, pos, col, font, start_newline_at_0, 1.0f);
            }


        /**
        * Draw a  text  starting at position (x,y) on the image using an adafruit font
        * Return the position after the last character (ie the position for the next char).
        * All position are w.r.t. the baseline
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        iVec2 drawText(const char* text, iVec2 pos, color_t col, const GFXfont& font, bool start_newline_at_0, float opacity)
            {
            return _drawTextGFX<true>(text, pos, col, font, start_newline_at_0, opacity);
            }


        /**
        * Draw a  text  starting at position (x,y) on the image using an ILI9341_t3 font
        * Return the position after the last character (ie the position for the next char).
        * All position are w.r.t. the baseline
        **/
        iVec2 drawText(const char * text, iVec2 pos, color_t col, const ILI9341_t3_font_t& font, bool start_newline_at_0)
            {
            return _drawTextILI<false>(text, pos, col, font, start_newline_at_0, 1.0f);
            }


        /**
        * Draw a  text  starting at position (x,y) on the image using an ILI9341_t3 font
        * Return the position after the last character (ie the position for the next char).
        * All position are w.r.t. the baseline
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        iVec2 drawText(const char* text, iVec2 pos, color_t col, const ILI9341_t3_font_t& font, bool start_newline_at_0, float opacity)
            {
            return _drawTextILI<true>(text, pos, col, font, start_newline_at_0, opacity);
            }












private:





    /************************************************************************************
    * 
    * 
    * Don't you dare look below... this is private :-p
    * 
    * 
    *************************************************************************************/




        /***************************************
        * GENERAL / COPY / RESIZE / BLIT
        ****************************************/


        /** Make sure the image parameters are ok, else mark the image as invalid. */
        inline void _checkvalid()
            {
            if ((_lx <= 0) || (_ly <= 0) || (_stride < _lx) || (_buffer == nullptr)) setInvalid();
            }


        /** set len consecutive pixels given color starting at pdest */
        static inline void _fast_memset(color_t* p_dest, color_t color, int32_t len);


        bool _blitClip(const Image& sprite, int& dest_x, int& dest_y, int& sprite_x, int& sprite_y, int& sx, int& sy);



        void _blit(const Image& sprite, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy);

        static void _blitRegion(color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy)
            {
            if ((size_t)pdest <= (size_t)psrc) 
                _blitRegionUp(pdest, dest_stride, psrc, src_stride, sx, sy);
            else 
                _blitRegionDown(pdest, dest_stride, psrc, src_stride, sx, sy);
            }

        static void _blitRegionUp(color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy);

        static void _blitRegionDown(color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy);




        void _blit(const Image& sprite, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy, float opacity);

        static void _blitRegion(color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy, float opacity)
            {
            if ((size_t)pdest <= (size_t)psrc)
                _blitRegionUp(pdest, dest_stride, psrc, src_stride, sx, sy, opacity);
            else
                _blitRegionDown(pdest, dest_stride, psrc, src_stride, sx, sy, opacity);
            }

        static void _blitRegionUp(color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy, float opacity);

        static void _blitRegionDown(color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy, float opacity);




        template<typename color_t_src, typename BLEND_OPERATOR>
        void _blit(const Image<color_t_src>& sprite, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy, const  BLEND_OPERATOR& blend_op);

        template<typename color_t_src, typename BLEND_OPERATOR>
        static void _blitRegion(color_t* pdest, int dest_stride, color_t_src * psrc, int src_stride, int sx, int sy, const  BLEND_OPERATOR& blend_op)
            {
            if ((size_t)pdest <= (size_t)psrc)
                _blitRegionUp(pdest, dest_stride, psrc, src_stride, sx, sy, blend_op);
            else
                _blitRegionDown(pdest, dest_stride, psrc, src_stride, sx, sy, blend_op);
            }

        template<typename color_t_src, typename BLEND_OPERATOR>
        static void _blitRegionUp(color_t* pdest, int dest_stride, color_t_src * psrc, int src_stride, int sx, int sy, const  BLEND_OPERATOR& blend_op);

        template<typename color_t_src, typename BLEND_OPERATOR>
        static void _blitRegionDown(color_t* pdest, int dest_stride, color_t_src* psrc, int src_stride, int sx, int sy, const  BLEND_OPERATOR& blend_op);




        void _blitMasked(const Image& sprite, color_t transparent_color, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy, float opacity);

        static void _maskRegion(color_t transparent_color, color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy, float opacity)
            {
            if ((size_t)pdest <= (size_t)psrc)
                _maskRegionUp(transparent_color, pdest, dest_stride, psrc, src_stride, sx, sy, opacity);
            else
                _maskRegionDown(transparent_color, pdest, dest_stride, psrc, src_stride, sx, sy, opacity);
            }

        static void _maskRegionUp(color_t transparent_color, color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy, float opacity);

        static void _maskRegionDown(color_t transparent_color, color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy, float opacity);




        template<typename color_t_src, int CACHE_SIZE, bool USE_BLENDING, bool USE_MASK, bool USE_CUSTOM_OPERATOR, typename BLEND_OPERATOR>
        void _blitScaledRotated(const Image<color_t_src>& src_im, color_t_src transparent_color, fVec2 anchor_src, fVec2 anchor_dst, float scale, float angle_degrees, float opacity, const BLEND_OPERATOR& blend_op);




        /***************************************
        * FLOOD FILLING
        ****************************************/


        template<bool UNICOLOR_COMP, int STACK_SIZE_BYTES> int _scanfill(int x, int y, color_t border_color, color_t new_color);



        /***************************************
        * BRESENHAM
        ****************************************/
     

        /** update a pixel on a bresenham segment */
        template<bool X_MAJOR, bool BLEND, int SIDE, bool CHECKRANGE = false> inline TGX_INLINE void _bseg_update_pixel(const BSeg & seg, color_t color, int32_t op)
            {
            const int x = seg.X();
            const int y = seg.Y();
            if (CHECKRANGE)
                {
                if ((x < 0) || (y < 0) || (x >= _lx) || (y >= _ly)) return;
                }
            if (SIDE != 0)
                {
                const int32_t o = ((seg.AA<SIDE, X_MAJOR>()) * op) >> 8;
                _buffer[TGX_CAST32(x) + TGX_CAST32(_stride) * TGX_CAST32(y)].blend256(color, (uint32_t)o);
                }
            else if (BLEND)
                {
                _buffer[TGX_CAST32(x) + TGX_CAST32(_stride) * TGX_CAST32(y)].blend256(color, (uint32_t)op);
                }
            else
                {
                _buffer[TGX_CAST32(x) + TGX_CAST32(_stride) * TGX_CAST32(y)] = color;
                }
            }


        template<bool CHECKRANGE = false> inline TGX_INLINE void _bseg_update_pixel(const BSeg & seg, color_t color, int32_t op, int32_t aa)
            {
            const int x = seg.X();
            const int y = seg.Y();
            if (CHECKRANGE)
                {
                if ((x < 0) || (y < 0) || (x >= _lx) || (y >= _ly)) return;
                }
            _buffer[TGX_CAST32(x) + TGX_CAST32(_stride) * TGX_CAST32(y)].blend256(color, (uint32_t)((op * aa)>>8));
            }





        template<int SIDE> void _bseg_draw_template(BSeg& seg, bool draw_first, bool draw_last, color_t color, int32_t op, bool checkrange);


        /**
         * Draw a Bresenham segment [P,Q|.
        **/
        void _bseg_draw(BSeg & seg, bool draw_first, bool draw_last, color_t color, int side, int32_t op, bool checkrange);


        /**
        * Draw an antialiased Bresenham segment [P,Q|.
        **/
        void _bseg_draw_AA(BSeg & seg, bool draw_first, bool draw_last, color_t color, int32_t op, bool checkrange);


     
        /** used by _bseg_avoid1 */
        template<int SIDE> void _bseg_avoid1_template(BSeg& segA, bool lastA, BSeg& segB, bool lastB, color_t color, int32_t op, bool checkrange);


        /**
         * Draw the bresenham segment [P,Q| while avoiding [P,A|
         * if drawP is set and AA on on , the pixel at P is draw using the minimum AA value.
         * 
         *             A
         *            /
         *           /
         *          /
         *        P+-------------Q
        **/
        void _bseg_avoid1(BSeg& PQ, BSeg& PA, bool drawP, bool drawQ, bool closedPA, color_t color, int side, int32_t op, bool checkrange);


         

        /** Used by _bseg_avoid2 */
        template<int SIDE> void _bseg_avoid2_template(BSeg& segA, bool lastA, BSeg& segB, bool lastB, BSeg& segC, bool lastC, color_t color, int32_t op, bool checkrange);


        /**
        * Draw the bresenham segment [P,Q| while avoiding [P,A| and [P,B|
        * 
        *     A       B
        *      \     /
        *       \   /
        *        \ /
        *         +--------------
        *         P             Q
        **/
        void _bseg_avoid2(BSeg & PQ, BSeg & PA, BSeg & PB, bool drawQ, bool closedPA, bool closedPB, color_t color, int side, int32_t op, bool checkrange);


        /** Used by _bseg_avoid11 */
        template<int SIDE> void _bseg_avoid11_template(BSeg& segA, BSeg& segB, bool lastB, BSeg& segD, bool lastD, color_t color, int32_t op, bool checkrange);



        /**
        * Draw the bresenham segment [P,Q| while avoiding [P,A| and [Q,B|
        * if drawP is set and AA on on , the pixel at P is draw using the minimum AA value.
        * if drawQ is set and AA on on , the pixel at Q is draw using the minimum AA value.
        *
        *     A                     B
        *      \                   /
        *       \                 /
        *        \               /
        *         +--------------
        *         P             Q

        **/
        void _bseg_avoid11(BSeg & PQ, BSeg& PA, BSeg & QB, bool drawP, bool drawQ, bool closedPA, bool closedQB, color_t color, int side, int32_t op, bool checkrange);



        /** Used by _bseg_avoid21 */
        template<int SIDE> void _bseg_avoid21_template(BSeg& segA, BSeg& segB, bool lastB, BSeg& segC, bool lastC, BSeg& segD, bool lastD, color_t color, int32_t op, bool checkrange);


        /**
        * Draw the bresenham segment [P,Q| while avoiding [P,A| , [P, B| and [Q,C|
        *
        *      A      B             C
        *      \     /             /
        *       \   /             /
        *        \ /             /
        *         +--------------
        *         P             Q
        **/
        void _bseg_avoid21(BSeg& PQ, BSeg& PA, BSeg& PB, BSeg& QC, bool closedPA, bool closedPB, bool closedQC, color_t color, int side, int32_t op, bool checkrange);



        /** Used by _bseg_avoid22 */
        template<int SIDE> void _bseg_avoid22_template(BSeg& segA, BSeg& segB, bool lastB, BSeg& segC, bool lastC, BSeg& segD, bool lastD, BSeg& segE, bool lastE, color_t color, int32_t op, bool checkrange);


        /**
        * Draw the bresenham segment [P,Q| while avoiding [P,A| , [P, B|,  [Q,C| and [Q,D|
        *
        *     A       B         C       D
        *      \     /           \     /
        *       \   /             \   /
        *        \ /               \ /
        *         +-----------------+
        *         P                 Q
        **/
        void _bseg_avoid22(BSeg& PQ, BSeg& PA, BSeg& PB, BSeg& QC, BSeg& QD, bool closedPA, bool closedPB, bool closedQC, bool closedQD, color_t color, int side, int32_t op, bool checkrange);



        /**
        * Fill the interior of a triangle. 
        * integer valued version
        **/
        //void _bseg_fill_triangle(iVec2 P1, iVec2 P2, iVec2 P3, color_t fillcolor, float opacity);


        /**
        * Fill the interior of a triangle.
        * floating point version with sub pixel precision
        **/
        void _bseg_fill_triangle(fVec2 fP1, fVec2 fP2, fVec2 fP3, color_t fillcolor, float opacity);


        /**
        * Fill the interior of a triangle.
        * floating point version with sub pixel precision and BSeg already defined
        **/
        void _bseg_fill_triangle_precomputed(fVec2 fP1, fVec2 fP2, fVec2 fP3, BSeg & seg12, BSeg & seg21, BSeg & seg23, BSeg & seg32, BSeg & seg31, BSeg & seg13, color_t fillcolor, float opacity);

        void _bseg_fill_triangle_precomputed_sub(fVec2 fP1, fVec2 fP2, fVec2 fP3, BSeg & seg12, BSeg & seg21, BSeg & seg23, BSeg & seg32, BSeg & seg31, BSeg & seg13, color_t fillcolor, float opacity);


        void _bseg_fill_interior_angle(iVec2 P, iVec2 Q1, iVec2 Q2, BSeg& seg1, BSeg& seg2, color_t color, bool fill_last, float opacity);


        /** like drawFasthLine but used by _bseg_fill_interior_angle_sub */
        void _triangle_hline(int x1, int x2, const int y, color_t color, float opacity)
            {
            x1 = tgx::max(0, x1);
            x2 = tgx::min(_lx - 1, x2);
            color_t* p = _buffer + TGX_CAST32(x1) + TGX_CAST32(y) * TGX_CAST32(_stride);
            if (opacity < 0)
                {
                while (x1++ <= x2) { (*p++) = color; }
                }
            else
                {
                while (x1++ <= x2) { (*p++).blend(color,opacity); }
                }
            }


        void _bseg_fill_interior_angle_sub(int dir, int y, int ytarget, BSeg& sega, BSeg& segb, color_t color, float opacity);


        
        template<typename T>
        inline T _triangleAera(Vec2<T> P1, Vec2<T> P2, Vec2<T> P3)
            {
            return P1.x * (P2.y - P3.y) + P2.x * (P3.y - P1.y) + P3.x * (P1.y - P2.y);
            }


        /*
        int32_t _bseg_intersect_AA(const BSeg& segPA, int side_PA, const BSeg& segPB)
            {   
            const fVec2 VA = segPA.unitVec(); 
            const fVec2 VB = segPB.unitVec();
            const float w = (((-VB.x) * (VB.y)) + ((VA.x) * (VA.y)) + ((VB.x - VA.x) * (VB.y + VA.y))) * side_PA;
            const float alpha = tgx::dotProduct(VA, VB);
            const int32_t aaA = segPA.AA(side_PA);
            const int32_t aaB = segPB.AA(-side_PA);
            const int32_t minAA = tgx::min(aaA, aaB);
            const int32_t maxAA = tgx::max(aaA, aaB);
            int32_t a;
            if (w >= 0)
                { // obtuse angle
                a = minAA * ((1 - alpha) / 2);
                }
            else
                {
                a = (int32_t)(maxAA + (minAA * ((1 + alpha) / 2)));
                }
            return ((a < 0) ? 0 : ((a > 256) ? 256 : a));
            }
        */






        /***************************************
        * DRAWING LINES
        ****************************************/


        template<bool CHECKRANGE, bool CHECK_VALID_BLEND = true> TGX_INLINE inline void _drawPixel(iVec2 pos, color_t color, float opacity)
            {
            if (CHECKRANGE)
                {
                if ((pos.x < 0) || (pos.y < 0) || (pos.x >= _lx) || (pos.y >= _ly)) return;
                }
            if ((CHECK_VALID_BLEND) && ((opacity < 0)||(opacity>1)))
                _buffer[TGX_CAST32(pos.x) + TGX_CAST32(_stride) * TGX_CAST32(pos.y)] = color;
            else
                _buffer[TGX_CAST32(pos.x) + TGX_CAST32(_stride) * TGX_CAST32(pos.y)].blend(color, opacity);
            }


        template<bool CHECKRANGE> TGX_INLINE inline void _drawPixel(iVec2 pos, color_t color)
            {
            if (CHECKRANGE)
                {
                if ((pos.x < 0) || (pos.y < 0) || (pos.x >= _lx) || (pos.y >= _ly)) return;
                }
            _buffer[TGX_CAST32(pos.x) + TGX_CAST32(_stride) * TGX_CAST32(pos.y)] = color;
            }


        TGX_INLINE inline void _drawPixel(bool checkrange, iVec2 pos, color_t color, float opacity)
            {
            if (checkrange)
                {
                if ((pos.x < 0) || (pos.y < 0) || (pos.x >= _lx) || (pos.y >= _ly)) return;
                }
            if ((opacity < 0)||(opacity>1))
                _buffer[TGX_CAST32(pos.x) + TGX_CAST32(_stride) * TGX_CAST32(pos.y)] = color;
            else
                _buffer[TGX_CAST32(pos.x) + TGX_CAST32(_stride) * TGX_CAST32(pos.y)].blend(color, opacity);
            }


        TGX_INLINE inline void _drawPixel(bool checkrange, iVec2 pos, color_t color)
            {
            if (checkrange)
                {
                if ((pos.x < 0) || (pos.y < 0) || (pos.x >= _lx) || (pos.y >= _ly)) return;
                }
            _buffer[TGX_CAST32(pos.x) + TGX_CAST32(_stride) * TGX_CAST32(pos.y)] = color;
            }



        template<bool CHECKRANGE = true> TGX_INLINE inline color_t _readPixel(iVec2 pos, color_t outside_color) const
            {
            if (CHECKRANGE) // optimized away at compile time
                {
                if ((pos.x < 0) || (pos.y < 0) || (pos.x >= _lx) || (pos.y >= _ly)) return outside_color;
                }
            return _buffer[TGX_CAST32(pos.x) + TGX_CAST32(_stride) * TGX_CAST32(pos.y)];
            }


        template<bool CHECKRANGE> void _drawFastVLine(iVec2 pos, int h, color_t color, float opacity);

        template<bool CHECKRANGE> void _drawFastHLine(iVec2 pos, int w, color_t color, float opacity);

        template<bool CHECKRANGE> void _drawFastVLine(iVec2 pos, int h, color_t color);

        template<bool CHECKRANGE> void _drawFastHLine(iVec2 pos, int w, color_t color);

        void _drawFastVLine(bool checkrange, iVec2 pos, int h, color_t color, float opacity) { if (checkrange) _drawFastVLine<true>(pos, h, color, opacity); else _drawFastVLine<false>(pos, h, color, opacity); }

        void _drawFastHLine(bool checkrange, iVec2 pos, int w, color_t color, float opacity) { if (checkrange) _drawFastHLine<true>(pos, w, color, opacity); else _drawFastHLine<false>(pos, w, color, opacity); }

        void _drawFastVLine(bool checkrange, iVec2 pos, int h, color_t color) { if (checkrange) _drawFastVLine<true>(pos, h, color); else _drawFastVLine<false>(pos, h, color); }

        void _drawFastHLine(bool checkrange, iVec2 pos, int w, color_t color) { if (checkrange) _drawFastHLine<true>(pos, w, color); else _drawFastHLine<false>(pos, w, color); }


        void _drawSeg(iVec2 P1, bool drawP1, iVec2 P2, bool drawP2, color_t color, float opacity) { _bseg_draw(BSeg(P1, P2), drawP1, drawP2, color, 0, (int32_t)(opacity * 256), true); }




        /** adapted from bodmer e_tft library */
        void _drawWedgeLine(float ax, float ay, float bx, float by, float aw, float bw, color_t color, float opacity);


        /** taken from bodmer e_tft library **/
        inline TGX_INLINE float _wedgeLineDistance(float pax, float pay, float bax, float bay, float dr)
            {
            float h = fmaxf(fminf((pax * bax + pay * bay) / (bax * bax + bay * bay), 1.0f), 0.0f);
            float dx = pax - bax * h, dy = pay - bay * h;
            return tgx::fast_sqrt(dx * dx + dy * dy) + h * dr;
            }





        /***************************************
        * RECT AND ROUNDED RECT
        ****************************************/

        void _fillRect(iBox2 B, color_t color, float opacity);


        template<bool CHECKRANGE>
        void _drawRoundRect(int x, int y, int w, int h, int r, color_t color);

        template<bool CHECKRANGE>
        void _drawRoundRect(int x, int y, int w, int h, int r, color_t color, float opacity);

        
        template<bool CHECKRANGE>
        void _fillRoundRect(int x, int y, int w, int h, int r, color_t color);

        
        template<bool CHECKRANGE>
        void _fillRoundRect(int x, int y, int w, int h, int r, color_t color, float opacity);


        void _fillSmoothRect(const fBox2& B, color_t color, float opacity);


        void _fillSmoothRoundedRect(const tgx::iBox2& B, float corner_radius, color_t color, float opacity);


        void _drawSmoothRoundRect(const tgx::iBox2& B, float corner_radius, color_t color, float opacity);


        void _drawSmoothWideRoundRect(const tgx::iBox2& B, float corner_radius, float thickness, color_t color, float opacity);





        /***************************************
        * TRIANGLES
        ****************************************/


        void _drawTriangle(const iVec2& P1, const iVec2& P2, const iVec2& P3, color_t color, float opacity);


        void _fillTriangle(const iVec2& P1, const iVec2& P2, const iVec2& P3, color_t interior_color, color_t outline_color, float opacity);


        void _drawSmoothTriangle(fVec2 P1, fVec2 P2, fVec2 P3, color_t color, float opacity);


        void _fillSmoothTriangle(fVec2 P1, fVec2 P2, fVec2 P3, color_t color, float opacity);


        /** 
         * Methods using the 3D triangle rasterizer 
         **/

        /** Convert to texture coordinates */
        inline TGX_INLINE tgx::fVec2 _coord_texture(tgx::fVec2 pos, tgx::iVec2 size) { return tgx::fVec2(pos.x / ((float)size.x), pos.y / ((float)size.y)); }

        /** Convert to viewport coordinates */
        inline TGX_INLINE tgx::fVec2 _coord_viewport(tgx::fVec2 pos, tgx::iVec2 size) { return tgx::fVec2((2.0f / ((float)size.x)) * (pos.x) - 1.0f, (2.0f / ((float)size.y)) * (pos.y) - 1.0f); }

        template<typename color_alt, bool USE_BLENDING>
        void _drawGradientTriangle(fVec2 P1, fVec2 P2, fVec2 P3, color_alt colorP1, color_alt colorP2, color_alt colorP3, float opacity);

        template<typename color_t_tex, bool GRADIENT, bool USE_BLENDING, bool MASKED>
        void _drawTexturedTriangle(const Image<color_t_tex>& src_im, color_t_tex transparent_color, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, color_t_tex C1, color_t_tex C2, color_t_tex C3, float opacity);



        /***************************************
        * QUADS
        ****************************************/


        void _fillSmoothQuad(fVec2 P1, fVec2 P2, fVec2 P3, fVec2 P4, color_t color, float opacity);




        /***************************************
        * POLYGONS AND POLYLINES
        ****************************************/



        /***************************************
        * CIRCLES AND ELLIPSE
        ****************************************/


        /** taken from Adafruit GFX library */
        template<bool CHECKRANGE>
        void _drawCircleHelper(int x0, int y0, int r, int cornername, color_t color, float opacity);



        /** taken from Adafruit GFX library */
        template<bool CHECKRANGE>
        void _fillCircleHelper(int x0, int y0, int r, int corners, int delta, color_t color, float opacity);



        /** filled circle drawing method */
        template<bool OUTLINE, bool FILL, bool CHECKRANGE>
        void _drawFilledCircle(int xm, int ym, int r, color_t color, color_t fillcolor, float opacity);



        /** adapted from bodmer e_tft library */
        template<bool OUTLINE, bool FILL, bool CHECKRANGE>
        void _drawEllipse(int x0, int y0, int rx, int ry, color_t outline_color, color_t interior_color, float opacity);



        void _fillSmoothQuarterCircle(tgx::fVec2 C, float R, int quarter, bool vertical_center_line, bool horizontal_center_line, color_t color, float opacity);


        void _fillSmoothCircle(tgx::fVec2 C, float R, color_t color, float opacity);


        void _smoothQuarterCircle(tgx::fVec2 C, float R, int quarter, bool vertical_center_line, bool horizontal_center_line, color_t color, float opacity);

        
        void _smoothCircle(tgx::fVec2 C, float R, color_t color, float opacity);

        
        void _smoothWideQuarterCircle(tgx::fVec2 C, float R, float thickness, int quarter, bool vertical_center_line, bool horizontal_center_line, color_t color, float opacity);


        void _smoothWideCircle(tgx::fVec2 C, float R, float thickness, color_t color, float opacity);


        void _fillSmoothQuarterEllipse(tgx::fVec2 C, float rx, float ry, int quarter, bool vertical_center_line, bool horizontal_center_line, color_t color, float opacity);


        void _fillSmoothEllipse(tgx::fVec2 C, float rx, float ry, color_t color, float opacity);;


        void _drawSmoothQuarterEllipse(tgx::fVec2 C, float rx, float ry, int quarter, bool vertical_center_line, bool horizontal_center_line, color_t color, float opacity);
 

        void _drawSmoothEllipse(tgx::fVec2 C, float rx, float ry, color_t color, float opacity);
        

        void _drawSmoothThickQuarterEllipse(tgx::fVec2 C, float rx, float ry, float thickness, int quarter, bool vertical_center_line, bool horizontal_center_line, color_t color, float opacity);
        

        void _drawSmoothThickEllipse(tgx::fVec2 C, float rx, float ry, float thickness, color_t color, float opacity);
        

        /** Fill a quarter circle but only on the pixel delimited by the half plane x*kx + y*ky + off > 0 */        
        void _fillSmoothQuarterCircleInterHP(tgx::fVec2 C, float R, int quarter, bool vertical_center_line, bool horizontal_center_line, color_t color, float opacity, int32_t kx, int32_t ky, int32_t off);

        /** Fill a quarter circle but only on the pixel delimited by the half plane x*kx + y*ky + off > 0 [SLOW: TODO improve this....] */
        void _fillSmoothCircleInterHP(tgx::fVec2 C, float R, color_t color, float opacity, int32_t kx, int32_t ky, int32_t off);

        void _fillSmoothCircleInterHP(tgx::fVec2 C, float R, color_t color, float opacity, const BSeg & seg, int side);


        /***************************************
        * DRAWING BEZIER
        ****************************************/


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




  



        /***************************************
        * DRAWING TEXT
        ****************************************/


        /** fetch a single bit from a bit array. (from ili9341_t3.cpp) */
        static inline uint32_t _fetchbit(const uint8_t* p, uint32_t index) { return (p[index >> 3] & (0x80 >> (index & 7))); }


        /** fetch 'required' bits from a bit array, returned as an unsigned integer  (from ili9341_t3.cpp)*/
        static uint32_t _fetchbits_unsigned(const uint8_t* p, uint32_t index, uint32_t required);


        /** fetch 'required' bits from a bit array, returned as a signed integer (from ili9341_t3.cpp) */
        static uint32_t _fetchbits_signed(const uint8_t* p, uint32_t index, uint32_t required);


        /** used for clipping a font bitmap */
        bool _clipit(int& x, int& y, int& sx, int& sy, int& b_left, int& b_up);


        template<bool BLEND>
        iVec2 _drawTextGFX(const char* text, iVec2 pos, color_t col, const GFXfont& font, bool start_newline_at_0, float opacity);


        template<bool BLEND>
        iVec2 _drawTextILI(const char* text, iVec2 pos, color_t col, const ILI9341_t3_font_t& font, bool start_newline_at_0, float opacity);


        template<bool BLEND>
        iVec2 _drawCharGFX(char c, iVec2 pos, color_t col, const GFXfont& font, float opacity);


        template<bool BLEND>
        iVec2 _drawCharILI(char c, iVec2 pos, color_t col, const ILI9341_t3_font_t& font, float opacity);


        /**
         * draw a character from an ILI9341_t3 font bitmap (version 1, with line compression).
         * The ILI9341_t3 font format is described here: https://forum.pjrc.com/threads/54316-ILI9341_t-font-structure-format
         **/
        template<bool BLEND>
        void _drawCharILI9341_t3(const uint8_t* bitmap, int32_t off, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col, float opacity);


        /** used by _drawCharILI9341_t3 to draw a single row of a font bitmap */
        template<bool BLEND>
        static void _drawcharline(const uint8_t* bitmap, int32_t off, color_t* p, int dx, color_t col, float opacity);


        /** 
         * draw a 1 bit per pixel char bitmap on the image 
         * Use to draw char from Adafruit font format: https://glenviewsoftware.com/projects/products/adafonteditor/adafruit-tgx-font-format/
         **/
        template<bool BLEND>
        void _drawCharBitmap_1BPP(const uint8_t* bitmap, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col, float opacity);


        /** draw a 2 bit per pixel char bitmap on the image 
        *  packed bdf format 23 : https://github.com/projectitis/packedbdf/blob/master/packedbdf.md
        **/
        template<bool BLEND>
        void _drawCharBitmap_2BPP(const uint8_t* bitmap, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col, float opacity);


        /** draw a 4 bit per pixel char bitmap on the image */
        template<bool BLEND>
        void _drawCharBitmap_4BPP(const uint8_t* bitmap, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col, float opacity);


        /** draw a 8 bit per pixel char bitmap on the image */
        template<bool BLEND>
        void _drawCharBitmap_8BPP(const uint8_t* bitmap, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col, float opacity);







    };





}


#include "Image.inl"


#endif

#endif

/** end of file */


