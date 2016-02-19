// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#include <oSystem/windows/win_skeleton.h>
#include <oConcurrency/mutex.h>
#include <oSystem/process_heap.h>

namespace ouro {
	namespace windows {
		namespace skeleton {
			
class context
{
public:
	static context& singleton();

	void register_source(handle _hSkeleton, const std::function<void(bone_info* _pSkeleton)>& _Get)
	{
		lock_guard<shared_mutex> lock(Mutex);
		if (Sources.find(_hSkeleton) != Sources.end())
			throw std::exception("redundant registration");
		Sources[_hSkeleton] = _Get;
	}

	void unregister_source(handle _hSkeleton)
	{
		lock_guard<shared_mutex> lock(Mutex);
		auto it = Sources.find(_hSkeleton);
		if (it != Sources.end())
			Sources.erase(it);
	}

	bool get_info(handle _hSkeleton, bone_info* _pSkeleton)
	{
		shared_lock<shared_mutex> lock(Mutex);
		auto it = Sources.find(_hSkeleton);
		if (it == Sources.end())
			oThrow(std::errc::invalid_argument, "");
		it->second(_pSkeleton);
		return true;
	}

private:
	shared_mutex Mutex;
	std::map<handle
		, std::function<void(bone_info* _pSkeleton)>
		, std::less<handle>
		, process_heap::std_allocator<std::pair<handle, std::function<void(bone_info* _pSkeleton)>>>>
	Sources;

	context() {}
	~context()
	{
		oAssert(Sources.empty(), "");
	}
};

oDEFINE_PROCESS_SINGLETON("ouro::windows::win_skeleton", context);

void register_source(handle _handle, const std::function<void(bone_info* _pSkeleton)>& _GetSkeleton) { context::singleton().register_source(_handle, _GetSkeleton); }
void unregister_source(handle _handle) { context::singleton().unregister_source(_handle); }
bool get_info(handle _handle, bone_info* _pSkeleton) { return context::singleton().get_info(_handle, _pSkeleton); }

		} // namespace skeleton
	}
}
