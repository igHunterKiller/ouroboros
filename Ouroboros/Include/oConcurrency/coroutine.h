// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// An implementation of a stack based co-routine using a Duffs device
// http://www.chiark.greenend.org.uk/~sgtatham/coroutines.html

// Example usage:
/*
// Inherit your context from a coroutine_context
class MyContext : public coroutine_context
{
	bool Flag;
};

void MyExecute(MyContext& _MyContext)
{
	oCoBegin(&_MyContext);
	MyASyncProcessingThatWillWriteFlagEventually(&_MyContext.Flag);
	// this causes MyExecute to return, but it has marked where it has left off
	oCoYield();
	if (_MyContext.Flag)
	{
		MyOtherProcessing();
	}
	oCoEnd();
}
*/
#pragma once

// Internal implementation details
#define oCoYielder__(continuation,_YieldID) coroutine_context__->yield_id(_YieldID); continuation; return; case _YieldID:
#define oCoYielderUnique__(continuation, _YieldID) oCoYielder__(continuation, _YieldID)

// Pass the address of an coroutine_context derivative to begin coroutine execution.
#define oCoBegin(_pCoroutineContext) ouro::coroutine_context* coroutine_context__ = _pCoroutineContext; switch(coroutine_context__->yield_id()) { case -1:

// End a coroutine block
#define oCoEnd() }

// Exit the coroutine block but record the location so that when reentering the 
// coroutine block the code starts from where it left off. Because of this 
// restart it is important to maintain all state in a class derived from 
// coroutine_context so all the data is still there when reentering the 
// coroutine block.
#define oCoYield() oCoYielderUnique__(;,__COUNTER__)

// Like oCoYield but calls a continuation function after recording the reentry point.
// This allows for safe stack-based asynchronous operations that may call back into
// the caller.
#define oCoYieldContinuation(continuation) oCoYielderUnique__(continuation, __COUNTER__)

namespace ouro {

// Client code should derive a coroutine context (like a thread context) from 
// coroutine_context and use that with macros defined below.
class coroutine_context
{
	// a unique number that indicates where a coroutine left off if it sleeps
	int YieldID;
public:
	coroutine_context() : YieldID(-1) {}
	inline void yield_id(int _YieldID) { YieldID = _YieldID; }
	inline int yield_id() const { return YieldID; }
};

}
