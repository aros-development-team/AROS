/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <aros/debug.h>

int main(void)
{
    char *password;

    bug("requesting password...\n");

    password = getpass("Please enter a password");

    bug("result : %s\n", password);

    return 0;
}
