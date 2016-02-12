// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// unit test framework implementation

#pragma once
#include <oSystem/process_stats_monitor.h>
#include <oSystem/golden_image.h>
#include <oBase/unit_test.h>
#include <oString/path.h>

class ouro_unit_test_framework : public ouro::unit_test::framework_services
{
public:
	void check_system_requirements(const ouro::unit_test::info_t& info) override;
	void pre_iterations(const ouro::unit_test::info_t& info) override;
	void seed_rand(uint32_t seed) override;
	void pre_test(const ouro::unit_test::info_t& info, const char* test_name) override;
	void post_test(const ouro::unit_test::info_t& info) override;
	bool has_memory_leaks(const ouro::unit_test::info_t& info) override;
	void vtrace(const char* format, va_list args) override;
	void vprintf(const ouro::unit_test::print_type& type, const char* format, va_list args) override;
	void vstatus(const char* format, va_list args) override;
	const char* status() const override;
	const char* root_path() const override;
	int rand() override;
	bool is_debugger_attached() const override;
	bool is_remote_session() const override;
	size_t total_physical_memory() const override;
	void get_cpu_utilization(float* out_avg, float* out_peek) override;
	void reset_cpu_utilization() override;
	ouro::blob load_buffer(const char* path) override;
	void check(const ouro::surface::image& img, int nth = 0, float max_rms_error = -1.0f) override;

private:
	char status_[2048];
	ouro::path_t root_;
	ouro::process_stats_monitor psm_;
	ouro::unit_test::info_t unit_test_info_;
	ouro::golden_image golden_image_;
	const char* test_name_;
};

// if filters is empty, this scans source control for the status of known ouroboros libs
// and filters out any that don't have modified files. To override this behavior, run
// the unit test with -i .*
void auto_filter_libs(std::vector<ouro::filter_chain::filter_t>& filters);
