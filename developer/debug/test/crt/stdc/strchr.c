/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <string.h>
#include "test.h"

int main() 
{
    char *string = "test";
    char *p;

    TEST( strchr( string, '\0' ) != NULL );
    
    p = strchr( string, 't' );
    TEST( *p == 't' && p == &string[0] );
    
    p = strchr( ++p, 't' );
    TEST( *p == 't' && p == &string[3] );
    
    TEST( strchr( ++p, 't' ) == NULL );

    return OK;
}

void cleanup() 
{
    /* Nothing to clean up */
}
