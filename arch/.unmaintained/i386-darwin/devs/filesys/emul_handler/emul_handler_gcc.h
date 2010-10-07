/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#ifndef EMUL_HANDLER_GCC_H
#define EMUL_HANDLER_GCC_H
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <exec/devices.h>
#include <dos/dos.h>
#include <proto/oop.h>
#include <hidd/unixio.h>

#include "emul_handler_intern.h"

#define expunge() \
__AROS_LC0(BPTR, expunge, struct emulbase *, emulbase, 3, emul_handler)

#endif
