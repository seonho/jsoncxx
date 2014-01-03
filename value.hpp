/**
 *	@file		value.hpp
 *	@author		seonho.oh@gmail.com
 *	@date		2013-11-01
 *	@copyright	2007-2013 seonho.oh@gmail.com
 *	@version	1.0
 */

#pragma once

#include <string>   // basic_string
#include <ostream>  // basic_ostream
#include <list>
#include <map>
#include <cstring>  // memset
#include <algorithm> // transform
#include <iterator> // ostream_iterator

namespace jsoncxx {
    
//! Types of JSON value
enum ValueType {
    NullType,	//!< null
    FalseType,	//!< false
    TrueType,	//!< true
    ObjectType,	//!< object
    ArrayType,	//!< array
    StringType,	//!< string
    NumberType,	//!< number
};

//! Type define of internal number types
#ifdef _MSC_VER
typedef __int64         natural;
#else
typedef long long       natural;
#endif
typedef double          real;

typedef unsigned int	size_type;
    
//! Types of number
enum NumericType : unsigned int {
    NaturalNumber,
    RealNumber,
};

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
    typedef typename Encoding::char_type	char_type;      //! Character type derived from Encoding.
    typedef Value<Encoding>					self_type;      //!
    typedef std::basic_string<char_type>	string;
    typedef std::basic_ostream<char_type, std::char_traits<char_type> >	ostream;
    
protected:
    //!	Represents a number type value.
	struct Number
	{
		//! Internal union structure for number types
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
        typedef Value<Encoding>                         elem_type;
        typedef std::list<elem_type>                    storage_type;
        typedef typename storage_type::iterator         iterator;
        typedef typename storage_type::const_iterator   const_iterator;
        
        elem_type& operator[] (size_t index)
        {
            if (index < size())
                return *std::next(elements_->begin(), index);
            else
                throw std::runtime_error("Out of range");
            
            return Value<Encoding>::null(); // unreachable
        }
        
        const elem_type& operator[] (size_t index) const
        {
            if (index < size())
                return *std::next(elements_->begin(), index);
            else
                throw std::runtime_error("Out of range");
            
            return Value<Encoding>::null(); // unreachable
        }
        
        inline void clear()                 { elements_->clear(); }
        inline size_type size() const       { return (size_type)elements_->size(); }
        inline bool empty() const           { return elements_->empty(); }
        
        inline iterator begin()             { return elements_->begin(); }
		inline iterator end()               { return elements_->end(); }
        
		inline const_iterator begin() const	{ return elements_->begin(); }
		inline const_iterator end() const	{ return elements_->end(); }
        const elem_type& back() const       { return elements_->back(); }
        
        storage_type*       elements_;
    };
    
    //!	Represents an object type value.
    struct Object
	{
        typedef Value<Encoding>                         key_type;
        typedef Value<Encoding>                         value_type;
        typedef std::map<key_type, value_type>          storage_type;
        typedef typename storage_type::iterator         iterator;
        typedef typename storage_type::const_iterator   const_iterator;
        
        value_type& operator[] (const string& key)
        {
            key_type _key = make_key(key);
			auto itr = members_->find(_key);
			if (itr != members_->end()) {
				_key.clear();
				return itr->second;
			}
            
			// R-value reference
			auto bi = members_->emplace(std::make_pair(std::move(_key), std::move(value_type())));
			assert(bi.second); // is it possible?
            
			return ((*(bi.first)).second);
        }
        
        const value_type& operator[] (const string& key) const
        {
            key_type _key = make_key(key);
			auto itr = members_->find(_key);
			if (itr != members_->end()) {
				_key.clear();
				return itr->second;
			}
            
			return value_type::null();
        }
        
        static key_type make_key(const string& key)
        {
            key_type _key(key);
            return _key;
        }
        
        inline void clear()                 { return members_->clear(); }
        inline size_type size() const       { return (size_type)members_->size(); }
        inline bool empty() const           { return members_->empty(); }
        
        inline iterator begin()             { return members_->begin(); }
		inline iterator end()               { return members_->end(); }
        
		inline const_iterator begin() const	{ return members_->begin(); }
		inline const_iterator end() const	{ return members_->end(); }
        
        storage_type*    members_;
    };
    
    //!	Internal union structure for value types
	union ValueHolder
	{
		String	s;
		Number	n;
		Object	o;
		Array	a;
	};
    
public:
	//! null object.
    static Value& null()
	{
		static Value<Encoding> nullobj;
		return nullobj;
	}
    
	//! default ctor creates a null value.
	Value() : type_(NullType) { }
    
    //! default dtor.
	~Value()
	{
		clear();
	}
    
    //! copy ctor.
	Value(const Value& other)
    : type_(other.type_)
	{
		switch (type_) {
            case ObjectType:
                value_.o.members_ = new typename Object::storage_type;
                // copy internal memory as well
                value_.o.members_->insert(other.value_.o.begin(), other.value_.o.end());
                break;
            case ArrayType:
                value_.a.elements_ = new typename Array::storage_type;
                // copy internal memory as well
                value_.a.elements_->assign(other.value_.a.begin(), other.value_.a.end());
                break;
            case StringType:
                value_.s.str_ = new string(*(other.value_.s.str_));
                value_.s.hash_ = other.value_.s.hash_;
                break;
            case NumberType:
                value_.n = other.value_.n;
                break;
            default:
                ; // do nothing
		}
	}
    
    //! copy assignment operator.
	Value& operator= (const Value& other)
	{
        if (this != &other) {
            clear();
            new (this) Value(other); // reuse constructor
        }
		return *this;
	}
    
    //! move ctor.
	Value(Value&& other)
    : type_(NullType)
	{
		*this = std::move(other); // delegate to move assignemnt
	}
    
    //! move assignemnt operator
    Value& operator= (Value&& other)
    {
        if (this != &other) {
            clear();
            type_ = other.type_;
            std::swap(value_, other.value_);
            other.type_ = NullType;
        }
        return *this;
    }
    
    //! ctor with ValueType.
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
    
    //! ctor for numeric types.
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
    
    //! ctor for boolean type.
	Value(bool value)
        : type_(value ? TrueType : FalseType)
	{
	}
    
    //! ctor for character pointer type.
	Value(const char_type* value)
        : type_(StringType)
	{
		value_.s.str_ = new string(value);
        value_.s.hash_ = std::hash<string>()(*value_.s.str_);
	}
    
	//! ctor for string type.
	Value(const string& value)
        : type_(StringType)
	{
		value_.s.str_ = new string(value);
        value_.s.hash_ = std::hash<string>()(*value_.s.str_);
	}
	
	//! ctor for string type.
	Value(const char_type* begin, const char_type* end)
        : type_(StringType)
	{
		value_.s.str_ = new string(begin, end);
        value_.s.hash_ = std::hash<string>()(*value_.s.str_);
	}
    
    //! @name	STL style functions.
    //! @{
    
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
    
    //! get size of value for array and object type
	inline size_type size() const
	{
		JSONCXX_ASSERT(type_ == NullType || type_ == ArrayType || type_ == ObjectType);
        
        size_type size_;
		switch (type_) {
            case ArrayType:
                size_ = (size_type)value_.a.size();
                break;
            case ObjectType:
                size_ = (size_type)value_.o.size();
                break;
            default:
                size_ = 0;
		}
        
		return size_;
	}
    
    //! @}
    
    //! @name	Property functions.
    //! @{
    
    //! get type of value
	inline ValueType type() const       { return type_; }
    
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
    
	//! get number type holder
	const Number& asNumber() const
	{
		JSONCXX_ASSERT(type_ == NumberType);
		return value_.n;
	}
    
	//! get array type holder
	const Array& asArray() const
	{
		JSONCXX_ASSERT(type_ == ArrayType);
		return value_.a;
	}
    
	//! get object type holder
	const Object& asObject() const
	{
		JSONCXX_ASSERT(type_ == ObjectType);
		return value_.o;
	}
    
    //! @}
    
    //! @name	Array functions.
    //! @{
    
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
    
    //! @}
    
    //! @name	Object functions.
    //! @{
    
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
        value_.o.members_->emplace(std::make_pair(std::move(key), std::move(value)));
	}
    //! @}
    
    //! Preliminary implement stream operator
	friend ostream& operator << (ostream& os, const Value& value)
	{
		switch (value.type()) {
            case NullType:
                os << "null";
                break;
            case FalseType:
                os << "false";
                break;
            case TrueType:
                os << "true";
                break;
            case ObjectType:
			{
				os << '{'; // Object begin
                
				const Object& o = value.asObject();
                
                if (!o.empty()) {
                    // lambda function variable
                    auto func = [&](const typename Object::storage_type::value_type& p) -> string {
                        std::basic_ostringstream<char_type> oss;
                        oss << p.first << " : " << p.second;
                        return oss.str();
                    };
                    
                    std::transform(o.begin(), std::next(o.begin(), value.size() - 1),
                                   std::ostream_iterator<string, char_type>(os, ", "),
                                   func);
                    os << func(*std::next(o.begin(), value.size() - 1));
                }
                
				os << '}'; // Object end
			}
                break;
            case ArrayType:
			{
				os << '['; // Array begin
				
				const Array& a = value.asArray();
                
                if (!a.empty()) {
                    std::copy(a.begin(), std::next(a.begin(), value.size() - 1), std::ostream_iterator<const Value<Encoding>&>(os, ", "));
                    os << a.back();
                }
                
				os << ']'; // Array end
			}
                break;
            case StringType:
                os << '\"'
				<< value.asString() 
				<< '\"';
                break;
            case NumberType:
                if (value.asNumber().type_ == NaturalNumber)
                    os << value.asNatural();
                else
                    os << value.asReal();
                break;
		}
        
		return os;
	}
    
    //! Comparator for map
    bool operator < (const self_type& other) const
    {
        if (value_.s.hash_ == other.value_.s.hash_)
            return *value_.s.str_ < *other.value_.s.str_;
        return value_.s.hash_ < other.value_.s.hash_;
    }
    
private:
	ValueHolder     value_;
	ValueType		type_;
};
    
#pragma pack (pop)

}

#ifndef DOXYGEN

namespace std {

	//! template specialization of std::hash for Value type
	template <typename Encoding>
	struct hash<jsoncxx::Value<Encoding> >
		: public unary_function<jsoncxx::Value<Encoding>, size_t>
	{
		typedef std::basic_string<typename Encoding::char_type> string;

		size_t operator()(const jsoncxx::Value<Encoding>& _KeyVal) const
		{
			if(_KeyVal.type() != jsoncxx::StringType)
				throw std::runtime_error("Non-String type Value cannot have hash value!");

			return hash<string>()(_KeyVal.asString());
		}
	};

}

#endif