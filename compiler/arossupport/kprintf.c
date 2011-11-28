/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Formats a message and makes sure the user will see it.
    Lang: english
*/

#include <aros/config.h>
#include <aros/arossupportbase.h>
#include <stdarg.h>

#include <aros/system.h>
#include <dos/bptr.h>
#include <proto/exec.h>
#include <proto/arossupport.h>
#undef kprintf
#undef vkprintf
#include <exec/execbase.h>

/* Can't use ctype.h *sigh* */
#define isdigit(x)      ((x) >= '0' && (x) <= '9')
#define isprint(x)      (((x) >= ' ' && (x) <= 128) || (x) >= 160)

/* Nor string.h */
static inline int my_strlen(const char *c)
{
    int i = 0;
    while (*(c++)) i++;
    return i;
}

/* Nor atoi */
static inline int atoi(const char *c)
{
    int i;
    int isneg = 0;

    if (*c == '+')
        c++;

    if (*c == '-') {
        isneg = 1;
        c++;
    }

    for (i = 0; *c && isdigit(*c); c++) {
        i *= 10;
        i += *c - '0';
    }

    return (isneg) ? -i : i;
}

/*****************************************************************************

    NAME */
	#include <proto/arossupport.h>

	int kprintf (

/*  SYNOPSIS */
	const char * fmt,
	...)

/*  FUNCTION
	Formats fmt with the specified arguments like printf() (and *not*
	like RawDoFmt()) and uses a secure way to deliver the message to
	the user; ie. the user *will* see this message no matter what.

    INPUTS
	fmt - printf()-style format string

    RESULT
	The number of characters output.

    NOTES
	This function is not part of a library and may thus be called
	any time.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	24-12-95    digulla created

******************************************************************************/
{
    va_list	 ap;
    int		 result;

    va_start (ap, fmt);
    result = vkprintf (fmt, ap);
    va_end (ap);

    return result;
} /* kprintf */



int vkprintf (const char * fmt, va_list args)
{
    int 	 ret;
    static const char uhex[] = "0123456789ABCDEF";
    static const char lhex[] = "0123456789abcdef";
    char       * fill;
    unsigned long val;
    long	 lval = 0;

    if (!fmt)
    {
	RawPutChars ("(null)", 6);
	return 6;
    }

    ret = 0;

    while (*fmt)
    {
	if (*fmt == '%')
	{
	    int width = 0;
	    int precision = 0;

	    fmt ++;

	    if (*fmt == '0')
	    {
		fill = "00000000";
		fmt ++;
	    }
	    else
	    {
		fill = "        ";
	    }

	    if (*fmt == '*')
	    {
	        width = va_arg (args, int);
		fmt++;
	    }
	    else
	    if (isdigit (*fmt))
	    {
		width = atoi (fmt);
	        while (isdigit(*fmt)) fmt++;
	    }

	    if (*fmt == '.') fmt++;

	    if (*fmt == '*')
	    {
		precision = va_arg (args, int);
		fmt++;
	    }
	    else
	    if (isdigit (*fmt))
		precision = atoi (fmt);

	    while (isdigit(*fmt) || *fmt=='.' || *fmt=='-' || *fmt=='+')
		fmt ++;

	    switch (*fmt)
	    {
	    case '%':
		RawPutChar (*fmt);
		ret ++;
		break;

	    case 'b':
#ifndef AROS_FAST_BPTR
	    {
		char * str = va_arg (args, char *);
		int len;

		if (str) {
		    str = (char *)((unsigned long)str << 2);
		    len = *str++;
		} else {
		    str = "(null)";
		    len = 6;
		}

		if (precision)
		    len = precision;

		RawPutChars (str, len);
		ret += len;

		break; }
#endif
	    case 's':
	    case 'S': {
		char * str = va_arg (args, char *);
		int len;

		if (!str)
		    str = "(null)";

		if (*fmt == 'S')
		{
		    RawPutChar ('"');
		    ret ++;
		}

		if (precision)
		    len = precision;
		else
		    len = my_strlen (str);

		RawPutChars (str, len);
		ret += len;

		if (*fmt == 'S')
		{
		    RawPutChar ('"');
		    ret ++;
		}

		break; }

	    case 'p': {
		int t;
		char puffer[sizeof (void *)*2];

		t = sizeof (void *)*2;
		val = va_arg (args, IPTR);

		while (t)
		{
		    puffer[--t] = lhex[val & 0x0F];
		    val >>= 4;
		}

		RawPutChars (puffer, sizeof (void *)*2);

		break; }

	    case 'c': {
		UBYTE c;

		c = va_arg (args, int);

		if (isprint (c))
		    RawPutChar (c);
		else
		{
		    RawPutChars ("'\\0x", 4);
		    RawPutChar (lhex[c / 16]);
		    RawPutChar (lhex[c & 15]);
		    RawPutChar ('\'');
		}

		break; }

	    case 'l': {
		int t;
		char puffer[32];

		if (fmt[1] == 'u' || fmt[1] == 'd' || fmt[1] == 'x' || fmt[1] == 'X')
		    fmt ++;

		if (*fmt == 'd')
		{
		    lval = va_arg (args, long);

		    val = (lval < 0) ? -lval : lval;
		}
		else
		{
		    val = va_arg (args, unsigned long);
		}

print_int:
		if (val==0)
		{
		    if (width == 0)
			width = 1;

		    if (*fill == ' ')
			width --;

		    while (width > 0)
		    {
			RawPutChars (fill, (width < 8) ? width : 8);
			width -= 8;
		    }

		    if (*fill == ' ')
			RawPutChar ('0');

		    ret ++;
		    break;
		}

		t = 32;

		if (*fmt == 'd' || *fmt == 'u')
		{
		    if (*fmt == 'd')
		    {
			if (lval < 0)
			{
			    RawPutChar ('-');
			    ret ++;
			    val = -lval;
			}
			else
			    val = lval;
		    }

		    while (val && t)
		    {
			puffer[--t] = lhex[val % 10];

			val /= 10;
		    }
		}
		else if (*fmt == 'x')
		{
		    while (val && t)
		    {
			puffer[--t] = lhex[val & 0x0F];

			val >>= 4;
		    }
		}
		else
		{
		    while (val && t)
		    {
			puffer[--t] = uhex[val & 0x0F];

			val >>= 4;
		    }
		}

		width -= 32-t;

		while (width > 0)
		{
		    RawPutChars (fill, (width < 8) ? width : 8);
		    width -= 8;
		}

		RawPutChars (&puffer[t], 32-t);
		ret += 32-t;

		break; }

	    default: {
		if (*fmt == 'd')
		{
		    lval = va_arg (args, int);

		    val = (lval < 0) ? -lval : lval;
		}
		else
		{
		    val = va_arg (args, unsigned int);
		}

		goto print_int;

		break; }
	    } /* switch */
	}
	else
	{
	    RawPutChar (*fmt);
	    ret ++;
	}

	fmt ++; /* Next char */
    } /* while (*fmt); */

    return ret;
} /* vkprintf */

