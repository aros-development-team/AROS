/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: most simple demo for AROS
    Lang: english
*/
#include <proto/dos.h>

static const char version[] = "$VER: helloworld 41.1 (14.3.1997)\n";

int main (int argc, char ** argv)
{
    Write (Output (), "hello, world\n", 13);
    return 0;
}
