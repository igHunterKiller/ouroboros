// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#include <oSystem/windows/win_crt_heap.h>
#include <oSystem/process_heap.h>
#include <crtdbg.h>

// _____________________________________________________________________________
// Copy-paste from $(VSInstallDir)crt\src\dbgint.h, to avoid including CRT 
// source code
#define nNoMansLandSize 4
typedef struct _CrtMemBlockHeader
{
	struct _CrtMemBlockHeader * pBlockHeaderNext;
	struct _CrtMemBlockHeader * pBlockHeaderPrev;
	char *                      szFileName;
	int                         nLine;
	#ifdef _WIN64
		/* These items are reversed on Win64 to eliminate gaps in the struct
		* and ensure that sizeof(struct)%16 == 0, so 16-byte alignment is
		* maintained in the debug heap.
		*/
		int                         nBlockUse;
		size_t                      nDataSize;
	#else  /* _WIN64 */
		size_t                      nDataSize;
		int                         nBlockUse;
	#endif  /* _WIN64 */
	long                        lRequest;
	unsigned char               gap[nNoMansLandSize];
	/* followed by:
	*  unsigned char           data[nDataSize];
	*  unsigned char           anotherGap[nNoMansLandSize];
	*/
} _CrtMemBlockHeader;

#define pHdr(pbData) (((_CrtMemBlockHeader *)pbData)-1)
// _____________________________________________________________________________

namespace ouro {
	namespace windows {
		namespace crt_heap {

_CrtMemBlockHeader* get_head()
{
	// New blocks are added to the head of the list
	void* p = malloc(1);
	_CrtMemBlockHeader* hdr = pHdr(p);
	free(p);
	return hdr;
}

bool is_valid(void* _Pointer)
{
	return !!_CrtIsValidHeapPointer(_Pointer);
}

void* get_pointer(struct _CrtMemBlockHeader* _pMemBlockHeader)
{
	return (void*)(_pMemBlockHeader+1);
}

void* next_pointer(void* _Pointer)
{
	return get_pointer(pHdr(_Pointer)->pBlockHeaderNext);
}

size_t size(void* _Pointer)
{
	return _Pointer ? pHdr(_Pointer)->nDataSize : 0;
}

bool is_free(void* _Pointer)
{
	return pHdr(_Pointer)->nBlockUse == _FREE_BLOCK;
}

bool is_normal(void* _Pointer)
{
	return pHdr(_Pointer)->nBlockUse == _NORMAL_BLOCK;
}

bool is_crt(void* _Pointer)
{
	return pHdr(_Pointer)->nBlockUse == _CRT_BLOCK;
}

bool is_ignore(void* _Pointer)
{
	return pHdr(_Pointer)->nBlockUse == _IGNORE_BLOCK;
}

bool is_client(void* _Pointer)
{
	return pHdr(_Pointer)->nBlockUse == _CLIENT_BLOCK;
}

const char* allocation_filename(void* _Pointer)
{
	return pHdr(_Pointer)->szFileName;
}

unsigned int allocation_line(void* _Pointer)
{
	return static_cast<unsigned int>(pHdr(_Pointer)->nLine);
}

unsigned int allocation_id(void* _Pointer)
{
	return static_cast<unsigned int>(pHdr(_Pointer)->lRequest);
}

void break_on_allocation(uintptr_t _AllocationID)
{
	_CrtSetBreakAlloc((long)_AllocationID);
}

void enable_at_exit_leak_report(bool _Enable)
{
	int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	_CrtSetDbgFlag(_Enable ? (flags | _CRTDBG_LEAK_CHECK_DF) : (flags &~ _CRTDBG_LEAK_CHECK_DF));
}

bool enable_memory_tracking(bool _Enable)
{
	int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	_CrtSetDbgFlag(_Enable ? (flags | _CRTDBG_ALLOC_MEM_DF) : (flags &~ _CRTDBG_ALLOC_MEM_DF));
	return (flags & _CRTDBG_ALLOC_MEM_DF) == _CRTDBG_ALLOC_MEM_DF;
}
		} // namespace crt_heap
	}
}
