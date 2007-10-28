/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

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
    struct DosLibrary *dosbase;
    BPTR seglist;
};

#ifdef DOSBase
    #undef DOSBase
#endif
#define DOSBase nilbase->dosbase

#endif

