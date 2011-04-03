/*
** GiggleDisk
**
**
** (c) 1998-2011 Guido Mersmann
*/

/*************************************************************************/

#define SOURCENAME "main.c"
#define NODEBUG  /* turns off debug of this file */

#include <internal/debug.h>
#include <internal/memory.h>

#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/dos.h>

#include <exec/exec.h>

#define CATCOMP_BLOCK 1     /* enable CATCOMP_BLOCK */
#include "cache.h"
#include "giggledisk.h"
#include "library.h"
#include "locale.h"
#include "locale_strings.h"
#include "main.h"
#include "process.h"
#include "readargs.h"
#include "requester.h"
#include "version.h"
#include "wbstartup.h"

/*************************************************************************/

char nil[NIL_SIZEOF];

long main_flags;

/*************************************************************************/

/* /// Main()
**
*/

/*************************************************************************/

int main( void )
{
ULONG result;

#ifndef __amigaos4__
SysBase = (*((struct ExecBase **) 4));
#endif
/* Call resource tracking module to init resource tracking */

    WBMessage_Get();
	Process_SetWindowPtr( (APTR) -1 );

	Locale_Open( APPLICATIONNAME ".catalog", VERSION, REVISION );

    result = MSG_Error_UnableToOpenLibrary;
    if( !(Libraries_Open()) ) {
        result = 0;
        if( ReadArgs_ReadArgs() ) {
            result = GiggleDisk();
        }
    }
	debug("Result: %ld\n", result );
    if( result && DOSBase ) {
		Requester_ShowErrorCLI( result );
        result = 20;
    } else {
        result = 0;
    }

    ReadArgs_FreeArgs();
    Locale_Close();
    Libraries_Close();
    Process_ClearWindowPtr();

/* Call resource tracking module to find any lost resources */

    WBMessage_Reply();

	return( result );
}
/* \\\ */

/*
** debug code
*/

#include "internal/debug.c"

