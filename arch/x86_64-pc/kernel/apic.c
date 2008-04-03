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
static const char str_APICsummit[] = "summit";

AROS_UFH1(int, probe_APIC_summit,
    AROS_UFHA(struct GenericAPIC *,	hook,	A0))
{
    AROS_USERFUNC_INIT

	return 0; /* to be probed later in mptable/ACPI hooks */

    AROS_USERFUNC_EXIT
} 

/**********************************************************/

static const char str_APICbigsmp[] = "bigsmp";

int dmi_bigsmp; /* can be set by dmi scanners */

AROS_UFH1(int, probe_APIC_bigsmp,
    AROS_UFHA(struct GenericAPIC *,	hook,	A0))
{
    AROS_USERFUNC_INIT

    return dmi_bigsmp; 

    AROS_USERFUNC_EXIT
}

/**********************************************************/
static const char str_APICdefault[] = "default";

AROS_UFH1(int, probe_APIC_default,
    AROS_UFHA(struct GenericAPIC *,	hook,	A0))
{
    AROS_USERFUNC_INIT

	return 1; /* should be called last. */

    AROS_USERFUNC_EXIT
} 

/**********************************************************/

static const struct GenericAPIC apic_default = {
    name : str_APICdefault,
    probe : (APTR)probe_APIC_default,
};

/*
static const struct GenericAPIC apic_summit = {
    name : str_APICsummit,
    probe : (APTR)probe_APIC_summit,
};

static const struct GenericAPIC apic_bigsmp = {
    name : str_APICbigsmp,
    probe : (APTR)probe_APIC_bigsmp,
};
*/

/**********************************************************/

static const void * const probe_APIC[] =
{ 
//	&apic_summit,
//	&apic_bigsmp, 
	&apic_default, /* must be last */
	NULL,
};

/************************************************************************************************/
/************************************************************************************************
                                    APIC RELATED FUNCTIONS
 ************************************************************************************************/
/************************************************************************************************/

IPTR core_ACPIProbeAPIC()
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
        rkprintf("[Kernel] core_ACPIProbeAPIC: No suitable APIC driver found.\n");
    }
    else
    {
        rkprintf("[Kernel] core_ACPIProbeAPIC: Using APIC driver '%s'\n", ((struct GenericAPIC *) KernACPIData.kb_APIC_Driver)->name);
    }

    return changed;
}
