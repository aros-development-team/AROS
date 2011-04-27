/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PARTITIONTABLES_H
#define PARTITIONTABLES_H

#include <exec/nodes.h>
#include <dos/filehandler.h>
#include <intuition/intuition.h>
#include <libraries/partition.h>

struct HDTBPartition;

struct PartitionTable
{
    const struct PartitionAttribute *tattrlist; /* supported partition table attributes */
    const struct PartitionAttribute *pattrlist; /* supported partition attributes */
    ULONG reserved;
    ULONG max_partitions;
    ULONG type;
};

BOOL findPartitionTable(struct HDTBPartition *);
void freePartitionTable(struct HDTBPartition *);
BOOL makePartitionTable(struct HDTBPartition *, ULONG);

void mountPartitions(struct List *);

#endif

