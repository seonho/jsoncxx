/**
 *	@file		value.hpp
 *	@author		seonho.oh@gmail.com
 *	@date		2013-11-01
 *	@copyright	2007-2013 seonho.oh@gmail.com, Copyright (c) 2011 Milo Yip (miloyip@gmail.com)
 *	@version	1.0
 */

#pragma once

#include "jsoncxx.hpp"
#include <regex>

namespace jsoncxx
{

///////////////////////////////////////////////////////////////////////////////
// ParseType

//! Combination of parse flag
enum ParseFlag : unsigned int
{
	ParseDefault,	//!< Default parse type. Non-destructive parsing. Text strings are decoded into allocated buffer.
	ParseInplace	//!< Inplace(destructive) parsing.
};

///////////////////////////////////////////////////////////////////////////////
// SkipWhitespace
// Copyright (c) 2011 Milo Yip (miloyip@gmail.com)
// Version 0.1

//! Skip the JSON white spaces in a stream.
/*! \param stream A input stream for skipping white spaces.
	\note This function has SSE2/SSE4.2 specialization.
*/
template<typename Stream>
void SkipWhitespace(Stream& stream) {
	Stream s = stream;	// Use a local copy for optimization
	while (s.peek() == ' ' || s.peek() == '\n' || s.peek() == '\r' || s.peek() == '\t')
		s.take();
	stream = s;
}

#ifdef JSONCXX_SSE42
//! Skip whitespace with SSE 4.2 pcmpistrm instruction, testing 16 8-byte characters at once.
inline const char *SkipWhitespace_SIMD(const char* p) {
	static const char whitespace[16] = " \n\r\t";
	__m128i w = _mm_loadu_si128((const __m128i *)&whitespace[0]);

	for (;;) {
		__m128i s = _mm_loadu_si128((const __m128i *)p);
		unsigned r = _mm_cvtsi128_si32(_mm_cmpistrm(w, s, _SIDD_UBYTE_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_BIT_MASK | _SIDD_NEGATIVE_POLARITY));
		if (r == 0)	// all 16 characters are whitespace
			p += 16;
		else {		// some of characters may be non-whitespace
#ifdef _MSC_VER		// Find the index of first non-whitespace
			unsigned long offset;
			if (_BitScanForward(&offset, r))
				return p + offset;
#else
			if (r != 0)
				return p + __builtin_ffs(r) - 1;
#endif
		}
	}
}

#elif defined(JSONCXX_SSE2)

//! Skip whitespace with SSE2 instructions, testing 16 8-byte characters at once.
inline const char *SkipWhitespace_SIMD(const char* p) {
	static const char whitespaces[4][17] = {
		"                ",
		"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n",
		"\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r",
		"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"};

	__m128i w0 = _mm_loadu_si128((const __m128i *)&whitespaces[0][0]);
	__m128i w1 = _mm_loadu_si128((const __m128i *)&whitespaces[1][0]);
	__m128i w2 = _mm_loadu_si128((const __m128i *)&whitespaces[2][0]);
	__m128i w3 = _mm_loadu_si128((const __m128i *)&whitespaces[3][0]);

	for (;;) {
		__m128i s = _mm_loadu_si128((const __m128i *)p);
		__m128i x = _mm_cmpeq_epi8(s, w0);
		x = _mm_or_si128(x, _mm_cmpeq_epi8(s, w1));
		x = _mm_or_si128(x, _mm_cmpeq_epi8(s, w2));
		x = _mm_or_si128(x, _mm_cmpeq_epi8(s, w3));
		unsigned short r = ~_mm_movemask_epi8(x);
		if (r == 0)	// all 16 characters are whitespace
			p += 16;
		else {		// some of characters may be non-whitespace
#ifdef _MSC_VER		// Find the index of first non-whitespace
			unsigned long offset;
			if (_BitScanForward(&offset, r))
				return p + offset;
#else
			if (r != 0)
				return p + __builtin_ffs(r) - 1;
#endif
		}
	}
}

#endif // JSONCXX_SSE2

#ifdef JSONCXX_SIMD
//! Template function specialization for inplace_stringstream
template<> inline void SkipWhitespace(inplace_stringstream& stream) { 
	stream.src_ = const_cast<char*>(SkipWhitespace_SIMD(stream.src_));
}

//! Template function specialization for stringstream
template<> inline void SkipWhitespace(stringstream& stream) {
	stream.src_ = SkipWhitespace_SIMD(stream.src_);
}
#endif // JSONCXX_SIMD

template <typename Stream, typename Encoding = UTF8<> >
class generic_reader
{
public:
	typedef typename Encoding::char_type char_type;

	//! ctor
	generic_reader() {}
	
	//bool hasParseError() const { return parseError_ != nullptr; }
	//const char* getParseError() const { return parseError_; }
	//size_type getErrorOffset() const { return errorOffset_; }

	bool parse(const char* filename, generic_value<Encoding>& root)
	{
		std::ifstream fin(filename);
		std::string json;
		std::getline(fin, json, (char)EOF);
		generic_stringstream<Encoding> s(json.c_str());
		return parse(s, root);
	}

	bool parse(Stream& stream, generic_value<Encoding>& root, ParseFlag flag = ParseDefault)
	{
		// comment string "//"
		// /* */
		SkipWhitespace(stream);

		try {
			if (stream.peek() == '\0')
				throw std::runtime_error("Text only contains white space(s).");
			else {
				switch (stream.peek()) {
				case '{':
					root = parseObject(stream, flag); 
					break;
				case '[':
					root = parseArray (stream, flag); 
					break;
				default:
					throw std::runtime_error("Except either an object or array at root.");
				}

				SkipWhitespace(stream);

				if (stream.peek() != '\0')
					throw std::runtime_error("Nothig should follow the root object or array.");
			}
		} catch (std::exception& e) {
			std::cerr << e.what() << std::endl;
			return false;
		}

		return true;
	}

//private:
	////! Parse JSON text.
	//template <unsigned int Flag, typename Stream, typename Handler>
	//bool parse(Stream& stream, Handler& handler)
	//{
	//	parseError_ = nullptr;
	//	errorOffset_ = 0;
	//	return true;
	//}

	generic_value<Encoding> parseObject(Stream& stream, ParseFlag flag)
	{
		generic_value<Encoding> ret(ObjectType);

		assert(stream.peek() == '{');
		stream.take(); // skip '{'
		SkipWhitespace(stream);

		if (stream.peek() == '}') {
			stream.take();
			return ret;
		}

		while(true) {
			if (stream.peek() != '"')
				throw std::runtime_error("Name of an object member must be a string"); // stream.tell();

			generic_value<Encoding> key = parseString(stream, flag);
			SkipWhitespace(stream);

			if (stream.take() != ':')
				throw std::runtime_error("There must be a colon after the name of object member"); // stream.tell();

			SkipWhitespace(stream);

			//generic_value<Encoding> value = parseValue(stream, flag);

			ret.insert(std::move(key), parseValue(stream, flag));

			SkipWhitespace(stream);

			switch (stream.take()) {
			case ',': SkipWhitespace(stream); break;
			case '}': return ret;
			default:
				throw std::runtime_error("Must be a comma or '}' after an object member"); // stream.tell();
			}
		}

		return ret;
	}

	//! Parse array: [ value, ... ]
	generic_value<Encoding> parseArray(Stream& stream, ParseFlag flag)
	{
		generic_value<Encoding> ret(ArrayType);

		assert(stream.peek() == '[');
		stream.take(); // skip '['
		SkipWhitespace(stream);

		if (stream.peek() == ']') {
			stream.take();
			return ret;
		}

		while (true) {
			ret.append(parseValue(stream, flag));
			SkipWhitespace(stream);

			switch (stream.take()) {
			case ',':
				SkipWhitespace(stream);
				break;
			case ']':
				return ret;
			default:
				throw std::runtime_error("Must be a comma or ']' after an array member"); // stream.tell();
			}
		}

		return ret;
	}

	generic_value<Encoding> parseNull(Stream& stream, ParseFlag flag)
	{
		assert(stream.peek() == 'n');
		stream.take();

		if (stream.take() == 'u' && 
			stream.take() == 'l' &&
			stream.take() == 'l')
			return generic_value<Encoding>(); // null
		else
			throw std::runtime_error("Invalid value"); // stream.tell() - 1;
	}

	generic_value<Encoding> parseTrue(Stream& stream, ParseFlag flag)
	{
		assert(stream.peek() == 't');
		stream.take();

		if (stream.take() == 'r' && 
			stream.take() == 'u' &&
			stream.take() == 'e')
			return generic_value<Encoding>(TrueType); // true
		else
			throw std::runtime_error("Invalid value"); // stream.tell() - 1;
	}

	generic_value<Encoding> parseFalse(Stream& stream, ParseFlag flag)
	{
		assert(stream.peek() == 'f');
		stream.take();

		if (stream.take() == 'a' && 
			stream.take() == 'l' &&
			stream.take() == 's' &&
			stream.take() == 'e')
			return generic_value<Encoding>(FalseType); // false
		else
			throw std::runtime_error("Invalid value"); // stream.tell() - 1;
	}

	generic_value<Encoding> parseNumber(Stream& stream, ParseFlag flag)
	{
		typedef std::basic_regex<char_type> generic_regex;

		Stream s = stream; // Local copy for optimization
		
		// parse number
		while ((s.peek() >= '0' && s.peek() <= '9') ||
				s.peek() == '.' ||
				s.peek() == 'e' || s.peek() == 'E' ||
				s.peek() == '-' || s.peek() == '+') s.take();
		
		/*std::regex re("^(?:-?[0-9]+\\.?[0-9]+([eE][-+]?[0-9]+)?)");
		std::smatch match;
		bool matched = std::regex_match(stream.src_, s.src_, match, re);*/

		generic_value<Encoding> ret(NumberType);

		std::string number(stream.src_, s.src_);

		if (number.find_first_of('.') == std::string::npos)
			ret = std::move(generic_value<Encoding>((natural)std::stoll(number)));
		else
			ret = std::move(generic_value<Encoding>(std::stod(number)));

		stream = s;

		return ret;
	}

	generic_value<Encoding> parseString(Stream& stream, ParseFlag flag)
	{
		generic_value<Encoding> ret;
		Stream s = stream;
		assert(s.peek() == '\"');
		s.take(); // skip '\"'

		char_type* head;
		size_type length;

		if (flag & ParseInplace)
			head = s.begin();
		else
			length = 0;

		//bool status = true;
		while (true/*status*/) {
			switch (s.peek()) {
			case '\\': // escape
				break;
			case 'u': // unicode
				break;
			case '\"':
				// add string
				ret = generic_value<Encoding>(stream.src_ + 1, s.src_); // without '\"'
				s.take();
				stream = s;
				return ret;
			case '\0':
				throw std::runtime_error("Lacks ending quation before the the end of string");
			default:
				s.take(); // normal character
			}
		}

		return ret; // unreachable
	}

	//! Parse any JSON value
	generic_value<Encoding> parseValue(Stream& stream, ParseFlag flag)
	{
		switch (stream.peek()) {
		case 'n': return parseNull  (stream, flag);
		case 't': return parseTrue  (stream, flag);
		case 'f': return parseFalse (stream, flag);
		case '"': return parseString(stream, flag);
		case '{': return parseObject(stream, flag);
		case '[': return parseArray (stream, flag);
		//default:  return parseNumber(stream, flag);
		}

		return parseNumber(stream, flag);
	}
};

//! Reader with UTF8 encoding.
typedef generic_reader<generic_stringstream<UTF8<> >, UTF8<> > Reader;

}