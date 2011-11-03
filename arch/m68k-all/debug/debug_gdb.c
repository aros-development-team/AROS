/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Initialize the debug interface
    Lang: english
*/

#include <aros/symbolsets.h>

#include <proto/exec.h>

/* This is needed in order to bring in definition of struct segment */
#include "debug_intern.h"

#ifdef AROS_MODULES_DEBUG

/* Provided for GdbStub debugging */
const struct ELF_ModuleInfo *Debug_KickList = NULL;
struct MinList *Debug_ModList;

typedef ULONG size_t;
/* 'malloc' and 'free' are needed for GDB's strcmp(), which is
 * used by the 'loadseg' method of the .gdbinit of AROS
 */
void *malloc(ULONG size)
{
    size_t *mem;

    size = (size + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1);

    mem = AllocMem(size, -1);
    *(mem++) = size;
    return mem;
}

void free(void *ptr)
{
    size_t *mem = ptr;

    mem--;
    FreeMem(mem, mem[0]);
}
#endif

static int Debug_GdbInit(struct DebugBase *DebugBase)
{
#ifdef AROS_MODULES_DEBUG
	/* Provision for gdbstub debugging */
	Debug_ModList = &DebugBase->db_Modules;
#endif

	return TRUE;
}

ADD2INITLIB(Debug_GdbInit, 1)
