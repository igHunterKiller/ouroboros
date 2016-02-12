// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// extensions to standard <algorithm>

#pragma once
#include <algorithm>
#include <utility>

namespace ouro {

// returns true if the item is found and erased, false otherwise
template<typename T, typename containerT>
bool find_and_erase(containerT& container, const T& item)
{
	auto end = std::end(container);
	auto it = std::find(std::begin(container), end, item);
	const bool found = it != std::end(container);
	if (found)
		container.erase(it);
	return found;
}

// scans for first occurrence whose operator bool() returns false and sets item
// to that slot. If the container is full, push_back is called.
template<typename T, typename containerT>
size_t sparse_set(containerT& container, const T& item)
{
	for (auto it = std::begin(container); it != std::end(container); ++it)
	{
		if (!*it)
		{
			*it = item;
			return std::distance(std::begin(container), it);
			break;
		}
	}

	container.push_back(item);
	return container.size() - 1;
}

// inserts item only if it's not already present and returns its index
template<typename T, typename containerT>
size_t push_back_unique(containerT& container, const T& item)
{
	auto it = std::find(std::begin(container), std::end(container), value);
	if (it != std::end(container))
		return std::distance(std::begin(container), it);
	container.push_back(item);
	return container.size() - 1;
}

template<typename T, typename containerT>
size_t push_back_unique(containerT& container, T&& item)
{
	auto it = std::find(std::begin(container), std::end(container), item);
	if (it != std::end(container))
	{
		auto tmp = std::move(item); // eviscerate input no matter what so behavior is uniform
		return std::distance(std::begin(container), it);
	}
	container.push_back(std::move(item));
	return container.size() - 1;
}

// resizes array so that container[index] = item succeeds
template<typename T, typename containerT>
void safe_set(containerT& container, size_t index, const T& item)
{
	if (container.size() <= index)
		container.resize(index + 1);
	container[index] = item;
}

template<typename T, typename containerT>
void safe_set(containerT& container, size_t index, T&& item)
{
	if (container.size() <= index)
		container.resize(index + 1);
	container[index] = std::move(item);
}

// container[index] = item if index is in range [0,size()) returns true if set, false otherwise
template<typename T, typename indexT, typename containerT>
bool ranged_set(containerT& container, const indexT& index, const T& item)
{
	const bool in_range = index >= 0 && index < static_cast<index>(container.size());
	if (in_range)
		container[index] = item;
	return in_range;
}

template<typename T, typename indexT, typename containerT>
bool ranged_set(containerT& container, const indexT& index, T&& item)
{
	const bool in_range = index >= 0 && index < static_cast<indexT>(container.size());
	if (in_range)
		container[index] = std::move(item);
	return in_range;
}

// fill container with parsed string tokens
template<typename traitsT, typename str_allocatorT, typename vec_allocatorT>
inline void tokenize(std::vector<std::basic_string<char, traitsT, str_allocatorT>, vec_allocatorT>& container, const char* tokens, const char* delim)
{
	container.clear();
	if (tokens)
	{
		std::string copy(tokens);
		char* ctx = nullptr;
		#ifdef _MSC_VER
			const char* tok = strtok_s((char*)copy.c_str(), delim, &ctx);
		#else
			const char* tok = strtok_r((char*)copy.c_str(), delim, &ctx); // posix
		#endif
		while (tok)
		{
			container.push_back(tok);
			#ifdef _MSC_VER
				tok = strtok_s(nullptr, delim, &ctx);
			#else
				tok = strtok_r(nullptr, delim, &ctx); // posix
			#endif
		}
	}
}

}
