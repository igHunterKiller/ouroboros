// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// extra type traits for std::atomic types

#pragma once
#include <atomic>

namespace ouro {

template<typename T> struct remove_atomic { typedef T type; };

template<> struct remove_atomic<std::atomic<bool>>               { typedef bool               type; };
template<> struct remove_atomic<std::atomic<char>>               { typedef char               type; };
template<> struct remove_atomic<std::atomic<signed char>>        { typedef signed char        type; };
template<> struct remove_atomic<std::atomic<unsigned char>>      { typedef unsigned char      type; };
template<> struct remove_atomic<std::atomic<short>>              { typedef short              type; };
template<> struct remove_atomic<std::atomic<unsigned short>>     { typedef unsigned short     type; };
template<> struct remove_atomic<std::atomic<int>>                { typedef int                type; };
template<> struct remove_atomic<std::atomic<unsigned int>>       { typedef unsigned int       type; };
template<> struct remove_atomic<std::atomic<long>>               { typedef long               type; };
template<> struct remove_atomic<std::atomic<unsigned long>>      { typedef unsigned long      type; };
template<> struct remove_atomic<std::atomic<long long>>          { typedef long long          type; };
template<> struct remove_atomic<std::atomic<unsigned long long>> { typedef unsigned long long type; };

template<> struct remove_atomic<std::atomic_bool>                { typedef bool               type; };
template<> struct remove_atomic<std::atomic_char>                { typedef char               type; };
template<> struct remove_atomic<std::atomic_schar>               { typedef signed char        type; };
template<> struct remove_atomic<std::atomic_uchar>               { typedef unsigned char      type; };
template<> struct remove_atomic<std::atomic_short>               { typedef short              type; };
template<> struct remove_atomic<std::atomic_ushort>              { typedef unsigned short     type; };
template<> struct remove_atomic<std::atomic_int>                 { typedef int                type; };
template<> struct remove_atomic<std::atomic_uint>                { typedef unsigned int       type; };
template<> struct remove_atomic<std::atomic_long>                { typedef long               type; };
template<> struct remove_atomic<std::atomic_ulong>               { typedef unsigned long      type; };
template<> struct remove_atomic<std::atomic_llong>               { typedef long long          type; };
template<> struct remove_atomic<std::atomic_ullong>              { typedef unsigned long long type; };

}
