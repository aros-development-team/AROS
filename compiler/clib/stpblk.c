/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Supplemental C function stpblk().
*/

#include <ctype.h>

/*****************************************************************************

    NAME */
#include <string.h>

	char * stpblk (

/*  SYNOPSIS */
	const char * str )

/*  FUNCTION
	Searches for the first non-blank character in a string. A blank 
	character is defined as one that isspace() treats like one 
	(ie. spaces, tabs and newlines).

    INPUTS
	str - String to search.

    RESULT
	A pointer to the first occurence of a non-blank character in str.

    NOTES
    	This function always returns a valid pointer as provided str isn't
	NULL. If there are no non-blank characters in the string, a pointer
	to the trailing '\0' is returned (ie. an empty string).
	
    EXAMPLE
    	char *hello = " Hello";
	char *empty = "      ";
    
    	printf( stpblk( hello ) );                 
	--> Hello
    	
	printf( stpblk( empty ) );                 
	-->
	
	printf( "%d", strlen( stpblk( hello ) ) );
	--> 5
	
	printf( "%d", strlen( stpblk( empty ) ) ); 
	--> 0
	
    BUGS

    SEE ALSO
	isspace()

    INTERNALS

******************************************************************************/
{
    if( str == NULL ) return NULL;
    
    while( *str != '\0' && isspace( *str ) ) 
    { 
    	str++;
    }
    
    return (char *)str;
} /* stpblk */
