/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Header file for a virtual filesystem that emulates the unixish root dir
    Lang: English
*/

#ifndef ROOTFS_HANDLER_GCC_H
#define ROOTFS_HANDLER_GCC_H
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <exec/devices.h>
#include <dos/dosextens.h>
#include <dos/dos.h>

#define expunge() \
AROS_LC0(BPTR, expunge, struct rootfsbase *, rootfsbase, 3, rootfs_handler)

#ifdef DOSBase
    #undef DOSBase
#endif
#define DOSBase rootfsbase->dosbase

#endif

