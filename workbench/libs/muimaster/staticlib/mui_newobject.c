/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#define AROS_TAGRETURNTYPE Object *
#include <utility/tagitem.h>
#include <proto/alib.h>
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/muimaster.h>
extern struct Library * MUIMasterBase;

	Object * MUI_NewObject (

/*  SYNOPSIS */
	char * classname,
	Tag tag1, 
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

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

bug("[MUI_NewObject] variadic call, ret addr = %016lx\n", __builtin_return_address(0));    
    retval = MUI_NewObjectA(classname, AROS_SLOWSTACKTAGS_ARG(tag1));
    
    AROS_SLOWSTACKTAGS_POST
} /* MUI_NewObject */
