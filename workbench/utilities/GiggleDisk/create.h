#ifndef CREATE_H
#define CREATE_H 1

/*************************************************************************/

#include <exec/types.h>

#include "header.h"

/*************************************************************************/

/*
** Prototypes
*/

ULONG Create_MountFile( STRPTR name, struct PartitionEntry *pei );
ULONG Create_DosDriver( STRPTR dir, struct PartitionEntry *pei );

/*************************************************************************/

#endif /* CREATE_H */

