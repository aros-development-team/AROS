 /*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "cgxvideo_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cgxvideo_protos.h>

	AROS_LH2(void, SetVLayerAttrTagList,

/*  SYNOPSIS */
	AROS_LHA(struct VLayerHandle *, VLayerHandle, A0),
	AROS_LHA(struct TagItem  *, TagItems, A1),

/*  LOCATION */
	struct Library *, CGXVideoBase, 12, Cgxvideo)

/*  FUNCTION
	Sets certain attributes for a given video layer

    INPUTS
	VLayerHandle - pointer to a previously created videolayer handle

	TagItems - pointer to a tag list which contains attributes to be
		modified

    RESULT
	none

    NOTES
	Tags available are:

		VOA_LeftIndent (ULONG) - additional offset from the left window
						border

		VOA_RightIndent (ULONG) - additional offset from the right window
						border

		VOA_TopIndent (ULONG) - additional offset from the top window border

		VOA_BottomIndent (ULONG) - additional offset from the bottom window
						border

    EXAMPLE

    BUGS

    SEE ALSO
	GetVLayerAttr()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,CGXVideoBase)
    extern void aros_print_not_implemented (char *);

    aros_print_not_implemented ("SetVLayerAttrTagList");

    AROS_LIBFUNC_EXIT
} /* SetVLayerAttrTagList */
