/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.6  1999/09/12 01:48:58  bernie
    more public screens support

    Revision 1.5  1998/10/20 16:45:59  hkiel
    Amiga Research OS

    Revision 1.4  1997/01/27 00:36:40  ldp
    Polish

    Revision 1.3  1996/12/10 14:00:05  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.2  1996/10/24 15:51:22  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/09/21 15:48:47  digulla
    New functions to handle Public Screens


    Desc:
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH1(struct Screen *, LockPubScreen,

/*  SYNOPSIS */
	AROS_LHA(UBYTE *, name, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 85, Intuition)

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
    struct Screen *scr = NULL;

    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)


    if (!name)
    {
	scr = GetPrivIBase(IntuitionBase)->DefaultPubScreen;
	//ASSERT(scr != NULL);
    }
    else
    {
	struct PubScreenNode *psn;

	/* Browse the public screen list */
	if ((psn = (struct PubScreenNode *)FindName(LockPubScreenList(), name)))
	{
		/* Don't lock screens in private state */
		if (!(psn->psn_Flags & PSNF_PRIVATE))
		{
			/* Increment screen lock count */
			psn->psn_VisitorCount++;
			scr = psn->psn_Screen;
		}
	}

	UnlockPubScreenList();
    }

    return scr;

    AROS_LIBFUNC_EXIT
} /* LockPubScreen */
