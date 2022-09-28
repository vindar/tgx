/** @file bseg.h */
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

#ifndef _TGX_BSEG_H_
#define _TGX_BSEG_H_

// only C++, no plain C
#ifdef __cplusplus


#include "Misc.h"
#include "Vec2.h"
#include "Box2.h"


namespace tgx
{



	
	struct BSegState;


	/** Bresenham segment */
	struct BSeg
		{

		/** Construct segment from P1 to P2 (integer-valued positions) */
		BSeg(const iVec2& P1, const iVec2& P2) { init(P1, P2); }


		/** Construct segment from P1 to P2 (real-valued positions) **/
		BSeg(const fVec2& Pf1, const fVec2& Pf2) { init(Pf1, Pf2); }


		/** Reverse the segment **/
		inline void reverse()
			{
			const int32_t len = _len;
			if (len > 0)
				{
				move(len);		// move to the other endpoint
				_len = len;		// reset the lenght
				}
			// reverse
			_stepx = -_stepx; 
			_stepy = -_stepy;
			if (_x_major) { _frac = -_dx - 1 - _frac; _frac += 2 * _dy; }
			else { _frac = -_dy - 1 - _frac; _frac += 2 * _dx; }
			}


		/** Return the reversed segment **/
		inline BSeg get_reverse() const
			{
			BSeg tmp(*this);
			tmp.reverse();
			return tmp;
			}


		/**
		* check whether this segment makes an obtuse angle with seg
		* when this line is oriented accoridng to side. 
		*
		* this segment and seg must have the same start point. 
		* 
		* side should be -1 or 1
		* returns  -1, 1 or 0. 
		**/
		int angle(int side, BSeg seg)
			{
			const iVec2 P1(_dx * _stepx, _dy * _stepy);
			const iVec2 P2(seg._dx * seg._stepx, seg._dy * seg._stepy);
			const auto a = ((P1.x * P2.y) - (P2.x * P1.y)) * side;
			return (a < 0) ? -1 : ((a > 0) ? 1: 0);
			}

		/* Return the equation of the line: off = kx*x + ky*y
		*  if off < mino : we are on a pixel on the left side of the line
		*  if off > maxo : we are on a pixel on the right side of the line
        *  if mino <= off <= maxo : we are on the line 
        *  
		* if invert_dir is set, left and right sides are exchanged
		*/
		void equation(int32_t & kx, int32_t & ky, int32_t & mino, int32_t & maxo, bool invert_dir = false) const
			{
			int32_t mi, ma;
			if (_x_major)
				{
				kx = _dy * _stepx;
				ky = -_dx * _stepy;
				mi = _dy - _dx;
				ma = _dy - 1;
				}
			else
				{
				kx = -_dy * _stepx;
				ky = _dx * _stepy;
				mi = _dx - _dy;
				ma = _dx - 1;
				}
			int32_t o = _frac - (_x * kx) - (_y * ky);

			bool xm =(_stepx * _stepy) > 0;
			if (_x_major) xm = (!xm);
			if (invert_dir) xm = (!xm);
			if (xm)
				{
				kx = -kx; 
				ky = -ky;
				maxo = o - mi;
				mino = o - ma;
				}
			else
				{
				mino = mi - o;
				maxo = ma - o;
				}
			}





		/** move on the line by one pixel. tmeplate version (for speed) */
		template<bool X_MAJOR> inline void move()
			{
			_len--;
			if (X_MAJOR)
				{
				if (_frac >= 0) { _y += _stepy; _frac -= _dx; }
				_x += _stepx; _frac += _dy;
				}
			else
				{
				if (_frac >= 0) { _x += _stepx; _frac -= _dy; }
				_y += _stepy; _frac += _dx;
				}
			}


		/** Same as above but not templated **/
		inline void move()
			{
			if (_x_major) move<true>(); else move<false>();
			}


		/**
		* Move the position pos by totlen pixels along a bresenham line.
		*/
		void move(int32_t totlen)
			{
			_len -= totlen;
			int32_t len = safeMultB(((_dx > _dy) ? _dx : _dy), totlen);				
			while (1)
				{
				if (_x_major)
					{
					if (_dx == 0) return; // should not happen ?
					_x += _stepx * len;
					_frac += _dy * len;
					int32_t u = _frac / _dx;
					_y += _stepy * u;
					_frac -= u * _dx;
					if (_frac >= _dy) { _frac -= _dx; _y += _stepy; }
					}
				else
					{
					if (_dy == 0) return; // should not happen ?
					_y += _stepy * len;
					_frac += _dx * len;
					int32_t u = _frac / _dy;
					_x += _stepx * u;
					_frac -= u * _dy;
					if (_frac >= _dx) { _frac -= _dy; _x += _stepx; }
					}
				totlen -= len;
				if (totlen <= 0) { return; }
				if (totlen < len) { len = totlen; }
				}
			}


		/**
		* Move the position pos by 1 pixel horizontally along the given bresenham line.
		* return the number of pixel traveled by the bresenham line.
		*
		* [x_major can be deduced from linedir but is given as template paramter for speed optimization]
		*/
		template<bool X_MAJOR> int32_t move_x_dir()
			{
			if (X_MAJOR) // compiler optimizes away the template conditionals
				{
				if (_frac >= 0) { _y += _stepy; _frac -= _dx; }
				_x += _stepx; _frac += _dy;
				_len--;
				return 1;
				}
			else
				{
				int32_t r = (_frac < ((_dx << 1) - _dy)) ? _rat : ((_dx - _frac) / _dx); // use rat value if just after a step.
				_y += (r * _stepy);
				_frac += (r * _dx);
				if (_frac < _dx) { _y += _stepy; _frac += _dx; r++; }
				_frac -= _dy;  _x += _stepx;
				_len -= r;
				return r;
				}
			}


		/** Same as above but not templated **/
		inline int32_t move_x_dir()
			{
			return (_x_major ? move_x_dir<true>() : move_x_dir<false>());
			}


		/**
		* Move the position pos by a given number of pixels horizontal along a bresenham line.
		* return the number of pixel traveled by the bresenham line.
		* do nothing and return 0 if lenx <= 0.
		*/
		int32_t move_x_dir(int32_t totlenx)
			{
			if (totlenx <= 0) return 0;
			int32_t lenx = safeMultB(((_dx > _dy) ? _dx : _dy), totlenx);
			int32_t res = 0;
			while (1)
				{
				if (_x_major)
					{
					_x += _stepx * lenx;
					_frac += _dy * lenx;
					int32_t u = _frac / _dx;
					_y += _stepy * u;
					_frac -= u * _dx;
					if (_frac >= _dy) { _frac -= _dx; _y += _stepy; }
					res += lenx;
					}
				else
					{
					int32_t k = ((lenx - 1) * _dy) / _dx;
					_frac += k * _dx;
					_y += k * _stepy;
					int32_t u = _frac / _dy;
					_frac -= u * _dy;
					if (_frac >= _dx) { u++; _frac -= _dy; }
					_x += u * _stepx;
					int32_t slen = _len; // backup because move_x_dir<> will modifiy _len
					while (u != lenx) { k += move_x_dir<false>(); u++; }
					_len = slen;
					res += k;
					}
				totlenx -= lenx;
				if (totlenx <= 0) { _len -= res; return res; }
				if (totlenx < lenx) { lenx = totlenx; }
				}
			}



		/**
		* Move the position pos by one pixel vertically along the given bresenham line.
		* return the number of pixel traveled by the bresenham line.
		*
		* [x_major can be deduced from linedir but given as template paramter for speed optimization]
		*/
		template<bool X_MAJOR> int32_t move_y_dir()
			{
			if (X_MAJOR) // compiler optimizes away the template conditionals
				{
				int32_t r = (_frac < ((_dy << 1) - _dx)) ? _rat : ((_dy - _frac) / _dy); // use rat value if just after a step.
				_x += (r * _stepx);
				_frac += (r * _dy);
				if (_frac < _dy) { _x += _stepx; _frac += _dy; r++; }
				_frac -= _dx;  _y += _stepy;
				_len -= r;
				return r;
				}
			else
				{
				if (_frac >= 0) { _x += _stepx; _frac -= _dy; }
				_y += _stepy; _frac += _dx;
				_len--;
				return 1;
				}
			}


		/** Same as above but not templated **/
		inline int32_t move_y_dir()
			{
			return (_x_major ? move_y_dir<true>() : move_y_dir<false>());
			}


		/**
		* Move the position pos along a line by a given number of pixels vertically along a bresenham line.
		* return the number of pixel traveled by the bresenham line.
		* do nothing and return 0 if leny <= 0.
		*/
		int32_t move_y_dir(int32_t totleny)
			{
			if (totleny <= 0) return 0;
			int32_t leny = safeMultB(((_dx > _dy) ? _dx : _dy), totleny);
			int32_t res = 0;
			while (1)
				{
				if (_x_major) // compiler optimizes away the template conditionals
					{
					int32_t k = ((leny - 1) * _dx) / _dy;
					_frac += k * _dy;
					_x += k * _stepx;
					int32_t u = _frac / _dx;
					_frac -= u * _dx;
					if (_frac >= _dy) { u++; _frac -= _dx; }
					_y += u * _stepy;
					int32_t slen = _len; // backup because move_x_dir<> will modifiy _len
					while (u != leny) { k += move_y_dir<true>(); u++; }
					_len = slen;
					res += k;
					}
				else
					{
					_y += _stepy * leny;
					_frac += _dx * leny;
					int32_t u = _frac / _dy;
					_x += _stepx * u;
					_frac -= u * _dy;
					if (_frac >= _dx) { _frac -= _dy; _x += _stepx; }
					res += leny;
					}
				totleny -= leny;
				if (totleny <= 0) { _len -= res; return res; }
				if (totleny < leny) { leny = totleny; }
				}
			}


		/**
		* Move the position until it is in the closed box B. Return the number of step performed by the
		* walk or -1 if the line never enters the box. (in this case, pos may be anywhere but len is set to -1)
		**/
		int32_t move_inside_box(const tgx::iBox2 & B)
			{
			if (B.isEmpty()) return -1;
			if (B.contains(tgx::iVec2{ _x, _y })) return 0;
			int32_t tot = 0;
			if (_x < B.minX)
				{
				if ((_stepx < 0) || (_dx == 0)) { _len = -1; return -1; }
				tot += move_x_dir(B.minX - _x);
				}
			else if (_x > B.maxX)
				{
				if ((_stepx > 0) || (_dx == 0)) { _len = -1; return -1; }
				tot += move_x_dir(_x - B.maxX);
				}
			if (_y < B.minY)
				{
				if ((_stepy < 0) || (_dy == 0)) { _len = -1; return -1; }
				tot += move_y_dir(B.minY - _y);
				}
			else if (_y > B.maxY)
				{
				if ((_stepy > 0) || (_dy == 0)) { _len = -1; return -1; }
				tot += move_y_dir(_y - B.maxY);
				}
			if (!B.contains(iVec2{ _x, _y })) { _len = -1; return -1; }
			return tot;
			}


		/**
		* Compute number of pixel of the line (that can be drawn) before it exits the box B.
		* If the box is empty of if pos is not in it, return 0. Otherwise, return at least 1.
		**/
		int32_t lenght_inside_box(const tgx::iBox2& B) const
			{
			if (!B.contains(iVec2{ _x, _y })) return 0;
			const int32_t hx = 1 + ((_stepx > 0) ? (B.maxX - _x) : (_x - B.minX)); // number of horizontal step before exit. 
			const int32_t hy = 1 + ((_stepy > 0) ? (B.maxY - _y) : (_y - B.minY)); // number of vertical step before exit. 
			int32_t nx = -1, ny = -1;
			if (_dx != 0) { BSeg tmp(*this); nx = tmp.move_x_dir(hx); }
			if (_dy != 0) { BSeg tmp(*this); ny = tmp.move_y_dir(hy); }
			if (nx == -1) { return ny; }
			if (ny == -1) { return nx; }
			return ((nx < ny) ? nx : ny);
			}


		/** Initialize with integer-valued positions */
		void init(iVec2 P1, iVec2 P2)
			{
			const int32_t EXP = 5;
			if (P1 == P2)
				{ // default horizontal line
				_x_major = true;
				_dx = 2; _dy = 0;
				_stepx = 1; _stepy = 1;
				_rat = 0;
				_amul = ((int32_t)1 << 28) / 2;
				_x = P1.x; _y = P1.y;
				_frac = -2;
				_len = 0;
				return;
				}
			int32_t dx = P2.x - P1.x; if (dx < 0) { dx = -dx;  _stepx = -1; } else { _stepx = 1; } dx <<= EXP; _dx = dx;
			int32_t dy = P2.y - P1.y; if (dy < 0) { dy = -dy;  _stepy = -1; } else { _stepy = 1; } dy <<= EXP; _dy = dy;
			if (dx >= dy) { _x_major = true; _rat = (dy == 0) ? 0 : (dx / dy); } else { _x_major = false; _rat = (dx == 0) ? 0 : (dy / dx); }
			_x = P1.x; _y = P1.y;
			int32_t flagdir = (P2.x > P1.x) ? 1 : 0; // used to compensante frac so that line [P1,P2] = [P2,P1]. 
			_frac = ((_x_major) ? (dy - (dx >> 1)) : (dx - (dy >> 1))) - flagdir;
			_amul = ((int32_t)1 << 28) / (_x_major ? _dx : _dy);
			_len = ((_x_major ? dx : dy) >> EXP);
			}


		/** Construct segment from P1 to P2 (real-valued positions) */
		void init(fVec2 Pf1, fVec2 Pf2)
			{
			// ************************************************************
			// Fall back to integer computations
			//init(tgx::iVec2((int)roundf(Pf1.x), (int)roundf(Pf1.y)), tgx::iVec2((int)roundf(Pf2.x), (int)roundf(Pf2.y)));
			//return;
			// ************************************************************	
			const int32_t PRECISION = 256; // 1024*16;
			bool sw = false;
			if ((Pf1.x > Pf2.x) || ((Pf1.x == Pf2.x) && (Pf1.y > Pf2.y))) { sw = true; tgx::swap(Pf1, Pf2); }
			iVec2 P1{ (int32_t)roundf(Pf1.x), (int32_t)roundf(Pf1.y) };
			iVec2 P2{ (int32_t)roundf(Pf2.x), (int32_t)roundf(Pf2.y) };
			_x = P1.x;
			_y = P1.y;
			const int32_t adx = abs(P2.x - P1.x);
			const int32_t ady = abs(P2.y - P1.y);
			const float fdx = (Pf2.x - Pf1.x);
			const float fdy = (Pf2.y - Pf1.y);
			_len = (adx > ady) ? adx : ady;
			if ((adx == 0) && (ady == 0))
				{ 
				// default horizontal line: could do better and compute _frac for shading
				// but single point line are usually not raw (since endpoint is not drawn)
				// so it does not really matter...
				if (sw) { tgx::swap(P1, P2); }
				_x_major = true;
				_dx = 2; _dy = 0;
				_stepx = 1; _stepy = 1;
				_rat = 0;
				_amul = ((int32_t)1 << 28) / 2;
				_x = P1.x; _y = P1.y;
				_frac = -2;
				_len = 0;
				return;
				}
			else if ((adx > ady) || ((adx == ady) && (abs(fdx) > abs(fdy)))) // (abs(fdx) > abs(fdy))
				{ // x major
				_x_major = true;
				const float mul = fdy / fdx;
				const float f1 = mul * (P1.x - Pf1.x) + Pf1.y - P1.y; // how much above
				const float f2 = mul * (P2.x - Pf2.x) + Pf2.y - P2.y; // how much below
				int32_t if1 = (int32_t)((2 * PRECISION) * f1); if (if1 <= -PRECISION) { if1 = -PRECISION + 1; } else if (if1 >= PRECISION) { if1 = PRECISION - 1; }
				int32_t if2 = (int32_t)((2 * PRECISION) * f2); if (if2 <= -PRECISION) { if2 = -PRECISION + 1; } else if (if2 >= PRECISION) { if2 = PRECISION - 1; }
				if (fdx < 0) { _stepx = -1; } else { _stepx = +1; }
				if (fdy < 0) { _stepy = -1;  if1 = -if1; if2 = -if2; } else { _stepy = +1; }
				_dx = adx * (2 * PRECISION);
				_dy = ady * (2 * PRECISION); _dy += -if1 + if2;
				_rat = (_dy == 0) ? 0 : (_dx / _dy);
				_amul = ((int32_t)1 << 28) / _dx;
				_frac = (if1 - PRECISION) * adx + _dy;
				}
			else
				{ // y major
				_x_major = false;
				const float mul = fdx / fdy;	
				const float f1 = mul * (P1.y - Pf1.y) + Pf1.x - P1.x;
				const float f2 = mul * (P2.y - Pf2.y) + Pf2.x - P2.x;
				int32_t if1 = (int32_t)((2 * PRECISION) * f1); if (if1 <= -PRECISION) { if1 = -PRECISION + 1; } else if (if1 >= PRECISION) { if1 = PRECISION - 1; }
				int32_t if2 = (int32_t)((2 * PRECISION) * f2); if (if2 <= -PRECISION) { if2 = -PRECISION + 1; } else if (if2 >= PRECISION) { if2 = PRECISION - 1; }
				if (fdx < 0) { _stepx = -1;  if1 = -if1; if2 = -if2; } else { _stepx = +1; }
				if (fdy < 0) { _stepy = -1; } else { _stepy = +1; }
				_dy = ady * (2 * PRECISION);
				_dx = adx * (2 * PRECISION); _dx += -if1 + if2;
				_rat = (_dx == 0) ? 0 : (_dy / _dx);
				_amul = ((int32_t)1 << 28) / _dy;
				_frac = (if1 - PRECISION) * ady + _dx;
				}
			if (sw)
				{
				reverse();
				}
			}


		/* Compute the aa value on a given side */
		template<int SIDE, bool X_MAJOR> inline int32_t AA() const
			{
			int32_t a;
			if (X_MAJOR)
				{
				a = _dy;
				a = (((a - _frac) * _amul) >> 20);
				if (SIDE > 0) { if (_stepx != _stepy) a = 256 - a; } else { if (_stepx == _stepy) a = 256 - a; }
				}
			else
				{
				a = _dx;
				a = (((a - _frac) * _amul) >> 20);
				if (SIDE > 0) { if (_stepx == _stepy) a = 256 - a; } else { if (_stepx != _stepy) a = 256 - a; }
				}
			//if (a < 32) a = 0; else if (a > 224) a = 256;
			//a = (a >> 2) + (a >> 1) + 48; // compensate (32 is the correct middle value but 48 look better IMO)
			return a; // a should be in [0,256]
			}


		/* Compute the aa value on a given side, non templatesd version */
		inline int32_t AA(int side) const
			{
			return (_x_major ? ((side > 0) ? AA<1,true>() : AA<-1,true>())
				             : ((side > 0) ? AA<1,false>() : AA<-1,false>()));
			}


		/* Compute the aa value for line anti-aliasing */
		template<bool X_MAJOR> inline int32_t AA_bothside(int & dir) const
			{
			int32_t a;
			if (X_MAJOR)
				{
				a = _dy;
				a = (((a - _frac) * _amul) >> 20);
				}
			else
				{
				a = _dx;
				a = (((a - _frac) * _amul) >> 20);
				}
			if (a > 127)
				{
				dir = (X_MAJOR) ? -_stepy : -_stepx;				
				a = (128 + 256) - a;
				}
			else
				{
				dir = (X_MAJOR) ? _stepy : _stepx;
				a = a + 128;
				}
			return a; // a should be in [0,256]
			}


		/**
        * Return a normalized direction vector
		* (use fast sqrt)
		*/
		TGX_INLINE inline fVec2 unitVec() const
			{
			return fVec2((float)(_dx * _stepx), (float)(_dy * _stepy)).getNormalize_fast();
			}

		/**
		* Query if the line is x_major
		*/
		TGX_INLINE inline bool x_major() const { return _x_major; }


		/**
		* Query step_x
		*/
		TGX_INLINE inline int32_t step_x() const { return _stepx; }


		/**
		* Query step_y
		*/
		TGX_INLINE inline int32_t step_y() const { return _stepy; }


		/**
		* Query the remaining distance to the endpoind
		*/
		TGX_INLINE inline const int32_t & len() const { return _len; }


		/**
		* remaining distance to the endpoind
		*/
		TGX_INLINE inline int32_t & len() { return _len; }


		/**
		* Increase len by 1
		*/
		TGX_INLINE inline void inclen() { _len++; }

		/**
		* Decrease len by 1
		*/
		TGX_INLINE inline void declen() { _len--; }

		/**
		* Query the current position on the line
		*/
		TGX_INLINE inline tgx::iVec2 pos() const { return { _x,_y }; }


		/**
		* x-coordinate of the current position
		*/
		TGX_INLINE inline int32_t X() const { return _x; }


		/**
		* y-coordinate of the current position
		*/
		TGX_INLINE inline int32_t Y() const { return _y; }


		/**
		* Operator== return true if both segment are currently at the same position.
		*/
		TGX_INLINE inline bool operator==(const BSeg& seg) const
			{
			return ((_x == seg._x) && (_y == seg._y));
			}


		/**
		* Operator!= return true if both segment are currently at different positions.
		*/
		TGX_INLINE inline bool operator!=(const BSeg& seg) const
			{
			return ((_x != seg._x) || (_y != seg._y));
			}


		/**
        * Save the current position 
		**/
		TGX_INLINE inline BSegState save() const;


		/**
        * Restore a position previously stored. 
		**/
		TGX_INLINE inline void restore(const BSegState& state);



		int32_t _x, _y;			// current pos
		int32_t _frac;			// fractional part
		int32_t _len;			// number of pixels
		int32_t _dx, _dy;		// step size in each direction
		int32_t _stepx, _stepy;	// directions (+/-1)
		int32_t _rat;			// ratio max(dx,dy)/min(dx,dy) to speed up computations
		int32_t _amul;			// multiplication factor to compute aa values. 
		bool  _x_major;			// true if the line is xmajor (ie dx > dy) and false if y major (dy >= dx).
	};




	/** object to save the current position on a bresenham segment */
	struct BSegState
		{
		BSegState(const BSeg& bseg) : _x(bseg._x), _y(bseg._y), _frac(bseg._frac), _len(bseg._len), _stepx(bseg._stepx), _stepy(bseg._stepy)
			{
			}

		int32_t _x, _y;			// current pos
		int32_t _frac;			// fractional part
		int32_t _len;			// number of pixels
		int32_t _stepx, _stepy;	// directions (+/-1)
		};


	BSegState BSeg::save() const
		{
		return BSegState(*this);
		}


	void BSeg::restore(const BSegState& state)
		{
		_x = state._x;
		_y = state._y;
		_frac = state._frac;
		_len = state._len;
		_stepx = state._stepx;
		_stepy = state._stepy;
		}


}


#endif

#endif

/** end of file **/

