/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/asmcall.h>
#include <proto/acpica.h>
#include <proto/exec.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_globals.h"
#include "kernel_intern.h"

#include "acpi.h"

#define D(x)

#define ACPI_MODPRIO_PM       10

/************************************************************************************************
                                    ACPI PM RELATED FUNCTIONS
 ************************************************************************************************/

const char *ACPI_TABLE_FADT_STR __attribute__((weak)) = ACPI_SIG_FADT;

/* Deafult ACPI PM Syscall Handlers */

void ACPI_HandleChangePMStateSC(struct ExceptionContext *regs)
{
    struct KernelBase *KernelBase = getKernelBase();
    struct ACPIData *acpiData  = KernelBase->kb_PlatformData->kb_ACPI;

    UBYTE pmState =
#if (__WORDSIZE==64)
        (UBYTE)regs->rbx;
#else
        (UBYTE)regs->ebx;
#endif

    D(bug("[Kernel:ACPI-PM] %s(0x%02x)\n", __func__, pmState));

    if (pmState == 0xFF)
    {
        ACPI_GENERIC_ADDRESS tmpReg;
        ACPI_TABLE_FADT *fadt = (ACPI_TABLE_FADT *)acpiData->acpi_fadt;

        D(bug("[Kernel:ACPI-PM] %s: STATE 0xFF - Cold Rebooting...\n", __func__));
        D(bug("[Kernel:ACPI-PM] %s: FADT Reset Register @ 0x%p, Width %d, Offset %d, MemSpace %d\n", __func__, (IPTR)fadt->ResetRegister.Address, fadt->ResetRegister.BitWidth, fadt->ResetRegister.BitOffset, fadt->ResetRegister.SpaceId));
        D(bug("[Kernel:ACPI-PM] %s: FADT Reset Value = 0x%2x\n", __func__, fadt->ResetValue));

        AcpiWrite (fadt->ResetValue, &fadt->ResetRegister);
        
        // If we got here the reset didnt happen,
        // check if we are the known "faulty" implementation.
        
        if ((fadt->Header.Revision >= 2) &&
            (fadt->ResetRegister.Address == 0xCF9))
        {
            D(bug("[Kernel:ACPI-PM] %s: Failed Reset? Using workaround...\n", __func__));
            tmpReg.Address = 0x64;
            tmpReg.BitWidth = fadt->ResetRegister.BitWidth;
            tmpReg.BitOffset = fadt->ResetRegister.BitOffset;
            tmpReg.SpaceId = fadt->ResetRegister.SpaceId;

            AcpiWrite (0xFE, &tmpReg);
        }
        D(bug("[Kernel:ACPI-PM] %s: Reset Failed.\n", __func__));
    }
    else if (pmState == 0)
    {
        D(bug("[Kernel:ACPI-PM] %s: STATE 0x00 - Powering Off...\n", __func__));

        // TODO:

        while (1) asm volatile("hlt");
    }
    else
    {
        // We cant handle any other states atm =/
        D(bug("[Kernel:ACPI-PM] %s: UNHANDLED STATE 0x%02x\n", __func__, pmState));
    }
}

struct syscallx86_Handler ACPI_SCChangePMStateHandler =
{
    {
        .ln_Name = (APTR)SC_X86CHANGEPMSTATE
    },
    (APTR)ACPI_HandleChangePMStateSC
};

/*
 * Process the FADT Table
 */
AROS_UFH3(static IPTR, ACPI_hook_Table_PM_Probe,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(ACPI_TABLE_FADT *, fadt, A2),
	  AROS_UFHA(struct ACPI_TABLESCAN_DATA *, tsdata, A1))
{
    AROS_USERFUNC_INIT

    struct PlatformData *pdata = tsdata->acpits_UserData;

    D(bug("[Kernel:ACPI-IOAPIC] ## %s()\n", __func__));

    /* cache the FADT pointer ... */
    pdata->kb_ACPI->acpi_fadt = fadt;

    if (fadt->Flags & ACPI_FADT_RESET_REGISTER)
    {
        /* register the ACPI ChangePMState SysCall Handler .. */
        krnAddSysCallHandler(pdata, &ACPI_SCChangePMStateHandler, FALSE);
    }

    return TRUE;

    AROS_USERFUNC_EXIT
}


void ACPI_PM_SUPPORT(struct PlatformData *pdata)
{
    struct ACPI_TABLE_HOOK *scanHook;

    scanHook = (struct ACPI_TABLE_HOOK *)AllocMem(sizeof(struct ACPI_TABLE_HOOK), MEMF_CLEAR);
    if (scanHook)
    {
        D(bug("[Kernel:ACPI-PM] %s: Registering PM Table Parser...\n", __func__));
        D(bug("[Kernel:ACPI-PM] %s: Table Hook @ 0x%p\n", __func__, scanHook));
        scanHook->acpith_Node.ln_Name = (char *)ACPI_TABLE_FADT_STR;
        scanHook->acpith_Node.ln_Pri = ACPI_MODPRIO_PM;
        scanHook->acpith_Hook.h_Entry = (APTR)ACPI_hook_Table_PM_Probe;
        scanHook->acpith_HeaderLen = 0;
        scanHook->acpith_EntryType = 0;
        scanHook->acpith_UserData = pdata;
        Enqueue(&pdata->kb_ACPI->acpi_tablehooks, &scanHook->acpith_Node);
    }
    D(bug("[Kernel:ACPI-PM] %s: Registering done\n", __func__));
}

DECLARESET(KERNEL__ACPISUPPORT)
ADD2SET(ACPI_PM_SUPPORT, KERNEL__ACPISUPPORT, 0)
