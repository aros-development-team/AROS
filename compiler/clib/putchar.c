/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    ISO C function putchar()
*/

#include <stdio.h>

int putchar(int c)
{
    return putc(c, stdout);
}
