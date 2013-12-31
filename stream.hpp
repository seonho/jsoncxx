/**
 *	@file		stream.hpp
 *	@author		seonho.oh@gmail.com
 *	@date		2013-11-01
 *	@copyright	2007-2013 seonho.oh@gmail.com, 2011-2012 Milo Yip (miloyip@gmail.com)
 *	@version	1.0
 */

#pragma once

#include "encoding.hpp"

namespace jsoncxx {

///////////////////////////////////////////////////////////////////////////////
//  Stream
//	Modified by Seonho Oh(seonho.oh@gmail.com)
//	Original code by
//		Copyright (c) 2011-2012 Milo Yip (miloyip@gmail.com)
//
/*! \class jsoncxx::stream
    \brief Concept for reading and writing characters.

    For read-only stream, no need to implement begin(), put() and end().

    For write-only stream, only need to implement put().
 
    \code
    concept Stream {
        typename char_type;	//!< Character type of the stream.

        //! Read the current character from stream without moving the read cursor.
        char_type peek() const;

        //! Read the current character from stream and moving the read cursor to next character.
        char_type take();

        //! Get the current read cursor.
        //! \return Number of characters read from start.
        size_t tell();

        //! Begin writing operation at the current read pointer.
        //! \return The begin Writer pointer.
        char_type* begin();

        //! Write a character.
        void Put(char_type c);

        //! End the writing operation.
        //! \param begin The begin write pointer returned by PutBegin().
        //! \return Number of characters written.
        size_t end(char_type* begin);
    }
    \endcode
 */

//! Put N copies of a character to a stream.
template<typename Stream, typename CharType>
inline void putN(Stream& stream, CharType c, size_t n) {
    for (size_t i = 0; i < n; i++)
        stream.put(c);
}

///////////////////////////////////////////////////////////////////////////////
// StringStream
//	Modified by Seonho Oh(seonho.oh@gmail.com)
//	Original code by
//		Copyright (c) 2011-2012 Milo Yip (miloyip@gmail.com)
//
//! Read-only string stream.
template <typename Encoding>
struct StringStream {
    typedef typename Encoding::char_type char_type;
    
    StringStream(const char_type *src) : src_(src), head_(src) {}
    
    inline char_type peek() const { return *src_; }
    inline char_type take() { return *src_++; }
    inline size_t tell() const { return src_ - head_; }
    
    inline char_type* begin() { JSONCXX_ASSERT(false); return 0; }
    inline void put(char_type) { JSONCXX_ASSERT(false); }
    inline size_t end(char_type*) { JSONCXX_ASSERT(false); return 0; }
    
    const char_type* src_;	//!< Current read position.
    const char_type* head_;	//!< Original head of the string.
};

}