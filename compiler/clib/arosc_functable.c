/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>

extern struct aroscbase *AROS_SLIB_ENTRY(open,arosc)();
extern BPTR AROS_SLIB_ENTRY(close,arosc)();
extern BPTR AROS_SLIB_ENTRY(expunge,arosc)();
extern int AROS_SLIB_ENTRY(null,arosc)();

#define SYSTEM_CALL(name) extern int name ();
#include <sys/syscall.def>
#undef SYSTEM_CALL


void *const arosc_functable[]=
{
    &AROS_SLIB_ENTRY(open,arosc),
    &AROS_SLIB_ENTRY(close,arosc),
    &AROS_SLIB_ENTRY(expunge,arosc),
    &AROS_SLIB_ENTRY(null,arosc),
#define SYSTEM_CALL(name)  &name,
#include <sys/syscall.def>
#undef SYSTEM_CALL
    (void *)-1
};

