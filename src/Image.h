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

#include <stdint.h>



namespace tgx
{

	/** always cast indexes as 32bit when doing pointer arithmetic */
	#define TGX_CAST32(a)	((int32_t)a)

		

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
	*   color(x,i) = buffer[x + y*_stride].  
	* 
	* - for new image, one can always choose _stride = lx (but allowing larger stride in 
	*   the class definition allows to efficiently represent sub-image as well).  
	*
	* Template parameter:
	* 
	* - color_t: Color type to use with the image, the memory buffer supplied should have 
	*             type color_t*. It must be one of the color types defined in color.h
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

		color_t* _buffer;		// pointer to the pixel buffer  - nullptr if the image is invalid.
		int		_lx, _ly;		// image size  - (0,0) if the image is invalid
		int		_stride;		// image stride - 0 if the image is invalid


	public:


	/************************************************************************************
	* 
	*  Creation of images and sub-images 
	* 
	* The memory buffer must be supplied at creation. Otherwise, the image is set as 
	* invalid until a valid buffer is supplied. 
	* 
	*************************************************************************************/


		/** Create default invalid/empty image. */
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
		Image(color_t* buffer, int lx, int ly, int stride = -1) : _buffer(buffer), _lx(lx), _ly(ly), _stride(stride < 0 ? lx : stride)
			{
			_checkvalid(); // make sure dimension/stride are ok else make the image invalid
			}


        /**
         * Constructor. Creates an image with a given size and a given buffer.
         *
         * @param [in,out]  buffer  the image buffer
         * @param           dim     the image dimension (width, height) in pixels.
         * @param           stride  the stide to use (equal to the image width if not specified).
        **/
		Image(color_t* buffer, iVec2 dim, int stride = -1) : Image(buffer, dim.x, dim.y, stride)
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
		template<typename T> Image(T * buffer, int lx, int ly, int stride = -1) : Image((color_t*)buffer, lx, ly, stride)
			{
			}


		/**
		 * Constructor. Creates an image with a given size and a given buffer.
		 *
		 * @param [in,out]  buffer  the image buffer
		 * @param           dim     the image dimension (width, height) in pixels.
		 * @param           stride  the stide to use (equal to the image width if not specified).
		**/
		template<typename T> Image(T * buffer, iVec2 dim, int stride = -1) : Image((color_t*)buffer, dim.x, dim.y, stride)
			{
			}


        /**
         * Constructor. Create a sub-image of a given image sharing the same buffer.
         *
         * @param   im      The source image.
         * @param   subbox  The region of the source image to use for this image.
         * @param   clamp   (Optional) True to clamp the values of the `subbox` parameter. If set,
         *                  `subbox` is intersected with the source image box to create a valid region.
         *                  Otherwise, if `subbox` does not fit inside the source image box, an empty image
         *                  is created.
        **/
		Image(const Image<color_t> & im, iBox2 subbox, bool clamp = true)
			{
			if (!im.isValid()) { setInvalid();  return; }
			if (clamp)
				{
				subbox &= im.imageBox();
				}
			else
				{
				if (!(im.imageBox().contains(subbox))) { setInvalid(); return; }
				}
			if (subbox.isEmpty()) { setInvalid(); return; }
			_lx = subbox.lx();
			_ly = subbox.ly(); 
			_stride = im._stride;
			_buffer = im._buffer + TGX_CAST32(subbox.minX) + TGX_CAST32(im._stride)* TGX_CAST32(subbox.minY);
			}


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
         * Crop this image by keeping only the region represented by subbox (intersected with the image
         * box).
         * 
         * This operation dos not change the underlying pixel buffer. It simply replace this image by a
         * sub-image of itself (with different dimension/stride).
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
         *                  Otherwise, if `subbox` does not fit inside the source image box, `then an
         *                  empty image is returned.
         *
         * @returns the cropped image.
        **/
		Image<color_t> getCrop(const iBox2& subbox, bool clamp = true) const
			{
			return Image<color_t>(*this, subbox, clamp);
			}


        /**
		 * Set/update the image parameters.
		 *
         * @param [in,out]  buffer  the pixel buffer.
         * @param           lx      image width.
         * @param           ly      image height.
         * @param           stride  (Optional) The stride. If not specified, the stride is set equal to
         *                          the image width.
        **/
		void set(color_t * buffer, int lx, int ly, int stride = -1)
			{
			_buffer = buffer;
			_lx = lx;
			_ly = ly;
			_stride = (stride < 0) ? lx : stride;
			_checkvalid(); // make sure dimension/stride are ok else make the image invalid
			}


		/**
		 * Set/update the image parameters.
		 *
		 * @param [in,out]  buffer  the pixel buffer.
		 * @param           lx      image width.
		 * @param           ly      image height.
		 * @param           stride  (Optional) The stride. If not specified, the stride is set equal to
		 *                          the image width.
		**/
		void set(void* buffer, int lx, int ly, int stride = -1)
			{
			set((color_t*)buffer, lx, ly, stride);
			}


        /**
         * Set/update the image parameters.
         *
         * @param [in,out]  buffer  the pixel buffer.
         * @param           dim     the image dimensions.
         * @param           stride  (Optional) The stride. If not specified, the stride is set equal to.
        **/
		void set(color_t* buffer, iVec2 dim, int stride = -1)
			{
			set(buffer, dim.x, dim.y, stride);
			}


		/**
		 * Set/update the image parameters.
		 *
		 * @param [in,out]  buffer  the pixel buffer.
		 * @param           dim     the image dimensions.
		 * @param           stride  (Optional) The stride. If not specified, the stride is set equal to.
		**/
		void set(void* buffer, iVec2 dim, int stride = -1)
			{
			set((color_t*)buffer, dim.x, dim.y, stride);
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


		/** Set the image as invalid. */
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
	* Blitting / copying / resizing images
	*
	*
	****************************************************************************/


        /**
         * Blit a sprite at a given position on the image.
         *
         * @param   sprite          The sprite image to blit.
         * @param   upperleftpos    Position of the upper left corner of the sprite in the image
        **/
		void blit(const Image<color_t> & sprite, iVec2 upperleftpos)
			{
			_blit(sprite, upperleftpos.x, upperleftpos.y, 0, 0, sprite.lx(), sprite.ly());
			}


		/**
		 * Blit a sprite at a given position on this image.
		 *
		 * @param   sprite          The sprite image to blit.
		 * @param   dest_x			x coordinate of the upper left corner of the sprite in the image.
		 * @param   dest_y			y coordinate of the upper left corner of the sprite in the image.
		**/
		void blit(const Image<color_t>& sprite, int dest_x, int dest_y)
			{
			_blit(sprite, dest_x, dest_y, 0, 0, sprite.lx(), sprite.ly());
			}


        /**
         * Blit a sprite at a given position on this image.
         * 
         * Use blending to draw the sprite over the image with a given opacity. If color_t has an alpha
         * channel, it is also used or blending.
         *
         * @param   sprite          The sprite image to blit.
         * @param   upperleftpos    Position of the upper left corner of the sprite in the image
         * @param   opacity         The opacity between 0.0f (fully transparent) and 1.0f (fully opaque).
        **/
		void blit(const Image<color_t>& sprite, iVec2 upperleftpos, float opacity)
			{
			_blit(sprite, upperleftpos.x, upperleftpos.y, 0, 0, sprite.lx(), sprite.ly(),opacity);
			}


		/**
		 * Blit a sprite at a given position on this image.
		 *
		 * Use blending to draw the sprite over the image with a given opacity. If color_t has an alpha
		 * channel, it is also used or blending.
		 *
		 * @param   sprite      The sprite image to blit.
		 * @param   dest_x		x coordinate of the upper left corner of the sprite in the image.
		 * @param   dest_y		y coordinate of the upper left corner of the sprite in the image.
		**/
		void blit(const Image<color_t>& sprite, int dest_x, int dest_y, float opacity)
			{
			_blit(sprite, dest_x, dest_y, 0, 0, sprite.lx(), sprite.ly(), opacity);
			}


        /**
         * Blit a sprite at a given position on this image. Sprite pixels with color `transparent_color`
         * are treated as transparent hence not copied on the image. Other pixels are blended with the
         * destination image using the opacity factor.
         *
         * @param   sprite              The sprite image to blit.
         * @param   transparent_color   The sprite color considered transparent.
         * @param   upperleftpos        Position of the upper left corner of the sprite in the image.
         * @param   opacity             The opacity between 0.0f (fully transparent) and 1.0f (fully opaque).
        **/
		void blitMasked(const Image<color_t>& sprite, color_t transparent_color, iVec2 upperleftpos, float opacity)
			{
			_blitMasked(sprite, transparent_color, upperleftpos.x, upperleftpos.y, 0, 0, sprite.lx(), sprite.ly(), opacity);
			}


        /**
         * Blit a sprite at a given position on this image. Sprite pixels with color `transparent_color`
         * are treated as transparent hence not copied on the image. Other pixels are blended with the
         * destination image using the opacity factor.
         *
         * @param   sprite              The sprite image to blit.
         * @param   transparent_color   The sprite color considered transparent.
		 * @param   dest_x				x coordinate of the upper left corner of the sprite in the image.
		 * @param   dest_y				y coordinate of the upper left corner of the sprite in the image.
		 * @param   opacity             The opacity between 0.0f (fully transparent) and 1.0f (fully
         *                              opaque).
        **/
		void blitMasked(const Image<color_t>& sprite, color_t transparent_color, int dest_x, int dest_y, float opacity)
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
		 * This is the inverse of the blit operation.
		 *
		 * @param   dst_sprite      The sprite to copy part of this image into.
		 * @param   dest_x			x coordinate of the upper left corner of the sprite in the image.
		 * @param   dest_y			y coordinate of the upper left corner of the sprite in the image.
		**/
		void blitBackward(Image<color_t>& dst_sprite, int dest_x, int dest_y) const
			{
			dst_sprite._blit(*this, 0, 0, dest_x, dest_y, dst_sprite.lx(), dst_sprite.ly());
			}



/************* TODO ***************

		void blitRotated(const Image<color_t>& sprite, iVec2 sprite_anchor, iVec2 dest_anchor, float angle_in_degre);

		void blitRotated(const Image<color_t>& sprite, int sprite_anchor_x, int sprite_anchor_y, int dest_anchor_x, int dest_anchor_y, float angle_in_degre);

		void blitRotated(const Image<color_t>& sprite, iVec2 sprite_anchor, iVec2 dest_anchor, float angle_in_degre, float opacity);

		void blitRotated(const Image<color_t>& sprite, int sprite_anchor_x, int sprite_anchor_y, int dest_anchor_x, int dest_anchor_y, float angle_in_degre, float opacity);

		void blitRotatedMasked(const Image<color_t>& sprite, color_t transparent_color, iVec2 sprite_anchor, iVec2 dest_anchor, float angle_in_degre, float opacity);

		void blitRotatedMasked(const Image<color_t>& sprite, color_t transparent_color, int sprite_anchor_x, int sprite_anchor_y, int dest_anchor_x, int dest_anchor_y, float angle_in_degre, float opacity);

***********************************/


		/**
		* Main method for converting between image types.
		*
		* Copy the src image onto this image. Resize/interpolate and change the color type 
		* if needed to match this image dimension and color type.
		* 
		* Beware: The method does not check for buffer overlap betwen source and destination !
		**/
		template<typename src_color_t> void copyFrom(const Image<src_color_t> & src);


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
		**/
		template<bool CHECKRANGE = true> TGX_INLINE inline void drawPixel(int x, int y, color_t color)
			{
			if (CHECKRANGE)	// optimized away at compile time
				{
				if ((!isValid()) || (x < 0) || (y < 0) || (x >= _lx) || (y >= _ly)) return;
				}
			_buffer[TGX_CAST32(x) + TGX_CAST32(_stride) * TGX_CAST32(y)] = color;
			}


		/**
		 * Blend a pixel with the current pixel color using opacity between 0.0f (fully transparent) and
		 * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		 *
		 * Use template parameter to disable range check for faster access (danger!).
		**/
		template<bool CHECKRANGE = true> TGX_INLINE inline void drawPixel(int x, int y, color_t color, float opacity)
			{
			if (CHECKRANGE)	// optimized away at compile time
				{
				if ((!isValid()) || (x < 0) || (y < 0) || (x >= _lx) || (y >= _ly)) return;
				}
			_buffer[TGX_CAST32(x) + TGX_CAST32(_stride) * TGX_CAST32(y)].blend(color,opacity);
			}


		/**
		* Set a pixel at a given position.
        * 
		* Use template parameter to disable range check for faster access (danger!)
		**/
		template<bool CHECKRANGE = true> TGX_INLINE inline void drawPixel(iVec2 pos, color_t color)
			{
			drawPixel<CHECKRANGE>(pos.x, pos.y, color);
			}


        /**
         * Blend a pixel with the current pixel color using opacity between 0.0f (fully transparent) and
         * 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
         * 
         * Use template parameter to disable range check for faster access (danger!).
        **/
		template<bool CHECKRANGE = true> TGX_INLINE inline void drawPixel(iVec2 pos, color_t color, float opacity)
			{
			drawPixel<CHECKRANGE>(pos.x, pos.y, color, opacity);
			}


		/**
		* Return the color of a pixel at a given position. 
		* If checkrange is true and outside_color is specified
		* then outside_color is returned when querying outside 
		* of the image. 
		**/
		template<bool CHECKRANGE = true> TGX_INLINE inline color_t readPixel(int x, int y, color_t * outside_color = nullptr)
			{
			if (CHECKRANGE)	// optimized away at compile time
				{
				if ((outside_color)&&((!isValid()) || (x < 0) || (y < 0) || (x >= _lx) || (y >= _ly))) return *outside_color;
				}
			return _buffer[TGX_CAST32(x) + TGX_CAST32(_stride) * TGX_CAST32(y)];
			}


		/**
		* Return the color of a pixel at a given position.
		* If checkrange is true and outside_color is specified
		* return outside_color when querying outside the image.
		**/
		template<bool CHECKRANGE = true> TGX_INLINE inline color_t readPixel(iVec2 pos, color_t* outside_color = nullptr)
			{
			return readPixel<CHECKRANGE>(pos.x, pos.y, outside_color);
			}


		/** 
		* Get a reference to a pixel (no range check!) const version 
		**/
		TGX_INLINE inline const color_t & operator()(int x, int y) const
			{
			return _buffer[TGX_CAST32(x) + TGX_CAST32(_stride) * TGX_CAST32(y)];
			}


		/**
		* Get a reference to a pixel (no range check!) const version
		**/
		TGX_INLINE inline const color_t& operator()(iVec2 pos) const
			{
			return _buffer[TGX_CAST32(pos.x) + TGX_CAST32(_stride) * TGX_CAST32(pos.y)];
			}


		/** 
		* Get a reference to a pixel (no range check!) non-const version
		**/
		TGX_INLINE inline color_t & operator()(int x, int y)
			{
			return _buffer[TGX_CAST32(x) + TGX_CAST32(_stride) * TGX_CAST32(y)];
			}


		/**
		* Get a reference to a pixel (no range check!) non-const version
		**/
		TGX_INLINE inline color_t& operator()(iVec2 pos)
			{
			return _buffer[TGX_CAST32(pos.x) + TGX_CAST32(_stride) * TGX_CAST32(pos.y)];
			}


        /**
         * Iterate over all the pixels of the image going from left to right and then top to bottom. 
         * The callback function cb_fun is called for each pixel and have a signature compatible with:
         * 
		 *  `bool cb_fun(tgx::iVec2 pos, color_t & color)`
         * 
         * where:
         * 
         * - `pos` is the position of the current pixel in the image
		 * - `color` is a reference to the current pixel color. 
		 * - the callback must return true to continue the iteration and false to abort the iteration.
         *
         * This method is particularly useful with lambdas, for example, to paint all black pixels
         * to red in a RGB565 image `im`:
		 *
         * im.iterate( [](tgx::iVec2 pos, tgx::RGB565 & color) { if (color == tgx::RGB565_Black) color = tgx::RGB565_Red; return true;});
         **/
		template<typename ITERFUN> void iterate(ITERFUN cb_fun)
			{
			iterate(cb_fun, imageBox());
			}


		/**
		* Same as above but iterate only over the pixels inside the sub-box B (intersected with the image box).
		**/
		template<typename ITERFUN> void iterate(ITERFUN cb_fun, tgx::iBox2 B);



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
		**/
		void fillScreen(color_t color)
			{
			fillRect(imageBox(), color);
			}


		/**
		* Fill the whole screen with a vertical color gradient between color1 and color2.
		* color interpolation takes place in RGB color space (even if color_t is HSV).
		**/
		void fillScreenVGradient(color_t color1, color_t color2)
			{
			fillRectVGradient(imageBox(), color1, color2);
			}


		/**
		* Fill the whole screen with an horizontal color gradient between color1 and color2.
		* color interpolation takes place in RGB color space (even if color_t is HSV).
		**/
		void fillScreenHGradient(color_t color1, color_t color2)
			{
			fillRectHGradient(imageBox(), color1, color2);
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
			drawFastVLine<CHECKRANGE>(pos.x, pos.y, h, color);
			}


		/**
		* Draw a vertical line of h pixels starting at (x,y).
		**/
		template<bool CHECKRANGE = true> inline void drawFastVLine(int x, int y, int h, color_t color)
			{
			if (CHECKRANGE)	// optimized away at compile time
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
		* Draw an vertical line of h pixels starting at pos.
		*
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		**/
		template<bool CHECKRANGE = true> TGX_INLINE inline void drawFastVLine(iVec2 pos, int h, color_t color, float opacity)
			{
			drawFastVLine<CHECKRANGE>(pos.x, pos.y, h, color, opacity);
			}


        /**
         * Draw a vertical line of h pixels starting at (x,y).
         * 
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		**/
		template<bool CHECKRANGE = true> inline void drawFastVLine(int x, int y, int h, color_t color, float opacity)
			{
			if (CHECKRANGE)	// optimized away at compile time
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
		* Draw an horizontal line of w pixels starting at (x,y). 
		**/
		template<bool CHECKRANGE = true> inline void drawFastHLine(int x, int y, int w, color_t color)
			{			
			if (CHECKRANGE)	// optimized away at compile time
				{
				if ((!isValid()) || (y < 0) || (y >= _ly) || (x >= _lx)) return;
				if (x < 0) { w += x; x = 0; }
				if (x + w > _lx) { w = _lx - x; }
				}	
			_fast_memset(_buffer + TGX_CAST32(x) + TGX_CAST32(y) * TGX_CAST32(_stride), color, w);
			}


		/**
		* Draw an horizontal line of w pixels starting at pos.
		**/
		template<bool CHECKRANGE = true> TGX_INLINE inline void drawFastHLine(iVec2 pos, int w, color_t color)
			{
			drawFastHLine<CHECKRANGE>(pos.x, pos.y, w, color);
			}


		/**
		* Draw an horizontal line of w pixels starting at pos.
		*
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		**/
		template<bool CHECKRANGE = true> TGX_INLINE inline void drawFastHLine(iVec2 pos, int w, color_t color, float opacity)
			{
			drawFastHLine<CHECKRANGE>(pos.x, pos.y, w, color, opacity);
			}


		/**
		* Draw an horizontal line of w pixels starting at (x,y).
		*
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		**/
		template<bool CHECKRANGE = true> inline void drawFastHLine(int x, int y, int w, color_t color, float opacity)
			{			
			if (CHECKRANGE)	// optimized away at compile time
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
		* Draw a line between (x0,y0) and (x1,y1) using Bresenham algorithm
		**/
		void drawLine(int x0, int y0, int x1, int y1, color_t color)
			{
			if (!isValid()) return;
			if ((x0 < 0) || (y0 < 0) || (x1 < 0) || (y1 < 0) || (x0 >= _lx) || (y0 >= _ly) || (x1 >= _lx) || (y1 >= _ly))
				_drawLine<true>(x0, y0, x1, y1, color);
			else
				_drawLine<false>(x0, y0, x1, y1, color);
			}


		/**
		* Draw a line between P1 and P2 using Bresenham algorithm
		**/
		inline void drawLine(const iVec2 & P1, const iVec2 & P2, color_t color)
			{
			drawLine(P1.x, P1.y, P2.x, P2.y, color);
			}


		/**
		* Draw a line between (x0,y0) and (x1,y1) using Bresenham algorithm
		*
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		**/
		void drawLine(int x0, int y0, int x1, int y1, color_t color, float opacity)
			{
			if (!isValid()) return;
			if ((x0 < 0) || (y0 < 0) || (x1 < 0) || (y1 < 0) || (x0 >= _lx) || (y0 >= _ly) || (x1 >= _lx) || (y1 >= _ly))
				_drawLine<true>(x0, y0, x1, y1, color, opacity);
			else
				_drawLine<false>(x0, y0, x1, y1, color, opacity);
			}


		/**
		* Draw a line between P1 and P2 using Bresenham algorithm
		*
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		**/
		inline void drawLine(const iVec2 & P1, const iVec2 & P2, color_t color, float opacity)
			{
			drawLine(P1.x, P1.y, P2.x, P2.y, color, opacity);
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
		void drawTriangle(int x1,int y1, int x2, int y2, int x3, int y3, color_t color)
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
		void drawTriangle(int x1,int y1, int x2, int y2, int x3, int y3, color_t color, float opacity)
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
		void fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, color_t interior_color, color_t outline_color)
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
		void fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, color_t interior_color, color_t outline_color, float opacity)
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
		**/
		void drawRect(const iBox2 & B, color_t color)
			{
			drawRect(B.minX, B.minY, B.lx(), B.ly(), color);
			}


		/**
		* Draw a rectangle with size (w,h) and upperleft corner at (x,y)
		**/
		void drawRect(int x, int y, int w, int h, color_t color)
			{
			drawFastHLine<true>(x, y, w, color);
			if (h > 1) drawFastHLine<true>(x, y + h - 1, w, color);
			drawFastVLine<true>(x, y + 1, h - 2, color);
			if (w > 1) drawFastVLine<true>(x + w - 1, y + 1, h - 2, color);
			}


		/**
		* Draw a rectangle corresponding to the box B.
		*
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		**/
		void drawRect(const iBox2& B, color_t color, float opacity)
			{
			drawRect(B.minX, B.minY, B.lx(), B.ly(), color, opacity);
			}


		/**
		* Draw a rectangle with size (w,h) and upperleft corner at (x,y)
		*
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		**/
		void drawRect(int x, int y, int w, int h, color_t color, float opacity)
			{
			drawFastHLine<true>(x, y, w, color, opacity);
			if (h > 1) drawFastHLine<true>(x, y + h - 1, w, color, opacity);
			drawFastVLine<true>(x, y + 1, h - 2, color, opacity);
			if (w > 1) drawFastVLine<true>(x + w - 1, y + 1, h - 2, color, opacity);
			}
	

		/**
		* Fill a rectangle region with a single color
		**/
		void fillRect(iBox2 B, color_t color);


		/**
		* Fill a rectangle region with a single color
		**/
		void fillRect(int x, int y, int w, int h, color_t color)
			{
			fillRect(iBox2( x, x + w - 1, y, y + h - 1 ), color);
			}


		/**
		* Fill a rectangle region with a single color
		*
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		**/
		void fillRect(iBox2 B, color_t color, float opacity);


		/**
		* Fill a rectangle region with a single color
		*
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		**/
		void fillRect(int x, int y, int w, int h, color_t color, float opacity)
			{
			fillRect(iBox2( x, x + w - 1, y, y + h - 1 ), color, opacity);
			}


		/**
		* Fill a rectangle with different colors for the outline and the interior
		**/
		void fillRect(iBox2 B, color_t color_interior, color_t color_outline)
			{
			drawRect(B, color_outline);
			fillRect(iBox2(B.minX + 1, B.maxX - 1, B.minY + 1, B.maxY - 1), color_interior);
			}


		/**
		* Fill a rectangle with different colors for the outline and the interior
		**/
		void fillRect(int x, int y, int w, int h, color_t color_interior, color_t color_outline)
			{
			fillRect(iBox2( x, x + w - 1, y, y + h - 1 ), color_interior, color_outline);
			}


		/**
		* Fill a rectangle with different colors for the outline and the interior
		*
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
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
		**/
		void fillRect(int x, int y, int w, int h, color_t color_interior, color_t color_outline, float opacity)
			{
			fillRect(iBox2( x, x + w - 1, y, y + h - 1 ), color_interior, color_outline, opacity);
			}


		/**
		* Draw a rounded rectangle in box B with corner radius r.
		**/
		void drawRoundRect(const iBox2 & B, int r, color_t color)
			{
			drawRoundRect(B.minX, B.minY, B.lx(), B.ly(), r, color);
			}


		/**
		* Draw a rounded rectangle with upper left corner (x,y), width w and 
		* height h and with corner radius r.
		**/
		void drawRoundRect(int x, int y, int w, int h, int r, color_t color)
			{
			if (!isValid() || (w <= 0) || (h <= 0)) return;
			if ((x >= 0) && (x + w < _lx) && (y >= 0) && (y + h < _ly))
				_drawRoundRect<false>(x, y, w, h, r, color);
			else
				_drawRoundRect<true>(x, y, w, h, r, color);
			}


		/**
		* Draw a rounded rectangle in box B with corner radius r.
		*
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		**/
		void drawRoundRect(const iBox2 & B, int r, color_t color, float opacity)
			{
			drawRoundRect(B.minX, B.minY, B.lx(), B.ly(), r, color, opacity);
			}


		/**
		* Draw a rounded rectangle with upper left corner (x,y), width w and 
		* height h and with corner radius r.
		*
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		**/
		void drawRoundRect(int x, int y, int w, int h, int r, color_t color, float opacity)
			{
			if (!isValid() || (w <= 0) || (h <= 0)) return;
			if ((x >= 0) && (x + w < _lx) && (y >= 0) && (y + h < _ly))
				_drawRoundRect<false>(x, y, w, h, r, color, opacity);
			else
				_drawRoundRect<true>(x, y, w, h, r, color, opacity);
			}


		/**
		* Draw a filled rounded rectangle in box B with corner radius r.
		**/
		void fillRoundRect(const iBox2 & B, int r, color_t color)
			{			
			fillRoundRect(B.minX, B.minY, B.lx(), B.ly(), r, color);			
			}


		/**
		* Draw a filled rounded rectangle with upper left corner (x,y), width w and
		* height h and with corner radius r.
		**/
		void fillRoundRect(int x, int y, int w, int h, int r, color_t color)
			{
			if (!isValid() || (w <= 0) || (h <= 0)) return;
			if ((x >= 0) && (x + w < _lx) && (y >= 0) && (y + h < _ly))
				_fillRoundRect<false>(x, y, w, h, r, color);
			else
				_fillRoundRect<true>(x, y, w, h, r, color);
			}


		/**
		* Draw a filled rounded rectangle in box B with corner radius r.
		*
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		**/
		void fillRoundRect(const iBox2& B, int r, color_t color, float opacity)
			{
			fillRoundRect(B.minX, B.minY, B.lx(), B.ly(), r, color, opacity);
			}


		/**
		* Draw a filled rounded rectangle with upper left corner (x,y), width w and 
		* height h and with corner radius r.
		*
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		**/
		void fillRoundRect(int x, int y, int w, int h, int r, color_t color, float opacity)
			{			
			if (!isValid() || (w <= 0) || (h <= 0)) return;
			if ((x >= 0) && (x + w < _lx) && (y >= 0) && (y + h < _ly))
				_fillRoundRect<false>(x, y, w, h, r, color, opacity);
			else
				_fillRoundRect<true>(x, y, w, h, r, color, opacity);
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
		void fillRectHGradient(int x, int y, int w, int h, color_t color1, color_t color2)
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
		void fillRectHGradient(int x, int y, int w, int h, color_t color1, color_t color2, float opacity)
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
		void fillRectVGradient(int x, int y, int w, int h, color_t color1, color_t color2) 
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
		void fillRectVGradient(int x, int y, int w, int h, color_t color1, color_t color2, float opacity) 
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
		void drawCircle(int cx, int cy, int r, color_t color)
			{
			drawCircle(iVec2(cx,cy), r, color);
			}


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
		*
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		**/
		void drawCircle(int cx, int cy, int r, color_t color, float opacity)
			{
			drawCircle(iVec2(cx,cy), r, color, opacity);
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
		* Draw both the outline and interior of a circle (possibly with distinct colors).
		**/
		void fillCircle(int cx, int cy, int r, color_t interior_color, color_t outline_color)
			{
			fillCircle(iVec2(cx, cy), r, interior_color, outline_color);
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
		*
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		**/
		void fillCircle(int cx, int cy, int r, color_t interior_color, color_t outline_color, float opacity)
			{
			fillCircle(iVec2(cx, cy), r, interior_color, outline_color, opacity);
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
		* Draw the outline of an ellipse (but not its interior).
		**/
		void drawEllipse(iVec2 center, int rx, int ry, color_t color)
			{
			drawEllipse(center.x, center.y, rx, ry, color, color);
			}


		/**
		* Draw the outline of an ellipse (but not its interior).
		**/
		void drawEllipse(int cx, int cy, int rx, int ry, color_t color)
			{
			if ((cx - rx >= 0) && (cx + rx < _lx) && (cy - ry >= 0) && (cy + ry < _ly))
				_drawEllipse<true, false, false>(cx, cy, rx , ry, color, color);
			else
				_drawEllipse<true, false, true>(cx, cy, rx, ry, color, color);
			}


		/**
		* Draw the outline of an ellipse (but not its interior).
		*
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		**/
		void drawEllipse(iVec2 center, int rx, int ry, color_t color, float opacity)
			{
			drawEllipse(center.x, center.y, rx, ry, color, color, opacity);
			}

		/**
		* Draw the outline of an ellipse (but not its interior).
		*
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		**/
		void drawEllipse(int cx, int cy, int rx, int ry, color_t color, float opacity)
			{
			if ((cx - rx >= 0) && (cx + rx < _lx) && (cy - ry >= 0) && (cy + ry < _ly))
				_drawEllipse<true, false, false>(cx, cy, rx , ry, color, color, opacity);
			else
				_drawEllipse<true, false, true>(cx, cy, rx, ry, color, color, opacity);
			}


		/**
		* Draw both the outline and interior of an ellipse (possibly with distinct colors).
		**/
		void fillEllipse(iVec2 center, int rx, int ry, color_t interior_color, color_t outline_color)
			{
			fillEllipse(center.x, center.y, rx, ry, interior_color, outline_color);
			}


		/**
		* Draw both the outline and interior of an ellipse (possibly with distinct colors).
		**/
		void fillEllipse(int cx, int cy, int rx, int ry, color_t interior_color, color_t outline_color)
			{
			if ((cx - rx >= 0) && (cx + rx < _lx) && (cy - ry >= 0) && (cy + ry < _ly))
				_drawEllipse<true, true, false>(cx, cy, rx , ry, outline_color, interior_color);
			else
				_drawEllipse<true, true, true>(cx, cy, rx, ry, outline_color, interior_color);
			}


		/**
		* Draw both the outline and interior of an ellipse (possibly with distinct colors).
		*
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		**/
		void fillEllipse(iVec2 center, int rx, int ry, color_t interior_color, color_t outline_color, float opacity)
			{
			fillEllipse(center.x, center.y, rx, ry, interior_color, outline_color, opacity);
			}


		/**
		* Draw both the outline and interior of an ellipse (possibly with distinct colors).
		*
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		**/
		void fillEllipse(int cx, int cy, int rx, int ry, color_t interior_color, color_t outline_color, float opacity)
			{
			if ((cx - rx >= 0) && (cx + rx < _lx) && (cy - ry >= 0) && (cy + ry < _ly))
				_drawEllipse<true, true, false>(cx, cy, rx , ry, outline_color, interior_color, opacity);
			else
				_drawEllipse<true, true, true>(cx, cy, rx, ry, outline_color, interior_color, opacity);
			}




		/************************************************************************************
		*
		*
		* High quality drawing: Anti-aliased primitives
		*
		*
		*************************************************************************************/


		/**
		* Draw an anti-aliased wide line from (ax,ay) to (bx,by) with thickness wd and radiuses ends.
		* Use floating point values for sub-pixel precision.
		*
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		*
		* CREDIT: Bodmer TFT_eSPI library : https://github.com/Bodmer/TFT_eSPI
		**/
		void drawWideLine(float ax, float ay, float bx, float by, float wd, color_t color, float opacity);


		/**
		* Draw an anti-aliased wide line from PA to PB with thickness wd and radiuses ends.
		* Use floating point values for sub-pixel precision.
		*
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		*
		* CREDIT: Bodmer TFT_eSPI library : https://github.com/Bodmer/TFT_eSPI
		**/
		void drawWideLine(fVec2 PA, fVec2 PB, float wd, color_t color, float opacity)
			{
			drawWideLine(PA.x, PA.y, PB.x, PB.y, wd, color, opacity);
			}


		/**
		* Draw an anti-aliased wide line from PA to PB with thickness wd and radiuses ends.
		* Use floating point values for sub-pixel precision.
		*
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		*
		* CREDIT: Bodmer TFT_eSPI library : https://github.com/Bodmer/TFT_eSPI
		**/
		void drawWideLine(iVec2 PA, iVec2 PB, float wd, color_t color, float opacity)
			{
			drawWideLine((float)PA.x, (float)PA.y, (float)PB.x, (float)PB.y, wd, color, opacity);
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
		void drawWedgeLine(float ax, float ay, float bx, float by, float aw, float bw, color_t color, float opacity);


		/**
		* Draw an anti-aliased wedge line from PA to PB with respective wideness aw and bw at the ends.
		* Use floating point values for sub-pixel precision.
		*
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		*
		* CREDIT: Bodmer TFT_eSPI library : https://github.com/Bodmer/TFT_eSPI
		**/
		void drawWedgeLine(fVec2 PA, fVec2 PB, float aw, float bw, color_t color, float opacity)
			{
			drawWedgeLine(PA.x, PA.y, PB.x, PB.y, aw, bw, color, opacity);
			}


		/**
		* Draw an anti-aliased wedge line from PA to PB with respective wideness aw and bw at the ends.
		* Integer value version: no sub-pixel precision.
		*
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		*
		* CREDIT: Bodmer TFT_eSPI library : https://github.com/Bodmer/TFT_eSPI
		**/
		void drawWedgeLine(iVec2 PA, iVec2 PB, float aw, float bw, color_t color, float opacity)
			{
			drawWedgeLine((float)PA.x, (float)PA.y, (float)PB.x, (float)PB.y, aw, bw, color, opacity);
			}


		/**
		* Draw an anti-aliased filled circle. Use floating point value for sub-pixel precision.
        * 
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		**/
		void drawSpot(float ax, float ay, float r, color_t color, float opacity)
			{
			// Filled circle can be created by the wide line function with length zero
			drawWideLine(ax, ay, ax, ay, 2.0f * r, color, opacity);
			}


		/**
		* Draw an anti-aliased filled circle. Use floating point value for sub-pixel precision.
        * 
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		**/
		void drawSpot(fVec2 center, float r, color_t color, float opacity)
			{
			drawSpot(center.x, center.y, r, color, opacity);
			}


		/**
		* Draw an anti-aliased filled circle. Integer valued version
        * 
		* Blend with the current color background using opacity between 0.0f (fully transparent) and
		* 1.0f (fully opaque). If color_t has an alpha channel, it is used (and multiplied by opacity).
		**/
		void drawSpot(iVec2 center, float r, color_t color, float opacity)
			{
			drawSpot((float)center.x, (float)center.y, r, color, opacity);
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

		void _blit(const Image& sprite, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy, float opacity);

		void _blitMasked(const Image& sprite, color_t transparent_color, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy, float opacity);


		/** blit a region while taking care of possible overlap */
		static void _blitRegion(color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy)
			{
			if ((size_t)pdest <= (size_t)psrc) 
				_blitRegionUp(pdest, dest_stride, psrc, src_stride, sx, sy);
			else 
				_blitRegionDown(pdest, dest_stride, psrc, src_stride, sx, sy);
			}

		/** blit a region while taking care of possible overlap */
		static void _blendRegion(color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy, float opacity)
			{
			if ((size_t)pdest <= (size_t)psrc)
				_blendRegionUp(pdest, dest_stride, psrc, src_stride, sx, sy, opacity);
			else
				_blendRegionDown(pdest, dest_stride, psrc, src_stride, sx, sy, opacity);
			}

		/** blit a region while taking care of possible overlap */
		static void _maskRegion(color_t transparent_color, color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy, float opacity)
			{
			if ((size_t)pdest <= (size_t)psrc)
				_maskRegionUp(transparent_color, pdest, dest_stride, psrc, src_stride, sx, sy, opacity);
			else
				_maskRegionDown(transparent_color, pdest, dest_stride, psrc, src_stride, sx, sy, opacity);
			}

		static void _blitRegionUp(color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy);

		static void _blitRegionDown(color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy);

		static void _blendRegionUp(color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy, float opacity);

		static void _blendRegionDown(color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy, float opacity);

		static void _maskRegionUp(color_t transparent_color, color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy, float opacity);

		static void _maskRegionDown(color_t transparent_color, color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy, float opacity);


		/***************************************
		* DRAWING PRIMITIVES
		****************************************/

		/** line drawing method */
		template<bool CHECKRANGE> inline void _drawLine(int x0, int y0, int x1, int y1, color_t color);


		/** line drawing method */
		template<bool CHECKRANGE> inline void _drawLine(int x0, int y0, int x1, int y1, color_t color, float opacity);


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


		/** taken from bodmer e_tft library */
		template<bool OUTLINE, bool FILL, bool CHECKRANGE>
		void _drawEllipse(int x0, int y0, int rx, int ry, color_t outline_color, color_t interior_color);


		/** taken from bodmer e_tft library */
		template<bool OUTLINE, bool FILL, bool CHECKRANGE>
		void _drawEllipse(int x0, int y0, int rx, int ry, color_t outline_color, color_t interior_color, float opacity);


		/**
		* Taken from Bodmer TFT_eSPI library : https://github.com/Bodmer/TFT_eSPI
		* Calculate distance of px,py to closest part of line
		**/
		inline float TGX_INLINE _wideLineDistance(float pax, float pay, float bax, float bay, float dr)
			{
			float h = fmaxf(fminf((pax * bax + pay * bay) / (bax * bax + bay * bay), 1.0f), 0.0f);
			float dx = pax - bax * h, dy = pay - bay * h;
			dx = dx * dx + dy * dy;
			if (dx < dr) return 0.0f;
			return sqrtf(dx);
			}


		/**
		* Taken from Bodmer TFT_eSPI library : https://github.com/Bodmer/TFT_eSPI
		* Calculate distance of px,py to closest part of line
		**/
		inline TGX_INLINE float _wedgeLineDistance(float pax, float pay, float bax, float bay, float dr)
			{
			float h = fmaxf(fminf((pax * bax + pay * bay) / (bax * bax + bay * bay), 1.0f), 0.0f);
			float dx = pax - bax * h, dy = pay - bay * h;
			return sqrtf(dx * dx + dy * dy) + h * dr;
			}



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















	/******************************************************************************************************************************
	* 
	* 
	* Implementation 
	*
	* 
	*******************************************************************************************************************************/







	/************************************************************************************
	* 
	*  Blitting / copying / resizing images
	* 
	*************************************************************************************/


	/** set len consecutive pixels given color starting at pdest */
	template<typename color_t>
	inline void Image<color_t>::_fast_memset(color_t* p_dest, color_t color, int32_t len)
		{		
		if(std::is_same <color_t, RGB565>::value) // optimized away at compile time
			{ // optimized code for RGB565.			
			if (len <= 0) return;
			uint16_t* pdest = (uint16_t*)p_dest;				// recasting
			const uint16_t col = (uint16_t)((RGB565)color);		// conversion to RGB565 does nothing but prevent compiler error when color_t is not RGB565
			// ! We assume here that pdest is already aligned mod 2 (it should be) ! 
			if (((intptr_t)pdest) & 3)
				{
				*(pdest++) = col;
				len--;
				}
			// now we are aligned mod 4
			const uint32_t c32 = col;
			const uint32_t cc = (c32 | (c32 << 16));
			uint32_t* pdest2 = (uint32_t*)pdest;
			int len32 = (len >> 5);			
			while (len32 > 0)
				{ // write 32 color pixels at once
				*(pdest2++) = cc; *(pdest2++) = cc; *(pdest2++) = cc; *(pdest2++) = cc;
				*(pdest2++) = cc; *(pdest2++) = cc; *(pdest2++) = cc; *(pdest2++) = cc;
				*(pdest2++) = cc; *(pdest2++) = cc; *(pdest2++) = cc; *(pdest2++) = cc;
				*(pdest2++) = cc; *(pdest2++) = cc; *(pdest2++) = cc; *(pdest2++) = cc;
				len32--;
				}				
			int len2 = ((len & 31) >> 1);		
			while (len2 > 0)
				{ // write 2 color pixels at once 
				*(pdest2++) = cc;
				len2--;
				}
			
			if (len & 1)
				{ // write the last pixel if needed. 
				*((uint16_t*)pdest2) = col;
				}				
			}
		else 
			{ // generic code for other color types
			while (len > 0) 
				{ 
				*(p_dest++) = color;
				len--;
                }
			}
		}


	template<typename color_t>
	void Image<color_t>::_blitRegionUp(color_t * pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy)
		{
		// TODO, make faster with specialization (writing 32bit at once etc...) 
		for (int j = 0; j < sy; j++)
			{
			color_t* pdest2 = pdest + TGX_CAST32(j) * TGX_CAST32(dest_stride);
			color_t* psrc2 = psrc + TGX_CAST32(j) * TGX_CAST32(src_stride);
			for (int i = 0; i < sx; i++) { pdest2[i] = psrc2[i]; }
			}
		}


	template<typename color_t>
	void Image<color_t>::_blitRegionDown(color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy)
		{
		// TODO, make faster with specialization (writing 32bit at once etc...)
		for (int j = sy - 1; j >= 0; j--)
			{
			color_t* pdest2 = pdest + TGX_CAST32(j) * TGX_CAST32(dest_stride);
			color_t* psrc2 = psrc + TGX_CAST32(j) * TGX_CAST32(src_stride);
			for (int i = sx - 1; i >= 0; i--) { pdest2[i] = psrc2[i]; }
			}	
		}


	template<typename color_t>
	void Image<color_t>::_blendRegionUp(color_t * pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy, float opacity)
		{
		const int op256 = (int)(opacity * 256);
		for (int j = 0; j < sy; j++)
			{
			color_t* pdest2 = pdest + TGX_CAST32(j) * TGX_CAST32(dest_stride);
			color_t* psrc2 = psrc + TGX_CAST32(j) * TGX_CAST32(src_stride);
			for (int i = 0; i < sx; i++) 
				{ 
				pdest2[i].blend256(psrc2[i], op256);
				}
			}
		}


	template<typename color_t>
	void Image<color_t>::_blendRegionDown(color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy, float opacity)
		{
		const int op256 = (int)(opacity * 256);
		for (int j = sy - 1; j >= 0; j--)
			{
			color_t* pdest2 = pdest + TGX_CAST32(j) * TGX_CAST32(dest_stride);
			color_t* psrc2 = psrc + TGX_CAST32(j) * TGX_CAST32(src_stride);
			for (int i = sx - 1; i >= 0; i--) 
				{
				pdest2[i].blend256(psrc2[i], op256);
				}
			}	
		}


	template<typename color_t>
	void Image<color_t>::_maskRegionUp(color_t transparent_color, color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy, float opacity)
		{
		const int op256 = (int)(opacity * 256);
		for (int j = 0; j < sy; j++)
			{
			color_t* pdest2 = pdest + TGX_CAST32(j) * TGX_CAST32(dest_stride);
			color_t* psrc2 = psrc + TGX_CAST32(j) * TGX_CAST32(src_stride);
			for (int i = 0; i < sx; i++) 
				{ 
				color_t c = psrc2[i];
				if (c != transparent_color) pdest2[i].blend256(c, op256);
				}
			}
		}


	template<typename color_t>
	void Image<color_t>::_maskRegionDown(color_t transparent_color, color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy, float opacity)
		{
		const int op256 = (int)(opacity * 256);
		for (int j = sy - 1; j >= 0; j--)
			{
			color_t* pdest2 = pdest + TGX_CAST32(j) * TGX_CAST32(dest_stride);
			color_t* psrc2 = psrc + TGX_CAST32(j) * TGX_CAST32(src_stride);
			for (int i = sx - 1; i >= 0; i--) 
				{
				color_t c = psrc2[i];
				if (c != transparent_color) pdest2[i].blend256(c, op256);
				}
			}	
		}


	template<typename color_t>
	bool Image<color_t>::_blitClip(const Image& sprite, int& dest_x, int& dest_y, int& sprite_x, int& sprite_y, int& sx, int& sy)
		{
		if ((!sprite.isValid()) || (!isValid())) { return false; } // nothing to draw on or from
		if (sprite_x < 0) { dest_x -= sprite_x; sx += sprite_x; sprite_x = 0; }
		if (sprite_y < 0) { dest_y -= sprite_y; sy += sprite_y; sprite_y = 0; }
		if (dest_x < 0) { sprite_x -= dest_x;   sx += dest_x; dest_x = 0; }
		if (dest_y < 0) { sprite_y -= dest_y;   sy += dest_y; dest_y = 0; }
		if ((dest_x >= _lx) || (dest_y >= _ly) || (sprite_x >= sprite._lx) || (sprite_x >= sprite._ly)) return false;
		sx -= max(0, (dest_x + sx - _lx));
		sy -= max(0, (dest_y + sy - _ly));
		sx -= max(0, (sprite_x + sx - sprite._lx));
		sy -= max(0, (sprite_y + sy - sprite._ly));
		if ((sx <= 0) || (sy <= 0)) return false;
		return true;
		}


	template<typename color_t>
	void Image<color_t>::_blit(const Image& sprite, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy)
		{
		if (!_blitClip(sprite, dest_x, dest_y, sprite_x, sprite_y, sx, sy)) return;
		_blitRegion(_buffer + TGX_CAST32(dest_y) * TGX_CAST32(_stride) + TGX_CAST32(dest_x), _stride, sprite._buffer + TGX_CAST32(sprite_y) * TGX_CAST32(sprite._stride) + TGX_CAST32(sprite_x), sprite._stride, sx, sy);
		}


	template<typename color_t>
	void Image<color_t>::_blit(const Image& sprite, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy, float opacity)
		{
		if (opacity < 0.0f) opacity = 0.0f; else if (opacity > 1.0f) opacity = 1.0f;
		if (!_blitClip(sprite, dest_x, dest_y, sprite_x, sprite_y, sx, sy)) return;
		_blendRegion(_buffer + TGX_CAST32(dest_y) * TGX_CAST32(_stride) + TGX_CAST32(dest_x), _stride, sprite._buffer + TGX_CAST32(sprite_y) * TGX_CAST32(sprite._stride) + TGX_CAST32(sprite_x), sprite._stride, sx, sy, opacity);
		}


	template<typename color_t>
	void Image<color_t>::_blitMasked(const Image& sprite, color_t transparent_color, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy, float opacity)
		{
		if (opacity < 0.0f) opacity = 0.0f; else if (opacity > 1.0f) opacity = 1.0f;
		if (!_blitClip(sprite, dest_x, dest_y, sprite_x, sprite_y, sx, sy)) return;
		_maskRegion(transparent_color, _buffer + TGX_CAST32(dest_y) * TGX_CAST32(_stride) + TGX_CAST32(dest_x), _stride, sprite._buffer + TGX_CAST32(sprite_y) * TGX_CAST32(sprite._stride) + TGX_CAST32(sprite_x), sprite._stride, sx, sy, opacity);
		}


	template<typename color_t>
	template<typename src_color_t>
	void Image<color_t>::copyFrom(const Image<src_color_t> & src)
		{ 
		// TODO : 
		//1) make it faster 
		//2) implement high quality resizing 
		if ((!src.isValid()) || (!isValid())) { return; }
		const int32_t ay = (src._ly > 1) ? (int32_t)(src._ly - 1) : (int32_t)(src._ly >> 1);
		const int32_t by = (_ly > 1) ? (int32_t)(_ly - 1) : 1;
		const int32_t ax = (src._lx > 1) ? (int32_t)(src._lx - 1) : (int32_t)(src._lx >> 1);
		const int32_t bx = (_lx > 1) ? (int32_t)(_lx - 1) : 1;
		for (int j = 0; j < _ly; j++)
			{
			const int32_t off_src = ((j * ay) / by) * TGX_CAST32(src._stride);
			const int32_t off_dest = j * TGX_CAST32(_stride);
			for (int i = 0; i < _lx; i++)
				{
				const int32_t x = (i * ax) / bx;
				_buffer[i + off_dest] = color_t(src._buffer[x + off_src]); // color conversion
				}
			}
		}



	template<typename color_t>
	Image<color_t> Image<color_t>::copyReduceHalf(const Image<color_t>& src_image)
		{
		if ((!isValid())||(!src_image.isValid())) { return Image<color_t>(); }
		if (src_image._lx == 1)
			{ 
			if (src_image._ly == 1)
				{ // stupid case 
				_buffer[0] = src_image._buffer[0];
				return Image<color_t>(*this, iBox2(0, 0, 0, 0), false);
				}
			if (_ly < (src_image._ly >> 1)) { return Image<color_t>(); }
			const color_t * p_src = src_image._buffer;
			color_t * p_dest = _buffer;
			int ny = (src_image._ly >> 1); 
			while (ny-- > 0)
				{
				(*p_dest) = meanColor(p_src[0], p_src[src_image._stride]);
				p_dest += _stride;
				p_src += (src_image._stride*2);
				}
			return Image<color_t>(*this, iBox2(0, 0, 0, (src_image._ly >> 1) - 1), false);
			}
		if (src_image._ly == 1)
			{
			if (_lx < (src_image._lx >> 1)) { return Image<color_t>(); }
			const color_t * p_src = src_image._buffer;
			color_t * p_dest = _buffer;
			int nx = (src_image._lx >> 1);
			while (nx-- > 0)
				{
				(*p_dest) = meanColor(p_src[0], p_src[1]);
				p_dest++;
				p_src += 2;
				}
			return Image<color_t>(*this, iBox2(0, (src_image._lx >> 1) - 1, 0, 0), false);
			}
		// source image dimension is strictly larger than 1 in each directions.
		if ((_lx < (src_image._lx >> 1)) || (_ly < (src_image._ly >> 1))) { return Image<color_t>(); }
		const int32_t ny = (int32_t)(src_image._ly >> 1);
		for(int32_t j=0; j < ny; j++)
			{
			const color_t * p_src = src_image._buffer + j * TGX_CAST32(2*src_image._stride);
			color_t * p_dest = _buffer + j * TGX_CAST32(_stride);
			int nx = (src_image._lx >> 1);
			while(nx-- > 0)
				{
				(*p_dest) = meanColor(p_src[0], p_src[1], p_src[src_image._stride], p_src[src_image._stride + 1]);
				p_dest++;
				p_src += 2;
				}
			}
		return Image<color_t>(*this, iBox2(0, (src_image._lx >> 1) - 1, 0, (src_image._ly >> 1) - 1), false);
		}




	/************************************************************************************
	* 
	*  Drawing primitives
	* 
	*************************************************************************************/

	/*****************************************************
	* Direct pixel access
	******************************************************/


	template<typename color_t>
	template<typename ITERFUN> void Image<color_t>::iterate(ITERFUN cb_fun, tgx::iBox2 B)
		{
		B &= imageBox();
		if (B.isEmpty()) return;
		for (int j = B.minY; j <= B.maxY; j++)
			{
			for (int i = B.minX; i <= B.maxX; i++)
				{
				if (!cb_fun(tgx::iVec2(i, j), operator()(i,j))) return;
				}
			}
		}



	/*****************************************************
	* Lines
	******************************************************/


	template<typename color_t>
	template<bool CHECKRANGE> void Image<color_t>::_drawLine(int x0, int y0, int x1, int y1, color_t color)
		{
		if (y0 == y1)
			{
			if (x1 >= x0) 
				drawFastHLine<CHECKRANGE>(x0, y0, x1 - x0 + 1, color); 
			else 
				drawFastHLine<CHECKRANGE>(x1, y0, x0 - x1 + 1, color);
			return;
			}
		else if (x0 == x1)
			{
			if (y1 >= y0) 
				drawFastVLine<CHECKRANGE>(x0, y0, y1 - y0 + 1, color);
			else
				drawFastVLine<CHECKRANGE>(x0, y1, y0 - y1 + 1, color);
			return;
			}
		bool steep = abs(y1 - y0) > abs(x1 - x0);
		if (steep)
			{
			swap(x0, y0);
			swap(x1, y1);
			}
		if (x0 > x1)
			{
			swap(x0, x1);
			swap(y0, y1);
			}
		int dx = x1 - x0;
		int dy = abs(y1 - y0);
		int err = dx / 2;
		int ystep = (y0 < y1) ? 1 : -1;
		int xbegin = x0;
		if (steep)
			{
			for (; x0 <= x1; x0++)
				{
				err -= dy;
				if (err < 0)
					{
					int len = x0 - xbegin;
					if (len)
						drawFastVLine<CHECKRANGE>(y0, xbegin, len + 1, color);
					else
						drawPixel<CHECKRANGE>(y0, x0, color);
					xbegin = x0 + 1;
					y0 += ystep;
					err += dx;
					}
				}
			if (x0 > xbegin + 1)
				drawFastVLine<CHECKRANGE>(y0, xbegin, x0 - xbegin, color);
			}
		else
			{
			for (; x0 <= x1; x0++)
				{
				err -= dy;
				if (err < 0)
					{
					int len = x0 - xbegin;
					if (len)
						drawFastHLine<CHECKRANGE>(xbegin, y0, len + 1, color);
					else
						drawPixel<CHECKRANGE>(x0, y0, color);
					xbegin = x0 + 1;
					y0 += ystep;
					err += dx;
					}
				}
			if (x0 > xbegin + 1)
				drawFastHLine<CHECKRANGE>(xbegin, y0, x0 - xbegin, color);
			}
		}


	template<typename color_t>
	template<bool CHECKRANGE> void Image<color_t>::_drawLine(int x0, int y0, int x1, int y1, color_t color, float opacity)
		{
		if (y0 == y1)
			{
			if (x1 >= x0) 
				drawFastHLine<CHECKRANGE>(x0, y0, x1 - x0 + 1, color, opacity); 
			else 
				drawFastHLine<CHECKRANGE>(x1, y0, x0 - x1 + 1, color, opacity);
			return;
			}
		else if (x0 == x1)
			{
			if (y1 >= y0) 
				drawFastVLine<CHECKRANGE>(x0, y0, y1 - y0 + 1, color, opacity);
			else
				drawFastVLine<CHECKRANGE>(x0, y1, y0 - y1 + 1, color, opacity);
			return;
			}
		bool steep = abs(y1 - y0) > abs(x1 - x0);
		if (steep)
			{
			swap(x0, y0);
			swap(x1, y1);
			}
		if (x0 > x1)
			{
			swap(x0, x1);
			swap(y0, y1);
			}
		int dx = x1 - x0;
		int dy = abs(y1 - y0);
		int err = dx / 2;
		int ystep = (y0 < y1) ? 1 : -1;
		int xbegin = x0;
		if (steep)
			{
			for (; x0 <= x1; x0++)
				{
				err -= dy;
				if (err < 0)
					{
					int len = x0 - xbegin;
					if (len)
						drawFastVLine<CHECKRANGE>(y0, xbegin, len + 1, color, opacity);
					else
						drawPixel<CHECKRANGE>(y0, x0, color, opacity);
					xbegin = x0 + 1;
					y0 += ystep;
					err += dx;
					}
				}
			if (x0 > xbegin + 1)
				drawFastVLine<CHECKRANGE>(y0, xbegin, x0 - xbegin, color, opacity);
			}
		else
			{
			for (; x0 <= x1; x0++)
				{
				err -= dy;
				if (err < 0)
					{
					int len = x0 - xbegin;
					if (len)
						drawFastHLine<CHECKRANGE>(xbegin, y0, len + 1, color, opacity);
					else
						drawPixel<CHECKRANGE>(x0, y0, color, opacity);
					xbegin = x0 + 1;
					y0 += ystep;
					err += dx;
					}
				}
			if (x0 > xbegin + 1)
				drawFastHLine<CHECKRANGE>(xbegin, y0, x0 - xbegin, color, opacity);
			}
		}





	/*****************************************************
	* Triangles
	******************************************************/


		/**Adapted from from adafruit gfx / Kurte ILI93412_t3n. */
		template<typename color_t>
		template<bool CHECKRANGE, bool DRAW_INTERIOR, bool DRAW_OUTLINE, bool BLEND>
		void Image<color_t>::_drawTriangle_sub(int x0, int y0, int x1, int y1, int x2, int y2, color_t interior_color, color_t outline_color, float opacity)
			{
			int a, b, y, last;
			if (y0 > y1)
				{
				swap(y0, y1);
				swap(x0, x1);
				}
			if (y1 > y2)
				{
				swap(y2, y1);
				swap(x2, x1);
				}
			if (y0 > y1)
				{
				swap(y0, y1);
				swap(x0, x1);
				}

			if (y0 == y2)
				{ // triangle is just a single horizontal line
				a = b = x0;
				if (x1 < a) a = x1; else if (x1 > b) b = x1;
				if (x2 < a) a = x2; else if (x2 > b) b = x2;
				if ((!DRAW_OUTLINE) && (DRAW_INTERIOR))
					{
					if (BLEND)
						drawFastHLine<CHECKRANGE>(a, y0, b - a + 1, interior_color, opacity);
					else
						drawFastHLine<CHECKRANGE>(a, y0, b - a + 1, interior_color);
					}
				if (DRAW_OUTLINE)
					{
					if (BLEND)
						drawFastHLine<CHECKRANGE>(a, y0, b - a + 1, outline_color, opacity);
					else
						drawFastHLine<CHECKRANGE>(a, y0, b - a + 1, outline_color);
					}
				return;
				}
			int dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0,
				dx12 = x2 - x1, dy12 = y2 - y1, sa = 0, sb = 0;
			if (y1 == y2) last = y1; else last = y1 - 1;

			if ((DRAW_OUTLINE) && (DRAW_INTERIOR) && (outline_color == interior_color))
				{
				for (y = y0; y <= last; y++)
					{
					a = x0 + sa / dy01;
					b = x0 + sb / dy02;
					sa += dx01;
					sb += dx02;
					if (a > b) swap(a, b);
					if (BLEND) drawFastHLine<CHECKRANGE>(a, y, b - a + 1, interior_color, opacity); else drawFastHLine<CHECKRANGE>(a, y, b - a + 1, interior_color);
					}
				sa = dx12 * (y - y1);
				sb = dx02 * (y - y0);
				for (; y <= y2; y++)
					{
					a = x1 + sa / dy12;
					b = x0 + sb / dy02;
					sa += dx12;
					sb += dx02;
					if (a > b) swap(a, b);
					if (BLEND) drawFastHLine<CHECKRANGE>(a, y, b - a + 1, interior_color, opacity); else drawFastHLine<CHECKRANGE>(a, y, b - a + 1, interior_color);
					}
				}
			else
				{
				int prv_a = x0 + ((dy01 != 0) ? sa / dy01 : 0);
				int prv_b = x0 + ((dy02 != 0) ? sb / dy02 : 0);
				for (y = y0; y <= last; y++)
					{
					a = x0 + sa / dy01;
					b = x0 + sb / dy02;
					sa += dx01;
					sb += dx02;
					if (a > b) swap(a, b);

					const auto hha = a - prv_a;
					int ia, ib;
					if (hha == 0)
						{
						if (DRAW_OUTLINE) { if (BLEND) drawPixel<CHECKRANGE>(prv_a, y, outline_color, opacity); else drawPixel<CHECKRANGE>(prv_a, y, outline_color, opacity); }
						ia = a + 1;
						}
					else if (hha > 0)
						{
						if (DRAW_OUTLINE) { if (BLEND) drawFastHLine<CHECKRANGE>(prv_a + 1, y, hha, outline_color, opacity); else drawFastHLine<CHECKRANGE>(prv_a + 1, y, hha, outline_color); }
						ia = a + 1;
						}
					else
						{
						if (DRAW_OUTLINE) { if (BLEND) drawFastHLine<CHECKRANGE>(a, y, -hha, outline_color, opacity); else drawFastHLine<CHECKRANGE>(a, y, -hha, outline_color); }
						ia = prv_a;
						}

					const auto hhb = b - prv_b;
					if (hhb == 0)
						{
						if (DRAW_OUTLINE) { if (BLEND) drawPixel<CHECKRANGE>(prv_b, y, outline_color, opacity); else drawPixel<CHECKRANGE>(prv_b, y, outline_color, opacity); }
						ib = prv_b - 1;
						}
					else if (hhb > 0)
						{
						if (DRAW_OUTLINE) { if (BLEND) drawFastHLine<CHECKRANGE>(prv_b + 1, y, hhb, outline_color, opacity); else drawFastHLine<CHECKRANGE>(prv_b + 1, y, hhb, outline_color); }
						ib = prv_b;
						}
					else
						{
						if (DRAW_OUTLINE) { if (BLEND) drawFastHLine<CHECKRANGE>(b, y, -hhb, outline_color, opacity); else drawFastHLine<CHECKRANGE>(b, y, -hhb, outline_color); }
						ib = b - 1;
						}

					if ((DRAW_OUTLINE) && ((y == y2) || (y == y0)))
						{
						if (BLEND) drawFastHLine<CHECKRANGE>(ia, y, ib - ia + 1, outline_color, opacity); else drawFastHLine<CHECKRANGE>(ia, y, ib - ia + 1, outline_color);
						}
					if ((DRAW_INTERIOR) && (y < y2) && (y > y0))
						{
						if (BLEND) drawFastHLine<CHECKRANGE>(ia, y, ib - ia + 1, interior_color, opacity); else drawFastHLine<CHECKRANGE>(ia, y, ib - ia + 1, interior_color);
						}
					prv_a = a;
					prv_b = b;
					}
				sa = dx12 * (y - y1);
				sb = dx02 * (y - y0);
				for (; y <= y2; y++)
					{
					a = x1 + sa / dy12;
					b = x0 + sb / dy02;
					sa += dx12;
					sb += dx02;
					if (a > b) swap(a, b);

					const auto hha = a - prv_a;
					int ia, ib;
					if (hha == 0)
						{
						if (DRAW_OUTLINE) { if (BLEND) drawPixel<CHECKRANGE>(prv_a, y, outline_color, opacity); else drawPixel<CHECKRANGE>(prv_a, y, outline_color, opacity); }
						ia = a + 1;
						}
					else if (hha > 0)
						{
						if (DRAW_OUTLINE) { if (BLEND) drawFastHLine<CHECKRANGE>(prv_a + 1, y, hha, outline_color, opacity); else drawFastHLine<CHECKRANGE>(prv_a + 1, y, hha, outline_color); }
						ia = a + 1;
						}
					else
						{
						if (DRAW_OUTLINE) { if (BLEND) drawFastHLine<CHECKRANGE>(a, y, -hha, outline_color, opacity); else drawFastHLine<CHECKRANGE>(a, y, -hha, outline_color); }
						ia = prv_a;
						}
					const auto hhb = b - prv_b;
					if (hhb == 0)
						{
						if (DRAW_OUTLINE) { if (BLEND) drawPixel<CHECKRANGE>(prv_b, y, outline_color, opacity); else drawPixel<CHECKRANGE>(prv_b, y, outline_color, opacity); }
						ib = prv_b - 1;
						}
					else if (hhb > 0)
						{
						if (DRAW_OUTLINE) { if (BLEND) drawFastHLine<CHECKRANGE>(prv_b + 1, y, hhb, outline_color, opacity); else drawFastHLine<CHECKRANGE>(prv_b + 1, y, hhb, outline_color); }
						ib = prv_b;
						}
					else
						{
						if (DRAW_OUTLINE) { if (BLEND) drawFastHLine<CHECKRANGE>(b, y, -hhb, outline_color, opacity); else drawFastHLine<CHECKRANGE>(b, y, -hhb, outline_color); }
						ib = b - 1;
						}
					if ((DRAW_OUTLINE) && ((y == y2) || (y == y0)))
						{
						if (BLEND) drawFastHLine<CHECKRANGE>(ia, y, ib - ia + 1, outline_color, opacity); else drawFastHLine<CHECKRANGE>(ia, y, ib - ia + 1, outline_color);
						}
					if ((DRAW_INTERIOR) && (y < y2) && (y > y0))
						{
						if (BLEND) drawFastHLine<CHECKRANGE>(ia, y, ib - ia + 1, interior_color, opacity); else drawFastHLine<CHECKRANGE>(ia, y, ib - ia + 1, interior_color);
						}
					prv_a = a;
					prv_b = b;
					}
				}
			}



	/*****************************************************
	* Rectangles
	******************************************************/


	/** Fill a rectangle region with a single color */
	template<typename color_t> 
	void Image<color_t>::fillRect(iBox2 B, color_t color)
		{
		if (!isValid()) return;
		B &= imageBox();
		if (B.isEmpty()) return;
		const int sx = B.lx();
		int sy = B.ly();
		color_t * p = _buffer + TGX_CAST32(B.minX) + TGX_CAST32(B.minY) * TGX_CAST32(_stride);
		if (sx == _stride) 
			{ // fast, set everything at once
			const int32_t len = TGX_CAST32(sy) * TGX_CAST32(_stride);
			_fast_memset(p, color, len);
			}
		else
			{ // set each line separately
			while (sy-- > 0)
				{
				_fast_memset(p, color, sx);
				p += _stride;
				}
			}
		return;
		}


	template<typename color_t>
	void Image<color_t>::fillRect(iBox2 B, color_t color, float opacity)
		{
		if (!isValid()) return;
		B &= imageBox();
		if (B.isEmpty()) return;
		const int sx = B.lx();
		int sy = B.ly();
		color_t * p = _buffer + TGX_CAST32(B.minX) + TGX_CAST32(B.minY) * TGX_CAST32(_stride);
		if (sx == _stride) 
			{ // fast, set everything at once
			int32_t len = TGX_CAST32(sy) * TGX_CAST32(_stride);
			while (len-- > 0) { (*(p++)).blend(color, opacity); }
			}
		else
			{ // set each line separately
			while (sy-- > 0)
				{
				int len = sx;
				while (len-- > 0) { (*(p++)).blend(color, opacity); }
				p += (_stride - sx);
				}
			}
		return;
		}


	template<typename color_t>
	void Image<color_t>::fillRectHGradient(iBox2 B, color_t color1, color_t color2)
		{
		if (!isValid()) return;
		B &= imageBox();
		if (B.isEmpty()) return;		
		const int w = B.lx();
		const uint16_t d = (uint16_t)((w > 1) ? (w - 1) : 1);
		RGB64 c64_a(color1);	// color conversion to RGB64
		RGB64 c64_b(color2);    //
		const int16_t dr = (c64_b.R - c64_a.R) / d;
		const int16_t dg = (c64_b.G - c64_a.G) / d;
		const int16_t db = (c64_b.B - c64_a.B) / d;
		const int16_t da = (c64_b.A - c64_a.A) / d;
		color_t * p = _buffer + TGX_CAST32(B.minX) + TGX_CAST32(_stride) * TGX_CAST32(B.minY);
		for (int h = B.ly(); h > 0; h--)
			{
			RGB64 c(c64_a);
			for (int i = 0; i < w; i++)
				{
				p[i] = color_t(c);	// convert back
				c.R += dr;
				c.G += dg;
				c.B += db;
				c.A += da;
				}
			p += _stride;
			}
		}


	template<typename color_t>
	void Image<color_t>::fillRectHGradient(iBox2 B, color_t color1, color_t color2, float opacity)
		{
		if (!isValid()) return;
		B &= imageBox();
		if (B.isEmpty()) return;		
		const int w = B.lx();
		const uint16_t d = (uint16_t)((w > 1) ? (w - 1) : 1);
		RGB64 c64_a(color1);	// color conversion to RGB64
		RGB64 c64_b(color2);    //
		const int16_t dr = (c64_b.R - c64_a.R) / d;
		const int16_t dg = (c64_b.G - c64_a.G) / d;
		const int16_t db = (c64_b.B - c64_a.B) / d;
		const int16_t da = (c64_b.A - c64_a.A) / d;
		color_t * p = _buffer + TGX_CAST32(B.minX) + TGX_CAST32(_stride) * TGX_CAST32(B.minY);
		for (int h = B.ly(); h > 0; h--)
			{
			RGB64 c(c64_a);
			for (int i = 0; i < w; i++)
				{
				p[i].blend(color_t(c), opacity);
				c.R += dr;
				c.G += dg;
				c.B += db;
				c.A += da;
				}
			p += _stride;
			}
		}


	template<typename color_t>
	void Image<color_t>::fillRectVGradient(iBox2 B, color_t color1, color_t color2)
		{
		if (!isValid()) return;
		B &= imageBox();
		if (B.isEmpty()) return;
		const int h = B.ly(); 
		const uint16_t d = (uint16_t)((h > 1) ? (h - 1) : 1);
		RGB64 c64_a(color1);	// color conversion to RGB64
		RGB64 c64_b(color2);	//
		const int16_t dr = (c64_b.R - c64_a.R) / d;
		const int16_t dg = (c64_b.G - c64_a.G) / d;
		const int16_t db = (c64_b.B - c64_a.B) / d;
		const int16_t da = (c64_b.A - c64_a.A) / d;
		color_t * p = _buffer + TGX_CAST32(B.minX) + TGX_CAST32(_stride) * TGX_CAST32(B.minY);
		for (int j = h; j > 0; j--)
			{
			_fast_memset(p, color_t(c64_a), B.lx());
			c64_a.R += dr;
			c64_a.G += dg;
			c64_a.B += db;
			c64_a.A += da;
			p += _stride;
			}
		}


	template<typename color_t>
	void Image<color_t>::fillRectVGradient(iBox2 B, color_t color1, color_t color2, float opacity)
		{
		if (!isValid()) return;
		B &= imageBox();
		if (B.isEmpty()) return;
		const int h = B.ly(); 
		const uint16_t d = (uint16_t)((h > 1) ? (h - 1) : 1);
		RGB64 c64_a(color1);	// color conversion to RGB64
		RGB64 c64_b(color2);	//
		const int16_t dr = (c64_b.R - c64_a.R) / d;
		const int16_t dg = (c64_b.G - c64_a.G) / d;
		const int16_t db = (c64_b.B - c64_a.B) / d;
		const int16_t da = (c64_b.A - c64_a.A) / d;
		color_t * p = _buffer + TGX_CAST32(B.minX) + TGX_CAST32(_stride) * TGX_CAST32(B.minY);
		for (int j = h; j > 0; j--)
			{
			int l = B.lx();
			while (l-- > 0) { (*(p++)).blend(color_t(c64_a), opacity); }
			c64_a.R += dr;
			c64_a.G += dg;
			c64_a.B += db;
			c64_a.A += da;
			p += _stride - B.lx();
			}
		}



	/*****************************************************
	* Rounded rectangles
	******************************************************/



	/** draw a rounded rectangle outline */
	template<typename color_t>
	template<bool CHECKRANGE> void Image<color_t>::_drawRoundRect(int x, int y, int w, int h, int r, color_t color)
		{
		int max_radius = ((w < h) ? w : h) / 2;
		if (r > max_radius) r = max_radius;
		drawFastHLine<CHECKRANGE>(x + r, y, w - 2 * r, color);
		drawFastHLine<CHECKRANGE>(x + r, y + h - 1, w - 2 * r, color);
		drawFastVLine<CHECKRANGE>(x, y + r, h - 2 * r, color);
		drawFastVLine<CHECKRANGE>(x + w - 1, y + r, h - 2 * r, color);
		_drawCircleHelper<CHECKRANGE>(x + r, y + r, r, 1, color);
		_drawCircleHelper<CHECKRANGE>(x + w - r - 1, y + r, r, 2, color);
		_drawCircleHelper<CHECKRANGE>(x + w - r - 1, y + h - r - 1, r, 4, color);
		_drawCircleHelper<CHECKRANGE>(x + r, y + h - r - 1, r, 8, color);
		}


	/** draw a rounded rectangle outline */
	template<typename color_t>
	template<bool CHECKRANGE> void Image<color_t>::_drawRoundRect(int x, int y, int w, int h, int r, color_t color, float opacity)
		{
		int max_radius = ((w < h) ? w : h) / 2;
		if (r > max_radius) r = max_radius;
		drawFastHLine<CHECKRANGE>(x + r, y, w - 2 * r, color, opacity);
		drawFastHLine<CHECKRANGE>(x + r, y + h - 1, w - 2 * r, color, opacity);
		drawFastVLine<CHECKRANGE>(x, y + r, h - 2 * r, color, opacity);
		drawFastVLine<CHECKRANGE>(x + w - 1, y + r, h - 2 * r, color, opacity);
		_drawCircleHelper<CHECKRANGE>(x + r, y + r, r, 1, color, opacity);
		_drawCircleHelper<CHECKRANGE>(x + w - r - 1, y + r, r, 2, color, opacity);
		_drawCircleHelper<CHECKRANGE>(x + w - r - 1, y + h - r - 1, r, 4, color, opacity);
		_drawCircleHelper<CHECKRANGE>(x + r, y + h - r - 1, r, 8, color, opacity);
		}


	/** fill a rounded rectangle  */
	template<typename color_t>
	template<bool CHECKRANGE> void Image<color_t>::_fillRoundRect(int x, int y, int w, int h, int r, color_t color)
		{
		int max_radius = ((w < h) ? w : h) / 2;
		if (r > max_radius) r = max_radius;
		fillRect(x + r, y, w - 2 * r, h, color);
		_fillCircleHelper<CHECKRANGE>(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
		_fillCircleHelper<CHECKRANGE>(x + r, y + r, r, 2, h - 2 * r - 1, color);
		}


	/** fill a rounded rectangle  */
	template<typename color_t>
	template<bool CHECKRANGE> void Image<color_t>::_fillRoundRect(int x, int y, int w, int h, int r, color_t color, float opacity)
		{
		int max_radius = ((w < h) ? w : h) / 2;
		if (r > max_radius) r = max_radius;
		fillRect(x + r, y, w - 2 * r, h, color, opacity);
		_fillCircleHelper<CHECKRANGE>(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color, opacity);
		_fillCircleHelper<CHECKRANGE>(x + r, y + r, r, 2, h - 2 * r - 1, color, opacity);
		}





	/*****************************************************
	* Circles
	******************************************************/


	/** taken from adafruit gfx */
	template<typename color_t>
	template<bool CHECKRANGE> void Image<color_t>::_drawCircleHelper(int x0, int y0, int r, int cornername, color_t color)
		{
		int f = 1 - r;
		int ddF_x = 1;
		int ddF_y = -2 * r;
		int x = 0;
		int y = r;
		while (x < y)
			{
			if (f >= 0)
				{
				y--;
				ddF_y += 2;
				f += ddF_y;
				}
			x++;
			ddF_x += 2;
			f += ddF_x;
			if (cornername & 0x4)
				{
				drawPixel<CHECKRANGE>(x0 + x, y0 + y, color);
				drawPixel<CHECKRANGE>(x0 + y, y0 + x, color);
				}
			if (cornername & 0x2)
				{
				drawPixel<CHECKRANGE>(x0 + x, y0 - y, color);
				drawPixel<CHECKRANGE>(x0 + y, y0 - x, color);
				}
			if (cornername & 0x8)
				{
				drawPixel<CHECKRANGE>(x0 - y, y0 + x, color);
				drawPixel<CHECKRANGE>(x0 - x, y0 + y, color);
				}
			if (cornername & 0x1)
				{
				drawPixel<CHECKRANGE>(x0 - y, y0 - x, color);
				drawPixel<CHECKRANGE>(x0 - x, y0 - y, color);
				}
			}
		}

	/** taken from adafruit gfx */
	template<typename color_t>
	template<bool CHECKRANGE> void Image<color_t>::_drawCircleHelper(int x0, int y0, int r, int cornername, color_t color, float opacity)
		{
		int f = 1 - r;
		int ddF_x = 1;
		int ddF_y = -2 * r;
		int x = 0;
		int y = r;
		while (x < y - 1)
			{
			if (f >= 0)
				{
				y--;
				ddF_y += 2;
				f += ddF_y;
				}
			x++;
			ddF_x += 2;
			f += ddF_x;
			if (cornername & 0x4)
				{
				drawPixel<CHECKRANGE>(x0 + x, y0 + y, color, opacity);
				if (x!= y) drawPixel<CHECKRANGE>(x0 + y, y0 + x, color, opacity);
				}
			if (cornername & 0x2)
				{
				drawPixel<CHECKRANGE>(x0 + x, y0 - y, color, opacity);
				if (x != y) drawPixel<CHECKRANGE>(x0 + y, y0 - x, color, opacity);
				}
			if (cornername & 0x8)
				{
				drawPixel<CHECKRANGE>(x0 - y, y0 + x, color, opacity);
				if (x != y) drawPixel<CHECKRANGE>(x0 - x, y0 + y, color, opacity);
				}
			if (cornername & 0x1)
				{
				drawPixel<CHECKRANGE>(x0 - y, y0 - x, color, opacity);
				if (x != y) drawPixel<CHECKRANGE>(x0 - x, y0 - y, color, opacity);
				}
			}
		}



	/** taken from Adafruiit GFX*/
	template<typename color_t>
	template<bool CHECKRANGE> void Image<color_t>::_fillCircleHelper(int x0, int y0, int r, int corners, int delta, color_t color) 
		{
		int f = 1 - r;
		int ddF_x = 1;
		int ddF_y = -2 * r;
		int x = 0;
		int y = r;
		int px = x;
		int py = y;
		delta++; // Avoid some +1's in the loop
		while (x < y) 
			{
			if (f >= 0) 
				{
				y--;
				ddF_y += 2;
				f += ddF_y;
				}
			x++;
			ddF_x += 2;
			f += ddF_x;
			// These checks avoid double-drawing certain lines, important
			// for the SSD1306 library which has an INVERT drawing mode.
			if (x < (y + 1)) 
				{
				if (corners & 1) drawFastVLine<CHECKRANGE>(x0 + x, y0 - y, 2 * y + delta, color);
				if (corners & 2) drawFastVLine<CHECKRANGE>(x0 - x, y0 - y, 2 * y + delta, color);
				}
			if (y != py) 
				{
				if (corners & 1) drawFastVLine<CHECKRANGE>(x0 + py, y0 - px, 2 * px + delta, color);
				if (corners & 2) drawFastVLine<CHECKRANGE>(x0 - py, y0 - px, 2 * px + delta, color);
				py = y;
				}
			px = x;
			}
		}


	/** taken from Adafruit GFX*/
	template<typename color_t>
	template<bool CHECKRANGE> void Image<color_t>::_fillCircleHelper(int x0, int y0, int r, int corners, int delta, color_t color, float opacity)
		{
		int f = 1 - r;
		int ddF_x = 1;
		int ddF_y = -2 * r;
		int x = 0;
		int y = r;
		int px = x;
		int py = y;
		delta++; // Avoid some +1's in the loop
		while (x < y)
			{
			if (f >= 0)
				{
				y--;
				ddF_y += 2;
				f += ddF_y;
				}
			x++;
			ddF_x += 2;
			f += ddF_x;
			// These checks avoid double-drawing certain lines, important
			// for the SSD1306 library which has an INVERT drawing mode.
			if (x < (y + 1))
				{
				if (corners & 1) drawFastVLine<CHECKRANGE>(x0 + x, y0 - y, 2 * y + delta, color, opacity);
				if (corners & 2) drawFastVLine<CHECKRANGE>(x0 - x, y0 - y, 2 * y + delta, color, opacity);
				}
			if (y != py)
				{
				if (corners & 1) drawFastVLine<CHECKRANGE>(x0 + py, y0 - px, 2 * px + delta, color, opacity);
				if (corners & 2) drawFastVLine<CHECKRANGE>(x0 - py, y0 - px, 2 * px + delta, color, opacity);
				py = y;
				}
			px = x;
			}
		}



	template<typename color_t>
	template<bool OUTLINE, bool FILL, bool CHECKRANGE> void Image<color_t>::_drawFilledCircle(int xm, int ym, int r, color_t color, color_t fillcolor)
		{
		if ((r <= 0) || (!isValid())) return;
		if ((CHECKRANGE) && (r > 2))
			{ // circle is large enough to check first if there is something to draw.
			if ((xm + r < 0) || (xm - r >= _lx) || (ym + r < 0) || (ym - r >= _ly)) return; // outside of image. 
			// TODO : check if the circle completely fills the image, in this case use FillScreen()
			}
		switch (r)
			{
			case 0:
				{
				if (OUTLINE)
					{
					drawPixel<CHECKRANGE>(xm, ym, color);
					}
				else if (FILL)
					{
					drawPixel<CHECKRANGE>(xm, ym, fillcolor);
					}
				return;
				}
			case 1:
				{
				if (FILL)
					{
					drawPixel<CHECKRANGE>(xm, ym, fillcolor);
					}
				drawPixel<CHECKRANGE>(xm + 1, ym, color);
				drawPixel<CHECKRANGE>(xm - 1, ym, color);
				drawPixel<CHECKRANGE>(xm, ym - 1, color);
				drawPixel<CHECKRANGE>(xm, ym + 1, color);
				return;
				}
			}
		int x = -r, y = 0, err = 2 - 2 * r;
		do {
			if (OUTLINE)
				{
				drawPixel<CHECKRANGE>(xm - x, ym + y, color);
				drawPixel<CHECKRANGE>(xm - y, ym - x, color);
				drawPixel<CHECKRANGE>(xm + x, ym - y, color);
				drawPixel<CHECKRANGE>(xm + y, ym + x, color);
				}
			r = err;
			if (r <= y)
				{
				if (FILL)
					{
					drawFastHLine<CHECKRANGE>(xm, ym + y, -x, fillcolor);
					drawFastHLine<CHECKRANGE>(xm + x + 1, ym - y, -x - 1, fillcolor);
					}
				err += ++y * 2 + 1;
				}
			if (r > x || err > y)
				{
				err += ++x * 2 + 1;
				if (FILL)
					{
					if (x)
						{
						drawFastHLine<CHECKRANGE>(xm - y + 1, ym - x, y - 1, fillcolor);
						drawFastHLine<CHECKRANGE>(xm, ym + x, y, fillcolor);
						}
					}
				}
			} while (x < 0);
		}


	template<typename color_t>
	template<bool OUTLINE, bool FILL, bool CHECKRANGE> void Image<color_t>::_drawFilledCircle(int xm, int ym, int r, color_t color, color_t fillcolor, float opacity)
		{
		if ((r <= 0) || (!isValid())) return;
		if ((CHECKRANGE) && (r > 2))
			{ // circle is large enough to check first if there is something to draw.
			if ((xm + r < 0) || (xm - r >= _lx) || (ym + r < 0) || (ym - r >= _ly)) return; // outside of image. 
			// TODO : check if the circle completely fills the image, in this case use FillScreen()
			}
		switch (r)
			{
			case 0:
				{
				if (OUTLINE)
					{
					drawPixel<CHECKRANGE>(xm, ym, color, opacity);
					}
				else if (FILL)
					{
					drawPixel<CHECKRANGE>(xm, ym, fillcolor, opacity);
					}
				return;
				}
			case 1:
				{
				if (FILL)
					{
					drawPixel<CHECKRANGE>(xm, ym, fillcolor, opacity);
					}
				drawPixel<CHECKRANGE>(xm + 1, ym, color, opacity);
				drawPixel<CHECKRANGE>(xm - 1, ym, color, opacity);
				drawPixel<CHECKRANGE>(xm, ym - 1, color, opacity);
				drawPixel<CHECKRANGE>(xm, ym + 1, color, opacity);
				return;
				}
			}
		int x = -r, y = 0, err = 2 - 2 * r;
		do {
			if (OUTLINE)
				{
				drawPixel<CHECKRANGE>(xm - x, ym + y, color, opacity);
				drawPixel<CHECKRANGE>(xm - y, ym - x, color, opacity);
				drawPixel<CHECKRANGE>(xm + x, ym - y, color, opacity);
				drawPixel<CHECKRANGE>(xm + y, ym + x, color, opacity);
				}
			r = err;
			if (r <= y)
				{
				if (FILL)
					{
					drawFastHLine<CHECKRANGE>(xm, ym + y, -x, fillcolor, opacity);
					drawFastHLine<CHECKRANGE>(xm + x + 1, ym - y, -x - 1, fillcolor, opacity);
					}
				err += ++y * 2 + 1;
				}
			if (r > x || err > y)
				{
				err += ++x * 2 + 1;
				if (FILL)
					{
					if (x)
						{
						drawFastHLine<CHECKRANGE>(xm - y + 1, ym - x, y - 1, fillcolor, opacity);
						drawFastHLine<CHECKRANGE>(xm, ym + x, y, fillcolor, opacity);
						}
					}
				}
			} while (x < 0);
		}


	/*****************************************************
	* Ellipses
	******************************************************/


	/** Adapted from Bodmer e_tft library. */
	template<typename color_t>
	template<bool OUTLINE, bool FILL, bool CHECKRANGE>
	void Image<color_t>::_drawEllipse(int x0, int y0, int rx, int ry, color_t outline_color, color_t interior_color)
		{
		if (!isValid()) return;
		if (rx < 2) return;
		if (ry < 2) return;
		int32_t x, y;
		int32_t rx2 = rx * rx;
		int32_t ry2 = ry * ry;
		int32_t fx2 = 4 * rx2;
		int32_t fy2 = 4 * ry2;
		int32_t s;
		int yt = ry;
		for (x = 0, y = ry, s = 2 * ry2 + rx2 * (1 - 2 * ry); ry2 * x <= rx2 * y; x++) 
			{
			if (OUTLINE)
				{
				drawPixel<CHECKRANGE>(x0 - x, y0 - y, outline_color);
				drawPixel<CHECKRANGE>(x0 - x, y0 + y, outline_color);
				if (x != 0)
					{
					drawPixel<CHECKRANGE>(x0 + x, y0 - y, outline_color);
					drawPixel<CHECKRANGE>(x0 + x, y0 + y, outline_color);
					}
				}
			if (s >= 0) 
				{
				s += fx2 * (1 - y);
				y--;
				if (FILL)
					{
					if (ry2 * x <= rx2 * y)
						{
						drawFastHLine<CHECKRANGE>(x0 - x, y0 - y, x + x + 1, interior_color);
						drawFastHLine<CHECKRANGE>(x0 - x, y0 + y, x + x + 1, interior_color);
						yt = y;
						}
					}
				}
			s += ry2 * ((4 * x) + 6);
			}
		
		for (x = rx, y = 0, s = 2 * rx2 + ry2 * (1 - 2 * rx); rx2 * y <= ry2 * x; y++) 
			{
			if (OUTLINE)
				{
				drawPixel<CHECKRANGE>(x0 - x, y0 - y, outline_color);
				drawPixel<CHECKRANGE>(x0 + x, y0 - y, outline_color);
				if (y != 0)
					{
					drawPixel<CHECKRANGE>(x0 - x, y0 + y, outline_color);
					drawPixel<CHECKRANGE>(x0 + x, y0 + y, outline_color);
					}
				}
			if (FILL)
				{
				if (y != yt)
					{
					if (y != 0)
						{
						drawFastHLine<CHECKRANGE>(x0 - x + 1, y0 - y, x + x - 1, interior_color);
						}
					drawFastHLine<CHECKRANGE>(x0 - x + 1, y0 + y, x + x - 1, interior_color);
					}
				}

			if (s >= 0)
				{
				s += fy2 * (1 - x);
				x--;
				}
			s += rx2 * ((4 * y) + 6);
			}
		}


	/** Adapted from Bodmer e_tft library. */
	template<typename color_t>
	template<bool OUTLINE, bool FILL, bool CHECKRANGE>
	void Image<color_t>::_drawEllipse(int x0, int y0, int rx, int ry, color_t outline_color, color_t interior_color, float opacity)
		{
		if (!isValid()) return;
		if (rx < 2) return;
		if (ry < 2) return;
		int32_t x, y;
		int32_t rx2 = rx * rx;
		int32_t ry2 = ry * ry;
		int32_t fx2 = 4 * rx2;
		int32_t fy2 = 4 * ry2;
		int32_t s;
		int yt = ry;
		for (x = 0, y = ry, s = 2 * ry2 + rx2 * (1 - 2 * ry); ry2 * x <= rx2 * y; x++) 
			{
			if (OUTLINE)
				{
				drawPixel<CHECKRANGE>(x0 - x, y0 - y, outline_color, opacity);
				drawPixel<CHECKRANGE>(x0 - x, y0 + y, outline_color, opacity);
				if (x != 0)
					{
					drawPixel<CHECKRANGE>(x0 + x, y0 - y, outline_color, opacity);
					drawPixel<CHECKRANGE>(x0 + x, y0 + y, outline_color, opacity);
					}
				}
			if (s >= 0) 
				{
				s += fx2 * (1 - y);
				y--;
				if (FILL)
					{
					if (ry2 * x <= rx2 * y)
						{
						drawFastHLine<CHECKRANGE>(x0 - x, y0 - y, x + x + 1, interior_color, opacity);
						drawFastHLine<CHECKRANGE>(x0 - x, y0 + y, x + x + 1, interior_color, opacity);
						yt = y;
						}
					}
				}
			s += ry2 * ((4 * x) + 6);
			}
		
		for (x = rx, y = 0, s = 2 * rx2 + ry2 * (1 - 2 * rx); rx2 * y <= ry2 * x; y++) 
			{
			if (OUTLINE)
				{
				drawPixel<CHECKRANGE>(x0 - x, y0 - y, outline_color, opacity);
				drawPixel<CHECKRANGE>(x0 + x, y0 - y, outline_color, opacity);
				if (y != 0)
					{
					drawPixel<CHECKRANGE>(x0 - x, y0 + y, outline_color, opacity);
					drawPixel<CHECKRANGE>(x0 + x, y0 + y, outline_color, opacity);
					}
				}
			if (FILL)
				{
				if (y != yt)
					{
					if (y != 0)
						{
						drawFastHLine<CHECKRANGE>(x0 - x + 1, y0 - y, x + x - 1, interior_color, opacity);
						}
					drawFastHLine<CHECKRANGE>(x0 - x + 1, y0 + y, x + x - 1, interior_color, opacity);
					}
				}

			if (s >= 0)
				{
				s += fy2 * (1 - x);
				x--;
				}
			s += rx2 * ((4 * y) + 6);
			}
		}




	/*****************************************************
	* Antialiased routines
	******************************************************/


	/** Adapted from Bodmer e_tft library. */
	template<typename color_t>
	void Image<color_t>::drawWideLine(float ax, float ay, float bx, float by, float wd, color_t color, float opacity)
		{
		const float LoAlphaTheshold = 64.0f / 255.0f;
		const float HiAlphaTheshold = 1.0f - LoAlphaTheshold;

		if ((abs(ax - bx) < 0.01f) && (abs(ay - by) < 0.01f)) bx += 0.01f;  // Avoid divide by zero

		wd = wd / 2.0f; // wd is now end radius of line

		iBox2 B((int)floorf(fminf(ax, bx) - wd), (int)ceilf(fmaxf(ax, bx) + wd), (int)floorf(fminf(ay, by) - wd), (int)ceilf(fmaxf(ay, by) + wd));
		B &= imageBox();
		if (B.isEmpty()) return;
		// Find line bounding box
		int x0 = B.minX;
		int x1 = B.maxX;
		int y0 = B.minY;
		int y1 = B.maxY;

		// Establish slope direction
		int xs = x0, yp = y1, yinc = -1;
		if ((ax > bx && ay > by) || (ax < bx && ay < by)) { yp = y0; yinc = 1; }

		float alpha = 1.0f; wd += 0.5f;
		int ri = (int)wd;

		float   wd2 = fmaxf(wd - 1.0f, 0.0f);
		wd2 = wd2 * wd2;
		float pax, pay, bax = bx - ax, bay = by - ay;

		// Scan bounding box, calculate pixel intensity from distance to line
		for (int y = y0; y <= y1; y++)
			{
			bool endX = false;                       // Flag to skip pixels
			pay = yp - ay;
			for (int xp = xs; xp <= x1; xp++)
				{
				if (endX) if (alpha <= LoAlphaTheshold) break;  // Skip right side of drawn line
				pax = xp - ax;
				alpha = wd - _wideLineDistance(pax, pay, bax, bay, wd2);
				if (alpha <= LoAlphaTheshold) continue;
				// Track left line boundary
				if (!endX) { endX = true; if ((y > (y0 + ri)) && (xp > xs)) xs = xp; }
				if (alpha > HiAlphaTheshold) { drawPixel(xp, yp, color, opacity); continue; }
				//Blend colour with background and plot
				drawPixel(xp, yp, color, alpha * opacity);
				}
			yp += yinc;
			}
		}



	/** Adapted from Bodmer e_tft library. */
	template<typename color_t>
	void Image<color_t>::drawWedgeLine(float ax, float ay, float bx, float by, float aw, float bw, color_t color, float opacity)
		{
		const float LoAlphaTheshold = 64.0f / 255.0f;
		const float HiAlphaTheshold = 1.0f - LoAlphaTheshold;

		if ((abs(ax - bx) < 0.01f) && (abs(ay - by) < 0.01f)) bx += 0.01f;  // Avoid divide by zero

		aw = aw / 2.0f;
		bw = bw / 2.0f;

		iBox2 B((int)floorf(fminf(ax - aw, bx - bw)), (int)ceilf(fmaxf(ax + aw, bx + bw)), (int)floorf(fminf(ay - aw, by - bw)), (int)ceilf(fmaxf(ay + aw, by + bw)));
		B &= imageBox();
		if (B.isEmpty()) return;
		// Find line bounding box
		int x0 = B.minX;
		int x1 = B.maxX;
		int y0 = B.minY;
		int y1 = B.maxY;

		// Establish slope direction
		int xs = x0, yp = y1, yinc = -1;
		if (((ax - aw) > (bx - bw) && (ay > by)) || ((ax - aw) < (bx - bw) && ay < by)) { yp = y0; yinc = 1; }

		bw = aw - bw; // Radius delta
		float alpha = 1.0f; aw += 0.5f;
		int ri = (int)aw;
		float pax, pay, bax = bx - ax, bay = by - ay;

		// Scan bounding box, calculate pixel intensity from distance to line
		for (int y = y0; y <= y1; y++)
			{
			bool endX = false;                       // Flag to skip pixels
			pay = yp - ay;
			for (int32_t xp = xs; xp <= x1; xp++)
				{
				if (endX) if (alpha <= LoAlphaTheshold) break;  // Skip right side of drawn line
				pax = xp - ax;
				alpha = aw - _wedgeLineDistance(pax, pay, bax, bay, bw);
				if (alpha <= LoAlphaTheshold) continue;
				// Track left line segment boundary
				if (!endX) { endX = true; if ((y > (y0 + ri)) && (xp > xs)) xs = xp; }
				if (alpha > HiAlphaTheshold) { drawPixel(xp, yp, color, opacity);  continue; }
				//Blend color with background and plot
				drawPixel(xp, yp, color, alpha * opacity);
				}
			yp += yinc;
			}
		}





	/************************************************************************************
	* 
	*  Drawing Text
	* 
	*************************************************************************************/


	template<typename color_t>
	uint32_t Image<color_t>::_fetchbits_unsigned(const uint8_t* p, uint32_t index, uint32_t required)
		{
		uint32_t val;
		uint8_t* s = (uint8_t*)&p[index >> 3];
	#ifdef UNALIGNED_IS_SAFE		// is this defined anywhere ? 
		val = *(uint32_t*)s; // read 4 bytes - unaligned is ok
		val = __builtin_bswap32(val); // change to big-endian order
	#else
		val = s[0] << 24;
		val |= (s[1] << 16);
		val |= (s[2] << 8);
		val |= s[3];
	#endif
		val <<= (index & 7); // shift out used bits
		if (32 - (index & 7) < required) 
			{ // need to get more bits
			val |= (s[4] >> (8 - (index & 7)));
			}
		val >>= (32 - required); // right align the bits
		return val;
		}


	template<typename color_t>
	uint32_t Image<color_t>::_fetchbits_signed(const uint8_t* p, uint32_t index, uint32_t required)
		{
		uint32_t val = _fetchbits_unsigned(p, index, required);
		if (val & (1 << (required - 1))) 
			{
			return (int32_t)val - (1 << required);
			}
		return (int32_t)val;
		}


	template<typename color_t>
	bool Image<color_t>::_clipit(int& x, int& y, int& sx, int& sy, int & b_left, int & b_up)
		{
		b_left = 0;
		b_up = 0; 
		if ((sx < 1) || (sy < 1) || (y >= _ly) || (y + sy <= 0) || (x >= _lx) || (x + sx <= 0))
			{ // completely outside of image
			return false;
			}
		if (y < 0)
			{
			b_up = -y;
			sy += y;
			y = 0;
			}
		if (y + sy > _ly)
			{
			sy = _ly - y;
			}
		if (x < 0)
			{
			b_left = -x;
			sx += x;
			x = 0;
			}
		if (x + sx > _lx)
			{
			sx = _lx - x;
			}
		return true;
		}


	template<typename color_t>
	iBox2 Image<color_t>::measureChar(char c, iVec2 pos, const GFXfont& font, int * xadvance)
		{
		uint8_t n = (uint8_t)c;
		if ((n < font.first) || (n > font.last)) return iBox2(); // nothing to draw. 
		auto& g = font.glyph[n - font.first];
		int x = pos.x + g.xOffset;
		int y = pos.y + g.yOffset;
		int sx = g.width;
		int sy = g.height;
		if (xadvance) *xadvance = g.xAdvance;
		return iBox2(x, x + sx - 1, y, y + sy - 1);
		}


	template<typename color_t>
	template<bool BLEND> iVec2 Image<color_t>::_drawCharGFX(char c, iVec2 pos, color_t col, const GFXfont& font, float opacity)
		{
		uint8_t n = (uint8_t)c;
		if ((n < font.first) || (n > font.last)) return pos; // nothing to draw. 
		auto& g = font.glyph[n - font.first];
		if ((!isValid()) || (font.bitmap == nullptr)) return pos;
		int x = pos.x + g.xOffset;
		int y = pos.y + g.yOffset;
		int sx = g.width;
		int sy = g.height;
		const int rsx = sx;	// save the real bitmap width; 
		int b_left, b_up;
		if (!_clipit(x, y, sx, sy, b_left, b_up)) return iVec2(pos.x + g.xAdvance, pos.y);
		_drawCharBitmap_1BPP<BLEND>(font.bitmap + g.bitmapOffset, rsx, b_up, b_left, sx, sy, x, y, col, opacity);
		return iVec2(pos.x + g.xAdvance, pos.y);
		}


	template<typename color_t>
	iBox2 Image<color_t>::measureText(const char* text, iVec2 pos, const GFXfont& font, bool start_newline_at_0)
		{
		const int startx = start_newline_at_0 ? 0 : pos.x;
		iBox2 B;
		B.empty();
		const size_t l = strlen(text);
		for (size_t i = 0; i < l; i++)
			{
			const char c = text[i];
			if (c == '\n')
				{
				pos.x = startx;
				pos.y += font.yAdvance;
				}
			else
				{
				int xa = 0;
				B |= measureChar(c, pos, font, &xa);
				pos.x += xa;
				}
			}
		return B; 
		}


	template<typename color_t>
	template<bool BLEND>
	iVec2 Image<color_t>::_drawTextGFX(const char* text, iVec2 pos, color_t col, const GFXfont& font, bool start_newline_at_0, float opacity)
		{
		const int startx = start_newline_at_0 ? 0 : pos.x;
		const size_t l = strlen(text);
		for (size_t i = 0; i < l; i++)
			{
			const char c = text[i];
			if (c == '\n')
				{
				pos.x = startx;
				pos.y += font.yAdvance;
				}
			else
				{
				pos = _drawCharGFX<BLEND>(c, pos, col, font, opacity);
				}
			}
		return pos;
		}


	template<typename color_t>
	iBox2 Image<color_t>::measureChar(char c, iVec2 pos, const ILI9341_t3_font_t& font, int* xadvance)
		{
		uint8_t n = (uint8_t)c;
		if ((n >= font.index1_first) && (n <= font.index1_last))
			{
			n -= font.index1_first;
			}
		else if ((n >= font.index2_first) && (n <= font.index2_last))
			{
			n = (n - font.index2_first) + (font.index1_last - font.index1_first + 1);
			}
		else
			{ // no char to draw
			return iBox2(); // nothing to draw. 
			}
		uint8_t* data = (uint8_t*)font.data + _fetchbits_unsigned(font.index, (n * font.bits_index), font.bits_index);
		int32_t off = 0;
		uint32_t encoding = _fetchbits_unsigned(data, off, 3);
		if (encoding != 0) return  pos; // wrong/unsupported format
		off += 3;
		const int sx = (int)_fetchbits_unsigned(data, off, font.bits_width);
		off += font.bits_width;
		const int sy = (int)_fetchbits_unsigned(data, off, font.bits_height);
		off += font.bits_height;
		const int xoffset = (int)_fetchbits_signed(data, off, font.bits_xoffset);
		off += font.bits_xoffset;
		const int yoffset = (int)_fetchbits_signed(data, off, font.bits_yoffset);
		off += font.bits_yoffset;
		if (xadvance)
			{
			*xadvance = (int)_fetchbits_unsigned(data, off, font.bits_delta);
			}
		const int x = pos.x + xoffset;
		const int y = pos.y - sy - yoffset;
		return iBox2(x, x + sx - 1, y, y + sy - 1);
		}



	template<typename color_t>
	template<bool BLEND> iVec2 Image<color_t>::_drawCharILI(char c, iVec2 pos, color_t col, const ILI9341_t3_font_t& font, float opacity)
		{
		if (!isValid()) return pos;
		uint8_t n = (uint8_t)c;
		if ((n >= font.index1_first) && (n <= font.index1_last))
			{
			n -= font.index1_first;
			}
		else if ((n >= font.index2_first) && (n <= font.index2_last))
			{
			n = (n - font.index2_first) + (font.index1_last - font.index1_first + 1);
			}
		else
			{ // no char to draw
			return pos;
			}
		uint8_t * data = (uint8_t *)font.data + _fetchbits_unsigned(font.index, (n*font.bits_index), font.bits_index);
		int32_t off = 0; 
		uint32_t encoding = _fetchbits_unsigned(data, off, 3);
		if (encoding != 0) return  pos; // wrong/unsupported format
		off += 3;
		int sx = (int)_fetchbits_unsigned(data, off, font.bits_width);
		off += font.bits_width;			
		int sy = (int)_fetchbits_unsigned(data, off, font.bits_height);
		off += font.bits_height;			
		const int xoffset = (int)_fetchbits_signed(data, off, font.bits_xoffset);
		off += font.bits_xoffset;
		const int yoffset = (int)_fetchbits_signed(data, off, font.bits_yoffset);
		off += font.bits_yoffset;
		const int delta = (int)_fetchbits_unsigned(data, off, font.bits_delta);
		off += font.bits_delta;
		int x = pos.x + xoffset;
		int y = pos.y - sy - yoffset;
		const int rsx = sx;	// save the real bitmap width; 
		int b_left, b_up;
		if (!_clipit(x, y, sx, sy, b_left, b_up)) return iVec2(pos.x + delta, pos.y);
		if (font.version == 1)
			{ // non-antialiased font
			_drawCharILI9341_t3<BLEND>(data, off, rsx, b_up, b_left, sx, sy, x, y, col, opacity);
			}
		else if (font.version == 23)
			{ // antialised font
			data += (off >> 3) + ((off & 7) ? 1 : 0); // bitmap begins at the next byte boundary
			switch (font.reserved)
				{
				case 0: 
					_drawCharBitmap_1BPP<BLEND>(data, rsx, b_up, b_left, sx, sy, x, y, col, opacity);
					break;
				case 1:
					_drawCharBitmap_2BPP<BLEND>(data, rsx, b_up, b_left, sx, sy, x, y, col, opacity);
					break;
				case 2:
					_drawCharBitmap_4BPP<BLEND>(data, rsx, b_up, b_left, sx, sy, x, y, col, opacity);
					break;
				case 3:
					_drawCharBitmap_8BPP<BLEND>(data, rsx, b_up, b_left, sx, sy, x, y, col, opacity);
					break;
				}
			}
		return iVec2(pos.x + delta, pos.y);
		}


	template<typename color_t>
	iBox2 Image<color_t>::measureText(const char* text, iVec2 pos, const ILI9341_t3_font_t& font, bool start_newline_at_0)
		{
		const int startx = start_newline_at_0 ? 0 : pos.x;
		iBox2 B;
		B.empty();
		const size_t l = strlen(text);
		for (size_t i = 0; i < l; i++)
			{
			const char c = text[i];
			if (c == '\n')
				{
				pos.x = startx;
				pos.y += font.line_space;
				}
			else
				{
				int xa = 0;
				B |= measureChar(c, pos, font, &xa);
				pos.x += xa;
				}
			}
		return B;
		}


	template<typename color_t>
	template<bool BLEND>
	iVec2 Image<color_t>::_drawTextILI(const char* text, iVec2 pos, color_t col, const ILI9341_t3_font_t& font, bool start_newline_at_0, float opacity)
		{
		const int startx = start_newline_at_0 ? 0 : pos.x;
		const size_t l = strlen(text);
		for (size_t i = 0; i < l; i++)
			{
			const char c = text[i];
			if (c == '\n')
				{
				pos.x = startx;
				pos.y += font.line_space;
				}
			else
				{
				pos = _drawCharILI<BLEND>(c, pos, col, font, opacity);
				}
			}
		return pos;
		}


	template<typename color_t>
	template<bool BLEND>
	void Image<color_t>::_drawCharILI9341_t3(const uint8_t* bitmap, int32_t off, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col, float opacity)
		{
		uint32_t rl = 0; // number of line repeat remaining. 
		while (b_up > 0)
			{ // need to skip lines
			if (_fetchbit(bitmap, off++))
				{ // this is a repeating line
				int n = (int)_fetchbits_unsigned(bitmap, off, 3) + 2; // number of repetition
				if (n <= b_up) 
					{  
					b_up -= n;
					off += (rsx + 3);
					}
				else
					{
					rl = (n - b_up); // number of repeat remaining (at least 1)
					off += 3; // beginning of the line pixels
					b_up = 0;
					break;
					}
				}
			else
				{ // skipping a single line
				b_up--; 
				off += rsx; 
				}
			}

		while (sy-- > 0)
			{ // iterate over lines to draw
			if (rl == 0)
				{ // need to read the line header. 
				if (_fetchbit(bitmap, off++))
					{ // repeating line
					rl = _fetchbits_unsigned(bitmap, off, 3) + 2; // number of repetition
					off += 3; // go the the beginning of the line pixels
					}
				else
					{
					rl = 1; // repeat only once, already at beginning of line pixels 
					}
				}
			// off is now pointing to the begining of the line pixels and we can draw it. 
			_drawcharline<BLEND>(bitmap, off + b_left, _buffer + TGX_CAST32(x) + TGX_CAST32(y) * TGX_CAST32(_stride), sx, col, opacity);
			if ((--rl) == 0)
				{ // done repeating so we move to the next line in the bitmap
				off += rsx;
				}
			y++; // next line in the destination image
			}
		}


	template<typename color_t>
	template<bool BLEND>
	void Image<color_t>::_drawcharline(const uint8_t* bitmap, int32_t off, color_t* p, int dx, color_t col, float opacity)
		{
		bitmap += (off >> 3);						// move to the correct byte
		uint8_t u = (uint8_t)(128 >> (off & 7));	// index of the first bit
		if (dx >= 8)
			{ // line has at least 8 pixels
			if (u != 128)
				{ // not at the start of a bitmap byte: we first finish it. 
				const uint8_t b = *(bitmap++); // increment bitmap now since we know we will finish this byte
				while (u > 0)
					{
					if (b & u) { if (BLEND) { (*p).blend(col, opacity);  } else { *p = col; } }
					p++; dx--; u >>= 1;
					}
				u = 128;
				}
			while (dx >= 8)
				{ // now we can write 8 pixels consecutively. 
				const uint8_t b = *(bitmap++);
				if (b)
					{ // there is something to write
					if (b & 128) {if (BLEND) { p[0].blend(col, opacity); } else { p[0] = col; }}
					if (b & 64) { if (BLEND) { p[1].blend(col, opacity); } else { p[1] = col; } }
					if (b & 32) { if (BLEND) { p[2].blend(col, opacity); } else { p[2] = col; } }
					if (b & 16) { if (BLEND) { p[3].blend(col, opacity); } else { p[3] = col; } }
					if (b & 8) { if (BLEND) { p[4].blend(col, opacity); } else { p[4] = col; } }
					if (b & 4) { if (BLEND) { p[5].blend(col, opacity); } else { p[5] = col; } }
					if (b & 2) { if (BLEND) { p[6].blend(col, opacity); } else { p[6] = col; } }
					if (b & 1) { if (BLEND) { p[7].blend(col, opacity); } else { p[7] = col; } }
					}
				p += 8;
				dx -= 8;
				}
			// strictly less than 8 pixels remain 
			if (dx > 0)
				{
				const uint8_t b = *bitmap;
				if (b)					
					{
					do
						{
						if (b & u) { if (BLEND) { (*p).blend(col, opacity); } else { *p = col; } }
						p++; dx--; u >>= 1;
					} while (dx > 0);
					}
				}
			}
		else
			{ // each row has less than 8 pixels
			if ((u >> (dx - 1)) == 0)
				{ // we cannot complete the line with this byte
				const uint8_t b = *(bitmap++); // increment bitmap now since we know we will finish this byte
				while (u > 0)
					{
					if (b & u) { if (BLEND) { (*p).blend(col, opacity); } else { *p = col; } }
					p++; dx--; u >>= 1;
					}
				u = 128;
				}
			if (dx > 0)
				{
				const uint8_t b = *bitmap; // we know we will complete the line with this byte
				if (b)
					{ // there is something to draw
					do
						{
						if (b & u) { if (BLEND) { (*p).blend(col, opacity); } else { *p = col; } }
						p++; dx--;  u >>= 1;
					} while (dx > 0);
					}
				}
			}
		}


	template<typename color_t>
	template<bool BLEND>
	void Image<color_t>::_drawCharBitmap_1BPP(const uint8_t* bitmap, int rsx, int b_up, int b_left, int sx, int sy,int x, int y, color_t col, float opacity)
		{
		int32_t off = TGX_CAST32(b_up) * TGX_CAST32(rsx) + TGX_CAST32(b_left);
		bitmap += (off >> 3);						// starting byte in the bitmap
		uint8_t u = (uint8_t)(128 >> (off & 7));	// index of the first bit 
		const int sk = (rsx - sx);				// number of bits to skip at the end of a row.
		color_t* p = _buffer + TGX_CAST32(x) + TGX_CAST32(_stride) * TGX_CAST32(y); // start position in destination buffer
		if (sx >= 8)
			{ // each row has at least 8 pixels
			for (int dy = sy; dy > 0; dy--)
				{
				int dx = sx; // begining of row, number of char to write
				if (u != 128)
					{ // not at the start of a bitmap byte: we first finish it. 
					const uint8_t b = *(bitmap++); // increment bitmap now since we know we will finish this byte
					while (u > 0)
						{
						if (b & u) { if (BLEND) { (*p).blend(col, opacity); } else { *p = col; } }
						p++; dx--; u >>= 1;
						}
					u = 128;
					}
				while (dx >= 8)
					{ // now we can write 8 pixels consecutively. 
					const uint8_t b = *(bitmap++);
					if (b)
						{
						if (b & 128) { if (BLEND) { p[0].blend(col, opacity); } else { p[0] = col; } }
						if (b & 64) { if (BLEND) { p[1].blend(col, opacity); } else { p[1] = col; } }
						if (b & 32) { if (BLEND) { p[2].blend(col, opacity); } else { p[2] = col; } }
						if (b & 16) { if (BLEND) { p[3].blend(col, opacity); } else { p[3] = col; } }
						if (b & 8) { if (BLEND) { p[4].blend(col, opacity); } else { p[4] = col; } }
						if (b & 4) { if (BLEND) { p[5].blend(col, opacity); } else { p[5] = col; } }
						if (b & 2) { if (BLEND) { p[6].blend(col, opacity); } else { p[6] = col; } }
						if (b & 1) { if (BLEND) { p[7].blend(col, opacity); } else { p[7] = col; } }
						}
					p += 8;
					dx -= 8;
					}
				// strictly less than 8 pixels remain on the row 
				if (dx > 0)
					{
					const uint8_t b = *bitmap; // do not increment bitmap now since we know we will not finish this byte now.
					do
						{
						if (b & u) { if (BLEND) { (*p).blend(col, opacity); } else { *p = col; } }
						p++; dx--; u >>= 1;
						} while (dx > 0);
					}
				// row is now complete
				p += (_stride - sx); // go to the beginning of the next row
				if (sk != 0)
					{ // we must skip some pixels...
					bitmap += (sk >> 3);
					uint16_t v = (((uint16_t)u) << (8 - (sk & 7)));
					if (v & 255)
						{
						u = (uint8_t)(v & 255);
						bitmap++;
						}
					else
						{
						u = (uint8_t)(v >> 8);
						}
					}
				}
			}
		else
			{ // each row has less than 8 pixels
			for (int dy = sy; dy > 0; dy--)
				{
				int dx = sx;
				if ((u >> (sx - 1)) == 0)
					{ // we cannot complete the row with this byte
					const uint8_t b = *(bitmap++); // increment bitmap now since we know we will finish this byte
					while (u > 0)
						{
						if (b & u) { if (BLEND) { (*p).blend(col, opacity); } else { *p = col; } }
						p++; dx--; u >>= 1;
						}
					u = 128;
					}
				if (dx > 0)
					{
					const uint8_t b = *bitmap; // we know we will complete the row with this byte
					do
						{
						if (b & u) { if (BLEND) { (*p).blend(col, opacity); } else { *p = col; } }
						p++; dx--;  u >>= 1;
						} while (dx > 0);
					}
				if (u == 0) { bitmap++; u = 128; }
				// row is now complete
				p += (_stride - sx); // go to the beginning of the next row
				if (sk != 0)
					{ // we must skip some pixels...
					bitmap += (sk >> 3);
					uint16_t v = (((uint16_t)u) << (8 - (sk & 7)));
					if (v & 255)
						{
						u = (uint8_t)(v & 255);
						bitmap++;
						}
					else
						{
						u = (uint8_t)(v >> 8);
						}
					}
				}
			}
		}


	template<typename color_t>
	template<bool BLEND>
	void Image<color_t>::_drawCharBitmap_2BPP(const uint8_t* bitmap, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col, float opacity)
		{ 
		int iop = 171 * (int)(256 * opacity);
		if (sx >= 4)
			{ // each row has at least 4 pixels
			for (int dy = 0; dy < sy; dy++)
				{
				int32_t off = TGX_CAST32(b_up + dy) * TGX_CAST32(rsx) + TGX_CAST32(b_left);
				color_t* p = _buffer + TGX_CAST32(_stride) * TGX_CAST32(y + dy) + TGX_CAST32(x); 
				int dx = sx;
				const int32_t uu = off & 3;
				if (uu)
					{// not at the start of a bitmap byte: we first finish it. 
					const uint8_t b = bitmap[off >> 2];
					switch (uu)
						{
						case 1: { const int v = ((b & 48) >> 4); (p++)->blend256(col, (v * iop) >> 9); off++; dx--; }
						case 2: { const int v = ((b & 12) >> 2); (p++)->blend256(col, (v * iop) >> 9); off++; dx--; }
						case 3: { const int v = (b & 3); (p++)->blend256(col, (v * iop) >> 9); off++; dx--; }
						}
					}
				while (dx >= 4)
					{ // now we can write 4 pixels consecutively. 
					const uint8_t b = bitmap[off >> 2];
					if (b)
						{
							{ const int v = ((b & 192) >> 6); p[0].blend256(col, (v * iop) >> 9); }
							{ const int v = ((b & 48) >> 4); p[1].blend256(col, (v * iop) >> 9); }
							{ const int v = ((b & 12) >> 2); p[2].blend256(col, (v * iop) >> 9); }
							{ const int v = (b & 3); p[3].blend256(col, (v * iop) >> 9); }
						}
					off += 4;
					p += 4;
					dx -= 4;
					}
				// strictly less than 4 pixels remain on the row 
				if (dx > 1)
					{
					const uint8_t b = bitmap[off >> 2];
					{const int v = ((b & 192) >> 6); (p++)->blend256(col, (v * iop) >> 9); }
					{const int v = ((b & 48) >> 4); (p++)->blend256(col, (v * iop) >> 9); }
					if (dx > 2) { const int v = ((b & 12) >> 2); (p++)->blend256(col, (v * iop) >> 9); }
					}
				else
					{
					if (dx > 0) 
						{ 
						const uint8_t b = bitmap[off >> 2];
						const int v = ((b & 192) >> 6); (p++)->blend256(col, (v * iop) >> 9); 
						}
					}					
				}
			}
		else
			{ // each row has less than 4 pixels
			for (int dy = 0; dy < sy; dy++)
				{
				int32_t off = TGX_CAST32(b_up + dy) * TGX_CAST32(rsx) + TGX_CAST32(b_left); // offset for the start of this line
				color_t* p = _buffer + TGX_CAST32(_stride) * TGX_CAST32(y + dy) + TGX_CAST32(x); // start position in destination buffer
				int dx = sx;
				const int32_t uu = off & 3;
				if ((4 - uu) < sx)
					{ // we cannot complete the row with this byte
					const uint8_t b = bitmap[off >> 2];
					switch (uu)
						{
						case 1: { const int v = ((b & 48) >> 4); (p++)->blend256(col, (v * iop) >> 9); off++; dx--; } // this case should never occur
						case 2: { const int v = ((b & 12) >> 2); (p++)->blend256(col, (v * iop) >> 9); off++; dx--; }
						case 3: { const int v = (b & 3); (p++)->blend256(col, (v * iop) >> 9); off++; dx--; }
						}
					}	
				if (dx > 0)
					{
					const uint8_t b = bitmap[off >> 2];
					while (dx-- > 0)
						{
						const int32_t uu = (off++) & 3;
						switch (uu)
							{
							case 0: { const int v = ((b & 192) >> 6); (p++)->blend256(col, (v * iop) >> 9); break; }
							case 1: { const int v = ((b & 48) >> 4); (p++)->blend256(col, (v * iop) >> 9); break; }
							case 2: { const int v = ((b & 12) >> 2); (p++)->blend256(col, (v * iop) >> 9); break; }
							case 3: { const int v = (b & 3); (p++)->blend256(col, (v * iop) >> 9); break; }
							}
						}
					}
				}
			}
		}


	template<typename color_t>
	template<bool BLEND>
	void Image<color_t>::_drawCharBitmap_4BPP(const uint8_t* bitmap, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col, float opacity)
		{ 
		int iop = 137 * (int)(256 * opacity);
		if (sx >= 2)
			{ // each row has at least 2 pixels
			for (int dy = 0; dy < sy; dy++)
				{
				int32_t off = TGX_CAST32(b_up + dy) * TGX_CAST32(rsx) + TGX_CAST32(b_left);
				color_t* p = _buffer + TGX_CAST32(_stride) * TGX_CAST32(y + dy) + TGX_CAST32(x); 
				int dx = sx;
				if (off & 1)
					{// not at the start of a bitmap byte: we first finish it. 
					const uint8_t b = bitmap[off >> 1];
				    const int v = (b & 15); (p++)->blend256(col, (v * iop) >> 11); 
					off++; dx--; 
					}
				while (dx >= 2)
					{
					const uint8_t b = bitmap[off >> 1];
					if (b)
						{
							{ const int v = ((b & 240) >> 4); p[0].blend256(col, (v * iop) >> 11); }
							{ const int v = (b & 15); p[1].blend256(col, (v * iop) >> 11); }
						}
					off += 2;
					p += 2;
					dx -= 2;
					}
				if (dx > 0)
					{
					const uint8_t b = bitmap[off >> 1];					
					const int v = ((b & 240) >> 4); p->blend256(col, (v * iop) >> 11);
					}
				}
			}
		else
			{ // each row has a single pixel 
			color_t* p = _buffer + TGX_CAST32(_stride) * TGX_CAST32(y) + TGX_CAST32(x);
			int32_t off = TGX_CAST32(b_up) * TGX_CAST32(rsx) + TGX_CAST32(b_left);
			while(sy-- > 0)
				{
				const uint8_t b = bitmap[off >> 1];
				const int v = (off & 1) ? (b & 15) : ((b & 240) >> 4);
				p->blend256(col, (v * iop) >> 11);
				p += _stride;
				off += rsx;
				}
			}
		}


	template<typename color_t>
	template<bool BLEND>
	void Image<color_t>::_drawCharBitmap_8BPP(const uint8_t* bitmap, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col, float opacity)
		{
		int iop = 129 * (int)(256 * opacity);
		const uint8_t * p_src = bitmap + TGX_CAST32(b_up) * TGX_CAST32(rsx) + TGX_CAST32(b_left);
		color_t * p_dst = _buffer + TGX_CAST32(x) + TGX_CAST32(_stride) * TGX_CAST32(y);
		const int sk_src = rsx - sx;
		const int sk_dst = _stride - sx;
		while (sy-- > 0)
			{
			int dx = sx;
			while (dx-- > 0)
				{
				uint32_t cc = *(p_src++);
				(*(p_dst++)).blend256(col, (cc*iop) >> 15); 
				}
			p_src += sk_src;
			p_dst += sk_dst;
			}
		}



#undef TGX_CAST32


}

#endif

#endif

/** end of file */


