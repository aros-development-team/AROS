/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"
#include <proto/exec.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <exec/libraries.h>

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
	Unloads a seglist loaded with InternalLoadSeg().
	
    INPUTS
	seglist  - Seglist
	freefunc - Function to be called to free memory

    RESULT
	DOSTRUE if everything wents O.K.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

  BPTR next;

  if (seglist)
  {
#if AROS_MODULES_DEBUG
    extern struct MinList debug_seglist;
    extern struct MinList free_debug_segnodes;
    struct debug_segnode *segnode;

    Forbid();
    ForeachNode(&debug_seglist, segnode)
    {
      if (segnode->seglist == seglist)
      {
	/* use the same free function as loadseg ! */
	struct seginfo *si;
	while ((si = (struct seginfo *)REMHEAD(&segnode->seginfos)))
	{
	  AROS_CALL2NR(void, freefunc,
	    AROS_LCA(APTR ,  (APTR)si, A1),
	    AROS_LCA(ULONG,  (ULONG)sizeof(struct seginfo), D0),
	    struct Library *, (struct Library *)SysBase
          );
	}

	REMOVE(segnode);
	ADDHEAD(&free_debug_segnodes, segnode);
        break;
      }
    }
    Permit();
#endif

    while (seglist)
    {
      next = *(BPTR *)BADDR(seglist);

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
