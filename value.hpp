/**
 *	@file		value.hpp
 *	@author		seonho.oh@gmail.com
 *	@date		2013-11-01
 *	@copyright	2007-2013 seonho.oh@gmail.com
 *	@version	1.0
 */

#pragma once

#include "jsoncxx.hpp"

#include <list>
#include <string>
#include <unordered_map>
#include <algorithm>	// for_each
#include <functional>	// mem_fun
#include <codecvt>		// code conversion

namespace jsoncxx
{

//! Represents a JSON value. Use value for UTF8 encoding.
/*!
	A JSON value can be one of types. This class is a variant type supporting these types.

	Use the Value if UTF8
	\tparam Encoding Encoding of the value. (Even non-string values need to have the same encoding in a document)
*/
#pragma pack (push, 4)
template <typename Encoding>
class Value
{
public:
	typedef Encoding						encoding_type;		//!< Encoding type from the template parameter.
	typedef typename Encoding::char_type	char_type;	//!< Character type derived from Encoding.
	typedef std::basic_string<char_type>	string;
	typedef Value<Encoding>					self_type;
	typedef std::basic_ostream<char_type, std::char_traits<char_type> >	ostream;

	//!	Represents a number type value.
	struct Number
	{
		union NumberHolder
		{
			natural	n;
			real	r;
		} num_;

		NumericType type_;
	};

	//!	Represents a string type value.
	struct String
	{
		string*		str_;
		size_t      hash_;
	};

	//!	Represents an array type value.
	struct Array
	{
		typedef Value<Encoding>         element_type;	//!< itself
		typedef std::list<element_type> storage_type;	//!< storage type

		storage_type* elements_;

		inline void clear()
		{
			//std::for_each(elements_->begin(), elements_->end(), std::mem_fun_ref<void, element_type>(&element_type::clear));
			elements_->clear();
		}

		inline size_type size() const
		{
			return elements_->size();
		}

		inline bool empty() const
		{
			return elements_->empty();
		}

		element_type& operator [] (size_type index)
		{
			if (index < size())
				return *std::next(elements_->begin(), index);
			else
				throw std::runtime_error("Out of range");

			return self_type::null(); // unreachable
		}

		const element_type& operator [] (size_type index) const
		{
			if (index < size())
				return *std::next(elements_->begin(), index);
			else
				throw std::runtime_error("Out of range");

			return self_type::null();
		}

		typedef typename storage_type::iterator			iterator;
		inline iterator begin()	{ return elements_->begin(); }
		inline iterator end()	{ return elements_->end(); }

		typedef typename storage_type::const_iterator	const_iterator;
		inline const_iterator begin() const	{ return elements_->begin(); }
		inline const_iterator end() const	{ return elements_->end(); }

		const element_type& back() const { return elements_->back(); }
	};

	//!	Represents an object type value.
	struct Object
	{
		typedef Value<Encoding>	key_type;	//!< string type value
		typedef Value<Encoding>	value_type;	//!< any value

		struct hash_from_member
			: public std::unary_function<Value<Encoding>, size_t>
		{
			size_t operator()(const Value<Encoding>& _Keyval) const
			{
				if(_Keyval.type() != StringType)
					throw std::runtime_error("Non-String type Value cannot have hash value!");

				return _Keyval.value_.s.hash_;
			}
		};
		
		typedef std::unordered_map<key_type, value_type, hash_from_member>	storage_type;
		typedef typename storage_type::value_type							member_type;
				
		storage_type* members_;

		inline void clear()
		{
			members_->clear();
		}

		inline size_type size() const
		{
			return members_->size();
		}

		inline bool empty() const
		{
			return members_->empty();
		}

		value_type& operator [] (const string& key)
		{
			key_type _key = make_key(key);
			auto itr = members_->find(_key);
			if (itr != members_->end()) {
				_key.clear();
				return (*itr).second;
			}

			// R-value reference
			auto bi = members_->emplace(
				std::move(std::make_pair(std::move(_key), 
									     std::move(value_type()) )
										 )
									);
			assert(bi.second); // is it possible?

			return ((*(bi.first)).second);
		}

		const value_type& operator [] (const string& key) const
		{
			key_type _key = make_key(key);
			auto itr = members_->find(_key);

			if (itr == members_->end())
				return self_type::null(); // null object
			return (*itr).second;
		}

		value_type& operator [] (key_type& key)
		{
			JSONCXX_ASSERT(key.type() == StringType);

			// make hash key
			key.value_.s.hash_ = std::hash<string>()(*key.value_.s.str_);

			auto itr = members_->find(key);
			if (itr != members_->end())
				return (*itr).second;

			// R-value reference
			auto bi = members_->emplace(
				std::move(std::make_pair(std::move(key), 
									     std::move(value_type()) )
										 )
									);
			assert(bi.second); // is it possible?

			return ((*(bi.first)).second);
		}

		typedef typename storage_type::iterator			iterator;
		inline iterator begin()	{ return members_->begin(); }
		inline iterator end()	{ return members_->end(); }

		typedef typename storage_type::const_iterator	const_iterator;

		inline const_iterator begin() const	{ return members_->begin(); }
		inline const_iterator end() const	{ return members_->end(); }
	};

protected:
	//!	Internal type union structure
	union ValueHolder
	{
		String	s;
		Number	n;
		Object	o;
		Array	a;
	};

public:
	static self_type& null()
	{
		static self_type nullobj;
		return nullobj;
	}

	//! default ctor creates a null value.
	Value() : type_(NullType) {}

	//! move ctor.
	Value(Value&& rhs)
		: type_(rhs.type_)
	{
		std::swap(value_, rhs.value_);

		rhs.type_ = NullType;
	}

	//! copy ctor
	explicit Value(const Value& rhs)
		: type_(rhs.type_)
	{
		switch (type_) {
		case ObjectType:
			value_.o.members_ = new typename Object::storage_type;
			// copy internal memory as well
			value_.o.members_->insert(rhs.value_.o.begin(), rhs.value_.o.end());
			break;
		case ArrayType:
			value_.a.elements_ = new typename Array::storage_type;
			// copy internal memory as well
			value_.a.elements_->assign(rhs.value_.a.begin(), rhs.value_.a.end());
			break;
		case StringType:
			value_.s.str_ = new string(*(rhs.value_.s.str_));
			value_.s.hash_ = rhs.value_.s.hash_;
			break;
		case NumberType:
			value_.n = rhs.value_.n;
			break;
		default:
			; // do nothing
		}
	}

	//! ctor for numeric types
	template <typename T>
	Value(T value, typename std::enable_if<std::is_arithmetic<T>::value, T>::type *p = nullptr)
		: type_(NumberType)
	{
		if (std::is_floating_point<T>::value) {
			value_.n.type_ = RealNumber;
			if (std::is_same<T, real>::value)
				value_.n.num_.r = (real)value; // to suppress compiler warning (this is no op)
			else
				value_.n.num_.r = static_cast<real>(value);
		} else {
			value_.n.type_ = NaturalNumber;
			if (std::is_same<T, natural>::value)
				value_.n.num_.n = (natural)value; // to suppress compiler warning (this is no op)
			else
				value_.n.num_.n = static_cast<natural>(value);
		}
	}
		
	Value(bool value)
		: type_(value ? TrueType : FalseType)
	{
	}

	Value(const char_type* value)
		: type_(StringType)
	{
		// copy string
		value_.s.str_ = new string(value);
		//value_.s.hash_ = std::hash<string>()(*value_.s.str_);
	}

	//! ctor for string
	Value(const string& value)
		: type_(StringType)
	{
		//value_.s.str_ = ;
		//std::swap(*(value_.s.str_), value);
		value_.s.str_ = new string(value);
		//value_.s.hash_ = std::hash<string>()(value);
	}
	
	//! ctor
	Value(const char_type* begin, const char_type* end)
		: type_(StringType)
	{
		value_.s.str_ = new string(begin, end);
	}

	//! Default dtor.
	~Value()
	{
		clear();
	}

	//! assignment with primitive types
	template <typename T>
	Value& operator= (T value)
	{
		this->~Value();
		new (this) Value(value);
		return *this;
	}

	Value& operator= (const Value& rhs)
	{
		this->~Value();
		new (this) Value(rhs); // reuse constructor
		return *this;
	}

private:
	//! 
	static typename Object::key_type make_key(const string& key)
	{
		typedef typename Object::key_type key_type;
		key_type _key(key);
		_key.value_.s.hash_ = std::hash<string>()(key);
		return _key;
	}

public:
	//! ctor with JSON value type.
	Value(ValueType type): type_(type)
	{
		memset(&value_, 0, sizeof(ValueHolder));

		switch (type_) {
		case ObjectType:
			value_.o.members_ = new typename Object::storage_type;
			break;
		case ArrayType:
			value_.a.elements_ = new typename Array::storage_type;
			break;
		case StringType:
			value_.s.str_ = new string();
			break;
		default:
			; // do nothing
		}
	}

	//! 
	void clear()
	{
		if (type_ == ArrayType || type_ == ObjectType || type_ == StringType) {

			switch (type_) {
			case ArrayType:
				if (value_.a.elements_) {
					value_.a.clear();
					delete value_.a.elements_;
				}
				break;
			case ObjectType:
				if (value_.o.members_) {
					value_.o.clear();
					delete value_.o.members_;
				}
				break;
			case StringType:
				if (value_.s.str_)
					delete value_.s.str_;
				break;
            default:
                ;
			}

			memset(&value_, 0, sizeof(ValueHolder));
			type_ = NullType;
		}
	}
	
	//! 
	inline ValueType type() const
	{
		return type_;
	}

	//! 
	inline size_type size() const
	{
		JSONCXX_ASSERT(type_ == NullType || type_ == ArrayType || type_ == ObjectType);
		switch (type_) {
		case NullType:
		case FalseType:
		case TrueType:
		case NumberType:
		case StringType:
			return 0;
		case ArrayType:
			return value_.a.size();
		case ObjectType:
			return value_.o.size();
		}

		return 0; // unreachable
	}

	//! Append a value if current value type is array.
	inline self_type& append(self_type&& value)
	{
		JSONCXX_ASSERT(type_ == NullType || type_ == ArrayType);
		
		if (type_ == NullType)
			*this = std::move(self_type(ArrayType));

		value_.a.elements_->push_back(std::move(value));
		return value_.a.elements_->back();
	}

	//! Append a value if current internal type is array.
	inline self_type& append(const self_type& value)
	{
		JSONCXX_ASSERT(type_ == NullType || type_ == ArrayType);
		if (type_ == NullType)
			*this = std::move(self_type(ArrayType));

		value_.a.elements_->push_back(value); // copy...
		return value_.a.elements_->back();
	}

	//! Access array element by index.
	inline self_type& operator [] (const size_type index)
	{
		JSONCXX_ASSERT(type_ == ArrayType);
		return value_.a[index];
	}

	//! Access array element by index.
	inline const self_type& operator [] (const size_type index) const
	{
		JSONCXX_ASSERT(type_ == ArrayType);
		return value_.a[index];
	}

	//!	Access object element by key.
	inline self_type& operator [] (const string& key)
	{
		JSONCXX_ASSERT(type_ == NullType || type_ == ObjectType);

		if (type_ == NullType)
			*this = std::move(self_type(ObjectType));

		return (value_.o)[key];
	}

	//!	Access object element by key.
	inline const self_type& operator [] (const string& key) const
	{
		JSONCXX_ASSERT(type_ == NullType || type_ == ObjectType);

		if (type_ == NullType)
			return self_type::null();

		return (value_.o)[key];
	}

	//!	Insert new key-value pair if internal type is object.
	inline void insert(self_type&& key, self_type&& value)
	{
		(value_.o)[key] = value;
	}
	
	//! move assignment operator.
	inline self_type& operator= (self_type&& rhs)
	{
		if (this != &rhs) {
			clear();

			std::swap(type_, rhs.type_);

			switch (type_) {
			case ObjectType:
			case ArrayType:
			case StringType:
			case NumberType:
				std::swap(value_, rhs.value_);
				break;
			default:
				; // do nothing
			}
		}

		return *this;
	}

	//! get boolean value
	bool asBool() const
	{
		JSONCXX_ASSERT(type_ == TrueType || type_ == FalseType);
		return (type_ == TrueType);
	}

	//! get string value
	const string& asString() const
	{
		JSONCXX_ASSERT(type_ == StringType);
		return *(value_.s.str_);
	}

	//! get natural value
	natural asNatural() const
	{
		JSONCXX_ASSERT(type_ == NumberType);
		if (value_.n.type_ == NaturalNumber)
			return value_.n.num_.n;
		return static_cast<natural>(value_.n.num_.r);
	}

	//! get real value
	real asReal() const
	{
		JSONCXX_ASSERT(type_ == NumberType);
		if (value_.n.type_ == RealNumber)
			return value_.n.num_.r;
		return static_cast<real>(value_.n.num_.n);
	}

	//! get number
	const Number& asNumber() const
	{
		JSONCXX_ASSERT(type_ == NumberType);
		return value_.n;
	}

	//! get array
	const Array& asArray() const
	{
		JSONCXX_ASSERT(type_ == ArrayType);
		return value_.a;
	}

	//! get object
	const Object& asObject() const
	{
		JSONCXX_ASSERT(type_ == ObjectType);
		return value_.o;
	}
	//! Preliminary implement stream operator
	friend ostream& operator << (ostream& stream, const Value& value)
	{
		switch (value.type()) {
		case NullType:
			stream << "null";
			break;
		case FalseType:
			stream << "false";
			break;
		case TrueType:
			stream << "true";
			break;
		case ObjectType:
			{
				stream << '{'; // Object begin

				const Object& o = value.asObject();

				// lambda function variable
				auto func = [&](const typename Object::member_type& p) -> string {
					std::basic_ostringstream<char_type> oss;
					oss << p.first
						<< " : " 
						<< p.second;
					return oss.str();
				};

				std::transform(o.begin(), std::next(o.begin(), value.size() - 1), 
								std::ostream_iterator<string, char_type>(stream, ", "), 
								func);
				stream << func(*std::next(o.begin(), value.size() - 1));

				stream << '}'; // Object end
			}
			break;
		case ArrayType:
			{
				stream << '['; // Array begin

				// get delimiter
				
				const Array& a = value.asArray();
				std::copy(a.begin(), std::next(a.begin(), value.size() - 1), std::ostream_iterator<const Value<Encoding>&>(stream, ", "));
				stream << a.back();

				stream << ']'; // Array end
			}
			break;
		case StringType:
			stream << '\"' 
				<< value.asString() 
				<< '\"';
			break;
		case NumberType:
			if (value.asNumber().type_ == NaturalNumber)
				stream << value.asNatural();
			else
				stream << value.asReal();
			break;
		}

		return stream;
	}

private:
	ValueHolder	value_;
	ValueType		type_;
};
#pragma pack (pop)

//!	Define value for UTF8 encoding
typedef Value<UTF8<> > value;

}

namespace std {

//! template specialization of std::hash for Value type
template <typename Encoding>
struct hash<jsoncxx::Value<Encoding> >
	: public unary_function<jsoncxx::Value<Encoding>, size_t>
{
	typedef std::basic_string<typename Encoding::char_type> string;

	size_t operator()(const typename jsoncxx::Value<Encoding>& _Keyval) const
	{
		if(_Keyval.type() != jsoncxx::StringType)
			throw std::runtime_error("Non-String type Value cannot have hash value!");

		return hash<string>()(_Keyval.asString());
	}
};

//! template specialization of std::equal_to for Value type
template <typename Encoding>
struct equal_to<jsoncxx::Value<Encoding> >
	: public binary_function<jsoncxx::Value<Encoding>, jsoncxx::Value<Encoding>, bool>
{
	bool operator() (const typename jsoncxx::Value<Encoding>& lhs, 
					 const typename jsoncxx::Value<Encoding>& rhs) const {
		return lhs.asString().compare(rhs.asString()) == 0;
	}
};

}