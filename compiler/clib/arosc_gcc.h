#ifndef DUMMYLIB_GCC_H
#define DUMMYLIB_GCC_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/libcall.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <dos/dos.h>

struct aroscbase
{
    struct Library library;
    BPTR seglist;
};
/*
#define init(aroscbase, segList) \
AROS_LC2(struct aroscbase *, init, AROS_LHA(struct aroscbase *, aroscbase, D0), AROS_LHA(BPTR, segList, A0), struct ExecBase *, SysBase, 0, dummy)

#define open(version) \
AROS_LC1(struct aroscbase *, open, AROS_LHA(ULONG, version, D0), struct aroscbase *, aroscbase, 1, dummy)

#define close() \
AROS_LC0(BPTR, close, struct aroscbase *, aroscbase, 2, dummy)
		  */
#define expunge() \
AROS_LC0(BPTR, expunge, struct aroscbase *, aroscbase, 3, dummy)
	  /*
#define null() \
AROS_LC0(int, null, struct aroscbase *, aroscbase, 4, dummy)
*/
#endif
