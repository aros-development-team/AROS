/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:40:56  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>
#include <dos/rdargs.h>
#include <dos/dosasl.h>
#include <dos/dosextens.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH3(LONG, ReadItem,

/*  SYNOPSIS */
	__AROS_LA(STRPTR,           buffer,   D1),
	__AROS_LA(LONG,             maxchars, D2),
	__AROS_LA(struct CSource *, input,    D3),

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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct DosLibrary *,DOSBase)

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
#define UNGET() if(input!=NULL) input->CS_CurChr--; else UnGetC(Input(),-1);

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
    __AROS_FUNC_EXIT
} /* ReadItem */
