/**
 *	@file		writer.hpp
 *	@author		seonho.oh@gmail.com
 *	@date		2013-11-01
 *	@copyright	2007-2013 seonho.oh@gmail.com
 *	@version	1.0
 */

#pragma once

#include "encoding.hpp"
#include "value.hpp"

#include <fstream>      // basic_ofstream

namespace jsoncxx
{

//!	Generic writer class
template <typename Stream, typename Encoding = UTF8<> >
class Writer
{
public:
	typedef typename Encoding::char_type        char_type;
    typedef std::basic_string<char_type>		string;
    typedef std::basic_ofstream<char_type>      ofstream;
    
	typedef typename Value<Encoding>::Number	Number;
	typedef typename Value<Encoding>::String	String;
	typedef typename Value<Encoding>::Array		Array;
	typedef typename Value<Encoding>::Object	Object;

	Writer(Stream& stream, size_type nestingLevel = 0)
		: stream_(stream), nestingLevel_(nestingLevel) {}

	Writer& operator << (const Value<Encoding>& value)
	{
		switch (value.type()) {
		case NullType:
			writeNull();
			break;
		case FalseType:
		case TrueType:
			writeBoolean(value.asBool());
			break;
		case ObjectType:
			writeObject(value.asObject());
			break;
		case ArrayType:
			writeArray(value.asArray());
			break;
		case StringType:
			writeString(value.asString());
			break;
		case NumberType:
			writeNumber(value.asNumber());
			break;
		}

		return *this;
	}

	Writer& operator << (const char_type* str)
	{
		stream_ << str;
		return *this;
	}

protected:
	//!	@name	Internal functions
	//!	@{
	void writeNull()
	{
		stream_.put('n'); stream_.put('u'); stream_.put('l'); stream_.put('l');
	}

	void writeBoolean(bool b)
	{
		if (b) {
			stream_.put('t'); stream_.put('r'); stream_.put('u'); stream_.put('e');
		} else {
			stream_.put('f'); stream_.put('a'); stream_.put('l'); stream_.put('s'); stream_.put('e');
		}
	}

	void writeNumber(const Number& n)
	{
		if (n.type_ == NaturalNumber)
			stream_ << n.num_.n;
		else
			stream_ << n.num_.r;
	}

	void writeString(const string& s)
	{
		stream_.put('\"');
		stream_ << s.c_str();
		stream_.put('\"');
	}

	void writeArray(const Array& a)
	{
		stream_.put('[');
		
		std::for_each(a.begin(), std::next(a.begin(), a.size() - 1), [&](const Value<Encoding>& value) {
			(*this) << value;
			stream_.put(',');
		});
		(*this) << a.back();

		stream_.put(']');
	}

	void writeObject(const Object& o)
	{
		stream_.put('{');

		auto func = [&](const typename Object::member_type& pair) {
			(*this) << pair.first;
			stream_.put(':');
			(*this) << pair.second;
		};

		std::for_each(o.begin(), std::next(o.begin(), o.size() - 1), [&](const typename Object::member_type& pair) {
			func(pair);
			stream_.put(',');
		});
		func(*std::next(o.begin(), o.size() - 1));

		stream_.put('}');
	}

	//!	@}

protected:
	//!
private:
	Stream&		stream_;		///< stream object
	size_type	nestingLevel_;	///< nesting level
};

}