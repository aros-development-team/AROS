/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
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

// str2 is a BCPL string
LONG noCaseStrCmp(char *str1, char *str2, UBYTE flags, int maxlen) {
UBYTE length, i=0;

	length=str2++[0];
	do {
		if (((*str1==0) || (length==maxlen)) && (i==length))
			return 1;
		i++;
	} while (capitalch(*str1++,flags)==capitalch(*str2++,flags));
	return 0;
}

LONG StrCmp(STRPTR str1, STRPTR str2) {
	do
	{
		if ((*str1==0) && (*str2==0))
			return 1;
	} while ((*str1++==*str2++));
	return 0;
}

ULONG StrLen(STRPTR str) {
ULONG length=0;

	while (*str++)
		length++;
	return length;
}

void StrCpyToBstr(char *src, char *dst, int maxlen) {
UWORD len=0;

	while (*src && (len<maxlen))
	{
		dst[len+1]=*src++;
		len++;
	}
	dst[0]=len;
}

