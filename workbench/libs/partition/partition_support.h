#ifndef PARTITION_SUPPORT_H
#define PARTITION_SUPPORT_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

*/

#include <exec/types.h>
#include <libraries/partition.h>
#include <utility/tagitem.h>

#include "partition_intern.h"

struct PTFunctionTable {
	ULONG		type; /* Partition Table Type */
	STRPTR	name;
	LONG		(*CheckPartitionTable)	(struct Library *, struct PartitionHandle *);
	LONG		(*OpenPartitionTable)	(struct Library *, struct PartitionHandle *);
	void		(*ClosePartitionTable)	(struct Library *, struct PartitionHandle *);
	LONG		(*WritePartitionTable)	(struct Library *, struct PartitionHandle *);
	LONG		(*CreatePartitionTable)	(struct Library *, struct PartitionHandle *);
	struct PartitionHandle *(*AddPartition)(struct Library *, struct PartitionHandle *, struct TagItem *);
	void 		(*DeletePartition)		(struct Library *, struct PartitionHandle *);
	LONG 		(*GetPartitionTableAttrs)(struct Library *, struct PartitionHandle *, struct TagItem *);
	LONG 		(*SetPartitionTableAttrs)(struct Library *, struct PartitionHandle *, struct TagItem *);
	LONG 		(*GetPartitionAttrs)		(struct Library *, struct PartitionHandle *, struct TagItem *);
	LONG 		(*SetPartitionAttrs)		(struct Library *, struct PartitionHandle *, struct TagItem *);
	struct PartitionAttribute *	(*QueryPartitionTableAttrs)(struct Library *);
	struct PartitionAttribute *	(*QueryPartitionAttrs)	(struct Library *);
	ULONG    (*DestroyPartitionTable) (struct Library *, struct PartitionHandle *);
};

extern struct PTFunctionTable *PartitionSupport[];

LONG PartitionGetGeometry(struct Library *, struct IOExtTD *, struct DriveGeometry *);
void PartitionNsdCheck(struct Library *, struct PartitionHandle *);
ULONG getStartBlock(struct PartitionHandle *);
LONG readBlock(struct Library *, struct PartitionHandle *, ULONG, void *);
LONG PartitionWriteBlock(struct Library *, struct PartitionHandle *, ULONG, void *);
struct TagItem *findTagItem(ULONG tag, struct TagItem *);
void fillMem(BYTE *, LONG, BYTE);

#define getGeometry PartitionGetGeometry
#define writeBlock  PartitionWriteBlock

#endif /* PARTITION_SUPPORT_H */
