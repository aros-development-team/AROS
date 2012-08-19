/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: autoinit library - functions for handling the PROGRAM_ENTRIES symbolset
*/
#include <aros/symbolsets.h>

DEFINESET(PROGRAM_ENTRIES);

static int __startup_entry_pos;

void __startup_entries_init(void)
{
    __startup_entry_pos = 1;
}

void ___startup_entries_next(struct ExecBase *SysBase)
{
    void (*entry_func)(struct ExecBase *SysBase);
 
    entry_func = SETNAME(PROGRAM_ENTRIES)[__startup_entry_pos];
    if (entry_func)
    {
        __startup_entry_pos++;
        entry_func(SysBase);
    }
}
