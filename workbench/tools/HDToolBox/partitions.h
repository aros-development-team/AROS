/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PARTITIONS_H
#define PARTITIONS_H

#include <exec/nodes.h>
#include <intuition/intuition.h>

#include "gui.h"
#include "partitiontables.h"

struct HDTBPartition {
	struct ListNode listnode;
	struct HDTBPartition *root;
	struct PartitionHandle *ph;
	struct DriveGeometry dg;
	struct DosEnvec de;
	struct PartitionType type;
	struct PartitionTable *table;
	ULONG flags;
	ULONG pos;
};

#define PNF_ACTIVE        (1<<0)
#define PNF_BOOTABLE      (1<<1)
#define PNF_AUTOMOUNT     (1<<2)

void findPartitions(struct ListNode *, struct HDTBPartition *);
void freePartitionNode(struct HDTBPartition *);
void freePartitionList(struct List *);
BOOL validValue(struct HDTBPartition *, struct HDTBPartition *, ULONG);
struct HDTBPartition *addPartition(struct HDTBPartition *, struct DosEnvec *);

#endif

