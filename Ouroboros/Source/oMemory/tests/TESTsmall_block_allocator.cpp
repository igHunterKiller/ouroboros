// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oCore/countof.h>
#include <oMemory/allocate.h>
#include <oCore/byte.h>
#include <oMemory/small_block_allocator.h>
#include <vector>

using namespace ouro;

oTEST(oMemory_small_block_allocator)
{
	uint16_t sBlockSizes[] = { 8, 16, 64, 256, 8192 };
	uint32_t size = 4;
	void* mem = nullptr;

	std::vector<char> tmp((size+1) * small_block_allocator::chunk_size);
	
	small_block_allocator a;

	{
		bool ExpectedFail = false;
		srv.trace("Expecting failure on invalid argument...");
		try
		{
			a.initialize(mem, size, sBlockSizes, countof(sBlockSizes));
		}
		catch (ouro::allocate_error&)
		{
			ExpectedFail = true;
		}

		oCHECK(ExpectedFail, "initialize should have thrown on invalid argument");
	}

	mem = align(tmp.data(), small_block_allocator::chunk_size);

	{
		bool ExpectedFail = false;
		srv.trace("Expecting failure on invalid argument...");
		try
		{
			a.initialize(mem, size, sBlockSizes, countof(sBlockSizes));
		}
		catch (ouro::allocate_error&)
		{
			ExpectedFail = true;
		}

		oCHECK(ExpectedFail, "initialize should have thrown on invalid argument");
	}
	
	size *= small_block_allocator::chunk_size;
	a.initialize(mem, size, sBlockSizes, countof(sBlockSizes));

	void* test = a.allocate(13);
	oCHECK(!test, "allocated unspecified block size");

	void* tests[16];

	for (int i = 0; i < countof(tests); i++)
		tests[i] = a.allocate(8192);

	for (int i = 0; i < 12; i++)
		oCHECK(tests[i], "expected a valid alloc");

	for (int i = 12; i < countof(tests); i++)
		oCHECK(!tests[i], "expected a nullptr");

	test = a.allocate(64);
	oCHECK(!test, "expected a nullptr because allocator is full");

	for (int i = 0; i < 3; i++)
	{
		a.deallocate(tests[i]);
		tests[i] = nullptr;
	}

	// that should've freed a chunk
	test = a.allocate(64);
	oCHECK(test, "smaller block should have allocated");
	a.deallocate(test);

	// that should've freed a chunk
	test = a.allocate(16);
	oCHECK(test, "different block should have allocated");

	for (int i = 6; i < 9; i++)
	{
		a.deallocate(tests[i]);
		tests[i] = nullptr;
	}

	// that should've freed a chunk
	void* test2 = a.allocate(64);
	oCHECK(test2, "should have a chunk ready for a new block size");

	a.deallocate(test2);
	a.deallocate(test);

	for (int i = 0; i < countof(tests); i++)
		a.deallocate(tests[i]);

	a.deinitialize();
}
