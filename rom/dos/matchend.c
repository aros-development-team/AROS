/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <dos/dosasl.h>
#include <proto/dos.h>

	AROS_LH1(void, MatchEnd,

/*  SYNOPSIS */
	AROS_LHA(struct AnchorPath *, AP, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 139, Dos)

/*  FUNCTION
	Free the memory that was allocated by calls to MatchFirst() and
	MatchNext()

    INPUTS
	AP  - pointer to Anchor Path structure which had been passed to
              MatchFirst() before.

    RESULT
	Allocated memory is returned and filelocks are freed.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-04-97    bergers, initial revision
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

  /* Free the AChain and unlock all locks that are still there */
  struct AChain * AC = AP->ap_Current;
  struct AChain * AC_tmp;

  /* Unlock everything */
  if (NULL == AC)
    return;

  while (NULL != AC->an_Parent)
  {
    UnLock(AC->an_Lock);
    AC = AC->an_Parent;
  }
 
  CurrentDir(AC->an_Lock);
  
  /* AC points to the very first AChain obj. in the list */
  /* Free the AChain List */
  while (NULL != AC)
  {
    AC_tmp = AC->an_Child;
    FreeVec(AC);
    AC = AC_tmp;
  }

  /* Cleanup AP */
  AP->ap_Base = NULL;
  AP->ap_Current = NULL;

  AROS_LIBFUNC_EXIT
} /* MatchEnd */
