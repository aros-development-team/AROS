#ifndef PARTITIONENTRY_H
#define PARTITIONENTRY_H 1

/*************************************************************************/

#include <exec/types.h>

#include "header.h"

/*************************************************************************/

/*
** Prototypes
*/

STRPTR PE_FindPartitionName( ULONG type );
void PE_ShrinkCycles( struct PartitionEntry *pe );
void PE_SetFileSystem( struct PartitionEntry *pe );
void PE_CalculateSize( struct PartitionEntry *pe );
ULONG PE_CheckSuperBlock( struct PartitionEntry *pe );
ULONG PE_IdentifySuperblock( BYTE *data, ULONG id );
ULONG PE_GetGeometry( struct PartitionEntry *pe );


/*************************************************************************/

#endif /* PARTITIONENTRY_H */

