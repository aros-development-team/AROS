/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.7  1999/09/12 01:48:58  bernie
    more public screens support

    Revision 1.6  1999/01/16 23:19:48  hkiel
    Added aros_print_not_implemented()

    Revision 1.5  1998/10/20 16:45:57  hkiel
    Amiga Research OS

    Revision 1.4  1997/01/27 00:36:38  ldp
    Polish

    Revision 1.3  1996/12/10 14:00:04  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.2  1996/10/24 15:51:20  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/09/21 15:48:48  digulla
    New functions to handle Public Screens


    Desc: Intuition function GetDefaultPubScreen()
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH1(void, GetDefaultPubScreen,

/*  SYNOPSIS */
	AROS_LHA(UBYTE *, nameBuffer, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 97, Intuition)

/*  FUNCTION
	Returns the name of the current default public screen.
	This will be "Workbench" if there is no default public screen.

    INPUTS
	nameBuffer - A buffer of length MAXPUBSCREENNAME

    RESULT
	None.

    NOTES
	Only Public Screen Manager utilities want to use this function
	since it is easy to open a window on the default public screen
	without specifying a name.

    EXAMPLE

    BUGS

    SEE ALSO
	SetDefaultPubScreen(), OpenWindow()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    LockPubScreenList();
    strcpy(nameBuffer, GetPrivScreen(GetPrivIBase(IntuitionBase)->DefaultPubScreen)
	->pubScrNode->psn_Node.ln_Name);
    UnlockPubScreenList();

    AROS_LIBFUNC_EXIT
} /* GetDefaultPubScreen */
