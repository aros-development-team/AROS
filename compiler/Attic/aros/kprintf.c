/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Formats a message and makes sure the user will see it.
    Lang: english
*/
#include <aros/arosbase.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <aros/system.h>
#include <proto/dos.h>
#include <proto/aros.h>
#undef kprintf
#include <unistd.h>

#define AROSBase	((struct AROSBase *)(SysBase->DebugData))

/*****************************************************************************

    NAME */
#include <proto/aros.h>

	int kprintf (

/*  SYNOPSIS */
	const UBYTE * fmt,
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
    va_list	 args;
    int 	 ret;
    static const char * hex = "0123456789ABCDEF";
    char       * fill;
    ULONG	 val;
    LONG	 lval;

    if (!fmt)
	return write (2, "(null)", 6);

    va_start (args, fmt);

    ret = 0;

    while (*fmt)
    {
	if (*fmt == '%')
	{
	    int width = 0;

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

	    if (isdigit (*fmt))
		width = atoi (fmt);

	    while (isdigit(*fmt) || *fmt=='.' || *fmt=='-' || *fmt=='+')
		fmt ++;

	    switch (*fmt)
	    {
	    case '%': break;
		write (2, fmt, 1);
		ret ++;
		break;

	    case 's':
	    case 'S': {
		char * str = va_arg (args, char *);
		int len;

		if (!str)
		    str = "(null)";

		if (*fmt == 'S')
		{
		    write (2, "\"", 1);
		    ret ++;
		}

		len = strlen (str);

		write (2, str, len);
		ret += len;

		if (*fmt == 'S')
		{
		    write (2, "\"", 1);
		    ret ++;
		}

		break; }

	    case 'p': {
		int t;
		char puffer[sizeof (void *)*2];
		ULONG val;

		t = sizeof (void *)*2;
		val = va_arg (args, ULONG);

		while (t)
		{
		    puffer[--t] = hex[val & 0x0F];
		    val >>= 4;
		}

		write (2, puffer, sizeof (void *)*2);

		break; }

	    case 'c': {
		char c;

		c = va_arg (args, char);

		write (2, &c, 1);

		break; }

	    case 'l': {
		int t;
		char puffer[32];

		if (fmt[1] == 'u' || fmt[1] == 'd' || tolower(fmt[1]) == 'x')
		    fmt ++;

		if (*fmt == 'd')
		{
		    lval = va_arg (args, LONG);

		    val = (lval < 0) ? -lval : lval;
		}
		else
		{
		    val = va_arg (args, ULONG);
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
			write (2, fill, (width < 8) ? width : 8);
			width -= 8;
		    }

		    if (*fill == ' ')
			write (2, "0", 1);

		    ret ++;
		    break;
		}

		t = 32;

		if (*fmt == 'd' || *fmt == 'u')
		{
		    if (*fmt == 'u')
		    {
			if (lval < 0)
			{
			    write (2, "-", 1);
			    ret ++;
			    val = -lval;
			}
			else
			    val = lval;
		    }

		    while (val && t)
		    {
			puffer[--t] = hex[val % 10];

			val /= 10;
		    }
		}
		else
		{
		    while (val && t)
		    {
			puffer[--t] = hex[val & 0x0F];

			val >>= 4;
		    }
		}

		width -= 32-t;

		while (width > 0)
		{
		    write (2, fill, (width < 8) ? width : 8);
		    width -= 8;
		}

		write (2, &puffer[t], 32-t);
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
	    write (2, fmt, 1);
	    ret ++;
	}

	fmt ++; /* Next char */
    } /* while (*fmt); */

    va_end (args);

    return ret;
} /* kprintf */

