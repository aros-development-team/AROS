/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <aros/debug.h>
#include <aros/symbolsets.h>

#include "debug_intern.h"

static int DebugRiscV_Init(struct DebugBase *DebugBase)
{
    /* A disassembler exists for this CPU - let alerts use it */
    DebugBase->db_Flags |= DBFF_DISASSEMBLE;

    return TRUE;
}

ADD2INITLIB(DebugRiscV_Init, 10)
