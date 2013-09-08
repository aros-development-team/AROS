/*
    Copyright © 2002-2012, The AROS Development Team. All rights reserved.
    $Id$

    SAS/C function stpsym().
*/

#include <ctype.h>

/*****************************************************************************

    NAME */
#include <string.h>

	char * stpsym (

/*  SYNOPSIS */
	char * str_ptr,
	char * dest_ptr,
	int dest_size)

/*  FUNCTION
	Searches for a symbol in a string.

    INPUTS
	str_ptr - points to the string to scan

	dest_ptr - points to the string where stpsym stores the symbol

	dest_size - specifies the size in bytes of *dest_ptr

    RESULT
	Pointer to the next character in the string after the symbol.
	If stpsym could not find a symbol, it returns str_ptr.

    NOTES
	A symbol consists of an alphabetic character followed by zero
	or more alphanumeric characters.  stpsym() does not skip leading
	space characters.

	Note that if you want to retrieve a symbol of length n, you need
	to ensure that *dest_ptr can accommodate at least n+1 elements,
	and that dest_size == n+1.  This extra element is needed for the
	terminating null character.

    EXAMPLE
	#include <string.h>
	#include <stdio.h>

	int main(void)
	{
	    char *text;
	    char symbol[10];
	    char *r;

	    text = "alpha1  2";
	    r = stpsym(text,symbol,10);
	    printf("%s",symbol);   // prints "alpha1"
	}

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    int count = 0;
    char *str = str_ptr;
    
    if(str == NULL)
	return NULL;

    if(isalpha(*str))
    {
        dest_ptr[count++] = *str++;
	while(isalnum(*str) && count<(dest_size-1))
	{
            dest_ptr[count++] = *str++;
	}
	dest_ptr[count] = 0;
    }

    return str;
} /* stpsym */
