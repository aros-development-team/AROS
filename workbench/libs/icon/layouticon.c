/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <exec/types.h>
#include <workbench/icon.h>
#include <utility/tagitem.h>
#include <proto/icon.h>

#include "icon_intern.h"

/*****************************************************************************

    NAME */

    AROS_LH3(BOOL, LayoutIconA,

/*  SYNOPSIS */
        AROS_LHA(struct DiskObject *, icon,   A0),
        AROS_LHA(struct Screen *,     screen, A1),
        AROS_LHA(struct TagItem *,    tags,   A2),

/*  LOCATION */
        struct IconBase *, IconBase, 32, Icon)

/*  FUNCTION
	Adapt a palette-mapped icon for display.
	
    INPUTS

    RESULT

    NOTES
	Not implemented.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct NativeIcon *nativeicon;
    struct RastPort rp;
    struct DrawInfo *dri;
    BOOL ret;
    ULONG sizex = 0, sizey = 0;
    const ULONG bflags = BMF_CLEAR | BMF_DISPLAYABLE | BMF_MINPLANES | BMF_INTERLEAVED;

    nativeicon = GetNativeIcon(icon, LB(IconBase));
    if (!nativeicon)
        return TRUE;

    /* Already mapped to this screen?
     */
    if (screen == nativeicon->iconscr)
        return TRUE;

    if (nativeicon->iconbm1) {
        FreeBitMap(nativeicon->iconbm1);
        nativeicon->iconbm1 = NULL;
    }
    if (nativeicon->iconbm2) {
        FreeBitMap(nativeicon->iconbm2);
        nativeicon->iconbm2 = NULL;
    }
    nativeicon->iconscr = NULL;

    if (screen == NULL)
        screen = GetDefaultPubScreen(NULL);

    dri = GetScreenDrawInfo(screen);
    if (dri == NULL)
        return FALSE;

    ret = FALSE;

    /* Get the sizex and sizey */
    IconControl(icon, ICONCTRLA_GetWidth,  (IPTR)&sizex,
                      ICONCTRLA_GetHeight, (IPTR)&sizey,
                      TAG_DONE);

    D(bug("%s: Screen %p, Depth %d\n", __func__, screen, dri->dri_Depth));
    nativeicon->iconbm1 = AllocBitMap(sizex, sizey, dri->dri_Depth, bflags, screen->RastPort.BitMap);
    if (nativeicon->iconbm1) {
        nativeicon->iconbm2 = AllocBitMap(sizex, sizey, dri->dri_Depth, bflags, screen->RastPort.BitMap);
        if (nativeicon->iconbm2) {

            /* Draw the normal state */
            InitRastPort(&rp);
            rp.BitMap = nativeicon->iconbm1;
            DrawIconStateA(&rp, icon, NULL, 0, 0, IDS_NORMAL, tags);

            /* Draw the selected state */
            InitRastPort(&rp);
            rp.BitMap = nativeicon->iconbm2;
            DrawIconStateA(&rp, icon, NULL, 0, 0, IDS_SELECTED, tags);

            nativeicon->iconscr = screen;

            ret = TRUE;
        } else {
            FreeBitMap(nativeicon->iconbm1);
            nativeicon->iconbm1 = NULL;
        }
    }

    FreeScreenDrawInfo(screen, dri);
    
    return ret;
    
    AROS_LIBFUNC_EXIT
} /* LayoutIconA() */
