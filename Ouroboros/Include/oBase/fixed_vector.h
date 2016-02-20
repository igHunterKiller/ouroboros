// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// std::array, but with some of std::vector's api

#pragma once
#include <oArch/compiler.h>
#include <oString/stringize.h>
#include <array>
#include <stdexcept>

namespace ouro {

template<typename T, size_t N>
class fixed_vector
{
public:
	typedef std::array<T, N> array_t;
	typedef typename array_t::const_iterator const_iterator;
	typedef typename array_t::const_pointer const_pointer;
	typedef typename array_t::const_reference const_reference;
	typedef typename array_t::const_iterator const_reverse_iterator;
	typedef typename array_t::difference_type difference_type;
	typedef typename array_t::iterator iterator;
	typedef typename array_t::pointer pointer;
	typedef typename array_t::reference reference;
	typedef typename array_t::iterator reverse_iterator;
	typedef typename array_t::size_type size_type;
	typedef typename array_t::value_type value_type;

	fixed_vector() : Size(0) {}
	fixed_vector(const fixed_vector& _That) { operator=(_That); }
	fixed_vector(fixed_vector&& _That) : Array(std::move(_That.Array)), Size(_That.Size) { _That.Size = 0;  }
	const array_t& operator=(const array_t& _That) { Array = _That.Array; Size = _That.Size; return *this; }
	array_t& operator=(array_t&& _That) { if (this != &_That) { Array = std::move(_That.Array); std::move(Size = _That.Size); } return *this; }

	template <class InputIterator>
	void assign(InputIterator _First, InputIterator _Last)
	{
		size_type NewSize = static_cast<>(std::distance(_First, _Last));
		check(NewSize);
		destroy_extra(NewSize);
		iterator it = begin();
		while (_First < _Last)
			*it++ = *_First++;
		Size = _NewSize;
	}
	
	void assign(size_type _N, const value_type& value) { resize(_N, value); }
	
	void fill(const value_type& value) { std::fill(begin(), end(), value); }

	reference at(size_type _Index) { return Array.at(_Index); }
	const_reference at(size_type _Index) const { return Array.at(_Index); }
	reference operator[](size_type _Index) { return Array[_Index]; }
	const_reference operator[](size_type _Index) const { return Array[_Index]; }
	value_type* data() noexcept { return Array.data(); }
	const value_type* data() const noexcept { return Array.data(); }

	reference front() { return Array.front(); }
	reference back() { return Array.at(Size-1); }
	const_reference front() const { return Array.front(); }
	const_reference back() const { return Array.at(Size-1); }

	iterator begin() noexcept { return Array.begin(); }
	iterator end() noexcept { return begin() + Size; }
	const_iterator begin() const noexcept { return Array.cbegin(); }
	const_iterator end() const noexcept { return cbegin() + Size; }
	const_iterator cbegin() const noexcept { return Array.cbegin(); }
	const_iterator cend() const noexcept { return cbegin() + Size; }
	reverse_iterator rbegin() noexcept { return rend() + Size; }
	reverse_iterator rend() noexcept { return Array.rend(); }
	const_reverse_iterator rbegin() const noexcept { return rend() + Size; }
	const_reverse_iterator rend() const noexcept { return Array.rend(); }
	const_reverse_iterator crbegin() const noexcept { return crend() + Size; }
	const_reverse_iterator crend() const noexcept { return Array.crend(); }

	void resize(size_type _NewSize, value_type _InitialValue = value_type())
	{
		check(_NewSize);
		destroy_extra(_NewSize);
		for (size_type i = Size; i < _NewSize; i++)
			Array[i] = _InitialValue;
		Size = _NewSize;
	}

	void clear() noexcept { resize(0); }
	bool empty() const noexcept { return Size == 0; }
	size_type max_size() const noexcept { return Array.max_size(); }
	size_type size() const noexcept { return Size; }
	size_type capacity() const noexcept { return Array.max_size(); }

	void push_back(const value_type& value)
	{
		check(Size + 1);
		Array[Size++] = value;
	}

	void push_back(value_type&& value)
	{
		check(Size + 1);
		Array[Size++] = std::move(value);
	}

	void pop_back()
	{
		if (Size)
			Array[--Size].~value_type();
	}

	iterator insert(const_iterator _Position, const value_type& value)
	{
		make_insertion_room(_Position, 1);
		*_Position = value;
		return _Position;
	}

	iterator insert(const_iterator _Position, value_type&& value)
	{
		make_insertion_room(_Position, 1);
		*_Position = std::move(value);
		return _Position;
	}

	iterator insert(const_iterator _Position, size_type _N, const value_type& value)
	{
		make_insertion_room(_Position, _N);
		std::fill(_Position, _Position + _N, value);
		return _Position;
	}

	template <class InputIterator>
	iterator insert(const_iterator _Position, InputIterator _First, InputIterator _Last)
	{
		size_type N = std::distance(_First, _Last);
		make_insertion_room(_Position, N);
		std::copy(_First, _Last, _Position);
	}

	iterator erase(iterator _Position)
	{
		return erase(_Position, _Position+1);
	}

	iterator erase(iterator _First, iterator _Last)
	{
		for (iterator it = _First; it != _Last; ++it)
			(*it).~value_type();

		std::copy(_Last,end(),_First);
		Size -= std::distance(_First, _Last);

		return _First;
	}

	void swap(fixed_vector& _That)
	{
		// Swap smaller run, then move balance
		iterator it = begin(), itEnd = end();
		iterator it2 = _That.begin(); it2End = _That.end();

		if (size() > _That.size())
		{
			std::swap(it, it2);
			std::swap(itEnd, it2End);
		}

		while (it < itEnd)
			std::swap(*it++, *it2++);

		while (it2 < it2End)
			*it = std::move(it2);

		std::swap(Size, _That.Size);
	}

protected:
	array_t Array;
	size_type Size;

	void destroy_extra(size_type _NewSize)
	{
		while (_NewSize < Size)
			Array[_NewSize++].~value_type();
	}

	void check(size_type _NewSize)
	{
		if (_NewSize > max_size())
			throw std::length_error("Array is at max_size");
	}

	void make_insertion_room(iterator _Position, size_type _N)
	{
		check(Size + _N);
		Size += _N;
		iterator itNewEnd = end() + _N - 1;
		for (iterator it = end() - 1, it >= _Position; --it)
			*itNewEnd-- = std::move(*it);
	}
};

template <typename T, size_t N> char* to_string(char* _StrDestination, size_t _SizeofStrDestination, const ouro::fixed_vector<T, N>& _Array) { return detail::to_string_container(_StrDestination, _SizeofStrDestination, _Array); }
template <typename T, size_t sourceN, size_t destinationN> char* to_string(char (&_StrDestination)[destinationN], const ouro::fixed_vector<T, sourceN>& _Array) { return detail::to_string_container(_StrDestination, Ndst, _Array); }
template <typename T, size_t N> bool from_string(ouro::fixed_vector<T, N>* _pValue, const char* _StrSource) { return detail::from_string_container(_pValue, _StrSource); }

}
