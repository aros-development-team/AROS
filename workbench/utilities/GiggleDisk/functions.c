
/*
** functions.c
**
** (c) 1998-2011 Guido Mersmann
** (c) 2011 The AROS Development Team.
*/

/*************************************************************************/

#define SOURCENAME "functions.c"
#define NODEBUG  /* turns off debug of this file */

#include <internal/debug.h>
#include <internal/memory.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <devices/serial.h>
#include <devices/parallel.h>
#include <devices/input.h>
#include <devices/keyboard.h>
#include <exec/io.h>

#include "functions.h"
#include "locale_strings.h"
#include "locale.h"

/*************************************************************************/

/*
** string functions
*/

/* /// String_HexToLong()
**
*/

/*************************************************************************/

long String_HexToLong( STRPTR hexstring, ULONG nibbles )
{
BYTE chr;
long result = 0;

    if( !nibbles ) {
        nibbles = 8; /* default is 8 */
    }

    while( nibbles-- ) {
        chr = *hexstring++;
        if( chr && ( chr != ' ' ) && ( chr != 9 ) ) {
            if( ( chr >= '0' ) && ( chr <= '9' ) ) {
                chr -= 0x30;
            } else {
                chr = (chr & 0x0f) + 9;
                if( (chr > 16) ) { break; }
            }
			result = ( ( result << 4 ) | chr );
        } else {
            break;
        }
    }
	return( result );
}
/* \\\ */
/* /// String_HexToLongPreFix()
**
*/

/*************************************************************************/

long String_HexToLongPreFix( STRPTR hexstring, ULONG nibbles UNUSED )
{

	if( hexstring[0] == '0' && (hexstring[1] == 'x' || hexstring[1] == 'X' )  ) {
		return( String_HexToLong( &hexstring[2], 0 ) );
    }

    if( hexstring[0] == '$' ) {
		return( String_HexToLong( &hexstring[1], 0 ) );
    }
	return( 0L );
}
/* \\\ */
/* /// String_CopySBSTR()
**
*/

/*************************************************************************/

void String_CopySBSTR( STRPTR source, STRPTR dest)
{
long i;

/* Read string size */
    if( (i = *source++)) {
        for( ; i ; i--) {
            *dest++ = *source++;
        }
    }
    *dest = 0x00;

}
/* \\\ */

/*
** dos functions
*/

/* /// Dos_CheckExistsDrawer()
**
*/

/*************************************************************************/

BOOL Dos_CheckExistsDrawer( STRPTR dir )
{
struct FileInfoBlock *fileinfoblock;
BPTR lock;
ULONG result;

    result = FALSE;

	if( (lock = Lock( dir, ACCESS_READ ) ) ) {
		if( (fileinfoblock = AllocDosObject( DOS_FIB, TAG_DONE ) ) ) {
            if( Examine( lock, fileinfoblock) ) {
                if( fileinfoblock->fib_DirEntryType >= 0 ) {
                    result = TRUE;
                }
            }
			FreeDosObject( DOS_FIB, fileinfoblock );
        }
		UnLock( lock );
    }
	return( result );
}
/* \\\ */

/*
** Exec Lists
*/

/* /// List_Free()
**
*/

/*************************************************************************/

void List_Free( struct List *list )
{
struct Node* node;
	if( list->lh_Head ) {

        while( (node = list->lh_Head)->ln_Succ ) {
            Remove( node );
            Memory_FreeVec( node );
        }
    }
}
/* \\\ */

