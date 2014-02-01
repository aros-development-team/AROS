/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "cybergraphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/cybergraphics.h>

	AROS_LH2(ULONG, CModeRequestTagList,

/*  SYNOPSIS */
	AROS_LHA(APTR            , , A0),
	AROS_LHA(struct TagItem *, , A1),

/*  LOCATION */
	struct Library *, CyberGfxBase, 11, Cybergraphics)

/*  FUNCTION
        Displays a requester that allows the user to select an RTG screenmode.
        Some of the requester's properties may be set using the following
        tags:
            CYBRMREQ_Screen (struct Screen *) - the screen on which the
                requester should be opened.
            CYBRMREQ_WinTitle (STRPTR) - window title.
            CYBRMREQ_OKText (STRPTR) - label text for OK button.
            CYBRMREQ_CancelText (STRPTR) - label text for Cancel button.
            CYBRMREQ_MinWidth (IPTR) - Minimum acceptable display width
                (defaults to 320).
            CYBRMREQ_MaxWidth (IPTR) - Maximum acceptable display width.
                (defaults to 1600).
            CYBRMREQ_MinHeight (IPTR) - Minimum acceptable display height.
                (defaults to 240).
            CYBRMREQ_MaxHeight (IPTR) - Maximum acceptable display height.
                (defaults to 1200).
            CYBRMREQ_MinDepth (IPTR) - Minimum acceptable display depth
                (defaults to 8).
            CYBRMREQ_MaxDepth (IPTR) - Maximum acceptable display depth
                (defaults to 32).
            CYBRMREQ_CModelArray (UWORD *) - array of permitted pixel formats.
                Any of the PIXFMT_#? constants may be specified (see
                LockBitMapTagList()), and the array must be terminated by ~0.
                By default, all pixel formats are acceptable.

    INPUTS
        requester - not used. Must be NULL.
        tagItems - options for the requester that will be created (may be
            NULL).

    RESULT
        result - user-selected screenmode ID, or zero on failure or
            user-cancellation.

    NOTES

    EXAMPLE

    BUGS
        This function is not implemented.

    SEE ALSO
        asl.library/AslRequest()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    extern void aros_print_not_implemented (char *);

    aros_print_not_implemented ("CModeRequestTagList");

    AROS_LIBFUNC_EXIT
} /* CModeRequestTagList */
