/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Formats a message and makes sure the user will see it.
    Lang: english
*/

#include <aros/config.h>
#include <aros/arossupportbase.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <aros/system.h>
#include <proto/exec.h>
#include <proto/arossupport.h>
#undef rkprintf
#include <unistd.h>

#define AROSBase	((struct AROSBase *)(SysBase->DebugAROSBase))

/* Can't use ctypt.h *sigh* */
#define isdigit(x)      ((x) >= '0' && (x) <= '9')
#define isprint(x)      (((x) >= ' ' && (x) <= 128) || (x) >= 160)

/*****************************************************************************

    NAME */
	#include <proto/arossupport.h>

	int rkprintf (

/*  SYNOPSIS */
	const STRPTR mainSystem,
	const STRPTR subSystem,
	int level,
	const UBYTE * fmt,
	...)

/*  FUNCTION
	Call kprintf if debugging for this main and subsystem is enabled
	at a level larger than the level above. The minimal level is 1
	(this way, debugging can be disabled in the debug config file
	by giving a level of 0).

	You should not call this function directly but use the rbug
	macro. The rbug macro does some magic to make the handling
	more simple.

    INPUTS
	mainSystem - The main system. Use one of the DBG_MAINSYSTEM_*
		macros to avoid typos.
	subSystem - The part of the main system. Use one of the
		DBG_*_SUBSYSTEM_* macros.
	level - The debug level. Higher levels should give more details.
		The lowest level is 1.
	fmt - printf()-style format string

    RESULT
	The number of characters output.

    NOTES
	This function is not part of a library and may thus be called
	any time.

    EXAMPLE
	if (cache)
	{
	    ...
	    D(rbug(INTUITION, INPUTHANDLER, 3,
		"allocating event from cache (%p)", event
	    ));
	    ...
	}

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	24-12-95    digulla created

******************************************************************************/
{
    va_list	 ap;
    int 	 ret;
    //AROS_GET_SYSBASE_OK

#warning FIXME check the systems and the debug level
    // Check SysBase->DebugAROSBase->DebugConfig

    va_start(ap, fmt);
    ret = vkprintf(fmt, ap);
    va_end(ap);

    return ret;
} 
