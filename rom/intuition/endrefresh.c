/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.10  2000/06/19 19:47:16  stegerg
    use new refreshlock semaphore. also use locklayerinfo, because for
    GZZ windows 2 layers are locked.

    Revision 1.9  2000/01/21 12:35:51  bergers
    No need to refresh window frame.

    Revision 1.8  2000/01/21 10:06:28  bergers
    Also refreshes window frame.

    Revision 1.7  2000/01/11 16:05:00  bergers
    Update. Move some code from intuition_driver to this directoy.

    Revision 1.6  1998/10/20 16:45:54  hkiel
    Amiga Research OS

    Revision 1.5  1997/01/27 00:36:37  ldp
    Polish

    Revision 1.4  1996/12/10 14:00:03  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.3  1996/11/08 11:28:01  aros
    All OS function use now Amiga types

    Moved intuition-driver protos to intuition_intern.h

    Revision 1.2  1996/10/24 15:51:19  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/10/21 17:06:48  aros
    A couple of new functions


    Desc:
    Lang: english
*/
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH2(void, EndRefresh,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, window, A0),
	AROS_LHA(BOOL           , complete, D0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 61, Intuition)

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

    /* Check whether the BeginRefresh was aborted due to a FALSE=BeginUpdate()*/
    if (window->Flags & WFLG_WINDOWREFRESH)
        EndUpdate(window->WLayer, complete);

    /* reset all bits indicating a necessary or ongoing refresh */
    window->Flags &= ~WFLG_WINDOWREFRESH;

    /* I reset this one only if Complete is TRUE!?! */
    if (TRUE == complete)
        window->WLayer->Flags &= ~LAYERREFRESH;

    /* Unlock the layers. */
    if (IS_GZZWINDOW(window))
        UnlockLayerRom(window->BorderRPort->Layer);

    UnlockLayerRom(window->WLayer);

    ReleaseSemaphore(&GetPrivScreen(window->WScreen)->RefreshLock);

    AROS_LIBFUNC_EXIT
} /* EndRefresh */
