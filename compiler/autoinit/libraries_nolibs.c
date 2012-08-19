/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    Desc: autoinit library - automatic library opening/closing handling
          Dummy functions to be used in case no libraries have to be opened/closed
    Lang: english
*/

#include <aros/symbolsets.h>

DEFINESET(LIBS)

int _set_open_libraries_list(const void * const list[], struct ExecBase *SysBase)   __attribute__ ((weak));
void _set_close_libraries_list(const void * const list[], struct ExecBase *SysBase) __attribute__ ((weak));

int _set_open_libraries_list(const void * const list[], struct ExecBase *SysBase)
{
    return 1;
}

void _set_close_libraries_list(const void * const list[], struct ExecBase *SysBase)
{
}
