/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: SAS C function stcu_d()
    Lang: english
*/

#include <stdio.h>

/*****************************************************************************

    NAME */
#include <string.h>

	int stcu_d (

/*  SYNOPSIS */
	char 		* out,
	unsigned	uivalue)

/*  FUNCTION
	Convert an unsigned integer to a decimal string

    INPUTS
	out     - Result will be put into this string
	uivalue - the value to convert

    RESULT
	The number of characters written into the string

    NOTES
	SAS C specific
	
    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	15.12.2000 stegerg created.

******************************************************************************/
{
 
    return sprintf(out, "%u", uivalue);
    
} /* stcu_d */
