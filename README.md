# VectorFont

#### Features

 - Parses hershey fonts.

#### Basic Usage

Parse the font from either a string or a file into a vectorfont object.

```C++
#include "vectorfont/hershey.h"

std::unique_ptr<vectorfont::VectorFont> p_font = vectorfont::load_hershey_font("hershey_sans.svg");

```


