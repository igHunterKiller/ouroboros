// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oCore/assert.h>
#include <oCore/finally.h>
#include <oConcurrency/concurrent_stack.h>
#include <oConcurrency/concurrency.h>
#include <oMemory/allocate.h>
#include <vector>

using namespace ouro;

struct Node
{
	Node() { memset(this, 0xaa, sizeof(Node)); }
	Node* next;
	size_t value;
};

static void test_intrusive_basics(unit_test::services& services)
{
	Node* values = (Node*)default_allocate(sizeof(Node) * 5, "test_intrusive_basics nodes", memory_alignment::align8);
	oFinally { default_deallocate(values); };
	concurrent_stack<Node> s;
	oCHECK(s.empty(), "Stack should be empty (init)");

	for (int i = 0; i < 5; i++)
	{
		values[i].value = i;
		s.push(&values[i]);
	}

 	oCHECK(s.size() == 5, "Stack size is not correct");

	Node* v = s.peek();
	oCHECK(v && v->value == 4, "Stack value is not correct (peek)");

	v = s.pop();
	oCHECK(v && v->value == 4, "Stack value is not correct (pop 1)");

	v = s.pop();
	oCHECK(v && v->value == 3, "Stack value is not correct (pop 2)");

	for (size_t i = 2; i < 5; i++)
		s.pop();

	oCHECK(s.empty(), "Stack should be empty (after pops)");

	// reinitialize
	for (int i = 0; i < 5; i++)
	{
		values[i].value = i;
		s.push(&values[i]);
	}

	v = s.pop_all();

	size_t i = 5;
	while (v)
	{
		oCHECK(v->value == --i, "Stack value is not correct (pop_all)");
		v = v->next;
	}
}

static void test_intrusive_concurrency(unit_test::services& services)
{
	std::vector<char> buf(sizeof(Node) * 40);
	Node* nodes = (Node*)buf.data();
	memset(nodes, 0xaa, sizeof(nodes));

	concurrent_stack<Node> s;

	ouro::parallel_for(0, 40, [&](size_t _Index)
	{
		nodes[_Index].value = _Index;
		s.push(&nodes[_Index]);
	});

	Node* n = s.peek();
	while (n)
	{
		n->value = 0xc001c0de;
		n = n->next;
	}

	for (int i = 0; i < 40; i++)
	{
		oCHECK(nodes[i].value != 0xaaaaaaa, "Node %d was never processed by task system", i);
		oCHECK(nodes[i].value == 0xc001c0de, "Node %d was never inserted into stack", i);
	}
	
	ouro::parallel_for(0, 40, [&](size_t _Index)
	{
		Node* popped = s.pop();
		popped->value = 0xdeaddead;
	});
	
	oCHECK(s.empty(), "Stack should be empty");

	for (int i = 0; i < 40; i++)
		oCHECK(nodes[i].value == 0xdeaddead, "Node %d was not popped correctly", i);
}

oTEST(oConcurrency_concurrent_stack)
{
	test_intrusive_basics(srv);
	test_intrusive_concurrency(srv);
}
