/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: i386unix version of Disable()
    Lang: english
*/

#include <exec/tasks.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/atomic.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/arossupport.h>
#include <aros/kernel.h>

#undef  Exec
#ifdef UseExecstubs
#    define Exec _Exec
#endif

AROS_LH0(void, Disable,
    struct ExecBase *, SysBase, 20, Exec)
{
#undef Exec
  AROS_LIBFUNC_INIT

  KRNWireImpl(Disable);

  if (krnDisableImpl) //enable/disable get called before kernel.resource is ready, and thus interrupts dont work yet anyhow
    CALLHOOKPKT(krnDisableImpl,0,0);

	AROS_LIBFUNC_EXIT
}
