# VectorFont
[![CMake](https://github.com/adrian-purser/vectorfont/actions/workflows/cmake.yml/badge.svg)](https://github.com/adrian-purser/vectorfont/actions/workflows/cmake.yml)

Load/Parse SVG Hershey font files. This is not intended to be a complete SVG library, quite the opposite. It is intended to be a small piece of code to simply parse these specific files. Currently, only the MOVETO and LINETO primitives are supported.

My intention is to add new formats as and when I need them.

#### Features

 - Parses SVG hershey fonts.

#### Basic Usage

##### Hershey Font Parsing

Parse the font from either a string or a file into a vectorfont object.

```C++
#include "vectorfont/hershey.h"

std::unique_ptr<vectorfont::VectorFont> p_font = vectorfont::load_hershey_font("hershey_sans.svg");

```

##### Using the vectorfont

```C++
int posx	= 0;	// Text Position x
int posy 	= 0;	// and y
int cx 		= 0;	// Cursor x
int cy 		= 0;	// and y
int size	= 32;	// Font Size

p_font->execute(
	"Hello VectorFont",
	[&](vectorfont::Primitive primitive,std::span<const int16_t> args)->bool
	{
		switch(primitive.command)
		{
			case vectorfont::command::MOVETO :
				if(args.size() >= 2)
				{
					cx = posx + ((size * args[0]) / font.units_per_em);
					cy = posy - ((size * args[1]) / font.units_per_em);
				}
				break;

			case vectorfont::command::LINETO :
				if(args.size() >= 2)
				{
					auto fx = cx;
					auto fy = cy;
					cx = position.x + ((size * args[0]) / font.units_per_em);
					cy = position.y - ((size * args[1]) / font.units_per_em);
					line(fx,fy,cx,cy);
				}
				break;

			case vectorfont::command::ADVANCE :
				if(args.size() >= 1)
					posx = posx + ((size * args[0]) / font.units_per_em);
				break;
		}
		return false;	// return false to indicate that we don't want to stop being called back.
	} );
```

