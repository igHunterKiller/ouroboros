// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/bit.h>
#include <oCore/countof.h>
#include <oSystem/cpu.h>
#include <oSystem/windows/win_error.h>
#include <oSystem/windows/win_version.h>

namespace ouro {

template<> const char* as_string<ouro::cpu::type>(const ouro::cpu::type& type)
{
	switch (type)
	{
		case ouro::cpu::type::unknown: return "unknown";
		case ouro::cpu::type::x86: return "x86";
		case ouro::cpu::type::x64: return "x64";
		case ouro::cpu::type::ia64: return "ia64";
		case ouro::cpu::type::arm: return "arm";
		default: break;
	}
	return "?";
}

template<> const char* as_string<ouro::cpu::support>(const ouro::cpu::support& support)
{
	switch (support)
	{
		case ouro::cpu::support::none: return "none";
		case ouro::cpu::support::not_found: return "not found";
		case ouro::cpu::support::hardware_only: return "hardware only";
		case ouro::cpu::support::full: return "full";
		default: break;
	}
	return "?";
}

namespace cpu { namespace detail {

struct cpu_feature
{
	const char* name;
	int index;
	int check_bit;
	windows::version min_version;
};

static const cpu_feature sFeatures[] = 
{
	{ "X87FPU",           3, 0,  windows::version::unknown  },
	{ "Hyperthreading",   3, 28, windows::version::unknown  },
	{ "8ByteAtomicSwap",  3, 8,  windows::version::unknown  },
	{ "SSE1",             2, 25, windows::version::unknown  },
	{ "SSE2",             3, 26, windows::version::unknown  },
	{ "SSE3",             2, 0,  windows::version::unknown  },
	{ "SSE4.1",           2, 19, windows::version::unknown  },
	{ "SSE4.2",           2, 20, windows::version::unknown  },
	{ "XSAVE/XSTOR",      2, 26, windows::version::win7_sp1 },
	{ "OSXSAVE",          2, 27, windows::version::win7_sp1 },
	{ "AVX1",            -1, 0,  windows::version::win7_sp1 }, // AVX detection is a special-case
};

static void get_cpu_string(char dst[16])
{
	int CPUInfo[4];
	__cpuid(CPUInfo, 0);
	*((int*) dst)    = CPUInfo[1];
	*((int*)(dst+4)) = CPUInfo[3];
	*((int*)(dst+8)) = CPUInfo[2];
	dst[12] = '\0';
}

static void get_cpu_brand_string(char dst[64])
{
	int CPUInfo[4];
	__cpuid(CPUInfo, 0x80000002);
	memcpy(dst, CPUInfo, sizeof(CPUInfo));
	__cpuid(CPUInfo, 0x80000003);
	memcpy(dst + sizeof(CPUInfo), CPUInfo, sizeof(CPUInfo));
	__cpuid(CPUInfo, 0x80000004);
	memcpy(dst + 2*sizeof(CPUInfo), CPUInfo, sizeof(CPUInfo));
	clean_whitespace(dst, 64, dst);
}

	} // namespace detail

info_t get_info()
{
	DWORD size_ex = 0;
	GetLogicalProcessorInformationEx(RelationNumaNode, 0, &size_ex);
	oAssert(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "");
	
	SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* lpi_ex = static_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(alloca(size_ex));
	memset(lpi_ex, 0xff, size_ex);
	oVB(GetLogicalProcessorInformationEx(RelationNumaNode, lpi_ex, &size_ex));

	// Skip to the requested CPU index
	static const int _CPUIndex = 0;
	size_t offset_ex = 0;
	for (int cpu = 0; cpu < _CPUIndex; cpu++)
	{
		offset_ex += lpi_ex->Size;
		oCheck(offset_ex < size_ex, std::errc::protocol_error, "");
		lpi_ex = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)((uint8_t*)lpi_ex + lpi_ex->Size);
	}

	info_t cpu_info;
	memset(&cpu_info, 0, sizeof(cpu_info));

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	switch (si.wProcessorArchitecture)
	{
		case PROCESSOR_ARCHITECTURE_AMD64: cpu_info.type = type::x64; break;
		case PROCESSOR_ARCHITECTURE_IA64: cpu_info.type = type::ia64; break;
		case PROCESSOR_ARCHITECTURE_INTEL: cpu_info.type = type::x86; break;
		default: cpu_info.type = type::unknown; break;
	}

	DWORD size = 0;
	GetLogicalProcessorInformation(0, &size);
	oAssert(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "");
	SYSTEM_LOGICAL_PROCESSOR_INFORMATION* lpi = static_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION*>(alloca(size));
	memset(lpi, 0xff, size);
	oVB(GetLogicalProcessorInformation(lpi, &size));

	const size_t numInfos = size / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);

	for (size_t i = 0; i < numInfos; i++)
	{
		if ((lpi[i].ProcessorMask & lpi_ex->NumaNode.GroupMask.Mask) != 0)
		{
			switch (lpi[i].Relationship)
			{
				case RelationProcessorCore:
				cpu_info.processor_count++;
				cpu_info.hardware_thread_count += bitcount(lpi[i].ProcessorMask);
				break;

				case RelationCache:
				{
					size_t cacheLevel = lpi[i].Cache.Level - 1;
					cache_info_t& c = lpi[i].Cache.Type == CacheData ? cpu_info.data_cache[cacheLevel] : cpu_info.instruction_cache[cacheLevel];
					c.size = lpi[i].Cache.Size;
					c.line_size = lpi[i].Cache.LineSize;
					c.associativity = lpi[i].Cache.Associativity;
				}
				break;

				case RelationProcessorPackage:
				cpu_info.processor_package_count++;
				break;

				default:
				break;
			}
		}
	}

	if (cpu_info.type == type::x86 || cpu_info.type == type::x64)
	{
		// http://msdn.microsoft.com/en-us/library/hskdteyh(VS.80).aspx

		detail::get_cpu_string(cpu_info.string);
		detail::get_cpu_brand_string(cpu_info.brand_string);
	}

	return cpu_info;
}

static const char* feature_name(int feature_index)
{
	return (feature_index < 0 || feature_index >= countof(detail::sFeatures)) ? nullptr : detail::sFeatures[feature_index].name;
}

static support check(const char* feature)
{
	int CPUInfo[4];
	__cpuid(CPUInfo, 1);

	int i = 0;
	const char* s = feature_name(i);
	while (s)
	{
		if (!strcmp(feature, s))
		{
			const detail::cpu_feature& f = detail::sFeatures[i];
			windows::version ver = windows::get_version();

			if (f.index == -1)
			{
				// special cases
				if (!strcmp(feature, "AVX1"))
				{
					// http://insufficientlycomplicated.wordpress.com/2011/11/07/detecting-intel-advanced-vector-extensions-avx-in-visual-studio/
					bool HasXSAVE_XSTOR = CPUInfo[2] & (1<<27) || false;
					bool HasAVX = CPUInfo[2] & (1<<28) || false;
					
					if (ver >= f.min_version)
					{
						if (HasXSAVE_XSTOR && HasAVX)
						{
							// Check if the OS will save the YMM registers
							unsigned long long xcrFeatureMask = _xgetbv(_XCR_XFEATURE_ENABLED_MASK);
							return ((xcrFeatureMask & 0x6) || false) ? support::full : support::hardware_only;
						}

						else
							return support::hardware_only;
					}

					else
						return support::hardware_only;
				}

				break;
			}

			else
			{
				if ((CPUInfo[f.index] & (1<<f.check_bit)) == 0)
					return support::none;

				if (ver < f.min_version)
					return support::hardware_only;

				return support::full;
			}
		}

		s = feature_name(++i);
	}

	return support::not_found;
}

void enumerate_features(enumerator_fn enumerator, void* user)
{
	if (!enumerator)
		return;
	int i = 0;
	const char* f = nullptr;
	while (nullptr != (f = feature_name(++i)))
	{
		support sup = check(f);
		enumerator(f, sup, user);
	}
}

}}
