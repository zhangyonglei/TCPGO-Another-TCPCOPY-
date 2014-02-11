/*********************************************
 * listmap.h
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 13 Feb, 2014
 ********************************************/

#ifndef _LISTMAP_H_
#define _LISTMAP_H_

#include "misc.h"
#include <boost/utility/enable_if.hpp>
#include <boost/iterator_adaptors.hpp>

/**
 * The iterator for class listmap.
 */
template <class Value>
class listmap_iter : public boost::iterator_adaptor<
		listmap_iter<Value>               // Derived
		, Value*                          // Base
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
		//this->base_reference() = this->base()->next();
	}
};

/**
 * This container combines std::map and std::list.
 * All the elements in this container can be accessed randomly using key whose byte is K
 * All the elements can also be accessed using the iterator which traverse over all of them
 * in a chronological order.
 */
template<typename K, typename V>
class listmap
{
public:
	typedef listmap_iter<V> listmap_iterator;
	typedef listmap_iter<V const> listmap_const_iterator;

public:
	listmap();
	virtual ~listmap();

	listmap_iterator begin();
	listmap_iterator end();

	listmap_const_iterator begin()const;
	listmap_const_iterator end()const;

	V* operator[](int index);

private:
	std::map<K, V*> _map_elements;
	std::list<V*> _list_elements;
};

template<typename K, typename V>
listmap<K, V>::listmap()
{
}

template<typename K, typename V>
listmap<K, V>::~listmap()
{
}

template<typename K, typename V>
typename listmap<K, V>::listmap_iterator listmap<K, V>::begin()
{
	return listmap_iterator();
}

template<typename K, typename V>
typename listmap<K, V>::listmap_iterator listmap<K, V>::end()
{
	return listmap_iterator();;
}

template<typename K, typename V>
typename listmap<K, V>::listmap_const_iterator listmap<K, V>::begin()const
{
	return listmap_iterator();;
}

template<typename K, typename V>
typename listmap<K, V>::listmap_const_iterator listmap<K, V>::end()const
{
	return listmap_iterator();;
}


#endif /* _LISTMAP_H_ */
