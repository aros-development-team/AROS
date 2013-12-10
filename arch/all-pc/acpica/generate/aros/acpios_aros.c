/*
 * Copyright (C) 2012, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */
#define __ACPICA_NOLIBBASE__
#define DEBUG 1
#include <aros/debug.h>

#include "acpica_intern.h"

#include <hardware/efi/config.h>

#include <proto/exec.h>
#include <proto/timer.h>
#include <proto/efi.h>
#include <proto/kernel.h>

#include <proto/acpica.h>

#include <asm/io.h>

#include <devices/timer.h>

#define _COMPONENT          ACPI_OS_SERVICES
        ACPI_MODULE_NAME    ("osarosxf")

/* FIXME: __aros_getbase_ACPICABase() for internal use should be handled
   properly by genmodule
*/
#undef __aros_getbase_ACPICABase
struct Library *__aros_getbase_ACPICABase(void);

ACPI_STATUS AcpiOsInitialize (void)
{
    struct ACPICABase *ACPICABase = (struct ACPICABase *)__aros_getbase_ACPICABase();

    D(bug("[ACPI]AcpiOsInitialize(): ACPICABase=0x%x\n", ACPICABase));

    if ((ACPICABase->ab_TimeMsgPort = CreateMsgPort())) {
        if ((ACPICABase->ab_TimeRequest = CreateIORequest(ACPICABase->ab_TimeMsgPort, sizeof(*ACPICABase->ab_TimeRequest)))) {
            ACPICABase->ab_TimerBase = (struct Library *)ACPICABase->ab_TimeRequest->tr_node.io_Device;
            return AE_OK;
        }
        DeleteMsgPort(ACPICABase->ab_TimeMsgPort);
    }

    return AE_NO_MEMORY;
}

ACPI_STATUS AcpiOsTerminate (void)
{
    struct ACPICABase *ACPICABase = (struct ACPICABase *)__aros_getbase_ACPICABase();

    D(bug("[ACPI]AcpiOsTerminate(): ACPICABase=0x%x\n", ACPICABase));

    DeleteIORequest(ACPICABase->ab_TimeRequest);
    DeleteMsgPort(ACPICABase->ab_TimeMsgPort);

    return AE_OK;
}

ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer(void)
{
    struct ACPICABase *ACPICABase = (struct ACPICABase *)__aros_getbase_ACPICABase();

    D(bug("[ACPI]AcpiOsGetRootPointer(): ACPICABase=0x%x\n", ACPICABase));

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
    return AllocVec(Size, MEMF_PUBLIC);
}

void AcpiOsFree(void *Memory)
{
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
    bug("FIXME: %s\n", __func__);
    return AE_NOT_IMPLEMENTED;
}

void AcpiOsSleep(UINT64 Milliseconds)
{
    struct ACPICABase *ACPICABase = (struct ACPICABase *)__aros_getbase_ACPICABase();

    D(bug("[ACPI]AcpiOsSleep(): ACPICABase=0x%x\n", ACPICABase));

    ACPICABase->ab_TimeRequest->tr_node.io_Command = TR_ADDREQUEST;
    ACPICABase->ab_TimeRequest->tr_time.tv_secs = Milliseconds / 1000;
    ACPICABase->ab_TimeRequest->tr_time.tv_micro = (Milliseconds % 1000) * 1000;
    DoIO((struct IORequest *)ACPICABase->ab_TimeRequest);
}

void AcpiOsStall(UINT32 Microseconds)
{
    struct ACPICABase *ACPICABase = (struct ACPICABase *)__aros_getbase_ACPICABase();

    D(bug("[ACPI]AcpiOsStall(): ACPICABase=0x%x\n", ACPICABase));

    ACPICABase->ab_TimeRequest->tr_node.io_Command = TR_ADDREQUEST;
    ACPICABase->ab_TimeRequest->tr_time.tv_secs = Microseconds / 1000000;
    ACPICABase->ab_TimeRequest->tr_time.tv_micro = (Microseconds % 1000000);
    DoIO((struct IORequest *)ACPICABase->ab_TimeRequest);
}

void AcpiOsWaitEventsComplete(void)
{
    bug("FIXME: %s\n", __func__);
}

ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits, UINT32 InitialUnits, ACPI_SEMAPHORE *OutHandle)
{
    struct SignalSemaphore *Handle;
    
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
    ACPI_FREE(Handle);
    return AE_OK;
}

ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units, UINT16 Timeout)
{
    if (Timeout != 0xffff)
        bug("FIXME: %s, Timeout=0x%04x\n", __func__, Timeout);

    if (Timeout == 0) {
        if (AttemptSemaphore(Handle)) {
            return AE_OK;
        } else {
            return AE_TIME;
        }
    }

    /* Forever.. */
    ObtainSemaphore(Handle);

    return AE_OK;
}

ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units)
{
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
    return AllocVec(sizeof(struct SpinLock), MEMF_ANY | MEMF_CLEAR);
}

static inline void DeleteSpin(struct SpinLock *sl)
{
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
    sl->sl_Lock--;
    Enable();
}

ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *OutHandle)
{
    *OutHandle = CreateSpin();

    return (*OutHandle == NULL) ? AE_NO_MEMORY : AE_OK;
}

void AcpiOsDeleteLock(ACPI_SPINLOCK Handle)
{
    DeleteSpin(Handle);
}

ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle)
{
    LockSpin(Handle);
    return 1;
}

void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags)
{
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

    ret = ai->ai_Handler(ai->ai_Context);

    return (ret == ACPI_INTERRUPT_HANDLED) ? TRUE : FALSE;

    AROS_INTFUNC_EXIT
}

ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 InterruptLevel, ACPI_OSD_HANDLER Handler, void *Context)
{
    struct AcpiOsInt *ai;

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
    bug("FIXME: %s (InterruptLevel=%d)\n", __func__, InterruptNumber);
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

    D(bug("[ACPI]AcpiOsReadPciConfiguration(): ACPICABase=0x%x\n", ACPICABase));

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

    D(bug("[ACPI]AcpiOsWritePciConfiguration(): ACPICABase=0x%x\n", ACPICABase));

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

    D(bug("[ACPI]AcpiOsGetTimer(): ACPICABase=0x%x\n", ACPICABase));

    TimerBase = ACPICABase->ab_TimerBase;

    GetSysTime(&tv);

    return (tv.tv_secs*1000000ULL + tv.tv_micro)*10;
}

ACPI_STATUS AcpiOsSignal(UINT32 Function, void *Info)
{
    bug("FIXME: %s\n", __func__);
    return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS AcpiOsGetLine(char *Buffer, UINT32 BufferLength, UINT32 *BytesRead)
{
    bug("FIXME: %s\n", __func__);
    return AE_NOT_IMPLEMENTED;
}

/*
 * AROS Custom Code
 */
LONG AcpiScanTables(const char *Signature, const struct Hook *Hook, APTR UserData)
{
    int i;
    LONG count;

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

    err = AcpiInitializeSubsystem();
    if (ACPI_FAILURE(err)) {
        D(bug("%s: AcpiInitializeSubsystem() = %d\n", __func__, err));
        return FALSE;
    }

    err = AcpiLoadTables();
    if (ACPI_FAILURE(err)) {
        D(bug("%s: AcpiLoadTables() = %d\n", __func__, err));
        return FALSE;
    }

    err = AcpiEnableSubsystem(initlevel);
    if (ACPI_FAILURE(err)) {
        D(bug("%s: AcpiEnableSubsystem(0x%02x) = %d\n", __func__, initlevel, err));
        return FALSE;
    }

    err = AcpiInitializeObjects(initlevel);
    if (ACPI_FAILURE(err)) {
        D(bug("%s: AcpiInitializeObjects(0x%02x) = %d\n", __func__, initlevel, err));
        return FALSE;
    }

    D(bug("[ACPI] Full initialization complete\n"));

    return TRUE;
}

int ACPICA_init(struct ACPICABase *ACPICABase)
{
    ACPI_TABLE_MCFG *mcfg;
    ACPI_STATUS err;
    struct Library *KernelBase;

    D(bug("[ACPI]ACPICA_init(ACPICABase=0x%x)\n", ACPICABase));

    if ((KernelBase = OpenResource("kernel.resource"))) {
        struct TagItem *cmdline = LibFindTagItem(KRN_CmdLine, KrnGetBootInfo());

        if (cmdline && strcasestr((char *)cmdline->ti_Data, "noacpi")) {
            D(bug("[ACPI] Disabled from command line\n"));
            return FALSE;
        }
    }

    AcpiDbgLevel = ~0;

    ACPICABase->ab_RootPointer = 0;

    err = AcpiInitializeTables(NULL, ACPI_MAX_INIT_TABLES, TRUE);
    if (ACPI_FAILURE(err)) {
        D(bug("%s: AcpiInitializeTables() = %d\n", __func__, err));
        return FALSE;
    }

    if (AcpiGetTable("MCFG", 1, (ACPI_TABLE_HEADER **)&mcfg) == AE_OK) {
        ACPICABase->ab_PCIs = (mcfg->Header.Length - sizeof(*mcfg)) / sizeof(ACPI_MCFG_ALLOCATION);
        ACPICABase->ab_PCI = (ACPI_MCFG_ALLOCATION *)&mcfg[1];
    } else {
        ACPICABase->ab_PCIs = 0;
    }

    /* Everything else is in the late initialization thread,
     * which will start at the highest priority once mulitasking begins
     */
    if (NewCreateTask(TASKTAG_PC, ACPICA_InitTask, TASKTAG_NAME, "ACPICA_InitTask", TASKTAG_PRI, 127, TASKTAG_ARG1, ACPICABase, TAG_DONE) == NULL) {
        AcpiTerminate();
        return FALSE;
    }

    return TRUE;
}
ADD2INITLIB(ACPICA_init,0)

int ACPICA_expunge(struct ACPICABase *ACPICABase)
{
    AcpiTerminate();

    return TRUE;
}
ADD2EXPUNGELIB(ACPICA_expunge, 0)
