/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.1  2001/09/29 03:59:37  falemagn
    Moved nil handler in a directory of its own so that it's possible to link it directly with the kernel -> no need to mount nil: anymore

    Revision 1.4  1998/10/20 16:47:27  hkiel
    Amiga Research OS

    Revision 1.3  1996/10/24 15:50:22  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.2  1996/10/19 17:03:56  aros
    Wrong #define to protect the file

    Revision 1.1  1996/09/11 12:52:54  digulla
    Two new devices by M. Fleischer: RAM: and NIL:

    Revision 1.2  1996/08/01 17:41:23  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#ifndef NIL_HANDLER_GCC_H
#define NIL_HANDLER_GCC_H
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
AROS_LC0(BPTR, expunge, struct nilbase *, nilbase, 3, nil_handler)

#ifdef SysBase
    #undef SysBase
#endif
#define SysBase nilbase->sysbase
#ifdef DOSBase
    #undef DOSBase
#endif
#define DOSBase nilbase->dosbase

#endif

