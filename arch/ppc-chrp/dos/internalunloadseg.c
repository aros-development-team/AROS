/*
    Copyright (C) 1995-2013, The AROS Development Team. All rights reserved.

    Desc:
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
      next = AROS_SEG_NEXT(seglist);

      char *seg = (ULONG)seglist;
      seg += ((LONG)AROS_SEG_SIZE(seglist)) / 2;
      if (KernelBase)
        KrnUnregisterModule(seg);

      AROS_CALL2NR(void, freefunc,
        AROS_LCA(APTR ,  AROS_SEG_BASE(seglist), A1),
        AROS_LCA(ULONG, AROS_SEG_SIZE(seglist), D0),
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
