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
	struct PartitionHandle *ph;
	struct HDUnitNode *hd;
	struct List pl;             /* partition list */
	struct List *typelist;      /* list of partition types */
	ULONG flags;
	ULONG *tattrlist;
	ULONG *pattrlist;
	struct DosEnvec de;
	ULONG reserved;
	ULONG maxpartitions;
	ULONG type;
};

#define PNF_TABLE_CHANGED (1<<0) /* partition table has been changed */

void findHDs(char *, ULONG);
struct PartitionTableNode *findPTPH(struct PartitionHandle *);
void addPartitionTable(struct PartitionNode *);
void findPartitionTables(struct Window *, struct List *);
struct PartitionTableNode *getPartitionTable(struct Window *, int);
BOOL reallyExit(struct List *);
void freePartitionTable(struct PartitionTableNode *);
void freePartitionTableList(struct List *);
void freeHDList(struct List *);
void saveChanges(struct Window *, struct PartitionTableNode *);

#endif

