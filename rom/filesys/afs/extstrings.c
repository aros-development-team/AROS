/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "extstrings.h"

UBYTE capitalch(UBYTE ch, UBYTE flags) {

	if ((flags==0) || (flags==1))
		return (UBYTE)((ch>='a') && (ch<='z') ? ch-('a'-'A') : ch);
	else		// DOS\(>=2)
		return (UBYTE)(((ch>=224) && (ch<=254) && (ch!=247)) ||
				 ((ch>='a') && (ch<='z')) ? ch-('a'-'A') : ch);
}

/*
 * AFFS stores strings as BCPL strings (BSTR), however AROS may be compiled with
 * fast bstr, which are effectively just strings ... because of this we dont use the AROS macros
 * to access the strings.
 */

LONG noCaseStrCmp(const char *str1, CONST_FSBSTR str2, UBYTE flags, int maxlen) {
UBYTE length, i=0;

	/* str2 is a BSTR */
	length=str2++[0];
	do {
		if (((*str1==0) || (length==maxlen)) && (i==length))
			return 1;
		i++;
	} while (capitalch(*str1++,flags)==capitalch(*str2++,flags));
	return 0;
}

LONG StrCmp(CONST_STRPTR str1, CONST_STRPTR str2) {
	do
	{
		if ((*str1==0) && (*str2==0))
			return 1;
	} while ((*str1++==*str2++));
	return 0;
}

void StrCpyToBstr(const char *src, FSBSTR dst, int maxlen) {
UWORD len=0;

    while (*src && (len < maxlen))
    {
	dst[1 + len++]=*src++;
    }
    dst[0]=len;
}

void StrCpyFromBstr(CONST_FSBSTR src, char *dst, int maxlen) {
UWORD len=0;

    if (*src != 0)
    {
	src++;
	while (len < maxlen)
	{
	    dst[len++]=*src++;
	}
    }
}
