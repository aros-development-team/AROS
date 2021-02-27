/*
    Copyright © 2021, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/debug.h>
#include <proto/exec.h>

#include "debug_intern.h"

static int Debugx86_Init(struct DebugBase *DebugBase)
{
		DebugBase->db_Flags |= DBFF_DISASSEMBLE;
	    return 1;
}

ADD2INITLIB(Debugx86_Init, 10)
