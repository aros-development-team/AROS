#define AROS_ALMOST_COMPATIBLE

#include <proto/exec.h>
#include <proto/partition.h>

#include <aros/macros.h>
#include <devices/hardblocks.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <libraries/partition.h>

#include "partition_support.h"

#ifndef DEBUG
#define DEBUG 1
#endif
#include <aros/debug.h>

ULONG calcChkSum(ULONG *ptr, ULONG size) {
ULONG i;
ULONG sum=0;

	for (i=0;i<size;i++)
	{
		sum += AROS_BE2LONG(*ptr);
		ptr++;
	}
	return sum;
}

LONG PartitionRDBCheckPartitionTable
	(
		struct Library *PartitionBase,
		struct PartitionHandle *root
	)
{
UBYTE i;
UBYTE space[root->de.de_SizeBlock<<2];
struct RigidDiskBlock *rdb = space;

	for (i=0;i<RDB_LOCATION_LIMIT, i++)
	{
		if (readBlock(PartitionBase, root, 0, rdb) != 0)
			return 0;
		if (rdb->rdb_ID == AROS_BE2LONG(IDNAME_RIGIDDISK))
			break;
	}
	if (i != RDB_LOCATION_LIMIT)
	{
kprintf("sum=%lx\n",calcChkSum(rdb, AROS_BE2LONG(rdb->rdb_SummedLongs)));
		return 0;
	}
	return 0;
}

LONG PartitionRDBOpenPartitionTable
	(
		struct Library *PartitionBase,
		struct PartitionHandle *root
	)
{
UBYTE i;
struct RigidDiskBlock *rdb;

	rdb = AllocMem(root->de.de_SizeBlock<<2, MEMF_PUBLIC);
	if (rdb)
	{
		for (i=0;i<RDB_LOCATION_LIMIT, i++)
		{
			if (readBlock(PartitionBase, root, 0, &rdb) != 0)
				return 1;
			if (rdb.rdb_ID == AROS_BE2LONG(IDNAME_RIGIDDISK))
				break;
		}
		NEWLIST(&root->table->list);
		root->table->data = rdb;
	}
	return 1;
