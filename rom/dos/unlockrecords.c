/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <dos/record.h>
#include <proto/dos.h>

	AROS_LH1(BOOL, UnLockRecords,

/*  SYNOPSIS */
	AROS_LHA(struct RecordLock *, recArray, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 48, Dos)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

  while (NULL != BADDR(recArray->rec_FH))
  {
    LONG success =
      UnLockRecord(
        recArray->rec_FH,
        recArray->rec_Offset,
        recArray->rec_Length
      );
    if (success != DOSTRUE)
      return success;
    /* everything OK -> advance to the next entry */
    recArray++;
  }
  return DOSTRUE;

  AROS_LIBFUNC_EXIT
} /* UnLockRecords */
