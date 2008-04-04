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
