/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Retrieve information about the OS
    Lang: english
*/
#define AROS_TAGRETURNTYPE  ULONG
#include <aros/arosbase.h>
#include "alib_intern.h"
#include <utility/tagitem.h>

extern struct Library * ArosBase;

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/aros.h>

	ULONG ArosInquire(

/*  SYNOPSIS */
	ULONG tag1,
	...)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	aros.library/ArosInquireA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = ArosInquireA(AROS_SLOWSTACKTAGS_ARG(tag1));

    AROS_SLOWSTACKTAGS_POST
} /* ArosInquire */
