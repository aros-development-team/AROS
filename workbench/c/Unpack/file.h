#ifndef PKG_FILE_H
#define PKG_FILE_H

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <dos/bptr.h>

/****************************************************************************/
/*** Prototypes *************************************************************/

LONG FILE_Read( BPTR file, APTR buffer, LONG length );
LONG FILE_Write( BPTR file, CONST_APTR buffer, LONG length );
LONG FILE_Position( BPTR file );

#endif /* PKG_FILE_H */

