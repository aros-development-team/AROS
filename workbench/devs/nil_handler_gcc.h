/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/09/11 12:52:54  digulla
    Two new devices by M. Fleischer: RAM: and NIL:

    Revision 1.2  1996/08/01 17:41:23  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#ifndef EMUL_HANDLER_GCC_H
#define EMUL_HANDLER_GCC_H
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <exec/devices.h>
#include <dos/dos.h>

struct nilbase
{
    struct Device device;
    struct ExecBase *sysbase;
    struct DosLibrary *dosbase;
    BPTR seglist;
};

#define expunge() \
__AROS_LC0(BPTR, expunge, struct nilbase *, nilbase, 3, nil_handler)

#ifdef SysBase
    #undef SysBase
#endif
#define SysBase nilbase->sysbase
#ifdef DOSBase
    #undef DOSBase
#endif
#define DOSBase nilbase->dosbase

#endif

