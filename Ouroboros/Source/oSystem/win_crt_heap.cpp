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

namespace ouro { namespace windows { namespace crt_heap {

_CrtMemBlockHeader* get_head()
{
	// New blocks are added to the head of the list
	void* p = malloc(1);
	_CrtMemBlockHeader* hdr = pHdr(p);
	free(p);
	return hdr;
}

void*        get_pointer(struct _CrtMemBlockHeader* hdr) { return (void*)(hdr+1); }
bool         is_valid           (void* ptr) { return !!_CrtIsValidHeapPointer(ptr); }
void*        next_pointer       (void* ptr) { return get_pointer(pHdr(ptr)->pBlockHeaderNext); }
size_t       size               (void* ptr) { return ptr ? pHdr(ptr)->nDataSize : 0; }
bool         is_free            (void* ptr) { return pHdr(ptr)->nBlockUse == _FREE_BLOCK; }
bool         is_normal          (void* ptr) { return pHdr(ptr)->nBlockUse == _NORMAL_BLOCK; }
bool         is_crt             (void* ptr) { return pHdr(ptr)->nBlockUse == _CRT_BLOCK; }
bool         is_ignore          (void* ptr) { return pHdr(ptr)->nBlockUse == _IGNORE_BLOCK; }
bool         is_client          (void* ptr) { return pHdr(ptr)->nBlockUse == _CLIENT_BLOCK; }
const char*  allocation_filename(void* ptr) { return pHdr(ptr)->szFileName; }
unsigned int allocation_line    (void* ptr) { return static_cast<unsigned int>(pHdr(ptr)->nLine); }
unsigned int allocation_id      (void* ptr) { return static_cast<unsigned int>(pHdr(ptr)->lRequest); }

void break_on_allocation(unsigned int alloc_id)
{
	_CrtSetBreakAlloc((long)alloc_id);
}

void report_leaks_on_exit(bool enable)
{
	int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	_CrtSetDbgFlag(enable ? (flags | _CRTDBG_LEAK_CHECK_DF) : (flags &~ _CRTDBG_LEAK_CHECK_DF));
}

bool enable_memory_tracking(bool enable)
{
	int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	_CrtSetDbgFlag(enable ? (flags | _CRTDBG_ALLOC_MEM_DF) : (flags &~ _CRTDBG_ALLOC_MEM_DF));
	return (flags & _CRTDBG_ALLOC_MEM_DF) == _CRTDBG_ALLOC_MEM_DF;
}

}}}
