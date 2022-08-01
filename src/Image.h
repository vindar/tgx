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






    /************************************************************************************
    * Image members variables (16 bytes). 
    *************************************************************************************/

        color_t* _buffer;       // pointer to the pixel buffer  - nullptr if the image is invalid.
        int     _lx, _ly;       // image size  - (0,0) if the image is invalid
        int     _stride;        // image stride - 0 if the image is invalid


    public:


        #if (MTOOLS_TGX_EXTENSIONS)
        #include <mtools/extensions/tgx/tgx_ext_Image.inl> 
        #endif


    /************************************************************************************
    * 
    *  Creation of images and sub-images 
    * 
    * The memory buffer must be supplied at creation. Otherwise, the image is set as 
    * invalid until a valid buffer is supplied. 
    * 
    *************************************************************************************/


        /** 
        * Create default invalid/empty image. 
        */
        Image() : _buffer(nullptr), _lx(0), _ly(0), _stride(0)
            {
            }


        /**
         * Constructor. Creates an image with a given size and a given buffer.
         *
         * @param [in,out]  buffer  the image buffer
         * @param           lx      the image width.
         * @param           ly      the image height.
         * @param           stride  the stride to use (equal to the image width if not specified).
        **/
        template<typename T> Image(T * buffer, int lx, int ly, int stride = -1) : _buffer((color_t*)buffer), _lx(lx), _ly(ly), _stride(stride < 0 ? lx : stride)
            {
            _checkvalid(); // make sure dimension/stride are ok else make the image invalid
            }


        /**
         * Constructor. Creates an image with a given size and a given buffer.
         *
         * @param [in,out]  buffer  the image buffer
         * @param           dim     the image dimension (width, height) in pixels.
         * @param           stride  the stride to use (equal to the image width if not specified).
        **/
        template<typename T> Image(T * buffer, iVec2 dim, int stride = -1) : Image<color_t>((color_t*)buffer, dim.x, dim.y, stride)
            {
            }


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
        template<typename T> void set(T * buffer, int lx, int ly, int stride = -1)
            {
            _buffer = (color_t*)buffer;
            _lx = lx;
            _ly = ly;
            _stride = (stride < 0) ? lx : stride;
            _checkvalid(); // make sure dimension/stride are ok else make the image invalid
            }


        /**
         * Set/update the image parameters.
         *
         * @param [in,out]  buffer  the pixel buffer.
         * @param           dim     the image dimensions.
         * @param           stride  (Optional) The stride. If not specified, the stride is set equal to.
        **/
        template<typename T> void set(T * buffer, iVec2 dim, int stride = -1)
            {
            set<color_t>((color_t*)buffer, dim.x, dim.y, stride);
            }


        /**
         * Crop this image by keeping only the region represented by subbox (intersected with the image
         * box).
         * 
         * This operation does not change the underlying pixel buffer: it simply replaces this image by
         * a sub-image of itself (with different dimension/stride).
         *
         * @param   subbox  The region to to keep.
        **/
        void crop(const iBox2& subbox)
            {
            *this = Image<color_t>(*this, subbox, true);
            }


        /**
         * Return a sub-image of this image (sharing the same pixel buffer).
         *
         * @param   subbox  The region to to keep.
         * @param   clamp   (Optional) True to clamp the values of the `subbox` parameter. If set,
         *                  `subbox` is intersected with this image box to create a valid region.
         *                  Otherwise, and if `subbox` does not fit inside the source image box, 
         *                  then an empty image is returned.
         *
         * @returns the cropped image.
        **/
        Image<color_t> getCrop(const iBox2& subbox, bool clamp = true) const
            {
            return Image<color_t>(*this, subbox, clamp);
            }




    /************************************************************************************
    * 
    *
    *  Image dimensions
    * 
    *
    *************************************************************************************/


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
         * Return the pixel buffer (const version).
         *
         * @returns A pointer to the start of the pixel buffer associated with this image (or nullptr if
         *          the image is invalid).
        **/
        inline TGX_INLINE const color_t * data() const { return _buffer; }


        /**
         * Return the pixel buffer.
         *
         * @returns A pointer to the start of the pixel buffer associated with this image (or nullptr if
         *          the image is invalid).
        **/
        inline TGX_INLINE color_t * data() { return _buffer; }


        /**
         * Query if this object is valid
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
        void setInvalid()
            {
            _buffer = nullptr;
            _lx = 0; 
            _ly = 0; 
            _stride = 0;
            }



    /****************************************************************************
    * 
    * 
    * Direct pixel access
    * 
    *
    ****************************************************************************/


        /**
         * Set a pixel at a given position.
         * 
         * Use template parameter to disable range check for faster access (danger!).
         *
         * @tparam  CHECKRANGE  True to check the range and false to disable checking.
         * @param   x       The x coordinate.
         * @param   y       The y coordinate.
         * @param   color   The color to set.
        **/
        template<bool CHECKRANGE = true> TGX_INLINE inline void drawPixel(int x, int y, color_t color)
            {
            if (CHECKRANGE) // optimized away at compile time
                {
                if ((!isValid()) || (x < 0) || (y < 0) || (x >= _lx) || (y >= _ly)) return;
                }
            _buffer[TGX_CAST32(x) + TGX_CAST32(_stride) * TGX_CAST32(y)] = color;
            }



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
            drawPixel<CHECKRANGE>(pos.x, pos.y, color);
            }


        /**
         * Blend a pixel with the current pixel color after multiplying its opacity with a given factor.
         * If type color_t has an alpha channel, then it is used for alpha blending.
         * 
         * Use template parameter to disable range check for faster access (danger!).
         *
         * @tparam  CHECKRANGE  True to check the range and false to disable checking.
         * @param   x       The x coordinate.
         * @param   y       The y coordinate.
         * @param   color   The color to blend.
         * @param   opacity mult. opacity factor from 0.0f (fully transparent) to 1.0f fully transparent.
        **/
        template<bool CHECKRANGE = true> TGX_INLINE inline void drawPixel(int x, int y, color_t color, float opacity)
            {
            if (CHECKRANGE) // optimized away at compile time
                {
                if ((!isValid()) || (x < 0) || (y < 0) || (x >= _lx) || (y >= _ly)) return;
                }
            _buffer[TGX_CAST32(x) + TGX_CAST32(_stride) * TGX_CAST32(y)].blend(color,opacity);
            }


        /**
         * Blend a pixel with the current pixel color after multiplying its opacity with a given factor.
         * If type color_t has an alpha channel, then it is used for alpha blending.
         *
         * Use template parameter to disable range check for faster access (danger!).
         *
         * @tparam  CHECKRANGE  True to check the range and false to disable checking.
         * @param   pos     The position
         * @param   color   The color to blend.
         * @param   opacity mult. opacity factor from 0.0f (fully transparent) to 1.0f fully transparent.
        **/
        template<bool CHECKRANGE = true> TGX_INLINE inline void drawPixel(iVec2 pos, color_t color, float opacity)
            {
            drawPixel<CHECKRANGE>(pos.x, pos.y, color, opacity);
            }


        /**
         * Return the color of a pixel at a given position.
         * 
         * If CHECKRANGE is true, outside_color is returned when querying outside of the image.
         *
         * @tparam  CHECKRANGE  True to check the range and false to disable it (danger!).
         * @param   x               The x coordinate.
         * @param   y               The y coordinate.
         * @param   outside_color   (Optional) color to return when querying outside the range.
         *
         * @returns The pixel color
        **/
        template<bool CHECKRANGE = true> TGX_INLINE inline color_t readPixel(int x, int y, color_t outside_color = color_t()) const
            {
            if (CHECKRANGE) // optimized away at compile time
                {
                if (((!isValid()) || (x < 0) || (y < 0) || (x >= _lx) || (y >= _ly))) return outside_color;
                }
            return _buffer[TGX_CAST32(x) + TGX_CAST32(_stride) * TGX_CAST32(y)];
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
            return readPixel<CHECKRANGE>(pos.x, pos.y, outside_color);
            }


        /**
         * Get a reference to a pixel (no range check!) const version
         *
         * @param   x   The x coordinate.
         * @param   y   The y coordinate.
         *
         * @returns a const reference to the pixel color.
        **/
        TGX_INLINE inline const color_t & operator()(int x, int y) const
            {
            return _buffer[TGX_CAST32(x) + TGX_CAST32(_stride) * TGX_CAST32(y)];
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
         * Get a reference to a pixel (no range check!)
         *
         * @param   x   The x coordinate.
         * @param   y   The y coordinate.
         *
         * @returns a reference to the pixel color.
        **/
        TGX_INLINE inline color_t & operator()(int x, int y)
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
        TGX_INLINE inline color_t& operator()(iVec2 pos)
            {
            return _buffer[TGX_CAST32(pos.x) + TGX_CAST32(_stride) * TGX_CAST32(pos.y)];
            }


        /**
         * Iterate over all the pixels of the image going from left to right and then from top to
         * bottom. The callback function cb_fun is called for each pixel and must have a signature
         * compatible with:
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
        template<typename ITERFUN> void iterate(ITERFUN cb_fun)
            {
            iterate(cb_fun, imageBox());
            }


        /**
         * Same as above but iterate only over the pixels inside the sub-box B (intersected with the
         * image box).
         *
         * @param   cb_fun  The callback function.
         * @param   B       The sub-box to iterate inside.
        **/
        template<typename ITERFUN> void iterate(ITERFUN cb_fun, tgx::iBox2 B);





    /****************************************************************************
    *
    *
    * Blitting / copying / resizing images
    *
    *
    ****************************************************************************/






        /**
         * Blit a sprite at a given position on the image.
         * 
         * Remark: If only part of the sprite should be blit onto this image, one simply has to create a
         * temporary sub-image like so: 'blit(sprite.getCrop(subbox),upperleftpos)'. No  copy is
         * performed when creating (shallow) sub-image so this does not incur any slowdown.
         *
         * @param   sprite          The sprite image to blit.
         * @param   upperleftpos    Position of the upper left corner of the sprite in the image.
        **/
        void blit(const Image<color_t> & sprite, iVec2 upperleftpos)
            {
            _blit(sprite, upperleftpos.x, upperleftpos.y, 0, 0, sprite.lx(), sprite.ly());
            }


        /**
         * Blit a sprite at a given position on this image.
         * Same as above using ints instead of iVec2 for position (deprecated).
        **/
        DEPRECATED_SCALAR_PARAMS void blit(const Image<color_t>& sprite, int dest_x, int dest_y)
            {
            _blit(sprite, dest_x, dest_y, 0, 0, sprite.lx(), sprite.ly());
            }


        /**
         * Blend a sprite over this image at a given position.
         * 
         * Use blending to draw the sprite over the image with a given opacity. If color_t has an alpha
         * channel, it is also used or blending.
         *
         * @param   sprite          The sprite image to blit.
         * @param   upperleftpos    Position of the upper left corner of the sprite in the image.
         * @param   opacity         The mult. opacity factor from 0.0f (transparent) and 1.0f (opaque).
        **/
        void blit(const Image<color_t>& sprite, iVec2 upperleftpos, float opacity)
            {
            _blit(sprite, upperleftpos.x, upperleftpos.y, 0, 0, sprite.lx(), sprite.ly(),opacity);
            }


        /**
         * Blend a sprite over this image at a given position.
         * Same as above using ints instead of iVec2 for position (deprecated).
        **/
        DEPRECATED_SCALAR_PARAMS void blit(const Image<color_t>& sprite, int dest_x, int dest_y, float opacity)
            {
            _blit(sprite, dest_x, dest_y, 0, 0, sprite.lx(), sprite.ly(), opacity);
            }


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
         * 1. To perform "classical alpha blending", use the blit() method with an opacity parameter instead   
         *    as it will be faster.        
         * 2. If only part of the sprite should be blended onto this image, one simply has to create a
         *    temporary sub-image like so: 'blend(sprite.getCrop(subbox),upperleftpos, blend_op)'. No copy 
         *    is performed when creating (shallow) sub-image so this does not incur any slowdown.
         *
         * @param   sprite          The sprite image to blend.
         * @param   upperleftpos    Position of the upper left corner of the sprite in the image.
         * @param   blend_op        The blending operator
        **/
        template<typename color_t_src, typename BLEND_OPERATOR> 
        void blit(const Image<color_t_src>& sprite, iVec2 upperleftpos, const BLEND_OPERATOR& blend_op)
            {
            _blit(sprite, upperleftpos.x, upperleftpos.y, 0, 0, sprite.lx(), sprite.ly(), blend_op);
            }



        /**
         * Blend a sprite at a given position on this image. Sprite pixels with color
         * `transparent_color` are treated as transparent hence not copied on the image. Other pixels
         * are blended with the destination image after being multiplied by the opacity factor.
         * 
         * Remark: this method is especially useful when color_t does not have an alpha channel hence no
         * predefined transparent color.
         *
         * @param   sprite              The sprite image to blit.
         * @param   transparent_color   The sprite color considered transparent.
         * @param   upperleftpos        Position of the upper left corner of the sprite in the image.
         * @param   opacity             The mult. opacity factor from 0.0f (transparent) and 1.0f (opaque).
        **/
        void blitMasked(const Image<color_t>& sprite, color_t transparent_color, iVec2 upperleftpos, float opacity)
            {
            _blitMasked(sprite, transparent_color, upperleftpos.x, upperleftpos.y, 0, 0, sprite.lx(), sprite.ly(), opacity);
            }


        /**
         * Blit a sprite at a given position on this image. Sprite pixels with color `transparent_color`
         * Same as above using ints instead of iVec2 for position (deprecated).
        **/
        DEPRECATED_SCALAR_PARAMS void blitMasked(const Image<color_t>& sprite, color_t transparent_color, int dest_x, int dest_y, float opacity)
            {
            _blitMasked(sprite, transparent_color, dest_x, dest_y, 0, 0, sprite.lx(), sprite.ly(), opacity);
            }


        /**
         * Reverse blitting. Copy part of the image into the sprite
         * This is the inverse of the blit operation.
         *
         * @param   dst_sprite      The sprite to copy part of this image into.
         * @param   upperleftpos    Position of the upper left corner of the sprite in the image.
        **/
        void blitBackward(Image<color_t> & dst_sprite, iVec2 upperleftpos) const
            {
            dst_sprite._blit(*this, 0, 0, upperleftpos.x, upperleftpos.y, dst_sprite.lx(), dst_sprite.ly());
            }


        /**
         * Reverse blitting. Copy part of the image into the sprite
         * Same as above using ints instead of iVec2 for position (deprecated).
        **/
        DEPRECATED_SCALAR_PARAMS void blitBackward(Image<color_t>& dst_sprite, int dest_x, int dest_y) const
            {
            dst_sprite._blit(*this, 0, 0, dest_x, dest_y, dst_sprite.lx(), dst_sprite.ly());
            }


        /**
         * Blit a sprite onto this image after rescaling and rotation. The anchor point 'anchor_src' in
         * the sprite is mapped to 'anchor_im' in this image. The rotation is performed in counter-
         * clockwise direction around the anchor point.
         * 
         * Remarks:
         * 
         * 1. The positions are given using floating point values to allow for sub-pixel precision for
         *    smoother animation.
         * 2. The method use bilinear interpolation for high quality rendering.  
         * 3. The sprite image can have a different color type from this image.
         * 4. This version does NOT use blending: pixel from the source are simply copied over this
         *    image.
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
         * @param   src_im          The sprite image to draw.
         * @param   anchor_src      Position of the anchor point in the sprite image.
         * @param   anchor_dst      Position of the anchor point in this (destination) image.
         * @param   scale           Scaling factor (1.0f for no scaling).
         * @param   angle_degrees   The rotation angle in degrees.
        **/
        template<typename color_t_src, int CACHE_SIZE = TGX_PROGMEM_DEFAULT_CACHE_SIZE>
        void blitScaledRotated(const Image<color_t_src>& src_im, fVec2 anchor_src, fVec2 anchor_dst, float scale, float angle_degrees)
            {
            _blitScaledRotated<color_t_src, CACHE_SIZE, false, false, false>(src_im, color_t_src(), anchor_src, anchor_dst, scale, angle_degrees, 1.0f, [](color_t_src cola, color_t colb) {return colb; });
            }


        /**
         * Blend a sprite onto this image after rescaling and rotation. The anchor point 'anchor_src' in
         * the sprite is mapped to 'anchor_im' in this image. The rotation is performed in counter-
         * clockwise direction around the anchor point.
         * 
         * Remarks:
         * 
         * 1. The positions are given using floating point values to allow for sub-pixel precision for
         *    smoother animation.
         * 2. The method use bilinear interpolation for high quality rendering.
         * 3. The sprite image can have a different color type from this image.
         * 4. This version use blending and opacity. If the sprite image color type has an alpha channel,
         *    then it is used for blending and multiplied by the opacity factor (even if this image does
         *    not have an alpha channel).
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
         * @param   src_im          The sprite image to draw.
         * @param   anchor_src      Position of the anchor point in the sprite image.
         * @param   anchor_dst      Position of the anchor point in this (destination) image.
         * @param   scale           Scaling factor (1.0f for no scaling).
         * @param   angle_degrees   The rotation angle in degrees.
         * @param   opacity         The opacity multiplier between 0.0f (transparent) and 1.0f (opaque).
        **/
        template<typename color_t_src, int CACHE_SIZE = TGX_PROGMEM_DEFAULT_CACHE_SIZE>
        void blitScaledRotated(const Image<color_t_src> src_im, fVec2 anchor_src, fVec2 anchor_dst, float scale, float angle_degrees, float opacity)
            {
            _blitScaledRotated<color_t_src, CACHE_SIZE, true, false, false>(src_im, color_t_src(), anchor_src, anchor_dst, scale, angle_degrees, opacity, [](color_t_src cola, color_t colb) {return colb; });
            }


        /**
         * Blend a sprite onto this image after rescaling and rotation using a custom blending operator.
         * 
         * The anchor point 'anchor_src' in the sprite is mapped to 'anchor_im' in this image. The rotation 
         * is performed in counter-clockwise direction around the anchor point.
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
         * @tparam  color_t_src Color type of the sprite image.
         * @tparam  CACHE_SIZE  Size of the MCU cache when reading from flash. This value is indicative
         *                      and used to optimize cache access to flash memory. You may try changing
         *                      the default value it if drawing takes a long time...
         * @param   src_im          The sprite image to draw.
         * @param   anchor_src      Position of the anchor point in the sprite image.
         * @param   anchor_dst      Position of the anchor point in this (destination) image.
         * @param   scale           Scaling factor (1.0f for no scaling).
         * @param   angle_degrees   The rotation angle in degrees.
         * @param   blend_op        The blending operator
        **/
        template<typename color_t_src, typename BLEND_OPERATOR, int CACHE_SIZE = TGX_PROGMEM_DEFAULT_CACHE_SIZE>
        void blitScaledRotated(const Image<color_t_src>& src_im, fVec2 anchor_src, fVec2 anchor_dst, float scale, float angle_degrees, const BLEND_OPERATOR& blend_op)
            {
            _blitScaledRotated<color_t_src, CACHE_SIZE, true, false, true>(src_im, color_t_src(), anchor_src, anchor_dst, scale, angle_degrees, 1.0f, blend_op);
            }


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
         * 4. This version use blending and opacity. If the sprite image color type has an alpha channel,
         *    then it is used for blending and multiplied by the opacity factor (even if this image does
         *    not have an alpha channel).
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
         * @param   opacity             The opacity multiplier between 0.0f (transparent) and 1.0f
         *                              (opaque).
        **/
        template<typename color_t_src, int CACHE_SIZE = TGX_PROGMEM_DEFAULT_CACHE_SIZE>
        void blitScaledRotatedMasked(const Image<color_t_src>& src_im, color_t_src transparent_color, fVec2 anchor_src, fVec2 anchor_dst, float scale, float angle_degrees, float opacity)
            {
            _blitScaledRotated<color_t_src, CACHE_SIZE, true, true, false>(src_im, transparent_color, anchor_src, anchor_dst, scale, angle_degrees, opacity, [](color_t_src cola, color_t colb) {return colb; });
            }

       


        /**
         * Copy the src image onto the destination image with resizing and color conversion.
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
        **/
        template<typename src_color_t> void copyFrom(const Image<src_color_t>& src_im);


        /**
         * Blend the src image onto the destination image with resizing and color conversion.
         * 
         * This version uses blending and opacity to combine the src over the existing image.
         * 
         * 1. the source image is resized to match this image size. Bilinear interpolation is used to
         *    improve quality.
         * 2. The source and destination image may have different color type. Conversion is automatic.
         * 3. This version use blending and opacity. If the source image color type has an alpha channel,
         *    then it is used for blending and multiplied by the opacity factor (even if this image does
         *    not have an alpha channel).
         * 
         * Beware: The method does not check for buffer overlap between source and destination !
         *
         * @tparam  src_color_t color type of the source image
         * @param   src_im  Source image to copy onto this image.
         * @param   opacity opacity between 0.0f (fully transparent) and 1.0f (fully opaque).
        **/
        template<typename src_color_t> void copyFrom(const Image<src_color_t>& src_im, float opacity);


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
        * Copy the source image pixels into this image, reducing it by half in the process.
        * Ignore the last row/column for odd dimensions larger than 1.
        * Resizing is done by averaging the color of the 4 neighbour pixels. 
        * 
        * This image must be large enough to accomodate the reduced image otherwise do nothing. 
        * The reduced image is copied on the top left corner of this image.
        * 
        * Return a sub-image of this image corresponding to the reduced image pixels (or an 
        * empty image if nothing was done).
        * 
        * REMARK: This is an old method. Use blitScaledRotated() for a more powerful method.
        **/
        Image<color_t> copyReduceHalf(const Image<color_t> & src_image);



        /**
        * Reduce this image by half. Use the same memory buffer and keep the same stride. 
        * Resizing is done by averaging the color of the 4 neighbour pixels.
        * Same as copyReduceHalf(*this)
        *
        * Return a sub-image of this image corresponding to the reduced image pixels
        * which correspond to the upper left half of this image. 
        **/
        Image<color_t> reduceHalf()
            {
            return copyReduceHalf(*this);
            }




        /*********************************************************************
        *
        * Flood filling
        * 
        **********************************************************************/



        /**
		* Flood fill a 4-connected region of the image.
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
        template<int STACK_SIZE = 1024> int fill(iVec2 start_pos, color_t new_color)
            {
            return _scanfill<true, STACK_SIZE>(start_pos.x,start_pos.y, new_color, new_color);
            }



		/**
		* Flood fill a 4-connected region of the image.
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
        template<int STACK_SIZE = 1024> int fill(iVec2 start_pos, color_t border_color, color_t new_color)
            {
            return _scanfill<false, STACK_SIZE>(start_pos.x, start_pos.y, border_color, new_color);
            }








    /****************************************************************************
    * 
    * 
    * Drawing primitives
    * 
    *
    ****************************************************************************/



        /*********************************************************************
        *
        * Screen filling
        * 
        **********************************************************************/


        /**
         * Fill the whole image with a single color
         *
         * @param   color   The color to use.
        **/
        void fillScreen(color_t color)
            {
            fillRect(imageBox(), color);
            }


        /**
         * Fill the whole image with a vertical color gradient between color1 and color2. color
         * interpolation takes place in RGB color space (even if color_t is HSV).
         *
         * @param   top_color       color at the top of the image
         * @param   bottom_color    color at the bottom of the image.
        **/
        void fillScreenVGradient(color_t top_color, color_t bottom_color)
            {
            fillRectVGradient(imageBox(),top_color, bottom_color);
            }


        /**
         * Fill the whole screen with an horizontal color gradient between color1 and color2. color
         * interpolation takes place in RGB color space (even if color_t is HSV).
         *
         * @param   left_color  color on the left side of the image
         * @param   right_color color on the right side of the image
        **/
        void fillScreenHGradient(color_t left_color, color_t right_color)
            {
            fillRectHGradient(imageBox(), left_color, right_color);
            }




        /*********************************************************************
        *
        * drawing lines
        * 
        **********************************************************************/


        /**
        * Draw an vertical line of h pixels starting at pos.
        **/
        template<bool CHECKRANGE = true> TGX_INLINE inline void drawFastVLine(iVec2 pos, int h, color_t color)
            {
            int x = pos.x;
            int y = pos.y;
            if (CHECKRANGE) // optimized away at compile time
                {
                if ((!isValid()) || (x < 0) || (x >= _lx) || (y >= _ly)) return;
                if (y < 0) { h += y; y = 0; }
                if (y + h > _ly) { h = _ly - y; }               
                }
            color_t* p = _buffer + TGX_CAST32(x) + TGX_CAST32(y) * TGX_CAST32(_stride);
            while (h-- > 0)
                {
                (*p) = color;
                p += _stride;
                }
            }


        /**
        * Draw a vertical line of h pixels starting at (x,y).
        **/
        template<bool CHECKRANGE = true> 
        DEPRECATED_SCALAR_PARAMS inline void drawFastVLine(int x, int y, int h, color_t color)
            {
            drawFastVLine<CHECKRANGE>({ x, y }, h, color);
            }


        /**
        * Draw an vertical line of h pixels starting at pos.
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        template<bool CHECKRANGE = true> TGX_INLINE inline void drawFastVLine(iVec2 pos, int h, color_t color, float opacity)
            {
            int x = pos.x;
            int y = pos.y;
            if (CHECKRANGE) // optimized away at compile time
                {
                if ((!isValid()) || (x < 0) || (x >= _lx) || (y >= _ly)) return;
                if (y < 0) { h += y; y = 0; }
                if (y + h > _ly) { h = _ly - y; }
                }
            color_t* p = _buffer + TGX_CAST32(x) + TGX_CAST32(y) * TGX_CAST32(_stride);
            while (h-- > 0)
                {
                (*p).blend(color, opacity);
                p += _stride;
                }   
            }


        /**
         * Draw a vertical line of h pixels starting at (x,y).
         * 
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        template<bool CHECKRANGE = true> 
        DEPRECATED_SCALAR_PARAMS inline void drawFastVLine(int x, int y, int h, color_t color, float opacity)
            {
            drawFastVLine<CHECKRANGE>({ x, y }, h, color, opacity);
            }



        /**
        * Draw an horizontal line of w pixels starting at pos.
        **/
        template<bool CHECKRANGE = true> TGX_INLINE inline void drawFastHLine(iVec2 pos, int w, color_t color)
            {
            int x = pos.x;
            int y = pos.y;
            if (CHECKRANGE) // optimized away at compile time
                {
                if ((!isValid()) || (y < 0) || (y >= _ly) || (x >= _lx)) return;
                if (x < 0) { w += x; x = 0; }
                if (x + w > _lx) { w = _lx - x; }
                }
            _fast_memset(_buffer + TGX_CAST32(x) + TGX_CAST32(y) * TGX_CAST32(_stride), color, w);
            }


        /**
        * Draw an horizontal line of w pixels starting at (x,y). 
        **/
        template<bool CHECKRANGE = true> 
        DEPRECATED_SCALAR_PARAMS inline void drawFastHLine(int x, int y, int w, color_t color)
            {           
            drawFastHLine<CHECKRANGE>({ x, y }, w, color);
            }


        /**
        * Draw an horizontal line of w pixels starting at pos.
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        template<bool CHECKRANGE = true> TGX_INLINE inline void drawFastHLine(iVec2 pos, int w, color_t color, float opacity)
            {
            int x = pos.x;
            int y = pos.y;
            if (CHECKRANGE) // optimized away at compile time
                {
                if ((!isValid()) || (y < 0) || (y >= _ly) || (x >= _lx)) return;
                if (x < 0) { w += x; x = 0; }
                if (x + w > _lx) { w = _lx - x; }
                }
            color_t* p = _buffer + TGX_CAST32(x) + TGX_CAST32(y) * TGX_CAST32(_stride);
            while (w-- > 0)
                {
                (*p++).blend(color, opacity);
                }
            }


        /**
        * Draw an horizontal line of w pixels starting at (x,y).
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        template<bool CHECKRANGE = true> 
        DEPRECATED_SCALAR_PARAMS inline void drawFastHLine(int x, int y, int w, color_t color, float opacity)
            {   
            drawFastHLine<CHECKRANGE>({ x, y }, w, color, opacity);
            }



        /**
        * Draw a line between P1 and P2 using Bresenham algorithm
        **/
        inline void drawLine(iVec2 P1, iVec2 P2, color_t color)
            {
            const int x0 = P1.x;
            const int y0 = P1.y;
            const int x1 = P2.x;
            const int y1 = P2.y;
            if (!isValid()) return;
            if ((x0 < 0) || (y0 < 0) || (x1 < 0) || (y1 < 0) || (x0 >= _lx) || (y0 >= _ly) || (x1 >= _lx) || (y1 >= _ly))
                _drawLine<true>(x0, y0, x1, y1, color);
            else
                _drawLine<false>(x0, y0, x1, y1, color);            
            }


        /**
        * Draw a line between (x0,y0) and (x1,y1) using Bresenham algorithm
        **/
        DEPRECATED_SCALAR_PARAMS void drawLine(int x0, int y0, int x1, int y1, color_t color)
            {
            drawLine({ x0,y0 }, { x1,y1 }, color);
            }


        /**
        * Draw a line between P1 and P2 using Bresenham algorithm
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        inline void drawLine(iVec2 P1, iVec2 P2, color_t color, float opacity)
            {
            const int x0 = P1.x;
            const int y0 = P1.y;
            const int x1 = P2.x;
            const int y1 = P2.y;
            if (!isValid()) return;
            if ((x0 < 0) || (y0 < 0) || (x1 < 0) || (y1 < 0) || (x0 >= _lx) || (y0 >= _ly) || (x1 >= _lx) || (y1 >= _ly))
                _drawLine<true>(x0, y0, x1, y1, color, opacity);
            else
                _drawLine<false>(x0, y0, x1, y1, color, opacity);
            }


        /**
        * Draw a line between (x0,y0) and (x1,y1) using Bresenham algorithm
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        DEPRECATED_SCALAR_PARAMS void drawLine(int x0, int y0, int x1, int y1, color_t color, float opacity)
            {
            drawLine({ x0,y0 }, { x1,y1 }, color, opacity);
            }


        /**
        * Draw a polyline i.e. a sequence of consecutif segments [P0,P1] , [P1,P2], ... , [Pn-1,Pn]
        *
        * @param	nbpoints		number of points in tabPoints
        * @param	tabPoints		array of points
        * @param	draw_last_point	true to draw the last point
        * @param	color			The color to use.
        **/
        void drawPolyLine(int nbpoints, const iVec2 tabPoints[], bool draw_last_point, color_t color)
            {
            _drawPolyLine<false>(nbpoints, tabPoints, draw_last_point, color, 1.0f);
            }


        /**
        * Draw a polyline i.e. a sequence of consecutif segments [P0,P1] , [P1,P2], ... , [Pn-1,Pn]
        * Same as above but use blending
        *
        * @param	nbpoints		number of points in tabPoints
        * @param	tabPoints		array of points
        * @param	draw_last_point	true to draw the last point
        * @param	color			The color to use.
        * @param    opacity         opacity scale factor between 0.0f (transparent) and 1.0f (fully opaque).
        **/
        void drawPolyLine(int nbpoints, const iVec2 tabPoints[], bool draw_last_point, color_t color, float opacity)
            {
            _drawPolyLine<true>(nbpoints, tabPoints, draw_last_point, color, opacity);
            }



        /**
        * Draw a closed polygon with vertices [P0,P2, .., PN]
        * Same as above but use blending
        *
        * @param	nbpoints		number of points in tabPoints
        * @param	tabPoints		array of points of the polygon
        * @param	color			The color to use.
        * @param    opacity         opacity scale factor between 0.0f (transparent) and 1.0f (fully opaque).
        **/
        void drawPolygon(int nbpoints, const iVec2 tabPoints[], color_t color)
            {
            _drawPolygon<false>(nbpoints, tabPoints, color, 1.0f);
            }


        /**
        * Draw a closed polygon with vertices [P0,P2, .., PN]
        * Same as above but use blending
        *
        * @param	nbpoints		number of points in tabPoints
        * @param	tabPoints		array of points of the polygon
        * @param	color			The color to use.
        * @param    opacity         opacity scale factor between 0.0f (transparent) and 1.0f (fully opaque).
        **/
        void drawPolygon(int nbpoints, const iVec2 tabPoints[], color_t color, float opacity)
            {
            _drawPolygon<true>(nbpoints, tabPoints, color, opacity);
            }



        /*********************************************************************
        *
        * drawing Bezier curves/splines
        *
        **********************************************************************/


        /**
        * Draw a quadratic (rational) Bezier curve.
        *
        * @param	P1				Start point.
        * @param	P2				End point.
        * @param	PC				Control point.
        * @param	wc				Control point weight (must be positive). Fastest for wc=1.
        * @param	draw_P2			true to draw the endpoint P2.
        * @param	color			The color to use.
        **/
        void drawQuadBezier(iVec2 P1, iVec2 P2, iVec2 PC, float wc, bool drawP2, color_t color)
            {
            _drawQuadBezier<false>(P1, P2, PC, wc, drawP2, color, 1.0f);
            }


        /**
        * Draw a quadratic (rational) Bezier curve. Same as above but uses blending.
        *
        * @param	P1				Start point.
        * @param	P2				End point.
        * @param	PC				Control point.
        * @param	wc				Control point weight (must be positive). Fastest for wc=1.
        * @param	draw_P2			true to draw the endpoint P2.
        * @param	color			The color to use.
        * @param    opacity         opacity scale factor between 0.0f (transparent) and 1.0f (fully opaque).
        **/
        void drawQuadBezier(iVec2 P1, iVec2 P2, iVec2 PC, float wc, bool drawP2, color_t color, float opacity)
            {
            _drawQuadBezier<true>(P1, P2, PC, wc, drawP2, color, opacity);
            }


        /**
        * Draw a cubic Bezier curve.
        *
        * @param	P1				Start point.
        * @param	P2				End point.
        * @param	PA				first control point.
        * @param	PB				second control point.
        * @param	draw_P2			true to draw the endpoint P2.
        * @param	color			The color to use.
        **/
        void drawCubicBezier(iVec2 P1, iVec2 P2, iVec2 PA, iVec2 PB, bool drawP2, color_t color)
            {
            _drawCubicBezier<false>(P1, P2, PA, PB, drawP2, color, 1.0f);
            }


        /**
        * Draw a cubic Bezier curve. Same as above but uses blending.
        *
        * @param	P1				Start point.
        * @param	P2				End point.
        * @param	PA				first control point.
        * @param	PB				second control point.
        * @param	draw_P2			true to draw the endpoint P2.
        * @param	color			The color to use.
        * @param    opacity         opacity scale factor between 0.0f (transparent) and 1.0f (fully opaque).
        **/
        void drawCubicBezier(iVec2 P1, iVec2 P2, iVec2 PA, iVec2 PB, bool drawP2, color_t color, float opacity)
            {
            _drawCubicBezier<true>(P1, P2, PA, PB, drawP2, color, opacity);
            }




        /**
        * Draw a quadratic spline interpolating between a given set of points. 
        *
        * The template parameter SPLINE_MAX_POINTS defines the maximum number of points
        * that a spline can have. Can be increased if needed but this will increase the
        * memory used on the stack for allocating the temporary point arrays...
        * 
        * @param	nbpoints	   	number of points in the spline
        *                           Must be smaller or equal to SPLINE_MAX_POINTS. 
        * @param	tabPoints	   	the array of points to interpolate.
        * @param	draw_last_point	true to draw the last point.
        * @param	color		   	The color to use.
        **/
        template<int SPLINE_MAX_POINTS = 32>
        void drawQuadSpline(int nbpoints, const iVec2 tabPoints[], bool draw_last_point, color_t color)
            {
            _drawQuadSpline< SPLINE_MAX_POINTS, false>(nbpoints, tabPoints, draw_last_point, color, 1.0f);
            }


        /**
        * Draw a quadratic spline interpolating between a given set of points. Same as above
        * but uses blending.
        *
        * The template parameter SPLINE_MAX_POINTS defines the maximum number of points
        * that a spline can have. Can be increased if needed but this will increase the
        * memory used on the stack for allocating the temporary point arrays...
        * 
        * @param	nbpoints	   	number of points in the spline
        *                           Must be smaller or equal to SPLINE_MAX_POINTS. 
        * @param	tabPoints	   	the array of points to interpolate.
        * @param	draw_last_point	true to draw the last point.
        * @param	color		   	The color to use.
        * @param    opacity         opacity scale factor between 0.0f (transparent) and 1.0f (fully opaque).
        **/
        template<int SPLINE_MAX_POINTS = 32>
        void drawQuadSpline(int nbpoints, const iVec2 tabPoints[], bool draw_last_point, color_t color, float opacity)
            {
            _drawQuadSpline< SPLINE_MAX_POINTS, true>(nbpoints, tabPoints, draw_last_point, color, opacity);
            }


        /**
        * Draw a cubic spline interpolating between a given set of points. 
        *
        * The template parameter SPLINE_MAX_POINTS defines the maximum number of points
        * that a spline can have. Can be increased if needed but this will increase the
        * memory used on the stack for allocating the temporary point arrays...
        * 
        * @param	nbpoints	   	number of points in the spline
        *                           Must be smaller or equal to SPLINE_MAX_POINTS. 
        * @param	tabPoints	   	the array of points to interpolate.
        * @param	draw_last_point	true to draw the last point.
        * @param	color		   	The color to use.
        **/
        template<int SPLINE_MAX_POINTS = 32>
        void drawCubicSpline(int nbpoints, const iVec2 tabPoints[], bool draw_last_point, color_t color)
            {
            _drawCubicSpline< SPLINE_MAX_POINTS, false>(nbpoints, tabPoints, draw_last_point, color, 1.0f);
            }


        /**
        * Draw a cubic spline interpolating between a given set of points. Same as above
        * but uses blending.
        *
        * The template parameter SPLINE_MAX_POINTS defines the maximum number of points
        * that a spline can have. Can be increased if needed but this will increase the
        * memory used on the stack for allocating the temporary point arrays...
        * 
        * @param	nbpoints	   	number of points in the spline
        *                           Must be smaller or equal to SPLINE_MAX_POINTS. 
        * @param	tabPoints	   	the array of points to interpolate.
        * @param	draw_last_point	true to draw the last point.
        * @param	color		   	The color to use.
        * @param    opacity         opacity scale factor between 0.0f (transparent) and 1.0f (fully opaque).
        **/
        template<int SPLINE_MAX_POINTS = 32>
        void drawCubicSpline(int nbpoints, const iVec2 tabPoints[], bool draw_last_point, color_t color, float opacity)
            {
            _drawCubicSpline< SPLINE_MAX_POINTS, true>(nbpoints, tabPoints, draw_last_point, color, opacity);
            }



        /**
        * Draw a closed quadratic spline interpolating between a given set of points.
        *
        * The template parameter SPLINE_MAX_POINTS defines the maximum number of points
        * that a spline can have. Can be increased if needed but this will increase the
        * memory used on the stack for allocating the temporary point arrays...
        *
        * @param	nbpoints	   	number of points in the spline
        *                           Must be smaller or equal to SPLINE_MAX_POINTS.
        * @param	tabPoints	   	the array of points to interpolate.
        * @param	draw_last_point	true to draw the last point.
        * @param	color		   	The color to use.
        **/
        template<int SPLINE_MAX_POINTS = 32>
        void drawClosedSpline(int nbpoints, const iVec2 tabPoints[], color_t color)
            {
            _drawClosedSpline<SPLINE_MAX_POINTS, false>(nbpoints, tabPoints, color, 1.0f);
            }


        /**
        * Draw a closed quadratic spline interpolating between a given set of points. Same as above
        * but uses blending.
        *
        * The template parameter SPLINE_MAX_POINTS defines the maximum number of points
        * that a spline can have. Can be increased if needed but this will increase the
        * memory used on the stack for allocating the temporary point arrays...
        *
        * @param	nbpoints	   	number of points in the spline
        *                           Must be smaller or equal to SPLINE_MAX_POINTS.
        * @param	tabPoints	   	the array of points to interpolate.
        * @param	draw_last_point	true to draw the last point.
        * @param	color		   	The color to use.
        * @param    opacity         opacity scale factor between 0.0f (transparent) and 1.0f (fully opaque).
        **/
        template<int SPLINE_MAX_POINTS = 32>
        void drawClosedSpline(int nbpoints, const iVec2 tabPoints[], color_t color, float opacity)
            {
            _drawClosedSpline<SPLINE_MAX_POINTS, true>(nbpoints, tabPoints, color, opacity);
            }


        /*********************************************************************
        *
        * drawing triangles
        *
        **********************************************************************/


        /**
        * Draw a triangle (but not its interior).
        **/
        void drawTriangle(const iVec2& P1, const iVec2& P2, const iVec2& P3, color_t color)
            {
            _drawTriangle<false, true, false>(P1, P2, P3, color, color, 1.0f);
            }


        /**
        * Draw a triangle (but not its interior). 
        **/
        DEPRECATED_SCALAR_PARAMS void drawTriangle(int x1,int y1, int x2, int y2, int x3, int y3, color_t color)
            {
            _drawTriangle<false, true, false>(iVec2(x1, y1), iVec2(x2, y2), iVec2(x3, y3), color, color, 1.0f);
            }


        /**
        * Draw a triangle (but not its interior).
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        void drawTriangle(const iVec2& P1, const iVec2& P2, const iVec2& P3, color_t color, float opacity)
            {
            _drawTriangle<false, true, true>(P1, P2, P3, color, color, opacity);
            }


        /**
        * Draw a triangle (but not its interior).
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        DEPRECATED_SCALAR_PARAMS void drawTriangle(int x1,int y1, int x2, int y2, int x3, int y3, color_t color, float opacity)
            {
            _drawTriangle<false, true, true>(iVec2(x1, y1), iVec2(x2, y2), iVec2(x3, y3), color, color, opacity);
            }


        /**
        * Draw a filled triangle with possibly different colors for the outline and the interior.
        **/
        void fillTriangle(const iVec2& P1, const iVec2& P2, const iVec2& P3, color_t interior_color, color_t outline_color)
            {
            _drawTriangle<true, true, false>(P1, P2, P3, interior_color, outline_color, 1.0f);
            }


        /**
        * Draw a filled triangle with possibly different colors for the outline and the interior.
        **/
        DEPRECATED_SCALAR_PARAMS void fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, color_t interior_color, color_t outline_color)
            {
            _drawTriangle<true, true, false>(iVec2(x1, y1), iVec2(x2, y2), iVec2(x3, y3), interior_color, outline_color, 1.0f);
            }


        /**
        * Draw a filled triangle with possibly different colors for the outline and the interior.
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        void fillTriangle(const iVec2& P1, const iVec2& P2, const iVec2& P3, color_t interior_color, color_t outline_color, float opacity)
            {
            _drawTriangle<true, true, true>(P1, P2, P3, interior_color, outline_color, opacity);
            }


        /**
        * Draw a filled triangle with possibly different colors for the outline and the interior.
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        DEPRECATED_SCALAR_PARAMS void fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, color_t interior_color, color_t outline_color, float opacity)
            {
            _drawTriangle<true, true, true>(iVec2(x1, y1), iVec2(x2, y2), iVec2(x3, y3), interior_color, outline_color, opacity);
            }





        /*********************************************************************
        *
        * drawing rectangles
        *
        **********************************************************************/


        /**
         * Draw a rectangle corresponding to the box B.
         *
         * @param   B       B representing the rectangle.
         * @param   color   Color to use.
        **/
        void drawRect(const iBox2 & B, color_t color)
            {
            drawRect({ B.minX, B.minY }, { B.lx(), B.ly() }, color);
            }


        /**
         * Draw a rectangle with given upper left corner and dimension.
         *
         * @param   upper_left_pos  Position of the upper left corner of the rectangle.
         * @param   dimension       (width,height) of the rectangle.
         * @param   color           color to use.
        **/
        void drawRect(iVec2 upper_left_pos, iVec2 dimension, color_t color)
            {
            const int x = upper_left_pos.x;
            const int y = upper_left_pos.y;
            const int w = dimension.x;
            const int h = dimension.y;
            drawFastHLine<true>({ x, y }, w, color);
            if (h > 1) drawFastHLine<true>({ x, y + h - 1 }, w, color);
            drawFastVLine<true>({ x, y + 1 }, h - 2, color);
            if (w > 1) drawFastVLine<true>({ x + w - 1, y + 1 }, h - 2, color);
            }


        /**
        * Draw a rectangle with size (w,h) and upperleft corner at (x,y)
        **/
        DEPRECATED_SCALAR_PARAMS void drawRect(int x, int y, int w, int h, color_t color)
            {
            drawRect({ x, y }, { w, h }, color);
            }


        /**
         * Draw a rectangle corresponding to the box B.
         * 
         * Blend with the current color background using opacity between 0.0f (fully transparent) and
         * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
         *
         * @param   B       B representing the rectangle.
         * @param   color   Color to use.
         * @param   opacity between 0.0f (transparent) to 1.0f (fully opaque).
        **/
        void drawRect(const iBox2& B, color_t color, float opacity)
            {
            drawRect({ B.minX, B.minY }, { B.lx(), B.ly() }, color, opacity);
            }


        /**
         * Draw a rectangle with given upper left corner and dimension.
         * 
         * Blend with the current color background using opacity between 0.0f (fully transparent) and
         * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
         *
         * @param   upper_left_pos  Position of the upper left corner of the rectangle.
         * @param   dimension       (width,height) of the rectangle.
         * @param   color           color to use.
         * @param   opacity         between 0.0f (transparent) to 1.0f (fully opaque).
        **/
        void drawRect(iVec2 upper_left_pos, iVec2 dimension, color_t color, float opacity)
            {
            const int x = upper_left_pos.x;
            const int y = upper_left_pos.y;
            const int w = dimension.x;
            const int h = dimension.y;
            drawFastHLine<true>({ x, y }, w, color, opacity);
            if (h > 1) drawFastHLine<true>({ x, y + h - 1 }, w, color, opacity);
            drawFastVLine<true>({ x, y + 1 }, h - 2, color, opacity);
            if (w > 1) drawFastVLine<true>({ x + w - 1, y + 1 }, h - 2, color, opacity);
            }


        /**
        * Draw a rectangle with size (w,h) and upperleft corner at (x,y)
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        DEPRECATED_SCALAR_PARAMS void drawRect(int x, int y, int w, int h, color_t color, float opacity)
            {
            drawRect({ x, y }, { w, h }, color, opacity);
            }


        /**
         * Fill a rectangle region with a single color
         *
         * @param   B       Box representing the rectangle
         * @param   color   The color to use
        **/
        void fillRect(iBox2 B, color_t color);


        /**
         * Fill a rectangle region with a single color
         *
         * @param   upper_left_pos  Position of the upper left corner of the rectangle.
         * @param   dimension       (width,height) of the rectangle.
         * @param   color           color to use.
        **/
        void fillRect(iVec2 upper_left_pos, iVec2 dimension, color_t color)
            {
            fillRect(iBox2(upper_left_pos.x, upper_left_pos.x + dimension.x - 1, upper_left_pos.y, upper_left_pos.y + dimension.y - 1), color);
            }


        /**
        * Fill a rectangle region with a single color
        **/
        DEPRECATED_SCALAR_PARAMS void fillRect(int x, int y, int w, int h, color_t color)
            {
            fillRect(iBox2( x, x + w - 1, y, y + h - 1 ), color);
            }



        /**
         * Fill a rectangle region with a single color
         * 
         * Blend with the current color background using opacity between 0.0f (fully transparent) and
         * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
         *
         * @param   B       An iBox2 to process.
         * @param   color   The color.
         * @param   opacity The opacity.
        **/
        void fillRect(iBox2 B, color_t color, float opacity);


        /**
         * Fill a rectangle region with a single color
         * 
         * Blend with the current color background using opacity between 0.0f (fully transparent) and
         * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
         *
         * @param   upper_left_pos  Position of the upper left corner of the rectangle.
         * @param   dimension       (width,height) of the rectangle.
         * @param   color           color to use.
         * @param   opacity         between 0.0f (transparent) to 1.0f (fully opaque).
        **/
        void fillRect(iVec2 upper_left_pos, iVec2 dimension, color_t color, float opacity)
            {
            fillRect(iBox2(upper_left_pos.x, upper_left_pos.x + dimension.x - 1, upper_left_pos.y, upper_left_pos.y + dimension.y - 1), color, opacity);
            }


        /**
         * Fill a rectangle region with a single color
         * 
         * Blend with the current color background using opacity between 0.0f (fully transparent) and
         * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
         *
        **/
        DEPRECATED_SCALAR_PARAMS void fillRect(int x, int y, int w, int h, color_t color, float opacity)
            {
            fillRect(iBox2( x, x + w - 1, y, y + h - 1 ), color, opacity);
            }


        /**
         * Fill a rectangle with different colors for the outline and the interior
         *
         * @param   B               box representing the rectangle
         * @param   color_interior  color for the interior
         * @param   color_outline   color for the outline
        **/
        void fillRect(iBox2 B, color_t color_interior, color_t color_outline)
            {
            drawRect(B, color_outline);
            fillRect(iBox2(B.minX + 1, B.maxX - 1, B.minY + 1, B.maxY - 1), color_interior);
            }


        /**
         * Fill a rectangle with different colors for the outline and the interior
         *
         * @param   upper_left_pos  upper left corner of the rectangle
         * @param   dimension       (width, height) of the rectangle
         * @param   color_interior  color for the interior.
         * @param   color_outline   color for the outline.
        **/
        void fillRect(iVec2 upper_left_pos, iVec2 dimension, color_t color_interior, color_t color_outline)
            {
            fillRect(iBox2(upper_left_pos.x, upper_left_pos.x + dimension.x - 1, upper_left_pos.y, upper_left_pos.y + dimension.y - 1), color_interior, color_outline);
            }


        /**
        * Fill a rectangle with different colors for the outline and the interior
        **/
        DEPRECATED_SCALAR_PARAMS void fillRect(int x, int y, int w, int h, color_t color_interior, color_t color_outline)
            {
            fillRect(iBox2( x, x + w - 1, y, y + h - 1 ), color_interior, color_outline);
            }


        /**
         * Fill a rectangle with different colors for the outline and the interior
         * 
         * Blend with the current color background using opacity between 0.0f (fully transparent) and
         * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
         *
         * @param   B               box representing the rectangle.
         * @param   color_interior  color for the interior.
         * @param   color_outline   color for the outline.
         * @param   opacity         opacity between 0.0f (transparent) and 1.0f (opaque).
        **/
        void fillRect(iBox2 B, color_t color_interior, color_t color_outline, float opacity)
            {
            drawRect(B, color_outline, opacity);
            fillRect(iBox2(B.minX + 1, B.maxX - 1, B.minY + 1, B.maxY - 1), color_interior, opacity);
            }


        /**
         * Fill a rectangle with different colors for the outline and the interior
         * 
         * Blend with the current color background using opacity between 0.0f (fully transparent) and
         * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
         *
         * @param   upper_left_pos  upper left corner of the rectangle.
         * @param   dimension       (width, height) of the rectangle.
         * @param   color_interior  color for the interior.
         * @param   color_outline   color for the outline.
         * @param   opacity         opacity between 0.0f (transparent) and 1.0f (opaque).
        **/
        void fillRect(iVec2 upper_left_pos, iVec2 dimension, color_t color_interior, color_t color_outline, float opacity)
            {
            fillRect(iBox2(upper_left_pos.x, upper_left_pos.x + dimension.x - 1, upper_left_pos.y, upper_left_pos.y + dimension.y - 1), color_interior, color_outline, opacity);
            }


        /**
        * Fill a rectangle with different colors for the outline and the interior
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        DEPRECATED_SCALAR_PARAMS void fillRect(int x, int y, int w, int h, color_t color_interior, color_t color_outline, float opacity)
            {
            fillRect(iBox2( x, x + w - 1, y, y + h - 1 ), color_interior, color_outline, opacity);
            }



        /**
        * Draw a rounded rectangle in box B with corner radius r.
        **/
        void drawRoundRect(const iBox2 & B, int r, color_t color)
            {
            const int x = B.minX;
            const int y = B.minY;
            const int w = B.lx();
            const int h = B.ly();
            if (!isValid() || (w <= 0) || (h <= 0)) return;
            if ((x >= 0) && (x + w < _lx) && (y >= 0) && (y + h < _ly))
                _drawRoundRect<false>(x, y, w, h, r, color);
            else
                _drawRoundRect<true>(x, y, w, h, r, color);
            }


        /**
        * Draw a rounded rectangle with upper left corner (x,y), width w and 
        * height h and with corner radius r.
        **/
        DEPRECATED_SCALAR_PARAMS void drawRoundRect(int x, int y, int w, int h, int r, color_t color)
            {
            drawRoundRect(iBox2{ x, x + w - 1 ,y, y + h - 1 }, r, color);
            }


        /**
        * Draw a rounded rectangle in box B with corner radius r.
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        void drawRoundRect(const iBox2 & B, int r, color_t color, float opacity)
            {
            const int x = B.minX;
            const int y = B.minY;
            const int w = B.lx();
            const int h = B.ly();
            if (!isValid() || (w <= 0) || (h <= 0)) return;
            if ((x >= 0) && (x + w < _lx) && (y >= 0) && (y + h < _ly))
                _drawRoundRect<false>(x, y, w, h, r, color, opacity);
            else
                _drawRoundRect<true>(x, y, w, h, r, color, opacity);
            }


        /**
        * Draw a rounded rectangle with upper left corner (x,y), width w and 
        * height h and with corner radius r.
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        DEPRECATED_SCALAR_PARAMS void drawRoundRect(int x, int y, int w, int h, int r, color_t color, float opacity)
            {
            drawRoundRect(iBox2{ x, x + w - 1 ,y, y + h - 1 }, r, color, opacity);
            }


        /**
        * Draw a filled rounded rectangle in box B with corner radius r.
        **/
        void fillRoundRect(const iBox2 & B, int r, color_t color)
            {   
            const int x = B.minX;
            const int y = B.minY;
            const int w = B.lx();
            const int h = B.ly();
            if (!isValid() || (w <= 0) || (h <= 0)) return;
            if ((x >= 0) && (x + w < _lx) && (y >= 0) && (y + h < _ly))
                _fillRoundRect<false>(x, y, w, h, r, color);
            else
                _fillRoundRect<true>(x, y, w, h, r, color);
            }


        /**
        * Draw a filled rounded rectangle with upper left corner (x,y), width w and
        * height h and with corner radius r.
        **/
        DEPRECATED_SCALAR_PARAMS void fillRoundRect(int x, int y, int w, int h, int r, color_t color)
            {
            fillRoundRect(iBox2{ x, x + w - 1 ,y, y + h - 1 }, r, color);
            }


        /**
        * Draw a filled rounded rectangle in box B with corner radius r.
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        void fillRoundRect(const iBox2& B, int r, color_t color, float opacity)
            {
            const int x = B.minX;
            const int y = B.minY;
            const int w = B.lx();
            const int h = B.ly();
            if (!isValid() || (w <= 0) || (h <= 0)) return;
            if ((x >= 0) && (x + w < _lx) && (y >= 0) && (y + h < _ly))
                _fillRoundRect<false>(x, y, w, h, r, color, opacity);
            else
                _fillRoundRect<true>(x, y, w, h, r, color, opacity);
            }


        /**
        * Draw a filled rounded rectangle with upper left corner (x,y), width w and 
        * height h and with corner radius r.
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        DEPRECATED_SCALAR_PARAMS void fillRoundRect(int x, int y, int w, int h, int r, color_t color, float opacity)
            {
            fillRoundRect(iBox2{ x, x + w - 1 ,y, y + h - 1 }, r, color, opacity);
            }



        /**
        * Fill a rectangle region with a horizontal color gradient between color1 and color2.
        * color interpolation takes place in RGB color space (even if color_t is HSV).
        **/
        void fillRectHGradient(iBox2 B, color_t color1, color_t color2);


        /**
        * Fill a rectangle region with a horizontal color gradient between color1 and color2.
        * color interpolation takes place in RGB color space (even if color_t is HSV).
        **/
        DEPRECATED_SCALAR_PARAMS void fillRectHGradient(int x, int y, int w, int h, color_t color1, color_t color2)
            {
            fillRectHGradient(iBox2(x, x + w - 1, y, y + h - 1), color1, color2);
            }


        /**
        * Fill a rectangle region with a horizontal color gradient between color1 and color2.
        * color interpolation takes place in RGB color space (even if color_t is HSV).
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        void fillRectHGradient(iBox2 B, color_t color1, color_t color2, float opacity);


        /**
        * Fill a rectangle region with a horizontal color gradient between color1 and color2.
        * color interpolation takes place in RGB color space (even if color_t is HSV).
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        DEPRECATED_SCALAR_PARAMS void fillRectHGradient(int x, int y, int w, int h, color_t color1, color_t color2, float opacity)
            {
            fillRectHGradient(iBox2(x, x + w - 1, y, y + h - 1), color1, color2, opacity);
            }


        /**
        * Fill a rectangle region with a vertical color gradient between color1 and color2.
        * color interpolation takes place in RGB color space (even if color_t is HSV).
        **/
        void fillRectVGradient(iBox2 B, color_t color1, color_t color2);


        /**
        * Fill a rectangle region with a vertical color gradient between color1 and color2.
        * color interpolation takes place in RGB color space (even if color_t is HSV).
        **/
        DEPRECATED_SCALAR_PARAMS void fillRectVGradient(int x, int y, int w, int h, color_t color1, color_t color2)
            {
            fillRectVGradient(iBox2(x, x + w - 1, y, y + h - 1), color1, color2);
            }


        /**
        * Fill a rectangle region with a vertical color gradient between color1 and color2.
        * color interpolation takes place in RGB color space (even if color_t is HSV).
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        void fillRectVGradient(iBox2 B, color_t color1, color_t color2, float opacity);


        /**
        * Fill a rectangle region with a vertical color gradient between color1 and color2.
        * color interpolation takes place in RGB color space (even if color_t is HSV).
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        DEPRECATED_SCALAR_PARAMS void fillRectVGradient(int x, int y, int w, int h, color_t color1, color_t color2, float opacity)
            {
            fillRectVGradient(iBox2(x, x + w - 1, y, y + h - 1), color1, color2, opacity);
            }






        /*********************************************************************
        *
        * drawing circles
        *
        **********************************************************************/


        /**
        * Draw a circle (the outline but not its interior). 
        **/
        void drawCircle(iVec2 center, int r, color_t color)
            {
            if ((center.x - r >= 0) && (center.x + r < _lx) && (center.y - r >= 0) && (center.y + r < _ly))
                _drawFilledCircle<true, false, false>(center.x, center.y, r, color, color);
            else
                _drawFilledCircle<true, false, true>(center.x, center.y, r, color, color);
            }


        /**
        * Draw a circle (the outline but not its interior). 
        **/
        DEPRECATED_SCALAR_PARAMS void drawCircle(int cx, int cy, int r, color_t color)
            {
            drawCircle(iVec2(cx,cy), r, color);
            }


        /**
        * Draw a circle (the outline but not its interior). 
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        void drawCircle(iVec2 center, int r, color_t color, float opacity)
            {
            if ((center.x - r >= 0) && (center.x + r < _lx) && (center.y - r >= 0) && (center.y + r < _ly))
                _drawFilledCircle<true, false, false>(center.x, center.y, r, color, color, opacity);
            else
                _drawFilledCircle<true, false, true>(center.x, center.y, r, color, color, opacity);
            }


        /**
        * Draw a circle (the outline but not its interior). 
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        DEPRECATED_SCALAR_PARAMS void drawCircle(int cx, int cy, int r, color_t color, float opacity)
            {
            drawCircle(iVec2(cx,cy), r, color, opacity);
            }


        /**
        * Draw both the outline and interior of a circle (possibly with distinct colors).
        **/
        void fillCircle(iVec2 center, int r, color_t interior_color, color_t outline_color)
            {
            if ((center.x - r >= 0) && (center.x + r < _lx) && (center.y - r >= 0) && (center.y + r < _ly))
                _drawFilledCircle<true, true, false>(center.x, center.y, r, outline_color, interior_color);
            else
                _drawFilledCircle<true, true, true>(center.x, center.y, r, outline_color, interior_color);
            }


        /**
        * Draw both the outline and interior of a circle (possibly with distinct colors).
        **/
        DEPRECATED_SCALAR_PARAMS void fillCircle(int cx, int cy, int r, color_t interior_color, color_t outline_color)
            {
            fillCircle(iVec2(cx, cy), r, interior_color, outline_color);
            }


        /**
        * Draw both the outline and interior of a circle (possibly with distinct colors).
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        void fillCircle(iVec2 center, int r, color_t interior_color, color_t outline_color, float opacity)
            {
            if ((center.x - r >= 0) && (center.x + r < _lx) && (center.y - r >= 0) && (center.y + r < _ly))
                _drawFilledCircle<true, true, false>(center.x, center.y, r, outline_color, interior_color, opacity);
            else
                _drawFilledCircle<true, true, true>(center.x, center.y, r, outline_color, interior_color, opacity);
            }


        /**
        * Draw both the outline and interior of a circle (possibly with distinct colors).
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        DEPRECATED_SCALAR_PARAMS void fillCircle(int cx, int cy, int r, color_t interior_color, color_t outline_color, float opacity)
            {
            fillCircle(iVec2(cx, cy), r, interior_color, outline_color, opacity);
            }



        /**
        * Draw the outline of an ellipse (but not its interior).
        **/
        void drawEllipse(iVec2 center, iVec2 radiuses, color_t color)
            {
            const int cx = center.x;
            const int cy = center.y;
            const int rx = radiuses.x;
            const int ry = radiuses.y;
            if ((cx - rx >= 0) && (cx + rx < _lx) && (cy - ry >= 0) && (cy + ry < _ly))
                _drawEllipse<true, false, false>(cx, cy, rx, ry, color, color);
            else
                _drawEllipse<true, false, true>(cx, cy, rx, ry, color, color);
            }


        /**
        * Draw the outline of an ellipse (but not its interior).
        **/
        DEPRECATED_SCALAR_PARAMS void drawEllipse(int cx, int cy, int rx, int ry, color_t color)
            {
            drawEllipse({ cx, cy }, { rx, ry }, color, color);
            }


        /**
        * Draw the outline of an ellipse (but not its interior).
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        void drawEllipse(iVec2 center, iVec2 radiuses, color_t color, float opacity)
            {
            const int cx = center.x;
            const int cy = center.y;
            const int rx = radiuses.x;
            const int ry = radiuses.y;
            if ((cx - rx >= 0) && (cx + rx < _lx) && (cy - ry >= 0) && (cy + ry < _ly))
                _drawEllipse<true, false, false>(cx, cy, rx , ry, color, color, opacity);
            else
                _drawEllipse<true, false, true>(cx, cy, rx, ry, color, color, opacity);
            }

        /**
        * Draw the outline of an ellipse (but not its interior).
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        DEPRECATED_SCALAR_PARAMS void drawEllipse(int cx, int cy, int rx, int ry, color_t color, float opacity)
            {
            drawEllipse({ cx, cy }, { rx, ry }, color, color, opacity);
            }


        /**
        * Draw both the outline and interior of an ellipse (possibly with distinct colors).
        **/
        void fillEllipse(iVec2 center, iVec2 radiuses, color_t interior_color, color_t outline_color)
            {
            const int cx = center.x;
            const int cy = center.y;
            const int rx = radiuses.x;
            const int ry = radiuses.y;
            if ((cx - rx >= 0) && (cx + rx < _lx) && (cy - ry >= 0) && (cy + ry < _ly))
                _drawEllipse<true, true, false>(cx, cy, rx, ry, outline_color, interior_color);
            else
                _drawEllipse<true, true, true>(cx, cy, rx, ry, outline_color, interior_color);
            }


        /**
        * Draw both the outline and interior of an ellipse (possibly with distinct colors).
        **/
        DEPRECATED_SCALAR_PARAMS void fillEllipse(int cx, int cy, int rx, int ry, color_t interior_color, color_t outline_color)
            {
            fillEllipse({ cx, cy }, { rx, ry }, interior_color, outline_color);
            }


        /**
        * Draw both the outline and interior of an ellipse (possibly with distinct colors).
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        void fillEllipse(iVec2 center, iVec2 radiuses, color_t interior_color, color_t outline_color, float opacity)
            {
            const int cx = center.x;
            const int cy = center.y;
            const int rx = radiuses.x;
            const int ry = radiuses.y;
            if ((cx - rx >= 0) && (cx + rx < _lx) && (cy - ry >= 0) && (cy + ry < _ly))
                _drawEllipse<true, true, false>(cx, cy, rx, ry, outline_color, interior_color, opacity);
            else
                _drawEllipse<true, true, true>(cx, cy, rx, ry, outline_color, interior_color, opacity);
            }


        /**
        * Draw both the outline and interior of an ellipse (possibly with distinct colors).
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        **/
        DEPRECATED_SCALAR_PARAMS void fillEllipse(int cx, int cy, int rx, int ry, color_t interior_color, color_t outline_color, float opacity)
            {
            fillEllipse({ cx, cy }, { rx, ry }, interior_color, outline_color, opacity);
            }




        /************************************************************************************
        *
        *
        * New high quality drawing methods: anti-aliasing, bilinear filtering, subpixel precison...
        *
        *
        *************************************************************************************/


        /**
         * Draw an anti-aliased wide line from PA to PB with thickness wd and radiuses ends. Use
         * floating point values for sub-pixel precision.
         * 
         * Blend with the current color background using opacity between 0.0f (fully transparent) and
         * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
         * 
         * CREDIT: Bodmer TFT_eSPI library : https://github.com/Bodmer/TFT_eSPI
         *
         * @param   PA      first end point.
         * @param   PB      second end point.
         * @param   wd      thickness.
         * @param   color   color.
         * @param   opacity opacity between 0.0f and 1.0f.
        **/
        void drawWideLine(fVec2 PA, fVec2 PB, float wd, color_t color, float opacity)
            {
            _drawWideLine(PA.x, PA.y, PB.x, PB.y, wd, color, opacity);
            }


        /**
        * Draw an anti-aliased wide line from (ax,ay) to (bx,by) with thickness wd and radiuses ends.
        * Use floating point values for sub-pixel precision.
        *
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        *
        * CREDIT: Bodmer TFT_eSPI library : https://github.com/Bodmer/TFT_eSPI
        **/
        DEPRECATED_SCALAR_PARAMS void drawWideLine(float ax, float ay, float bx, float by, float wd, color_t color, float opacity)
            {
            _drawWideLine(ax, ay , bx, by , wd, color, opacity);
            }


        /**
         * Draw an anti-aliased wedge line from PA to PB with respective wideness aw and bw at the ends.
         * Use floating point values for sub-pixel precision.
         * 
         * Blend with the current color background using opacity between 0.0f (fully transparent) and
         * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
         * 
         * CREDIT: Bodmer TFT_eSPI library : https://github.com/Bodmer/TFT_eSPI
         *
         * @param   PA      first end point
         * @param   PB      second end point
         * @param   aw      radius at the first end point
         * @param   bw      radius at the second end point
         * @param   color   color.
         * @param   opacity opacity between 0.0f and 1.0f.
        **/
        void drawWedgeLine(fVec2 PA, fVec2 PB, float aw, float bw, color_t color, float opacity)
            {
            _drawWedgeLine(PA.x, PA.y, PB.x, PB.y, aw, bw, color, opacity);
            }


        /**
        * Draw an anti-aliased wedge line from (ax,ay) to (bx,by) with respective wideness aw and bw at the ends.
        * Use floating point values for sub-pixel precision. 
        * 
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        *
        * CREDIT: Bodmer TFT_eSPI library : https://github.com/Bodmer/TFT_eSPI
        **/
        DEPRECATED_SCALAR_PARAMS void drawWedgeLine(float ax, float ay, float bx, float by, float aw, float bw, color_t color, float opacity)
            {
            _drawWedgeLine(ax, ay, bx, by, aw, bw, color, opacity);
            }


        /**
         * Draw an anti-aliased filled circle. Use floating point value for sub-pixel precision.
         * 
         * Blend with the current color background using opacity between 0.0f (fully transparent) and
         * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
         * 
         * CREDIT: Bodmer TFT_eSPI library : https://github.com/Bodmer/TFT_eSPI
         *
         * @param   center  center point
         * @param   r       radius
         * @param   color   color.
         * @param   opacity opacity between 0.0f and 1.0f.
        **/
        void drawSpot(fVec2 center, float r, color_t color, float opacity)
            {
            // Filled circle can be created by the wide line function with length zero
            _drawWideLine(center.x, center.y, center.x, center.y, 2.0f * r, color, opacity);
            }


        /**
        * Draw an anti-aliased filled circle. Use floating point value for sub-pixel precision.
        * 
        * Blend with the current color background using opacity between 0.0f (fully transparent) and
        * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
        *
        * CREDIT: Bodmer TFT_eSPI library : https://github.com/Bodmer/TFT_eSPI
        **/
        DEPRECATED_SCALAR_PARAMS void drawSpot(float ax, float ay, float r, color_t color, float opacity)
            {
            // Filled circle can be created by the wide line function with length zero
            _drawWideLine(ax, ay, ax, ay, 2.0f * r, color, opacity);
            }


        /**
         * Draw a triangle with gradient color specified by the colors on its vertices.
         * 
         * Remarks:
         * 
         * 1. The positions are given using floating point values to allow for sub-pixel precision (for
         * smoother animation).
         * 3. The color type can be different from that of this image.
         * 4. This version does NOT use blending: pixel from the source are simply copied over this
         * image.
         *
         * @tparam  color_alt   Type of the color alternate.
         * @param   P1      First triangle vertex.
         * @param   P2      Second triangle vertex.
         * @param   P3      Third triangle vertex.
         * @param   colorP1 color at the first vertex.
         * @param   colorP2 color at the second vertex.
         * @param   colorP3 color at the third vertex.
        **/
        template<typename color_alt>
        void drawGradientTriangle(fVec2 P1, fVec2 P2, fVec2 P3, color_alt colorP1, color_alt colorP2, color_alt colorP3)
            {
            _drawGradientTriangle<color_alt, false>(P1, P2, P3, colorP1, colorP2, colorP3, 1.0f);
            }


        /**
         * Blend a triangle with gradient color specified by the colors on its vertices.
         * 
         * Remarks:
         * 
         * 1. The positions are given using floating point values to allow for sub-pixel precision (for
         * smoother animation).
         * 3. The color type can be different from that of this image.
         * 4. This version uses blending, the color gradient color is blended over this image using the  
         * alpha channel if available and multiplied by the opacity factor
         *
         * @tparam  color_alt   Type of the color alternate.
         * @param   P1      First triangle vertex.
         * @param   P2      Second triangle vertex.
         * @param   P3      Third triangle vertex.
         * @param   colorP1 color at the first vertex.
         * @param   colorP2 color at the second vertex.
         * @param   colorP3 color at the third vertex.
         * @param   opacity The opacity multiplier between 0.0f (transparent) and 1.0f (opaque).
        **/
        template<typename color_alt>
        void drawGradientTriangle(fVec2 P1, fVec2 P2, fVec2 P3, color_alt colorP1, color_alt colorP2, color_alt colorP3, float opacity)
            {
            _drawGradientTriangle<color_alt, true>(P1, P2, P3, colorP1, colorP2, colorP3, opacity);
            }



        /**
         * Draw textured triangle (does not use blending).
         * 
         * Remarks:
         * 
         * 1. The positions are given using floating point values to allow for sub-pixel precision (for
         * smoother animation).
         * 2. The method use bilinear interpolation for high quality rendering.
         * 3. The sprite image can have a different color type from this image.
         * 4. This version does NOT use blending: pixel from the source are simply copied over this
         * image.
         *
         * IMPORTANT WARNING !
         *
         * For certain orientation of the triangle, access to the texture pixels is highly non-linear.
         * For texture store in flash memory, this can cause dramatic slowdown because the read cache
         * becomes basically useless. To overcome this problem, two solutions are possible:
         *
         * a. Move the texture to a faster memory location before drawing.
         *
         * b. Tessellate the triangle into smaller triangles so that each the memory data for each
         *    triangle can fit into the cache memory. Note that doing so will note cause any artifact
         *    because the triangle rasterizer is "pixel perfect" so no pixel will be written twice.
         *    However, all the small triangles must be given in the same winding direction !
         *
         * @param   src_im  the image/texture to map onto the triangle.
         * @param   srcP1   coords of point 1 on the texture.
         * @param   srcP2   coords of point 2 on the texture.
         * @param   srcP3   coords of point 3 on the texture.
         * @param   dstP1   coords of point 1 on this image.
         * @param   dstP2   coords of point 2 on this image.
         * @param   dstP3   coords of point 3 on this image.
        **/
        template<typename color_t_tex>
        void drawTexturedTriangle(const Image<color_t_tex>& src_im, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3)
            {
            _drawTexturedTriangle<color_t_tex, false, false, false>(src_im, color_t_tex(), srcP1, srcP2, srcP3, dstP1, dstP2, dstP3, color_t_tex(), color_t_tex(), color_t_tex(), 1.0f);
            }


        /**
         * Blend a textured triangle over this image.
         * 
         * Remarks:
         * 
         * 1. The positions are given using floating point values to allow for sub-pixel precision (for
         * smoother animation).
         * 2. The method use bilinear interpolation for high quality rendering.
         * 3. The sprite image can have a different color type from this image.
         * 4. This version use blending and opacity.If the sprite image color type has an alpha channel,
         * then it is used for blending and multiplied by the opacity factor(even if this image does not
         * have an alpha channel).           *
         * 
         * IMPORTANT WARNING !
         * 
         * For certain orientation of the triangle, access to the texture pixels is highly non-linear.
         * For texture store in flash memory, this can cause dramatic slowdown because the read cache
         * becomes basically useless. To overcome this problem, two solutions are possible:
         * 
         * a. Move the texture to a faster memory location before drawing.
         * 
         * b. Tessellate the triangle into smaller triangles so that each the memory data for each  
         *    triangle can fit into the cache memory. Note that doing so will note cause any artifact
         *    because the triangle rasterizer is "pixel perfect" so no pixel will be written twice.
         *    However, all the small triangles must be given in the same winding direction !
         *
         * @tparam  color_t_tex Type of the color t tex.
         * @param   src_im  the image/texture to map onto the triangle.
         * @param   srcP1   coords of point 1 on the texture.
         * @param   srcP2   coords of point 2 on the texture.
         * @param   srcP3   coords of point 3 on the texture.
         * @param   dstP1   coords of point 1 on this image.
         * @param   dstP2   coords of point 2 on this image.
         * @param   dstP3   coords of point 3 on this image.
         * @param   opacity The opacity multiplier between 0.0f (transparent) and 1.0f (opaque).
        **/
        template<typename color_t_tex>
        void drawTexturedTriangle(const Image<color_t_tex>& src_im, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, float opacity)
            {
            _drawTexturedTriangle<color_t_tex, false, true, false>(src_im, color_t_tex(), srcP1, srcP2, srcP3, dstP1, dstP2, dstP3, color_t_tex(), color_t_tex(), color_t_tex(), opacity);
            }
 

        /**
         * Blend a textured triangle over this image using a custom blend operator to combine to color 
         * from the sprite with the color of this image. 
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
         * must return a valid color (of any type but preferably of type color_t for best performance).         * 
         * 
         * Remarks:
         * 
         * 1. The positions are given using floating point values to allow for sub-pixel precision (for
         * smoother animation).
         * 2. The method use bilinear interpolation for high quality rendering.
         * 3. The sprite image can have a different color type from this image.  
         * 4. To perform "classical alpha blending", use the blitScaleRotated() method with an opacity
         *    parameter instead as it will be faster.
         * 
         * IMPORTANT WARNING !
         * 
         * For certain orientation of the triangle, access to the texture pixels is highly non-linear.
         * For texture store in flash memory, this can cause dramatic slowdown because the read cache
         * becomes basically useless. To overcome this problem, two solutions are possible:
         * 
         * a. Move the texture to a faster memory location before drawing.
         * 
         * b. Tessellate the triangle into smaller triangles so that each the memory data for each  
         *    triangle can fit into the cache memory. Note that doing so will note cause any artifact
         *    because the triangle rasterizer is "pixel perfect" so no pixel will be written twice.
         *    However, all the small triangles must be given in the same winding direction !
         *
         * @tparam  color_t_tex Type of the color t tex.
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
         * Draw textured triangle combined with a color gradient (does not use blending).
         * 
         * Remarks:
         * 
         * 1. The positions are given using floating point values to allow for sub-pixel precision (for
         * smoother animation).
         * 2. The method use bilinear interpolation for high quality rendering.
         * 3. The sprite image can have a different color type from this image.
         * 4. This version does NOT use blending: pixel from the source are simply copied over this
         * image.
         *
         * IMPORTANT WARNING !
         *
         * For certain orientation of the triangle, access to the texture pixels is highly non-linear.
         * For texture store in flash memory, this can cause dramatic slowdown because the read cache
         * becomes basically useless. To overcome this problem, two solutions are possible:
         *
         * a. Move the texture to a faster memory location before drawing.
         *
         * b. Tessellate the triangle into smaller triangles so that each the memory data for each
         *    triangle can fit into the cache memory. Note that doing so will note cause any artifact
         *    because the triangle rasterizer is "pixel perfect" so no pixel will be written twice.
         *    However, all the small triangles must be given in the same winding direction !
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
        **/
        template<typename color_t_tex>
        void drawTexturedTriangleGradient(const Image<color_t_tex>& src_im, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, color_t_tex C1, color_t_tex C2, color_t_tex C3)
            {
            _drawTexturedTriangle<color_t_tex, true, false, false>(src_im, color_t_tex(), srcP1, srcP2, srcP3, dstP1, dstP2, dstP3, C1, C2, C3, 1.0f);
            }


        /**
         * Blend textured triangle combined with a color gradient (use blending).
         * 
         * Remarks:
         * 
         * 1. The positions are given using floating point values to allow for sub-pixel precision (for
         * smoother animation).
         * 2. The method use bilinear interpolation for high quality rendering.
         * 3. The sprite image can have a different color type from this image.
         * 4. This version use blending and opacity.If the sprite image color type has an alpha channel,
         * then it is used for blending and multiplied by the opacity factor(even if this image does not
         * have an alpha channel).           *
         *
         * IMPORTANT WARNING !
         *
         * For certain orientation of the triangle, access to the texture pixels is highly non-linear.
         * For texture store in flash memory, this can cause dramatic slowdown because the read cache
         * becomes basically useless. To overcome this problem, two solutions are possible:
         *
         * a. Move the texture to a faster memory location before drawing.
         *
         * b. Tessellate the triangle into smaller triangles so that each the memory data for each
         *    triangle can fit into the cache memory. Note that doing so will note cause any artifact
         *    because the triangle rasterizer is "pixel perfect" so no pixel will be written twice.
         *    However, all the small triangles must be given in the same winding direction !
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
         * @param   opacity global opacity multiplier between 0.0f (transparent) and 1.0f (opaque).
        **/
        template<typename color_t_tex>
        void drawTexturedTriangleGradient(const Image<color_t_tex>& src_im, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, color_t_tex C1, color_t_tex C2, color_t_tex C3, float opacity)
            {
            _drawTexturedTriangle<color_t_tex, true, true, false>(src_im, color_t_tex(), srcP1, srcP2, srcP3, dstP1, dstP2, dstP3, C1, C2, C3, opacity);
            }



        /**
         * Blend textured triangle with a transparency mask (i.e. a specific color is treated as fully 
         * opaque, which is useful for drawing sprite when the source image color type does not have an 
         * alpha channel).
         * 
         * Remarks:
         * 
         * 1. The positions are given using floating point values to allow for sub-pixel precision (for
         * smoother animation).
         * 2. The method use bilinear interpolation for high quality rendering.
         * 3. The sprite image can have a different color type from this image.
         * 4. This version use blending (if src_im has an alpha channel).
         *
         * IMPORTANT WARNING !
         *
         * For certain orientation of the triangle, access to the texture pixels is highly non-linear.
         * For texture store in flash memory, this can cause dramatic slowdown because the read cache
         * becomes basically useless. To overcome this problem, two solutions are possible:
         *
         * a. Move the texture to a faster memory location before drawing.
         *
         * b. Tessellate the triangle into smaller triangles so that each the memory data for each
         *    triangle can fit into the cache memory. Note that doing so will note cause any artifact
         *    because the triangle rasterizer is "pixel perfect" so no pixel will be written twice.
         *    However, all the small triangles must be given in the same winding direction !
         *
         * @param   src_im              the image/texture to map onto the triangle.
         * @param   transparent_color   the color considered transparent in the source texture.
         * @param   srcP1               coords of point 1 on the texture.
         * @param   srcP2               coords of point 2 on the texture.
         * @param   srcP3               coords of point 3 on the texture.
         * @param   dstP1               coords of point 1 on this image.
         * @param   dstP2               coords of point 2 on this image.
         * @param   dstP3               coords of point 3 on this image.
         * @param   opacity             The opacity multiplier between 0.0f (transparent) and 1.0f
         *                              (opaque).
        **/
        template<typename color_t_tex>
        void drawTexturedTriangleMasked(const Image<color_t_tex>& src_im, color_t_tex transparent_color, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, float opacity)
            {
            _drawTexturedTriangle<color_t_tex,false,true,true>(src_im, transparent_color, srcP1, srcP2, srcP3, dstP1, dstP2, dstP3, color_t_tex(), color_t_tex(), color_t_tex(), opacity);
            }


        /**
         * Blend textured triangle with a transparency mask (i.e. a specific color is treated as fully
         * opaque, which is useful for drawing sprite when the source image color type does not have an
         * alpha channel). Combine the drawing with a color gradient.
         * 
         * Remarks:
         * 
         * 1. The positions are given using floating point values to allow for sub-pixel precision (for
         *    smoother animation).
         * 2. The method use bilinear interpolation for high quality rendering.
         * 3. The sprite image can have a different color type from this image.
         * 4. This version use blending (if src_im has an alpha channel).
         *
         * IMPORTANT WARNING !
         *
         * For certain orientation of the triangle, access to the texture pixels is highly non-linear.
         * For texture store in flash memory, this can cause dramatic slowdown because the read cache
         * becomes basically useless. To overcome this problem, two solutions are possible:
         *
         * a. Move the texture to a faster memory location before drawing.
         *
         * b. Tessellate the triangle into smaller triangles so that each the memory data for each
         *    triangle can fit into the cache memory. Note that doing so will note cause any artifact
         *    because the triangle rasterizer is "pixel perfect" so no pixel will be written twice.
         *    However, all the small triangles must be given in the same winding direction !
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
         * @param   opacity             The opacity multiplier between 0.0f (transparent) and 1.0f
         *                              (opaque).
        **/
        template<typename color_t_tex>
        void drawTexturedTriangleMaskedGradient(const Image<color_t_tex>& src_im, color_t_tex transparent_color, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, color_t_tex C1, color_t_tex C2, color_t_tex C3, float opacity)
            {
            _drawTexturedTriangle<color_t_tex, true, true, true>(src_im, transparent_color, srcP1, srcP2, srcP3, dstP1, dstP2, dstP3, C1, C2, C3, opacity);
            }



        /**
        * Draw a quad with a gradient color specified by the color at the four vertices. 
        * Does not use blending
        *
        * See drawGradientTriangle() for details.
        *
        * NOTE: the vertices can be given in clockwise or counter-clockwise order.
        **/
        template<typename color_alt>
        void drawGradientQuad(fVec2 P1, fVec2 P2,  fVec2 P3, fVec2 P4, color_alt colorP1, color_alt colorP2, color_alt colorP3, color_alt colorP4)
            {
            drawGradientTriangle(P1, P2, P3, colorP1, colorP2, colorP3);
            drawGradientTriangle(P1, P3, P4, colorP1, colorP3, colorP4);
            }


        /**
        * Draw a quad with a gradient color specified by the color at the four vertices. 
        * Use blending
        *
        * See drawGradientTriangle() for details.
        *
        * NOTE: the vertices can be given in clockwise or counter-clockwise order.
        **/
        template<typename color_alt>
        void drawGradientQuad(fVec2 P1, fVec2 P2, fVec2 P3, fVec2 P4, color_alt colorP1, color_alt colorP2, color_alt colorP3, color_alt colorP4, float opacity)
            {
            drawGradientTriangle(P1, P2, P3, colorP1, colorP2, colorP3, opacity);
            drawGradientTriangle(P1, P3, P4, colorP1, colorP3, colorP4, opacity);
            }


        /**
        * Draw a textured quad (no blending).
        * 
        * See drawTexturedTriangle() for details.
        * 
        * NOTE: the vertices can be given either in clockwise or counter-clockwise order.
        **/
        template<typename color_t_tex>
        void drawTexturedQuad(const Image<color_t_tex> & src_im, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 srcP4, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, fVec2 dstP4)
            {
            drawTexturedTriangle(src_im, srcP1, srcP2, srcP3, dstP1, dstP2, dstP3);
            drawTexturedTriangle(src_im, srcP1, srcP3, srcP4, dstP1, dstP3, dstP4);
            }


        /**
        * Draw a textured quad (with alpha blending).
        *
        * See drawTexturedTriangle() for details.
        *
        * NOTE: the vertices can be given either in clockwise or counter-clockwise order.
        **/
        template<typename color_t_tex>
        void drawTexturedQuad(const Image<color_t_tex> & src_im, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 srcP4, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, fVec2 dstP4, float opacity)
            {
            drawTexturedTriangle(src_im, srcP1, srcP2, srcP3, dstP1, dstP2, dstP3, opacity);
            drawTexturedTriangle(src_im, srcP1, srcP3, srcP4, dstP1, dstP3, dstP4, opacity);
            }


        /**
        * Draw a textured quad with a custom blending operator.
        *
        * See drawTexturedTriangle() for more details.
        *
        * NOTE: the vertices can be given either in clockwise or counter-clockwise order.
        **/
        template<typename color_t_tex, typename BLEND_OPERATOR>
        void drawTexturedQuad(const Image<color_t_tex> & src_im, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 srcP4, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, fVec2 dstP4, const BLEND_OPERATOR& blend_op)
            {
            drawTexturedTriangle(src_im, srcP1, srcP2, srcP3, dstP1, dstP2, dstP3, blend_op);
            drawTexturedTriangle(src_im, srcP1, srcP3, srcP4, dstP1, dstP3, dstP4, blend_op);
            }




        /**
        * Draw a textured quad combined with a color gradient (no blending).
        *
        * See drawTexturedTriangleGradient() for details.
        *
        * NOTE: the vertices can be given either in clockwise or counter-clockwise order.
        **/
        template<typename color_t_tex>
        void drawTexturedQuadGradient(const Image<color_t_tex> & src_im, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 srcP4, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, fVec2 dstP4, color_t_tex C1, color_t_tex C2, color_t_tex C3, color_t_tex C4)
            {
            drawTexturedTriangleGradient(src_im, srcP1, srcP2, srcP3, dstP1, dstP2, dstP3, C1, C2, C3);
            drawTexturedTriangleGradient(src_im, srcP1, srcP3, srcP4, dstP1, dstP3, dstP4, C1, C3, C4);
            }


        /**
        * Draw a textured quad combined with a color gradient (with blending).
        *
        * See drawTexturedTriangleGradient() for details.
        *
        * NOTE: the vertices can be given either in clockwise or counter-clockwise order.
        **/
        template<typename color_t_tex>
        void drawTexturedQuadGradient(const Image<color_t_tex> & src_im, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 srcP4, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, fVec2 dstP4, color_t_tex C1, color_t_tex C2, color_t_tex C3, color_t_tex C4, float opacity)
            {
            drawTexturedTriangleGradient(src_im, srcP1, srcP2, srcP3, dstP1, dstP2, dstP3, C1, C2, C3, opacity);
            drawTexturedTriangleGradient(src_im, srcP1, srcP3, srcP4, dstP1, dstP3, dstP4, C1, C3, C4, opacity);
            }


        /**
        * Draw a textured quad with a mask (i.e. a fixed transparent color).
        *
        * See drawTexturedTriangleMasked() for details.
        *
        * NOTE: the vertices can be given either in clockwise or counter-clockwise order.
        **/
        template<typename color_t_tex>
        void drawTexturedQuadMasked(const Image<color_t_tex> & src_im, color_t_tex transparent_color, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 srcP4, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, fVec2 dstP4, float opacity)
            {
            drawTexturedTriangleMasked(src_im, transparent_color, srcP1, srcP2, srcP3, dstP1, dstP2, dstP3, opacity);
            drawTexturedTriangleMasked(src_im, transparent_color, srcP1, srcP3, srcP4, dstP1, dstP3, dstP4, opacity);
            }


        /**
        * Draw a textured quad with a mask (i.e. a fixed transparent color) and combined with a color gradient.
        *
        * See drawTexturedTriangleMaskedGradient() for details.
        *
        * NOTE: the vertices can be given either in clockwise or counter-clockwise order.
        **/
        template<typename color_t_tex>
        void drawTexturedQuadMaskedGradient(const Image<color_t_tex>& src_im, color_t_tex transparent_color, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 srcP4, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, fVec2 dstP4, color_t_tex C1, color_t_tex C2, color_t_tex C3, color_t_tex C4, float opacity)
            {
            drawTexturedTriangleMaskedGradient(src_im, transparent_color, srcP1, srcP2, srcP3, dstP1, dstP2, dstP3, C1, C2, C3, opacity);
            drawTexturedTriangleMaskedGradient(src_im, transparent_color, srcP1, srcP3, srcP4, dstP1, dstP3, dstP4, C1, C3, C4, opacity);
            }





    /************************************************************************************
    *
    * Drawing text 
    * 
    * supported font format: 
    *     - AdafruitGFX            https ://glenviewsoftware.com/projects/products/adafonteditor/adafruit-gfx-font-format/
    *     - ILI9341_t3 v1          https://forum.pjrc.com/threads/54316-ILI9341_t-font-structure-format
    *       and v23 (antialiased). https://github.com/projectitis/packedbdf/blob/master/packedbdf.md
    *
    *************************************************************************************/


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
    * Don't you dare look below... this is private :-p
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
        * DRAWING PRIMITIVES
        ****************************************/

        /** line drawing method */
        template<bool CHECKRANGE> inline void _drawLine(int x0, int y0, int x1, int y1, color_t color);


        /** line drawing method */
        template<bool CHECKRANGE> inline void _drawLine(int x0, int y0, int x1, int y1, color_t color, float opacity);


        /** Set/blend a pixel. */
        template<bool BLEND> inline void _drawPixel(bool checkrange, int x, int y, color_t color, float opacity)
        {
            if (checkrange) // optimized away at compile time
            {
                if ((x < 0) || (y < 0) || (x >= _lx) || (y >= _ly)) return;
            }
            if (BLEND)
                _buffer[TGX_CAST32(x) + TGX_CAST32(_stride) * TGX_CAST32(y)].blend(color, opacity);
            else
                _buffer[TGX_CAST32(x) + TGX_CAST32(_stride) * TGX_CAST32(y)] = color;
        }


        /* draw segment (same as line but possibly without endpoint) */
        template<bool BLEND> void _drawSeg(bool checkrange, int x0, int y0, int x1, int y1, bool draw_last, color_t color, float opacity);

        template<bool BLEND> void _drawPolyLine(int nbpoints, const iVec2 tabPoints[], bool draw_last_point, color_t color, float opacity);

        template<bool BLEND> void _drawPolygon(int nbpoints, const iVec2 tabPoints[], color_t color, float opacity);


        /**
        * Bezier / Splines
        **/

        template<bool BLEND> void _plotQuadRationalBezierSeg(const bool checkrange, int x0, int y0, int x1, int y1, int x2, int y2, float w, const color_t color, const float opacity);

        template<bool BLEND> void _plotQuadRationalBezier(const bool checkrange, int x0, int y0, int x1, int y1, int x2, int y2, float w, const bool draw_P2, const color_t color, const float opacity);

        template<bool BLEND> void _drawQuadBezier(iVec2 P1, iVec2 P2, iVec2 PC, float wc, bool drawP2, color_t color, float opacity);

        template<bool BLEND> void _plotCubicBezierSeg(const bool checkrange, int x0, int y0, float x1, float y1, float x2, float y2, int x3, int y3, const color_t color, const float opacity);

        template<bool BLEND> void _plotCubicBezier(const bool checkrange, int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, bool draw_P2, const color_t color, const float opacity);

        template<bool BLEND> void _drawCubicBezier(iVec2 P1, iVec2 P2, iVec2 PA, iVec2 PB, bool drawP2, color_t color, float opacity);

        template<bool BLEND> void _plotQuadSpline(int n, int x[], int y[], bool draw_last, const color_t color, const float opacity);

        template<int SPLINE_MAX_POINTS, bool BLEND> void _drawQuadSpline(int nbpoints, const iVec2* tabPoints, bool draw_last_point, color_t color, float opacity);

        template<bool BLEND> void _plotClosedSpline(int n, int x[], int y[], const color_t color, const float opacity);

        template<int SPLINE_MAX_POINTS, bool BLEND> void _drawClosedSpline(int nbpoints, const iVec2* tabPoints, color_t color, float opacity);

        template<bool BLEND> void _plotCubicSpline(int n, int x[], int y[], bool draw_last, const color_t color, const float opacity);

        template<int SPLINE_MAX_POINTS, bool BLEND> void _drawCubicSpline(int nbpoints, const iVec2* tabPoints, bool draw_last_point, color_t color, float opacity);




        /** triangle drawing method */
        template<bool DRAW_INTERIOR, bool DRAW_OUTLINE, bool BLEND>
        void _drawTriangle(iVec2 P1, iVec2 P2, iVec2 P3, color_t interior_color, color_t outline_color, float opacity)
            {
            const iBox2 B = imageBox();
            if (B.contains(P1) && B.contains(P2)&& B.contains(P3))
                _drawTriangle_sub<false, DRAW_INTERIOR, DRAW_OUTLINE, BLEND>(P1.x, P1.y, P2.x, P2.y, P3.x, P3.y, interior_color, outline_color, opacity);
            else
                _drawTriangle_sub<true, DRAW_INTERIOR, DRAW_OUTLINE, BLEND> (P1.x, P1.y, P2.x, P2.y, P3.x, P3.y, interior_color, outline_color, opacity);
            }


        /** triangle drawing method templated on CHECKRANGE */
        template<bool CHECKRANGE, bool DRAW_INTERIOR, bool DRAW_OUTLINE, bool BLEND>
        void _drawTriangle_sub(int x0, int y0, int x1, int y1, int x2, int y2, color_t interior_color, color_t outline_color, float opacity);

    
        /** draw a rounded rectangle outline */
        template<bool CHECKRANGE> 
        void _drawRoundRect(int x, int y, int w, int h, int r, color_t color);


        /** draw a rounded rectangle outline */
        template<bool CHECKRANGE> 
        void _drawRoundRect(int x, int y, int w, int h, int r, color_t color, float opacity);


        /** fill a rounded rectangle  */
        template<bool CHECKRANGE> 
        void _fillRoundRect(int x, int y, int w, int h, int r, color_t color);


        /** fill a rounded rectangle  */
        template<bool CHECKRANGE> 
        void _fillRoundRect(int x, int y, int w, int h, int r, color_t color, float opacity);


        /** taken from Adafruit GFX library */
        template<bool CHECKRANGE> 
        void _drawCircleHelper(int x0, int y0, int r, int cornername, color_t color);


        /** taken from Adafruit GFX library */
        template<bool CHECKRANGE> 
        void _drawCircleHelper(int x0, int y0, int r, int cornername, color_t color, float opacity);


        /** taken from Adafruit GFX library */
        template<bool CHECKRANGE> 
        void _fillCircleHelper(int x0, int y0, int r, int corners, int delta, color_t color);


        /** taken from Adafruit GFX library */
        template<bool CHECKRANGE> 
        void _fillCircleHelper(int x0, int y0, int r, int corners, int delta, color_t color, float opacity);


        /** filled circle drawing method */
        template<bool OUTLINE, bool FILL, bool CHECKRANGE> 
        void _drawFilledCircle(int xm, int ym, int r, color_t color, color_t fillcolor);


        /** filled circle drawing method */
        template<bool OUTLINE, bool FILL, bool CHECKRANGE> 
        void _drawFilledCircle(int xm, int ym, int r, color_t color, color_t fillcolor, float opacity);


        /** adapted from bodmer e_tft library */
        template<bool OUTLINE, bool FILL, bool CHECKRANGE>
        void _drawEllipse(int x0, int y0, int rx, int ry, color_t outline_color, color_t interior_color);


        /** adapted from bodmer e_tft library */
        template<bool OUTLINE, bool FILL, bool CHECKRANGE>
        void _drawEllipse(int x0, int y0, int rx, int ry, color_t outline_color, color_t interior_color, float opacity);



        /***************************************
        * High Quality drawing primitive
        ****************************************/



        /** adapted from bodmer e_tft library */
        void _drawWideLine(float ax, float ay, float bx, float by, float wd, color_t color, float opacity);
    

        /** adapted from bodmer e_tft library */
        void _drawWedgeLine(float ax, float ay, float bx, float by, float aw, float bw, color_t color, float opacity);


        /**
        * adapted from bodmer e_tft library
        * Calculate distance of px,py to closest part of line
        **/
        inline float TGX_INLINE _wideLineDistance(float pax, float pay, float bax, float bay, float dr)
            {
            float h = fmaxf(fminf((pax * bax + pay * bay) / (bax * bax + bay * bay), 1.0f), 0.0f);
            float dx = pax - bax * h, dy = pay - bay * h;
            dx = dx * dx + dy * dy;
            if (dx < dr) return 0.0f;
            return tgx::fast_sqrt(dx);
            }


        /**
        * adapted from bodmer e_tft library
        * Calculate distance of px,py to closest part of line
        **/
        inline TGX_INLINE float _wedgeLineDistance(float pax, float pay, float bax, float bay, float dr)
            {
            float h = fmaxf(fminf((pax * bax + pay * bay) / (bax * bax + bay * bay), 1.0f), 0.0f);
            float dx = pax - bax * h, dy = pay - bay * h;
            return tgx::fast_sqrt(dx * dx + dy * dy) + h * dr;
            }


        /** Convert to texture coordinates */
        inline TGX_INLINE tgx::fVec2 _coord_texture(tgx::fVec2 pos, tgx::iVec2 size)
            {
            return tgx::fVec2(pos.x / ((float)size.x) , pos.y / ((float)size.y));
            }


        /** Convert to viewport coordinates */
        inline TGX_INLINE tgx::fVec2 _coord_viewport(tgx::fVec2 pos, tgx::iVec2 size)
            {
            return tgx::fVec2((2.0f / ((float)size.x)) * (pos.x) - 1.0f, (2.0f / ((float)size.y)) * (pos.y) - 1.0f);
            }



        /** template method for drawing a 2D gradient triangle*/        
        template<typename color_alt, bool USE_BLENDING>
        void _drawGradientTriangle(fVec2 P1, fVec2 P2, fVec2 P3, color_alt colorP1, color_alt colorP2, color_alt colorP3, float opacity);


        /** template method for drawing 2D textured (possibly with gradient and mask) triangle */
        template<typename color_t_tex, bool GRADIENT, bool USE_BLENDING, bool MASKED>
        void _drawTexturedTriangle(const Image<color_t_tex>& src_im, color_t_tex transparent_color, fVec2 srcP1, fVec2 srcP2, fVec2 srcP3, fVec2 dstP1, fVec2 dstP2, fVec2 dstP3, color_t_tex C1, color_t_tex C2, color_t_tex C3, float opacity);






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


