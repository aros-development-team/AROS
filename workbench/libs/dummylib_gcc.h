#ifndef DUMMYLIB_GCC_H
#define DUMMYLIB_GCC_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Demo library
    Lang: english
*/
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <dos/dos.h>

struct dummybase
{
    struct Library library;
    struct ExecBase *sysbase;
    BPTR seglist;
};

#define init(dummybase, segList) \
AROS_LC2(struct dummybase *, init, AROS_LHA(struct dummybase *, dummybase, D0), AROS_LHA(BPTR, segList, A0), struct ExecBase *, SysBase, 0, dummy)

#define open(version) \
AROS_LC1(struct dummybase *, open, AROS_LHA(ULONG, version, D0), struct dummybase *, dummybase, 1, dummy)

#define close() \
AROS_LC0(BPTR, close, struct dummybase *, dummybase, 2, dummy)

#define expunge() \
AROS_LC0(BPTR, expunge, struct dummybase *, dummybase, 3, dummy)

#define null() \
AROS_LC0(int, null, struct dummybase *, dummybase, 4, dummy)

#define add(a, b) \
AROS_LC2(ULONG, add, AROS_LHA(ULONG,a,D0), AROS_LHA(ULONG,b,D1), struct dummybase *,dummybase,5,dummy)

#define asl(a, b) \
AROS_LC2(ULONG, asl, AROS_LHA(ULONG,a,D0), AROS_LHA(ULONG,b,D1), struct dummybase *,dummybase,6,dummy)

#endif /* DUMMYLIB_GCC_H */
