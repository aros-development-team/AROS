/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "cgxvideo_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cgxvideo_protos.h>

	AROS_LH3(ULONG, AttachVLayerTagList,

/*  SYNOPSIS */
	AROS_LHA(struct VLayerHandle *, VLayerHandle, A0),
	AROS_LHA(struct Window *, Window, A1),
	AROS_LHA(struct TagItem  *, TagItems, A2),

/*  LOCATION */
	struct Library *, CGXVideoBase, 7, Cgxvideo)

/*  FUNCTION
	Attaches a previously created videolayer handle to the specified window.
	The video overlay should now be dynamically linked to the window.
	If the window is moved or resized, the overlay is also moved or resized.

    INPUTS
	VLayerHandle - pointer to a previously created videolayer handle

	Window - pointer to the window that the videolayer should be attached to

	TagItems - pointer to an optional tag list

    RESULT
	result - 0 if videolayer could be attached to the window

    NOTES
	Tags available are:

		VOA_LeftIndent (ULONG) - additional offset from the left window border

		VOA_RightIndent (ULONG) - additional offset from the right window border

		VOA_TopIndent (ULONG) - additional offset from the top window border

		VOA_BottomIndent (ULONG) - additional offset from the bottom window border

    EXAMPLE

    BUGS

    SEE ALSO
	DetachVLayer()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,CGXVideoBase)
    extern void aros_print_not_implemented (char *);

    aros_print_not_implemented ("AttachVLayerTagList");

    AROS_LIBFUNC_EXIT
} /* AttachVLayerTagList */
