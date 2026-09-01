/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Platform defaults for partition.library on Amiga hardware.
*/

#include <exec/types.h>

#include "partition_support.h"

/* Memory is scarce: keep the traditional buffer count */
ULONG PartitionDefaultNumBuffers(void)
{
    return DE_NUMBUFFERS_MIN;
}
