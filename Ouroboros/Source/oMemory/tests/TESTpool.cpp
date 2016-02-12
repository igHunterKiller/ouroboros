// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oMemory/pool.h>
#include <oMemory/object_pool.h>
#include <vector>

using namespace ouro;

static const unsigned int kMagicValue = 0xc001c0de;

struct test_obj
{
	test_obj() : Value(0), pDestroyed(nullptr) {}

	test_obj(bool* _pDestroyed)
		: Value(kMagicValue)
		, pDestroyed(_pDestroyed)
	{
		*pDestroyed = false;
	}

	~test_obj()
	{
		*pDestroyed = true;
	}

	size_t Value;
	bool* pDestroyed;
};

template<typename IndexPoolT>
static void test_index_pool(unit_test::services& services)
{
	const uint32_t CAPACITY = 4;
	const uint32_t BYTES = IndexPoolT::calc_size(CAPACITY, sizeof(uint32_t));
	std::vector<uint32_t> buffer(256, 0xcccccccc);

	IndexPoolT a(buffer.data(), BYTES, sizeof(uint32_t));

	oCHECK(a.full(), "index_allocator did not initialize correctly.");
	oCHECK(a.capacity() == CAPACITY, "Capacity mismatch.");

	unsigned int index[4];
	for (auto i = 0; i < 4; i++)
		index[i] = a.allocate_index();

	oCHECK(a.nullidx == a.allocate_index(), "allocate succeed past allocator capacity");

	for (auto i = 0; i < 4; i++)
		oCHECK(index[i] == static_cast<unsigned int>(i), "Allocation mismatch %u.", i);

	a.deallocate(index[1]);
	a.deallocate(index[0]);
	a.deallocate(index[2]);
	a.deallocate(index[3]);

	oCHECK(a.full(), "A deallocate failed.");
}

template<typename test_block_poolT>
static void test_allocate(unit_test::services& services)
{
	static const uint32_t NumBlocks = 20;
	static const uint32_t NumBytes = test_block_poolT::calc_size(NumBlocks);
	std::vector<char> scopedArena(NumBytes);
	
	test_block_poolT Allocator(scopedArena.data(), NumBytes);
	oCHECK(NumBlocks == Allocator.size_free(), "There should be %u available blocks (after init)", NumBlocks);

	void* tests[NumBlocks];
	for (size_t i = 0; i < NumBlocks; i++)
	{
		tests[i] = Allocator.allocate();
		oCHECK(tests[i], "test_obj %u should have been allocated", i);
	}

	void* shouldBeNull = Allocator.allocate();
	oCHECK(!shouldBeNull, "Allocation should have failed");

	oCHECK(0 == Allocator.size_free(), "There should be 0 available blocks");

	for (size_t i = 0; i < NumBlocks; i++)
		Allocator.deallocate(tests[i]);

	oCHECK(NumBlocks == Allocator.size_free(), "There should be %u available blocks (after deallocate)", NumBlocks);
}

template<typename test_obj_poolT>
static void test_create(unit_test::services& services)
{
	static const uint32_t NumBlocks = 20;
	static const uint32_t NumBytes = test_obj_poolT::calc_size(NumBlocks);
	std::vector<char> scopedArena(NumBytes);
	test_obj_poolT Allocator(scopedArena.data(), NumBytes);
		
	bool testdestroyed[NumBlocks];
	test_obj* tests[NumBlocks];
	for (size_t i = 0; i < NumBlocks; i++)
	{
		tests[i] = Allocator.create(&testdestroyed[i]);
		oCHECK(tests[i], "test_obj %u should have been allocated", i);
		oCHECK(tests[i]->Value == kMagicValue, "test_obj %u should have been constructed", i);
		oCHECK(tests[i]->pDestroyed && false == *tests[i]->pDestroyed, "test_obj %u should have been constructed", i);
	}

	for (size_t i = 0; i < NumBlocks; i++)
	{
		Allocator.destroy(tests[i]);
		oCHECK(testdestroyed[i] == true, "test_obj %u should have been destroyed", i);
	}

	oCHECK(NumBlocks == Allocator.size_free(), "There should be %u available blocks (after deallocate)", NumBlocks);
}

oTEST(oMemory_pool)
{
	test_index_pool<pool>(srv);
	test_allocate<object_pool<test_obj>>(srv);
	test_create<object_pool<test_obj>>(srv);
}
