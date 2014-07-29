/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

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
        or quoted strings separated by whitespace or '=' just like on
        the commandline. The separator is unread and the output string
        is terminated by a NUL character.

    INPUTS
        buffer   - Buffer to be filled.
        maxchars - Size of the buffer. Must be at least 1 (for the NUL
                   terminator).
        input    - A ready to use CSource structure or NULL which means
                   "read from the input stream".

    RESULT
        One of ITEM_UNQUOTED - Normal word read.
               ITEM_QUOTED   - Quoted string read.
               ITEM_NOTHING  - End of line found. Nothing read.
               ITEM_EQUAL    - '=' read. Buffer is empty.
               ITEM_ERROR    - An error happened.

    NOTES
        This function handles conversion of '**', '*"', etc. inside quotes.

        This function has well known bugs, and should be avoided
        in new applications.

    EXAMPLE

    BUGS
        1. Forgets to unread a separator character (equal sign, whitespace or
           tabulation).
        2. Tries to unread an end-of-line, which actually causes unreading the
           last read character of CSource if supplied. Even if it's not a
           separator, but belongs to the last read item.
        3. IoErr() is never modified by this function.

        As AOS programs that use ReadItem() depend on this broken behaviour,
        it will not be fixed.

        4. If maxchars == 0, buffer[0] is set to NUL anyway.

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

/*
 * WARNING!!!
 * As mentioned above, this code has some intentional (but not obvious) bugs.
 * They must not be fixed.
 * If you change something here, be sure that the code passes unit tests
 * in test/dos/readitem. Those unit tests are verified to pass on AmigaOS 3.1.
 */

/* Macro to get a character from the input source */
#define GET(c)                                  \
if(input!=NULL)                                 \
{                                               \
    if(input->CS_CurChr>=input->CS_Length)      \
        c=EOF;                                  \
    else                                        \
        c=input->CS_Buffer[input->CS_CurChr++]; \
}else                                           \
{                                               \
    c=FGetC(Input());                           \
}

/* Macro to push the character back */
#define UNGET() {if(input!=NULL) input->CS_CurChr--; else UnGetC(Input(),-1);}

    STRPTR b=buffer;
    LONG c;

    /* No buffer? */
    if (buffer == NULL)
        return ITEM_NOTHING;

    if (!maxchars) {
        *b = 0;
        return ITEM_NOTHING;
    }

    /* Skip leading whitespace characters */
    do
    {
        GET(c);
    } while (c==' '||c=='\t');

    if(!c||c=='\n'||c==EOF||c==';')
    {
        *b=0;
        if (c != EOF)
            UNGET();
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
                b[-1]=0;
                return ITEM_NOTHING;
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
                    UNGET();
                    *b=0;
                    return ITEM_ERROR;
                }else if(c=='n'||c=='N')
                    c='\n';
                else if(c=='e'||c=='E')
                    c=0x1b;
            }else if(!c||c=='\n'||c==EOF)
            {
                UNGET();
                *b=0;
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
            b[-1]=0;
            return ITEM_ERROR;
        }
        maxchars--;
        *b++=c;
        /* Read up to the next terminator. */
        for(;;)
        {
            if(!maxchars)
            {
                b[-1]=0;
                return ITEM_ERROR;
            }
            maxchars--;
            GET(c);
            /* Check for terminator */
            if(!c||c==' '||c=='\t'||c=='\n'||c=='='||c==EOF)
            {
                /* To be compatible with AOS, we need
                 * to *not* UNGET() here if we see a space
                 * or equals sign.
                 *
                 * Yes, it's broken, but so are any programs
                 * that actually used ReadItem(), and relied
                 * on this behaviour.
                 */
                if (c != '=' && c != ' ' && c != '\t')
                    UNGET();
                *b=0;
                return ITEM_UNQUOTED;
            }
            *b++=c;
        }
    }
    AROS_LIBFUNC_EXIT
} /* ReadItem */
