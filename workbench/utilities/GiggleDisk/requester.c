
/*
** requester.c
**
** (c) 1998-2011 Guido Mersmann
**
** This file contains requester handling. When MUI is available it's
** requester will be used. If not a simple EasyRequest opens.
*/

/*************************************************************************/

#define SOURCENAME "requester.c"
#define NODEBUG  /* turns off debug of this file */

#include <internal/debug.h>
#include <internal/memory.h>

#include <proto/intuition.h>
#include <proto/locale.h>
#include <proto/dos.h>

#include "version.h"
#include "sprintf.h"
#include "locale.h"
#include "locale_strings.h"

/*************************************************************************/

ULONG requester_args[16];

/*************************************************************************/

/* /// Requester_ShowErrorCLI()
**
*/

/*************************************************************************/

ULONG Requester_ShowErrorCLI(ULONG msg)
{
#define REQBUFFER_SIZEOF 0x200

char buffer[REQBUFFER_SIZEOF+1];

STRPTR str = buffer;

    if( msg ) {

        SPrintfn( Locale_GetString( msg), buffer, REQBUFFER_SIZEOF, requester_args);

        do {
            if( *str == 0x0a ) {
                *str = ' ';
            }
        } while( *str++ );

        *(str-1) = 0x0a;
        *str     = 0x00;
        PutStr( app_appname );
        PutStr( ": " );
        PutStr( buffer );

    }
    return( msg );
}
/* \\\ Requester_ShowErrorCLI */

