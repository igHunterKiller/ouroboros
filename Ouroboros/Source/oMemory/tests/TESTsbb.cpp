// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oCore/finally.h>
#include <oMemory/sbb.h>
#include <stdlib.h>
#include <vector>

using namespace ouro;

void TESTsbb_trivial(unit_test::services& services)
{
	const size_t kArenaSize = 64;
	const size_t kMinBlockSize = 16;
	const size_t kBookkeepingSize = sbb_bookkeeping_size(kArenaSize, kMinBlockSize);
	
	char* bookkeeping = new char[kBookkeepingSize];
	oFinally { if (bookkeeping) delete [] bookkeeping; };

	char* arena = new char[kArenaSize];
	oFinally { if (arena) delete [] arena; };

	sbb_t sbb = sbb_create(arena, kArenaSize, kMinBlockSize, bookkeeping);
	oFinally { if (sbb) sbb_destroy(sbb); };

	void* ExpectedFailBiggerThanArena = sbb_malloc(sbb, 65);
	oCHECK0(!ExpectedFailBiggerThanArena);

//   0    0x7fffffffffffffff
//   1    
// 1   1
//1 1 1 1

	void* a = sbb_malloc(sbb, 1);

//   0   0x17ffffffffffffff
//   0
// 0   1
//0 1 1 1

	oCHECK0(a == arena);
	void* b = sbb_malloc(sbb, 1);

//   0    0x13ffffffffffffff
//   0    
// 0   1
//0 0 1 1

	oCHECK0(b == ((char*)arena + kMinBlockSize));
	void* c = sbb_malloc(sbb, 17);

//   0    0x03ffffffffffffff
//   0    
// 0   0
//0 0 1 1

	oCHECK0(c);
	void* ExpectedFailOOM = sbb_malloc(sbb, 1);
	oCHECK0(!ExpectedFailOOM);

	sbb_free(sbb, b);

//   0   0x07ffffffffffffff
//   0
// 0   0
//0 1 1 1

	sbb_free(sbb, c);

//   0    0x17ffffffffffffff
//   0    
// 0   1
//0 1 1 1

	sbb_free(sbb, a);

//   0  0x7fffffffffffffff  
//   1    
// 1   1
//1 1 1 1

	void* d = sbb_malloc(sbb, kArenaSize);
	oCHECK0(d);
	sbb_free(sbb, d);
}

oTEST(oMemory_sbb)
{
	TESTsbb_trivial(srv);

	const size_t kBadArenaSize = 123445;
	const size_t kBadMinBlockSize = 7;
	const size_t kArenaSize = 512 * 1024 * 1024;
	const size_t kMinBlockSize = 16;
	const size_t kMaxAllocSize = 10 * 1024 * 1024;

	const size_t kBookkeepingSize = sbb_bookkeeping_size(kArenaSize, kMinBlockSize);

	char* bookkeeping = new char[kBookkeepingSize];
	oFinally { if (bookkeeping) delete [] bookkeeping; };

	char* arena = new char[kArenaSize];
	oFinally { if (arena) delete [] arena; };

	bool ExpectedFailSucceeded = true;
	srv.trace("About to test an invalid case, an exception may be caught by the debugger. CONTINUE.");
	try
	{
		sbb_create(arena, kBadArenaSize, kBadMinBlockSize, bookkeeping);
		ExpectedFailSucceeded = false;
	}

	catch (...) {}
	oCHECK0(ExpectedFailSucceeded);

	srv.trace("About to test an invalid case, an exception may be caught by the debugger. CONTINUE.");
	try
	{
		sbb_create(arena, kArenaSize, kBadMinBlockSize, bookkeeping);
		ExpectedFailSucceeded = false;
	}

	catch (...) {}
	oCHECK0(ExpectedFailSucceeded);

	sbb_t sbb = sbb_create(arena, kArenaSize, kMinBlockSize, bookkeeping);
	oFinally { if (sbb) sbb_destroy(sbb); };

	static const size_t kNumIterations = 1000;

	std::vector<void*> pointers(kNumIterations);
	std::fill(std::begin(pointers), std::end(pointers), nullptr);
	for (size_t i = 0; i < kNumIterations; i++)
	{
		const size_t r = srv.rand();
		const size_t amt = __min(kMaxAllocSize, r);
		pointers[i] = sbb_malloc(sbb, amt);
	}

	const size_t count = srv.rand() % kNumIterations;
	for (size_t i = 0; i < count; i++)
	{
		const size_t j = srv.rand() % kNumIterations;
		sbb_free(sbb, pointers[j]);
		pointers[j] = nullptr;
	}

	for (size_t i = 0; i < kNumIterations; i++)
	{
		if (pointers[i])
			continue;

		const size_t r = srv.rand();
		const size_t amt = __min(kMaxAllocSize, r);
		pointers[i] = sbb_malloc(sbb, amt);
	}

	for (size_t i = 0; i < kNumIterations; i++)
	{
		sbb_free(sbb, pointers[i]);
	}

	void* FullBlock = sbb_malloc(sbb, kArenaSize);
	oCHECK0(FullBlock);
}
