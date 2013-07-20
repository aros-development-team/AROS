/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"
#include <proto/exec.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <exec/libraries.h>
#include <proto/kernel.h>

#include <proto/dos.h>

/* See rom/dos/internalunloadseg.c for documentation */

AROS_LH2(BOOL, InternalUnLoadSeg,
    AROS_LHA(BPTR     , seglist , D1),
    AROS_LHA(VOID_FUNC, freefunc, A1),
    struct DosLibrary *, DOSBase, 127, Dos)
{
  AROS_LIBFUNC_INIT

  BPTR next;

  if (seglist)
  {
    APTR KernelBase = OpenResource("kernel.resource");

    while (seglist)
    {
      next = *(BPTR *)BADDR(seglist);

      char *seg = (ULONG)seglist;
      seg += (*(LONG *)((LONG)BADDR(seglist) - sizeof(ULONG))) / 2;
      if (KernelBase)
        KrnUnregisterModule(seg);

      AROS_CALL2NR(void, freefunc,
        AROS_LCA(APTR ,  (BPTR *)((LONG)BADDR(seglist) - sizeof(ULONG)), A1),
        AROS_LCA(ULONG, *(LONG *)((LONG)BADDR(seglist) - sizeof(ULONG)), D0),
        struct Library *, (struct Library *)SysBase
      );

      seglist = next;
    }
    return TRUE;
  }
  else
    return FALSE;

  AROS_LIBFUNC_EXIT
} /* InternalUnLoadSeg */
