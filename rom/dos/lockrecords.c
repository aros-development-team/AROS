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

	AROS_LH2(BOOL, LockRecords,

/*  SYNOPSIS */
	AROS_LHA(struct RecordLock *, recArray, D1),
	AROS_LHA(ULONG              , timeout, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 46, Dos)

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
      LockRecord(
        recArray->rec_FH,
        recArray->rec_Offset,
        recArray->rec_Length,
        recArray->rec_Mode,
        timeout
      );
    if (success != DOSTRUE)
      return success;
    /* everything OK -> advance to the next entry */
    recArray++;
  }
  return DOSTRUE;

  AROS_LIBFUNC_EXIT
} /* LockRecords */
