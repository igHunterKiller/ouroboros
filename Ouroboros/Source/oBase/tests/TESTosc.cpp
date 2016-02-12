// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oBase/osc.h>
#include <oMemory/memory.h>
#include "test_struct.h"
#include <vector>

using namespace ouro;
using namespace ouro::osc;

static const char* type_tags() { return ",Tc3Tih[fff]iTTs2f1sicbsi[iiii]dIfbt"; }

oTEST(oBase_osc)
{
	// Set up test object and a reference object we'll test against after going
	// through the OSC serialization and deserialization.
	char b1[10];
	memset(b1, 21, sizeof(b1));
	char b2[20];
	memset(b2, 22, sizeof(b2));

	test_struct sent, received;
	memset4(&sent, 0xdeadc0de, sizeof(sent));
	memset4(&received, 0xdeadc0de, sizeof(received));
	init_test_struct(&sent, b1, sizeof(b1), b2, sizeof(b2));

	const char* _MessageName = "/TESTOSC/Run/TEST/sent";

	oCHECK(((void*)(&sent.c2 + 1) != (void*)(&sent.b1size)), "Expected padding");

	size_t TestSize = sizeof(test_struct);
	oCHECK(TestSize == calc_deserialized_struct_size(type_tags()), "calc_deserialized_struct_size failed to compute correct size");

	static const size_t kExpectedArgsSize = 216;

	size_t argsSize = calc_args_data_size(type_tags(), sent);
	oCHECK(argsSize == kExpectedArgsSize, "calc_args_data_size failed to compute correct size");

	size_t msgSize = calc_msg_size(_MessageName, type_tags(), argsSize);

	{
		std::vector<char> ScopedBuffer(msgSize);
		size_t SerializedSize = serialize_struct_to_msg(_MessageName, type_tags(), sent, ScopedBuffer.data(), ScopedBuffer.size());
		oCHECK(SerializedSize > 0, "Failed to serialize buffer");
		oCHECK(deserialize_msg_to_struct(ScopedBuffer.data(), &received), "Deserialization failed");

		// NOTE: Remember that received has string and blob pointers that point
		// DIRECTLY into the ScopedBuffer, so either the client code here needs to
		// copy those values to their own buffers, or received can only remain in
		// the scope of the ScopedBuffer.
		oCHECK(sent == received, "Sent and received buffers do not match");
	}
}
