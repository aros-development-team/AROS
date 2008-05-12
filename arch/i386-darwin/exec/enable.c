/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: i386unix version of Enable()
    Lang: english
*/

#include <exec/tasks.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/atomic.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <aros/kernel.h>
#include <proto/arossupport.h>


#undef  Exec
#ifdef UseExecstubs
#    define Exec _Exec
#endif

AROS_LH0(void, Enable,
    struct ExecBase *, SysBase, 21, Exec)
{
#undef EXEC
    AROS_LIBFUNC_INIT

  KRNWireImpl(Enable);
  if (krnEnableImpl) //enable/disable get called before kernel.resource is ready, and thus interrupts dont work yet anyhow
    CALLHOOKPKT(krnEnableImpl,0,0);
  
  AROS_LIBFUNC_EXIT
}
