/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id: apic.c,v 1.7 2004/01/07 07:13:03 nicja Exp $
*/
#include <inttypes.h>

#include "exec_intern.h"
#include "etask.h"

#include <exec/lists.h>
#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <asm/segments.h>

#include "kernel_intern.h"

//#define CONFIG_LAPICS

extern struct KernelACPIData KernACPIData;

/**********************************************************
                            HOOKS
 **********************************************************/
static const char str_APICdefault[] = "default";

AROS_UFH1(int, probe_APIC_default,
    AROS_UFHA(struct GenericAPIC *,	hook,	A0))
{
    AROS_USERFUNC_INIT

    /*  Default to PIC(8259) interrupt routing model.  This gets overriden later if IOAPICs are enumerated */
    KernACPIData.kb_APIC_IRQ_Model = ACPI_IRQ_MODEL_PIC;

	return 1; /* should be called last. */

    AROS_USERFUNC_EXIT
} 

/**********************************************************/

static const struct GenericAPIC apic_default = {
    name : str_APICdefault,
    probe : (APTR)probe_APIC_default,
};

/**********************************************************/

static const void * const probe_APIC[] =
{ 
	&apic_default, /* must be last */
	NULL,
};

/************************************************************************************************/
/************************************************************************************************
                                    APIC RELATED FUNCTIONS
 ************************************************************************************************/
/************************************************************************************************/

IPTR core_APICProbe()
{
	int driver_count, retval, changed = 0;

	for ( driver_count = 0; !changed && probe_APIC[driver_count]; driver_count++ )
    { 
		if (retval = AROS_UFC1(IPTR, ((struct GenericAPIC *)probe_APIC[driver_count])->probe,
                        AROS_UFCA(struct GenericAPIC *, probe_APIC[driver_count], A0)))
        {
			changed = 1;
			KernACPIData.kb_APIC_Driver = (struct GenericAPIC *)probe_APIC[driver_count]; 
		} 
	}

	if (!changed)
    {
        rkprintf("[Kernel] core_APICProbe: No suitable APIC driver found.\n");
    }
    else
    {
        rkprintf("[Kernel] core_APICProbe: Using APIC driver '%s'\n", ((struct GenericAPIC *) KernACPIData.kb_APIC_Driver)->name);
    }

    return changed;
}

IPTR core_APICGetMSRAPICBase()
{
    IPTR  _apic_base = NULL;

#warning "TODO: Obtain APIC base from MSR"

    _apic_base = 0xfee00000;

    rkprintf("[Kernel] core_APICGetMSRAPICBase: MSR APIC Base @ %p\n", _apic_base);
    return _apic_base;
}

UBYTE core_APICGetID()
{
    IPTR _apic_id = 0;

    asm volatile
    (
        "movl	$1,%%eax\n\t"
        "cpuid\n\t"
        "movl	%%ebx,%0":"=m"(_apic_id):
    );
    
    /* Mask out the APIC's ID Bits */
    _apic_id &= 0xff000000;
    _apic_id = _apic_id >> 24;
    rkprintf("[Kernel] core_APICGetID: APIC ID %d\n", _apic_id);
    
    return (UBYTE)_apic_id & 0xff;
}

#if defined(CONFIG_LAPICS)
asm (".globl __APICTrampolineCode_start\n\t"
     ".globl __APICTrampolineCode_end\n\t"
     ".type __APICTrampolineCode_start,@function\n"
     "__APICTrampolineCode_start:\n\t"
//     "movs	%cs,%ds\n\t"
     "movl    $1,%ebx\n\t"         // Flag an SMP trampoline
     "cli\n\t"
     "movl    $1,%eax\n\t"
//     "movw	%eax,%cr0\n\t"        // Enter protected mode
     "jmp    flush_instr\n\t"
     "flush_instr:\n\t"
     "jmp kernel_cstart\n\t" // jump into kernel resource ..
     "__APICTrampolineCode_end:");
#endif
#define                 APICICR_INT_LEVELTRIG      0x8000
#define                 APICICR_INT_ASSERT         0x4000
#define                 APICICR_DM_INIT            0x500

unsigned long core_APICIPIWake(UBYTE wake_apicid, IPTR wake_apicstartrip)
{
    struct KernelBase *KernelBase = TLS_GET(KernelBase);
    unsigned long delay_time, status_ipisend, ipisend_timeout, status_ipirecv = 0;
    unsigned int apic_ver, maxlvt, start_count, max_starts = 2;

    rkprintf("[Kernel] core_APICIPIWake(%d @ %p)\n", wake_apicid, wake_apicstartrip);

    /* Send the IPI by setting APIC_ICR : Set INIT on target APIC
       by writing the apicid to the destfield of APIC_ICR2 */
    *((uint32_t*)(KernelBase->kb_APICBase + 0x310)) = ((wake_apicid)<<24);
    *((uint32_t*)(KernelBase->kb_APICBase + 0x300)) = APICICR_INT_LEVELTRIG | APICICR_INT_ASSERT | APICICR_DM_INIT;

    rkprintf("[Kernel] core_APICIPIWake: Waiting for IPI INIT to complete...\n");
    ipisend_timeout = 1000;
    do {
        delay_time = 100;
        do {} while (delay_time--);

        status_ipisend = *((uint32_t*)(KernelBase->kb_APICBase + 0x300)) & 0x1000;
    } while (status_ipisend && (ipisend_timeout--));

    delay_time = 10 * 1000;
    do {} while (delay_time--);

    /* Send the IPI by setting APIC_ICR */
    *((uint32_t*)(KernelBase->kb_APICBase + 0x310)) = ((wake_apicid)<<24); /* Set the target APIC */
    *((uint32_t*)(KernelBase->kb_APICBase + 0x300)) = APICICR_INT_LEVELTRIG | APICICR_DM_INIT;

    rkprintf("[Kernel] core_APICIPIWake: Waiting for IPI INIT to deassert ...\n");
    ipisend_timeout = 1000;
    do {
        delay_time = 100;
        do {} while (delay_time--);

        status_ipisend = *((uint32_t*)(KernelBase->kb_APICBase + 0x300)) & 0x1000;
    } while (status_ipisend && (ipisend_timeout--));

    /* memory barrier */
    do { asm volatile("mfence":::"memory"); }while(0);

    /* check for Pentium erratum 3AP .. */
    apic_ver = (*((uint32_t*)(KernelBase->kb_APICBase + 0x30)) & 0xFF);
    maxlvt = (apic_ver & 0xF0) ? ((*((uint32_t*)(KernelBase->kb_APICBase + 0x30)) >> 16) & 0xFF) : 2; /* 82489DXs doesnt report no. of LVT entries. */

    /* Perform IPI STARTUP loop */
    for (start_count = 1; start_count<=max_starts; start_count++)
    {
        rkprintf("[Kernel] core_APICIPIWake: Attempting STARTUP .. %d\n", start_count);
        *((uint32_t*)(KernelBase->kb_APICBase + 0x280)) = 0;
        status_ipisend = *(uint32_t*)(KernelBase->kb_APICBase + 0x280);
        rkprintf("[Kernel] core_APICIPIWake: IPI STARTUP sent\n");

        /* STARTUP IPI */
        *((uint32_t*)(KernelBase->kb_APICBase + 0x310)) = ((wake_apicid)<<24); /* Set the target APIC */
        *((uint32_t*)(KernelBase->kb_APICBase + 0x300)) = APICICR_DM_INIT | (wake_apicstartrip>>12);

        /* Allow the target APIC to accept the IPI */
        delay_time = 300;
        do {} while (delay_time--);

        rkprintf("[Kernel] core_APICIPIWake: Waiting for IPI STARTUP to complete...\n");
        ipisend_timeout = 1000;
        do {
            delay_time = 100;
            do {} while (delay_time--);

            status_ipisend = *((uint32_t*)(KernelBase->kb_APICBase + 0x300)) & 0x1000;
        } while (status_ipisend && (ipisend_timeout--));

        /* Allow the target APIC to accept the IPI */
        delay_time = 200;
        do {} while (delay_time--);

        if (maxlvt > 3)
            *((uint32_t*)(KernelBase->kb_APICBase + 0x280)) = 0;

        status_ipirecv = *((uint32_t*)(KernelBase->kb_APICBase + 0x280)) & 0xEF;
        if (status_ipisend || status_ipirecv) break;
    }
    rkprintf("[Kernel] core_APICIPIWake: STARTUP run finished...\n");

    if (status_ipisend)
    {
        rkprintf("[Kernel] core_APICIPIWake: APIC delivery failed\n");
    }
    if (status_ipirecv)
    {
        rkprintf("[Kernel] core_APICIPIWake: APIC delivery error (%lx)\n", status_ipirecv);
    }

    return (status_ipisend | status_ipirecv);
}