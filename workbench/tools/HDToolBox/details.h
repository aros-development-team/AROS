#ifndef DETAILS_H
#define DETAILS_H

#include "partitions.h"

void det_Init(struct Window *, struct PartitionNode *);
void det_Ok(struct PartitionNode *);
void det_Cancel(struct PartitionNode *);
void changeType(struct Window *, struct PartitionNode *, UBYTE *, UWORD);
void setDetGadgets(struct Window *, struct PartitionNode *);
void setPartitionTable(struct Window *, struct PartitionNode *, BOOL);
#endif /* DETAILS_H */

