/*
    Copyright (C) 2001 AROS - The Amiga Research OS
    $Id$

    ISO C function putchar()
*/
#include <stdio.h>

int putchar(int c)
{
    GETUSER;

    return putc(c, stdout);
}
