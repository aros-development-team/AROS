/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Format a string and emit it.
    Lang: english
*/
#include <dos/dos.h>
#include <aros/machine.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <proto/exec.h>

#include <stdarg.h>

#define align_arg(ptr, type) \
    ((APTR)(((IPTR)ptr + __alignof__(type) - 1) & ~(__alignof__(type) - 1)))

#define stack_arg(DataStream, type)           \
({                                            \
    DataStream = align_arg(DataStream, type); \
                                              \
    *(((type *)DataStream)++);                \
})

/* Macro to fetch the data from the stream. The stream can either be a
   va_list or stack memory. Variables are put on the stack following
   the default argument promotion rule of the C standard, which states that:

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
	  had different sizes (quite unusual).

   This code assumes that alignment requirements are decided only on
   the basis of the type sizes, without considering the types themselves:
   whether this is a safe assumption, I really don't know, I just know
   that for now it works :-).  */

#define fetch_arg(DataStream, type)                                     \
({                                                                      \
    type res;                                                           \
                                                                        \
    if (sizeof(type) <= sizeof(int))                                    \
    {                                                                   \
        if (in_va_list)                                                 \
	    res = (type)va_arg((*(va_list *)DataStream), int);          \
	else                                                            \
	    res = (type)stack_arg((DataStream), int);                   \
    }                                                                   \
    else                                                                \
    if (sizeof(type) == sizeof(long))                                   \
    {                                                                   \
        if (in_va_list)                                                 \
	    res = (type)va_arg((*(va_list *)DataStream), long);         \
	else                                                            \
	    res = (type)stack_arg((DataStream), long);                  \
    }                                                                   \
    else                                                                \
    {                                                                   \
        if (in_va_list)                                                 \
	    res = (type)(IPTR)va_arg((*(va_list *)DataStream), void *); \
	else                                                            \
	    res = (type)(IPTR)stack_arg((DataStream), void *);          \
    }                                                                   \
                                                                        \
    res;                                                                \
})

#define PutCh(ch, data)           \
    AROS_UFC2(void, PutChProc,    \
    AROS_UFCA(UBYTE, (ch),   D0), \
    AROS_UFCA(APTR , (data), A3)) \

#define FIX_EXEC_BUGS 0
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
		       size	 - 'l' means longword. Defaults to word.
		       type	 - 'b' BCPL string. A BPTR to a one byte
				       byte count followed by the characters.
				   'c' single character.
				   'd' signed decimal number.
				   's' C string. NULL terminated.
				   'u' unsigned decimal number.
				   'x' unsigned sedecimal number.

		       As an AROS extension, the following special options
		       are allowed:

		       %[type]

		       type - 'v' means that the current data in the DataStream is a pointer
		                  to a va_list type, as defined in <stdarg.h>.

			          From this point on the data is fetched from the va_list
			          and not from the original DataStream array anymore.

			          If the pointer is NULL, however, then nothing changes
				  and data fetching proceeds as if no %v had been specified
				  at all.

		            - 'V' it's like 'v', but requires an additional parameter which,
			          if non NULL, instructs RawDoFmt to switch to another
				  format string, whose address is the value of the parameter.

	DataStream   - Array of the data items.
	PutChProc    - Callback function. Called for each character, including
		       the NUL terminator.
	PutChData    - Data propagated to each call of the callback hook.

    RESULT
	Pointer to the rest of the DataStream.

    NOTES
	The field size defaults to words which may be different from the
	default integer size of the compiler.

    EXAMPLE
	build a sprintf style function

	static void callback(UBYTE chr, UBYTE **data)
	{   *(*data)++=chr;   }

	void my_sprintf(UBYTE *buffer, UBYTE *format, ...)
	{   RawDoFmt(format, &format+1, &callback, &buffer);   }

    BUGS
	PutChData cannot be modified from the callback hook on non-m68k
	systems.

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    int in_va_list = 0;

    /* As long as there is something to format left */
    while(*FormatString)
    {
	/* Check for '%' sign */
	if(*FormatString=='%')
	{
	    /*
		left	 - left align flag
		fill	 - pad character
		minus	 - 1: number is negative
		minwidth - minimum width
		maxwidth - maximum width
		larg	 - long argument flag
		width	 - width of printable string
		buf	 - pointer to printable string
	    */
	    int left=0;
	    int fill=' ';
	    int minus=0;
	    ULONG minwidth=0;
	    ULONG maxwidth=~0;
	    int larg=0;
	    ULONG width=0;
	    UBYTE *buf;

	    /*
		Number of decimal places required to convert a unsigned long to
		ascii. The formula is: ceil(number_of_bits*log10(2)).
		Since I can't do this here I use .302 instead of log10(2) and
		+1 instead of ceil() which most often leads to exactly the
		same result (and never becomes smaller).

		Note that when the buffer is large enough for decimal it's
		large enough for sedecimal as well.
	    */
#define CBUFSIZE (sizeof(ULONG)*8*302/1000+1)
	    /* The buffer for converting long to ascii */
	    UBYTE cbuf[CBUFSIZE];
	    ULONG i;

	    /* Skip over '%' character */
	    FormatString++;

	    /* Possibly switch to a va_list type stream.  */
	    if (*FormatString == 'v')
	    {
	        va_list *list_ptr = fetch_arg(DataStream, va_list *);
		if (list_ptr != NULL)
		{
	            in_va_list = 1;
		    DataStream = (APTR)list_ptr;
		}

		FormatString++;
		continue;
	    }

	    /* Possibly switch to a va_list type stream and also to a new
	       format string.  */
	    if (*FormatString == 'V')
	    {
	        va_list *list_ptr   = fetch_arg(DataStream, va_list *);
	        char    *new_format = fetch_arg(DataStream, char *);

                FormatString++;

		if (list_ptr != NULL)
		{
	            in_va_list = 1;
		    DataStream = (APTR)list_ptr;
		}

		if (new_format != NULL)
		    FormatString = new_format;

		continue;
	    }

	    /* '-' modifier? (left align) */
	    if(*FormatString=='-')
		left=*FormatString++;

	    /* '0' modifer? (pad with zeros) */
	    if(*FormatString=='0')
		fill=*FormatString++;

	    /* Get minimal width */
	    if(*FormatString>='0'&&*FormatString<='9')
	    {
		do
		{
		    minwidth=minwidth*10+(*FormatString++-'0');
		} while(*FormatString>='0'&&*FormatString<='9');
	    }

	    /* Dot following width modifier? */
	    if(*FormatString=='.')
	    {
		FormatString++;
		/* Get maximum width */
		if(*FormatString>='0'&&*FormatString<='9')
		{
		    maxwidth=0;
		    do
			maxwidth=maxwidth*10+(*FormatString++-'0');
		    while(*FormatString>='0'&&*FormatString<='9');
		}
	    }

	    /* 'l' modifier? (long argument) */
	    if(*FormatString=='l')
		larg=*FormatString++;

	    /* Switch over possible format characters. Sets minus, width and buf. */
	    switch(*FormatString)
	    {
		/* BCPL string */
		case 'b':
		    buf = BADDR(fetch_arg(DataStream, BPTR));

		    /* Set width */
		    width = *buf++;

		    /* Strings may be modified with the maxwidth modifier */
		    if(width>maxwidth)
			width=maxwidth;
		    break;

		/* signed decimal value */
		case 'd':
		/* unsigned decimal value */
		case 'u':
		    {
			ULONG n;

			/* Get value */
			if(larg)
			{
			    /*
				For longs reading signed and unsigned
				doesn't make a difference.
			    */
			    n = fetch_arg(DataStream, ULONG);

			    /* But for words it may do. */
			}else
			    /*
				Sorry - it may not: Stupid exec always treats
				UWORD as WORD even when 'u' is used.
			    */
			    if((FIX_EXEC_BUGS && *FormatString=='d') || !FIX_EXEC_BUGS)
			        n = fetch_arg(DataStream, WORD);
			    else
			        n = fetch_arg(DataStream, WORD);

			/* Negative number? */
			if(*FormatString=='d'&&(LONG)n<0)
			{
			    minus=1;
			    n=-n;
			}

			/* Convert to ASCII */
			{
			    buf=&cbuf[CBUFSIZE];
			    do
			    {
				/*
				 * divide 'n' by 10 and get quotient 'n'
				 * and remainder 'r'
				 */
				*--buf=(n%10)+'0';
				n/=10;
				width++;
			    }while(n);
			}
		    }
#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
		    /*
			If maxwidth is set (illegal for numbers), assume they
			forgot the '.' (Example: C:Avail)
		    */
		    if (maxwidth != ~0)
		    {
			minwidth=maxwidth;
		    }
#endif
		    break;

		/* unsigned sedecimal value */
		case 'x':
		    {
			ULONG n;

			/* Get value */
			if(larg)
			    n = fetch_arg(DataStream, ULONG);
			else
			    n = fetch_arg(DataStream, UWORD);

			/* Convert to ASCII */
			{
			    buf=&cbuf[CBUFSIZE];
			    do
			    {
				/*
				 * Uppercase characters for lowercase 'x'?
				 * Stupid exec original!
				 */
				*--buf="0123456789ABCDEF"[n&15];
				n>>=4;
				width++;
			    }while(n);
			}
		    }
#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
		    if (maxwidth != ~0)
		    {
			minwidth=maxwidth;
		    }
#endif
		    break;

		/* C string */
		case 's':
		    {
			UBYTE *buffer;

			buf = fetch_arg(DataStream, UBYTE *);

                        if (!buf)
                        {
                            buf   = "(null)";
                            width = 7;
                        }
                        else
                        {
			    /* width=strlen(buf) */
			    buffer=buf;
			    while(*buffer++);
			    width=~(buf-buffer);
                        }

			/* Strings may be modified with the maxwidth modifier */
			if(width>maxwidth)
			    width=maxwidth;
		    }
		    break;

		/* single character */
		case 'c':
		{
		    /* Some space for the result */
		    buf=cbuf;
		    width=1;

		    /* Get value */
		    if(larg)
		    {
			*buf = fetch_arg(DataStream, ULONG);
		    }else
		    {
			*buf = fetch_arg(DataStream, UWORD);
		    }
#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
		    if (maxwidth != ~0)
		    {
			minwidth=maxwidth;
		    }
#endif
		    break;
		}
		/* '%' before '\0'? */
		case '\0':
		    /*
			This is nonsense - but do something useful:
			Instead of reading over the '\0' reuse the '\0'.
		    */
		    FormatString--;
		    /* Get compiler happy */
		    buf=NULL;
		    break;

		/* Convert '%unknown' to 'unknown'. This includes '%%' to '%'. */
		default:
		    buf=(UBYTE *)FormatString;
		    width=1;
		    break;
	    }
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

	    /*
		Stupid exec always prints the '-' sign directly before
		the decimals. Even if the pad character is a '0'.
	    */

	    /* Print '-' (if there is one and the pad character is no space) */
	    if(FIX_EXEC_BUGS && minus&&fill!=' ')
	        PutCh('-', PutChData);

	    /* Pad left if not left aligned */
	    if(!left)
		for(i=width+minus;i<minwidth;i++)
		    PutCh(fill, PutChData);

	    /* Print '-' (if there is one and the pad character is a space) */
	    if(FIX_EXEC_BUGS && (minus&&fill == ' ') || (!FIX_EXEC_BUGS && minus))
                PutCh('-', PutChData);

	    /* Print body upto width */
	    for(i=0; i<width; i++)
	        PutCh(*(buf++), PutChData);

	    /* Pad right if left aligned */
	    if(left)
		for(i=width+minus;i<minwidth;i++)
		    /* Pad right with '0'? Sigh - if the user wants to! */
		    PutCh(fill, PutChData);
	}
	else
	{
	    /* No '%' sign? Put the formatstring out */
	    PutCh(*(FormatString++), PutChData);
	}
    }
    /* All done. Put the terminator out. */
    PutCh('\0', PutChData);

    /* Return the rest of the DataStream. */
    return DataStream;

    AROS_LIBFUNC_EXIT
} /* RawDoFmt */

