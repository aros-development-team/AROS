/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef HDTOOLBOX_SUPPORT_H
#define HDTOOLBOX_SUPPORT_H

#include <proto/partition.h>
#include <proto/alib.h>
#include <exec/nodes.h>
#include <exec/lists.h>

#include "partitions.h"

struct Node *getNumNode(struct List *, int);
ULONG getNodeNum(struct Node *);
ULONG countNodes(struct List *, UBYTE);
LONG RequestList(struct List *, ULONG *);
void typestrncpy(STRPTR, STRPTR, ULONG);
void getSizeStr(STRPTR, ULONG);
ULONG sizeStrToUL(STRPTR);
UWORD strcpyESC(STRPTR dst, STRPTR fmt);
#ifdef __AROS__
#ifndef HDTB_HAVE_VARARGPROTOS
LONG GetPartitionAttrsA(struct PartitionHandle *, IPTR, ... );
LONG SetPartitionAttrsA(struct PartitionHandle *, IPTR, ... );
LONG GetPartitionTableAttrsA(struct PartitionHandle *, IPTR, ... );
#endif
#else
UWORD strcpyESC(STRPTR dst, STRPTR fmt, ...);
LONG GetPartitionAttrsA(struct PartitionHandle *, IPTR, ... ) __stackparm;
LONG SetPartitionAttrsA(struct PartitionHandle *, IPTR, ... ) __stackparm;
LONG GetPartitionTableAttrsA(struct PartitionHandle *, IPTR, ... ) __stackparm;
#endif
ULONG getAttrInfo(const struct PartitionAttribute *, ULONG);
UBYTE getBitNum(ULONG);

#endif
