/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "iffparse_intern.h"

/*****************************************************************************

    NAME */
#include <proto/iffparse.h>

	AROS_LH1(void, CloseClipboard,

/*  SYNOPSIS */
	AROS_LHA(struct ClipboardHandle *, clipHandle, A0),

/*  LOCATION */
	struct Library *, IFFParseBase, 42, IFFParse)

/*  FUNCTION
	Closes the clipboard.device and frees the ClipboardHandle

    INPUTS
	clip - pointer to a ClipboardHandle struct created with OpenClipboard.

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	OpenClipboard(), InitIFFAsClip()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    if (clipHandle != NULL)
    {
	/* Delete the messageports */

	ClosePort (&(clipHandle->cbh_CBport), IPB(IFFParseBase));
	ClosePort (&(clipHandle->cbh_SatisfyPort), IPB(IFFParseBase));

        CloseDevice((struct IORequest *)&(clipHandle->cbh_Req));

	/*
	    Free the IO request is just a question of freiing the memory
	    allocated for it. Since the ioClipReq structure resides inside
	    the clipboardhandle, (it's all just one big portion of memory),
	    we just free the clipboardhandle.
	*/
	FreeMem(clipHandle, sizeof (struct ClipboardHandle));
    }

    AROS_LIBFUNC_EXIT
} /* CloseClipboard */
