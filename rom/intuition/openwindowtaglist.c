/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.9  1999/08/16 21:06:55  stegerg
    taglist is now handled in openwindow

    Revision 1.8  1999/01/03 21:49:09  nlorentz
    Added handling of WA_Zoom tag

    Revision 1.7  1998/10/20 16:46:01  hkiel
    Amiga Research OS

    Revision 1.6  1998/09/12 20:20:08  hkiel
    converted TODO/FIXME comments to #warnings

    Revision 1.5  1997/01/27 00:36:42  ldp
    Polish

    Revision 1.4  1996/12/10 14:00:07  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.3  1996/10/24 15:51:23  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.2  1996/09/21 14:20:53  digulla
    Deleted empty line

    Revision 1.1  1996/09/17 16:18:17  digulla
    A new function


    Desc:
    Lang: english
*/
#include "intuition_intern.h"
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/utility.h>

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

	AROS_LH2(struct Window *, OpenWindowTagList,

/*  SYNOPSIS */
	AROS_LHA(struct NewWindow *, newWindow, A0),
	AROS_LHA(struct TagItem   *, tagList, A1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 101, Intuition)

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

    struct ExtNewWindow nw =
    {
	0,0, /* Left, Top */
	640,200, /* Width, Height */
	0xFF, 0xFF, /* DetailPen, BlockPen */
	0L, /* IDCMPFlags */
	0L, /* Flags */
	NULL, /* FirstGadget */
	NULL, /* CheckMark */
	NULL, /* Title */
	NULL, /* Screen */
	NULL, /* BitMap */
	0,0, /* MinWidth, MinHeight */
	-1,-1, /* MaxWidth, MaxHeight */
	WBENCHSCREEN, /* Type */
	NULL /* Extension (taglist) */
    };
    struct Window * window;

    if (newWindow)
	CopyMem (newWindow, &nw, (newWindow->Flags & WFLG_NW_EXTENDED) ? sizeof (struct ExtNewWindow) :
									 sizeof (struct NewWindow));

    if (tagList)
    {
    	nw.Extension = tagList;
    	nw.Flags |= WFLG_NW_EXTENDED;
    }
    
    /* stegerg: taglist is now handled in openwindow! */
    
    window = OpenWindow ((struct NewWindow *)&nw);

    return window;
    AROS_LIBFUNC_EXIT
} /* OpenWindowTagList */
