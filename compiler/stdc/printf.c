/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    C99 function printf().
*/
#include <libraries/stdcio.h>

#define DEBUG 0
#include <aros/debug.h>
#if DEBUG
#include <aros/libcall.h>
#endif

/*****************************************************************************

    NAME */
#include <stdio.h>
#include <stdarg.h>

	int printf (

/*  SYNOPSIS */
	const char * restrict format,
	...)

/*  FUNCTION
	Formats a list of arguments and prints them to standard out.

	The format string is composed of zero or more directives: ordinary
	characters (not %), which are copied unchanged to the output
	stream; and conversion specifications, each of which results in
	fetching zero or more subsequent arguments Each conversion
	specification is introduced by the character %. The arguments must
	correspond properly (after type promotion) with the conversion
	specifier. After the %, the following appear in sequence:

	\begin{itemize}
	\item Zero or more of the following flags:

	\begin{description}
	\item{#} specifying that the value should be converted to an
	``alternate form''. For c, d, i, n, p, s, and u conversions, this
	option has no effect. For o conversions, the precision of the
	number is increased to force the first character of the output
	string to a zero (except if a zero value is printed with an
	explicit precision of zero). For x and X conversions, a non-zero
	result has the string `0x' (or `0X' for X conversions) prepended to
	it. For e, E, f, g, and G conversions, the result will always
	contain a decimal point, even if no digits follow it (normally, a
	decimal point appears in the results of those conversions only if a
	digit follows). For g and G conversions, trailing zeros are not
	removed from the result as they would otherwise be.

	\item{0} specifying zero padding. For all conversions except n, the
	converted value is padded on the left with zeros rather than
	blanks. If a precision is given with a numeric conversion (d, i, o,
	u, i, x, and X), the 0 flag is ignored.

	\item{-} (a negative field width flag) indicates the converted
	value is to be left adjusted on the field boundary. Except for n
	conversions, the converted value is padded on the right with
	blanks, rather than on the left with blanks or zeros. A -
	overrides a 0 if both are given.

	\item{ } (a space) specifying that a blank should be left before a
	positive number produced by a signed conversion (d, e, E, f, g, G,
	or i). + specifying that a sign always be placed before a number
	produced by a signed conversion. A + overrides a space if both are
	used.

	\item{'} specifying that in a numerical argument the output is to
	be grouped if the locale information indicates any. Note that many
	versions of gcc cannot parse this option and will issue a warning.

	\end{description}

	\item An optional decimal digit string specifying a minimum field
	width. If the converted value has fewer characters than the field
	width, it will be padded with spaces on the left (or right, if the
	left-adjustment flag has been given) to fill out the field width.

	\item An optional precision, in the form of a period (`.') followed
	by an optional digit string. If the digit string is omitted, the
	precision is taken as zero. This gives the minimum number of digits
	to appear for d, i, o, u, x, and X conversions, the number of
	digits to appear after the decimal-point for e, E, and f
	conversions, the maximum number of significant digits for g and G
	conversions, or the maximum number of characters to be printed from
	a string for s conversions.

	\item The optional character h, specifying that a following d, i,
	o, u, x, or X conversion corresponds to a short int or unsigned
	short int argument, or that a following n conversion corresponds to
	a pointer to a short int argument.

	\item The optional character l (ell) specifying that a following d,
	i, o, u, x, or X conversion applies to a pointer to a long int or
	unsigned long int argument, or that a following n conversion
	corresponds to a pointer to a long int argument. Linux provides a
	non ANSI compliant use of two l flags as a synonym to q or L. Thus
	ll can be used in combination with float conversions. This usage
	is, however, strongly discouraged.

	\item The character L specifying that a following e, E,
	f, g, or G conversion corresponds to a long double
	argument, or a following d, i, o, u, x, or X conversion corresponds to a long long argument. Note
	that long long is not specified in ANSI C and
	therefore not portable to all architectures.

	\item The optional character q. This is equivalent to L. See the
	STANDARDS and BUGS sections for comments on the use of ll, L, and
	q.

	\item A Z character specifying that the following integer (d, i, o,
	u, i, x, and X), conversion corresponds to a size_t argument.

	\item A character that specifies the type of conversion to be
	applied.

	A field width or precision, or both, may be indicated by an
	asterisk `*' instead of a digit string. In this case, an int
	argument supplies the field width or precision. A negative field
	width is treated as a left adjustment flag followed by a positive
	field width; a negative precision is treated as though it were
	missing.

	The conversion specifiers and their meanings are:

	\begin{description}
	\item{diouxX} The int (or appropriate variant) argument is
	converted to signed decimal (d and i), unsigned octal (o, unsigned
	decimal (u, or unsigned hexadecimal (x and X) notation. The letters
	abcdef are used for x conversions; the letters ABCDEF are used for
	X conversions. The precision, if any, gives the minimum number of
	digits that must appear; if the converted value requires fewer
	digits, it is padded on the left with zeros.

	\item{eE} The double argument is rounded and converted in the style
	[<->]d.dddedd where there is one digit before the decimal-point
	character and the number of digits after it is equal to the
	precision; if the precision is missing, it is taken as 6; if the
	precision is zero, no decimal-point character appears. An E
	conversion uses the letter E (rather than e) to introduce the
	exponent. The exponent always contains at least two digits; if the
	value is zero, the exponent is 00.

	\item{f} The double argument is rounded and converted to decimal
	notation in the style [-]ddd.ddd, where the number of digits after
	the decimal-point character is equal to the precision
	specification. If the precision is missing, it is taken as 6; if
	the precision is explicitly zero, no decimal-point character
	appears. If a decimal point appears, at least one digit appears
	before it.

	\item{g} The double argument is converted in style f or e (or E for
	G conversions). The precision specifies the number of significant
	digits. If the precision is missing, 6 digits are given; if the
	precision is zero, it is treated as 1. Style e is used if the
	exponent from its conversion is less than -4 or greater than or
	equal to the precision. Trailing zeros are removed from the
	fractional part of the result; a decimal point appears only if it
	is followed by at least one digit.

	\item{c} The int argument is converted to an unsigned char, and the
	resulting character is written.

	\item{s} The ``char *'' argument is expected to be a pointer to an
	array of character type (pointer to a string). Characters from the
	array are written up to (but not including) a terminating NUL
	character; if a precision is specified, no more than the number
	specified are written. If a precision is given, no null character
	need be present; if the precision is not specified, or is greater
	than the size of the array, the array must contain a terminating
	NUL character.

	\item{p} The ``void *'' pointer argument is printed in hexadecimal
	(as if by %#x or %#lx).

	\item{n} The number of characters written so far is stored into the
	integer indicated by the ``int *'' (or variant) pointer argument.
	No argument is converted.

	\item{%} A `%' is written. No argument is converted. The complete
	conversion specification is `%%'.

	\end{description}
	\end{itemize}

	In no case does a non-existent or small field width cause
	truncation of a field; if the result of a conversion is wider than
	the field width, the field is expanded to contain the conversion
	result.

    INPUTS
	format - Format string as described above
	... - Arguments for the format string

    RESULT
	The number of characters written to stdout or EOF on error.

    NOTES

    EXAMPLE
	To print a date and time in the form `Sunday, July 3,
	10:02', where weekday and month are pointers to strings:

	    #include <stdio.h>

	    fprintf (stdout, "%s, %s %d, %.2d:%.2d\n",
		    weekday, month, day, hour, min);

	To print to five decimal places:

	    #include <math.h>
	    #include <stdio.h>

	    fprintf (stdout, "pi = %.5f\n", 4 * atan(1.0));

	To allocate a 128 byte string and print into it:

	    #include <stdio.h>
	    #include <stdlib.h>
	    #include <stdarg.h>

	    char *newfmt(const char *fmt, ...)
	    {
		char *p;
		va_list ap;

		if ((p = malloc(128)) == NULL)
		    return (NULL);

		va_start(ap, fmt);

		(void) vsnprintf(p, 128, fmt, ap);

		va_end(ap);

		return (p);
	    }

    BUGS
	All functions are fully ANSI C3.159-1989 conformant, but provide
	the additional flags q, Z and ' as well as an additional behaviour
	of the L and l flags. The latter may be considered to be a bug, as
	it changes the behaviour of flags defined in ANSI C3.159-1989.

	The effect of padding the %p format with zeros (either by the 0
	flag or by specifying a precision), and the benign effect (i.e.,
	none) of the # flag on %n and %p conversions, as well as
	nonsensical combinations such as are not standard; such
	combinations should be avoided.

	Some combinations of flags defined by ANSI C are not making sense
	in ANSI C (e.g. %Ld). While they may have a well-defined behaviour
	on Linux, this need not to be so on other architectures. Therefore
	it usually is better to use flags that are not defined by ANSI C at
	all, i.e. use q instead of L in combination with diouxX conversions
	or ll. The usage of q is not the same as on BSD 4.4, as it may be
	used in float conversions equivalently to L.

	Because sprintf and vsprintf assume an infinitely long string,
	callers must be careful not to overflow the actual space; this is
	often impossible to assure.

    SEE ALSO
	fprintf(), vprintf(), vfprintf(), sprintf(), vsprintf(),
	vsnprintf()

    INTERNALS

******************************************************************************/
{
    struct StdCIOBase *StdCIOBase = __aros_getbase_StdCIOBase();
    int     retval;
    va_list args;

    D(bug("[printf]: StdCIOBase: 0x%x, stdout=0x%x\n",
          StdCIOBase, StdCIOBase->_stdout
    ));

    va_start (args, format);

    retval = vfprintf (StdCIOBase->_stdout, format, args);

    va_end (args);

    return retval;
} /* printf */
