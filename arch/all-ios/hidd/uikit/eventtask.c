/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1

#include <aros/asmcall.h>
#include <aros/debug.h>
#include <dos/dos.h>
#include <hardware/intbits.h>
#include <proto/exec.h>
#include <proto/hostlib.h>

#include "classbase.h"

#ifdef __i386__
/*
 * MacOS X x86 ABI says that stack must be aligned at 16 bytes boundary.
 * Some functions crash if this condition is not met. For example, CFRunLoopRunInMode()
 * crashes deeply inside UIKit (CATimeWithHostTime() function).
 * We add also noinline because gcc loves to inline such short functions, consequently
 * omitting stack realignment.
 * Unfortunately we can't do this on Darwin side. Darwin gcc simply ignores
 * force_align_arg_pointer attribute. :(
 */
#define __stackalign __attribute__((force_align_arg_pointer, noinline))
#else
#define __stackalign static inline
#endif

static AROS_INTH1(vblHandler, struct UIKitBase *,base)
{
    AROS_INTFUNC_INIT

    Signal(base->eventTask, base->eventMask);
    return FALSE;

    AROS_INTFUNC_EXIT
}

__stackalign void PollEvents(struct UIKitBase *base)
{
    base->iface->PollEvents();
    AROS_HOST_BARRIER
}

void EventTask(struct UIKitBase *base)
{
    ULONG sigset;
    LONG signal = AllocSignal(-1);

    D(bug("[UIKit] Event poll task started, signal %d, base 0x%p\n", signal, base));

    base->eventMask = 1 << signal;
    base->eventInt.is_Node.ln_Name = "Cocoa Touch events poll";
    base->eventInt.is_Code	   = (VOID_FUNC)vblHandler;
    base->eventInt.is_Data	   = base;

    AddIntServer(INTB_VERTB, &base->eventInt);

    do
    {
    	sigset = Wait(base->eventMask | SIGBREAKF_CTRL_C);

    	if (sigset & base->eventMask)
    	{
  	    HostLib_Lock();

    	    PollEvents(base);

    	    HostLib_Unlock();
    	}
    } while (!(sigset & SIGBREAKF_CTRL_C));

    RemIntServer(INTB_VERTB, &base->eventInt);
}
