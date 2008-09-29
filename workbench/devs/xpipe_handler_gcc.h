#ifndef XPIPE_HANDLER_GCC_H
#define XPIPE_HANDLER_GCC_H

/*
    Copyright © 2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/libcall.h>
#include <exec/execbase.h>
#include <exec/devices.h>
#include <dos/dos.h>

struct XPipeBase
{
    struct Device      device;
    struct DosLibrary *dosbase;
    struct Process    *proc;
};

#ifdef DOSBase
    #undef DOSBase
#endif
#define DOSBase xpipebase->dosbase

#endif /* !XPIPE_HANDLER_GCC_H */
