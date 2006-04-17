//
// pmdlist.h
//
// PopupMenu Library - Linked Lists
//
// Copyright (C)2000 Henrik Isaksson <henrik@boing.nu>
// All Rights Reserved.
//


#ifndef PM_DLIST_H
#define PM_DLIST_H

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif

struct PM_GenericListNode {
	struct MinNode	n;
	UWORD		Length;
};

typedef struct MinList PMDList;
typedef APTR PMNode;
typedef struct PM_GenericListNode PMGLN;

#define PM_NextNode(x)	(((struct MinNode *)x)->mln_Succ)

PMDList *PM_InitList(void);		// Create a new list header. *
void PM_FreeList(PMDList *list);	// Free a list. *
PMDList *PM_CopyList(PMDList *list);	// Copy a list. *

void PM_AddToList(PMDList *l, PMNode *A);	// Add A to l. *
void PM_Unlink(PMDList *l, PMNode *A);		// Remove A from l. *
void PM_FreeNode(PMNode *A);			// Free a node. *
PMNode *PM_CopyNode(PMNode *A);			// Copy a node. *

#endif
