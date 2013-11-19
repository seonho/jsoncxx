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

namespace jsoncxx
{

//! Represents a JSON value. Use Value for UTF8 encoding.
/*!
	A JSON value can be one of types. This class is a variant type supporting these types.

	Use the Value if UTF8
	\tparam Encoding Encoding of the value. (Even non-string values need to have the same encoding in a document)
*/
#pragma pack (push, 4)
template <typename Encoding>
class generic_value
{
public:
	typedef Encoding encoding_type;		//!< Encoding type from the template parameter.
	typedef typename Encoding::char_type char_type;	//!< Character type derived from Encoding.
	typedef std::basic_string<typename Encoding::char_type> generic_string;

	typedef generic_value<Encoding>	self_type;

	struct number
	{
		union numeric_holder
		{
			natural	n;
			real	r;
		} num_;

		NumericType type_;
	};

	struct string
	{
		generic_string*	str_;
		size_type		hash_;
	};

	struct array
	{
		typedef generic_value<Encoding>	element_type;	//!< itself
		typedef std::list<element_type>	storage_type;	//!< storage type

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
			return elements_.empty();
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

	struct object
	{
		typedef generic_value<Encoding> key_type;	//!< string type value
		typedef generic_value<Encoding>	value_type;	//!< any value

		struct hash_from_member
			: public std::unary_function<jsoncxx::generic_value<Encoding>, size_t>
		{
			size_t operator()(const typename jsoncxx::generic_value<Encoding>& _Keyval) const
			{
				if(_Keyval.type() != jsoncxx::StringType)
					throw std::runtime_error("Non-String type Value cannot have hash value!");

				return _Keyval.value_.s.hash_;
			}
		};
		
		typedef std::unordered_map<key_type, value_type, hash_from_member> storage_type;
		typedef typename storage_type::value_type		member_type;
				
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

		value_type& operator [] (const generic_string& key)
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

		const value_type& operator [] (const generic_string& key) const
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
			key.value_.s.hash_ = std::hash<generic_string>()(*key.value_.s.str_);

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
	union value_holder
	{
		string	s;
		number	n;
		object	o;
		array	a;
	};

public:
	static self_type& null()
	{
		static self_type nullobj;
		return nullobj;
	}

	//! default ctor creates a null value.
	generic_value() : type_(NullType) {}

	//! move ctor.
	generic_value(generic_value&& rhs)
		: type_(rhs.type_)
	{
		std::swap(value_, rhs.value_);

		rhs.type_ = NullType;
	}

	//! copy ctor
	explicit generic_value(const generic_value& rhs)
		: type_(rhs.type_)
	{
		switch (type_) {
		case ObjectType:
			value_.o.members_ = new object::storage_type;
			// copy internal memory as well
			value_.o.members_->insert(rhs.value_.o.begin(), rhs.value_.o.end());
			break;
		case ArrayType:
			value_.a.elements_ = new array::storage_type;
			// copy internal memory as well
			value_.a.elements_->assign(rhs.value_.a.begin(), rhs.value_.a.end());
			break;
		case StringType:
			value_.s.str_ = new generic_string(*(rhs.value_.s.str_));
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
	generic_value(T value, typename std::enable_if<std::is_arithmetic<T>::value, T>::type *p = nullptr)
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
		
	generic_value(bool value)
		: type_(value ? TrueType : FalseType)
	{
	}

	generic_value(const char_type* value)
		: type_(StringType)
	{
		// copy string
		value_.s.str_ = new generic_string(value);
		//value_.s.hash_ = std::hash<generic_string>()(*value_.s.str_);
	}

	//! ctor for generic_string
	generic_value(const generic_string& value)
		: type_(StringType)
	{
		//value_.s.str_ = ;
		//std::swap(*(value_.s.str_), value);
		value_.s.str_ = new generic_string(value);
		//value_.s.hash_ = std::hash<generic_string>()(value);
	}
	
	//! ctor
	generic_value(const char_type* begin, const char_type* end)
		: type_(StringType)
	{
		value_.s.str_ = new std::string(begin, end);
	}

	//! Default dtor.
	~generic_value()
	{
		clear();
	}

	//! assignment with primitive types
	template <typename T>
	generic_value& operator= (T value)
	{
		this->~generic_value();
		new (this) generic_value(value);
		return *this;
	}

	generic_value& operator= (const generic_value& rhs)
	{
		this->~generic_value();
		new (this) generic_value(rhs); // reuse constructor
		return *this;
	}

private:
	//! 
	static typename object::key_type make_key(const generic_string& key)
	{
		typedef typename object::key_type key_type;
		key_type _key(key);
		_key.value_.s.hash_ = std::hash<generic_string>()(key);
		return _key;
	}

public:
	//! ctor with JSON value type.
	generic_value(ValueType type): type_(type)
	{
		memset(&value_, 0, sizeof(value_holder));

		switch (type_) {
		case ObjectType:
			value_.o.members_ = new object::storage_type;
			break;
		case ArrayType:
			value_.a.elements_ = new array::storage_type;
			break;
		case StringType:
			value_.s.str_ = new generic_string();
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
			}

			memset(&value_, 0, sizeof(value_holder));
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

	//! 
	inline self_type& append(self_type&& value)
	{
		JSONCXX_ASSERT(type_ == NullType || type_ == ArrayType);
		
		if (type_ == NullType)
			*this = std::move(self_type(ArrayType));

		value_.a.elements_->push_back(std::move(value));
		return value_.a.elements_->back();
	}

	//! 
	inline self_type& append(const self_type& value)
	{
		JSONCXX_ASSERT(type_ == NullType || type_ == ArrayType);
		if (type_ == NullType)
			*this = std::move(self_type(ArrayType));

		value_.a.elements_->push_back(value); // copy...
		return value_.a.elements_->back();
	}

	//! 
	inline self_type& operator [] (const size_type index)
	{
		JSONCXX_ASSERT(type_ == ArrayType);
		return value_.a[index];
	}

	//! 
	inline const self_type& operator [] (const size_type index) const
	{
		JSONCXX_ASSERT(type_ == ArrayType);
		return value_.a[index];
	}

	inline self_type& operator [] (const generic_string& key)
	{
		JSONCXX_ASSERT(type_ == NullType || type_ == ObjectType);

		if (type_ == NullType)
			*this = std::move(self_type(ObjectType));

		return (value_.o)[key];
	}

	inline const self_type& operator [] (const generic_string& key) const
	{
		JSONCXX_ASSERT(type_ == NullType || type_ == ObjectType);

		if (type_ == NullType)
			return self_type::null();

		return (value_.o)[key];
	}

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

	bool asBool() const
	{
		JSONCXX_ASSERT(type_ == TrueType || type_ == FalseType);
		return (type_ == TrueType);
	}

	//! 
	const generic_string& asString() const
	{
		JSONCXX_ASSERT(type_ == StringType);
		return *(value_.s.str_);
	}

	//!
	natural asNatural() const
	{
		JSONCXX_ASSERT(type_ == NumberType || value_.n.type_ == NaturalNumber);
		return value_.n.num_.n;
	}

	//!
	real asReal() const
	{
		JSONCXX_ASSERT(type_ == NumberType || value_.n.type_ == RealNumber);
		return value_.n.num_.r;
	}

	const number& asNumber() const
	{
		JSONCXX_ASSERT(type_ == NumberType);
		return value_.n;
	}

	const array& asArray() const
	{
		JSONCXX_ASSERT(type_ == ArrayType);
		return value_.a;
	}

	const object& asObject() const
	{
		JSONCXX_ASSERT(type_ == ObjectType);
		return value_.o;
	}

	friend std::basic_ostream<char_type, std::char_traits<char_type> >& operator << (std::basic_ostream<char_type, std::char_traits<char_type> >& stream, const generic_value& value)
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

				const object& o = value.asObject();

				// lambda function variable
				auto func = [&](const typename object::member_type& p) -> generic_string {
					std::ostringstream oss;
					oss << p.first
						<< " : " 
						<< p.second;
					return oss.str();
				};

				std::transform(o.begin(), std::next(o.begin(), value.size() - 1), 
								std::ostream_iterator<generic_string>(stream, ", "), 
								func);
				stream << func(*std::next(o.begin(), value.size() - 1));

				stream << '}'; // Object end
			}
			break;
		case ArrayType:
			{
				stream << '['; // Array begin
				
				const array& a = value.asArray();
				std::copy(a.begin(), std::next(a.begin(), value.size() - 1), std::ostream_iterator<const generic_value<Encoding>&>(stream, ", "));
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
	value_holder	value_;
	ValueType		type_;
};
#pragma pack (pop)

typedef generic_value<UTF8<> > Value;

}

namespace std {

//! template specialization of std::hash for #generic_value type
template <typename Encoding>
struct hash<jsoncxx::generic_value<Encoding> >
	: public unary_function<jsoncxx::generic_value<Encoding>, size_t>
{
	typedef std::basic_string<typename Encoding::char_type> generic_string;

	size_t operator()(const typename jsoncxx::generic_value<Encoding>& _Keyval) const
	{
		if(_Keyval.type() != jsoncxx::StringType)
			throw std::runtime_error("Non-String type Value cannot have hash value!");

		return hash<generic_string>()(_Keyval.asString());
	}
};

//! template specialization of std::equal_to for #generic_value type
template <typename Encoding>
struct equal_to<jsoncxx::generic_value<Encoding> >
	: public binary_function<jsoncxx::generic_value<Encoding>, jsoncxx::generic_value<Encoding>, bool>
{
	bool operator() (const typename jsoncxx::generic_value<Encoding>& lhs, 
					 const typename jsoncxx::generic_value<Encoding>& rhs) const {
		return lhs.asString().compare(rhs.asString()) == 0;
	}
};

}