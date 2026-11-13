/*
    Copyright (C) 2020-2026, The AROS Development Team. All rights reserved.

    Desc: Support for ARM EABI unwinding
*/


/* The common unwinding code refers to __gnu_Unwind_Find_exidx and the
 *  __cxa_* symbols, which are not in AROS kernels on ARM.
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

/* Dummy versions of the C++ runtime entry points libgcc's ARM unwinder
 * calls, to be overridden by the libstdc++ ones when we link with it.
 *
 * These have to live here rather than in a link library: libgcc references
 * them weakly, and a weak undefined reference never pulls a member out of an
 * archive, so a C-only link would be left with them undefined - which the
 * AROS loader rejects. This object is linked into every ARM program, so the
 * definitions are always present. They are only reached while a C++
 * exception propagates, which cannot happen without libstdc++.  */

void * __attribute__((weak))
__cxa_type_match ()
{
  return (void *) 0;
}

int __attribute__((weak))
__cxa_begin_cleanup ()
{
  return 0;
}

void __attribute__((weak))
__cxa_call_unexpected ()
{
}
