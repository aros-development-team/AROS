/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$

    ISO/ANSI C function mblen().
*/

#include <string.h>
#include <stdlib.h>

size_t mblen(const char *s, size_t n)
{
#   warning Implement mblen() properly
    return strlen(s);
}
