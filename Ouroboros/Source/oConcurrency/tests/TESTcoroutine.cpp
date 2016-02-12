// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oConcurrency/coroutine.h>
#include <oConcurrency/future.h>
#include <thread>

using namespace ouro;

class MyContext : public coroutine_context
{
public:
	MyContext() 
		: Flag(false)
		, Done(false)
		, Counter(0)
	{}

	future<void> Future;
	bool Flag;
	bool Done;
	int Counter;
};

void MyExecute(MyContext& _MyContext)
{
	_MyContext.Counter++;
	oCoBegin(&_MyContext);
	_MyContext.Future = ouro::async([&](){ _MyContext.Flag = true; });
	oCoYield(); // this causes MyExecute to return, but it has marked where it has left off
	if (!_MyContext.Future.is_ready())
		return;
	_MyContext.Done = true;
	oCoEnd();
}

oTEST(oConcurrency_coroutine)
{
	MyContext myContext;

	while (!myContext.Done)
	{
		MyExecute(myContext);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}
