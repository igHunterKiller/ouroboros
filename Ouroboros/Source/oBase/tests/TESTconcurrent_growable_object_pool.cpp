// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oCore/finally.h>
#include <oBase/concurrent_growable_object_pool.h>
#include <oCore/assert.h>
#include <oConcurrency/concurrency.h>
#include <oSystem/windows/win_crt_leak_tracker.h>
#include <vector>

using namespace ouro;

struct test_obj
{
	test_obj(bool* _pDestroyed)
		: Value(0xc001c0de)
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

oTEST(oBase_concurrent_growable_object_pool)
{
	static const int NumBlocks = 2000;
	static const int NumBlocksPerChunk = 10;
 	concurrent_growable_object_pool<test_obj> Allocator(NumBlocksPerChunk);
	try 
	{
		bool destroyed[NumBlocks];
		memset(destroyed, 0, sizeof(destroyed));

		test_obj* tests[NumBlocks];
		memset(tests, 0xaa, sizeof(tests));

		//ouro::windows::crt_leak_tracker::enable(false);
		//finally reenable([&] { ouro::windows::crt_leak_tracker::enable(true); });

		ouro::parallel_for(0, NumBlocks, [&](size_t _Index)
		{
			tests[_Index] = Allocator.create(&destroyed[_Index]);
			tests[_Index]->Value = _Index;
		});

		size_t expectedChunks = NumBlocks / NumBlocksPerChunk;
		if ((NumBlocks % NumBlocksPerChunk) != 0)
			expectedChunks++;

		// Because of concurrency, it could be that two or more threads come to the 
		// conclusion that all chunks are exhausted and BOTH decide to create new 
		// chunks. If that happens on the last iteration of the test, then more than
		// expected chunks might be created.
		size_t currentNumChunks = Allocator.num_chunks();
		oCHECK(currentNumChunks >= expectedChunks, "Unexpected number of chunks allocated");

		Allocator.shrink(2);
		oCHECK(Allocator.num_chunks() == currentNumChunks, "Unexpected number of chunks allocated (after false shrink try)"); // shouldn't modify because there is 100% allocation

		oTRACE("ouro::parallel_for { Allocator.destroy }");

		ouro::parallel_for(0, NumBlocks, [&](size_t _Index)
		{
			Allocator.destroy(tests[_Index]);
		});

		Allocator.shrink(2);
		oCHECK(Allocator.num_chunks() == 1, "Unexpected number of chunks allocated (after real shrink)");

		Allocator.grow(5 * Allocator.chunk_capacity());
		oCHECK(Allocator.num_chunks() == 5, "Unexpected number of chunks allocated (after grow)");

		Allocator.shrink(0);
		oCHECK(Allocator.num_chunks() == 0, "Unexpected number of chunks allocated (after 2nd shrink)");

		oTRACE("ouro::parallel_for { if (odd) destroy }");

		ouro::parallel_for(0, NumBlocks, [&](size_t _Index)
		{
			tests[_Index] = Allocator.create(&destroyed[_Index]);
			tests[_Index]->Value = _Index;
			if (_Index & 0x1)
				Allocator.destroy(tests[_Index]);
		});

		oTRACE("ouro::parallel_for { Destroy remaining evens }");

		for (size_t i = 0; i < NumBlocks; i++)
		{
			if (i & 0x1)
				oCHECK(destroyed[i], "Destruction did not occur for allocation %d", i);
			else
			{
				oCHECK(tests[i]->Value == i, "Constructor did not occur for allocation %d", i);
				Allocator.destroy(tests[i]);
			}
		}
	}

	catch (std::exception&)
	{
	}
}
