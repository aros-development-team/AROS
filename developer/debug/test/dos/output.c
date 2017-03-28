/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/dos.h>

int main(void)
{
    FPuts(Output(), (STRPTR)"Testing Output() with FPuts\n");
    FPrintf(Output(), (STRPTR)"Testing Output with FPrintf\n");
    
    return 0;
}
