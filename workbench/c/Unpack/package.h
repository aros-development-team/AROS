#ifndef PKG_PACKAGE_H
#define PKG_PACKAGE_H

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <dos/dos.h>

/****************************************************************************/
/** Prototypes **************************************************************/

/** Low-level functions *****************************************************/

APTR PKG_Open( CONST_STRPTR filename, LONG mode );
void PKG_Close( APTR package );
LONG PKG_Read( APTR package, APTR buffer, LONG length );

/** High-level functions ****************************************************/

LONG /* error */ PKG_ExtractFile( APTR package );
LONG /* error */ PKG_ExtractEverything( APTR package );

#endif /* PKG_PACKAGE_H */
