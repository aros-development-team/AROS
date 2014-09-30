/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "sysmon_intern.h"

#include <clib/alib_protos.h>

/* Memory functions */
static BOOL InitMemory(struct SysMonData *smdata)
{
    return TRUE;
}

static VOID DeInitMemory(struct SysMonData *smdata)
{
}

VOID UpdateMemoryStaticInformation(struct SysMonData * smdata)
{
    TEXT buffer[64] = {0};
    ULONG size = 0;

    /* Size */
    size = AvailMem(MEMF_ANY | MEMF_TOTAL) / 1024;
    __sprintf(buffer, "%ld kB", size);
    set(smdata->memorysize[MEMORY_RAM], MUIA_Text_Contents, buffer);

    size = AvailMem(MEMF_CHIP | MEMF_TOTAL) / 1024;
    __sprintf(buffer, "%ld kB", size);
    set(smdata->memorysize[MEMORY_CHIP], MUIA_Text_Contents, buffer);

    size = AvailMem(MEMF_FAST | MEMF_TOTAL) / 1024;
    __sprintf(buffer, "%ld kB", size);
    set(smdata->memorysize[MEMORY_FAST], MUIA_Text_Contents, buffer);
}

VOID UpdateMemoryInformation(struct SysMonData * smdata)
{
    TEXT buffer[64] = {0};
    ULONG size = 0;

    /* Free */
    size = AvailMem(MEMF_ANY) / 1024;
    __sprintf(buffer, "%ld kB", size);
    set(smdata->memoryfree[MEMORY_RAM], MUIA_Text_Contents, buffer);

    size = AvailMem(MEMF_CHIP) / 1024;
    __sprintf(buffer, "%ld kB", size);
    set(smdata->memoryfree[MEMORY_CHIP], MUIA_Text_Contents, buffer);

    size = AvailMem(MEMF_FAST) / 1024;
    __sprintf(buffer, "%ld kB", size);
    set(smdata->memoryfree[MEMORY_FAST], MUIA_Text_Contents, buffer);
}

struct SysMonModule memorymodule =
{
    .Init = InitMemory,
    .DeInit = DeInitMemory,
};

