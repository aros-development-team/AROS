/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>

char * config;

int main (int argc, char ** argv)
{
    config = &argv[0][1];

    return 0;
}
