#ifndef FUNCTIONS_H
#define FUNCTIONS_H 1

/*************************************************************************/

#include <exec/ports.h>
#include <exec/lists.h>
#include <exec/devices.h>

#ifdef __AROS__
#include "io_aros.h"
#else
#include "io_builtin.h"
#endif

/*************************************************************************/

/*
** prototypes
*/

long String_HexToLong( STRPTR hexstring, ULONG nibbles );
long String_HexToLongPreFix( STRPTR hexstring, ULONG nibbles );
void String_CopySBSTR( STRPTR source, STRPTR dest );

BOOL Dos_CheckExistsDrawer( STRPTR dir );

BOOL Device_Open (struct MDevice *mdevice, char *name, ULONG unit, ULONG flags, ULONG iosize );
void Device_Close(struct MDevice *mdevice );

void List_Free( struct List *list );

/*************************************************************************/

#endif /* FUNCTIONS_H */

