/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.6  1999/09/12 01:48:58  bernie
    more public screens support

    Revision 1.5  1998/10/20 16:46:04  hkiel
    Amiga Research OS

    Revision 1.4  1997/01/27 00:36:43  ldp
    Polish

    Revision 1.3  1996/12/10 14:00:09  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.2  1996/10/24 15:51:24  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/09/21 15:48:48  digulla
    New functions to handle Public Screens


    Desc:
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH1(void, SetDefaultPubScreen,

/*  SYNOPSIS */
	AROS_LHA(UBYTE *, name, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 90, Intuition)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    if (name)
    {
	struct PubScreenNode * psn;

	if ((psn = (struct PubScreenNode *)FindName (LockPubScreenList(), name)))
	    GetPrivIBase(IntuitionBase)->DefaultPubScreen = psn->psn_Screen;

	UnlockPubScreenList();
    }
    else
	GetPrivIBase(IntuitionBase)->DefaultPubScreen =
	    GetPrivIBase(IntuitionBase)->WorkBench;

    AROS_LIBFUNC_EXIT
} /* SetDefaultPubScreen */
