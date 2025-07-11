/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function iswprint_l.
*/

#include <wctype.h>

int iswprint_l(wint_t wc, locale_t locale)
{
	return iswprint(wc);
}
