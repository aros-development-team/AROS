/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef HDTOOLBOX_SUPPORT_H
#define HDTOOLBOX_SUPPORT_H

#include <proto/partition.h>
#include <exec/nodes.h>
#include <exec/lists.h>

#include "partitions.h"

struct Node *getNumNode(struct List *, int);
ULONG getNodeNum(struct Node *);
ULONG countNodes(struct List *);
LONG RequestList(struct List *, ULONG *);
void typestrncpy(STRPTR, STRPTR, ULONG);
ULONG typestrtol(STRPTR, STRPTR *);
void getSizeStr(STRPTR, ULONG);
LONG GetPartitionAttrsA(struct PartitionHandle *, LONG, ...);
LONG SetPartitionAttrsA(struct PartitionHandle *, LONG, ...);
LONG GetPartitionTableAttrsA(struct PartitionHandle *, LONG, ...);
BOOL existsAttr(ULONG *, ULONG);
#endif
