// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oSystem/filesystem.h>
#include <oSystem/filesystem_monitor.h>
#include <oConcurrency/event.h>
#include <oCore/timer.h>

using namespace ouro;

static const char* RelFolderToMonitor = "Test/";
static const char* RelTestFile = "Test/monitor_test.txt";

struct TESTMonitorEvents
{
	double LastEventTimestamp;

	event FileModified;
	event FileAdded;
	event FileRemoved;

	event NZFileAccessible;
	event ZFileAccessible;
};

struct monitor_ctx
{
	unit_test::services* srv;
	TESTMonitorEvents* events;
	double start_time;
};

void dir_monitor(filesystem::file_event event, const path_t& path, void* user)
{
	auto& ctx    = *(monitor_ctx*)user;
	auto& srv    = *ctx.srv;
	auto& events = *ctx.events;

	int timeMS = static_cast<int>((timer::now() - ctx.start_time) * 1000);
	switch (event)
	{
		case filesystem::file_event::added:
			srv.trace("file %s was added at time %dms from start", path.c_str(), timeMS);
			events.FileAdded.set();
			break;
		case filesystem::file_event::removed:
			srv.trace("file %s was removed at time %dms from start", path.c_str(), timeMS);
			events.FileRemoved.set();
			break;
		case filesystem::file_event::modified:
		{
			srv.trace("file %s was modified at time %dms from start", path.c_str(), timeMS);
			events.FileModified.set();
			break;
		}
		case filesystem::file_event::accessible:
		{
			srv.trace("file %s is accessible at time %dms from start", path.c_str(), timeMS);
			if (strstr(path, "-NZ.txt"))
				events.NZFileAccessible.set();
			else if (strstr(path, "-Z.txt"))
				events.ZFileAccessible.set();
			break;
		}
	}

	events.LastEventTimestamp = timer::now();
	srv.trace("Events->LastEventTimestamp = %f", events.LastEventTimestamp);
}

oTEST(oSystem_filesystem_monitor)
{
	auto Events = std::make_shared<TESTMonitorEvents>();

	auto root = filesystem::data_path();

	auto NonZeroFile = root / RelTestFile;
	NonZeroFile.replace_extension_with_suffix("-NZ.txt");

	auto ZeroFile = root / RelTestFile;
	ZeroFile.replace_extension_with_suffix("-Z.txt");

	auto TestFile = root / RelTestFile;

	filesystem::remove_filename(TestFile); // should fail, but try anyway in case it was left over from a previous canceled or failed run.
	filesystem::remove_filename(NonZeroFile);
	filesystem::remove_filename(ZeroFile);

	double startTime = timer::now();

	monitor_ctx ctx;
	ctx.srv = &srv;
	ctx.events = Events.get();
	ctx.start_time = startTime;

	// It is not normally a good practice to capture the events by reference, 
	// but for simplicity doing it here. counting on not getting further 
	// callbacks once the test is finished.
	filesystem::monitor::info fsmi;
	fsmi.accessibility_poll_rate_ms = 1000; // crank this up for testing since we're not doing anything else
	std::shared_ptr<filesystem::monitor> Monitor = filesystem::monitor::make(fsmi, dir_monitor, &ctx);

	auto FolderToMonitor = root / RelFolderToMonitor;
	Monitor->watch(FolderToMonitor, 65536, true);

	{
		// should generate a modified event, windows a little iffy on that though, 
		// may get one from this, or it may get delayed until the file is closed if 
		// it sits in a cache.
		char testData[64];
		filesystem::save(TestFile, testData, sizeof(testData), filesystem::save_option::binary_write);
	}

	filesystem::copy_file(TestFile, NonZeroFile, filesystem::copy_option::overwrite_if_exists);

	// write a 0-len file (should generate an added event)
	{
		filesystem::file_handle hFile = filesystem::open(TestFile, filesystem::open_option::binary_write);
		filesystem::close(hFile);
	}

	filesystem::copy_file(TestFile, ZeroFile, filesystem::copy_option::overwrite_if_exists);

	static const std::chrono::milliseconds kTimeout(fsmi.accessibility_poll_rate_ms * 3);

	oCHECK(Events->FileAdded.wait_for(kTimeout), "timed out waiting for the added event");
	oCHECK(Events->FileModified.wait_for(kTimeout), "timed out waiting for the modified event");

	oCHECK(Events->NZFileAccessible.wait_for(kTimeout), "timed out waiting for the non-zero-sized file accessible event");
	oCHECK(Events->ZFileAccessible.wait_for(kTimeout), "timed out waiting for the zero-sized file accessible event");

	filesystem::remove_filename(NonZeroFile);
	filesystem::remove_filename(ZeroFile);

	filesystem::remove_filename(TestFile); // should generate a removed event
	oCHECK(Events->FileRemoved.wait_for(kTimeout), "timed out waiting for the removed event");
}
