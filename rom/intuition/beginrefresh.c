/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.8  2000/01/21 12:35:22  bergers
    No more debugging output.

    Revision 1.7  2000/01/11 16:05:00  bergers
    Update. Move some code from intuition_driver to this directoy.

    Revision 1.6  1998/10/20 16:45:51  hkiel
    Amiga Research OS

    Revision 1.5  1997/01/27 00:36:36  ldp
    Polish

    Revision 1.4  1996/12/10 14:00:00  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.3  1996/11/08 11:28:00  aros
    All OS function use now Amiga types

    Moved intuition-driver protos to intuition_intern.h

    Revision 1.2  1996/10/24 15:51:17  aros
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

	AROS_LH1(void, BeginRefresh,

/*  SYNOPSIS */
	AROS_LHA(struct Window *, window, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 59, Intuition)

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

  /* lock all necessary layers */
  LockLayerRom(window->WLayer);
  /* Find out whether it's a GimmeZeroZero window with an extra layer to lock */
  if (0 != (window->Flags & WFLG_GIMMEZEROZERO))
    LockLayerRom(window->BorderRPort->Layer);


  /* I don't think I ever have to update the BorderRPort's layer */
  if (FALSE == BeginUpdate(window->WLayer))
  {
    EndUpdate(window->WLayer, FALSE);
//kprintf("%s :BeginUpdate returned FALSE!->Aborting BeginUpdate()\n",__FUNCTION__);
    return;
  }

  /* let the user know that we're currently doing a refresh */
  window->Flags |= WFLG_WINDOWREFRESH;


  AROS_LIBFUNC_EXIT
} /* BeginRefresh */
