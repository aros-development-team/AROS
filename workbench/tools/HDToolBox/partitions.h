/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PARTITIONS_H
#define PARTITIONS_H

#include <exec/nodes.h>
#include <intuition/intuition.h>

#include "partitiontables.h"

struct PartitionNode {
	struct Node ln;
	struct PartitionTableNode *root;
	struct PartitionHandle *ph;
	ULONG flags;
	struct DosEnvec de;
	ULONG pos;
	ULONG type;
};

#define PNF_CHANGED    (1<<0)
#define PNF_ADDED      (1<<1)
#define PNF_NEW_TABLE  (1<<2)
#define PNF_DEL_TABLE  (1<<3)
#define PNF_TABLE      (1<<4)

void par_Init(struct Window *, struct PartitionTableNode *);
void viewPartitionData(struct Window *, struct PartitionTableNode *, struct PartitionNode *);
void viewDosEnvecData(struct Window *, struct PartitionTableNode *, struct DosEnvec *);
BOOL pcp_Ok(struct PartitionTableNode *);
void pcp_Cancel(struct PartitionTableNode *);
void changeStartCyl(struct Window *, struct PartitionTableNode *, struct PartitionNode *, ULONG);
void changeEndCyl(struct Window *, struct PartitionTableNode *, struct PartitionNode *, ULONG);
void changeTotalCyl(struct Window *, struct PartitionTableNode *, struct PartitionNode *, ULONG);
struct PartitionNode *addPartition(struct Window *, struct PartitionTableNode *, struct DosEnvec *);
void deletePartition(struct Window *, struct PartitionNode *);

#endif

