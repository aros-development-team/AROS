#ifndef PREFS_H
#define PREFS_H

#include <exec/nodes.h>
#include <libraries/partition.h>

struct TableTypeNode {
	struct Node ln;
	struct List *typelist;
	struct PartitionTableInfo *pti;
	struct PartitionType defaulttype;
};

struct TypeNode {
	struct Node ln;
	struct PartitionType type;
};

struct TableTypeNode *findTableTypeNodeName(STRPTR);
struct TableTypeNode *findTableTypeNode(ULONG);
struct TypeNode *findPartitionType(struct PartitionType *, ULONG);
void LoadPrefs(STRPTR);

#endif /* PREFS_H */

