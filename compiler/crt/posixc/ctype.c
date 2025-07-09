/*
    Copyright © 2025, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Stub locale aware ctype functions for inclusion in the linklib only.
		    used when inlines cannot be.

    Lang: english
*/

#define POSIXC_NOINLINE_CTYPE

#include <locale.h>
#include <ctype.h>
#include <aros/types/locale_s.h>

int isupper_l(int c, locale_t loc)
{
	return (__get_lctype(loc, c) & _ctype_upper) != 0;
}

int islower_l(int c, locale_t loc)
{
	return (__get_lctype(loc, c) & _ctype_lower) != 0;
}

int isalpha_l(int c, locale_t loc)
{
	return (__get_lctype(loc, c) & _ctype_alpha) != 0;
}

int isdigit_l(int c, locale_t loc)
{
	return (__get_lctype(loc, c) & _ctype_digit) != 0;
}

int isxdigit_l(int c, locale_t loc)
{
	return (__get_lctype(loc, c) & _ctype_xdigit) != 0;
}

int isspace_l(int c, locale_t loc)
{
	return (__get_lctype(loc, c) & _ctype_space) != 0;
}

int isprint_l(int c, locale_t loc)
{
	return (__get_lctype(loc, c) & (_ctype_alpha | _ctype_digit | _ctype_punct | _ctype_space)) != 0;
}

int isgraph_l(int c, locale_t loc)
{
	return (__get_lctype(loc, c) & (_ctype_alpha | _ctype_digit | _ctype_punct)) != 0;
}

int iscntrl_l(int c, locale_t loc)
{
	return (__get_lctype(loc, c) & _ctype_cntrl) != 0;
}

int ispunct_l(int c, locale_t loc)
{
	return (__get_lctype(loc, c) & _ctype_punct) != 0;
}

int isalnum_l(int c, locale_t loc)
{
	return (__get_lctype(loc, c) & (_ctype_alpha | _ctype_digit)) != 0;
}
