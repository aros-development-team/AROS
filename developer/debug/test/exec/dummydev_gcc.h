/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef DUMMYDEV_GCC_H
#define DUMMYDEV_GCC_H
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <exec/io.h>
#include <exec/devices.h>
#include <dos/dos.h>

struct dummybase
{
    struct Device device;
    struct ExecBase *sysbase;
    BPTR seglist;
    ULONG count;
};

struct dummyrequest
{
    struct IORequest iorequest;
    ULONG id;
};

#define init(dummybase, segList) \
__AROS_LC2(struct dummybase *, init, __AROS_LA(struct dummybase *, dummybase, D0), __AROS_LA(BPTR, segList, A0), struct ExecBase *, SysBase, 0, dummy)

#define open(iob, unitnum, flags) \
__AROS_LC3(void, open, __AROS_LA(struct dummyrequest *, iob, A1), __AROS_LA(ULONG, unitnum, D0), __AROS_LA(ULONG, flags, D0), struct dummybase *, dummybase, 1, dummy)

#define close(iob) \
__AROS_LC1(BPTR, close, __AROS_LA(struct dummyrequest *, iob, A1), struct dummybase *, dummybase, 2, dummy)

#define expunge() \
__AROS_LC0(BPTR, expunge, struct dummybase *, dummybase, 3, dummy)

#define null() \
__AROS_LC0(int, null, struct dummybase *, dummybase, 4, dummy)

#define beginio(iob) \
__AROS_LC1(void, beginio, __AROS_LA(struct dummyrequest *, iob, A1), struct dummybase *, dummybase, 5, dummy)

#define abortio(iob) \
__AROS_LC1(LONG, abortio, __AROS_LA(struct dummyrequest *, iob, A1), struct dummybase *, dummybase, 6, dummy)

#endif

