#include <exec/types.h>

#include "blocks.h"
#include "struct.h"


/* StackSwap is not used (or required) for NG systems (at least not MorphOS), thus dummy */
void AfsDie(void)
{
}

/* Let MorphOS know that this is a native MorphOS ELF and not PowerUP binary */
#ifdef __MORPHOS__
const LONG __READONLY__ __abox__ __USED__ = 1;
#endif

/*
;-----------------------------------------------------------------------------
;	ULONG divide (ULONG d0, UWORD d1)
;-----------------------------------------------------------------------------
*/
ULONG divide(ULONG d0, UWORD d1)
{
	ULONG q = d0 / d1;
	/* NOTE: I doubt anything depends on this, but lets simulate 68k divu overflow anyway - Piru */
	if (q > 65535UL) return d0;
	return ((d0 % d1) << 16) | q;
}

/*
;-----------------------------------------------------------------------------
;	void ctodstr(cstring *a0, dstring *a1)
;
;	converts cstring a0 to dstring a1
;-----------------------------------------------------------------------------
*/
void ctodstr(const unsigned char *a0, unsigned char *a1)
{
	unsigned char *lenp = a1++;
	unsigned int len = 0;
	while ((*a1++ = *a0++))
		len++;
	*lenp = len;
}

/* SAS/C compatibility routines - only included if not SAS/C */
#ifndef __SASC

int stcu_d(char *out, unsigned int val)
{
	char tmp[11];
	char *p = &tmp[sizeof(tmp)];
	int len;
	*--p = '\0';
	do
	{
		*--p = '0' + (val % 10);
		val = val / 10;
	}
	while (val);
	for (len = 0; (*out++ = *p++); len++)
		;
	return len;
}

/* SAS/C function - similar to strcpy but return ptr to terminating \0 */
char *stpcpy(char *dst, const char *src)
{
	while ((*dst++ = *src++))
		;
	return dst - 1;
}
#endif


/*
;-----------------------------------------------------------------------------
;	void intltoupper(dstr *a0)
;
;	converts dstring a0 to uppercase in international mode
;	zie intlcmp
;-----------------------------------------------------------------------------
*/
void intltoupper(unsigned char *a0)
{
	unsigned char len = *a0++;
	while (len--)
	{
#ifdef __GNUC__
		switch (*a0)
		{
			case 0x61 ... 0x7a:
			case 0xe0 ... 0xf6:
			case 0xf8 ... 0xfe:
				*a0 -= 0x20;
				/* fall thru */
			default:
				a0++;
				break;
		}
#else
		unsigned char c = *a0++;
		if ((c >= 0x61 && c <= 0x7a) ||
		    (c >= 0xe0 && c <= 0xf6) ||
		    (c >= 0xf8 && c <= 0xfe))
		{
			a0[-1] = c - 0x20;
		}
#endif
	}
}

/*
;-----------------------------------------------------------------------------
;	bool intlcmp(dstr *a0, dstr *a1)
;
;	compares dstring a with dstring b in international mode.
;	a0 must be 'uppercased' as follows:
;
;	0x00 - 0x60 -> 0x00 - 0x60
;	0x61 - 0x7a -> 0x41 - 0x5a diff 0x20
;	0x7b - 0xdf -> 0x7b - 0xdf
;	0xe0 - 0xf6 -> 0xc0 - 0xd6 diff 0x20
;	0xf7	    -> 0xf7
;	0xf8 - 0xfe -> 0xd8 - 0xde diff 0x20
;	0xff	    -> 0xff
;
;	So if match then (d0 = d1) \/ (d1-d0 = 0x20)
;-----------------------------------------------------------------------------
*/
int intlcmp(const unsigned char *a0, const unsigned char *a1)
{
	unsigned char len = *a0++;
	if (len != *a1++)
		return 0; /* different size, can't match */

	while (len--)
	{
		unsigned char c = *a0++;
		unsigned char d = *a1++;
		if ((d - c) > d) return 0; /* carry set */
		if (c == d) continue;
		if (d - c != 0x20) return 0;
#ifdef __GNUC__
		switch (c)
		{
			case 0x41 ... 0x5a:
			case 0xc0 ... 0xd6:
			case 0xd8 ... 0xde:
				break;
			default:
				return 0;
		}
#else
		if (!((c >= 0x41 && c <= 0x5a) ||
		      (c >= 0xc0 && c <= 0xd6) ||
		      (c >= 0xd8 && c <= 0xde)))
		{
			return 0;
		}
#endif
	}
	return 1;
}

/*
;-----------------------------------------------------------------------------
;	bool intlcdcmp(cstr *a0, dstr *a1)
;
;	compares cstring a with dstring b in international mode.
;	a0 must be 'uppercased' as follows:
;
;	0x00 - 0x60 -> 0x00 - 0x60
;	0x61 - 0x7a -> 0x41 - 0x5a diff 0x20
;	0x7b - 0xdf -> 0x7b - 0xdf
;	0xe0 - 0xf6 -> 0xc0 - 0xd6 diff 0x20
;	0xf7	    -> 0xf7
;	0xf8 - 0xfe -> 0xd8 - 0xde diff 0x20
;	0xff	    -> 0xff
;
;	So if match then (d0 = d1) \/ (d1-d0 = 0x20)
;-----------------------------------------------------------------------------
*/
int intlcdcmp(const unsigned char *a0, const unsigned char *a1)
{
	unsigned char len = *a1++;
	if (len == 0)
		return *a0 == '\0'; /* zero length string */

	while (len--)
	{
		unsigned char c = *a0++;
		unsigned char d;
		if (c == '\0') return 0;
		d = *a1++;
		if ((d - c) > d) return 0; /* carry set */
		if (c == d) continue;
		if (d - c != 0x20) return 0;
#ifdef __GNUC__
		switch (c)
		{
			case 0x41 ... 0x5a:
			case 0xc0 ... 0xd6:
			case 0xd8 ... 0xde:
				break;
			default:
				return 0;
		}
#else
		if (!((c >= 0x41 && c <= 0x5a) ||
		      (c >= 0xc0 && c <= 0xd6) ||
		      (c >= 0xd8 && c <= 0xde)))
		{
			return 0;
		}
#endif
	}
	return *a0 == '\0';
}
