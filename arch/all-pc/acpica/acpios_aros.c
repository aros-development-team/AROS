/*
 * Copyright (C) 2012-2018, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef __ACPICA_NOLIBBASE__
#define __ACPICA_NOLIBBASE__
#endif /* !__ACPICA_NOLIBBASE__ */

#include <aros/debug.h>

#include "acpica_intern.h"

#include <hardware/efi/config.h>

#include <proto/exec.h>
#include <proto/timer.h>
#include <proto/efi.h>
#include <proto/kernel.h>

#include <proto/acpica.h>

#include <asm/io.h>
#include <exec/resident.h>
#include <devices/timer.h>

#define _COMPONENT          ACPI_OS_SERVICES
        ACPI_MODULE_NAME    ("osarosxf")

#define DLOCK(a)

/* FIXME: __aros_getbase_ACPICABase() for internal use should be handled
   properly by genmodule
*/
#undef __aros_getbase_ACPICABase
struct Library *__aros_getbase_ACPICABase(void);

ACPI_STATUS AcpiOsInitialize (void)
{
    struct ACPICABase *ACPICABase = (struct ACPICABase *)__aros_getbase_ACPICABase();

    D(bug("[ACPI] %s: ACPICABase=0x%p\n", __func__, ACPICABase));

    if ((ACPICABase->ab_TimeMsgPort = CreateMsgPort())) {
        D(bug("[ACPI] %s: MsgPort @ %p\n", __func__, ACPICABase->ab_TimeMsgPort));
        if ((ACPICABase->ab_TimeRequest = CreateIORequest(ACPICABase->ab_TimeMsgPort, sizeof(*ACPICABase->ab_TimeRequest)))) {
            D(bug("[ACPI] %s: TimeRequest @ %p\n", __func__, ACPICABase->ab_TimeRequest));

			if (ACPICABase->ab_Flags & ACPICAF_TIMER)
				if (0 == OpenDevice(TIMERNAME, UNIT_MICROHZ, (struct IORequest *)ACPICABase->ab_TimeRequest, 0))	
				{
					ACPICABase->ab_TimerBase = (struct Library *)ACPICABase->ab_TimeRequest->tr_node.io_Device;
				}

            return AE_OK;
        }
        DeleteMsgPort(ACPICABase->ab_TimeMsgPort);
    }

    D(bug("[ACPI] %s: Failed\n", __func__));

    return AE_NO_MEMORY;
}

ACPI_STATUS AcpiOsTerminate (void)
{
    struct ACPICABase *ACPICABase = (struct ACPICABase *)__aros_getbase_ACPICABase();

    D(bug("[ACPI] %s: ACPICABase=0x%p\n", __func__, ACPICABase));

	if (ACPICABase->ab_TimeRequest->tr_node.io_Device)
	{
		ACPICABase->ab_TimerBase = NULL;
		CloseDevice((struct IORequest *)ACPICABase->ab_TimeRequest);
	}

    DeleteIORequest(ACPICABase->ab_TimeRequest);
    DeleteMsgPort(ACPICABase->ab_TimeMsgPort);

    return AE_OK;
}

ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer(void)
{
    struct ACPICABase *ACPICABase = (struct ACPICABase *)__aros_getbase_ACPICABase();

    D(bug("[ACPI] %s: ACPICABase=0x%p\n", __func__, ACPICABase));

    if (ACPICABase->ab_RootPointer == 0) {
        struct Library *EFIBase = OpenResource("efi.resource");
        if (EFIBase) {
            const uuid_t acpi_20_guid = ACPI_20_TABLE_GUID;
            const uuid_t acpi_10_guid = ACPI_TABLE_GUID;
            ACPICABase->ab_RootPointer = (ACPI_PHYSICAL_ADDRESS)EFI_FindConfigTable(&acpi_20_guid);

            /* No ACPI 2.0 table? */
            if (ACPICABase->ab_RootPointer == 0) {
                ACPICABase->ab_RootPointer = (ACPI_PHYSICAL_ADDRESS)EFI_FindConfigTable(&acpi_10_guid);
            }
        }
    }

    /* Nope, no EFI available... Scan the ROM area
     */
    if (ACPICABase->ab_RootPointer == 0) {
        AcpiFindRootPointer(&ACPICABase->ab_RootPointer);
    }

    return ACPICABase->ab_RootPointer;
}

ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *PredefinedObject, ACPI_STRING *NewValue)
{
    *NewValue = NULL;
    return AE_OK;
}

ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_TABLE_HEADER **NewTable)
{
    *NewTable = NULL;
    return AE_OK;
}

ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_PHYSICAL_ADDRESS *NewAddress, UINT32 *NewTableLength)
{
    *NewAddress = 0;
    return AE_OK;
}


void *AcpiOsMapMemory (ACPI_PHYSICAL_ADDRESS PhysicalAddress, ACPI_SIZE Length)
{
    return (void *)PhysicalAddress;
}

void AcpiOsUnmapMemory(void *LogicalAddress, ACPI_SIZE Length)
{
    return;
}

ACPI_STATUS AcpiOsGetPhysicalAddress(void *LogicalAddress, ACPI_PHYSICAL_ADDRESS *PhysicalAddress)
{
    *PhysicalAddress = (IPTR)LogicalAddress;
    return AE_OK;
}

void *AcpiOsAllocate(ACPI_SIZE Size)
{
    D(bug("[ACPI] %s(%d)\n", __func__, Size));
    return AllocVec(Size, MEMF_PUBLIC);
}

void *AcpiOsAllocateZeroed(ACPI_SIZE Size)
{
    D(bug("[ACPI] %s(%d)\n", __func__, Size));
    return AllocVec(Size, MEMF_PUBLIC|MEMF_CLEAR);
}

void AcpiOsFree(void *Memory)
{
    D(bug("[ACPI] %s(0x%p)\n", __func__, Memory));
    FreeVec(Memory);
}

BOOLEAN AcpiOsReadable(void *Memory, ACPI_SIZE Length)
{
    return TRUE;
}

BOOLEAN AcpiOsWritable(void *Memory, ACPI_SIZE Length)
{
    /* First 4K page is not writable on any AROS architecture */
    return ((IPTR)Memory < 4096) ? FALSE : TRUE;
}

ACPI_THREAD_ID AcpiOsGetThreadId(void)
{
    ACPI_THREAD_ID tid;

    DLOCK(bug("[ACPI] %s()\n", __func__));

    tid = (ACPI_THREAD_ID)(ACPI_PHYSICAL_ADDRESS)FindTask(NULL);

    /* If we are running during kernel bring-up, return
     * TID 1
     */
    if (tid == 0)
        tid = 1;

    return tid;
}

ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type, ACPI_OSD_EXEC_CALLBACK Function, void *Context)
{
    /* TODO: Create a thread */
    bug("[ACPI] %s: FIXME!\n", __func__);

    return AE_NOT_IMPLEMENTED;
}

void AcpiOsSleep(UINT64 Milliseconds)
{
    struct ACPICABase *ACPICABase = (struct ACPICABase *)__aros_getbase_ACPICABase();

    D(bug("[ACPI] %s: ACPICABase=0x%p\n", __func__, ACPICABase));

    ACPICABase->ab_TimeRequest->tr_node.io_Command = TR_ADDREQUEST;
    ACPICABase->ab_TimeRequest->tr_time.tv_secs = Milliseconds / 1000;
    ACPICABase->ab_TimeRequest->tr_time.tv_micro = (Milliseconds % 1000) * 1000;
    DoIO((struct IORequest *)ACPICABase->ab_TimeRequest);
}

void AcpiOsStall(UINT32 Microseconds)
{
    struct ACPICABase *ACPICABase = (struct ACPICABase *)__aros_getbase_ACPICABase();

    D(bug("[ACPI] %s: ACPICABase=0x%p\n", __func__, ACPICABase));

    ACPICABase->ab_TimeRequest->tr_node.io_Command = TR_ADDREQUEST;
    ACPICABase->ab_TimeRequest->tr_time.tv_secs = Microseconds / 1000000;
    ACPICABase->ab_TimeRequest->tr_time.tv_micro = (Microseconds % 1000000);
    DoIO((struct IORequest *)ACPICABase->ab_TimeRequest);
}

void AcpiOsWaitEventsComplete(void)
{
    bug("[ACPI] %s: FIXME!\n", __func__);
}

ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits, UINT32 InitialUnits, ACPI_SEMAPHORE *OutHandle)
{
    struct SignalSemaphore *Handle;

    DLOCK(bug("[ACPI] %s()\n", __func__);)

    Handle = ACPI_ALLOCATE(sizeof(*Handle));
    if (Handle) {
        InitSemaphore(Handle);
        *OutHandle = Handle;
        return AE_OK;
    }
    return AE_NO_MEMORY;
}

ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle)
{
    DLOCK(bug("[ACPI] %s()\n", __func__);)

    ACPI_FREE(Handle);
    return AE_OK;
}

ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units, UINT16 Timeout)
{
    DLOCK(bug("[ACPI] %s()\n", __func__);)

    if (Timeout == ACPI_DO_NOT_WAIT) {
        if (!AttemptSemaphore(Handle))
            return AE_TIME;
    }
    else
    {
        /* Forever.. */
        ObtainSemaphore(Handle);
    }

    return AE_OK;
}

ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units)
{
    DLOCK(bug("[ACPI] %s()\n", __func__);)

    ReleaseSemaphore(Handle);
    return AE_OK;
}

/* FIXME: Use SpinLock primitives once they exist in kernel.resource! */
#define MIN_PRI -128
struct SpinLock {
    volatile ULONG sl_Lock;
};

static inline struct SpinLock *CreateSpin(VOID)
{
    DLOCK(bug("[ACPI] %s()\n", __func__));

    return AllocVec(sizeof(struct SpinLock), MEMF_ANY | MEMF_CLEAR);
}

static inline void DeleteSpin(struct SpinLock *sl)
{
    DLOCK(bug("[ACPI] %s()\n", __func__);)

    Disable();
    while (sl->sl_Lock > 0) {
        Enable();
        sl->sl_Lock--;
    }
    Enable();

    FreeVec(sl);
}

static inline VOID LockSpin(struct SpinLock *sl)
{
    BYTE pri, pri_lower;
    struct Task *task = FindTask(NULL);

    DLOCK(bug("[ACPI] %s()\n", __func__);)

    pri = task->tc_Node.ln_Pri;
    pri_lower = pri;

    do {
        Disable();
        if (sl->sl_Lock == 0) {
            sl->sl_Lock++;
            if (pri_lower != pri)
                SetTaskPri(task, pri);
            break;
        }
        Enable();
        if (pri_lower > MIN_PRI)
            pri_lower--;
        SetTaskPri(task, pri_lower);
    } while (1);
}

static inline void UnlockSpin(struct SpinLock *sl)
{
    DLOCK(bug("[ACPI] %s()\n", __func__));

    sl->sl_Lock--;
    Enable();
}

ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *OutHandle)
{
    DLOCK(bug("[ACPI] %s()\n", __func__));

    *OutHandle = CreateSpin();

    return (*OutHandle == NULL) ? AE_NO_MEMORY : AE_OK;
}

void AcpiOsDeleteLock(ACPI_SPINLOCK Handle)
{
    DeleteSpin(Handle);
}

ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle)
{
    DLOCK(bug("[ACPI] %s()\n", __func__));

    LockSpin(Handle);
    return 1;
}

void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags)
{
    DLOCK(bug("[ACPI] %s()\n", __func__));

    if (Flags == 1)
        UnlockSpin(Handle);
}

struct AcpiOsInt {
    struct Interrupt ai_Interrupt;
    ACPI_OSD_HANDLER ai_Handler;
    void *ai_Context;
};

static AROS_INTH1(AcpiOsIntServer, struct AcpiOsInt *, ai)
{
    AROS_INTFUNC_INIT

    UINT32 ret;

    D(bug("[ACPI] %s()\n", __func__));

    ret = ai->ai_Handler(ai->ai_Context);

    return (ret == ACPI_INTERRUPT_HANDLED) ? TRUE : FALSE;

    AROS_INTFUNC_EXIT
}

ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 InterruptLevel, ACPI_OSD_HANDLER Handler, void *Context)
{
    struct AcpiOsInt *ai;

    D(bug("[ACPI] %s()\n", __func__));

    if ((ai = ACPI_ALLOCATE(sizeof(*ai)))) {
        ai->ai_Interrupt.is_Node.ln_Name = "ACPI";
        ai->ai_Interrupt.is_Code = (APTR)AcpiOsIntServer;
        ai->ai_Interrupt.is_Data = (APTR)ai;
        ai->ai_Handler = Handler;
        ai->ai_Context = Context;
        AddIntServer(INTB_KERNEL + InterruptLevel, &ai->ai_Interrupt);
        return AE_OK;
    }

    return AE_NO_MEMORY;
}

ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER Handler)
{
    bug("[ACPI] %s: FIXME! (InterruptLevel=%d)\n", __func__, InterruptNumber);

    return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 *Value, UINT32 Width)
{
    switch (Width) {
    case  8: *Value = *(UINT8 *)Address; break; 
    case 16: *Value = *(UINT16 *)Address; break; 
    case 32: *Value = *(UINT32 *)Address; break; 
    case 64: *Value = *(UINT64 *)Address; break; 
    default: *Value = ~0; break;
    }

    return AE_OK;
}

ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 Value, UINT32 Width)
{
    switch (Width) {
    case  8: *(UINT8 *)Address = (UINT8)Value; break; 
    case 16: *(UINT16 *)Address = (UINT16)Value; break; 
    case 32: *(UINT32 *)Address = (UINT32)Value; break; 
    case 64: *(UINT64 *)Address = (UINT64)Value; break; 
    default: break;
    }

    return AE_OK;
}

ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32 *Value, UINT32 Width)
{
    switch (Width) {
    case  8: *Value = inb(Address); break;
    case 16: *Value = inw(Address); break;
    case 32: *Value = inl(Address); break;
    default: *Value = ~0; break;
    }
    return AE_OK;
}

ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width)
{
    switch (Width) {
    case  8: outb(Value,Address); break;
    case 16: outw(Value,Address); break;
    case 32: outl(Value,Address); break;
    default: break;
    }
    return AE_OK;
}

static UINT8 *find_pci(struct ACPICABase *ACPICABase, ACPI_PCI_ID *PciId)
{
    int i;

    D(bug("[ACPI] %s()\n", __func__));

    for (i = 0; i < ACPICABase->ab_PCIs; i++) {
        ACPI_MCFG_ALLOCATION *ma = &ACPICABase->ab_PCI[i];
        if (PciId->Segment != ma->PciSegment)
            continue;
        if (PciId->Bus < ma->StartBusNumber ||
            PciId->Bus > ma->EndBusNumber)
            continue;

        return (UINT8 *)(ACPI_PHYSICAL_ADDRESS)ma->Address;
    }

    return NULL;
}

ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register, UINT64 *Value, UINT32 Width)
{
    struct ACPICABase *ACPICABase = (struct ACPICABase *)__aros_getbase_ACPICABase();
    UINT8 *ecam;

    D(bug("[ACPI] %s: ACPICABase=0x%p\n", __func__, ACPICABase));

    if ((ecam = find_pci(ACPICABase, PciId))) {
        UINT32 offset = (PciId->Bus << 20) | (PciId->Device << 15) | (PciId->Function << 12) | Register;
        switch (Width) {
        case  8: *Value = *(volatile UINT8 *)(ecam + offset); break;
        case 16: *Value = *(volatile UINT16 *)(ecam + offset); break;
        case 32: *Value = *(volatile UINT32 *)(ecam + offset); break;
        case 64: *Value = *(volatile UINT64 *)(ecam + offset); break;
        default: *Value = 0; break;
        }

        return AE_OK;
    }

    return AE_NOT_FOUND;
}

ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register, UINT64 Value, UINT32 Width)
{
    struct ACPICABase *ACPICABase = (struct ACPICABase *)__aros_getbase_ACPICABase();
    UINT8 *ecam;

    D(bug("[ACPI] %s: ACPICABase=0x%p\n", __func__, ACPICABase));

    if ((ecam = find_pci(ACPICABase, PciId))) {
        UINT32 offset = (PciId->Bus << 20) | (PciId->Device << 15) | (PciId->Function << 12) | Register;
        switch (Width) {
        case  8: *(volatile UINT8 *)(ecam + offset) = Value & 0xff; break;
        case 16: *(volatile UINT16 *)(ecam + offset) = Value & 0xffff; break;
        case 32: *(volatile UINT32 *)(ecam + offset) = Value & 0xffffffff; break;
        case 64: *(volatile UINT64 *)(ecam + offset) = Value; break;
        default: break;
        }

        return AE_OK;
    }

    return AE_NOT_FOUND;
}

void AcpiOsPrintf(const char *Fmt, ...)
{
    va_list Args;

    va_start (Args, Fmt);
    AcpiOsVprintf (Fmt, Args);
    va_end (Args);
}

void AcpiOsVprintf(const char *Format, va_list Args)
{
    vkprintf(Format, Args);
}

/* Return current time in 100ns units
 */
UINT64 AcpiOsGetTimer(void)
{
    struct ACPICABase *ACPICABase = (struct ACPICABase *)__aros_getbase_ACPICABase();
    struct Library *TimerBase;
    struct timeval tv;
    UINT64 retVal = 0;

    D(bug("[ACPI] %s: ACPICABase=0x%p\n", __func__, ACPICABase));

    if ((TimerBase = ACPICABase->ab_TimerBase))
    {
        D(bug("[ACPI] %s: TimerBase=0x%p\n", __func__, TimerBase));

        GetSysTime(&tv);

        D(bug("[ACPI] %s: GetSysTime returned\n", __func__));
        retVal = (tv.tv_secs*1000000ULL + tv.tv_micro)*10;
    }
    return retVal;
}

ACPI_STATUS AcpiOsSignal(UINT32 Function, void *Info)
{
    bug("FIXME: %s\n", __func__);
    return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS AcpiOsGetLine(char *Buffer, UINT32 BufferLength, UINT32 *BytesRead)
{
    bug("[ACPI] %s: FIXME!\n", __func__);

    return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS
AcpiOsEnterSleep (
    UINT8                   SleepState,
    UINT32                  RegaValue,
    UINT32                  RegbValue)
{
    D(bug("[ACPI] %s()\n", __func__));

    return (AE_OK);
}

/*
 * AROS Custom Code
 */
UINT8 AcpiGetInfoFlags(ACPI_DEVICE_INFO *DevInfo)
{
    return DevInfo->Flags;
}

UINT8 *AcpiGetInfoLowDstates(ACPI_DEVICE_INFO *DevInfo)
{
    if (DevInfo->Valid & ACPI_VALID_SXWS)
        return DevInfo->LowestDstates;
    return NULL;
}

UINT8 *AcpiGetInfoHighDstates(ACPI_DEVICE_INFO *DevInfo)
{
    if (DevInfo->Valid & ACPI_VALID_SXDS)
        return DevInfo->HighestDstates;
    return NULL;
}

UINT64 AcpiGetInfoAddress(ACPI_DEVICE_INFO *DevInfo)
{
    if (DevInfo->Valid & ACPI_VALID_ADR)
        return DevInfo->Address;
    return 0;
}

ACPI_PNP_DEVICE_ID *AcpiGetInfoHardwareId(ACPI_DEVICE_INFO *DevInfo)
{
    if (DevInfo->Valid & ACPI_VALID_HID)
        return &DevInfo->HardwareId;
    return NULL;
}

ACPI_PNP_DEVICE_ID *AcpiGetInfoUniqueId(ACPI_DEVICE_INFO *DevInfo)
{
    if (DevInfo->Valid & ACPI_VALID_UID)
        return &DevInfo->UniqueId;
    return NULL;
}

ACPI_PNP_DEVICE_ID *AcpiGetInfoClassCode(ACPI_DEVICE_INFO *DevInfo)
{
    ACPI_PNP_DEVICE_ID *classCode = NULL;
#if defined(ACPI_VALID_CLS)
    if (DevInfo->Valid & ACPI_VALID_CLS)
        classCode = &DevInfo->ClassCode;
#endif
    return classCode;
}

ACPI_PNP_DEVICE_ID_LIST *AcpiGetInfoCompatIdList(ACPI_DEVICE_INFO *DevInfo)
{
    if (DevInfo->Valid & ACPI_VALID_CID)
        return &DevInfo->CompatibleIdList;
    return NULL;
}

LONG AcpiScanTables(const char *Signature, const struct Hook *Hook, APTR UserData)
{
    int i;
    LONG count;

    D(bug("[ACPI] %s()\n", __func__));

    for (count = 0, i = 1; ; i++) {
        ACPI_STATUS err;
        ACPI_TABLE_HEADER *hdr;
        IPTR ok;

        err = AcpiGetTable((ACPI_STRING)Signature, i, &hdr);
        if (err != AE_OK)
            break;

        if (Hook) {
            ok = CALLHOOKPKT((struct Hook *)Hook, hdr, UserData);
        } else {
            ok = TRUE;
        }
        if (ok)
            count++;
    }

    return count;
}

#define ACPI_MAX_INIT_TABLES 64

static int ACPICA_InitTask(struct ACPICABase *ACPICABase)
{
    ACPI_STATUS err;
    const UINT8 initlevel = ACPI_FULL_INITIALIZATION;

    D(bug("[ACPI] %s: Starting Full initialization...\n", __func__));

    err = AcpiInitializeSubsystem();
    if (ACPI_FAILURE(err)) {
        D(bug("[ACPI] %s: AcpiInitializeSubsystem returned error %d\n", __func__, err));
        return FALSE;
    }

    D(bug("[ACPI] %s: Subsystem Initialized\n", __func__));

    err = AcpiLoadTables();
    if (ACPI_FAILURE(err)) {
        D(bug("[ACPI] %s: AcpiLoadTables returned error %d\n", __func__, err));
        return FALSE;
    }

    D(bug("[ACPI] %s: Tables Initialized\n", __func__));
    
    err = AcpiEnableSubsystem(initlevel);
    if (ACPI_FAILURE(err)) {
        D(bug("[ACPI] %s: AcpiEnableSubsystem(0x%02x) returned error %d\n", __func__, initlevel, err));
        return FALSE;
    }

    D(bug("[ACPI] %s: Subsystem Enabled\n", __func__));

    err = AcpiInitializeObjects(initlevel);
    if (ACPI_FAILURE(err)) {
        D(bug("[ACPI] %s: AcpiInitializeObjects(0x%02x) returned error %d\n", __func__, initlevel, err));
        return FALSE;
    }

    D(bug("[ACPI] %s: Full initialization complete\n", __func__));

    return TRUE;
}

int ACPICA_init(struct ACPICABase *ACPICABase)
{
    ACPI_TABLE_MCFG *mcfg;
    ACPI_STATUS err;
    struct Library *KernelBase;

    D(bug("[ACPI] %s: ACPICABase=0x%p\n", __func__, ACPICABase));

    if ((KernelBase = OpenResource("kernel.resource"))) {
        struct TagItem *cmdline = LibFindTagItem(KRN_CmdLine, KrnGetBootInfo());

        if (cmdline && strcasestr((char *)cmdline->ti_Data, "noacpi")) {
            D(bug("[ACPI] %s: Disabled from command line\n", __func__));
            return FALSE;
        }
    }
    ACPICABase->ab_Flags |= ACPICAF_ENABLED;

    AcpiDbgLevel = ~0;

    ACPICABase->ab_RootPointer = 0;

    err = AcpiInitializeTables(NULL, ACPI_MAX_INIT_TABLES, TRUE);
    if (ACPI_FAILURE(err)) {
        D(bug("[ACPI] %s: AcpiInitializeTables returned error %d\n", __func__, err));
        return FALSE;
    }

    if (AcpiGetTable("MCFG", 1, (ACPI_TABLE_HEADER **)&mcfg) == AE_OK) {
        ACPICABase->ab_PCIs = (mcfg->Header.Length - sizeof(*mcfg)) / sizeof(ACPI_MCFG_ALLOCATION);
        ACPICABase->ab_PCI = (ACPI_MCFG_ALLOCATION *)&mcfg[1];
    } else {
        ACPICABase->ab_PCIs = 0;
    }

    return TRUE;
}
ADD2INITLIB(ACPICA_init,0)

int ACPICA_expunge(struct ACPICABase *ACPICABase)
{
    D(bug("[ACPI] %s()\n", __func__));

    if ((ACPICABase->ab_Flags & ACPICAF_ENABLED) != 0)
        AcpiTerminate();

    return TRUE;
}
ADD2EXPUNGELIB(ACPICA_expunge, 0)

/******************* ACPICA Post Exec/Kernel Setup ******************/

extern void acpicapost_end(void);

static AROS_UFP3 (APTR, ACPICAPost,
		  AROS_UFPA(struct Library *, lh, D0),
		  AROS_UFPA(BPTR, segList, A0),
		  AROS_UFPA(struct ExecBase *, sysBase, A6));

static const TEXT acpicapost_namestring[] = "acpica.post";
static const TEXT acpicapost_versionstring[] = "acpica.post 1.0\n";

const struct Resident acpicapost_romtag =
{
   RTC_MATCHWORD,
   (struct Resident *)&acpicapost_romtag,
   (APTR)&acpicapost_end,
   RTF_COLDSTART,
   1,
   NT_UNKNOWN,
   119,
   (STRPTR)acpicapost_namestring,
   (STRPTR)acpicapost_versionstring,
   (APTR)ACPICAPost
};

extern struct syscallx86_Handler x86_SCRebootHandler;
extern struct syscallx86_Handler x86_SCChangePMStateHandler;

static AROS_UFH3 (APTR, ACPICAPost,
		  AROS_UFHA(struct Library *, lh, D0),
		  AROS_UFHA(BPTR, segList, A0),
		  AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT

    struct ACPICABase *ACPICABase;

    D(bug("[ACPI] %s()\n", __func__));

    /* If ACPICA isn't available, don't run */
    ACPICABase = (struct ACPICABase *)OpenLibrary("acpica.library", 0);
    if (!ACPICABase) {
        D(bug("[ACPI] %s(): Can't open acpica.library. Quitting\n", __func__));
        return NULL;
    }

    /* Start up the late initialization thread at the highest priority */
    if (NewCreateTask(TASKTAG_PC, ACPICA_InitTask, TASKTAG_NAME, "ACPICA_InitTask",
        TASKTAG_PRI, 127, TASKTAG_ARG1, ACPICABase, TAG_DONE) == NULL) {
        bug("[ACPI] %s: Failed to start ACPI init task\n", __func__);
    }

    D(bug("[ACPI] %s: Finished\n", __func__));

    AROS_USERFUNC_EXIT

    return NULL;
}

void acpicapost_end(void) { };

/******************* Timer Setup - Runs after timer.device is initialised ******************/

extern void acpicatimer_end(void);

static AROS_UFP3 (APTR, ACPICATimerSetup,
		  AROS_UFPA(struct Library *, lh, D0),
		  AROS_UFPA(BPTR, segList, A0),
		  AROS_UFPA(struct ExecBase *, sysBase, A6));

static const TEXT acpicatimer_namestring[] = "acpica.timer";
static const TEXT acpicatimer_versionstring[] = "acpica.timer 1.0\n";

const struct Resident acpicatimer_romtag =
{
   RTC_MATCHWORD,
   (struct Resident *)&acpicatimer_romtag,
   (APTR)&acpicatimer_end,
   RTF_COLDSTART,
   1,
   NT_UNKNOWN,
   49,
   (STRPTR)acpicatimer_namestring,
   (STRPTR)acpicatimer_versionstring,
   (APTR)ACPICATimerSetup
};

extern struct syscallx86_Handler x86_SCRebootHandler;
extern struct syscallx86_Handler x86_SCChangePMStateHandler;

static AROS_UFH3 (APTR, ACPICATimerSetup,
		  AROS_UFHA(struct Library *, lh, D0),
		  AROS_UFHA(BPTR, segList, A0),
		  AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT

    struct ACPICABase *ACPICABase;

    D(bug("[ACPI] %s()\n", __func__));

    /* If ACPICA isn't available, don't run */
    ACPICABase = (struct ACPICABase *)OpenLibrary("acpica.library", 0);
    if (!ACPICABase) {
        D(bug("[ACPI] %s(): Can't open acpica.library. Quitting\n", __func__));
        return NULL;
    }

	if (0 == OpenDevice(TIMERNAME, UNIT_MICROHZ, (struct IORequest *)ACPICABase->ab_TimeRequest, 0))	
	{
		ACPICABase->ab_Flags |= ACPICAF_TIMER;
		ACPICABase->ab_TimerBase = (struct Library *)ACPICABase->ab_TimeRequest->tr_node.io_Device;
	}
    D(bug("[ACPI] %s: Finished\n", __func__));

    AROS_USERFUNC_EXIT

    return NULL;
}

void acpicatimer_end(void) { };
