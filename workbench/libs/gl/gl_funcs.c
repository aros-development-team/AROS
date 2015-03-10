/*
    Copyright 2014-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <GL/gla.h>

/*****************************************************************************

    NAME */

    GLAContext glACreateContextTags(

/*  SYNOPSIS */
    Tag Tag1,
    ...)

/*  FUNCTION
        This is the varargs version of gl.library/glACreateContext().
        For information see gl.library/glACreateContext().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        gl.library/glACreateContext()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE_AS(Tag1, GLAContext)
    retval = glACreateContext(AROS_SLOWSTACKTAGS_ARG(Tag1));
    AROS_SLOWSTACKTAGS_POST
}
