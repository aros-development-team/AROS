/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <dos/dos.h>

#include <stdio.h>
#include <strings.h>

/*
    Reason for this test is that formatted printing with %f
    had crashed on x86_64.
*/

int main(void)
{
    int retval = RETURN_OK;

    char buffer[50];
    float xf = 3.14;
    double xd = 6.28;
    
    sprintf(buffer, "%6.3f %6.3f", xf, xd);

    if (strcmp(buffer, " 3.140  6.280"))
    {
        bug("created string is: %s\n", buffer);
        retval = RETURN_ERROR;
    }
    return retval;
}
