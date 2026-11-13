/*
    Copyright (C) 2002-2026, The AROS Development Team.
    All rights reserved.
    
*/

#define AROS_TAGRETURNTYPE Object *
#include <utility/tagitem.h>
#include <proto/alib.h>

#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/muimaster.h>
extern struct Library * MUIMasterBase;

/*****************************************************************************

    NAME */
        Object * MUI_NewObject (

/*  SYNOPSIS */
        const char * classname,
        Tag tag1,
        ...)

/*  FUNCTION
        Create an object of the named MUI class using tag/value pairs.
        This is the varargs form of MUI_NewObjectA().

    INPUTS
        classname - Case-sensitive MUI class identifier.
        tag1      - First tag in a sequence of tag/value pairs terminated
                    by TAG_DONE.

    RESULT
        Pointer to the object, or NULL if the class cannot be found
        or the object cannot be created.

    NOTES
        Dispose of the returned object with MUI_DisposeObject().

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = MUI_NewObjectA(classname, AROS_SLOWSTACKTAGS_ARG(tag1));
    
    AROS_SLOWSTACKTAGS_POST
} /* MUI_NewObject */
