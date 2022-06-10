//=============================================================================
//	FILE:					vectorfont.h
//	SYSTEM:				 	
//	DESCRIPTION:	Stroked Vector Font Structure
//-----------------------------------------------------------------------------
//  COPYRIGHT:		(C)Copyright 2020 Adrian Purser. All Rights Reserved.
//	LICENCE:
//	MAINTAINER:		AJP - Adrian Purser <ade&arcadestuff.com>
//	CREATED:			22-DEC-2020 Adrian Purser <ade&arcadestuff.com>
//=============================================================================
#ifndef GUARD_ADE_VECTORFONT_H
#define GUARD_ADE_VECTORFONT_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <algorithm>
#include <functional>
#include <span>
#include "rectangle.h"

namespace vectorfont
{

using Rect = vectorfont::Rectangle<int16_t>;

namespace command
{
enum 
{
// 	Command						Parameters
		MOVETO,				//	x, y
		LINETO,				//	x, y
		ADVANCE,			//	delta_x
};
} // namespace command

namespace cap
{
enum
{
	ROUND,
	SQUARE
};
} // namesapce cap

struct Primitive
{
	unsigned char command:4;
	unsigned char cap:2 = 0;
};

struct Glyph
{
	uint32_t					code;
	vectorfont::Rect	rect;
	uint16_t					parameter_index;
	uint16_t					primitive_index;
	uint16_t					primitive_count;
	int16_t						advance_x;
};

struct Font
{
	std::vector<int16_t>								parameters;
	std::vector<vectorfont::Primitive>	primitives;
	std::vector<vectorfont::Glyph>			glyphs;
	Rect																rect;
	
	std::string													id;

	int16_t															ascent 				= 0;
	int16_t															descent				= 0;
	int16_t															units_per_em 	= 0;
	int16_t															missing_adv_x	= 0;

	int16_t																			width() const noexcept 			{return rect.width();}
	int16_t																			height() const noexcept 		{return rect.height();}
	inline const vectorfont::Glyph *				 		get_glyph(uint32_t code) const;
	inline void 																update_size(int16_t x,int16_t y) noexcept;
	inline void																	start_glyph(uint32_t code, int16_t advance_x);
	inline void																	moveto(int16_t x, int16_t y);	
	inline void																	lineto(int16_t x, int16_t y, int linecap = vectorfont::cap::ROUND);

	template<typename T>	void									execute(const T& string, std::function<bool(vectorfont::Primitive,std::span<const int16_t>)> callback) const;
	inline void																	execute(uint32_t code, std::function<bool(vectorfont::Primitive,std::span<const int16_t>)> callback) const;

	template<typename T>	vectorfont::Rect			string_rect(const T& string) const;
};

inline const vectorfont::Glyph * 	
Font::get_glyph(uint32_t code) const
{
	auto ifind = std::find_if(begin(glyphs),end(glyphs),[code](const vectorfont::Glyph & glyph)->bool{return glyph.code == code;});
	if(ifind == end(glyphs))
		return nullptr;
	return &(*ifind); 
}

void Font::start_glyph(uint32_t code, int16_t advance_x )
{
	vectorfont::Glyph glyph;
	glyph.code 						= code;
	glyph.advance_x				= advance_x;
	glyph.parameter_index = parameters.size();
	glyph.primitive_index = primitives.size();
	glyph.primitive_count	= 0;
	glyphs.push_back(glyph);
}

void Font::moveto(int16_t x, int16_t y)
{
	if(!glyphs.empty())
	{
		parameters.push_back(x);
		parameters.push_back(y);

		Primitive primitive;
		primitive.command = vectorfont::command::MOVETO;
		primitive.cap			= 0;
		primitives.push_back(primitive);

		auto & glyph = glyphs.back();
		glyph.primitive_count++;
		glyph.rect.add(x,y);
		rect.add(x,y);
	}
}

void Font::lineto(int16_t x, int16_t y, int linecap)
{
	if(!glyphs.empty())
	{
		parameters.push_back(x);
		parameters.push_back(y);

		Primitive primitive;
		primitive.command = vectorfont::command::LINETO;
		primitive.cap			= linecap;
		primitives.push_back(primitive);

		auto & glyph = glyphs.back();
		glyph.primitive_count++;
		glyph.rect.add(x,y);
		rect.add(x,y);
	}
}

template<typename T>
void
Font::execute(const T & string, std::function<bool(vectorfont::Primitive,std::span<const int16_t>)> callback) const
{
	for(uint32_t code : string)
		execute(code,callback);
}

void
Font::execute(uint32_t code, std::function<bool(vectorfont::Primitive,std::span<const int16_t>)> callback) const
{
	const auto p_glyph = get_glyph(code);
	if(p_glyph != nullptr)
	{
		auto glyph = *p_glyph;

		while(glyph.primitive_count-- > 0)
		{
			size_t pcount=1;
			switch(primitives[glyph.primitive_index++].command)
			{
				case vectorfont::command::MOVETO : pcount=2; glyph.parameter_index += 2; break;
				case vectorfont::command::LINETO : pcount=2; glyph.parameter_index += 2; break;
			}
			if(callback( primitives[glyph.primitive_index], {&parameters[glyph.parameter_index],pcount} ))
				break;
		}
		callback({vectorfont::command::ADVANCE},{&glyph.advance_x,1});
	}
	else
		callback({vectorfont::command::ADVANCE},{&missing_adv_x,1});
}


template<typename T>	
vectorfont::Rect
Font::string_rect(const T& string) const
{
	vectorfont::Rect rect;
	int16_t x = 0;

	for(uint32_t code : string)
	{
		const auto p_glyph = get_glyph(code);
		if(p_glyph)
		{
			auto grect = p_glyph->rect;
			grect.move_relative(x,0);
			rect.add(grect);
			x += p_glyph->advance_x;
		}
		else
			x += missing_adv_x;		
	}

	return rect;
}




} // namespace vectorfont

#endif // ! defined GUARD_ADE_VECTORFONT_H
