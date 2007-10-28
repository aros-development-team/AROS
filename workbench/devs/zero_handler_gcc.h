/*
    Copyright © 2001-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ZERO: handler
    Lang: English
*/

#ifndef ZERO_HANDLER_GCC_H
#define ZERO_HANDLER_GCC_H
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <exec/devices.h>
#include <dos/dos.h>

struct zerobase
{
    struct Device device;
    struct DosLibrary *dosbase;
};

#define expunge() \
AROS_LC0(BPTR, expunge, struct zerobase *, zerobase, 3, zero_handler)

#ifdef DOSBase
    #undef DOSBase
#endif
#define DOSBase zerobase->dosbase

#endif

