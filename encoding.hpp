/**
 *	@file		encoding.hpp
 *	@brief		Implement Encoding types.
 *	@author		seonho.oh@gmail.com
 *	@date		2013-11-01
 *	@copyright	2013-2015 seonho.oh@gmail.com
 *	@version	1.0
 */

#pragma once

namespace jsoncxx
{
    
///////////////////////////////////////////////////////////////////////////////
// Encoding
//	Modified by Seonho Oh(seonho.oh@gmail.com)
//	Original code by
//		Copyright (c) 2011-2012 Milo Yip (miloyip@gmail.com)
//		Version 0.11
//
/*! \class jsoncxx::Encoding
    \brief Concept for encoding of Unicode characters.

    \code
        concept Encoding {
        typename char_type;	//! Type of character.

        //! \brief Encode a Unicode codepoint to a buffer.
        //! \param buffer pointer to destination buffer to store the result. It should have sufficient size of encoding one character.
        //! \param codepoint An unicode codepoint, ranging from 0x0 to 0x10FFFF inclusively.
        //! \returns the pointer to the next character after the encoded data.
        static char_type* Encode(char_type *buffer, unsigned codepoint);
    };
    \endcode
 */

///////////////////////////////////////////////////////////////////////////////
// UTF8

//! UTF-8 encoding.
/*! http://en.wikipedia.org/wiki/UTF-8
    \tparam CharType Type for storing 8-bit UTF-8 data. Default is char.
    \implements Encoding
 */
template<typename CharType = char>
struct UTF8
{
    typedef CharType char_type;
    
    static char_type* Encode(char_type *buffer, char32_t codepoint)
    {
        if (codepoint <= 0x7F)
            *buffer++ = codepoint & 0xFF;
        else if (codepoint <= 0x7FF) {
            *buffer++ = 0xC0 | ((codepoint >> 6) & 0xFF);
            *buffer++ = 0x80 | ((codepoint & 0x3F));
        }
        else if (codepoint <= 0xFFFF) {
            *buffer++ = 0xE0 | ((codepoint >> 12) & 0xFF);
            *buffer++ = 0x80 | ((codepoint >> 6) & 0x3F);
            *buffer++ = 0x80 | (codepoint & 0x3F);
        }
        else {
            JSONCXX_ASSERT(codepoint <= 0x10FFFF);
            *buffer++ = 0xF0 | ((codepoint >> 18) & 0xFF);
            *buffer++ = 0x80 | ((codepoint >> 12) & 0x3F);
            *buffer++ = 0x80 | ((codepoint >> 6) & 0x3F);
            *buffer++ = 0x80 | (codepoint & 0x3F);
        }
        return buffer;
    }
};

///////////////////////////////////////////////////////////////////////////////
// UTF16

//! UTF-16 encoding.
/*! http://en.wikipedia.org/wiki/UTF-16
    \tparam CharType Type for storing 16-bit UTF-16 data. Default is wchar_t. C++11 may use char16_t instead.
    \implements Encoding
 */
template<typename CharType = char16_t>
struct UTF16
{
    typedef CharType char_type;
    
    static char_type* Encode(char_type* buffer, char32_t codepoint)
    {
        if (codepoint <= 0xFFFF) {
            JSONCXX_ASSERT(codepoint < 0xD800 || codepoint > 0xDFFF); // Code point itself cannot be surrogate pair
            *buffer++ = static_cast<char_type>(codepoint);
        }
        else {
            JSONCXX_ASSERT(codepoint <= 0x10FFFF);
            unsigned v = codepoint - 0x10000;
            *buffer++ = static_cast<char_type>((v >> 10) + 0xD800);
            *buffer++ = (v & 0x3FF) + 0xDC00;
        }
        return buffer;
    }
};

///////////////////////////////////////////////////////////////////////////////
// UTF32

//! UTF-32 encoding. 
/*! http://en.wikipedia.org/wiki/UTF-32
    \tparam CharType Type for storing 32-bit UTF-32 data. Default is unsigned. C++11 may use char32_t instead.
    \implements Encoding
 */
template<typename CharType = char32_t>
struct UTF32
{
    typedef CharType char_type;
    
    static char_type *Encode(char_type* buffer, char32_t codepoint)
    {
        JSONCXX_ASSERT(codepoint <= 0x10FFFF);
        *buffer++ = codepoint;
        return buffer;
    }
};
    
}