#pragma once

// ntstatus.h exists I know but then it throws billion
// redefinition warnings
#define STATUS_UNSUCCESSFUL              ((NTSTATUS)0xC0000001L)
#define STATUS_INSUFFICIENT_RESOURCES    ((NTSTATUS)0xC000009AL)
#define STATUS_INVALID_PARAMETER_1       ((NTSTATUS)0xC00000EFL)
#define STATUS_INVALID_PARAMETER_2       ((NTSTATUS)0xC00000F0L)
#define STATUS_INVALID_PARAMETER_3       ((NTSTATUS)0xC00000F1L)
#define STATUS_NOT_FOUND                 ((NTSTATUS)0xC0000225L)
#define STATUS_NOT_SUPPORTED             ((NTSTATUS)0xC00000BBL)

extern BOOL kernelCallbackCalled;
extern PVOID driverBuffer;
extern PVOID kernelBase;

extern NTSTATUS mapStatus;
extern NTSTATUS driverStatus;

typedef enum _POOL_TYPE
{
	NonPagedPool,
	NonPagedPoolExecute = NonPagedPool,
	PagedPool,
	NonPagedPoolMustSucceed = NonPagedPool + 2,
	DontUseThisType,
	NonPagedPoolCacheAligned = NonPagedPool + 4,
	PagedPoolCacheAligned,
	NonPagedPoolCacheAlignedMustS = NonPagedPool + 6,
	MaxPoolType,
	NonPagedPoolBase = 0,
	NonPagedPoolBaseMustSucceed = NonPagedPoolBase + 2,
	NonPagedPoolBaseCacheAligned = NonPagedPoolBase + 4,
	NonPagedPoolBaseCacheAlignedMustS = NonPagedPoolBase + 6,
	NonPagedPoolSession = 32,
	PagedPoolSession = NonPagedPoolSession + 1,
	NonPagedPoolMustSucceedSession = PagedPoolSession + 1,
	DontUseThisTypeSession = NonPagedPoolMustSucceedSession + 1,
	NonPagedPoolCacheAlignedSession = DontUseThisTypeSession + 1,
	PagedPoolCacheAlignedSession = NonPagedPoolCacheAlignedSession + 1,
	NonPagedPoolCacheAlignedMustSSession = PagedPoolCacheAlignedSession + 1,
	NonPagedPoolNx = 512,
	NonPagedPoolNxCacheAligned = NonPagedPoolNx + 4,
	NonPagedPoolSessionNx = NonPagedPoolNx + 32,
} POOL_TYPE;

typedef PVOID(*ExAllocatePool_t)(POOL_TYPE type, SIZE_T numberOfBytes);

typedef NTSTATUS(NTAPI DRIVER_INITIALIZE)(void* dummy1, void* dummy2);
typedef DRIVER_INITIALIZE* PDRIVER_INITIALIZE;

int CustomCompare(const char* a, const char* b);
DWORD64 ResolveExport(PVOID imageBase, const char* functionName);
void KernelCallback(void* first, void* second);