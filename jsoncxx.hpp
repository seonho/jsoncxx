/**
 *	@file		jsoncxx.hpp
 *	@author		seonho.oh@gmail.com
 *	@date		2013-11-01
 *	@copyright	2007-2014 seonho.oh@gmail.com
 *	@version	1.0
 */

#pragma once

#include <cassert>
#define JSONCXX_ASSERT(x) assert(x)

#include "jsoncxx.hpp"
#include "stream.hpp"
#include "reader.hpp"
#include "writer.hpp"

//!	A template-based JSON parser and generator with simple and intuitive interface.
namespace jsoncxx
{
    typedef Value<UTF8<> >                          value;
    typedef Reader<StringStream<UTF8<> >, UTF8<> >  reader;
    typedef Writer<StringStream<UTF8<> >, UTF8<> >  writer;
}