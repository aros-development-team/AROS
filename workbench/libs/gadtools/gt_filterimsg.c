/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include "gadtools_intern.h"

/*********************************************************************

    NAME */
#include <proto/gadtools.h>
#include <intuition/intuition.h>

	AROS_LH1(struct IntuiMessage *, GT_FilterIMsg,

/*  SYNOPSIS */
	AROS_LHA(struct IntuiMessage *, imsg, A1),

/*  LOCATION */
	struct Library *, GadToolsBase, 17, GadTools)

/*  FUNCTION
	Processes an intuition message. Normally, you should not use this
	function and call GT_GetIMsg() instead. If this functions returns
	with a value != NULL, you have to call GT_PostFilterIMsg(), when
	you are done with processing the message. If it return a NULL
	pointer, you have to ReplyMsg() the message, you passed to
	GT_FilterIMsg().

    INPUTS
	imsg - pointer to the intuition message to process

    RESULT
	Either a pointer to a processed intuition message or NULL, in which
	case the message had only meaning to gadtools.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	GT_PostFilterIMsg(), GT_GetIMsg(), intuition.library/ReplyMsg()

    INTERNALS

    HISTORY

***************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GadToolsBase *,GadToolsBase)

    return imsg;

    AROS_LIBFUNC_EXIT
} /* GT_FilterIMsg */
