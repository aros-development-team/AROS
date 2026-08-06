/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: GetCoreInfo() - Provides information about a single hart
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/libcall.h>
#include <proto/utility.h>
#include <resources/processor.h>

#include "processor_intern.h"
#include "processor_arch_intern.h"

#include <proto/processor.h>

/* See rom/processor/getcoreinfo.c for documentation */

AROS_LH2(BOOL, GetCoreInfo,
    AROS_LHA(ULONG, coreNo, D0),
    AROS_LHA(struct TagItem *, tagList, A0),
    struct ProcessorBase *, ProcessorBase, 3, Processor)
{
    AROS_LIBFUNC_INIT

    struct TagItem *passedTag;

    if (coreNo >= ProcessorBase->cpucount)
        return FALSE;

    while ((passedTag = NextTagItem(&tagList)) != NULL)
        Processor_AnswerTag(ProcessorBase, coreNo, passedTag);

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* GetCoreInfo() */
