/*********************************************
 * listmap.h
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 13 Feb, 2014
 ********************************************/

/**
 * NOTICE: several articles may be referenced to fully understand this file.
 * http://www.boost.org/doc/libs/1_55_0/libs/iterator/doc/index.html
 * http://www.boost.org/doc/libs/1_55_0/libs/iterator/doc/iterator_facade.html
 * http://www.codeproject.com/Articles/36530/An-Introduction-to-Iterator-Traits
 * http://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
 */

#ifndef _LISTMAP_H_
#define _LISTMAP_H_

#include <map>
#include <boost/utility/enable_if.hpp>
#include <boost/iterator/iterator_facade.hpp>

// This class is intended for double-linking.
template<typename V> struct Quadrille  // 四元组 is a proxy to implement iterator.
{
	// to comply with STL specification.
	typedef V          value_type;
	typedef ptrdiff_t  difference_type;
	typedef boost::forward_traversal_tag iterator_category;
	typedef V*         pointer;
	typedef V&         reference;

	Quadrille() : _prev(NULL), _next(NULL), _data(NULL), _auxil_info(NULL)
	{}

	reference dereference()
	{
		return *_data;
	}

	bool equal(const V* v)
	{
		bool b;
		if (v == NULL || _data == NULL)
		{
			b = (v == _data);
		}
		else
		{
			b = (*v == *_data);
		}

		return b;
	}

	bool equal(const Quadrille<V>& t)
	{
		return equal(t._data);
	}

	void increment()
	{
		if (NULL != _next)
		{
			_prev = this;
			_data = _next->_data;
			_auxil_info = _next->_auxil_info;
			_next = _next->_next;
		}
		else
		{
			// the position past the last element.
			_prev = this;
			_data = NULL;
			_auxil_info = NULL;
			_next = NULL;
		}
	}

	bool operator==(const Quadrille& t)
	{
		bool b;
		b = (_data == t._data);
		assert(_auxil_info == t._auxil_info);

		return b;
	}

	bool operator!=(const Quadrille& t)
	{
		return !operator==(t);
	}

	Quadrille<V>* _prev;
	Quadrille<V>* _next;
	V* _data;
	void* _auxil_info;
};

/**
 * The iterator for class listmap.
 */
template <typename Value>
class listmap_iter
: public boost::iterator_facade<
  listmap_iter<Value>             // CRTP
, Value                           // The actual value that listmap_iter wraps.
, boost::forward_traversal_tag    // CategoryOrTraversal
>
{
public:
	explicit listmap_iter(Quadrille<Value>* p = NULL) : _ptr2quadrille(p) {
	}

    template <class OtherValue> listmap_iter(listmap_iter<OtherValue> const& other)
     	 	 : _ptr2quadrille(other._ptr2quadrille) {}

private:
   friend class boost::iterator_core_access;
   template <class, class> friend class listmap;
   template <class> friend class listmap_iter;

   template <class OtherValue>
   bool equal(listmap_iter<OtherValue> const& other)const
   {
	   bool b;
	   if ( NULL == _ptr2quadrille || NULL == other._ptr2quadrille)
	   {
		   b = (_ptr2quadrille == other._ptr2quadrille);
		   return b;
	   }

       b = _ptr2quadrille->equal(*other._ptr2quadrille);
       return b;
   }

   void increment()
   {
	   _ptr2quadrille = _ptr2quadrille->_next;
   }

   Value& dereference()const
   {
	   return _ptr2quadrille->dereference();
   }

   Quadrille<Value>* current_quadrille()
   {
	   return _ptr2quadrille;
   }

private:
   Quadrille<Value>* _ptr2quadrille;
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
public:
	typedef Quadrille<V> MappedElementType;
	typedef std::map<K, MappedElementType> InternalMap;
	typedef listmap_iter<V> iterator;
	typedef listmap_iter<V const> const_iterator;
	typedef int size_type;
	typedef K key_type;
	typedef V value_type;

public:
	listmap();
	virtual ~listmap();

	iterator begin();
	iterator end();
	// just lack the motivation to support the const version until it's necessary to do so.
//	const_iterator begin()const;
//	const_iterator end()const;

	// comply with the std::map's behavior of operator[].
	V& operator[](K key);

	iterator find (const key_type& k);

	void erase (iterator position);
	size_type erase (const key_type& k);

	void clear();

	size_type size()const;

	bool empty()const;

	K get_key(iterator ite)const;

private:
	MappedElementType* _head; ///< this variable is intended for building the iterator returned by begin()
	MappedElementType* _tail;   ///< point to the tail

	InternalMap _map_elements;
};

template<typename K, typename V>
listmap<K, V>::listmap()
{
	_head = NULL;
	_tail = NULL;
}

template<typename K, typename V>
listmap<K, V>::~listmap()
{
	clear();
}

template<typename K, typename V>
V& listmap<K, V>::operator[](K key)
{
	std::pair<typename InternalMap::iterator, bool> ret;

	ret = _map_elements.insert(std::pair<K, MappedElementType>(key, MappedElementType()));
	MappedElementType& elem = ret.first->second;
	if (ret.second)  // the newly created element has been inserted successfully..
	{
		int sz;

		elem._data = new V;
		elem._auxil_info = new typename InternalMap::iterator(ret.first);
		sz = _map_elements.size();

		if ( sz != 1)
		{
			_tail->_next = &elem;
			elem._prev = _tail;
			elem._next = NULL;
			_tail = &elem;
		}
		else  // the first element.
		{
			assert(sz == 1);
			assert(_head == NULL);
			assert(_tail == NULL);

			elem._prev = NULL;
			elem._next = NULL;
			_head = &elem;
			_tail = _head;
		}
	}

	return *elem._data;
}

template<typename K, typename V>
typename listmap<K, V>::iterator listmap<K, V>::find(const key_type& k)
{
	typename InternalMap::iterator map_ite;

	map_ite = _map_elements.find(k);
	if (map_ite != _map_elements.end())
	{
		return iterator(&map_ite->second);
	}
	else  // failed to find the key.
	{
		return end();
	}
}

template<typename K, typename V>
void listmap<K, V>::erase(iterator position)
{
	assert(position != end());

	// remove the first element.
	if (position == iterator(_head))
	{
		assert(NULL != _head);

		MappedElementType* orig_head;
		orig_head = _head;
		_head = _head->_next;

		delete orig_head->_data;
		typename InternalMap::iterator* ptr_ite = (typename InternalMap::iterator*)orig_head->_auxil_info;
		_map_elements.erase(*ptr_ite);
		delete ptr_ite;
	}
	else
	{
		MappedElementType* prev;
		MappedElementType* next;
		MappedElementType* current;

		current = position.current_quadrille();
		prev = current->_prev;
		next = current->_next;

		// relink
		prev->_next = current->_next;
		if (next != NULL)
		{
			next->_prev = current->_prev;
		}
		else  // the tail will be removed
		{
			_tail = prev;
		}

		delete current->_data;
		typename InternalMap::iterator* ptr_ite = (typename InternalMap::iterator*)current->_auxil_info;
		_map_elements.erase(*ptr_ite);
		delete ptr_ite;
	}
}

template<typename K, typename V>
typename listmap<K, V>::size_type listmap<K, V>::erase(const key_type& k)
{
	typename InternalMap::iterator ite;
	MappedElementType* elem;

	ite = _map_elements.find(k);
	if (ite == _map_elements.end())
	return 0;

	elem = &ite->second;
	iterator ite_rm(elem);
	erase(ite_rm);
	return 1;
}

template<typename K, typename V>
void listmap<K, V>::clear()
{
	typename InternalMap::iterator* ptr;
	for (typename InternalMap::iterator ite = _map_elements.begin();
			ite != _map_elements.end();
			++ite )
	{
		ptr = (typename InternalMap::iterator*)ite->second._auxil_info;
		delete ptr;
		delete ite->second._data;
	}

	_map_elements.clear();
	_head = NULL;
	_tail = NULL;
}

template<typename K, typename V>
typename listmap<K, V>::size_type listmap<K, V>::size()const
{
	return _map_elements.size();
}

template<typename K, typename V>
bool listmap<K, V>::empty()const
{
	int sz = size();
	return sz == 0;
}

template<typename K, typename V>
typename listmap<K, V>::key_type listmap<K, V>::get_key(iterator ite)const
{
	typename InternalMap::iterator* stdmap_ite = (typename InternalMap::iterator*)(ite._ptr2quadrille->_auxil_info);
	return (*stdmap_ite)->first;
}

template<typename K, typename V>
typename listmap<K, V>::iterator listmap<K, V>::begin()
{
	return iterator(_head);
}

template<typename K, typename V>
typename listmap<K, V>::iterator listmap<K, V>::end()
{
	return iterator(NULL);
}

/*
template<typename K, typename V>
typename listmap<K, V>::const_iterator listmap<K, V>::begin()const
{
	return iterator();
}

template<typename K, typename V>
typename listmap<K, V>::const_iterator listmap<K, V>::end()const
{
	return iterator();
}*/

#endif /* _LISTMAP_H_ */
