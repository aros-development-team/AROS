#define __NOLIBBASE__

#include <proto/exec.h>
#if defined(__AROS__)
#include <stdio.h>
#include <string.h>
#else
#include <exec/rawfmt.h>
#endif

#include <stdarg.h>

extern struct Library *SysBase;


STRPTR strnew(APTR pool, STRPTR original)
{
	ULONG l = 1;
	STRPTR copy, p, s = original;

	if (original) {
		while (*original++) l++;

		if (copy = AllocVecPooled(pool, l))
		{
			p = copy;
			p--;
			s--;

			while (*++p = *++s);
		}
	} else {
		copy = AllocVecPooled(pool, 1);
		if (copy)
			*copy = '\0';
	}
	return copy;
}


STRPTR vfmtnew(APTR pool, STRPTR fmt, ...)
{
	ULONG l = 0;
	STRPTR s;

	va_list args;

#if defined(__AROS__)
	va_start(args, fmt);
	static UBYTE strng_tmp[1024];
	sprintf((STRPTR)strng_tmp, fmt, args);
	va_end(args);
	l = strlen(strng_tmp);
#else
	va_list copy;

	__va_copy(copy, args);

	VNewRawDoFmt(fmt, (APTR(*)(APTR, UBYTE))RAWFMTFUNC_COUNT, (STRPTR)&l, args);
#endif

	if (s = AllocVecPooled(pool, l + 1))
	{
#if defined(__AROS__)
		strcpy(s, strng_tmp);
#else
		VNewRawDoFmt(fmt, RAWFMTFUNC_STRING, s, copy);
#endif
	}
	return s;
}


STRPTR fmtnew(APTR pool, STRPTR fmt, ...)
{
	STRPTR s;
	va_list args;

	va_start(args, fmt);
	s = vfmtnew(pool, fmt, args);
	va_end(args);
	return s;
}



