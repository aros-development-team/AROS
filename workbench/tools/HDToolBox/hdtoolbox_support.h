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
ULONG countNodes(struct List *, UBYTE);
LONG RequestList(struct List *, ULONG *);
void typestrncpy(STRPTR, STRPTR, ULONG);
UWORD strcpyESC(STRPTR, STRPTR, ...);
void getSizeStr(STRPTR, ULONG);
ULONG sizeStrToUL(STRPTR);
LONG GetPartitionAttrsA(struct PartitionHandle *, LONG, ...) __stackparm;
LONG SetPartitionAttrsA(struct PartitionHandle *, LONG, ...) __stackparm;
LONG GetPartitionTableAttrsA(struct PartitionHandle *, LONG, ...) __stackparm;
ULONG getAttrInfo(struct PartitionAttribute *, ULONG);
UBYTE getBitNum(ULONG);
#endif
