/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PARTITIONTABLES_H
#define PARTITIONTABLES_H

#include <exec/nodes.h>
#include <dos/filehandler.h>
#include <intuition/intuition.h>
#include <libraries/partition.h>

struct PartitionNode;

struct HDUnitNode {
	struct Node ln;
	ULONG unit;
	struct PartitionHandle *root;
};

struct PartitionTableNode {
	struct Node ln;
	struct List pl;
	struct PartitionHandle *ph;
	ULONG flags;
	ULONG *tattrlist;
	ULONG *pattrlist;
	struct DosEnvec de;
	ULONG reserved;
	ULONG maxpartitions;
	ULONG type;
};

#define PNF_TABLE_CHANGED (1<<0) /* partition table has been changed */

struct PartitionTableNode *findPTPH(struct PartitionHandle *);
void addPartitionTable(struct PartitionNode *);
void findPartitionTables(struct Window *, char *, int);
struct PartitionTableNode *getPartitionTable(struct Window *, int);
BOOL reallyExit(struct List *);
void freePartitionTable(struct PartitionTableNode *);
void freePartitionTableList(struct List *);
void freeHDList(struct List *);
void saveChanges(struct Window *, struct PartitionTableNode *);

#endif

