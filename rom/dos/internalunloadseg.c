/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"
#include <proto/exec.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <exec/libraries.h>

extern void Exec_FreeMem();

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(BOOL, InternalUnLoadSeg,

/*  SYNOPSIS */
        AROS_LHA(BPTR     , seglist , D1),
        AROS_LHA(VOID_FUNC, freefunc, A1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 127, Dos)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

  BPTR next;

  if (seglist)
  {
    while (seglist)
    {
      next = *(BPTR *)BADDR(seglist);

      AROS_CALL2(void, freefunc,
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
