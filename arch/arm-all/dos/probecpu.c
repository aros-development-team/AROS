/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Obtain ARM CPU information for ELF loader
    Lang: english
*/

#define DEBUG 1

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <resources/processor.h>
#include <proto/processor.h>

#include "dos_intern.h"

static ULONG ProbeCPU(struct IntDosBase *base)
{
    APTR ProcessorBase = OpenResource("processor.resource");

    base->arm_Arch   = CPUFAMILY_ARM_3;
    base->arm_VFP    = FALSE;
    base->arm_VFP_v3 = FALSE;

    if (ProcessorBase)
    {
        GetCPUInfoTags(GCIT_Family       , &base->arm_Arch,
                       GCIT_SupportsVFP  , &base->arm_VFP,
                       GCIT_SupportsVFPv3, &base->arm_VFP_v3,
                       TAG_DONE);
    }

    D(bug("[DOS] CPU architecture: %d [VFP support: %d, VFPv3 support: %d]\n", base->arm_Arch, base->arm_VFP, base->arm_VFP_v3));

    return TRUE;
}

ADD2INITLIB(ProbeCPU, 0);
