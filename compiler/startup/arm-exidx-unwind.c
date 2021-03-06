/*
    Copyright (C) 2020, The AROS Development Team. All rights reserved.

    Desc: Support for ARM EABI unwinding
*/


/* The common unwinding code refers to __gnu_Unwind_Find_exidx and
 *  __cxa_type_match symbols, which are not in AROS kernels on ARM.
 */

#include <exec/types.h>

extern unsigned __exidx_end;
extern unsigned __exidx_start;

typedef unsigned _Unwind_Ptr __attribute__((__mode__(__pointer__)));

_Unwind_Ptr __gnu_Unwind_Find_exidx(_Unwind_Ptr pc __unused,
                                    int* pcount)
{
    *pcount = (__exidx_end-__exidx_start)/8;
    return __exidx_start;
}

/* __cxa_type_match.  A dummy version to be overridden by the libstdc++ one
 * when we link with it.  */

void * __attribute__((weak))
__cxa_type_match ()
{
  return (void *) 0;
}
