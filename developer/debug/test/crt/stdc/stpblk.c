/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <string.h>
#include "test.h"

int main() 
{
    char *hello = "  Hello";
    char *empty = "       ";
    
    TEST( strcmp( "Hello", stpblk( hello ) ) == 0 );
    TEST( strcmp( "",      stpblk( empty ) ) == 0 );
    TEST( strlen( stpblk( hello ) ) == 5 );
    TEST( strlen( stpblk( empty ) ) == 0 );
    
    return OK;
}

void cleanup() 
{
    /* Nothing to clean up */
}
