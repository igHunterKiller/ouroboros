// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oCore/countof.h>
#include <oConcurrency/concurrency.h>

using namespace ouro;

static void test_a(size_t _Index, int* _pArray)
{
	_pArray[_Index] = (int)(_Index);
}

static void test_ab(size_t _Index, int* _pArray, int _Loop)
{
	int final = (int)_Index;
	for (int i = 0; i < _Loop; i++)
		final *= final; 
	_pArray[_Index] = final;
}

static void test_abc(size_t _Index, int* _pArray, int _Loop, int _Test)
{
	int final = (int)_Index;
	for(int i = 0; i < _Loop; i++)
		final *= final; 
	_pArray[_Index] = _Index % _Test ? 42 : final;
}

oTEST(oConcurrency_parallel_for)
{
	static const int mArraySize = 128;
	int mTestArrayA[mArraySize];
	int mTestArrayB[mArraySize];

	// Test single parameter
	for (auto i = 0; i < countof(mTestArrayA); i++)
		test_a(i, mTestArrayA);

	parallel_for(0, mArraySize, std::bind(test_a, std::placeholders::_1, mTestArrayB));
	oCHECK(memcmp(mTestArrayA, mTestArrayB, mArraySize * sizeof(int)) == 0, 
		"ouro::parallel_for single param failed to compute singlethreaded result");

	// Test two parameters
	for (auto i = 0; i < countof(mTestArrayA); i++)
		test_ab(i, mTestArrayA, 2);

	parallel_for(0, mArraySize, std::bind(test_ab, std::placeholders::_1, &mTestArrayB[0], 2));
	oCHECK(memcmp(mTestArrayA, mTestArrayB, mArraySize * sizeof(int)) == 0
		, "ouro::parallel_for failed to compute singlethreaded result");

	// Test three parameters
	for (auto i = 0; i < countof(mTestArrayA); i++)
		test_abc(i, mTestArrayA, 2, 7);

	parallel_for(0, mArraySize, std::bind(test_abc, std::placeholders::_1, &mTestArrayB[0], 2, 7));
	oCHECK(memcmp(mTestArrayA, mTestArrayB, mArraySize * sizeof(int)) == 0
		, "ouro::parallel_for failed to compute singlethreaded result");

	parallel_for(0, mArraySize, std::bind(test_ab, std::placeholders::_1, &mTestArrayB[0], 2));
};
