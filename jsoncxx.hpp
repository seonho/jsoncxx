#pragma once

#include <cassert>
#define JSONCXX_ASSERT(x) assert(x)

#include "encoding.hpp"
#include "stream.hpp"

// Here defines natural and real types in global namespace.
// If user have their own definition, can define following types as differently.
typedef __int64		natural;
typedef double		real;

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

	//! Type of number
	enum NumericType : unsigned int {
		NaturalNumber,
		RealNumber,
	};

	typedef unsigned int			size_type;
}