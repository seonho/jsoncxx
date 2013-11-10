#pragma once

#include "encoding.hpp"

namespace jsoncxx {

///////////////////////////////////////////////////////////////////////////////
//  Stream

/*! \class jsoncxx::stream
	\brief Concept for reading and writing characters.

	For read-only stream, no need to implement PutBegin(), Put() and PutEnd().

	For write-only stream, only need to implement Put().

\code
concept Stream {
	typename char_type;	//!< Character type of the stream.

	//! Read the current character from stream without moving the read cursor.
	char_type Peek() const;

	//! Read the current character from stream and moving the read cursor to next character.
	char_type Take();

	//! Get the current read cursor.
	//! \return Number of characters read from start.
	size_t Tell();

	//! Begin writing operation at the current read pointer.
	//! \return The begin writer pointer.
	char_type* PutBegin();

	//! Write a character.
	void Put(char_type c);

	//! End the writing operation.
	//! \param begin The begin write pointer returned by PutBegin().
	//! \return Number of characters written.
	size_t PutEnd(char_type* begin);
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

//! Read-only string stream.
/*! \implements Stream
*/
template <typename Encoding>
struct generic_stringstream {
	typedef typename Encoding::char_type char_type;

	generic_stringstream(const char_type *src) : src_(src), head_(src) {}

	char_type peek() const { return *src_; }
	char_type take() { return *src_++; }
	size_t tell() const { return src_ - head_; }

	char_type* begin() { JSONCXX_ASSERT(false); return 0; }
	void put(char_type) { JSONCXX_ASSERT(false); }
	size_t end(char_type*) { JSONCXX_ASSERT(false); return 0; }

	const char_type* src_;		//!< Current read position.
	const char_type* head_;	//!< Original head of the string.
};

typedef generic_stringstream<UTF8<> > stringstream;

///////////////////////////////////////////////////////////////////////////////
// InsituStringStream

//! A read-write string stream.
/*! This string stream is particularly designed for in-situ parsing.
	\implements Stream
*/
template <typename Encoding>
struct generic_inplace_stringstream {
	typedef typename Encoding::char_type char_type;

	generic_inplace_stringstream(char_type *src) : src_(src), dst_(0), head_(src) {}

	// Read
	char_type peek() { return *src_; }
	char_type take() { return *src_++; }
	size_t tell() { return src_ - head_; }

	// Write
	char_type* begin() { return dst_ = src_; }
	void put(char_type c) { JSONCXX_ASSERT(dst_ != 0); *dst_++ = c; }
	size_t end(char_type* begin) { return dst_ - begin; }

	char_type* src_;
	char_type* dst_;
	char_type* head_;
};

typedef generic_inplace_stringstream<UTF8<> > inplace_stringstream;

}