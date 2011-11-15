/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Obtain ARM CPU information for ELF loader
    Lang: english
*/

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <resources/processor.h>
#include <proto/processor.h>

#include "dos_intern.h"

static ULONG ProbeCPU(struct IntDosBase *base)
{
    APTR ProcessorBase = OpenResource("processor.resource");

    if (!ProcessorBase)
    {
        D(bug("[DOS] No processor.resource! Won't boot!\n"));
        return FALSE;
    }    

    /*
     * Durty hack allowing to boot up a BSP without working processor.resource implementation.
     * It allows you to run any binaries. But be careful! It's strongly recommended to implement
     * processor.resource for all ARM ports.
     * The hack is based on the fact that not implemented processor.resource tags will leave their
     * data untouched.
     */
    base->arm_Arch   = CPUFAMILY_ARM_7;
    base->arm_VFP    = TRUE;
    base->arm_VFP_v3 = TRUE;

    GetCPUInfoTags(GCIT_Family       , &base->arm_Arch,
                   GCIT_SupportsVFP  , &base->arm_VFP,
                   GCIT_SupportsVFPv3, &base->arm_VFP_v3,
                   TAG_DONE);

    D(bug("[DOS] CPU architecture: %d, VFP support: %d, VFPv3 support: %d\n", base->arm_Arch, base->arm_VFP, base->arm_VFP_v3));

    return TRUE;
}

ADD2INITLIB(ProbeCPU, 0);
