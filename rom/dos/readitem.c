/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.8  1998/10/20 16:44:49  hkiel
    Amiga Research OS

    Revision 1.7  1998/01/23 11:48:59  hkiel
    Made GCC-2.8.0 happy (Changed return type of main() to int, added braces
    to avoid ambiguous ´else´)

    Revision 1.6  1997/01/27 00:36:28  ldp
    Polish

    Revision 1.5  1996/12/09 13:53:39  aros
    Added empty templates for all missing functions

    Moved #include's into first column

    Revision 1.4  1996/10/24 15:50:35  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.3  1996/08/13 13:52:50  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced AROS_LA by AROS_LHA

    Revision 1.2  1996/08/01 17:40:56  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include <dos/rdargs.h>
#include <dos/dosasl.h>
#include <dos/dosextens.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH3(LONG, ReadItem,

/*  SYNOPSIS */
	AROS_LHA(STRPTR,           buffer,   D1),
	AROS_LHA(LONG,             maxchars, D2),
	AROS_LHA(struct CSource *, input,    D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 135, Dos)

/*  FUNCTION
	Read an item from a given character source. Items are words
	or quoted strings seperated by whitespace or '=' just like on
	the commandline. The seperator is unread and the read string
	is terminated by a NUL character.

    INPUTS
	buffer   - Buffer to be filled.
	maxchars - Size of the buffer. Must be at least 1 (for the terminator).
	input    - A ready to use CSource structure or NULL which means
		   "read from the input stream".

    RESULT
	One of ITEM_UNQUOTED - Normal word read.
	       ITEM_QUOTED   - Quoted string read.
	       ITEM_NOTHING  - End of line found. Nothing read.
	       ITEM_EQUAL    - '=' read. Buffer is empty.
	       ITEM_ERROR    - An error happened. IoErr() gives additional
	                       information in that case.

    NOTES
	This function handles conversion of '**', '*"', etc inside quotes.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

/* Macro to get a character from the input source */
#define GET(c) 					\
if(input!=NULL)					\
{						\
    if(input->CS_CurChr>=input->CS_Length)	\
        c=EOF;					\
    else					\
        c=input->CS_Buffer[input->CS_CurChr++];	\
}else						\
{       					\
    c=FGetC(Input());				\
    if(c==EOF&&*result)				\
        return ITEM_ERROR;			\
}

/* Macro to push the character back */
#define UNGET() {if(input!=NULL) input->CS_CurChr--; else UnGetC(Input(),-1);}

    STRPTR b=buffer;
    LONG c;
    LONG *result=&((struct Process *)FindTask(NULL))->pr_Result2;
    
    /* Skip leading whitespace characters */
    do
    {
        GET(c);
    }while(c==' '||c=='\t'||c=='\n');

    if(!c||c=='\n'||c==EOF)
    {
        /*
    	    End of line found. Note that unlike the Amiga DOS original
            this funtion doesn't know about ';' comments. Comments are
            the shell's job, IMO. I don't need them here.
        */
        if(c!=EOF)
            UNGET();
        *b=0;
        return ITEM_NOTHING;
    }else if(c=='=')
    {
        /* Found '='. Return it. */
        *b=0;
        return ITEM_EQUAL;
    }else if(c=='\"')
        /* Quoted item found. Convert Contents. */
        for(;;)
        {
            if(!maxchars)
            {
                *buffer=0;
                *result=ERROR_BUFFER_OVERFLOW;
                return ITEM_ERROR;
            }
            maxchars--;
            GET(c);
            /* Convert ** to *, *" to ", *n to \n and *e to 0x1b. */
            if(c=='*')
            {
                GET(c);
                /* Check for premature end of line. */
                if(!c||c=='\n'||c==EOF)
                {
                    if(c!=EOF)
                        UNGET();
                    *buffer=0;
                    *result=ERROR_UNMATCHED_QUOTES;
                    return ITEM_ERROR;
                }else if(c=='n'||c=='N')
                    c='\n';
                else if(c=='e'||c=='E')
                    c=0x1b;
            }else if(!c||c=='\n'||c==EOF)
            {
                if(c!=EOF)
                    UNGET();
                *buffer=0;
                *result=ERROR_UNMATCHED_QUOTES;
                return ITEM_ERROR;
            }else if(c=='\"')
            {
                /* " ends the item. */
                *b=0;
                return ITEM_QUOTED;
            }
            *b++=c;
        }
    else
    {
        /* Unquoted item. Store first character. */
        if(!maxchars)
        {
            *buffer=0;
            *result=ERROR_BUFFER_OVERFLOW;
            return ITEM_ERROR;
        }
        maxchars--;
        *b++=c;
        /* Read upto the next terminator. */
        for(;;)
        {
            if(!maxchars)
            {
                *buffer=0;
                *result=ERROR_BUFFER_OVERFLOW;
                return ITEM_ERROR;
            }
            maxchars--;
            GET(c);
            /* Check for terminator */
            if(!c||c==' '||c=='\t'||c=='\n'||c=='='||c==EOF)
            {
                if(c!=EOF)
                    UNGET();
                *b=0;
                return ITEM_UNQUOTED;
            }
            *b++=c;
        }
    }
    AROS_LIBFUNC_EXIT
} /* ReadItem */
