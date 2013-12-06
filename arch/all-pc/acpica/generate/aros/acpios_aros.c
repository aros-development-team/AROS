/*
 * Copyright (C) 2012, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */
#define DEBUG 1
#include <aros/debug.h>

#include <hardware/efi/config.h>

#include <proto/exec.h>
#include <proto/timer.h>
#include <proto/efi.h>
#include <proto/kernel.h>

#include <asm/io.h>

#include <devices/timer.h>

#include "acpi.h"
#include "accommon.h"
#include "amlcode.h"
#include "acparser.h"
#include "acdebug.h"
#include "acmacros.h"

#define _COMPONENT          ACPI_OS_SERVICES
        ACPI_MODULE_NAME    ("osarosxf")

struct ACPICABase {
    struct Library ab_Lib;
    struct MsgPort *ab_TimeMsgPort;
    struct timerequest *ab_TimeRequest;
    struct Library *ab_TimerBase;

    ACPI_MCFG_ALLOCATION *ab_PCI;
    int ab_PCIs;

    ACPI_PHYSICAL_ADDRESS ab_RootPointer;
};

#if 1 /* Use a global. Icky */
struct ACPICABase *Global_ACPICABase;

#define NEED_ACPICABASE         struct ACPICABase *ACPICABase = Global_ACPICABase;
#define THIS_ACPICABASE(base)   Global_ACPICABase = base
#endif

ACPI_STATUS AcpiOsInitialize (void)
{
    NEED_ACPICABASE

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
    NEED_ACPICABASE

    DeleteIORequest(ACPICABase->ab_TimeRequest);
    DeleteMsgPort(ACPICABase->ab_TimeMsgPort);

    return AE_OK;
}

ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer(void)
{
    NEED_ACPICABASE

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
    NEED_ACPICABASE

    ACPICABase->ab_TimeRequest->tr_node.io_Command = TR_ADDREQUEST;
    ACPICABase->ab_TimeRequest->tr_time.tv_secs = Milliseconds / 1000;
    ACPICABase->ab_TimeRequest->tr_time.tv_micro = (Milliseconds % 1000) * 1000;
    DoIO((struct IORequest *)ACPICABase->ab_TimeRequest);
}

void AcpiOsStall(UINT32 Microseconds)
{
    NEED_ACPICABASE

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

ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *OutHandle)
{
    return AcpiOsCreateSemaphore (1, 1, OutHandle);
}

void AcpiOsDeleteLock(ACPI_SPINLOCK Handle)
{
    AcpiOsDeleteSemaphore (Handle);
}

ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle)
{
    AcpiOsWaitSemaphore (Handle, 1, 0xFFFF);
    return AE_OK;
}

void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags)
{
    AcpiOsSignalSemaphore (Handle, 1);
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
    UINT8 *ecam;

    NEED_ACPICABASE

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
    UINT8 *ecam;

    NEED_ACPICABASE

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
    struct Library *TimerBase;
    struct timeval tv;

    NEED_ACPICABASE

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

    if ((KernelBase = OpenResource("kernel.resource"))) {
        struct TagItem *cmdline = LibFindTagItem(KRN_CmdLine, KrnGetBootInfo());

        if (cmdline && strcasestr((char *)cmdline->ti_Data, "noacpi")) {
            D(bug("[ACPI] Disabled from command line\n"));
            return FALSE;
        }
    }

    THIS_ACPICABASE(ACPICABase);

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
