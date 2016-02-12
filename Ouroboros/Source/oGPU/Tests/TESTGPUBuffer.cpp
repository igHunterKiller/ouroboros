// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oCore/algorithm.h>
#include <oCore/countof.h>
#include <oGPU/gpu.h>
#include "gpu_pipeline_state.h"

using namespace ouro;
using namespace ouro::gpu;

oTEST(oGPU_buffer)
{
	static int GPU_BufferAppendIndices[20] = { 5, 6, 7, 18764, 2452, 2423, 52354, 344, -1542, 3434, 53, -4535, 3535, 88884747, 534535, 88474, -445, 4428855, -1235, 4661};
	static const int num_entries = countof(GPU_BufferAppendIndices);
	
	device_init init("GPU Buffer test");
	init.enable_driver_reporting = true;
	auto dev = new_device(init);

	dev->new_pso("test buffer", get_pipeline_state_desc(tests::pipeline_state::buffer_test), tests::pipeline_state::buffer_test);

	auto append_buffer = dev->new_uav("append buffer", sizeof(int), num_entries, uav_extension::append);
	auto append_rb = dev->new_rb("append buffer rb", sizeof(int), num_entries);
	auto append_count = dev->new_rb("append buffer count rb", sizeof(int));

	auto cl = dev->immediate();
	cl->set_pso(tests::pipeline_state::buffer_test);

	uint32_t zero = 0;
	cl->set_rtvs(0, nullptr, nullptr, 0, nullptr, 0, 1, &append_buffer, &zero);

	cl->draw(num_entries);
	cl->copy_structure_count(append_count, 0, append_buffer);

	int count = 0;
	oCHECK(append_count->copy_to(&count), "counter copy-to failed");
	oCHECK(num_entries == count, "Append counter didn't reach %d", num_entries);

	cl->copy(append_rb, append_buffer->get_resource());

	std::vector<int> Values(20);
	append_rb->copy_to(Values.data());

	for(int i = 0; i < num_entries; i++)
		oCHECK(find_and_erase(Values, GPU_BufferAppendIndices[i]), "GPU Appended bad value");
}
