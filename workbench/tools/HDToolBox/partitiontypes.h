#ifndef PARTITIONTYPES_H
#define PARTITIONTYPES_H

/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/nodes.h>
#include "partitiontables.h"

struct PartitionTypeNode {
	struct Node ln;
	UBYTE *id;      /* magic value / id     */
	UWORD id_len;   /* length of identifier */
};

struct PartitionTypeNode *getPartitionTypeNode(struct PartitionTableNode *, struct PartitionType *);
void setPartitionType(struct PartitionTableNode *);

#endif /* PARTITIONTYPES_H */
