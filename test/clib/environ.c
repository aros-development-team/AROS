/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>

extern char **environ;

int main(void)
{
    int i;

    for(i = 0; environ[i] != NULL; i++)
        puts(environ[i]);

    puts("-- No environment variables left.");

    return 0;
}
