/*
	$Id$
*/

#ifndef WORKPCP_H
#define WORKPCP_H

#include <exec/nodes.h>
#include <intuition/intuition.h>

#include "workmain.h"

struct PartitionNode {
	struct Node ln;
	struct PCPartitionTable *pt_entry;
};

void findPartitions(struct Window *, struct HDUnitNode *);
struct PartitionNode *getPartition(struct Window *, struct HDUnitNode *, int);
void pcp_Ok(struct List *);
void pcp_Cancel(void);
BOOL changeStartCyl(struct Window *, struct HDUnitNode *, struct PartitionNode *, LONG);
BOOL changeEndCyl(struct Window *, struct HDUnitNode *, struct PartitionNode *, LONG);
BOOL changeTotalCyl(struct Window *, struct HDUnitNode *, struct PartitionNode *, LONG);
BOOL changeType(struct Window *, struct PartitionNode *, LONG);
void deletePartition(struct Window *, struct PartitionNode *);

#endif

