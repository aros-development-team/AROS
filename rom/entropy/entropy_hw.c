/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Generic (no hardware) entropy back-end for entropy.resource.

    This is the default Entropy_HW_Init() used on architectures that have no
    dedicated entropy hardware support compiled in.  It installs no hardware
    gather function, so the resource runs purely on the software collector in
    entropy_software.c.  Architecture ports override this file (for example
    arch/all-pc/entropy/entropy_hw.c uses the x86 RDRAND/RDSEED instructions)
    and fall back to this behaviour when the hardware is unavailable.
*/

#include <exec/types.h>

#include "entropy_intern.h"

void Entropy_HW_Init(struct EntropyBase *EntropyBase)
{
    /* No hardware entropy source on this platform - software only. */
    EntropyBase->eb_HWGather = NULL;
    EntropyBase->eb_HWData   = NULL;
    EntropyBase->eb_Flags   |= EIF_SOFTWARE;
}
