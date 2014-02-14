/*********************************************
 * listmap.h
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 13 Feb, 2014
 ********************************************/

/**
 * NOTICE: several articles may be referenced to clearly understand this file.
 * http://www.boost.org/doc/libs/1_55_0/libs/iterator/doc/index.html
 * http://www.boost.org/doc/libs/1_55_0/libs/iterator/doc/iterator_facade.html
 * http://www.boost.org/doc/libs/1_55_0/libs/iterator/doc/iterator_adaptor.html
 * http://www.codeproject.com/Articles/36530/An-Introduction-to-Iterator-Traits
*/

#ifndef _LISTMAP_H_
#define _LISTMAP_H_

#include <map>
#include <boost/utility/enable_if.hpp>
#include <boost/iterator_adaptors.hpp>

/**
 * The iterator for class listmap.
 */
template <class Value>
class listmap_iter : public boost::iterator_adaptor<
		listmap_iter<Value>               // Derived
		, typename Value::Base            // Base
		, boost::use_default              // Value
		, boost::forward_traversal_tag    // CategoryOrTraversal
>
{
private:
	struct enabler {};  // a private type avoids misuse

public:
	listmap_iter()
	: listmap_iter::iterator_adaptor_(0) {}

	explicit listmap_iter(Value* p)
	: listmap_iter::iterator_adaptor_(p) {}

    template <class OtherValue>
    listmap_iter(
    	  listmap_iter<OtherValue> const& other
    	  , typename boost::enable_if<boost::is_convertible<OtherValue*,Value*>, enabler >::type = enabler()
    )
      : listmap_iter::iterator_adaptor_(other.base()) {}

private:
	friend class boost::iterator_core_access;
	void increment()
	{
		this->base_reference() = this->base()->next();
	}
};

/**
 * This container combines std::map and double linked list.
 * All the elements in this container can be accessed randomly using key whose type is specified by K.
 * All the elements can also be accessed using the iterator which traverses over all of them
 * in the chronological order.
 */
template<typename K, typename V>
class listmap
{
private:
	// This class is intended for double-linking.
	struct Trident  // 三叉戟
	{
		typedef V*         value_type;
		typedef ptrdiff_t  difference_type;
		typedef boost::forward_traversal_tag iterator_category;
		typedef V**        pointer;
		typedef V*&        reference;

		Trident* _pre;
		Trident* _next;
		V* _value;

		Trident() : _pre(NULL), _next(NULL), _value(NULL)
		{}

		typedef Trident Base;

		void next()
		{
			if (NULL != _next)
			{
				_pre = this;
				_value = _next->_value;
				_next = _next->_next;
			}
			else
			{
				abort();
			}
		}
	};

public:
	typedef listmap_iter<Trident> listmap_iterator;
	typedef listmap_iter<Trident const> listmap_const_iterator;

public:
	listmap();
	virtual ~listmap();

	listmap_iterator begin();
	listmap_iterator end();

	listmap_const_iterator begin()const;
	listmap_const_iterator end()const;

	// comply with the std::map's behavior of operator[].
	V& operator[](K key);

private:
	typedef std::map<K, Trident> InternalMap;
	InternalMap _map_elements;
};

template<typename K, typename V>
listmap<K, V>::listmap()
{
}

template<typename K, typename V>
listmap<K, V>::~listmap()
{
	for (typename InternalMap::iterator ite = _map_elements.begin();
			ite != _map_elements.end();
			++ite )
	{
		delete ite->second._value;
	}
}

template<typename K, typename V>
V& listmap<K, V>::operator[](K key)
{
	Trident t;
	t = _map_elements[key];
	if (NULL == t->_value)
	{
		t->_value = new V;
	}

	return *t->_value;
}

template<typename K, typename V>
typename listmap<K, V>::listmap_iterator listmap<K, V>::begin()
{
	return listmap_iterator();
}

template<typename K, typename V>
typename listmap<K, V>::listmap_iterator listmap<K, V>::end()
{
	return listmap_iterator();
}

template<typename K, typename V>
typename listmap<K, V>::listmap_const_iterator listmap<K, V>::begin()const
{
	return listmap_iterator();
}

template<typename K, typename V>
typename listmap<K, V>::listmap_const_iterator listmap<K, V>::end()const
{
	return listmap_iterator();
}

#endif /* _LISTMAP_H_ */
