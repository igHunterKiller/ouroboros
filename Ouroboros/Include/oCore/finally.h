// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Use this to call a by-reference lamba capture on the exit of the scope.
// This is useful for handling errors in an exception-friendly way.
// Example:
//
// void myclass::initialize()
// {
//   void* mem = malloc(big_size);
//   oFinally { if (mem) free(mem); }
//   if (!calculate_lots_of_data(mem, big_size))
//     throw std::exception("failed");
//   this->memory_ = mem;
//   mem = nullptr; // IMPORTANT TO PREVENT oFinally'S FREE
// }

#pragma once
#include <utility>

namespace ouro { namespace detail {

template<typename functionT>
class finally_base
{
public:
	finally_base(functionT&& fn) : fn_(std::move(fn)) {}
	finally_base(finally_base&& that) : fn_(std::move(that.fn_)) {}
	~finally_base() { fn_(); }

private:
	finally_base();
	finally_base(const finally_base&);
	const finally_base& operator=(const finally_base&);

	functionT fn_;
};

struct finally_tag {};

template <typename functionT>
finally_base<functionT> operator|(finally_tag, functionT&& fn)
{
  return finally_base<functionT>(std::move(fn));
}

}}

#define oFINALLY_CONCAT1(X,Y) X##Y
#define oFINALLY_CONCAT(X,Y) oFINALLY_CONCAT1(X,Y)

// This is the macro to use
#define oFinally auto oFINALLY_CONCAT(ouro_finally_, __LINE__) = ::ouro::detail::finally_tag() | [&]
