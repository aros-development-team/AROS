/*
    Copyright (C) 2022, The AROS Development Team. All rights reserved.

    Desc: Formats a message and makes sure the user will see it.
*/

#include <aros/config.h>
#include <aros/arossupportbase.h>
#include <stdarg.h>

#include <aros/system.h>
#include <proto/exec.h>
#define __KERNEL_NOLIBBASE__
#include <proto/kernel.h>
#include <proto/arossupport.h>
#undef vkprintf
#include <exec/execbase.h>

#if defined(DEBUG_USEATOMIC)
#include <aros/atomic.h>
#include <asm/cpu.h>
extern volatile ULONG   _arosdebuglock;
#endif

#include <proto/arossupport.h>

extern int _vkprintf(const char * format, va_list args);

/******************************************************************************/
int vkprintf (const char * format, va_list args)
{
    int outcount;

#if defined(DEBUG_USEATOMIC)
    if (_arosdebuglock & 1)
    {
        while (bit_test_and_set_long((ULONG*)&_arosdebuglock, 1)) { asm volatile("pause"); };
    }
#endif

    outcount = _vkprintf (format, args);

#if defined(DEBUG_USEATOMIC)
    if (_arosdebuglock & 1)
    {
        __AROS_ATOMIC_AND_L(_arosdebuglock, ~(1 << 1));
    }
#endif

  return outcount;
} /* vkprintf */
