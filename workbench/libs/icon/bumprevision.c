/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include "icon_intern.h"

/*****************************************************************************

    NAME */
#include <proto/icon.h>

	AROS_LH2(UBYTE *, BumpRevision,

/*  SYNOPSIS */
	AROS_LHA(UBYTE *, newname, A0),
	AROS_LHA(UBYTE *, oldname, A1),

/*  LOCATION */
	struct IconBase *, IconBase, 18, Icon)

/*  FUNCTION
	Computes the right copy revision for a file name.

    INPUTS
	newname  -  a buffer for the new string. Should be at least 31 bytes.
	oldname  -  the old name to be revisioned.

    RESULT
	pointer to the supplied buffer.o

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    UBYTE tempstr[50]; /* A string to hold the new string, this one is big
			  enough to hold any new possible string. Then we
			  truncate this one into the newstr  */
    LONG    copy_number = 0;
    BOOL    founddigit	= FALSE;
    UBYTE   c;
    UBYTE * oldnameptr;

    oldnameptr = oldname;

    if (!strncmp (oldname, "copy_", 5))
    {
	/* Just a  usual filename */
	strcpy (tempstr, "copy_of_");
	strcpy (tempstr + 8, oldname);
    }
    else
    {
	/* Possible double or multiple-copy */
	if (!strncmp (oldname,"copy_of_", 8)) /* Is this a first copy ?*/
	{
	    /* A double copy */
	    strcpy (tempstr, "copy_2_of_");
	    strcat (tempstr, oldname + 8);
	}
	else
	{
	    /* Possible multiple copy */
	    /* Step past "copy_" */
	    oldnameptr += 5;

	    /* Convert number from text into integer */
	    while (c = *oldnameptr, ((c>='0') && (c<='9')) )
	    {
		/* Get the number of this copy. (copy_xxx_of_) */
		copy_number *= 10;
		copy_number += (c - 0x30);
		oldnameptr  ++;
		founddigit = TRUE;
	    }

	    /* Valid ?	(multiple copy or rubbish ?) */

	    if (((!strncmp(oldnameptr,"_of_",4)) && founddigit))
	    {
		/* convert back from num to text, but first increase copycount */
		copy_number ++;

		snprintf (tempstr,
		    sizeof (tempstr),
		    "copy_%ld%s",
		    (long)copy_number,
		    oldnameptr
		);
	    }
	    else
	    {
		/* Rubbish, add copy_of_ and move into tempstr */
		strcpy (tempstr, "copy_of_");
		strcpy (tempstr + 8, oldname);
	    }
	}
    }

    /* Truncate tempstr into newstr */
	CopyMem(tempstr, newname, 30);

    /* Be sure that it is nullterminated */
    newname[30] = 0; /* The 31th character */

    return newname;
    AROS_LIBFUNC_EXIT
} /* BumpRevision */
