//=============================================================================
//	FILE:					hershey.h
//	SYSTEM:				
//	DESCRIPTION:	Parse hershey font files into vectorfont object.
//-----------------------------------------------------------------------------
//  COPYRIGHT:		(C)Copyright 2020 Adrian Purser. All Rights Reserved.
//	LICENCE:
//	MAINTAINER:		AJP - Adrian Purser <ade&arcadestuff.com>
//	CREATED:			28-DEC-2020 Adrian Purser <ade&arcadestuff.com>
//=============================================================================
#ifndef GUARD_ADE_VECTORFONT_HERSHEY_H
#define GUARD_ADE_VECTORFONT_HERSHEY_H

#include <string>
#include <string_view>
#include <optional>
#include <memory>
#include "vectorfont.h"

namespace vectorfont
{

std::unique_ptr<vectorfont::Font>		parse_hershey_font(const std::string_view src);
std::unique_ptr<vectorfont::Font>		load_hershey_font(std::string_view filename);

} // namespace vectorfont

#endif // ! defined GUARD_ADE_VECTORFONT_HERSHEY_H
