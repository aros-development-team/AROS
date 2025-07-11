/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function strcoll_l.
*/

#include <string.h>

int strcoll_l(const char *s1, const char *s2, locale_t loc)
{
	return strcoll(s1, s2);
}