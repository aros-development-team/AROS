#ifndef  KEYBOARD_GCC_H
#define  KEYBOARD_GCC_H

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
#include "keyboard_intern.h"

#define init(KBBase, segList) \
AROS_LC2(struct KeyboardBase *, init, AROS_LCA(struct KeyboardBase *, KBBase, D0), AROS_LCA(BPTR, segList, A0), struct ExecBase *, SysBase, 0, Keyboard)

#define open(ioreq, unitnum, flags) \
AROS_LC3(void, open, AROS_LCA(struct IORequest *, ioreq, A1), AROS_LCA(ULONG, unitnum, D0), AROS_LCA(ULONG, flags, D0), struct KeyboardBase *, KBBase, 1, Keyboard)

#define close(ioreq) \
AROS_LC1(BPTR, close, AROS_LCA(struct IORequest *, ioreq, A1), struct KeyboardBase *, KBBase, 2, Keyboard)

#define expunge() \
AROS_LC0(BPTR, expunge, struct KeyboardBase *, KBBase, 3, Keyboard)

#define null() \
AROS_LC0(int, null, struct KeyboardBase *, KBBase, 4, Keyboard)

#define beginio(ioreq) \
AROS_LC1(void, beginio, AROS_LCA(struct IORequest *, ioreq, A1), struct KeyboardBase *, KBBase, 5, Keyboard)

#define abortio(ioreq) \
AROS_LC1(LONG, abortio, AROS_LCA(struct IORequest *, ioreq, A1), struct KeyboardBase *, KBBase, 6, Keyboard)


#endif /* KEYBOARD_GCC_H */
