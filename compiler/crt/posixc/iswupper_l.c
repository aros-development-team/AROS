/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function iswupper_l.
*/

#include <wctype.h>

int iswupper_l(wint_t wc, locale_t locale)
{
	return iswupper(wc);
}
