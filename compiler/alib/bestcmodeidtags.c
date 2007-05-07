/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of AllocAslRequestA()
    Lang: english
*/
#define AROS_TAGRETURNTYPE ULONG
#include <utility/tagitem.h>


/*****************************************************************************

    NAME */
// #include <libraries/cybergraphics.h>
#include <proto/cybergraphics.h>
extern struct Library *CyberGfxBase;
#undef BestCModeIDTags /* Get rid of the macro from inline/ */

	ULONG BestCModeIDTags (

/*  SYNOPSIS */
	Tag     tag1,
	...)

/*  FUNCTION
	This is the varargs version of the cybergraphics.library call
    BestCModeIDTagList().
	For more information see the documentation of 
    cybergraphics.library/BestCModeIDTagList().

    INPUTS
	tag1        -   TagList of display requirements.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	11-11-2002  Gabriele Greco  Wrote.

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = BestCModeIDTagList(AROS_SLOWSTACKTAGS_ARG(tag1));

    AROS_SLOWSTACKTAGS_POST

} /* BestCModeIDTags */
