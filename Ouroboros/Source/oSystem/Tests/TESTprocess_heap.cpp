// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oSystem/process_heap.h>

struct TestStaticContext
{
	TestStaticContext()
		: Counter(1234)
	{}

	int Counter;

	static void Ctor(void* _Memory) { new (_Memory) TestStaticContext(); }
};

oTEST(oSystem_process_heap)
{
	TestStaticContext* c = 0;
	bool allocated = process_heap::find_or_allocate(
		sizeof(TestStaticContext)
		, typeid(TestStaticContext).name()
		, process_heap::per_process
		, process_heap::leak_tracked
		, TestStaticContext::Ctor
		, nullptr
		, (void**)&c);
	
	oCHECK(allocated && c && c->Counter == 1234, "Failed to construct context");
	
	c->Counter = 4321;
	allocated = process_heap::find_or_allocate(
		sizeof(TestStaticContext)
		, typeid(TestStaticContext).name()
		, process_heap::per_process
		, process_heap::leak_tracked
		, TestStaticContext::Ctor
		, nullptr
		, (void**)&c);
	oCHECK(!allocated && c && c->Counter == 4321, "Failed to attach context");

	process_heap::deallocate(c);
}
