/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function iswdigit_l.
*/

#include <wctype.h>

int iswdigit_l(wint_t wc, locale_t locale)
{
	return iswdigit(wc);
}
