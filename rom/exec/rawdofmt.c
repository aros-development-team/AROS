/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:27:14  digulla
    Added copyright notics and made headers conform

    Desc:
    Lang: english
*/
#include <dos/dos.h>
#include "machine.h"
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH4I(APTR,RawDoFmt,

/*  SYNOPSIS */
	__AROS_LA(STRPTR,    FormatString, A0),
	__AROS_LA(APTR,      DataStream,   A1),
	__AROS_LA(VOID_FUNC, PutChProc,    A2),
	__AROS_LA(APTR,      PutChData,    A3),

/*  LOCATION */
	struct ExecBase *, SysBase, 87, Exec)

/*  FUNCTION
	printf-style formatting function with callback hook.

    INPUTS
	FormatString - Pointer to the format string with any of the following
		       stream formatting options allowed:

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
				   's' C string. NUL terminated.
				   'u' unsigned decimal number.
				   'x' unsigned sedecimal number.

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
	PutChData cannot be modified from the callback hook.

    SEE ALSO

    INTERNALS

    HISTORY
	22-10-95    created by m. fleischer

******************************************************************************/
{
    __AROS_FUNC_INIT
    /* Cast for easier access */
    ULONG stream=(ULONG)DataStream;

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
		    minwidth=minwidth*10+(*FormatString++-'0');
	        while(*FormatString>='0'&&*FormatString<='9');

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
	        else
	        {
		    /* No. It was in fact a maxwidth modifier */
		    maxwidth=minwidth;
		    minwidth=0;
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
		    /* Get address, but align datastream first */
		    if(LONGALIGN>WORDALIGN)
			stream=(stream+LONGALIGN-1)&~(LONGALIGN-1);
		    buf=(UBYTE *)BADDR(*(BPTR *)stream);
		    stream+=sizeof(BPTR);

		    /* Set width */
		    width=*buf++;

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
			    /* Align datastream */
			    if(LONGALIGN>WORDALIGN)
				stream=(stream+LONGALIGN-1)&~(LONGALIGN-1);
			    /*
				For longs reading signed and unsigned
				doesn't make a difference.
			    */
			    n=*(ULONG *)stream;
			    stream+=sizeof(ULONG);

			    /* But for words it may do. */
			}else
			    /*
				Sorry - it may not: Stupid exec always treats
				UWORD as WORD even when 'u' is used.
			    */
#ifdef FIX_EXEC_BUGS
			    if(*FormatString=='d')
#else
			    if(1)
#endif
			    {
				n=*(WORD *)stream;
				stream+=sizeof(WORD);
			    }else
			    {
				n=*(UWORD *)stream;
				stream+=sizeof(UWORD);
			    }

			/* Negative number? */
			if(*FormatString=='d'&&(LONG)n<0)
			{
			    minus=1;
			    n=-n;
			}

			/* Convert to ASCII */
			buf=&cbuf[CBUFSIZE];
			do
			{
			    ULONG r;

			    /*
				divide 'n' by 10 and get quotient 'n'
				and remainder 'r'
			    */
			    UDIVMOD10(n,n,r);
			    *--buf=r+'0';
			    width++;
			}while(n);
		    }
		    break;

		/* unsigned sedecimal value */
		case 'x':
		    {
			ULONG n;

			/* Get value */
			if(larg)
			{
			    /* Align datastream */
			    if(LONGALIGN>WORDALIGN)
				stream=(stream+LONGALIGN-1)&~(LONGALIGN-1);
			    n=*(ULONG *)stream;
			    stream+=sizeof(ULONG);
			}else
			{
			    n=*(UWORD *)stream;
			    stream+=sizeof(UWORD);
			}

			/* Convert to ASCII */
			buf=&cbuf[CBUFSIZE];
			do
			{
			    /*
				Uppercase characters for lowercase 'x'?
				Stupid exec original!
			    */
			    *--buf="0123456789ABCDEF"[n&15];
			    n>>=4;
			    width++;
			}while(n);
		    }
		    break;

		/* C string */
		case 's':
		    {
			UBYTE *buffer;

			/* Get address, but align datastream first */
			if(PTRALIGN>WORDALIGN)
			    stream=(stream+PTRALIGN-1)&~(PTRALIGN-1);
			buf=*(UBYTE **)stream;
			stream+=sizeof(UBYTE *);

			/* width=strlen(buf) */
			buffer=buf;
			while(*buffer++)
			    ;
			width=~(buf-buffer);

			/* Strings may be modified with the maxwidth modifier */
			if(width>maxwidth)
			    width=maxwidth;
		    }
		    break;

		/* single character */
		case 'c':
		    /* Some space for the result */
		    buf=cbuf;
		    width=1;

		    /* Get value */
		    if(larg)
		    {
			/* Align datastream */
			if(LONGALIGN>WORDALIGN)
			    stream=(stream+LONGALIGN-1)&~(LONGALIGN-1);
			*buf=*(ULONG *)stream;
			stream+=sizeof(ULONG);
		    }else
		    {
			*buf=*(UWORD *)stream;
			stream+=sizeof(UWORD);
		    }
		    break;

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
		    buf=FormatString;
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
#ifdef FIX_EXEC_BUGS
	    /* Print '-' (if there is one and the pad character is no space) */
	    if(minus&&fill!=' ')
		RDFCALL(PutChProc,'-',PutChData);
#endif
	    /* Pad left if not left aligned */
	    if(!left)
		for(i=width+minus;i<minwidth;i++)
		    RDFCALL(PutChProc,fill,PutChData);

	    /* Print '-' (if there is one and the pad character is a space) */
#ifdef FIX_EXEC_BUGS
	    if(minus&&fill==' ')
#else
	    if(minus)
#endif
		RDFCALL(PutChProc,'-',PutChData);

	    /* Print body upto width */
	    for(i=0;i<width;i++)
	    {
		RDFCALL(PutChProc,*buf,PutChData);
		buf++;
	    }

	    /* Pad right if left aligned */
	    if(left)
		for(i=width+minus;i<minwidth;i++)
		    /* Pad right with '0'? Sigh - if the user wants to! */
		    RDFCALL(PutChProc,fill,PutChData);
	}else
	{
	    /* No '%' sign? Put the formatstring out */
	    RDFCALL(PutChProc,*FormatString,PutChData);
	    FormatString++;
	}
    }
    /* All done. Put the terminator out. */
    RDFCALL(PutChProc,'\0',PutChData);

    /* Return the rest of the datastream. */
    return (APTR)stream;
    __AROS_FUNC_EXIT
} /* RawDoFmt */

