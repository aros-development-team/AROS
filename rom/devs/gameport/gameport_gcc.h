#ifndef  GAMEPORT_GCC_H
#define  GAMEPORT_GCC_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*  Johan Alfredsson  */

#include <aros/libcall.h>
#include <exec/execbase.h>
#include <exec/io.h>
#include <exec/devices.h>
#include <dos/dos.h>
#include "gameport_intern.h"

#define init(GPBase, segList) \
AROS_LC2(struct GameportBase *, init, AROS_LCA(struct GameportBase *, GPBase, D0), AROS_LCA(BPTR, segList, A0), struct ExecBase *, SysBase, 0, Gameport)

#define open(ioreq, unitnum, flags) \
AROS_LC3(void, open, AROS_LCA(struct IORequest *, ioreq, A1), AROS_LCA(ULONG, unitnum, D0), AROS_LCA(ULONG, flags, D0), struct GameportBase *, GPBase, 1, Gameport)

#define close(ioreq) \
AROS_LC1(BPTR, close, AROS_LCA(struct IORequest *, ioreq, A1), struct GameportBase *, GPBase, 2, Gameport)

#define expunge() \
AROS_LC0(BPTR, expunge, struct GameportBase *, GPBase, 3, Gameport)

#define null() \
AROS_LC0(int, null, struct GameportBase *, GPBase, 4, Gameport)

#define beginio(ioreq) \
AROS_LC1(void, beginio, AROS_LCA(struct IORequest *, ioreq, A1), struct GameportBase *, GPBase, 5, Gameport)

#define abortio(ioreq) \
AROS_LC1(LONG, abortio, AROS_LCA(struct IORequest *, ioreq, A1), struct GameportBase *, GPBase, 6, Gameport)


#endif /* GAMEPORT_GCC_H */
