#ifndef READWRITE_H
#define READWRITE_H 1

/*************************************************************************/

#include <exec/types.h>

/*************************************************************************/

/*
** Prototypes
*/

ULONG Device_ReadBlock( UBYTE *buffer, ULONG block, ULONG secsize );

/*************************************************************************/

#endif /* READWRITE_H */

