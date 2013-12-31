/**
 *	@file		reader.hpp
 *	@author		seonho.oh@gmail.com
 *	@date		2013-11-01
 *	@copyright	2007-2013 seonho.oh@gmail.com, Copyright (c) 2011 Milo Yip (miloyip@gmail.com)
 *	@version	1.0
 */

#pragma once

#include "encoding.hpp"
#include "value.hpp"

#include <stdexcept>    // runtime_error
#include <sstream>      // stringstream
#include <string>       // basic_stream
#include <fstream>      // basic_ifstream

namespace jsoncxx {
    
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
template<> inline void SkipWhitespace(insitustringstream& stream) { 
    stream.src_ = const_cast<char*>(SkipWhitespace_SIMD(stream.src_));
}

//! Template function specialization for stringstream
template<> inline void SkipWhitespace(stringstream& stream) {
    stream.src_ = SkipWhitespace_SIMD(stream.src_);
}
#endif // JSONCXX_SIMD
    
//!	defines parsing error exception
class parsing_error
    : public std::runtime_error
{
public:
    typedef std::runtime_error _Mybase;
    
    explicit parsing_error(const std::string& _Message, const std::string& _File, size_t _Line, const std::string& _Func)
    : _Mybase(_Message)
    {
        std::ostringstream oss;
        oss << "Parsing error at " << _Func << std::endl;
        oss << _File << "(" << _Line << "): " << _Message;
        msg_ = oss.str();
    }
    
    explicit parsing_error(const char *_Message, const char *_File, size_t _Line, char *_Func)
    : _Mybase(_Message)
    {
        std::ostringstream oss;
        oss << "Parsing error at " << _Func << std::endl;
        oss << _File << "(" << _Line << "): " << _Message;
        msg_ = oss.str();
    }
    
    ~parsing_error() throw() {}
    
    const char* what() const throw() { return msg_.c_str(); }
    
private:
    std::string msg_;
};

#define JSONCXX_PARSING_ERROR(msg) throw parsing_error(msg, __FILE__, __LINE__, __FUNCTION__)

//!	Generic reader class
template <typename Stream, typename Encoding = UTF8<> >
class Reader
{
public:
    typedef typename Encoding::char_type	char_type;
    typedef std::basic_string<char_type>	string;
    typedef std::basic_ifstream<char_type>	ifstream;
    typedef StringStream<Encoding>          stringstream;
    typedef Value<Encoding>                 value_type;
    typedef Value<Encoding>                 key_type;
    
    bool parse(const string& filename, value_type& root)
	{
        ifstream fin(filename);
        if (fin.is_open()) {
            
            string json;
            std::getline(fin, json, (char_type)EOF);
            fin.close(); // preliminary implementation read entire file at once.
            
            stringstream s(json.c_str());
            root = parse(s);
            
            return true;
        }
        return false;
    }
    
    value_type parse(Stream& s)
    {
        SkipWhitespace(s);
        
        switch (s.peek()) {
            case 'n': return parseNull  (s);
            case 't': return parseTrue  (s);
            case 'f': return parseFalse (s);
            case '"': return parseString(s);
            case '{': return parseObject(s);
            case '[': return parseArray (s);
		}
        
		return parseNumber(s);
    }
    
private:
    //! @brief  Internal handlers for each of the value types.
    //! @{
    
    //!	@brief	Parse object from stream
	//!			object:{name:value, ...}
	value_type parseObject(Stream& s)
	{
        assert(s.peek() == '{');
        
		value_type ret(ObjectType);
        s.take(); // skip '{'
		SkipWhitespace(s);
        
		if (s.peek() == '}') { // empty object
			s.take();
			return ret;
		}
        
        while(true) {
			if (s.peek() != '"')
				JSONCXX_PARSING_ERROR("Name of an object member must be a string"); // stream.tell();
            
            key_type key = parseString(s);
            
			SkipWhitespace(s);
            
			if (s.take() != ':')
				JSONCXX_PARSING_ERROR("There must be a colon after the name of object member"); // stream.tell();
            
			SkipWhitespace(s);
            
			ret.insert(std::move(key), parse(s));
            
			SkipWhitespace(s);
            
			switch (s.take()) {
                case ',': SkipWhitespace(s); break;
                case '}': return ret;
                default: JSONCXX_PARSING_ERROR("Must be a comma or '}' after an object member"); // stream.tell();
			}
		}
        
        return ret;
    }
    
    //!	@brief	Parse array from stream
	//!			array: [ value, ... ]
	value_type parseArray(Stream& s)
    {
        assert(s.peek() == '[');
		s.take(); // skip '['
		SkipWhitespace(s);
        
        value_type ret(ArrayType);
        
		if (s.peek() == ']') {
			s.take();
			return ret;
		}
        
		while (true) {
			ret.append(parse(s));
            
			SkipWhitespace(s);
            
			switch (s.take()) {
                case ',': SkipWhitespace(s); break;
                case ']': return ret;
                default: JSONCXX_PARSING_ERROR("Must be a comma or ']' after an array member"); // stream.tell();
			}
		}
        
		return ret;
    }
    
    //!	Parse null value from stream
	value_type parseNull(Stream& s)
    {
        JSONCXX_ASSERT(s.peek() == 'n');
        s.take();
        
        if (s.take() == 'u' &&
			s.take() == 'l' &&
			s.take() == 'l')
			return value_type(); // null
		else
			JSONCXX_PARSING_ERROR("Invalid value"); // stream.tell() - 1;
    }
    
    //!	Parse true value from stream
	value_type parseTrue(Stream& s)
	{
		JSONCXX_ASSERT(s.peek() == 't');
		s.take();
        
		if (s.take() == 'r' &&
			s.take() == 'u' &&
			s.take() == 'e')
			return value_type(TrueType); // true
		else
			JSONCXX_PARSING_ERROR("Invalid value"); // stream.tell() - 1;
	}
    
	//! Parse false value from stream
	value_type parseFalse(Stream& s)
	{
		JSONCXX_ASSERT(s.peek() == 'f');
		s.take();
        
		if (s.take() == 'a' &&
			s.take() == 'l' &&
			s.take() == 's' &&
			s.take() == 'e')
			return value_type(FalseType); // false
		else
			JSONCXX_PARSING_ERROR("Invalid value"); // stream.tell() - 1;
	}
    
    //! Parse number value from stream
	value_type parseNumber(Stream& s)
	{
		Stream s_ = s; // Local copy for optimization
		
		// parse number
		while ((s_.peek() >= '0' && s_.peek() <= '9') ||
               s_.peek() == '.' ||
               s_.peek() == 'e' || s_.peek() == 'E' ||
               s_.peek() == '-' || s_.peek() == '+')
            s_.take();
        
		value_type ret;
        
		std::string number(s.src_, s_.src_);
        
		if (number.find_first_of('.') == std::string::npos)
			ret = std::move(value_type((natural)std::stoll(number)));
		else
			ret = std::move(value_type((real)std::stod(number)));
        
		s = s_;
        
		return ret;
	}
    
	//!	@brief	Parse string value from stream
	//!	@note	This implementation do not support "u" literal, 4 hexadecimal digits
	value_type parseString(Stream& s)
	{
        JSONCXX_ASSERT(s.peek() == '\"');
        s.take(); // skip '\"'
        
		Stream s_ = s;
		
        value_type ret;
        
		while (true) {
			switch (s_.peek()) {
                case '\"':
                    ret = value_type(s.src_, s_.src_);
                    s_.take();
                    s = s_;
                    return ret;
                case '\0': JSONCXX_PARSING_ERROR("Lacks ending quation before the the end of string");
                case '\\': JSONCXX_PARSING_ERROR("Currently not supported!");
                default: s_.take(); // normal character
			}
		}
        
		return ret; // unreachable
	}
    
    //! @}
};

}