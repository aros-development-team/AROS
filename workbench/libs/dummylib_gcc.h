#ifndef DUMMYLIB_GCC_H
#define DUMMYLIB_GCC_H
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
__AROS_LC2(struct dummybase *, init, __AROS_LA(struct dummybase *, dummybase, D0), __AROS_LA(BPTR, segList, A0), struct ExecBase *, SysBase, 0, dummy)

#define open(version) \
__AROS_LC1(struct dummybase *, open, __AROS_LA(ULONG, version, D0), struct dummybase *, dummybase, 1, dummy)

#define close() \
__AROS_LC0(BPTR, close, struct dummybase *, dummybase, 2, dummy)

#define expunge() \
__AROS_LC0(BPTR, expunge, struct dummybase *, dummybase, 3, dummy)

#define null() \
__AROS_LC0(int, null, struct dummybase *, dummybase, 4, dummy)

#define add(a, b) \
__AROS_LC2(ULONG, add, __AROS_LA(ULONG,a,D0), __AROS_LA(ULONG,b,D1), struct dummybase *,dummybase,5,dummy)

#define asl(a, b) \
__AROS_LC2(ULONG, asl, __AROS_LA(ULONG,a,D0), __AROS_LA(ULONG,b,D1), struct dummybase *,dummybase,6,dummy)

#endif

