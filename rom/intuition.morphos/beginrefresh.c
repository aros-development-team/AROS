/*
	(C) 1995-96 AROS - The Amiga Research OS
	$Id$

	Desc:
	Lang: english
*/
#include <proto/layers.h>
#include <proto/graphics.h>
#include "intuition_intern.h"
#include "inputhandler_actions.h"
#include "inputhandler_support.h"
#include "renderwindowframe.h"

#define IW(x) ((struct IntWindow *)x)

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

	ULONG oldclip;
	ULONG mode = NO_DOUBLEBUFFER;

#ifdef BEGINUPDATEGADGETREFRESH
	BOOL gadgetrefresh = FALSE;
#endif

	DEBUG_REFRESH(dprintf("BeginRefresh: Window 0x%lx\n", window));

	SANITY_CHECK(window)

	//jDc: makes sure we've got 1 refresing window at a time
	LockLayerInfo(&window->WScreen->LayerInfo);

	/* jDc: MUST lock it here! */
	LOCKGADGET

	/* Find out whether it's a GimmeZeroZero window with an extra layer to lock */
	if (BLAYER(window))
		LockLayer(0,BLAYER(window));

	/* jDc: in actual implementation border layer is created as the 1st one. this means it's added to
	** screens layer semaphore list at the end (layers use AddTail), so here we also need to lock it
	** as 1st one, otherwise we run into a deadlock with LockLayers() !!!
	*/

	LockLayer(0,WLAYER(window));

	/* jDc: in current opaque implementation the damaged regions are added to
	** window's internal damage list and matched against actual damage here
	*/

	#ifdef DAMAGECACHE
	if (IW(window)->specialflags & SPFLAG_LAYERREFRESH)
	{
		//match the existing layer damage againt the stored one
		if (IW(window)->trashregion)
		{

			OrRegionRegion(IW(window)->trashregion,WLAYER(window)->DamageList);
			ClearRegion(IW(window)->trashregion);

		}

		if (IW(window)->specialflags & SPFLAG_LAYERRESIZED) mode = DOUBLEBUFFER;

		IW(window)->specialflags &= ~(SPFLAG_LAYERREFRESH|SPFLAG_LAYERRESIZED);
		#ifdef BEGINUPDATEGADGETREFRESH
		gadgetrefresh = TRUE;
		#endif
	}
	#else
	#ifdef BEGINUPDATEGADGETREFRESH
	if (IW(window)->specialflags & SPFLAG_LAYERREFRESH)
	{
		gadgetrefresh = TRUE;
		if (IW(window)->specialflags & SPFLAG_LAYERRESIZED) mode = DOUBLEBUFFER;
		IW(window)->specialflags &= ~(SPFLAG_LAYERREFRESH|SPFLAG_LAYERRESIZED);
	}
	#endif
	#endif

	if (IW(window)->specialflags & SPFLAG_WANTBUFFER) mode = DOUBLEBUFFER;

	/* I don't think I ever have to update the BorderRPort's layer */
	if (FALSE == BeginUpdate(WLAYER(window)))
	{
		EndUpdate(WLAYER(window), FALSE);

		//dprintf("%s :BeginUpdate returned FALSE!->Aborting BeginUpdate()\n",__FUNCTION__);
		return;
	}

	/* jDc: because with opaque move window borders/gadgets are not refreshed to
	** speed things up we do this here - this reduces cpu time spent in inputhandler
	*/

#ifdef BEGINUPDATEGADGETREFRESH
	if (gadgetrefresh)
	{

		if (!IS_GZZWINDOW(window))
		{
			if (window->Flags & WFLG_BORDERLESS)
			{
				int_refreshglist(window->FirstGadget, window, NULL, -1, 0, 0, IntuitionBase);
			} else {
				int_RefreshWindowFrame(window,0,0,mode,IntuitionBase);
			}
		} else {
			/* refresh all gadgets except border and gadtools gadgets */
			int_refreshglist(window->FirstGadget, window, NULL, -1, 0, REFRESHGAD_BORDER , IntuitionBase);
		}
	}
#endif

	/* let the user know that we're currently doing a refresh */
	window->Flags |= WFLG_WINDOWREFRESH;

	AROS_LIBFUNC_EXIT
} /* BeginRefresh */
