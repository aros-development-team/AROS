/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.
*/

#include <stdlib.h>

#include <aros/debug.h>

int main(void)
{
    abort();

    bug("Abort() did not work!\n");

    return 20;
}
