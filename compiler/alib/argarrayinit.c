/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************

    NAME */

#include <exec/types.h>
#include <proto/icon.h>
#include <proto/dos.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>

extern struct Library *IconBase;

/* NOTE: Global variable! */
struct DiskObject *__alib_dObject = NULL;	/* Used for reading tooltypes */


	UBYTE  **ArgArrayInit(

/*  SYNOPSIS */
	ULONG	 argc,
	UBYTE  **argv        )

/*  FUNCTION
	Initializes a NULL terminated array of strings that may be passed
	to icon.library/FindToolType() or the functions ArgInt() and
	ArgString().  This function can be passed the arguments given to
	main() regardless of whether the program was called from shell or
	from workbench.

    INPUTS
	argc  --  number of arguments to the program when called from shell
		  or 0 if called from workbench
	argv  --  'argc' pointers to the strings of the arguments; this array
		  should be NULL terminated

    RESULT
	A tooltype array to use with FindToolType(); the result of that
	function can be used with ArgString() and ArgInt() to extract
	values from the tooltype array. If the process was started from
	shell the function just returns 'argv'. If no tooltype array could
	be created, NULL is returned.

    NOTES
	This function builds some structures that should be disposed of
	when the tooltype array is no longer needed. The function
	ArgArrayDone() does that.  This function requires that icon.library
	has been opened and that IconBase is valid; in fact IconBase must
	be valid until a call to ArgArrayDone() is made.

    EXAMPLE

    BUGS

    SEE ALSO
	ArgString(), ArgInt(), ArgArrayDone(), icon.library/FindToolType()

    INTERNALS

    HISTORY
	27.04.98  SDuvan  implemented

*****************************************************************************/
{
    BPTR olddir;
    struct WBStartup *startup = (struct WBStartup *) argv;

    if(argc != 0)
	return argv;		/* We were called from shell */

    if(startup->sm_NumArgs >= 1) /* Paranoia? */
    {
	olddir = CurrentDir(startup->sm_ArgList[0].wa_Lock);
	__alib_dObject = GetDiskObject(startup->sm_ArgList[0].wa_Name);
	CurrentDir(olddir);
    }
    else
	return NULL;

    if(__alib_dObject == NULL)
	return NULL;

    return (UBYTE **)__alib_dObject->do_ToolTypes;
} /* ArgArrayInit */
