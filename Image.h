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

#pragma once


#include "Misc.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"
#include "Box2.h"
#include "Color.h"

#include <stdint.h>



/****************************************************************************
* FONT FORMATS
* 
* !!! The Adafruit GFX and ILI9341_t3 libraries should be included before 
*     this header !!!
****************************************************************************/


/**
* ADAFRUIT FONT
* Documentation on the Adafruit font format:
* https://glenviewsoftware.com/projects/products/adafonteditor/adafruit-gfx-font-format/
**/

#if __has_include(<gfxfont.h>)
#include <gfxfont.h>
#endif

#ifndef _GFXFONT_H_
#define _GFXFONT_H_

// Font data stored PER GLYPH
typedef struct {
	uint16_t bitmapOffset; // Pointer into GFXfont->bitmap
	uint8_t width;         // Bitmap dimensions in pixels
	uint8_t height;        // Bitmap dimensions in pixels
	uint8_t xAdvance;      // Distance to advance cursor (x axis)
	int8_t xOffset;        // X dist from cursor pos to UL corner
	int8_t yOffset;        // Y dist from cursor pos to UL corner
} GFXglyph;


// Data stored for FONT AS A WHOLE
typedef struct {
	uint8_t* bitmap;  // Glyph bitmaps, concatenated
	GFXglyph* glyph;  // Glyph array
	uint8_t first;    // ASCII extents (first char)
	uint8_t last;     // ASCII extents (last char)
	uint8_t yAdvance; // Newline distance (y axis)
} GFXfont;

#endif 



/**
* ILI9341_t3 PJRC FONT FORMAT
* Documentation on the ILI9341_t3 font data format:
* https://forum.pjrc.com/threads/54316-ILI9341_t-font-structure-format (basic version)
* https://github.com/projectitis/packedbdf/blob/master/packedbdf.md (anti-aliased extension)
**/

#if __has_include(<ILI9341_t3.h>)
#include <ILI9341_t3.h>
#endif

#ifndef _ILI9341_t3H_
#define _ILI9341_t3H_


typedef struct {
	const unsigned char* index;
	const unsigned char* unicode;
	const unsigned char* data;
	unsigned char version;
	unsigned char reserved;
	unsigned char index1_first;
	unsigned char index1_last;
	unsigned char index2_first;
	unsigned char index2_last;
	unsigned char bits_index;
	unsigned char bits_width;
	unsigned char bits_height;
	unsigned char bits_xoffset;
	unsigned char bits_yoffset;
	unsigned char bits_delta;
	unsigned char line_space;
	unsigned char cap_height;
} ILI9341_t3_font_t;

#endif 





namespace tgx
{

	/** always cast indexes as 32bit when doing pointer arithmetic */
	#define CAST32(a)	((int32_t)a)

		

	/************************************************************************************
	* Template class for an "image object" that draws into a memory buffer.   
	*  
	* The class object itself is very small (16 bytes) thus image objects can easily be 
	* passed around/copied without much cost. Also, sub-images sharing the same memory 
	* buffer can be created and are particularly useful for clipping drawing to a given 
	* region of the image. 
	*
	* No memory allocation is performed by the class: it is the user's responsability to
	* allocate memory and supply and assign the buffer to the image.   
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
	* The memory buffer should be supplied at creation. Otherwise, the image is set as 
	* invalid until a valid buffer is supplied. 
	* 
	*************************************************************************************/


		/** 
		* Create default invalid/empty image. 
		**/
		Image() : _buffer(nullptr), _lx(0), _ly(0), _stride(0)
			{
			}


		/**
		* Ctor, create an image with a given size and a given buffer
		* (if the stride is omitted, it defaults to stride = lx);
		***/
		constexpr Image(color_t* buffer, int lx, int ly, int stride = -1) : _buffer(buffer), _lx(lx), _ly(ly), _stride(stride < 0 ? lx : stride)
			{
			_checkvalid(); // make sure dimension/stride are ok else make the image invalid
			}


		/**
		* Ctor, create an image with a given size and a given buffer
		* (if the stride is omitted, it defaults to stride = lx);
		***/
		constexpr Image(color_t* buffer, iVec2 dim, int stride = -1) : Image(buffer, dim.x, dim.y, stride)
			{
			}


		/**
		* Ctor, create an image with a given size and a given buffer.
		* (if the stride is omitted, it defaults to stride = lx);
		***/
		template<typename T> constexpr Image(T * buffer, int lx, int ly, int stride = -1) : Image((color_t*)buffer, lx, ly, stride)
			{
			}


		/**
		* Ctor, create an image with a given size and a given buffer
		* (if the stride is omitted, it defaults to stride = lx);
		***/
		template<typename T> constexpr Image(T * buffer, iVec2 dim, int stride = -1) : Image((color_t*)buffer, dim.x, dim.y, stride)
			{
			}


		/**
		* Ctor, create a sub-image of a given image, sharing the same buffer.
		* - if clamp if true, subbox is intersected with the image box to create a valid subbox.
		* - if clamp = false and subbox does not already fit inside the image box, an empty image is created.
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
			_buffer = im._buffer + CAST32(subbox.minX) + CAST32(im._stride)* CAST32(subbox.minY);
			}


		/** 
		* Default copy ctor. (shallow copy: both images reference the same memory buffer) 
		**/
		Image(const Image<color_t> & im) = default;


		/** 
		* Default assignement operator. (shallow copy: both images reference the same memory buffer)
		**/
		Image& operator=(const Image<color_t> & im) = default;


		/**
		* Crop this image by keeping only the region represented by subbox (intersected with the 
		* image box).
		**/
		inline void crop(const iBox2& subbox)
			{
			*this = Image<color_t>(*this, subbox, true);
			}


		/**
		* Return a sub-image of this image. 
		* if clamp = true: intersect subbox with imageBox() if needed.
		* if clamp = false: return an invalid image if subbox is not included in imageBox().
		**/
		inline Image<color_t> getCrop(const iBox2& subbox, bool clamp = true) const
			{
			return Image<color_t>(*this, subbox, clamp);
			}


		/**
		* Set/update the buffer, size and stride of the image.
		* (if the stride is omitted, it defaults to stride = lx);
		***/
		inline void set(color_t * buffer, int lx, int ly, int stride = -1)
			{
			_buffer = buffer;
			_lx = lx;
			_ly = ly;
			_stride = (stride < 0) ? lx : stride;
			_checkvalid(); // make sure dimension/stride are ok else make the image invalid
			}


		/**
		* Set/update the buffer, size and stride of the image. 
		* (if the stride is omitted, it defaults to stride = lx);
		***/
		inline void set(void* buffer, int lx, int ly, int stride = -1)
			{
			set((color_t*)buffer, lx, ly, stride);
			}


		/**
		* Set/update the buffer, size and stride of the image.
		* (if the stride is omitted, it defaults to stride = lx);
		***/
		inline void set(color_t* buffer, iVec2 dim, int stride = -1)
			{
			set(buffer, dim.x, dim.y, stride);
			}


		/**
		* Set/update the buffer, size and stride of the image.
		* (if the stride is omitted, it defaults to stride = lx);
		***/
		inline void set(void* buffer, iVec2 dim, int stride = -1)
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
		* Return the image width (0 for an invalid image).
		**/
		inline int width() const { return _lx; }


		/** 
		* Return the image height (0 for an invalid image). 
		**/
		inline int height() const { return _ly; }


		/** 
		* Return the image stride  (0 for an invalid image).
		***/
		inline int stride() const { return _stride; }


		/** 
		* Return the image dimensions as a vector  ((0,0) for an invalid image).
		***/
		inline iVec2 dim() const { return iVec2{ _lx, _ly }; }


		/** 
		* Return the image box of the form {0, width-1, 0 height-1 } 
		* or an empty box (0,-1,0,-1) if the image is invalid. 
		***/
		inline iBox2 imageBox() const { return iBox2(0, _lx - 1, 0, _ly - 1); }


		/** 
		* Return a pointer to the pixel buffer (const version). 
		* Return nullptr if the image is invalid.
		**/
		inline const color_t * data() const { return _buffer; }


		/** 
		* Return a pointer to the pixel buffer (non-const version) 
		* Return nullptr if the image is invalid.
		**/
		inline color_t * data() { return _buffer; }


		/** 
		* Query if the image is valid.
		***/
		inline bool isValid() const { return (_buffer != nullptr); }


		/** 
		 * Set the image as invalid.
		 **/
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
		* Blit a sprite at a given position on this image
		* upperleftpos : upper left corner (on this image) where blitting occurs.
		**/
		inline void blit(const Image<color_t> & sprite, iVec2 upperleftpos)
			{
			blit(sprite, upperleftpos.x, upperleftpos.y, 0, 0, sprite.width(), sprite.height());
			}


		/**
		* Blit a sprite at a given position on this image
		* upperleftpos : upper left corner (on this image) where blitting occurs.
		**/
		inline void blit(const Image<color_t> & sprite, int dest_x = 0, int dest_y = 0)
			{
			blit(sprite, dest_x, dest_y, 0, 0, sprite.width(), sprite.height());
			}


		/**
		* Blit (part of) a sprite at a given position on this image
		* upperleftpos : upper left corner (on this image) where blitting start.
		* sprite_subbox: delimits the sub-image of sprite than should be blit.
		**/
		inline void blit(const Image<color_t> & sprite, iVec2 upperleftpos, const iBox2 & sprite_subbox)
			{
			blit(sprite, upperleftpos.x, upperleftpos.y, sprite_subbox.minX, sprite_subbox.minY, sprite_subbox.lx(), sprite_subbox.ly());
			}


		/**
		* Blit (part of) a sprite onto a given position in this image
		**/
		void blit(const Image<color_t> & sprite, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy);


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
		inline Image<color_t> reduceHalf()
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
		* use template parameter to disable range check for faster access.
		**/
		template<bool CHECKRANGE = true> TGX_INLINE inline void drawPixel(int x, int y, color_t color)
			{
			if (CHECKRANGE)	// optimized away at compile time
				{
				if ((!isValid()) || (x < 0) || (y < 0) || (x >= _lx) || (y >= _ly)) return;
				}
			_buffer[CAST32(x) + CAST32(_stride) * CAST32(y)] = color;
			}


		/**
		* Set a pixel at a given position.
		* use template parameter to disable range check for faster access.
		**/
		template<bool CHECKRANGE = true> TGX_INLINE inline void drawPixel(iVec2 pos, color_t color)
			{
			drawPixel<CHECKRANGE>(pos.x, pos.y, color);
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
			return _buffer[CAST32(x) + CAST32(_stride) * CAST32(y)];
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
			return _buffer[CAST32(x) + CAST32(_stride) * CAST32(y)];
			}


		/**
		* Get a reference to a pixel (no range check!) const version
		**/
		TGX_INLINE inline const color_t& operator()(iVec2 pos) const
			{
			return _buffer[CAST32(pos.x) + CAST32(_stride) * CAST32(pos.y)];
			}


		/** 
		* Get a reference to a pixel (no range check!) non-const version
		**/
		TGX_INLINE inline color_t & operator()(int x, int y)
			{
			return _buffer[CAST32(x) + CAST32(_stride) * CAST32(y)];
			}


		/**
		* Get a reference to a pixel (no range check!) non-const version
		**/
		TGX_INLINE inline color_t& operator()(iVec2 pos)
			{
			return _buffer[CAST32(pos.x) + CAST32(_stride) * CAST32(pos.y)];
			}






	/****************************************************************************
	* 
	* 
	* Drawing primitives
	* 
	*
	****************************************************************************/



		/**
		* Fill the whole image with a single color
		**/
		void fillScreen(color_t color)
			{
			if (!isValid()) return;
			fillRect<false>(imageBox(), color);
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
			color_t* p = _buffer + CAST32(x) + CAST32(y) * CAST32(_stride);
			while (h-- > 0)
				{
				(*p) = color;
				p += _stride;
				}
			}


		/**
		* Draw an vertical line of h pixels starting at pos.
		**/
		template<bool CHECKRANGE = true> TGX_INLINE inline void drawFastVLine(iVec2 pos, int h, color_t color)
			{
			drawFastVLine<CHECKRANGE>(pos.x, pos.y, h, color);
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
			_fast_memset(_buffer + CAST32(x) + CAST32(y) * CAST32(_stride), color, w);
			}


		/**
		* Draw an horizontal line of w pixels starting at pos.
		**/
		template<bool CHECKRANGE = true> TGX_INLINE inline void drawFastHLine(iVec2 pos, int w, color_t color)
			{
			drawFastHLine<CHECKRANGE>(pos.x, pos.y, w, color);
			}


		/**
		* Draw a line between P1 and P2 using bresenham algorithm
		**/
		inline void drawLine(const iVec2 & P1, const iVec2 & P2, color_t color)
			{
			drawLine(P1.x, P1.y, P2.x, P2.y, color);
			}


		/**
		* Draw a line between (x0,y0) and (x1,y1) using bresenham algorithm
		**/
		void drawLine(int x0, int y0, int x1, int y1, color_t color)
			{
			if (!isValid()) return;
			if ((x0 < 0) || (y0 < 0) || (x1 < 0) || (y1 < 0) || (x0 >= _lx) || (y0 >= _ly) || (x1 >= _lx) || (y1 >= _ly))
				{
				_drawLine<true>(x0, y0, x1, y1, color);
				}
			else
				{
				_drawLine<false>(x0, y0, x1, y1, color);
				}
			}



		/*********************************************************************
		*
		* drawing triangles
		*
		**********************************************************************/


		/**
		* Draw a tirangle (but not its interior).
		**/
		inline void drawTriangle(const iVec2& P1, const iVec2& P2, const iVec2& P3, color_t color)
			{
			drawTriangle(P1.x, P1.y, P2.x, P2.y, P3.x, P3.y, color);
			}


		/**
		* Draw a tirangle (but not its interior). 
		**/
		void drawTriangle(int x1,int y1, int x2, int y2, int x3, int y3, color_t color)
			{
			drawLine(x1, y1, x2, y2, color);
			drawLine(x2, y2, x3, y3, color);
			drawLine(x3, y3, x1, y1, color);
			}



		/**
		* Draw a filled triangle with a given color. 		
		**/
		void fillTriangle(const iVec2& P1, const iVec2& P2, const iVec2& P3, color_t color)
			{
			fillTriangle(P1.x, P1.y, P2.x, P2.y, P3.x, P3.y, color);
			}


		/**
		* Draw a filled triangle with a given color.
		* (taken from adafruit gfx / Kurte ILI93412_t3n). 
		**/
		void fillTriangle(int x0,int y0, int x1, int y1, int x2, int y2, color_t color)
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
				{ // Handle awkward all-on-same-line case as its own thing
				a = b = x0;
				if (x1 < a) a = x1; else if (x1 > b) b = x1;
				if (x2 < a) a = x2; else if (x2 > b) b = x2;
				drawFastHLine<true>(a, y0, b - a + 1, color);
				return;
				}
			int dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0,
			    dx12 = x2 - x1, dy12 = y2 - y1, sa = 0, sb = 0;
			if (y1 == y2) last = y1; else last = y1 - 1; 
			for (y = y0; y <= last; y++) 
				{
				a = x0 + sa / dy01;
				b = x0 + sb / dy02;
				sa += dx01;
				sb += dx02;
				if (a > b) swap(a, b);
				drawFastHLine<true>(a, y, b - a + 1, color);
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
				drawFastHLine<true>(a, y, b - a + 1, color);
				}
			}




		/*********************************************************************
		*
		* drawing rectangles
		*
		**********************************************************************/


		/**
		* Draw a rectangle with size (w,h) and upperleft corner at (x,y)
		**/
		template<bool CHECKRANGE = true> inline void drawRect(int x, int y, int w, int h, color_t color)
			{
			drawFastHLine<CHECKRANGE>(x, y, w, color);
			drawFastHLine<CHECKRANGE>(x, y + h - 1, w, color);
			drawFastVLine<CHECKRANGE>(x, y, h, color);
			drawFastVLine<CHECKRANGE>(x + w - 1, y, h, color);
			}


		/**
		* Draw a rectangle corresponding to the box B.
		**/
		template<bool CHECKRANGE = true> inline void drawRect(iBox2 B, color_t color)
			{
			drawRect(B.minX, B.minY, B.lx(), B.ly());
			}


		/**
		* Fill a rectangle region with a single color
		**/
		template<bool CHECKRANGE = true> void fillRect(iBox2 B, color_t color);


		/**
		* Fill a rectangle region with a single color
		**/
		template<bool CHECKRANGE = true> inline void fillRect(int x, int y, int w, int h, color_t color)
			{
			fillRect<CHECKRANGE>({ x, x + w - 1, y, y + h - 1 }, color);
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
		inline void fillRectHGradient(int x, int y, int w, int h, color_t color1, color_t color2)
			{
			fillRectHGradient(iBox2(x, x + w - 1, y, y + h - 1), color1, color2);
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
		inline  void fillRectVGradient(int x, int y, int w, int h, color_t color1, color_t color2) 
			{
			fillRectVGradient(iBox2(x, x + w - 1, y, y + h - 1), color1, color2);
			}




		/*********************************************************************
		*
		* drawing circles
		*
		**********************************************************************/


		/**
		* Draw a circle (but not its interior). 
		**/
		void drawCircle(iVec2 center, int r, color_t color)   TGX_INLINE
			{
			if ((center.x - r >= 0) && (center.x + r < _lx) && (center.y - r >= 0) && (center.y + r < _ly))
				_drawFilledCircle<true, false, false>(center.x, center.y, r, color, color);
			else
				_drawFilledCircle<true, false, true>(center.x, center.y, r, color, color);
			}


		/**
		* Fill the interior of a circle (but not the outline).
		**/
		void fillCircle(iVec2 center, int r, color_t color)   TGX_INLINE
			{
			if ((center.x - r >= 0) && (center.x + r < _lx) && (center.y - r >= 0) && (center.y + r < _ly))
				_drawFilledCircle<false, true, false>(center.x, center.y, r, color, color);
			else
				_drawFilledCircle<false, true, true>(center.x, center.y, r, color, color);
			}


		/**
		* Draw both the outline and interior of a circle (possibly with distinct colors).
		**/
		void drawFilledCircle(iVec2 center, int r, color_t color, color_t outline_color = 0)
			{
			if ((center.x - r >= 0) && (center.x + r < _lx) && (center.y - r >= 0) && (center.y + r < _ly))
				_drawFilledCircle<true, true, false>(center.x, center.y, r, outline_color, color);
			else
				_drawFilledCircle<true, true, true>(center.x, center.y, r, outline_color, color);
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
		iVec2 drawChar(char c, iVec2 pos, color_t col, const GFXfont& font);


		/**
		* Draw a character from an ili9341_t3 font at position (x,y) on the image.
		* Return the position for the next character (on the same line).
		* position is w.r.t. the baseline
		**/
		iVec2 drawChar(char c, iVec2 pos, color_t col, const ILI9341_t3_font_t& font);


		/**
		* Draw a  text  starting at position (x,y) on the image.
		* Return the position after the last character (ie the position for the next char).
		* All position are w.r.t. the baseline
		**/
		iVec2 drawText(const char* text, iVec2 pos, color_t col, const GFXfont& font, bool start_newline_at_0 = false);


		/**
		* Draw a  text  starting at position (x,y) on the image.
		* Return the position after the last character (ie the position for the next char).
		* All position are w.r.t. the baseline
		**/
		iVec2 drawText(const char * text, iVec2 pos, color_t col, const ILI9341_t3_font_t& font, bool start_newline_at_0 = false);






	/****************************************************************************
	* 
	* 
	* 3D Rasterizer
	* 
	*
	****************************************************************************/


	/** options for triangle rasterization */

	#define	SHADER_FLAT (0)
	#define SHADER_GOURAUD (1)
	#define SHADER_TEXTURE (2)


		/**
		* Class used by the rasterizeTriangle() methods.
		*
		* Extension of the fVec4 class that holds the 'varying' parameters (in opengl sense)
		* associated with a vertex and passed to the triangle rasterizer when doing 3D rendering.
		**/
		struct RasterizerVec4 : public tgx::fVec4
			{
			tgx::RGBf color;     // vertex color for gouraud shading (or light intensity for gouraud shading with texturing). 
			tgx::fVec2 T;        // texture coordinates if applicable 
			};



		/**
		* Class used by the rasterizeTriangle() methods.
		*
		* Structure that holds the 'uniform' parameters (in opengl sense) passed
		* to the triangle rasterizer when doing 3D rendering
		**/
		struct RasterizerParams
			{
			float* zbuf;       // pointer to the z buffer (if applicable).
			tgx::RGBf     facecolor;  // pointer to the face color when using flat shading.  
			const Image* tex;        // pointer to the texture (if applicable).
			};



	#define RASTERIZE_SUBPIXEL_BITS (8) // <- change this to adjust sub-pixel precision (between 1 and 8)
	#define RASTERIZE_SUBPIXEL256 (1 << RASTERIZE_SUBPIXEL_BITS)
	#define RASTERIZE_SUBPIXEL128 (1 << (RASTERIZE_SUBPIXEL_BITS -1))
	#define RASTERIZE_MULT256(X) ((X) << (RASTERIZE_SUBPIXEL_BITS))
	#define RASTERIZE_MULT128(X) ((X) << (RASTERIZE_SUBPIXEL_BITS -1))
	#define RASTERIZE_DIV256(X) ((X) >> (RASTERIZE_SUBPIXEL_BITS))



		/**
		* Main method for rasterizing a triangle onto the image for 3D graphics:
		* 
		* FEATURES:
		* 
		* - Pixel perfect rasterization with adjustable subpixel precisions (8 bit default). 
		* 
		* - Tile rasterization: large viewport can be splitted in multiple sub-images. 
		* 
		* - Zbuffer depth testing
		* 
		* - Perspective/Orthographic projection rasterization. 
		* 
		* - Flat shading / gouraud shading. 
		*  
		* - Perspective correct texture mapping 
		* 
		* TEMPLATE PARAMETERS : 
		* 
		* - (LX, LY)     the viewport size (at most 2048x2048 when using 8bit subpixels). 
		*                The image itself may be smaller than the viewport and an offset may be 
		*                specified so it is possible to draw the whole viewport  in 'tile" mode 
		*                be calling this method several time with different offsets.
		*
		* - ZBUFFER : if true, use zbuffer depth test. The buffer must be passed in data.zbuf 
		*             and must be an array of size at least LX*LY floats.
		* 
		* - ORTHO : if true, use orthographic rasterization (which differs from projective
		*           rasterization when doing texture mapping). 
		* 
		* PARAMETERS:
		* 
		* - raster_type  specify the type of shader to use during rasterization. Combination of
		*                SHADER_FLAT, SHADER_GOURAUD, SHADER_TEXTURE
		*
		*                - SHADER_FLAT: Use flat shading (uniform color on the triangle). 
		*                  the triangle color is passed in data.facecolor.
		* 
		*                - SHADER_GOURAUD: Overwrites the SHADER_FLAT flag and enables gouraud
		*                  shading which correspond to linear interpolation of the triangle vertices
		*                  colors. the color of each vertex is passed in V.color
		* 
		*				 - SHADER_TEXTURE: Enable perspective correct texture mapping on the 
		*                  triangle. the texture image is passed in data.tex. THE DIMENSION OF
		*                  THE TEXTURE MUST BE POWERS OF TWO ! the texture coordinate for each 
		*                  vertex are passed in V.T members and the 'illumination' for each vertex 
		*                  is passed in V.color.
		* 
		* - V0,V1,V2     Normalized coordinates of the vertices of the triangle (x,y,z,w)  where, 
		*                'a la opengl' the viewport is mapped to [-1, 1]^2.
		*                The projective w coordinate should contains 1/z when using perspective 
		*                projection (ORTHO=false) and should contain 2 - z when using orthographic 
		*                projection (ORTHO = true). This value is used for depth testing when 
		*                ZBUFFER = true. These vectors also optionally contain  the 'varying' 
		*                parameters associated with each vertex, namely the  texture coords. 
		*                and the color associated with each vertex (when applicable).
		* 
		*			     NOTE: the (x,y) coordinates of the vertices V0,V1,V2 do not need to 
		*                be inside the viewport [-1.0,1.0]^2 and the triangle will still be 
		*                perfectly rasterized provided that they are not 'too far away'. This 
		*                'too far away' depends on the viewport size and the number of subpixel
		*                bits. For example:
		*
		*                - [-2.0,2.0]^2 will work for any viewport at most 1024x1024.
		*				 
		*                - For a viewport of size 320x240 and 8bit subpixels, it suffices for the
		*                  coordinates stay inside [-6.0, 6.0].
		*		
		* - offset_x, offset_y  Offset of this image inside the viewport. So the image corresponds to
		*                       to the box  [offset_x, offset_x + im.width[x[offset_x, offset_x + im.height[
		*                       and only the intersection of this box with the viewport box [0, LX[x[0, LY[ 
		*                       is drawn onto the image. 
		* 
		* - data : contain the 'uniform' parameters depending on the rasterization type.
		* 
		* REMARKS: color are passed in RGBf format irrespectively of the image color type to improve 
		*          quality and simplify handling of different image types. 
		**/
		template<int LX, int LY, bool ZBUFFER, bool ORTHO>
		void rasterizeTriangle(const int raster_type, const RasterizerVec4 & V0, const RasterizerVec4 & V1, const RasterizerVec4 & V2, const int32_t offset_x, const int32_t offset_y, const RasterizerParams& data)
			{
			// assuming that clipping was already perfomed and that V0, V1, V2 are in a reasonable "range" so no overflow will occur. 
			const float mx = (float)(RASTERIZE_MULT128(LX));
			const float my = (float)(RASTERIZE_MULT128(LY));
			const iVec2  P0((int32_t)floorf(V0.x * mx), (int32_t)floorf(V0.y * my));

			const iVec2 sP1((int32_t)floorf(V1.x * mx), (int32_t)floorf(V1.y * my));
			const iVec2 sP2((int32_t)floorf(V2.x * mx), (int32_t)floorf(V2.y * my));

			int32_t xmin = (min(min(P0.x, sP1.x), sP2.x) + RASTERIZE_MULT128(LX)) / RASTERIZE_SUBPIXEL256; // use division and not bitshift  
			int32_t xmax = (max(max(P0.x, sP1.x), sP2.x) + RASTERIZE_MULT128(LX)) / RASTERIZE_SUBPIXEL256; // in case values are negative.
			int32_t ymin = (min(min(P0.y, sP1.y), sP2.y) + RASTERIZE_MULT128(LY)) / RASTERIZE_SUBPIXEL256; //
			int32_t ymax = (max(max(P0.y, sP1.y), sP2.y) + RASTERIZE_MULT128(LY)) / RASTERIZE_SUBPIXEL256; //

			// intersect the sub-image with the triangle bounding box. 			
			int32_t sx = _lx;
			int32_t sy = _ly;
			int32_t ox = offset_x;
			int32_t oy = offset_y;
			if (ox < xmin) { sx -= (xmin - ox); ox = xmin; }
			if (ox + sx > xmax) { sx = xmax - ox + 1; }
			if (sx <= 0) return;
			if (oy < ymin) { sy -= (ymin - oy); oy = ymin; }
			if (oy + sy > ymax) { sy = ymax - oy + 1; }
			if (sy <= 0) return;
		
			const int64_t a = (((int64_t)(sP2.x - P0.x)) * ((int64_t)(sP1.y - P0.y))) - (((int64_t)(sP2.y - P0.y)) * ((int64_t)(sP1.x - P0.x))); // aera

			if (a == 0) return; // do not draw flat triangles

			const RasterizerVec4& fP1 = (a > 0) ? V1 : V2;
			const RasterizerVec4& fP2 = (a > 0) ? V2 : V1;
			const iVec2& P1  = (a > 0) ? sP1 : sP2;
			const iVec2& P2  = (a > 0) ? sP2 : sP1;

			const int32_t us = RASTERIZE_MULT256(ox) - RASTERIZE_MULT128(LX) + RASTERIZE_SUBPIXEL128;   // start pixel position
			const int32_t vs = RASTERIZE_MULT256(oy) - RASTERIZE_MULT128(LY) + RASTERIZE_SUBPIXEL128;   //

			ox -= offset_x;
			oy -= offset_y;

			const int32_t dx1 = P1.y - P0.y;
			const int32_t dy1 = P0.x - P1.x;
			int64_t dO1 = (((int64_t)(us - P0.x)) * ((int64_t)dx1)) + (((int64_t)(vs - P0.y)) * ((int64_t)dy1));
			if ((dx1 < 0) || ((dx1 == 0) && (dy1 < 0))) dO1--; // top left rule (beware, changes total aera).
			int32_t O1 = (dO1 >= 0) ? ((int32_t)RASTERIZE_DIV256(dO1)) : -((int32_t)RASTERIZE_DIV256(-dO1 + (RASTERIZE_SUBPIXEL256 - 1)));

			const int32_t dx2 = P2.y - P1.y;
			const int32_t dy2 = P1.x - P2.x;
			int64_t dO2 = (((int64_t)(us - P1.x)) * ((int64_t)dx2)) + (((int64_t)(vs - P1.y)) * ((int64_t)dy2));
			if ((dx2 < 0) || ((dx2 == 0) && (dy2 < 0))) dO2--; // top left rule (beware, changes total aera).  
			int32_t O2 = (dO2 >= 0) ? ((int32_t)RASTERIZE_DIV256(dO2)) : -((int32_t)RASTERIZE_DIV256(-dO2 + (RASTERIZE_SUBPIXEL256 - 1)));

			const int32_t dx3 = P0.y - P2.y;
			const int32_t dy3 = P2.x - P0.x;
			int64_t dO3 = (((int64_t)(us - P2.x)) * ((int64_t)dx3)) + (((int64_t)(vs - P2.y)) * ((int64_t)dy3));
			if ((dx3 < 0) || ((dx3 == 0) && (dy3 < 0))) dO3--; // top left rule (beware, changes total aera).  
			int32_t O3 = (dO3 >= 0) ? ((int32_t)RASTERIZE_DIV256(dO3)) : -((int32_t)RASTERIZE_DIV256(-dO3 + (RASTERIZE_SUBPIXEL256 - 1)));
					
			if (sx == 1)
				{
				while (((O1 | O2 | O3) < 0) && (sy > 0))
					{
					sy--;
					oy++;
					O1 += dy1;
					O2 += dy2;
					O3 += dy3;
					}
				if (sy == 0) return;
				}
			else if (sy == 1)
				{
				while (((O1 | O2 | O3) < 0) && (sx > 0))
					{
					sx--;
					ox++;
					O1 += dx1;
					O2 += dx2;
					O3 += dx3;
					}
				if (sx == 0) return;
				}
	
			if (dx1 > 0)
				{
				_rasterizeTriangle(DummyTypeBB<ZBUFFER, ORTHO>(), raster_type, ox + (_stride * oy), sx, sy,
					dx1, dy1, O1, fP2,
					dx2, dy2, O2, V0,
					dx3, dy3, O3, fP1,
					data);
				}
			else if (dx2 > 0)
				{
				_rasterizeTriangle(DummyTypeBB<ZBUFFER, ORTHO>(), raster_type, ox + (_stride * oy), sx, sy,
					dx2, dy2, O2, V0,
					dx3, dy3, O3, fP1,
					dx1, dy1, O1, fP2,
					data);
				}
			else
				{
				_rasterizeTriangle(DummyTypeBB<ZBUFFER, ORTHO>(), raster_type, ox + (_stride * oy), sx, sy,
					dx3, dy3, O3, fP1,
					dx1, dy1, O1, fP2,
					dx2, dy2, O2, V0,
					data);
				}
			return;
			}


#undef RASTERIZE_SUBPIXEL_BITS
#undef RASTERIZE_SUBPIXEL256
#undef RASTERIZE_SUBPIXEL128
#undef RASTERIZE_MULT256
#undef RASTERIZE_MULT128
#undef RASTERIZE_DIV256





private:



	/************************************************************************************
	* 
	* Don't you dare look below... this is private :-p
	* 
	*************************************************************************************/


	/** Called when ZBUFFER=false, ORTHO = false*/
	void _rasterizeTriangle(DummyTypeBB<false, false> DT, const int raster_type, const int32_t& offset, const int32_t& lx, const int32_t& ly,
		const int32_t& dx1, const int32_t& dy1, int32_t O1, const RasterizerVec4& fP1,
		const int32_t& dx2, const int32_t& dy2, int32_t O2, const RasterizerVec4& fP2,
		const int32_t& dx3, const int32_t& dy3, int32_t O3, const RasterizerVec4& fP3,
		const RasterizerParams& data)
		{
		if (raster_type & SHADER_TEXTURE)
			{
			if (raster_type & SHADER_GOURAUD)
				_rasterizeTriangleGouraudTexture(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
			else
				_rasterizeTriangleFlatTexture(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
			}
		else
			{
			if (raster_type & SHADER_GOURAUD)
				_rasterizeTriangleGouraud(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
			else
				_rasterizeTriangleFlat(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
			}
		}


	/** Called when ZBUFFER=true, ORTHO = false*/
	void _rasterizeTriangle(DummyTypeBB<true, false> DT, const int raster_type, const int32_t& offset, const int32_t& lx, const int32_t& ly,
		const int32_t& dx1, const int32_t& dy1, int32_t O1, const RasterizerVec4& fP1,
		const int32_t& dx2, const int32_t& dy2, int32_t O2, const RasterizerVec4& fP2,
		const int32_t& dx3, const int32_t& dy3, int32_t O3, const RasterizerVec4& fP3,
		const RasterizerParams& data)
		{
		if (raster_type & SHADER_TEXTURE)
			{
			if (raster_type & SHADER_GOURAUD)
				_rasterizeTriangleGouraudTextureZbuffer(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
			else
				_rasterizeTriangleFlatTextureZbuffer(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
			}
		else
			{
			if (raster_type & SHADER_GOURAUD)
				_rasterizeTriangleGouraudZbuffer(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
			else
				_rasterizeTriangleFlatZbuffer(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
			}
		}

	/** Called when ZBUFFER=false, ORTHO = true*/
	void _rasterizeTriangle(DummyTypeBB<false, true> DT, const int raster_type, const int32_t& offset, const int32_t& lx, const int32_t& ly,
		const int32_t& dx1, const int32_t& dy1, int32_t O1, const RasterizerVec4& fP1,
		const int32_t& dx2, const int32_t& dy2, int32_t O2, const RasterizerVec4& fP2,
		const int32_t& dx3, const int32_t& dy3, int32_t O3, const RasterizerVec4& fP3,
		const RasterizerParams& data)
		{
		if (raster_type & SHADER_TEXTURE)
			{
			if (raster_type & SHADER_GOURAUD)
				_rasterizeTriangleGouraudTextureOrtho(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
			else
				_rasterizeTriangleFlatTextureOrtho(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
			}
		else
			{
			if (raster_type & SHADER_GOURAUD)
				_rasterizeTriangleGouraud(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data); // same as perspective projection
			else
				_rasterizeTriangleFlat(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data); // same as perspective projection
			}
		}


	/** Called when ZBUFFER=true, ORTHO = true*/
	void _rasterizeTriangle(DummyTypeBB<true, true> DT, const int raster_type, const int32_t& offset, const int32_t& lx, const int32_t& ly,
		const int32_t& dx1, const int32_t& dy1, int32_t O1, const RasterizerVec4& fP1,
		const int32_t& dx2, const int32_t& dy2, int32_t O2, const RasterizerVec4& fP2,
		const int32_t& dx3, const int32_t& dy3, int32_t O3, const RasterizerVec4& fP3,
		const RasterizerParams& data)
		{
		if (raster_type & SHADER_TEXTURE)
			{
			if (raster_type & SHADER_GOURAUD)
				_rasterizeTriangleGouraudTextureZbufferOrtho(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
			else
				_rasterizeTriangleFlatTextureZbufferOrtho(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);
			}
		else
			{
			if (raster_type & SHADER_GOURAUD)
				_rasterizeTriangleGouraudZbuffer(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);  // same as perspective projection
			else
				_rasterizeTriangleFlatZbuffer(offset, lx, ly, dx1, dy1, O1, fP1, dx2, dy2, O2, fP2, dx3, dy3, O3, fP3, data);  // same as perspective projection
			}
		}
	




		/**
		* FLAT SHADING (NO ZBUFFER)
		**/		
		void _rasterizeTriangleFlat(const int32_t & offset, const int32_t & lx, const int32_t & ly,
			const int32_t & dx1, const int32_t & dy1, int32_t O1, const RasterizerVec4 & fP1,
			const int32_t & dx2, const int32_t & dy2, int32_t O2, const RasterizerVec4 & fP2,
			const int32_t & dx3, const int32_t & dy3, int32_t O3, const RasterizerVec4 & fP3,			
			const RasterizerParams& data)
			{
			color_t col = (color_t)data.facecolor;
			color_t* buf = _buffer + offset;
			const int32_t stride = _stride;

			const uintptr_t end = (uintptr_t)(buf + (ly * stride));

			while ((uintptr_t)(buf) < end)
				{ // iterate over scanlines
				int32_t bx = 0; // start offset
				if (O1 < 0)
					{
					// we know that dx1 > 0					
					bx = (-O1 + dx1 - 1) / dx1; // first index where it becomes positive
					}
				if (O2 < 0)
					{
					if (dx2 <= 0)
						{
						if (dy2 <= 0) return;
						const int32_t by = (-O2 + dy2 - 1) / dy2;
						O1 += (by * dy1);
						O2 += (by * dy2);
						O3 += (by * dy3);
						const int32_t offs = by * stride;
						buf += offs;
						continue;
						}
					bx = max(bx, ((-O2 + dx2 - 1) / dx2));
					}
				if (O3 < 0)
					{
					if (dx3 <= 0)
						{
						if (dy3 <= 0) return;
						const int32_t by = (-O3 + dy3 - 1) / dy3;
						O1 += (by * dy1);
						O2 += (by * dy2);
						O3 += (by * dy3);
						const int32_t offs = by * stride;
						buf += offs;
						continue;
						}
					bx = max(bx, ((-O3 + dx3 - 1) / dx3));
					}
				
				int32_t C2 = O2 + (dx2 * bx);
				int32_t C3 = O3 + (dx3 * bx);
				while ((bx < lx) && ((C2 | C3) >= 0))
					{
					buf[bx] = col;
					C2 += dx2;
					C3 += dx3;
					bx++;
					}

				O1 += dy1;
				O2 += dy2;
				O3 += dy3;
				buf += stride;
				}
			}


		/**
		* GOURAUD SHADING (NO Z BUFFER)
		**/
		void _rasterizeTriangleGouraud(const int32_t & offset, const int32_t & lx, const int32_t & ly,
			const int32_t & dx1, const int32_t & dy1, int32_t O1, const RasterizerVec4 & fP1,
			const int32_t & dx2, const int32_t & dy2, int32_t O2, const RasterizerVec4 & fP2,
			const int32_t & dx3, const int32_t & dy3, int32_t O3, const RasterizerVec4 & fP3,			
			const RasterizerParams& data)
			{
			color_t* buf = _buffer + offset;
			const int32_t stride = _stride;

			const color_t col1 = (color_t)fP1.color;
			const color_t col2 = (color_t)fP2.color;
			const color_t col3 = (color_t)fP3.color;
			
			const uintptr_t end = (uintptr_t)(buf + (ly * stride));
			const int32_t aera = O1 + O2 + O3;

			while ((uintptr_t)(buf) < end)
				{ // iterate over scanlines
				int32_t bx = 0; // start offset
				if (O1 < 0)
					{
					// we know that dx1 > 0					
					bx = (-O1 + dx1 - 1) / dx1; // first index where it becomes positive
					}
				if (O2 < 0)
					{
					if (dx2 <= 0)
						{
						if (dy2 <= 0) return;
						const int32_t by = (-O2 + dy2 - 1) / dy2;
						O1 += (by * dy1);
						O2 += (by * dy2);
						O3 += (by * dy3);
						const int32_t offs = by * stride;
						buf += offs;
						continue;
						}
					bx = max(bx, ((-O2 + dx2 - 1) / dx2));
					}
				if (O3 < 0)
					{
					if (dx3 <= 0)
						{
						if (dy3 <= 0) return;
						const int32_t by = (-O3 + dy3 - 1) / dy3;
						O1 += (by * dy1);
						O2 += (by * dy2);
						O3 += (by * dy3);
						const int32_t offs = by * stride;
						buf += offs;
						continue;
						}
					bx = max(bx, ((-O3 + dx3 - 1) / dx3));
					}

				int32_t C2 = O2 + (dx2 * bx);
				int32_t C3 = O3 + (dx3 * bx);
				while ((bx < lx) && ((C2 | C3) >= 0))
					{
					buf[bx] = blend(col2, C2, col3, C3, col1, aera);
					C2 += dx2;
					C3 += dx3;
					bx++;
					}

				O1 += dy1;
				O2 += dy2;
				O3 += dy3;
				buf += stride;
				}
			}


		/**
		* TEXTURE + FLAT SHADING (NO ZBUFFER)
		**/	
		void _rasterizeTriangleFlatTexture(const int32_t & offset, const int32_t & lx, const int32_t & ly,
			const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4 & fP1,
			const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4 & fP2,
			const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4 & fP3,			
			const RasterizerParams& data)
			{					
			const color_t * tex = data.tex->data(); 
			const int32_t texsize_x = data.tex->width();
			const int32_t texsize_y = data.tex->height();

			color_t* buf = _buffer + offset;
			
			const int32_t stride = _stride;

			const uintptr_t end = (uintptr_t)(buf + (ly * stride));
			const int32_t aera = O1 + O2 + O3;

			const float invaera = 1.0f / aera;
			const float fP1a = fP1.w * invaera;
			const float fP2a = fP2.w * invaera;
			const float fP3a = fP3.w * invaera;
			
			const float dw = (dx1 * fP1a) + (dx2 * fP2a) + (dx3 * fP3a);

			const RGBf& cf = (RGBf)data.facecolor;
			const int fPR = (int)(256 * cf.R);
			const int fPG = (int)(256 * cf.G);
			const int fPB = (int)(256 * cf.B);

			// the texture coord
			fVec2 T1 = fP1.T;			
			fVec2 T2 = fP2.T;
			fVec2 T3 = fP3.T;

			// divide the texture coord by z * aera
			T1 *= fP1a;
			T2 *= fP2a;
			T3 *= fP3a;
			T1.x *= texsize_x;
			T2.x *= texsize_x;
			T3.x *= texsize_x;
			T1.y *= texsize_y;
			T2.y *= texsize_y;
			T3.y *= texsize_y;

			const float dtx = ((T1.x * dx1) + (T2.x * dx2) + (T3.x * dx3));
			const float dty = ((T1.y * dx1) + (T2.y * dx2) + (T3.y * dx3));

			while ((uintptr_t)(buf) < end)
				{ // iterate over scanlines
				int32_t bx = 0; // start offset
				if (O1 < 0)
					{
					// we know that dx1 > 0					
					bx = (-O1 + dx1 - 1) / dx1; // first index where it becomes positive
					}
				if (O2 < 0)
					{
					if (dx2 <= 0)
						{
						if (dy2 <= 0) return;
						const int32_t by = (-O2 + dy2 - 1) / dy2;
						O1 += (by * dy1);
						O2 += (by * dy2);
						O3 += (by * dy3);
						const int32_t offs = by * stride;
						buf += offs;
						continue;
						}
					bx = max(bx, ((-O2 + dx2 - 1) / dx2));
					}
				if (O3 < 0)
					{
					if (dx3 <= 0)
						{
						if (dy3 <= 0) return;
						const int32_t by = (-O3 + dy3 - 1) / dy3;
						O1 += (by * dy1);
						O2 += (by * dy2);
						O3 += (by * dy3);
						const int32_t offs = by * stride;
						buf += offs;
						continue;
						}
					bx = max(bx, ((-O3 + dx3 - 1) / dx3));
					}

				int32_t C1 = O1 + (dx1 * bx);
				int32_t C2 = O2 + (dx2 * bx);
				int32_t C3 = O3 + (dx3 * bx);
				float cw = ((C1 * fP1a) + (C2 * fP2a) + (C3 * fP3a));

				float tx = ((T1.x * C1) + (T2.x * C2) + (T3.x * C3));
				float ty = ((T1.y * C1) + (T2.y * C2) + (T3.y * C3));
				
				while ((bx < lx) && ((C2 | C3) >= 0))
					{
					const float icw = 1.0f / cw;
					const int ttx = ((int)((tx * icw))) & (texsize_x - 1); 
					const int tty = ((int)((ty * icw))) & (texsize_y - 1);

					color_t col = tex[ttx + (tty)*texsize_x];
					col.mult256(fPR, fPG, fPB);
					buf[bx] = col;

					C2 += dx2;
					C3 += dx3;
					cw += dw;

					tx += dtx;
					ty += dty;

					bx++;
					}

				O1 += dy1;
				O2 += dy2;
				O3 += dy3;
				buf += stride;
				}
			}


		/**
		* TEXTURE + GOURAUD SHADING (NO ZBUFFER)
		**/
		void _rasterizeTriangleGouraudTexture(const int32_t & offset, const int32_t & lx, const int32_t & ly,
			const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4 & fP1,
			const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4 & fP2,
			const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4 & fP3,			
			const RasterizerParams& data)
			{					
			const color_t * tex = data.tex->data(); 
			const int32_t texsize_x = data.tex->width();
			const int32_t texsize_y = data.tex->height();

			color_t* buf = _buffer + offset;

			const int32_t stride = _stride;

			const uintptr_t end = (uintptr_t)(buf + (ly * stride));
			const int32_t aera = O1 + O2 + O3;

			const float invaera = 1.0f / aera;
			const float fP1a = fP1.w * invaera;
			const float fP2a = fP2.w * invaera;
			const float fP3a = fP3.w * invaera;
			
			const float dw = (dx1 * fP1a) + (dx2 * fP2a) + (dx3 * fP3a);
				
			const RGBf & cf1 = (RGBf)fP1.color;
			const RGBf & cf2 = (RGBf)fP2.color;
			const RGBf & cf3 = (RGBf)fP3.color;
			const int fP1R = (int)(256 * cf1.R);
			const int fP1G = (int)(256 * cf1.G);
			const int fP1B = (int)(256 * cf1.B);
			const int fP21R = (int)(256 * (cf2.R - cf1.R));
			const int fP21G = (int)(256 * (cf2.G - cf1.G));
			const int fP21B = (int)(256 * (cf2.B - cf1.B));
			const int fP31R = (int)(256 * (cf3.R - cf1.R));
			const int fP31G = (int)(256 * (cf3.G - cf1.G));
			const int fP31B = (int)(256 * (cf3.B - cf1.B));

			// the texture coord
			fVec2 T1 = fP1.T;			
			fVec2 T2 = fP2.T;
			fVec2 T3 = fP3.T;

			// divide the texture coord by z * aera
			T1 *= fP1a;
			T2 *= fP2a;
			T3 *= fP3a;
			T1.x *= texsize_x;
			T2.x *= texsize_x;
			T3.x *= texsize_x;
			T1.y *= texsize_y;
			T2.y *= texsize_y;
			T3.y *= texsize_y;


			const float dtx = ((T1.x * dx1) + (T2.x * dx2) + (T3.x * dx3));
			const float dty = ((T1.y * dx1) + (T2.y * dx2) + (T3.y * dx3));

			while ((uintptr_t)(buf) < end)
				{ // iterate over scanlines
				int32_t bx = 0; // start offset
				if (O1 < 0)
					{
					// we know that dx1 > 0					
					bx = (-O1 + dx1 - 1) / dx1; // first index where it becomes positive
					}
				if (O2 < 0)
					{
					if (dx2 <= 0)
						{
						if (dy2 <= 0) return;
						const int32_t by = (-O2 + dy2 - 1) / dy2;
						O1 += (by * dy1);
						O2 += (by * dy2);
						O3 += (by * dy3);
						const int32_t offs = by * stride;
						buf += offs;
						continue;
						}
					bx = max(bx, ((-O2 + dx2 - 1) / dx2));
					}
				if (O3 < 0)
					{
					if (dx3 <= 0)
						{
						if (dy3 <= 0) return;
						const int32_t by = (-O3 + dy3 - 1) / dy3;
						O1 += (by * dy1);
						O2 += (by * dy2);
						O3 += (by * dy3);
						const int32_t offs = by * stride;
						buf += offs;
						continue;
						}
					bx = max(bx, ((-O3 + dx3 - 1) / dx3));
					}

				int32_t C1 = O1 + (dx1 * bx);
				int32_t C2 = O2 + (dx2 * bx);
				int32_t C3 = O3 + (dx3 * bx);
				float cw = ((C1 * fP1a) + (C2 * fP2a) + (C3 * fP3a));

				float tx = ((T1.x * C1) + (T2.x * C2) + (T3.x * C3));
				float ty = ((T1.y * C1) + (T2.y * C2) + (T3.y * C3));
				
				while ((bx < lx) && ((C2 | C3) >= 0))
					{
					const float icw = 1.0f / cw;
					const int ttx = ((int)((tx * icw))) & (texsize_x - 1);
					const int tty = ((int)((ty * icw))) & (texsize_y - 1);

					const int r = fP1R + ((C2 * fP21R + C3 * fP31R) / aera);
					const int g = fP1G + ((C2 * fP21G + C3 * fP31G) / aera);
					const int b = fP1B + ((C2 * fP21B + C3 * fP31B) / aera);	
					color_t col = tex[ttx + (tty)*texsize_x];
					col.mult256(r, g, b);
					buf[bx] = col;

					C2 += dx2;
					C3 += dx3;
					cw += dw;

					tx += dtx;
					ty += dty;

					bx++;
					}

				O1 += dy1;
				O2 += dy2;
				O3 += dy3;
				buf += stride;
				}
			}




		/**
		* ZBUFFER + FLAT SHADING
		**/
		void _rasterizeTriangleFlatZbuffer(const int32_t offset, const int32_t & lx, const int32_t & ly,
			const int32_t & dx1, const int32_t & dy1, int32_t O1, const RasterizerVec4 & fP1,
			const int32_t & dx2, const int32_t & dy2, int32_t O2, const RasterizerVec4 & fP2,
			const int32_t & dx3, const int32_t & dy3, int32_t O3, const RasterizerVec4 & fP3,			
			const RasterizerParams& data)
			{			
			const color_t col = (color_t)data.facecolor;
			color_t* buf = _buffer + offset;
			float* zbuf = data.zbuf + offset;

			const int32_t stride = _stride;

			const uintptr_t end = (uintptr_t)(buf + (ly * stride));
			const int32_t aera = O1 + O2 + O3;

			const float invaera = 1.0f / aera;
			const float fP1a = fP1.w * invaera;
			const float fP2a = fP2.w * invaera;
			const float fP3a = fP3.w * invaera;
			const float dw = (dx1 * fP1a) + (dx2 * fP2a) + (dx3 * fP3a);

			while ((uintptr_t)(buf) < end)
				{ // iterate over scanlines
				int32_t bx = 0; // start offset
				if (O1 < 0)
					{
					// we know that dx1 > 0					
					bx = (-O1 + dx1 - 1) / dx1; // first index where it becomes positive
					}
				if (O2 < 0)
					{
					if (dx2 <= 0)
						{
						if (dy2 <= 0) return;
						const int32_t by = (-O2 + dy2 - 1) / dy2;
						O1 += (by * dy1);
						O2 += (by * dy2);
						O3 += (by * dy3);
						const int32_t offs = by * stride;
						buf += offs;
						zbuf += offs;
						continue;
						}
					bx = max(bx, ((-O2 + dx2 - 1) / dx2));
					}
				if (O3 < 0)
					{
					if (dx3 <= 0)
						{
						if (dy3 <= 0) return;
						const int32_t by = (-O3 + dy3 - 1) / dy3;
						O1 += (by * dy1);
						O2 += (by * dy2);
						O3 += (by * dy3);
						const int32_t offs = by * stride;
						buf += offs;
						zbuf += offs;
						continue;
						}
					bx = max(bx, ((-O3 + dx3 - 1) / dx3));
					}

				const int32_t C1 = O1 + (dx1 * bx);
				int32_t C2 = O2 + (dx2 * bx);
				int32_t C3 = O3 + (dx3 * bx);
				float cw = ((C1 * fP1a) + (C2 * fP2a) + (C3 * fP3a));

				while ((bx < lx) && ((C2 | C3) >= 0))
					{
					float& W = zbuf[bx];
					if (W < cw)
						{
						W = cw;
						buf[bx] = col;
						}
					C2 += dx2;
					C3 += dx3;
					cw += dw;
					bx++;
					}

				O1 += dy1;
				O2 += dy2;
				O3 += dy3;
				buf += stride;
				zbuf += stride;
				}
			}


		/**
		* ZBUFFER + GOURAUD SHADING
		**/	
		void _rasterizeTriangleGouraudZbuffer(const int32_t & offset, const int32_t & lx, const int32_t & ly,
			const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4 & fP1,
			const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4 & fP2,
			const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4 & fP3,			
			const RasterizerParams& data)
			{
			color_t* buf = _buffer + offset;
			float* zbuf = data.zbuf + offset;

			const int32_t stride = _stride;

			const color_t col1 = (color_t)fP1.color;
			const color_t col2 = (color_t)fP2.color;
			const color_t col3 = (color_t)fP3.color;
			
			const uintptr_t end = (uintptr_t)(buf + (ly * stride));
			const int32_t aera = O1 + O2 + O3;

			const float invaera = 1.0f / aera;
			const float fP1a = fP1.w * invaera;
			const float fP2a = fP2.w * invaera;
			const float fP3a = fP3.w * invaera;
			const float dw = (dx1 * fP1a) + (dx2 * fP2a) + (dx3 * fP3a);

			while ((uintptr_t)(buf) < end)
				{ // iterate over scanlines
				int32_t bx = 0; // start offset
				if (O1 < 0)
					{
					// we know that dx1 > 0					
					bx = (-O1 + dx1 - 1) / dx1; // first index where it becomes positive
					}
				if (O2 < 0)
					{
					if (dx2 <= 0)
						{
						if (dy2 <= 0) return;
						const int32_t by = (-O2 + dy2 - 1) / dy2;
						O1 += (by * dy1);
						O2 += (by * dy2);
						O3 += (by * dy3);
						const int32_t offs = by * stride;
						buf += offs;
						zbuf += offs;
						continue;
						}
					bx = max(bx, ((-O2 + dx2 - 1) / dx2));
					}
				if (O3 < 0)
					{
					if (dx3 <= 0)
						{
						if (dy3 <= 0) return;
						const int32_t by = (-O3 + dy3 - 1) / dy3;
						O1 += (by * dy1);
						O2 += (by * dy2);
						O3 += (by * dy3);
						const int32_t offs = by * stride;
						buf += offs;
						zbuf += offs;
						continue;
						}
					bx = max(bx, ((-O3 + dx3 - 1) / dx3));
					}

				const int32_t C1 = O1 + (dx1 * bx);
				int32_t C2 = O2 + (dx2 * bx);
				int32_t C3 = O3 + (dx3 * bx);
				float cw = ((C1 * fP1a) + (C2 * fP2a) + (C3 * fP3a));

				while ((bx < lx) && ((C2 | C3) >= 0))
					{
					float& W = zbuf[bx];
					if (W < cw)
						{
						W = cw;
						buf[bx] = blend(col2, C2, col3, C3, col1, aera);
						}
					C2 += dx2;
					C3 += dx3;
					cw += dw;
					bx++;
					}

				O1 += dy1;
				O2 += dy2;
				O3 += dy3;
				buf += stride;
				zbuf += stride;
				}
			}


		/**
		* ZBUFFER + TEXTURE + FLAT SHADING
		**/
		void _rasterizeTriangleFlatTextureZbuffer(const int32_t & offset, const int32_t & lx, const int32_t & ly,
			const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4 & fP1,
			const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4 & fP2,
			const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4 & fP3,			
			const RasterizerParams& data)
			{					
			const color_t * tex = data.tex->data(); 
			const int32_t texsize_x = data.tex->width();
			const int32_t texsize_y = data.tex->height();

			color_t* buf = _buffer + offset;
			float* zbuf = data.zbuf + offset;

			const int32_t stride = _stride;

			const uintptr_t end = (uintptr_t)(buf + (ly * stride));
			const int32_t aera = O1 + O2 + O3;

			const float invaera = 1.0f / aera;
			const float fP1a = fP1.w * invaera;
			const float fP2a = fP2.w * invaera;
			const float fP3a = fP3.w * invaera;
			
			const float dw = (dx1 * fP1a) + (dx2 * fP2a) + (dx3 * fP3a);

			const RGBf& cf = (RGBf)data.facecolor;
			const int fPR = (int)(256 * cf.R);
			const int fPG = (int)(256 * cf.G);
			const int fPB = (int)(256 * cf.B);


			// the texture coord
			fVec2 T1 = fP1.T;			
			fVec2 T2 = fP2.T;
			fVec2 T3 = fP3.T;

			// divide the texture coord by z * aera
			T1 *= fP1a;
			T2 *= fP2a;
			T3 *= fP3a;
			T1.x *= texsize_x;
			T2.x *= texsize_x;
			T3.x *= texsize_x;
			T1.y *= texsize_y;
			T2.y *= texsize_y;
			T3.y *= texsize_y;

			const float dtx = ((T1.x * dx1) + (T2.x * dx2) + (T3.x * dx3));
			const float dty = ((T1.y * dx1) + (T2.y * dx2) + (T3.y * dx3));

			while ((uintptr_t)(buf) < end)
				{ // iterate over scanlines
				int32_t bx = 0; // start offset
				if (O1 < 0)
					{
					// we know that dx1 > 0					
					bx = (-O1 + dx1 - 1) / dx1; // first index where it becomes positive
					}
				if (O2 < 0)
					{
					if (dx2 <= 0)
						{
						if (dy2 <= 0) return;
						const int32_t by = (-O2 + dy2 - 1) / dy2;
						O1 += (by * dy1);
						O2 += (by * dy2);
						O3 += (by * dy3);
						const int32_t offs = by * stride;
						buf += offs;
						zbuf += offs;
						continue;
						}
					bx = max(bx, ((-O2 + dx2 - 1) / dx2));
					}
				if (O3 < 0)
					{
					if (dx3 <= 0)
						{
						if (dy3 <= 0) return;
						const int32_t by = (-O3 + dy3 - 1) / dy3;
						O1 += (by * dy1);
						O2 += (by * dy2);
						O3 += (by * dy3);
						const int32_t offs = by * stride;
						buf += offs;
						zbuf += offs;
						continue;
						}
					bx = max(bx, ((-O3 + dx3 - 1) / dx3));
					}

				int32_t C1 = O1 + (dx1 * bx);
				int32_t C2 = O2 + (dx2 * bx);
				int32_t C3 = O3 + (dx3 * bx);
				float cw = ((C1 * fP1a) + (C2 * fP2a) + (C3 * fP3a));

				float tx = ((T1.x * C1) + (T2.x * C2) + (T3.x * C3));
				float ty = ((T1.y * C1) + (T2.y * C2) + (T3.y * C3));
				
				while ((bx < lx) && ((C2 | C3) >= 0))
					{
					float& W = zbuf[bx];
					if (W < cw)
						{
						W = cw;	

						const float icw = 1.0f / cw;
						const int ttx = ((int)((tx * icw))) & (texsize_x - 1); 
						const int tty = ((int)((ty * icw))) & (texsize_y - 1);

						color_t col = tex[ttx + (tty)*texsize_x];
						col.mult256(fPR, fPG, fPB);
						buf[bx] = col;
						}

					C2 += dx2;
					C3 += dx3;
					cw += dw;

					tx += dtx;
					ty += dty;

					bx++;
					}

				O1 += dy1;
				O2 += dy2;
				O3 += dy3;
				buf += stride;
				zbuf += stride;
				}
			}


		/**
		* ZBUFFER + TEXTURE + GOURAUD SHADING
		**/
		void _rasterizeTriangleGouraudTextureZbuffer(const int32_t & offset, const int32_t & lx, const int32_t & ly,
			const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4 & fP1,
			const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4 & fP2,
			const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4 & fP3,			
			const RasterizerParams& data)
			{					
			const color_t * tex = data.tex->data(); 
			const int32_t texsize_x = data.tex->width();
			const int32_t texsize_y = data.tex->height();

			color_t* buf = _buffer + offset;
			float* zbuf = data.zbuf + offset;

			const int32_t stride = _stride;

			const uintptr_t end = (uintptr_t)(buf + (ly * stride));
			const int32_t aera = O1 + O2 + O3;

			const float invaera = 1.0f / aera;
			const float fP1a = fP1.w * invaera;
			const float fP2a = fP2.w * invaera;
			const float fP3a = fP3.w * invaera;
			
			const float dw = (dx1 * fP1a) + (dx2 * fP2a) + (dx3 * fP3a);
					
			const RGBf & cf1 = (RGBf)fP1.color;
			const RGBf & cf2 = (RGBf)fP2.color;
			const RGBf & cf3 = (RGBf)fP3.color;
			const int fP1R = (int)(256 * cf1.R);
			const int fP1G = (int)(256 * cf1.G);
			const int fP1B = (int)(256 * cf1.B);
			const int fP21R = (int)(256 * (cf2.R - cf1.R));
			const int fP21G = (int)(256 * (cf2.G - cf1.G));
			const int fP21B = (int)(256 * (cf2.B - cf1.B));
			const int fP31R = (int)(256 * (cf3.R - cf1.R));
			const int fP31G = (int)(256 * (cf3.G - cf1.G));
			const int fP31B = (int)(256 * (cf3.B - cf1.B));

			// the texture coord
			fVec2 T1 = fP1.T;			
			fVec2 T2 = fP2.T;
			fVec2 T3 = fP3.T;

			// divide the texture coord by z * aera
			T1 *= fP1a;
			T2 *= fP2a;
			T3 *= fP3a;
			T1.x *= texsize_x;
			T2.x *= texsize_x;
			T3.x *= texsize_x;
			T1.y *= texsize_y;
			T2.y *= texsize_y;
			T3.y *= texsize_y;

			const float dtx = ((T1.x * dx1) + (T2.x * dx2) + (T3.x * dx3));
			const float dty = ((T1.y * dx1) + (T2.y * dx2) + (T3.y * dx3));

			while ((uintptr_t)(buf) < end)
				{ // iterate over scanlines
				int32_t bx = 0; // start offset
				if (O1 < 0)
					{
					// we know that dx1 > 0					
					bx = (-O1 + dx1 - 1) / dx1; // first index where it becomes positive
					}
				if (O2 < 0)
					{
					if (dx2 <= 0)
						{
						if (dy2 <= 0) return;
						const int32_t by = (-O2 + dy2 - 1) / dy2;
						O1 += (by * dy1);
						O2 += (by * dy2);
						O3 += (by * dy3);
						const int32_t offs = by * stride;
						buf += offs;
						zbuf += offs;
						continue;
						}
					bx = max(bx, ((-O2 + dx2 - 1) / dx2));
					}
				if (O3 < 0)
					{
					if (dx3 <= 0)
						{
						if (dy3 <= 0) return;
						const int32_t by = (-O3 + dy3 - 1) / dy3;
						O1 += (by * dy1);
						O2 += (by * dy2);
						O3 += (by * dy3);
						const int32_t offs = by * stride;
						buf += offs;
						zbuf += offs;
						continue;
						}
					bx = max(bx, ((-O3 + dx3 - 1) / dx3));
					}

				int32_t C1 = O1 + (dx1 * bx);
				int32_t C2 = O2 + (dx2 * bx);
				int32_t C3 = O3 + (dx3 * bx);
				float cw = ((C1 * fP1a) + (C2 * fP2a) + (C3 * fP3a));

				float tx = ((T1.x * C1) + (T2.x * C2) + (T3.x * C3));
				float ty = ((T1.y * C1) + (T2.y * C2) + (T3.y * C3));
				
				while ((bx < lx) && ((C2 | C3) >= 0))
					{
					float& W = zbuf[bx];
					if (W < cw)
						{
						W = cw;
										
						const float icw = 1.0f / cw;
						const int ttx = ((int)((tx * icw))) & (texsize_x - 1);
						const int tty = ((int)((ty * icw))) & (texsize_y - 1);

						const int r = fP1R + ((C2 * fP21R + C3 * fP31R) / aera);
						const int g = fP1G + ((C2 * fP21G + C3 * fP31G) / aera);
						const int b = fP1B + ((C2 * fP21B + C3 * fP31B) / aera);					
						color_t col = tex[ttx + (tty)*texsize_x];
						col.mult256(r, g, b);
						buf[bx] = col;

						}

					C2 += dx2;
					C3 += dx3;
					cw += dw;

					tx += dtx;
					ty += dty;

					bx++;
					}

				O1 += dy1;
				O2 += dy2;
				O3 += dy3;
				buf += stride;
				zbuf += stride;
				}
			}




		/**
		* TEXTURE + FLAT SHADING (NO ZBUFFER) + ORTHOGRAPHIC
		**/	
		void _rasterizeTriangleFlatTextureOrtho(const int32_t & offset, const int32_t & lx, const int32_t & ly,
			const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4 & fP1,
			const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4 & fP2,
			const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4 & fP3,			
			const RasterizerParams& data)
			{					
			const color_t * tex = data.tex->data(); 
			const int32_t texsize_x = data.tex->width();
			const int32_t texsize_y = data.tex->height();

			color_t* buf = _buffer + offset;
			
			const int32_t stride = _stride;

			const uintptr_t end = (uintptr_t)(buf + (ly * stride));
			const int32_t aera = O1 + O2 + O3;

			const float invaera = 1.0f / aera;
		
			const RGBf& cf = (RGBf)data.facecolor;
			const int fPR = (int)(256 * cf.R);
			const int fPG = (int)(256 * cf.G);
			const int fPB = (int)(256 * cf.B);

			// the texture coord
			fVec2 T1 = fP1.T;			
			fVec2 T2 = fP2.T;
			fVec2 T3 = fP3.T;

			// divide the texture coord by aera
			T1 *= invaera;
			T2 *= invaera;
			T3 *= invaera;			
			T1.x *= texsize_x;
			T2.x *= texsize_x;
			T3.x *= texsize_x;
			T1.y *= texsize_y;
			T2.y *= texsize_y;
			T3.y *= texsize_y;
			
			const float dtx = ((T1.x * dx1) + (T2.x * dx2) + (T3.x * dx3));
			const float dty = ((T1.y * dx1) + (T2.y * dx2) + (T3.y * dx3));

			while ((uintptr_t)(buf) < end)
				{ // iterate over scanlines
				int32_t bx = 0; // start offset
				if (O1 < 0)
					{
					// we know that dx1 > 0					
					bx = (-O1 + dx1 - 1) / dx1; // first index where it becomes positive
					}
				if (O2 < 0)
					{
					if (dx2 <= 0)
						{
						if (dy2 <= 0) return;
						const int32_t by = (-O2 + dy2 - 1) / dy2;
						O1 += (by * dy1);
						O2 += (by * dy2);
						O3 += (by * dy3);
						const int32_t offs = by * stride;
						buf += offs;
						continue;
						}
					bx = max(bx, ((-O2 + dx2 - 1) / dx2));
					}
				if (O3 < 0)
					{
					if (dx3 <= 0)
						{
						if (dy3 <= 0) return;
						const int32_t by = (-O3 + dy3 - 1) / dy3;
						O1 += (by * dy1);
						O2 += (by * dy2);
						O3 += (by * dy3);
						const int32_t offs = by * stride;
						buf += offs;
						continue;
						}
					bx = max(bx, ((-O3 + dx3 - 1) / dx3));
					}

				int32_t C1 = O1 + (dx1 * bx);
				int32_t C2 = O2 + (dx2 * bx);
				int32_t C3 = O3 + (dx3 * bx);
				
				float tx = ((T1.x * C1) + (T2.x * C2) + (T3.x * C3));
				float ty = ((T1.y * C1) + (T2.y * C2) + (T3.y * C3));
				
				while ((bx < lx) && ((C2 | C3) >= 0))
					{
					const int ttx = ((int)tx) & (texsize_x - 1); 
					const int tty = ((int)ty) & (texsize_y - 1);

					color_t col = tex[ttx + (tty)*texsize_x];
					col.mult256(fPR, fPG, fPB);
					buf[bx] = col;

					C2 += dx2;
					C3 += dx3;					

					tx += dtx;
					ty += dty;

					bx++;
					}

				O1 += dy1;
				O2 += dy2;
				O3 += dy3;
				buf += stride;
				}
			}


		/**
		* TEXTURE + GOURAUD SHADING (NO ZBUFFER) + ORTHOGRAPHIC
		**/
		void _rasterizeTriangleGouraudTextureOrtho(const int32_t & offset, const int32_t & lx, const int32_t & ly,
			const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4 & fP1,
			const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4 & fP2,
			const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4 & fP3,			
			const RasterizerParams& data)
			{					
			const color_t * tex = data.tex->data(); 
			const int32_t texsize_x = data.tex->width();
			const int32_t texsize_y = data.tex->height();

			color_t* buf = _buffer + offset;

			const int32_t stride = _stride;

			const uintptr_t end = (uintptr_t)(buf + (ly * stride));
			const int32_t aera = O1 + O2 + O3;

			const float invaera = 1.0f / aera;
		
			const RGBf & cf1 = (RGBf)fP1.color;
			const RGBf & cf2 = (RGBf)fP2.color;
			const RGBf & cf3 = (RGBf)fP3.color;
			const int fP1R = (int)(256 * cf1.R);
			const int fP1G = (int)(256 * cf1.G);
			const int fP1B = (int)(256 * cf1.B);
			const int fP21R = (int)(256 * (cf2.R - cf1.R));
			const int fP21G = (int)(256 * (cf2.G - cf1.G));
			const int fP21B = (int)(256 * (cf2.B - cf1.B));
			const int fP31R = (int)(256 * (cf3.R - cf1.R));
			const int fP31G = (int)(256 * (cf3.G - cf1.G));
			const int fP31B = (int)(256 * (cf3.B - cf1.B));

			// the texture coord
			fVec2 T1 = fP1.T;			
			fVec2 T2 = fP2.T;
			fVec2 T3 = fP3.T;

			// divide the texture coord by aera
			T1 *= invaera;
			T2 *= invaera;
			T3 *= invaera;
			T1.x *= texsize_x;
			T2.x *= texsize_x;
			T3.x *= texsize_x;
			T1.y *= texsize_y;
			T2.y *= texsize_y;
			T3.y *= texsize_y;

			const float dtx = ((T1.x * dx1) + (T2.x * dx2) + (T3.x * dx3));
			const float dty = ((T1.y * dx1) + (T2.y * dx2) + (T3.y * dx3));

			while ((uintptr_t)(buf) < end)
				{ // iterate over scanlines
				int32_t bx = 0; // start offset
				if (O1 < 0)
					{
					// we know that dx1 > 0					
					bx = (-O1 + dx1 - 1) / dx1; // first index where it becomes positive
					}
				if (O2 < 0)
					{
					if (dx2 <= 0)
						{
						if (dy2 <= 0) return;
						const int32_t by = (-O2 + dy2 - 1) / dy2;
						O1 += (by * dy1);
						O2 += (by * dy2);
						O3 += (by * dy3);
						const int32_t offs = by * stride;
						buf += offs;
						continue;
						}
					bx = max(bx, ((-O2 + dx2 - 1) / dx2));
					}
				if (O3 < 0)
					{
					if (dx3 <= 0)
						{
						if (dy3 <= 0) return;
						const int32_t by = (-O3 + dy3 - 1) / dy3;
						O1 += (by * dy1);
						O2 += (by * dy2);
						O3 += (by * dy3);
						const int32_t offs = by * stride;
						buf += offs;
						continue;
						}
					bx = max(bx, ((-O3 + dx3 - 1) / dx3));
					}

				int32_t C1 = O1 + (dx1 * bx);
				int32_t C2 = O2 + (dx2 * bx);
				int32_t C3 = O3 + (dx3 * bx);

				float tx = ((T1.x * C1) + (T2.x * C2) + (T3.x * C3));
				float ty = ((T1.y * C1) + (T2.y * C2) + (T3.y * C3));
				
				while ((bx < lx) && ((C2 | C3) >= 0))
					{
					const int ttx = ((int)(tx)) & (texsize_x - 1); 
					const int tty = ((int)(ty)) & (texsize_y - 1);

					const int r = fP1R + ((C2 * fP21R + C3 * fP31R) / aera);
					const int g = fP1G + ((C2 * fP21G + C3 * fP31G) / aera);
					const int b = fP1B + ((C2 * fP21B + C3 * fP31B) / aera);	
					color_t col = tex[ttx + (tty)*texsize_x];
					col.mult256(r, g, b);
					buf[bx] = col;

					C2 += dx2;
					C3 += dx3;

					tx += dtx;
					ty += dty;

					bx++;
					}

				O1 += dy1;
				O2 += dy2;
				O3 += dy3;
				buf += stride;
				}
			}



		/**
		* ZBUFFER + TEXTURE + FLAT SHADING + ORTHOGRAPHIC
		**/
		void _rasterizeTriangleFlatTextureZbufferOrtho(const int32_t & offset, const int32_t & lx, const int32_t & ly,
			const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4 & fP1,
			const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4 & fP2,
			const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4 & fP3,			
			const RasterizerParams& data)
			{					
			const color_t * tex = data.tex->data(); 
			const int32_t texsize_x = data.tex->width();
			const int32_t texsize_y = data.tex->height();

			color_t* buf = _buffer + offset;
			float* zbuf = data.zbuf + offset;

			const int32_t stride = _stride;

			const uintptr_t end = (uintptr_t)(buf + (ly * stride));
			const int32_t aera = O1 + O2 + O3;

			const float invaera = 1.0f / aera;
			const float fP1a = fP1.w * invaera;
			const float fP2a = fP2.w * invaera;
			const float fP3a = fP3.w * invaera;
			
			const float dw = (dx1 * fP1a) + (dx2 * fP2a) + (dx3 * fP3a);

			const RGBf& cf = (RGBf)data.facecolor;
			const int fPR = (int)(256 * cf.R);
			const int fPG = (int)(256 * cf.G);
			const int fPB = (int)(256 * cf.B);


			// the texture coord
			fVec2 T1 = fP1.T;			
			fVec2 T2 = fP2.T;
			fVec2 T3 = fP3.T;

			// divide the texture coord by aera
			T1 *= invaera;
			T2 *= invaera;
			T3 *= invaera;
			T1.x *= texsize_x; 
			T2.x *= texsize_x;
			T3.x *= texsize_x;
			T1.y *= texsize_y;
			T2.y *= texsize_y;
			T3.y *= texsize_y;

			const float dtx = ((T1.x * dx1) + (T2.x * dx2) + (T3.x * dx3));
			const float dty = ((T1.y * dx1) + (T2.y * dx2) + (T3.y * dx3));

			while ((uintptr_t)(buf) < end)
				{ // iterate over scanlines
				int32_t bx = 0; // start offset
				if (O1 < 0)
					{
					// we know that dx1 > 0					
					bx = (-O1 + dx1 - 1) / dx1; // first index where it becomes positive
					}
				if (O2 < 0)
					{
					if (dx2 <= 0)
						{
						if (dy2 <= 0) return;
						const int32_t by = (-O2 + dy2 - 1) / dy2;
						O1 += (by * dy1);
						O2 += (by * dy2);
						O3 += (by * dy3);
						const int32_t offs = by * stride;
						buf += offs;
						zbuf += offs;
						continue;
						}
					bx = max(bx, ((-O2 + dx2 - 1) / dx2));
					}
				if (O3 < 0)
					{
					if (dx3 <= 0)
						{
						if (dy3 <= 0) return;
						const int32_t by = (-O3 + dy3 - 1) / dy3;
						O1 += (by * dy1);
						O2 += (by * dy2);
						O3 += (by * dy3);
						const int32_t offs = by * stride;
						buf += offs;
						zbuf += offs;
						continue;
						}
					bx = max(bx, ((-O3 + dx3 - 1) / dx3));
					}

				int32_t C1 = O1 + (dx1 * bx);
				int32_t C2 = O2 + (dx2 * bx);
				int32_t C3 = O3 + (dx3 * bx);
				float cw = ((C1 * fP1a) + (C2 * fP2a) + (C3 * fP3a));

				float tx = ((T1.x * C1) + (T2.x * C2) + (T3.x * C3));
				float ty = ((T1.y * C1) + (T2.y * C2) + (T3.y * C3));
				
				while ((bx < lx) && ((C2 | C3) >= 0))
					{
					float& W = zbuf[bx];
					if (W < cw)
						{
						W = cw;	
						const int ttx = ((int)(tx)) & (texsize_x - 1); 
						const int tty = ((int)(ty)) & (texsize_y - 1);
						color_t col = tex[ttx + (tty)*texsize_x];
						col.mult256(fPR, fPG, fPB);
						buf[bx] = col;
						}

					C2 += dx2;
					C3 += dx3;
					cw += dw;

					tx += dtx;
					ty += dty;

					bx++;
					}

				O1 += dy1;
				O2 += dy2;
				O3 += dy3;
				buf += stride;
				zbuf += stride;
				}
			}


		/**
		* ZBUFFER + TEXTURE + GOURAUD SHADING + ORTHOGRAPHIC
		**/
		void _rasterizeTriangleGouraudTextureZbufferOrtho(const int32_t & offset, const int32_t & lx, const int32_t & ly,
			const int32_t dx1, const int32_t dy1, int32_t O1, const RasterizerVec4 & fP1,
			const int32_t dx2, const int32_t dy2, int32_t O2, const RasterizerVec4 & fP2,
			const int32_t dx3, const int32_t dy3, int32_t O3, const RasterizerVec4 & fP3,			
			const RasterizerParams& data)
			{					
			const color_t * tex = data.tex->data(); 
			const int32_t texsize_x = data.tex->width();
			const int32_t texsize_y = data.tex->height();

			color_t* buf = _buffer + offset;
			float* zbuf = data.zbuf + offset;

			const int32_t stride = _stride;

			const uintptr_t end = (uintptr_t)(buf + (ly * stride));
			const int32_t aera = O1 + O2 + O3;

			const float invaera = 1.0f / aera;
			const float fP1a = fP1.w * invaera;
			const float fP2a = fP2.w * invaera;
			const float fP3a = fP3.w * invaera;
			
			const float dw = (dx1 * fP1a) + (dx2 * fP2a) + (dx3 * fP3a);
					
			const RGBf & cf1 = (RGBf)fP1.color;
			const RGBf & cf2 = (RGBf)fP2.color;
			const RGBf & cf3 = (RGBf)fP3.color;
			const int fP1R = (int)(256 * cf1.R);
			const int fP1G = (int)(256 * cf1.G);
			const int fP1B = (int)(256 * cf1.B);
			const int fP21R = (int)(256 * (cf2.R - cf1.R));
			const int fP21G = (int)(256 * (cf2.G - cf1.G));
			const int fP21B = (int)(256 * (cf2.B - cf1.B));
			const int fP31R = (int)(256 * (cf3.R - cf1.R));
			const int fP31G = (int)(256 * (cf3.G - cf1.G));
			const int fP31B = (int)(256 * (cf3.B - cf1.B));

			// the texture coord
			fVec2 T1 = fP1.T;			
			fVec2 T2 = fP2.T;
			fVec2 T3 = fP3.T;

			// divide the texture coord by aera
			T1 *= invaera;
			T2 *= invaera;
			T3 *= invaera;
			T1.x *= texsize_x;
			T2.x *= texsize_x;
			T3.x *= texsize_x;
			T1.y *= texsize_y;
			T2.y *= texsize_y;
			T3.y *= texsize_y;

			const float dtx = ((T1.x * dx1) + (T2.x * dx2) + (T3.x * dx3));
			const float dty = ((T1.y * dx1) + (T2.y * dx2) + (T3.y * dx3));

			while ((uintptr_t)(buf) < end)
				{ // iterate over scanlines
				int32_t bx = 0; // start offset
				if (O1 < 0)
					{
					// we know that dx1 > 0					
					bx = (-O1 + dx1 - 1) / dx1; // first index where it becomes positive
					}
				if (O2 < 0)
					{
					if (dx2 <= 0)
						{
						if (dy2 <= 0) return;
						const int32_t by = (-O2 + dy2 - 1) / dy2;
						O1 += (by * dy1);
						O2 += (by * dy2);
						O3 += (by * dy3);
						const int32_t offs = by * stride;
						buf += offs;
						zbuf += offs;
						continue;
						}
					bx = max(bx, ((-O2 + dx2 - 1) / dx2));
					}
				if (O3 < 0)
					{
					if (dx3 <= 0)
						{
						if (dy3 <= 0) return;
						const int32_t by = (-O3 + dy3 - 1) / dy3;
						O1 += (by * dy1);
						O2 += (by * dy2);
						O3 += (by * dy3);
						const int32_t offs = by * stride;
						buf += offs;
						zbuf += offs;
						continue;
						}
					bx = max(bx, ((-O3 + dx3 - 1) / dx3));
					}

				int32_t C1 = O1 + (dx1 * bx);
				int32_t C2 = O2 + (dx2 * bx);
				int32_t C3 = O3 + (dx3 * bx);
				float cw = ((C1 * fP1a) + (C2 * fP2a) + (C3 * fP3a));

				float tx = ((T1.x * C1) + (T2.x * C2) + (T3.x * C3));
				float ty = ((T1.y * C1) + (T2.y * C2) + (T3.y * C3));
				
				while ((bx < lx) && ((C2 | C3) >= 0))
					{
					float& W = zbuf[bx];
					if (W < cw)
						{
						W = cw;
										
						const int ttx = ((int)((tx))) & (texsize_x - 1); 
						const int tty = ((int)((ty))) & (texsize_y - 1);

						const int r = fP1R + ((C2 * fP21R + C3 * fP31R) / aera);
						const int g = fP1G + ((C2 * fP21G + C3 * fP31G) / aera);
						const int b = fP1B + ((C2 * fP21B + C3 * fP31B) / aera);					
						color_t col = tex[ttx + (tty)*texsize_x];
						col.mult256(r, g, b);
						buf[bx] = col;

						}

					C2 += dx2;
					C3 += dx3;
					cw += dw;

					tx += dtx;
					ty += dty;

					bx++;
					}

				O1 += dy1;
				O2 += dy2;
				O3 += dy3;
				buf += stride;
				zbuf += stride;
				}
			}










		/** Make sure the image parameters are ok, else mark the image as invalid. */
		constexpr inline void _checkvalid()
			{
			if ((_lx <= 0) || (_ly <= 0) || (_stride < _lx) || (_buffer == nullptr)) setInvalid();
			}


		/** set len consecutive pixels given color starting at pdest */
		static inline void _fast_memset(color_t* p_dest, color_t color, int32_t len);


		/** blit a region while taking care of possible overlap */
		static void _blitRegion(color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy)
			{
			if ((size_t)pdest <= (size_t)psrc) 
				_blitRegionUp(pdest, dest_stride, psrc, src_stride, sx, sy);
			else 
				_blitRegionDown(pdest, dest_stride, psrc, src_stride, sx, sy);
			}


		static void _blitRegionUp(color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy);

		static void _blitRegionDown(color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy);



		/** fetch a single bit from a bit array. (from ili9341_t3.cpp) */
		static inline uint32_t _fetchbit(const uint8_t* p, uint32_t index) { return (p[index >> 3] & (0x80 >> (index & 7))); }


		/** fetch 'required' bits from a bit array, returned as an unsigned integer  (from ili9341_t3.cpp)*/
		static uint32_t _fetchbits_unsigned(const uint8_t* p, uint32_t index, uint32_t required);


		/** fetch 'required' bits from a bit array, returned as a signed integer (from ili9341_t3.cpp) */
		static uint32_t _fetchbits_signed(const uint8_t* p, uint32_t index, uint32_t required);


		/** used for clipping a font bitmap */
		bool _clipit(int& x, int& y, int& sx, int& sy, int& b_left, int& b_up);


		/**
		 * draw a character from an ILI9341_t3 font bitmap (version 1, with line compression).
		 * The ILI9341_t3 font format is described here: https://forum.pjrc.com/threads/54316-ILI9341_t-font-structure-format
		 **/
		void _drawCharILI9341_t3(const uint8_t* bitmap, int32_t off, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col);


		/** used by _drawCharILI9341_t3 to draw a single row of a font bitmap */
		static void _drawcharline(const uint8_t* bitmap, int32_t off, color_t* p, int dx, color_t col);


		/** 
		 * draw a 1 bit per pixel char bitmap on the image 
		 * Use to draw char from Adafruit font format: https://glenviewsoftware.com/projects/products/adafonteditor/adafruit-tgx-font-format/
		 **/
		void _drawCharBitmap_1BPP(const uint8_t* bitmap, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col);


		/** draw a 2 bit per pixel char bitmap on the image 
		*  packed bdf format 23 : https://github.com/projectitis/packedbdf/blob/master/packedbdf.md
		**/
		void _drawCharBitmap_2BPP(const uint8_t* bitmap, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col);


		/** draw a 4 bit per pixel char bitmap on the image */
		void _drawCharBitmap_4BPP(const uint8_t* bitmap, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col);


		/** draw a 8 bit per pixel char bitmap on the image */
		void _drawCharBitmap_8BPP(const uint8_t* bitmap, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col);





		template<bool CHECKRANGE> inline void _drawLine(int x0, int y0, int x1, int y1, color_t color)
			{
			if (y0 == y1) 
				{
				if (x1 > x0) 
					{
					drawFastHLine<CHECKRANGE>(x0, y0, x1 - x0 + 1, color);
					}
				else if (x1 < x0) 
					{
					drawFastHLine<CHECKRANGE>(x1, y0, x0 - x1 + 1, color);
					}
				else 
					{
					drawPixel<CHECKRANGE>(x0, y0, color);
					}
				return;
				}
			else if (x0 == x1) 
				{
				if (y1 > y0) 
					{
					drawFastVLine<CHECKRANGE>(x0, y0, y1 - y0 + 1, color);
					}
				else 
					{
					drawFastVLine<CHECKRANGE>(x0, y1, y0 - y1 + 1, color);
					}
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

			int dx, dy;
			dx = x1 - x0;
			dy = abs(y1 - y0);

			int err = dx / 2;
			int ystep;

			if (y0 < y1) 
				{
				ystep = 1;
				}
			else 
				{
				ystep = -1;
				}
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
							{
							drawFastVLine<CHECKRANGE>(y0, xbegin, len + 1, color);
							}
						else 
							{
							drawPixel<CHECKRANGE>(y0, x0, color);
							}
						xbegin = x0 + 1;
						y0 += ystep;
						err += dx;
						}
					}
				if (x0 > xbegin + 1) 
					{
					drawFastVLine<CHECKRANGE>(y0, xbegin, x0 - xbegin, color);
					}
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
							{
							drawFastHLine<CHECKRANGE>(xbegin, y0, len + 1, color);
							}
						else 
							{
							drawPixel<CHECKRANGE>(x0, y0, color);
							}
						xbegin = x0 + 1;
						y0 += ystep;
						err += dx;
						}
					}
				if (x0 > xbegin + 1) 
					{
					drawFastHLine<CHECKRANGE>(xbegin, y0, x0 - xbegin, color);
					}
				}
			}



		template<bool OUTLINE, bool FILL, bool CHECKRANGE> void _drawFilledCircle(int xm, int ym, int r, color_t color, color_t fillcolor)
			{	
			if ((r <= 0) || (!isValid())) return;
			if ((CHECKRANGE)&&(r > 2))
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
						drawFastHLine<CHECKRANGE>(xm, ym + y,  -x, fillcolor);
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
				} 
			while (x < 0);
			}



	};







	/**********************************************************************************
	* 
	* Implementation details
	*
	***********************************************************************************/






	/************************************************************************************
	*  Blitting / copying / resizing images
	*************************************************************************************/


	template<typename color_t>
	void Image<color_t>::blit(const Image& sprite, int dest_x, int dest_y, int sprite_x, int sprite_y, int sx, int sy)
		{
		if ((!sprite.isValid()) || (!isValid())) { return; } // nothing to draw on or from
		if (sprite_x < 0) { dest_x -= sprite_x; sx += sprite_x; sprite_x = 0; }
		if (sprite_y < 0) { dest_y -= sprite_y; sy += sprite_y; sprite_y = 0; }
		if (dest_x < 0) { sprite_x -= dest_x;   sx += dest_x; dest_x = 0; }
		if (dest_y < 0) { sprite_y -= dest_y;   sy += dest_y; dest_y = 0; }
		if ((dest_x >= _lx) || (dest_y >= _ly) || (sprite_x >= sprite._lx) || (sprite_x >= sprite._ly)) return;
		sx -= max(0, (dest_x + sx - _lx));
		sy -= max(0, (dest_y + sy - _ly));
		sx -= max(0, (sprite_x + sx - sprite._lx));
		sy -= max(0, (sprite_y + sy - sprite._ly));
		if ((sx <= 0) || (sy <= 0)) return;
		_blitRegion(_buffer + CAST32(dest_y) * CAST32(_stride) + CAST32(dest_x), _stride, sprite._buffer + CAST32(sprite_y) * CAST32(sprite._stride) + CAST32(sprite_x), sprite._stride, sx, sy);
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
			const int32_t off_src = ((j * ay) / by) * CAST32(src._stride);
			const int32_t off_dest = j * CAST32(_stride);
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
			const color_t * p_src = src_image._buffer + j * CAST32(2*src_image._stride);
			color_t * p_dest = _buffer + j * CAST32(_stride);
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
	*  Drawing primitives
	*************************************************************************************/


	/** Fill a rectangle region with a single color */
	template<typename color_t> 
	template<bool CHECKRANGE> void Image<color_t>::fillRect(iBox2 B, color_t color)
		{
		if (CHECKRANGE)
			{
			if (!isValid()) return;
			B &= imageBox();
			if (B.isEmpty()) return;
			}
		const int sx = B.lx();
		int sy = B.ly();
		color_t * p = _buffer + CAST32(B.minX) + CAST32(B.minY) * CAST32(_stride);
		if (sx == _stride) 
			{ // fast, set everything at once
			const int32_t len = CAST32(sy) * CAST32(_stride);
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
		color_t * p = _buffer + CAST32(B.minX) + CAST32(_stride) * CAST32(B.minY);
		for (int h = B.ly(); h > 0; h--)
			{
			RGB64 c(c64_a);
			for (int i = 0; i < w; i++)
				{
				p[i] = color_t(c);	// convert back
				c.R += dr;
				c.G += dg;
				c.B += db;
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
		color_t * p = _buffer + CAST32(B.minX) + CAST32(_stride) * CAST32(B.minY);
		for (int j = h; j > 0; j--)
			{
			_fast_memset(p, color_t(c64_a), B.lx());
			c64_a.R += dr;
			c64_a.G += dg;
			c64_a.B += db;
			p += _stride;
			}
		}






	/************************************************************************************
	*  Drawing Text
	*************************************************************************************/



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
	iVec2 Image<color_t>::drawChar(char c, iVec2 pos, color_t col, const GFXfont& font)
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
		_drawCharBitmap_1BPP(font.bitmap + g.bitmapOffset, rsx, b_up, b_left, sx, sy, x, y, col);
		return iVec2(pos.x + g.xAdvance, pos.y);
		}


	template<typename color_t>
	iBox2 Image<color_t>::measureText(const char* text, iVec2 pos, const GFXfont& font, bool start_newline_at_0)
		{
		const int startx = start_newline_at_0 ? 0 : pos.x;
		iBox2 B;
		B.empty();
		const size_t l = strlen(text);
		for (int i = 0; i < l; i++)
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
	iVec2 Image<color_t>::drawText(const char* text, iVec2 pos, color_t col, const GFXfont& font, bool start_newline_at_0)
		{
		const int startx = start_newline_at_0 ? 0 : pos.x;
		const int l = strlen(text);
		for (int i = 0; i < l; i++)
			{
			const char c = text[i];
			if (c == '\n')
				{
				pos.x = startx;
				pos.y += font.yAdvance;
				}
			else
				{
				pos = drawChar(c, pos, col, font);
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
	iVec2 Image<color_t>::drawChar(char c, iVec2 pos, color_t col, const ILI9341_t3_font_t & font)
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
			_drawCharILI9341_t3(data, off, rsx, b_up, b_left, sx, sy, x, y, col);
			}
		else if (font.version == 23)
			{ // antialised font
			data += (off >> 3) + ((off & 7) ? 1 : 0); // bitmap begins at the next byte boundary
			switch (font.reserved)
				{
				case 0: 
					_drawCharBitmap_1BPP(data, rsx, b_up, b_left, sx, sy, x, y, col);
					break;
				case 1:
					_drawCharBitmap_2BPP(data, rsx, b_up, b_left, sx, sy, x, y, col);
					break;
				case 2:
					_drawCharBitmap_4BPP(data, rsx, b_up, b_left, sx, sy, x, y, col);
					break;
				case 3:
					_drawCharBitmap_8BPP(data, rsx, b_up, b_left, sx, sy, x, y, col);
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
	iVec2 Image<color_t>::drawText(const char* text, iVec2 pos, color_t col, const ILI9341_t3_font_t& font, bool start_newline_at_0)
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
				pos = drawChar(c, pos, col, font);
				}
			}
		return pos;
		}



	/************************************************************************************
	*  Implementation of private routines
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
			while (len-- > 0) { *(p_dest++) = color; }
			}
		}


	template<typename color_t>
	void Image<color_t>::_blitRegionUp(color_t * pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy)
		{
		// TODO, make faster with specialization (writing 32bit at once etc...) 
		for (int j = 0; j < sy; j++)
			{
			color_t* pdest2 = pdest + CAST32(j) * CAST32(dest_stride);
			color_t* psrc2 = psrc + CAST32(j) * CAST32(src_stride);
			for (int i = 0; i < sx; i++) { pdest2[i] = psrc2[i]; }
			}
		}


	template<typename color_t>
	void Image<color_t>::_blitRegionDown(color_t* pdest, int dest_stride, color_t* psrc, int src_stride, int sx, int sy)
		{
		// TODO, make faster with specialization (writing 32bit at once etc...)
		for (int j = sy - 1; j >= 0; j--)
			{
			color_t* pdest2 = pdest + CAST32(j) * CAST32(dest_stride);
			color_t* psrc2 = psrc + CAST32(j) * CAST32(src_stride);
			for (int i = sx - 1; i >= 0; i--) { pdest2[i] = psrc2[i]; }
			}	
		}


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
	void Image<color_t>::_drawCharILI9341_t3(const uint8_t* bitmap, int32_t off, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col)
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
			_drawcharline(bitmap, off + b_left, _buffer + CAST32(x) + CAST32(y) * CAST32(_stride), sx, col);
			if ((--rl) == 0)
				{ // done repeating so we move to the next line in the bitmap
				off += rsx;
				}
			y++; // next line in the destination image
			}
		}


	template<typename color_t>
	void Image<color_t>::_drawcharline(const uint8_t* bitmap, int32_t off, color_t* p, int dx, color_t col)
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
					if (b & u) { *p = col; }
					p++; dx--; u >>= 1;
					}
				u = 128;
				}
			while (dx >= 8)
				{ // now we can write 8 pixels consecutively. 
				const uint8_t b = *(bitmap++);
				if (b)
					{ // there is something to write
					if (b & 128) p[0] = col;
					if (b & 64) p[1] = col;
					if (b & 32) p[2] = col;
					if (b & 16) p[3] = col;
					if (b & 8)  p[4] = col;
					if (b & 4)  p[5] = col;
					if (b & 2)  p[6] = col;
					if (b & 1)  p[7] = col;
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
						if (b & u) { *p = col; }
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
					if (b & u) { *p = col; }
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
						if (b & u) { *p = col; }
						p++; dx--;  u >>= 1;
					} while (dx > 0);
					}
				}
			}
		}


	template<typename color_t>
	void Image<color_t>::_drawCharBitmap_1BPP(const uint8_t* bitmap, int rsx, int b_up, int b_left, int sx, int sy,int x, int y, color_t col)
		{
		int32_t off = CAST32(b_up) * CAST32(rsx) + CAST32(b_left);
		bitmap += (off >> 3);						// starting byte in the bitmap
		uint8_t u = (uint8_t)(128 >> (off & 7));	// index of the first bit 
		const int sk = (rsx - sx);				// number of bits to skip at the end of a row.
		color_t* p = _buffer + CAST32(x) + CAST32(_stride) * CAST32(y); // start position in destination buffer
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
						if (b & u) { *p = col; }
						p++; dx--; u >>= 1;
						}
					u = 128;
					}
				while (dx >= 8)
					{ // now we can write 8 pixels consecutively. 
					const uint8_t b = *(bitmap++);
					if (b)
						{
						if (b & 128) p[0] = col;
						if (b & 64) p[1] = col;
						if (b & 32) p[2] = col;
						if (b & 16) p[3] = col;
						if (b & 8)  p[4] = col;
						if (b & 4)  p[5] = col;
						if (b & 2)  p[6] = col;
						if (b & 1)  p[7] = col;
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
						if (b & u) { *p = col; }
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
						if (b & u) { *p = col; }
						p++; dx--; u >>= 1;
						}
					u = 128;
					}
				if (dx > 0)
					{
					const uint8_t b = *bitmap; // we know we will complete the row with this byte
					do
						{
						if (b & u) { *p = col; }
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
	void Image<color_t>::_drawCharBitmap_2BPP(const uint8_t* bitmap, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col)
		{ 
		if (sx >= 4)
			{ // each row has at least 4 pixels
			for (int dy = 0; dy < sy; dy++)
				{
				int32_t off = CAST32(b_up + dy) * CAST32(rsx) + CAST32(b_left);
				color_t* p = _buffer + CAST32(_stride) * CAST32(y + dy) + CAST32(x); 
				int dx = sx;
				const int32_t uu = off & 3;
				if (uu)
					{// not at the start of a bitmap byte: we first finish it. 
					const uint8_t b = bitmap[off >> 2];
					switch (uu)
						{
						case 1: { const int v = ((b & 48) >> 4); (p++)->blend256(col, (v * 171) >> 1); off++; dx--; }
						case 2: { const int v = ((b & 12) >> 2); (p++)->blend256(col, (v * 171) >> 1); off++; dx--; }
						case 3: { const int v = (b & 3); (p++)->blend256(col, (v * 171) >> 1); off++; dx--; }
						}
					}
				while (dx >= 4)
					{ // now we can write 4 pixels consecutively. 
					const uint8_t b = bitmap[off >> 2];
					if (b)
						{
							{ const int v = ((b & 192) >> 6); p[0].blend256(col, (v * 171) >> 1); }
							{ const int v = ((b & 48) >> 4); p[1].blend256(col, (v * 171) >> 1); }
							{ const int v = ((b & 12) >> 2); p[2].blend256(col, (v * 171) >> 1); }
							{ const int v = (b & 3); p[3].blend256(col, (v * 171) >> 1); }
						}
					off += 4;
					p += 4;
					dx -= 4;
					}
				// strictly less than 4 pixels remain on the row 
				if (dx > 1)
					{
					const uint8_t b = bitmap[off >> 2];
					{const int v = ((b & 192) >> 6); (p++)->blend256(col, (v * 171) >> 1); }
					{const int v = ((b & 48) >> 4); (p++)->blend256(col, (v * 171) >> 1); }
					if (dx > 2) { const int v = ((b & 12) >> 2); (p++)->blend256(col, (v * 171) >> 1); }
					}
				else
					{
					if (dx > 0) 
						{ 
						const uint8_t b = bitmap[off >> 2];
						const int v = ((b & 192) >> 6); (p++)->blend256(col, (v * 171) >> 1); 
						}
					}					
				}
			}
		else
			{ // each row has less than 4 pixels
			for (int dy = 0; dy < sy; dy++)
				{
				int32_t off = CAST32(b_up + dy) * CAST32(rsx) + CAST32(b_left); // offset for the start of this line
				color_t* p = _buffer + CAST32(_stride) * CAST32(y + dy) + CAST32(x); // start position in destination buffer
				int dx = sx;
				const int32_t uu = off & 3;
				if ((4 - uu) < sx)
					{ // we cannot complete the row with this byte
					const uint8_t b = bitmap[off >> 2];
					switch (uu)
						{
						case 1: { const int v = ((b & 48) >> 4); (p++)->blend256(col, (v * 171) >> 1); off++; dx--; } // this case should never occur
						case 2: { const int v = ((b & 12) >> 2); (p++)->blend256(col, (v * 171) >> 1); off++; dx--; }
						case 3: { const int v = (b & 3); (p++)->blend256(col, (v * 171) >> 1); off++; dx--; }
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
							case 0: { const int v = ((b & 192) >> 6); (p++)->blend256(col, (v * 171) >> 1); break; }
							case 1: { const int v = ((b & 48) >> 4); (p++)->blend256(col, (v * 171) >> 1); break; }
							case 2: { const int v = ((b & 12) >> 2); (p++)->blend256(col, (v * 171) >> 1); break; }
							case 3: { const int v = (b & 3); (p++)->blend256(col, (v * 171) >> 1); break; }
							}
						}
					}
				}
			}
		}


	template<typename color_t>
	void Image<color_t>::_drawCharBitmap_4BPP(const uint8_t* bitmap, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col)
		{ 
		if (sx >= 2)
			{ // each row has at least 2 pixels
			for (int dy = 0; dy < sy; dy++)
				{
				int32_t off = CAST32(b_up + dy) * CAST32(rsx) + CAST32(b_left);
				color_t* p = _buffer + CAST32(_stride) * CAST32(y + dy) + CAST32(x); 
				int dx = sx;
				if (off & 1)
					{// not at the start of a bitmap byte: we first finish it. 
					const uint8_t b = bitmap[off >> 1];
				    const int v = (b & 15); (p++)->blend256(col, (v * 137) >> 3); 
					off++; dx--; 
					}
				while (dx >= 2)
					{
					const uint8_t b = bitmap[off >> 1];
					if (b)
						{
							{ const int v = ((b & 240) >> 4); p[0].blend256(col, (v * 137) >> 3); }
							{ const int v = (b & 15); p[1].blend256(col, (v * 137) >> 3); }
						}
					off += 2;
					p += 2;
					dx -= 2;
					}
				if (dx > 0)
					{
					const uint8_t b = bitmap[off >> 1];					
					const int v = ((b & 240) >> 4); p->blend256(col, (v * 137) >> 3);
					}
				}
			}
		else
			{ // each row has a single pixel 
			color_t* p = _buffer + CAST32(_stride) * CAST32(y) + CAST32(x);
			int32_t off = CAST32(b_up) * CAST32(rsx) + CAST32(b_left);
			while(sy-- > 0)
				{
				const uint8_t b = bitmap[off >> 1];
				const int v = (off & 1) ? (b & 15) : ((b & 240) >> 4);
				p->blend256(col, (v * 137) >> 3);
				p += _stride;
				off += rsx;
				}
			}
		}


	template<typename color_t>
	void Image<color_t>::_drawCharBitmap_8BPP(const uint8_t* bitmap, int rsx, int b_up, int b_left, int sx, int sy, int x, int y, color_t col)
		{
		const uint8_t * p_src = bitmap + CAST32(b_up) * CAST32(rsx) + CAST32(b_left);
		color_t * p_dst = _buffer + CAST32(x) + CAST32(_stride) * CAST32(y);
		const int sk_src = rsx - sx;
		const int sk_dst = _stride - sx;
		while (sy-- > 0)
			{
			int dx = sx;
			while (dx-- > 0)
				{
				uint32_t cc = *(p_src++);
				(*(p_dst++)).blend256(col, (cc*129) >> 7); 
				}
			p_src += sk_src;
			p_dst += sk_dst;
			}
		}





}

/** end of file */

