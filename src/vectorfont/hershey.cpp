//=============================================================================
//	FILE:					hershey.cpp
//	SYSTEM:				
//	DESCRIPTION:	
//-----------------------------------------------------------------------------
//  COPYRIGHT:		(C)Copyright 2020 Adrian Purser. All Rights Reserved.
//	LICENCE:
//	MAINTAINER:		AJP - Adrian Purser <ade&arcadestuff.com>
//	CREATED:			28-DEC-2020 Adrian Purser <ade&arcadestuff.com>
//=============================================================================

#include <charconv>
#include <iterator>
#include <array>
#include <memory>
#include "vectorfont/hershey.h"
#include "vectorfont/xml.h"

namespace vectorfont
{

static int
parse_svg_path_data(const std::string_view src, std::function<bool (char,const std::array<int,8> &, int argc)> callback)
{

	std::array<int,8>	args;
	char 							command = 0;
	int 							argc = 0;
	int								argi = 0;
	int								argstart = -1;
	bool							b_error = false;
	bool 							b_finished = false;

//	std::cout << "Parsing : " << src << std::endl;

	for(std::string::size_type i=0;(i<=src.size()) && !b_error && !b_finished;++i)
	{
		// Check whether we have arrived at a break point.
		if( (i>=src.size()) || isalpha(src[i]) || (src[i] == ' ') || (src[i] == '\t') )
		{
			if((command != 0) && (argstart > 0))
			{
				auto str = src.substr(argstart,i-argstart);
				
				int arg;
				if(auto [p, ec] = std::from_chars(str.data(),str.data() + str.size(), arg); ec == std::errc())
				{
					args[argi++] = arg;
					if(argi >= argc)
					{
						b_finished = !callback(command,args,argi);

//						std::cout << "  callback: " << command << ' ';
//						for(int ai=0;ai<argi;++ai)
//							std::cout << args[ai] << ' ';
//						std::cout << std::endl;
						argi = 0;
					}
				}
				else
				{
					std::cerr << "Failed to parse argument '" << src.substr(argstart,i-argstart) << "'\n";
					b_error = true;
					break;
				}
				argstart = -1;
			}
		}

		if(i<src.size())
		{
			if(isalpha(src[i]))
			{
				command = src[i];

				switch(command)
				{
					case 'M' :
					case 'L' :
					case 'm' :
					case 'l' :
						argc = 2;
						break;

					default :
						argc = 0;
						command = 0;
						break;
				}

				argstart = -1;
			}
			else if((command != 0) && (src[i] != ' ') && (src[i] != '\t'))
			{
				if(argstart < 0)
					argstart  = i;
			}
		}
	
	}
	return 0;
}

static bool
parse_glyph(const ade::xml::XMLElement & glyph_element, vectorfont::Font & font)
{
	std::string attr;
	uint32_t code = 0U;
	bool b_error = false;

	if(!glyph_element.get_attribute("unicode",attr))
		return true;

	if(attr.size() > 1)
		return false;

	code = attr[0];

	int adv_x = font.missing_adv_x;
	glyph_element.get_attribute("horiz-adv-x",adv_x);

	if(glyph_element.get_attribute("d",attr))
	{
		font.start_glyph(code,adv_x);
		int16_t cursor_x = 0;
		int16_t cursor_y = 0;

		parse_svg_path_data(attr, [&](char command,const std::array<int,8> & args,int argc) -> bool
			{
				if(argc < 2)
				{
					std::cerr << "Missing args in SVG path command '" << command << "'\n";
					b_error = true;
					return false;
				}

				switch(command)
				{
					case 'M' : cursor_x = args[0]; cursor_y = args[1]; 		font.moveto(cursor_x,cursor_y); break;
					case 'L' : cursor_x = args[0]; cursor_y = args[1]; 		font.lineto(cursor_x,cursor_y); break;
					case 'm' : cursor_x += args[0]; cursor_y += args[1]; 	font.moveto(cursor_x,cursor_y); break;
					case 'l' : cursor_x += args[0]; cursor_y += args[1]; 	font.lineto(cursor_x,cursor_y); break;
					default : 	std::cerr << "Unsupported command '" << command << "'\n"; b_error = true; return false;
				}
				return true;
			});
	}

	return b_error;
}

static bool
parse_font_face(const ade::xml::XMLElement & font_face, vectorfont::Font & font)
{
	font_face.get_attribute("units-per-em", font.units_per_em);
	font_face.get_attribute("ascent", font.ascent);
	font_face.get_attribute("descent", font.descent);
	return false;
}

static
std::unique_ptr<vectorfont::Font>
parse_font_element(const ade::xml::XMLElement & font_element)
{
	auto p_font = std::make_unique<vectorfont::Font>();
	auto & font = *p_font;

	int advance_x = 0;

	font_element.get_attribute("id",font.id);
	font_element.get_attribute("horiz-adv-x",advance_x);

	std::cout << "id: " << font.id << " advx: " << advance_x << std::endl;

	bool b_error = false;

	font_element.find_elements("",[&](const ade::xml::XMLElement * p_element)->bool
		{
			auto name = p_element->value();
			if(name == "font-face") 						b_error = parse_font_face(*p_element,font);
			else if(name == "missing-glyph")		p_element->get_attribute("horiz-adv-x", font.missing_adv_x);
			else if(name == "glyph")						b_error = parse_glyph(*p_element,font);
			else std::cout << "Unhandled element: '" << name << "'\n";
			return b_error;
		});


	std::cout 	<< "\nFONT: id: " << font.id 
							<< "\n    ascent:          " << font.ascent
							<< "\n    descent:         " << font.descent
							<< "\n    units-per-em:    " << font.units_per_em
							<< "\n    missing-adv-x:   " << font.missing_adv_x
							<< std::endl;

	if(b_error)
		return nullptr;

	return p_font;
}

std::unique_ptr<vectorfont::Font>
parse_hershey_font(const std::string_view src)
{

	//-------------------------------------------------------------------------
	//  Parse the document
	//-------------------------------------------------------------------------
	ade::xml::XMLDocument   doc;

	if(doc.parse(src.data(),src.size()))
	{
		std::cerr << "Failed to parse file! - " << doc.get_error_string() << std::endl;
		return {};
	}

	auto p_root = doc.get_root_element();
	if(!p_root)
	{
		std::cerr << "Failed to get document root!" << std::endl;
		return {};
	}

	std::cout << "Loaded!\nRoot = " << p_root->value() << std::endl;

	if(p_root->value() != "svg")
	{
		std::cerr << "File is not SVG! (" << p_root->value() << '\n';
		return {};
	}

	auto p_defs = p_root->get_element("defs");
	if(p_defs == nullptr)
	{
		std::cerr << "'defs' element not found!\n";
		return {};
	}

	auto p_font = p_defs->get_element("font");
	if(p_font == nullptr)
	{
		std::cerr << "'font' element not found!\n";
		return {};
	}
	
	return parse_font_element(*p_font);
}


std::unique_ptr<vectorfont::Font>
load_hershey_font(std::string_view filename)
{
	std::ifstream infile(filename.data());
	if(infile.fail())
		return {};

	std::string str(std::istreambuf_iterator<char>{infile}, {});

	return parse_hershey_font(str);
}

} // namespace vectorfont

