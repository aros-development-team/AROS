/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Platform defaults for partition.library. Architectures override this file.
*/

#include <proto/exec.h>
#include <exec/memory.h>

#include "partition_support.h"

/* One cache buffer per MiB of free memory, within DE_NUMBUFFERS_MIN..DE_NUMBUFFERS_MAX */
ULONG PartitionDefaultNumBuffers(void)
{
    IPTR avail = AvailMem(MEMF_ANY) >> 20;

    if (avail > DE_NUMBUFFERS_MAX)
        return DE_NUMBUFFERS_MAX;
    if (avail > DE_NUMBUFFERS_MIN)
        return (ULONG)avail;
    return DE_NUMBUFFERS_MIN;
}
