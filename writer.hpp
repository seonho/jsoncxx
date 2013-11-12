/**
 *	@file		value.hpp
 *	@author		seonho.oh@gmail.com
 *	@date		2013-11-01
 *	@copyright	2007-2013 seonho.oh@gmail.com
 *	@version	1.0
 */

#pragma once

#include "jsoncxx.hpp"

namespace jsoncxx
{

template <typename Stream, typename Encoding = UTF8<> >
class writer
{
	typedef typename generic_value<Encoding>::number	Number;
	typedef typename generic_value<Encoding>::string	String;
	typedef typename generic_value<Encoding>::array		Array;
	typedef typename generic_value<Encoding>::object	Object;
	typedef std::basic_string<typename Encoding::char_type> generic_string;

public:
	typedef typename Encoding::char_type char_type;

	writer(Stream& stream, size_type nestingLevel = 0)
		: stream_(stream), nestingLevel_(nestingLevel) {}

	writer& operator << (const generic_value<Encoding>& value)
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

	writer& operator << (const char_type* str)
	{
		stream_ << str;
		return *this;
	}

protected:
	//@name	
	//@{
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

	void writeString(const generic_string& s)
	{
		stream_.put('\"');
		stream_ << s.c_str();
		stream_.put('\"');
	}

	void writeArray(const Array& a)
	{
		stream_.put('[');
		
		std::for_each(a.begin(), std::next(a.begin(), a.size() - 1), [&](const generic_value<Encoding>& value) {
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

	//@}

protected:
	//!
private:
	Stream&		stream_;
	size_type	nestingLevel_;
};

}

