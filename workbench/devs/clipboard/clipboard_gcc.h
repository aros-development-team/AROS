#ifndef  CLIPBOARD_GCC_H
#define  CLIPBOARD_GCC_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/*  Johan Alfredsson  */

#include <aros/libcall.h>
#include <exec/execbase.h>
#include <exec/io.h>
#include <exec/devices.h>
#include <dos/dos.h>


#define init(CBBase, segList) \
AROS_LC2(struct ClipboardBase *, init, AROS_LCA(struct ClipboardBase *, CBBase, D0), AROS_LCA(BPTR, segList, A0), struct ExecBase *, SysBase, 0, Clipboard)

#define open(ioreq, unitnum, flags) \
AROS_LC3(void, open, AROS_LCA(struct IORequest *, ioreq, A1), AROS_LCA(ULONG, unitnum, D0), AROS_LCA(ULONG, flags, D0), struct ClipboardBase *, CBBase, 1, Clipboard)

#define close(ioreq) \
AROS_LC1(BPTR, close, AROS_LCA(struct IORequest *, ioreq, A1), struct ClipboardBase *, CBBase, 2, Clipboard)

#define expunge() \
AROS_LC0(BPTR, expunge, struct ClipboardBase *, CBBase, 3, Clipboard)

#define null() \
AROS_LC0(int, null, struct ClipboardBase *, CBBase, 4, Clipboard)

#define beginio(ioreq) \
AROS_LC1(void, beginio, AROS_LCA(struct IORequest *, ioreq, A1), struct ClipboardBase *, CBBase, 5, Clipboard)

#define abortio(ioreq) \
AROS_LC1(LONG, abortio, AROS_LCA(struct IORequest *, ioreq, A1), struct ClipboardBase *, CBBase, 6, Clipboard)


#endif /* CLIPBOARD_GCC_H */
