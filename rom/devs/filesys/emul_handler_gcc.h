#ifndef EMUL_HANDLER_GCC_H
#define EMUL_HANDLER_GCC_H
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <exec/devices.h>
#include <dos/dos.h>

struct emulbase
{
    struct Device device;
    struct Unit *stdin;
    struct Unit *stdout;
    struct Unit *stderr;
    struct ExecBase *sysbase;
    struct DosLibrary *dosbase;
    BPTR seglist;
};

#define expunge() \
__AROS_LC0(BPTR, expunge, struct emulbase *, emulbase, 3, emul_handler)

#ifdef SysBase
    #undef SysBase
#endif
#define SysBase emulbase->sysbase
#ifdef DOSBase
    #undef DOSBase
#endif
#define DOSBase emulbase->dosbase

#endif

