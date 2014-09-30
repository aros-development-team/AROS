/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "test.h"
#include <stdio.h>
#include <unistd.h>

int main() 
{
    TEST((usleep(1000000) != -1));
    return OK;
}

void cleanup()
{
}
