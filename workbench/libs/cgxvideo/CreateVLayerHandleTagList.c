/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "cgxvideo_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cgxvideo_protos.h>

	AROS_LH2(struct VLayerHandle *, CreateVLayerHandleTagList,

/*  SYNOPSIS */
	AROS_LHA(struct Screen *, Screen, A0),
	AROS_LHA(struct TagItem  *, TagItems, A1),

/*  LOCATION */
	struct Library *, CGXVideoBase, 5, Cgxvideo)

/*  FUNCTION
	Creates a video layer handle for the given screen 

    INPUTS
	Screen - Screen we wish to create a handle for

	TagItems - pointer to an optional tag list

    RESULT
	VLayerHandle - pointer to the created videolayer handle or 0

    NOTES
	Tags available are:

		VOA_SrcType (ULONG) - specifies source type that is used for video
                              overlay data

			Currently supported formats:

				SRCFMT_YUV16 (not recommended, use YCbCr16 instead)
				SRCFMT_YCbCr16
				SRCFMT_RGB15PC
				SRCFMT_RGB16PC

		VOA_SrcWidth (ULONG) - source width in pixel units

		VOA_SrcHeight (ULONG) -  source height in pixel units

		VOA_Error (ULONG *) - If you specify VOA_Error with ti_Data pointing
                              to an ULONG, you will get more detailed information
                              if the creation of the video layer handle fails

		VOA_UseColorKey (BOOL) - If you specify VOA_UseColorKey as TRUE, color
			keying is enabled for the video layer. A
			certain color key is generated then and the
			stream data is only visible where this color
			could be found.

		VOA_UseBackFill (BOOL) - If you specify VOA_UseBackFill as TRUE automatic
			backfilling for the videolayer is enabled. This
			option is only available if color keying is
			enabled.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,CGXVideoBase)
    extern void aros_print_not_implemented (char *);

    aros_print_not_implemented ("CreateVLayerHandleTagList");

    AROS_LIBFUNC_EXIT
} /* CreateVLayerHandleTagList */
