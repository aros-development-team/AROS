/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH3(BOOL, AddPart,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, dirname, D1),
	AROS_LHA(STRPTR, filename, D2),
	AROS_LHA(ULONG , size, D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 147, Dos)

/*  FUNCTION
	AddPart() will add a file, directory or other path name to a
	directory path. It will take into account any pre-existing
	separator characters (':','/').

	If filename is a fully qualified path, then it will replace
	the current value of dirname.

    INPUTS
	dirname     -   the path to add the new path to
	filename    -   the path you wish added
	size        -   The size of the dirname buffer, must NOT be 0

    RESULT
	non-zero if everything succeed, FALSE if the buffer would have
	overflowed.

	If the buffer would have overflowed, then dirname will not have
	been changed.

    NOTES

    EXAMPLE
	UBYTE buffer[128];
	buffer[0]='\0';
	AddPart(buffer, "Work:", 80);
	AddPart(buffer, "Programming/Include/exec");

	FPuts(Output(), buffer);
	--> Work:Programming/Include/exec

	AddPart(buffer, "/graphics", 80);

	FPuts(Output(), buffer);
	--> Work:Programming/Include/graphics

	AddPart(buffer, "gfxmacros.h", 80);
	FPuts(Output(), buffer);
	--> Work:Programming/Include/graphics/gfxmacros.h

    BUGS

    SEE ALSO
	FilePart(), PathPart()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    LONG didx, fidx;
    BOOL gotfull = FALSE;

    /*
	Make sure the buffer wouldn't overflow, also finds the ends
	of the strings...
    */

    didx = fidx = 0;

    while(dirname[didx])    didx++;
    while(filename[fidx])
    {
	/*
	    If this has a colon, and its not the first char,
	    then this is probably a FQ path.
	*/
	if((filename[fidx] == ':') && (fidx != 0))
	    gotfull = TRUE;

	fidx++;
    }

    /* If we got a fully qualified path, then just do a copy. */
    if(gotfull == TRUE)
    {
	/* +1 for NULL byte */
	if( fidx + 1 > size )
	    return DOSFALSE;

	while(*filename)
	{
	    *dirname++ = *filename++;
	}
	*dirname = '\0';
	return DOSTRUE;
    }

    /* Otherwise correctly add the subpath on to the end */
    else
    {
	/* +1 for NULL byte, +1 for '/' */
	if((didx + fidx + 2) > size)
	    return DOSFALSE;

	/*
	    Add a '/' onto the end of the current path, unless of course
	    the new path has a ':' or '/' on it already or current path
	    is emtpy (stegerg) ...
	*/
	if(    (*filename != '/')
	    && (didx != 0 )
	    && (dirname[didx - 1] != ':')
	    && (dirname[didx - 1] != '/')
	  )
	{
	    dirname[didx++] = '/';
	}
	
	/*
	    Handle leading '/'s 
	*/
	while (*filename == '/')
	{
	    filename ++; 
	    while ((dirname[didx] != '/') && (dirname[didx] != ':') && didx)
	    	didx --;
	    
	    /*
	    	if we are at start of dirname buf then break even
	    	if there are more leading '/'s
	    */
	    if	(!didx)
	    	break;
	    
	    /* 
	    	Another leading '/' ?.
	    	Only move up a dir if we are not at the root
	    */
	    if ((*filename== '/') && (dirname[didx] != ':'))
	    	didx --;
	    	
	}
	/* If at root, don't overwrite the ':' */ 
	if (dirname[didx] == ':')
	     didx ++;
	/* 
	    if filename not only was a number of '/'s but also contained
	    a subpath, then be sure to skip the found '/' of dirname.
	*/
	else if ((dirname[didx] == '/') && *filename)
	    didx ++;

	/* Now add the parts, making sure to do any backtracking... */
	while(*filename)
	{
	    if(*filename == ':')
	    {
		/*
		    Search back for a ':' or the start of the buffer
		    do the ':' test first, so we test for didx = 0 after...
		*/
		while( (dirname[didx] != ':') && didx)
		{
		    didx--;
		}
		dirname[didx++] = *filename++;
	    }
	    else
		dirname[didx++] = *filename++;
	} /* while(*filename) */

	dirname[didx] = '\0';
    }
    return DOSTRUE;

    AROS_LIBFUNC_EXIT
} /* AddPart */
