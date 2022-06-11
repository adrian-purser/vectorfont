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
#include <functional>
#include <string_view>
#include "vectorfont/hershey.h"
#include "vectorfont/xml.h"

namespace vectorfont
{

//=============================================================================
//	tokenize
//-----------------------------------------------------------------------------
//	Extracts tokens from a string based upon the specified delimeters and quote
//	marks. The tokens are returned through a callback.
//	Returns the number of tokens found.
//=============================================================================
static 
int
tokenize(	const std::string_view & str,
					std::function<bool(const std::string_view &)> callback,
					std::string_view delimeters = ",",
					std::string_view quotes = "\"'" )
{
	std::string_view::size_type index = 0;
	bool b_finished = false;
	int count = 0;

	while(!b_finished && (index < str.size()))
	{
		const auto ch = str[index];

		if((ch == ' ') || (ch == '\t'))
		{
			++index;
			continue;
		}

		auto qpos = quotes.find_first_of(ch);
		if(qpos != std::string_view::npos)
		{
			++index;
			if(index < str.size())
			{
				const auto qepos = str.find_first_of(quotes[qpos],index);
				if(qepos == std::string_view::npos)
				{
					b_finished = !callback(str.substr(index));
					++count;
					index = str.size();
				}
				else
				{
					b_finished = !callback(str.substr(index,qepos-index));
					++count;
					index = qepos+1;

					const auto dpos = str.find_first_of(delimeters,index);
					index = (dpos == std::string_view::npos ? str.size() : dpos + 1); 
				}
			}
		}
		else
		{
			const auto dpos = delimeters.find_first_of(ch);
			if(dpos != std::string_view::npos)
			{
				b_finished = !callback(std::string_view());
				++count;
				index = dpos+1;
			}
			else
			{
				const auto depos = str.find_first_of(delimeters,index);
				if(depos == std::string_view::npos)
				{
					b_finished = !callback(str.substr(index));
					++count;
					index = str.size();
				}
				else
				{
					b_finished = !callback(str.substr(index,depos-index));
					++count;
					index = depos+1;
				}
			}
		}
	}

	return count;
}


static int
parse_svg_path_data(const std::string_view src, std::function<bool (char,const std::array<int,8> &, int argc)> callback)
{
	std::array<int,8>	args;
	char 							command = 0;
	int 							argc = 0;
	int								argi = 0;
	bool							b_error = false;
	bool 							b_finished = false;

	tokenize(src,[&](const std::string_view str)->bool
		{
			if(str.empty())
				return true;

			if(command == 0)
			{
				command = str[0];

				switch(command)
				{
					case 'M' :
					case 'L' :
					case 'm' :
					case 'l' :
						argc = 2;
						argi = 0;
						break;

					default :
						argc = 0;
						command = 0;
						break;
				}
			}
			else
			{
				int arg;
				if(auto [p, ec] = std::from_chars(str.data(),str.data() + str.size(), arg); ec == std::errc())
				{
					args[argi++] = arg;
					if(argi >= argc)
					{
						b_finished = !callback(command,args,argi);
						command = 0;
					}
				}
				else
				{
					std::cerr << "Failed to parse argument '" << str << "'\n";
					b_error = true;
				}
			}

			return !b_error && !b_finished;
		}, " \t");

		return b_error ? -1 : 0;
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
				switch(argc)
				{
					case 2 :
						switch(command)
						{
							case 'M' : cursor_x = args[0]; cursor_y = args[1]; 		font.moveto(cursor_x,cursor_y); break;
							case 'L' : cursor_x = args[0]; cursor_y = args[1]; 		font.lineto(cursor_x,cursor_y); break;
							case 'm' : cursor_x += args[0]; cursor_y += args[1]; 	font.moveto(cursor_x,cursor_y); break;
							case 'l' : cursor_x += args[0]; cursor_y += args[1]; 	font.lineto(cursor_x,cursor_y); break;
							default : 	
								//std::cerr << "Unsupported command '" << command << "'\n"; 
								//b_error = true; 
								return false;
						}
						break;

					default :
						//std::cerr << "Unsupported command '" << command << "'\n";
						break;
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

/*
	std::cout 	<< "\nFONT: id: " << font.id 
							<< "\n    ascent:          " << font.ascent
							<< "\n    descent:         " << font.descent
							<< "\n    units-per-em:    " << font.units_per_em
							<< "\n    missing-adv-x:   " << font.missing_adv_x
							<< std::endl;
*/
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

