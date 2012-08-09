/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Format a string and emit it.
    Lang: english
*/

#include <dos/dos.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <exec/rawfmt.h>
#include <proto/exec.h>
#include <string.h>

#include <stdarg.h>

#include "exec_intern.h"
#include "exec_util.h"

#ifdef __arm__

#define is_va_list(ap) ap.__ap
#define null_va_list(ap) va_list ap = {NULL}
#define VA_NULL {NULL}

#else

#define is_va_list(ap) ap
#define null_va_list(ap) void *ap = NULL

#endif

/* Fetch the data from a va_list.

   Variables are allocated in the va_list using the default argument
   promotion rule of the C standard, which states that:

       "types char and short int are promoted to int, and float is promoted
        to double" (http://www.eskimo.com/~scs/C-faq/q15.2.html)

   That rule directly translates into relations on types sizes, rather
   than the types themselves, since sizeof(char) and sizeof(short) is always
   less than or equal to sizeof(int), and sizeof(float) is always less than
   or equal to sizeof(double). In addition, we also handle the case of
   sizeof(long), whilst the sizeof(double) case is not handled for two reasons:

       1) RawDoFmt() doesn't handle floating point values.
       2) Given (1), sizeof(type) > sizeof(long) would hold true if
          and only if type were a 64 bit pointer and long's and pointers
	  had different sizes (quite unusual).  */

#define fetch_va_arg(type)                              \
({                                                      \
    type res;                                           \
                                                        \
    if (sizeof(type) <= sizeof(int))                    \
	res = (type)(IPTR)va_arg(VaListStream, int);    \
    else                                                \
    if (sizeof(type) == sizeof(long))                   \
	res = (type)(IPTR)va_arg(VaListStream, long);   \
    else                                                \
	res = (type)(IPTR)va_arg(VaListStream, void *); \
                                                        \
    res;                                                \
})

/* Fetch an argument from memory.

   We can be sure data is always aligned to a WORD boundary,
   which is what we need.

   However, on some architectures some kind of data needs to
   have a certain alignment greater than 2 bytes, and this
   code will miserably fail if DataStream is not properly
   aligned; in such cases data should be fetched in pieces,
   taking into account the endianess of the machine.

   Currently it is assumed that ULONG values are stored
   in an array of IPTRs. This results in good portability for
   both 32- and 64-bit systems. */

#define fetch_mem_arg(type)              	\
({                                       	\
    IPTR res;					\
    						\
    if (sizeof(type) <= sizeof(short))		\
    {						\
    	res = *(short *)DataStream	;	\
    	DataStream = (short *)DataStream + 1;	\
    }						\
    else if (sizeof(type) <= sizeof(long))	\
    {						\
    	res = *(long *)DataStream;		\
    	DataStream = (long *)DataStream + 1;	\
    }						\
    (type)res;					\
})

/* Fetch the data either from memory or from the va_list, depending
   on the value of VaListStream.  */
#define fetch_arg(type) \
    (is_va_list(VaListStream) ? fetch_va_arg(type) : fetch_mem_arg(type))

/*
 * Fetch a number from the stream.
 *
 * size - one of 'h', 'l', 'i'
 * sign - <0 or >= 0.
 *
 * EXPERIMENTAL: 'i' is used to represent full IPTR value on 64-bit systems
 */
#define fetch_number(size, sign)                                                               \
    (sign >= 0                                                                                 \
     ? (size == 'i' ? fetch_arg(IPTR)  : (size == 'l' ? fetch_arg(ULONG) : fetch_arg(UWORD)))   \
     : (size == 'i' ? fetch_arg(SIPTR) : (size == 'l' ? fetch_arg(LONG)  : fetch_arg(WORD))))

/* Call the PutCharProc funtion with the given parameters.  */
#define PutCh(ch)                         \
do                                        \
{                                         \
    switch ((IPTR)PutChProc)              \
    {                                     \
    case (IPTR)RAWFMTFUNC_STRING:	  \
	*(PutChData++) = ch;               \
	break;				  \
    case (IPTR)RAWFMTFUNC_SERIAL:	  \
	RawPutChar(ch);			  \
	break;				  \
    case (IPTR)RAWFMTFUNC_COUNT:	  \
	(*((ULONG *)PutChData))++;	  \
	break;				  \
    default:				  \
	if (is_va_list(VaListStream))			\
	{						\
	    APTR (*proc)(APTR, UBYTE) = (APTR)PutChProc;\
	    PutChData = proc((APTR)PutChData, ch);	\
	}						\
	else						\
	{						\
        AROS_UFC2NR(void, PutChProc,        		\
	    AROS_UFCA(UBYTE, (ch), D0),       		\
	    AROS_UFCA(APTR , PutChData, A3)); 		\
	}						\
    }                                     \
} while (0)

/*
 * DataStream == NULL can't be used to select between new or old style PutChProc() because
 * RawDoFmt(<string without parameters>, NULL, PutChProc, PutChData); is valid and used by
 * m68k programs.
 * In order to get around we use specially formed va_list with NULL value.
 */

APTR InternalRawDoFmt(CONST_STRPTR FormatString, APTR DataStream, VOID_FUNC PutChProc,
		      APTR inPutChData, va_list VaListStream)
{
#if defined(mc68000)
    /* Frequently, AmigaOS users of RawDoFmt() rely upon the AmigaOS
     * behaviour that A3 *in this routine* is the pointer to PutChData,
     * *and* that it can be modified in PutChProc.
     */
    register volatile UBYTE  *PutChData asm("%a3");
#else
    UBYTE *PutChData = inPutChData;
#endif

    /* As long as there is something to format left */
    while (*FormatString)
    {
	/* Check for '%' sign */
	if (*FormatString == '%')
	{
	    /*
		left	 - left align flag
		fill	 - pad character
		minus	 - 1: number is negative
		minwidth - minimum width
		maxwidth - maximum width
		size	 - one of 'h', 'l', 'i'.
		width	 - width of printable string
		buf	 - pointer to printable string
	    */
	    int left  = 0;
	    int fill  = ' ';
	    int minus = 0;
	    int size  = 'h';
	    ULONG minwidth = 0;
	    ULONG maxwidth = ~0;
	    ULONG width    = 0;
	    UBYTE *buf;

            /* Number of decimal places required to convert a unsigned long to
               ascii. The formula is: ceil(number_of_bits*log10(2)).
	       Since I can't do this here I use .302 instead of log10(2) and
	       +1 instead of ceil() which most often leads to exactly the
	       same result (and never becomes smaller).

	       Note that when the buffer is large enough for decimal it's
	       large enough for hexadecimal as well.  */

	    #define CBUFSIZE (sizeof(IPTR)*8*302/1000+1)
	    /* The buffer for converting long to ascii.  */
	    UBYTE cbuf[CBUFSIZE];
	    ULONG i;

	    /* Skip over '%' character */
	    FormatString++;

	    /* '-' modifier? (left align) */
	    if (*FormatString == '-')
		left = *FormatString++;

	    /* '0' modifer? (pad with zeros) */
	    if (*FormatString == '0')
		fill = *FormatString++;

	    /* Get minimal width */
	    while (*FormatString >= '0' && *FormatString <= '9')
	    {
	        minwidth = minwidth * 10 + (*FormatString++ - '0');
	    }

	    /* Dot following width modifier? */
	    if(*FormatString == '.')
	    {
		FormatString++;
		/* Get maximum width */

		if(*FormatString >= '0' && *FormatString <= '9')
		{
		    maxwidth = 0;
		    do
			maxwidth = maxwidth *10 + (*FormatString++ - '0');
		    while (*FormatString >= '0' && *FormatString <= '9');
		}
	    }

	    /* size modifiers */
	    switch (*FormatString)
	    {
	    case 'l':
	    case 'i':
	    	size = *FormatString++;
		break;
	    }

	    /* Switch over possible format characters. Sets minus, width and buf. */
	    switch(*FormatString)
	    {
		/* BCPL string */
		case 'b':
                {
                    BSTR s = fetch_arg(BSTR);
                    
                    if (s)
                    {
		    	buf = AROS_BSTR_ADDR(s);
		    	width = AROS_BSTR_strlen(s);
		    }
		    else
		    {
		    	buf = "";
		    	width = 0;
		    }

		    break;
                }

		/* C string */
		case 's':
  		    buf = fetch_arg(UBYTE *);

                    if (!buf)
                        buf = "";
		    width = strlen(buf);

		    break;
		{
		    IPTR number = 0; int base;
		    static const char digits[] = "0123456789ABCDEF";

		    case 'p':
		    case 'P':
			fill = '0';
			minwidth = sizeof(APTR)*2;
			size = 'i';
		    case 'x':
		    case 'X':
		        base   = 16;
			number = fetch_number(size, 1);

                        goto do_number;

		    case 'd':
		    case 'D':
		        base   = 10;
  		        number = fetch_number(size, -1);
			minus  = (SIPTR)number < 0;

			if (minus) number = -number;

			goto do_number;

		    case 'u':
		    case 'U':
		        base = 10;
  		        number = fetch_number(size, 1);

		    do_number:

		        buf = &cbuf[CBUFSIZE];
			do
			{
  		            *--buf = digits[number % base];
			    number /= base;
		            width++;
			} while (number);

		    break;
		}


		/* single character */
		case 'c':
		    /* Some space for the result */
		    buf   = cbuf;
		    width = 1;

		    *buf = fetch_number(size, 1);

		    break;

		/* '%' before '\0'? */
		case '\0':
		    /*
			This is nonsense - but do something useful:
			Instead of reading over the '\0' reuse the '\0'.
		    */
		    FormatString--;
		    /* Get compiler happy */
		    buf = NULL;
		    break;

		/* Convert '%unknown' to 'unknown'. This includes '%%' to '%'. */
		default:
		    buf   = (UBYTE *)FormatString;
		    width = 1;
		    break;
	    }

	    if (width > maxwidth) width = maxwidth;

	    /* Skip the format character */
	    FormatString++;

	    /*
		Now everything I need is known:
		buf	 - contains the string to be printed
		width	 - the size of the string
		minus	 - is 1 if there is a '-' to print
		fill	 - is the pad character
		left	 - is 1 if the string should be left aligned
		minwidth - is the minimal width of the field
		(maxwidth is already part of width)

		So just print it.
	    */

	    /* Print '-' (if there is one and the pad character is no space) */
	    if (minus && fill != ' ')
	        PutCh('-');

	    /* Pad left if not left aligned */
	    if (!left)
		for (i = width + minus; i < minwidth; i++)
		    PutCh(fill);

	    /* Print '-' (if there is one and the pad character is a space) */
	    if(minus && fill == ' ')
                PutCh('-');

	    /* Print body upto width */
	    for(i=0; i<width; i++) {
	        PutCh(*buf);
		buf++;
	    }

	    /* Pad right if left aligned */
	    if(left)
		for(i = width + minus; i<minwidth; i++)
		    PutCh(fill);
	}
	else
	{
	    /* No '%' sign? Put the formatstring out */
	    PutCh(*FormatString);
	    FormatString++;
	}
    }
    /* All done. Put the terminator out. */
    PutCh('\0');

    /* Return the rest of the DataStream or buffer. */
    return is_va_list(VaListStream) ? (APTR)PutChData : DataStream;
}

/*****************************************************************************

    NAME */

	AROS_LH4I(APTR,RawDoFmt,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, FormatString, A0),
	AROS_LHA(APTR,         DataStream,   A1),
	AROS_LHA(VOID_FUNC,    PutChProc,    A2),
	AROS_LHA(APTR,         PutChData,    A3),

/*  LOCATION */
	struct ExecBase *, SysBase, 87, Exec)

/*  FUNCTION
	printf-style formatting function with callback hook.

    INPUTS
	FormatString - Pointer to the format string with any of the following
		       DataStream formatting options allowed:

		       %[leftalign][minwidth.][maxwidth][size][type]

		       leftalign - '-' means align left. Default: align right.
		       minwidth  - minimum width of field. Defaults to 0.
		       maxwidth  - maximum width of field (for strings only).
				   Defaults to no limit.

		       size	 - 'l' means LONG. Defaults to WORD, if nothing is specified.

		       type	 - 'b' BSTR. It will use the internal representation
                                       of the BSTR defined by the ABI.
				   'c' single character.
				   'd' signed decimal number.
				   's' C string. NULL terminated.
				   'u' unsigned decimal number.
				   'x' unsigned hexadecimal number.
				   'P' pointer. Size depends on the architecture.
				   'p' The same as 'P', for AmigaOS v4 compatibility.

	DataStream   - Pointer to a zone of memory containing the data. Data has to be
	               WORD aligned.

	PutChProc    - Callback function. In caseCalled for each character, including
		       the NULL terminator. The fuction is called as follow:

                       AROS_UFC2(void, PutChProc,
                                 AROS_UFCA(UBYTE, char,      D0),
                                 AROS_UFCA(APTR , PutChData, A3));
		    
		       Additionally, PutChProc can be set to one of the following
		       magic values:

			 RAWFMTFUNC_STRING - Write output to string buffer pointed
					     to by PutChData which is incremented
					     every character.
			 RAWFMTFUNC_SERIAL - Write output to debug output. PutChData
					     is ignored and not touched.
			 RAWFMTFUNC_COUNT  - Count number of characters in the result.
					     PutChData is a pointer to ULONG which
					     is incremented every character. Initial
					     value of the cointer is kept as it is.

		       If you want to be compatible with AmigaOS you
		       should check that exec.library has at least version 45.

	PutChData    - Data propagated to each call of the callback hook.

    RESULT
	Pointer to the rest of the DataStream.

    NOTES
	The field size defaults to WORD which may be different from the
	default integer size of the compiler. If you don't take care about
	this the result will be messy.
	
	There are different solutions for GCC:
	- Define Datastream between #pragma pack(2) / #pragma pack().
	- Use __attribute__((packed)) for Datastream.
	- Only use type of LONG/ULONG for integer variables. Additionally only use
	  %ld/%lu in FormatString.

    EXAMPLE
	Build a sprintf style function:

	    __stackparm void my_sprintf(UBYTE *buffer, UBYTE *format, ...);

	    static void callback(UBYTE chr __reg(d0), UBYTE **data __reg(a3))
	    {
	       *(*data)++=chr;
	    }

	    void my_sprintf(UBYTE *buffer, UBYTE *format, ...)
	    {
	        RawDoFmt(format, &format+1, &callback, &buffer);
            }

	The above example uses __stackparm attribute in the function
	prototype in order to make sure that arguments are all passed on
	the stack on all architectures. The alternative is to use
	VNewRawDoFmt() function which takes va_list instead of array
	DataStream.

    BUGS
	PutChData cannot be modified from the callback hook on non-m68k
	systems.

    SEE ALSO

    INTERNALS
	In AROS this function supports also 'i' type specifier
	standing for full IPTR argument. This makes difference on
	64-bit machines. At the moment this addition is not stable
	and subject to change. Consider using %P or %p to output
	full 64-bit pointers.

	When locale.library starts up this function is replaced
	with advanced version, supporting extensions supported
	by FormatString() function.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    null_va_list(vaListStream);

    return InternalRawDoFmt(FormatString, DataStream, PutChProc, PutChData, vaListStream);

    AROS_LIBFUNC_EXIT
} /* RawDoFmt */
