/**
 *	@file		jsoncxx.hpp
 *	@author		seonho.oh@gmail.com
 *	@date		2013-11-01
 *	@copyright	2007-2013 seonho.oh@gmail.com
 *	@version	1.0
 */

#pragma once

#include <cassert>
#define JSONCXX_ASSERT(x) assert(x)

#include "encoding.hpp"
#include "stream.hpp"

///////////////////////////////////////////////////////////////////////////////
// Type
namespace jsoncxx
{
	//! Type of JSON value
	enum ValueType {
		NullType,	//!< null
		FalseType,	//!< false
		TrueType,	//!< true
		ObjectType,	//!< object
		ArrayType,	//!< array
		StringType,	//!< string
		NumberType,	//!< number
	};

	//! Type define of internal number types
	typedef __int64		natural;
	typedef double		real;

	//! Type of number
	enum NumericType : unsigned int {
		NaturalNumber,
		RealNumber,
	};

	typedef unsigned int			size_type;
}