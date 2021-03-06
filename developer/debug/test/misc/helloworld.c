/*
    Copyright (C) 1995-96, The AROS Development Team. All rights reserved.

    Desc: most simple demo for AROS
*/
#include <stdio.h>

static const char version[] __attribute__((used)) = "$VER: helloworld 41.1 (14.3.1997)\n";

int main (int argc, char ** argv)
{
    printf ("Hello, world\n");
    return 0;
}
