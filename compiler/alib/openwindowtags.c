/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/09/17 18:05:45  digulla
    Same names for same parameters

    Revision 1.1  1996/09/17 16:19:00  digulla
    New function: OpenWindowTags()


    Desc:
    Lang: english
*/
#include <intuition/intuitionbase.h>

extern struct IntuitionBase * IntuitionBase;

/*****************************************************************************

    NAME */
	#include <intuition/intuition.h>
	#include <clib/intuition_protos.h>

	struct Window * OpenWindowTags (

/*  SYNOPSIS */
	struct NewWindow * newWindow,
	ULONG		   tag1,
	...		   )

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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    return OpenWindowTagList (newWindow, (struct TagItem *)&tag1);

    __AROS_FUNC_EXIT
} /* OpenWindowTags */
