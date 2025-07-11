/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function strxfrm_l.
*/

#include <string.h>

size_t strxfrm_l(char *dest, const char *src, size_t n, locale_t loc)
{
	return strxfrm(dest, src, n);
}
