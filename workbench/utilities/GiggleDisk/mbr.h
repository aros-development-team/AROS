#ifndef MBR_H
#define MBR_H 1

/*************************************************************************/

#include <exec/types.h>

#include "header.h"

/*************************************************************************/

/*
** Prototypes
*/

ULONG CreatePartition_MBR( ULONG block );
BOOL MBR_Check( struct MBRPartitionBlock *mbr );


/*************************************************************************/

#endif /* MBR_H */
