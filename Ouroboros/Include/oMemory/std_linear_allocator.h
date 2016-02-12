// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Wraps a linear_allocator in std::allocator's API as well as falling back
// on the default std::allocator if the original arena is insufficient so
// this can gain efficiency if the size of allocations is known ahead of time
// but also won't fail if that estimate comes up short.

#pragma once
#include <oMemory/std_allocator.h>
#include <oMemory/linear_allocator.h>

namespace ouro {

template<typename T> class std_linear_allocator
{
public:
	oDEFINE_STD_ALLOCATOR_BOILERPLATE(std_linear_allocator)
	std_linear_allocator(linear_allocator* alloc, size_t* out_highwater)
			: alloc_(alloc)
			, highwater_(out_highwater)
	{
		if (highwater_)
			*highwater_ = 0;
	}

	~std_linear_allocator() {}
	
	template<typename U> std_linear_allocator(std_linear_allocator<U> const& that)
		: alloc_(that.alloc_)
		, highwater_(that.highwater_)
	{}
	
	inline const std_linear_allocator& operator=(const std_linear_allocator& that)
	{
		alloc_ = that.alloc_;
		fallback_ = that.fallback_;
		highwater_ = that.highwater_;
		return *this;
	}
	
	inline pointer allocate(size_type count, const_pointer hint = 0)
	{
		const size_t nBytes = sizeof(T) * count;
		void* p = alloc_->allocate(nBytes);
		if (!p)
			p = fallback_.allocate(nBytes, hint);
		if (p && highwater_)
			*highwater_ = max(*highwater_, nBytes);
		return static_cast<pointer>(p);
	}
	
	inline void deallocate(pointer p, size_type count)
	{
		if (!alloc_->owns(p))
			fallback_.deallocate(p, count);
	}
	
	inline void reset() { alloc_->reset(); }
	
	inline size_t highwater() const { return highwater_ ? *highwater_ : 0; }

private:
	template<typename> friend class std_linear_allocator;
	linear_allocator* alloc_;
	size_t* highwater_;
	std::allocator<T> fallback_;
};

}

oDEFINE_STD_ALLOCATOR_VOID_INSTANTIATION(ouro::std_linear_allocator)
oDEFINE_STD_ALLOCATOR_OPERATOR_EQUAL(ouro::std_linear_allocator) { return a.alloc_ == b.alloc_; }
