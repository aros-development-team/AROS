/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: arosc library - support code for entering and leaving a program
    Lang: english
*/
#include "__arosc_privdata.h"
#include "__exitfunc.h"

#define DEBUG 0
#include <aros/debug.h>

void __arosc_program_startup(void)
{
    D(bug("[__arosc_program_startup]\n"));

    __get_arosc_privdata()->acpd_flags |= ACPD_NEWSTARTUP;
}

void __arosc_program_end(void)
{
    D(bug("[__arosc_program_end]\n"));

    __callexitfuncs();
}
