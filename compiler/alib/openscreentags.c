/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/09/21 14:10:58  digulla
    New function: OpenScreenTags()


    Desc:
    Lang: english
*/
#include <intuition/intuitionbase.h>

extern struct IntuitionBase * IntuitionBase;

/*****************************************************************************

    NAME */
	#include <intuition/screens.h>
	#include <clib/intuition_protos.h>

	struct Screen * OpenScreenTags (

/*  SYNOPSIS */
	struct NewScreen * newScreen,
	unsigned long	   tag1,
	...)

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
    return OpenScreenTagList (newScreen, (struct TagItem *)&tag1);
} /* OpenScreenTags */
