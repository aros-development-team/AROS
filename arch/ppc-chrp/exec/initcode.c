/*
    Copyright � 1995-2007, The AROS Development Team. All rights reserved.
    $Id: initcode.c 27909 2008-02-25 23:13:36Z schulz $

    Desc: Initialize resident modules
    Lang: english
*/
#include "exec_intern.h"
#include <exec/resident.h>
#include <proto/exec.h>
#define DEBUG 1
#include <aros/debug.h>

int Kernel_KrnBug(const char * format, va_list args, void * KernelBase);
#undef bug

extern void *priv_KernelBase;

static inline void bug(const char *format, ...)
{
    void *kbase = priv_KernelBase;
    va_list args;
    va_start(args, format);
    AROS_SLIB_ENTRY(KrnBug, Kernel)(format, args, kbase);
    va_end(args);
}

/*****************************************************************************

    NAME */

        AROS_LH2(void, InitCode,

/*  SYNOPSIS */
        AROS_LHA(ULONG, startClass, D0),
        AROS_LHA(ULONG, version, D1),

/*  LOCATION */
        struct ExecBase *, SysBase, 12, Exec)

/*  FUNCTION
        Traverse the ResModules array and InitResident() all modules with
        versions greater than or equal to version, and of a class equal to
        startClass.

    INPUTS
        startClass - which type of module to start
        version - a version number

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    IPTR *list = SysBase->ResModules;

    D(bug("enter InitCode(0x%x, %d)\n", startClass, version));

    if(list)
    {
        while(*list)
        {
            /*
                If bit 0 is set, this doesn't point to a Resident module, but
                to another list of modules.
            */
            if(*list & 0x00000001) list = (IPTR *)(*list & 0xfffffffe);

            if( (((struct Resident *)*list)->rt_Version >= (UBYTE)version)
             && (((struct Resident *)*list)->rt_Flags & (UBYTE)startClass) )
            {
                D(bug("calling InitResident(\"%s\", NULL)\n",
                        ((struct Resident *)(*list))->rt_Name));
                InitResident((struct Resident *)*list, NULL);
            }
            else
                D(bug("NOT calling InitResident(\"%s\", NULL)\n",
                      ((struct Resident *)(*list))->rt_Name)
                );
            list++;
        }
    }

    D(bug("leave InitCode(0x%x, %d)\n", startClass, version));

    AROS_LIBFUNC_EXIT
} /* InitCode */
