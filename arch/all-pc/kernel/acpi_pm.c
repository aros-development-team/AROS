/*
    Copyright © 2017-2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/asmcall.h>
#include <hardware/intbits.h>
#include <proto/acpica.h>
#include <proto/exec.h>

#include <acpica/acnames.h>
#include <acpica/accommon.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_globals.h"
#include "kernel_intern.h"
#include "kernel_intr.h"

#include "acpi.h"

#define D(x)
#define DPOFF(x)

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
        D(bug("[Kernel:ACPI-PM] %s:     FADT Reset Register @ 0x%p, Width %d, Offset %d, MemSpace %d\n", __func__, (IPTR)fadt->ResetRegister.Address, fadt->ResetRegister.BitWidth, fadt->ResetRegister.BitOffset, fadt->ResetRegister.SpaceId));
        D(bug("[Kernel:ACPI-PM] %s:     FADT Reset Value = 0x%2x\n", __func__, fadt->ResetValue));

        AcpiWrite (fadt->ResetValue, &fadt->ResetRegister);
        
        // If we got here the reset didn't happen,
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
        bug("[Kernel:ACPI-PM] %s: Reset Failed.\n", __func__);
    }
    else if (pmState == 0)
    {
        ACPI_STATUS             status;
        ACPI_OBJECT             arg = { ACPI_TYPE_INTEGER };
        ACPI_OBJECT_LIST        arg_list = { 1, &arg };
        char                    *pathName;
        UINT8                   acpiSleepTypeA, acpiSleepTypeB;
        ACPI_TABLE_FADT *fadt = (ACPI_TABLE_FADT *)acpiData->acpi_fadt;

        DPOFF(bug("[Kernel:ACPI-PM] %s: STATE 0x00 - Powering Off...\n", __func__));

        // we must be in user-mode with interrupts enabled to use AcpiCA
        krnLeaveSupervisorRing(FLAGS_INTENABLED);

        status = AcpiGetSleepTypeData(ACPI_STATE_S5,
                    &acpiSleepTypeA, &acpiSleepTypeB);

        if (!ACPI_FAILURE(status))
        {
            DPOFF(bug("[Kernel:ACPI-PM] %s:     %d:%d\n", __func__, acpiSleepTypeA, acpiSleepTypeB));

            // call Prepare To Sleep
            arg.Integer.Value = ACPI_STATE_S5;
            pathName = METHOD_PATHNAME__PTS;
            DPOFF(bug("[Kernel:ACPI-PM] %s:     > ACPI %s\n", __func__, &pathName[1]));
            status = AcpiEvaluateObject(NULL,
                                        pathName,
                                        &arg_list,
                                        NULL);
        }
        else
        {
            pathName = (char *)" ACPI Sleep Type Data";
        }

        if (!ACPI_FAILURE(status) || (status == AE_NOT_FOUND))
        {
            // call System State Transition...
            // N.B. this is an optional method so we ignore the return value
            arg.Integer.Value = ACPI_SST_INDICATOR_OFF;
            pathName = METHOD_PATHNAME__SST;
            DPOFF(bug("[Kernel:ACPI-PM] %s:     > ACPI %s\n", __func__, &pathName[1]));
            status = AcpiEvaluateObject(NULL,
                                        pathName,
                                        &arg_list,
                                        NULL);
            status = AE_OK;
        }

        if (!ACPI_FAILURE(status))
        {
            if (fadt->Flags & ACPI_FADT_HW_REDUCED)
            {
                pathName = (char *)" ACPI Sleep Control";
                if (!fadt->SleepControl.Address ||
                    !fadt->SleepStatus.Address)
                {
                    status = AE_NOT_EXIST;
                }
                else
                {
                    acpiSleepTypeB = ((acpiSleepTypeA << ACPI_X_SLEEP_TYPE_POSITION) & ACPI_X_SLEEP_TYPE_MASK);

                    DPOFF(bug("[Kernel:ACPI-PM] %s:     Sleep Control Register @ 0x%p, Width %d, Offset %d, MemSpace %d\n", __func__, (IPTR)fadt->SleepControl.Address, fadt->SleepControl.BitWidth, fadt->SleepControl.BitOffset, fadt->SleepControl.SpaceId));
                    DPOFF(bug("[Kernel:ACPI-PM] %s:     Sleep Value = 0x%2x\n", __func__, acpiSleepTypeB));

                    status = AcpiWrite ((UINT64) (acpiSleepTypeB | ACPI_X_SLEEP_ENABLE), &fadt->SleepControl);
                }
            }
            else
            {
                ACPI_BIT_REGISTER_INFO  *sleepTypeRegInfo;
                ACPI_BIT_REGISTER_INFO  *sleepEnableRegInfo;
                UINT32 xpm1aControl, xpm1bControl;

                sleepTypeRegInfo = AcpiHwGetBitRegisterInfo (ACPI_BITREG_SLEEP_TYPE);
                sleepEnableRegInfo = AcpiHwGetBitRegisterInfo (ACPI_BITREG_SLEEP_ENABLE);

                // Clear ACPI Wake status
                pathName = " ACPI Wake Status";
                status = AcpiWriteBitRegister (ACPI_BITREG_WAKE_STATUS, ACPI_CLEAR_STATUS);

                // Read the PM1A control value
                if (!ACPI_FAILURE (status))
                {
                    pathName = " PM1A Control Value";
                    status = AcpiReadBitRegister (ACPI_REGISTER_PM1_CONTROL, &xpm1aControl);
                }

                // First, write only SLP_TYP to PM1
                if (!ACPI_FAILURE (status))
                {
                    pathName = " PM1 SLP_TYP";
                    xpm1aControl &= ~(sleepTypeRegInfo->AccessBitMask |
                                     sleepEnableRegInfo->AccessBitMask);
                    xpm1bControl = xpm1aControl;

                    xpm1aControl |= (acpiSleepTypeA << sleepTypeRegInfo->BitPosition);
                    xpm1bControl |= (acpiSleepTypeB << sleepTypeRegInfo->BitPosition);

                    DPOFF(bug("[Kernel:ACPI-PM] %s:     PM1A = 0xd\n", __func__, xpm1aControl));

                    status = AcpiWrite (xpm1aControl, &fadt->XPm1aControlBlock);
                }

                if ((!ACPI_FAILURE (status)) && (fadt->XPm1bControlBlock.Address))
                {
                    status = AcpiWrite (xpm1bControl, &fadt->XPm1bControlBlock);
                }

                // Second, write both SLP_TYP and SLP_EN to PM1
                if (!ACPI_FAILURE (status))
                {
                    pathName = " PM1 SLP_TYP|SLP_EN";
                    xpm1aControl |= sleepEnableRegInfo->AccessBitMask;
                    xpm1bControl |= sleepEnableRegInfo->AccessBitMask;

                    DPOFF(bug("[Kernel:ACPI-PM] %s:     PM1A = 0xd\n", __func__, xpm1aControl));

                    CacheClearU();

                    status = AcpiWrite (xpm1aControl, &fadt->XPm1aControlBlock);
                }

                if ((!ACPI_FAILURE (status)) && (fadt->XPm1bControlBlock.Address))
                {
                    status = AcpiWrite (xpm1bControl, &fadt->XPm1bControlBlock);
                }
            }
        }

        if (ACPI_FAILURE(status))
        {
            bug("[Kernel:ACPI-PM] %s: Error evaluating %s: %s\n", __func__, &pathName[1], AcpiFormatException(status));
        }
    }
    else if (pmState == 0x90)
    {
#if defined(__AROSEXEC_SMP__)
        UQUAD timeSleep = RDTSC();
        UQUAD timeWake;
        struct APICData *apicData = KernelBase->kb_PlatformData->kb_APIC;
        apicid_t cpunum = core_APIC_GetNumber(apicData);
#endif
#if (__WORDSIZE==64)
        asm volatile ("pushfq; sti; hlt; popfq");
#else
        asm volatile ("pushfl; sti; hlt; popfl");
#endif
#if defined(__AROSEXEC_SMP__)
        timeWake = RDTSC();
        apicData->cores[cpunum].cpu_SleepTime += timeWake - timeSleep;
#endif
        if (SysBase->SysFlags & SFF_SoftInt)
            core_Cause(INTB_SOFTINT, 1L << INTB_SOFTINT);
    }
    else
    {
        // We cant handle any other states atm =/
        bug("[Kernel:ACPI-PM] %s: UNHANDLED STATE 0x%02x\n", __func__, pmState);
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

    // cache the FADT pointer ...
    pdata->kb_ACPI->acpi_fadt = fadt;

    if (fadt->Flags & ACPI_FADT_RESET_REGISTER)
    {
        // register the ACPI ChangePMState SysCall Handler ..
        krnAddSysCallHandler(pdata, &ACPI_SCChangePMStateHandler, TRUE, FALSE);
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
