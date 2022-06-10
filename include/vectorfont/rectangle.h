//=============================================================================
//	FILE:					rectangle.h
//	SYSTEM:				
//	DESCRIPTION:	
//-----------------------------------------------------------------------------
//  COPYRIGHT:		(C) Copyright 2017 Adrian Purser. All Rights Reserved.
//	LICENCE:		
//	MAINTAINER:		Adrian Purser <ade@adrianpurser.co.uk>
//	CREATED:			24-MAR-2017 Adrian Purser <ade@adrianpurser.co.uk>
//=============================================================================
#ifndef GUARD_ADE_VECTORFONT_RECTANGLE_H
#define GUARD_ADE_VECTORFONT_RECTANGLE_H

namespace vectorfont
{

template<typename T>
struct Rectangle
{
	T		top;
	T		left;
	T		bottom;
	T		right;

	typedef T value_type;

	enum
	{
		OUTSIDE = 0,
		INSIDE,
		OVERLAPPED
	};

	Rectangle() :top((T)0), left((T)0), bottom((T)0), right((T)0) {};
	Rectangle(const Rectangle & r) : top(r.top), left(r.left), bottom(r.bottom), right(r.right) {};
	Rectangle(T l, T t, T w, T h) : top(t), left(l), bottom((t + h) - 1), right((l + w) - 1) {};
	Rectangle(T w, T h) : top((T)0), left((T)0), bottom(h - 1), right(w - 1) {};

	template<typename Tcopy>
	Rectangle(const Rectangle<Tcopy> & r)
		: top(static_cast<T>(r.top))
		, left(static_cast<T>(r.left))
		, bottom(static_cast<T>(r.bottom))
		, right(static_cast<T>(r.right)) {}

	~Rectangle() {}

	Rectangle<T> & operator=(const Rectangle<T> & r)	
	{
		top 		= r.top;
		left 		= r.left;
		bottom 	= r.bottom;
		right 	= r.right;
		return *this;
	}

	bool operator== (const Rectangle<T> & r)
	{
		return	(r.top == top) &&
			(r.left == left) &&
			(r.bottom == bottom) &&
			(r.right == right);
	}

	bool operator!= (const Rectangle<T> & r)
	{
		return	(r.top != top) ||
			(r.left != left) ||
			(r.bottom != bottom) ||
			(r.right != right);
	}

	void operator|= (const Rectangle<T> & r)
	{
		left	= (r.left < left ? r.left : left);
		top		= (r.top < top ? r.top : top);
		right	= (r.right > right ? r.right : right);
		bottom	= (r.bottom > bottom ? r.bottom : bottom);
	}

	const Rectangle<T> operator+(const Rectangle<T> & rhs) const
	{
		Rectangle<T> result(*this);
		result.add(rhs);
		return result;
	}

	Rectangle<T> & operator+=(const Rectangle<T> & rhs)
	{
		add(rhs);
		return *this;
	}

	void reset()
	{
		top = left = right = bottom = (T)0;
	}

	void set(T x, T y, T w, T h)
	{
		top		= y;
		left	= x;
		right	= (x + w) - 1;
		bottom	= (y + h) - 1;
	}

	const Rectangle<T> & resize(T w, T h)
	{
		right	= (left + w) - 1;
		bottom	= (top + h) - 1;
		return *this;
	}

	void move(T x, T y)
	{
		right	= x + (right - left);
		bottom	= y + (bottom - top);
		left	= x;
		top		= y;
	}

	const Rectangle<T> & move_relative(T x, T y)
	{
		left	+= x;
		right	+= x;
		top		+= y;
		bottom	+= y;
		return *this;
	}

	T		width() const { return (right - left) + 1; }
	T		height() const { return (bottom - top) + 1; }
	bool	pt_in_rect(T x, T y) const { return !!((x >= left) && (x <= right) && (y >= top) && (y <= bottom)); }

	const Rectangle<T> & deflate(T val)
	{
		top		+= val;
		bottom	-= val;
		left	+= val;
		right	-= val;
		if (top>bottom)	top = bottom = (bottom + ((top - bottom) / ((T)2)));
		if (left>right)	left = right = (right + ((left - right) / ((T)2)));
		return *this;
	}

	const Rectangle<T> & inflate(T val)
	{
		top		-= val;
		bottom	+= val;
		left	-= val;
		right	+= val;
		return *this;
	}

	void add(T x, T y)
	{
		if (x < left)	left = x;
		if (y < top)	top = y;
		if (x > right)	right = x;
		if (y > bottom)	bottom = y;
	}

	void add(T x, T y, T w, T h)
	{
		if (x<left)				left = x;
		if (y<top)				top = y;
		if ((x + w) > right)	right = (x + w);
		if ((y + h) > bottom)	bottom = (y + h);
	}

	void add(const Rectangle<T> & r)
	{
		if (r.left<left)		left = r.left;
		if (r.top<top)			top = r.top;
		if (r.right > right)	right = r.right;
		if (r.bottom > bottom)	bottom = r.bottom;
	}

	bool contains(const Rectangle<T> & r) const
	{
		return((r.left >= left) &&
			(r.right <= right) &&
			(r.top >= top) &&
			(r.bottom <= bottom));
	}

	bool touches(const Rectangle<T> & r) const
	{
		return !((r.right <= left) ||
			(r.left >= right) ||
			(r.top >= bottom) ||
			(r.bottom <= top));
	}

	int compare(const Rectangle<T> & r) const
	{
		if ((r.right <= left) ||
			(r.left >= right) ||
			(r.top >= bottom) ||
			(r.bottom <= top))
			return OUTSIDE;

		if ((r.left >= left) &&
			(r.right <= right) &&
			(r.top >= top) &&
			(r.bottom <= bottom))
			return INSIDE;

		return OVERLAPPED;
	}

	bool clip(const Rectangle<T> & r)
	{
		if ((right <= r.left) ||
			(left >= r.right) ||
			(top >= r.bottom) ||
			(bottom <= r.top))
		{
			right = left = top = bottom = (T)0;
			return false;
		}

		if ((left >= r.left) &&
			(right <= r.right) &&
			(top >= r.top) &&
			(bottom <= r.bottom))
			return true;


		if (left < r.left)		left = r.left;
		if (right > r.right)	right = r.right;
		if (top < r.top)		top = r.top;
		if (bottom > r.bottom)	bottom = r.bottom;

		return true;
	}

	bool point_inside(const T & x, const T & y) const noexcept
	{
		return !!((x >= left) && (y >= top) && (x<right) && (y<bottom));
	}

	T area() const { return (right - left)*(bottom - top); }

};

} // namespace vectorfont

#endif // ! defined GUARD_ADE_VECTORFONT_RECTANGLE_H

