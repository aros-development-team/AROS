#ifndef __CON_HANDLER_INTERN_H
#define __CON_HANDLER_INTERN_H
/*
    Copyright (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Internal header-file for emulation-handler.
    Lang: english
*/

/* AROS includes */
#include <exec/libraries.h>
#include <exec/types.h>
#include <dos/dosextens.h>
#include <hidd/hidd.h>

/* POSIX includes */
#include <dirent.h>
#include <sys/types.h>


struct conbase
{
    struct Device	device;
    struct ExecBase   * sysbase;
    struct DosLibrary * dosbase;
    struct IntuitionBase *intuibase;

    BPTR seglist;
};

struct filehandle
{
    struct IOStdReq	*conio;
    struct MsgPort	*conmp;
    struct Window	*window;
};


typedef struct IntuitionBase IntuiBase;

#ifdef SysBase
#   undef SysBase
#endif
#define SysBase conbase->sysbase
#ifdef DOSBase
#   undef DOSBase
#endif
#define DOSBase conbase->dosbase
#ifdef IntuitionBase
#   undef IntuitionBase
#endif
#define IntuitionBase conbase->intuibase



#endif /* __CON_HANDLER_INTERN_H */
