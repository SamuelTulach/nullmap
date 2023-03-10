#pragma once

typedef struct _OBJECT_TYPE_INFORMATION
{
    UNICODE_STRING TypeName;
    ULONG TotalNumberOfObjects;
    ULONG TotalNumberOfHandles;
    ULONG TotalPagedPoolUsage;
    ULONG TotalNonPagedPoolUsage;
    ULONG TotalNamePoolUsage;
    ULONG TotalHandleTableUsage;
    ULONG HighWaterNumberOfObjects;
    ULONG HighWaterNumberOfHandles;
    ULONG HighWaterPagedPoolUsage;
    ULONG HighWaterNonPagedPoolUsage;
    ULONG HighWaterNamePoolUsage;
    ULONG HighWaterHandleTableUsage;
    ULONG InvalidAttributes;
    GENERIC_MAPPING GenericMapping;
    ULONG ValidAccessMask;
    BOOLEAN SecurityRequired;
    BOOLEAN MaintainHandleCount;
    BOOLEAN TypeIndex;
    CHAR ReservedByte;
    ULONG PoolType;
    ULONG DefaultPagedPoolCharge;
    ULONG DefaultNonPagedPoolCharge;
} OBJECT_TYPE_INFORMATION, * POBJECT_TYPE_INFORMATION;

typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO
{
    unsigned short UniqueProcessId;
    unsigned short CreatorBackTraceIndex;
    unsigned char ObjectTypeIndex;
    unsigned char HandleAttributes;
    unsigned short HandleValue;
    void* Object;
    unsigned long GrantedAccess;
    long __PADDING__[1];
} SYSTEM_HANDLE_TABLE_ENTRY_INFO, * PSYSTEM_HANDLE_TABLE_ENTRY_INFO;

typedef struct _SYSTEM_HANDLE_INFORMATION
{
    unsigned long NumberOfHandles;
    struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO Handles[1];
} SYSTEM_HANDLE_INFORMATION, * PSYSTEM_HANDLE_INFORMATION;

typedef struct _DISPATCHER_HEADER
{
    union
    {
        volatile long Lock;
        long LockNV;
        struct
        {
            unsigned char Type;
            unsigned char Signalling;
            unsigned char Size;
            unsigned char Reserved1;
        };
        struct
        {
            unsigned char TimerType;
            union
            {
                unsigned char TimerControlFlags;
                struct
                {
                    struct
                    {
                        unsigned char Absolute : 1;
                        unsigned char Wake : 1;
                        unsigned char EncodedTolerableDelay : 6;
                    };
                    unsigned char Hand;
                    union
                    {
                        unsigned char TimerMiscFlags;
                        struct
                        {
                            unsigned char Index : 6;
                            unsigned char Inserted : 1;
                            volatile unsigned char Expired : 1;
                        };
                    };
                };
            };
        };
        struct
        {
            unsigned char Timer2Type;
            union
            {
                unsigned char Timer2Flags;
                struct
                {
                    struct
                    {
                        unsigned char Timer2Inserted : 1;
                        unsigned char Timer2Expiring : 1;
                        unsigned char Timer2CancelPending : 1;
                        unsigned char Timer2SetPending : 1;
                        unsigned char Timer2Running : 1;
                        unsigned char Timer2Disabled : 1;
                        unsigned char Timer2ReservedFlags : 2;
                    };
                    unsigned char Timer2ComponentId;
                    unsigned char Timer2RelativeId;
                };
            };
        };
        struct
        {
            unsigned char QueueType;
            union
            {
                unsigned char QueueControlFlags;
                struct
                {
                    struct
                    {
                        unsigned char Abandoned : 1;
                        unsigned char DisableIncrement : 1;
                        unsigned char QueueReservedControlFlags : 6;
                    };
                    unsigned char QueueSize;
                    unsigned char QueueReserved;
                };
            };
        };
        struct
        {
            unsigned char ThreadType;
            unsigned char ThreadReserved;
            union
            {
                unsigned char ThreadControlFlags;
                struct
                {
                    struct
                    {
                        unsigned char CycleProfiling : 1;
                        unsigned char CounterProfiling : 1;
                        unsigned char GroupScheduling : 1;
                        unsigned char AffinitySet : 1;
                        unsigned char Tagged : 1;
                        unsigned char EnergyProfiling : 1;
                        unsigned char SchedulerAssist : 1;
                        unsigned char ThreadReservedControlFlags : 1;
                    };
                    union
                    {
                        unsigned char DebugActive;
                        struct
                        {
                            unsigned char ActiveDR7 : 1;
                            unsigned char Instrumented : 1;
                            unsigned char Minimal : 1;
                            unsigned char Reserved4 : 2;
                            unsigned char AltSyscall : 1;
                            unsigned char Emulation : 1;
                            unsigned char Reserved5 : 1;
                        };
                    };
                };
            };
        };
        struct
        {
            unsigned char MutantType;
            unsigned char MutantSize;
            unsigned char DpcActive;
            unsigned char MutantReserved;
        };
    };
    long SignalState;
    LIST_ENTRY WaitListHead;
} DISPATCHER_HEADER, * PDISPATCHER_HEADER;

typedef struct _KEVENT
{
    struct _DISPATCHER_HEADER Header;
} KEVENT, * PKEVENT;

typedef struct _NT_IORING_CREATE_FLAGS
{
    enum _NT_IORING_CREATE_REQUIRED_FLAGS Required;
    enum _NT_IORING_CREATE_ADVISORY_FLAGS Advisory;
} NT_IORING_CREATE_FLAGS, * PNT_IORING_CREATE_FLAGS;

typedef struct _NT_IORING_INFO
{
    enum IORING_VERSION IoRingVersion;
    struct _NT_IORING_CREATE_FLAGS Flags;
    unsigned int SubmissionQueueSize;
    unsigned int SubmissionQueueRingMask;
    unsigned int CompletionQueueSize;
    unsigned int CompletionQueueRingMask;
    struct _NT_IORING_SUBMISSION_QUEUE* SubmissionQueue;
    struct _NT_IORING_COMPLETION_QUEUE* CompletionQueue;
} NT_IORING_INFO, * PNT_IORING_INFO;

typedef struct _IOP_MC_BUFFER_ENTRY
{
    USHORT Type;
    USHORT Reserved;
    ULONG Size;
    ULONG ReferenceCount;
    ULONG Flags;
    LIST_ENTRY GlobalDataLink;
    PVOID Address;
    ULONG Length;
    CHAR AccessMode;
    ULONG MdlRef;
    struct _MDL* Mdl;
    KEVENT MdlRundownEvent;
    PULONG64 PfnArray;
    BYTE PageNodes[0x20];
} IOP_MC_BUFFER_ENTRY, * PIOP_MC_BUFFER_ENTRY;

typedef struct _IORING_OBJECT
{
    short Type;
    short Size;
    struct _NT_IORING_INFO UserInfo;
    void* Section;
    struct _NT_IORING_SUBMISSION_QUEUE* SubmissionQueue;
    struct _MDL* CompletionQueueMdl;
    struct _NT_IORING_COMPLETION_QUEUE* CompletionQueue;
    unsigned __int64 ViewSize;
    long InSubmit;
    unsigned __int64 CompletionLock;
    unsigned __int64 SubmitCount;
    unsigned __int64 CompletionCount;
    unsigned __int64 CompletionWaitUntil;
    struct _KEVENT CompletionEvent;
    unsigned char SignalCompletionEvent;
    struct _KEVENT* CompletionUserEvent;
    unsigned int RegBuffersCount;
    struct _IOP_MC_BUFFER_ENTRY** RegBuffers;
    unsigned int RegFilesCount;
    void** RegFiles;
} IORING_OBJECT, * PIORING_OBJECT;

typedef struct _HIORING
{
    HANDLE handle;
    NT_IORING_INFO Info;
    ULONG IoRingKernelAcceptedVersion;
    PVOID RegBufferArray;
    ULONG BufferArraySize;
    PVOID Unknown;
    ULONG FileHandlesCount;
    ULONG SubQueueHead;
    ULONG SubQueueTail;
}_HIORING;

BOOL IoRingSetup(PIORING_OBJECT* ioRingObjectPointer);
BOOL IoRingRead(PULONG64 registerBuffers, ULONG64 readAddress, PVOID readBuffer, ULONG readLength);
BOOL IoRingWrite(PULONG64 registerBuffers, ULONG64 writeAddress, PVOID writeBuffer, ULONG writeLength);
BOOL IoRingReadHelper(ULONG64 readAddress, PVOID readBuffer, ULONG readLength);
BOOL IoRingWriteHelper(ULONG64 writeAddress, PVOID writeBuffer, ULONG writeLength);
BOOL IoRingRundown();