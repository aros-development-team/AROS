/*
   Copyright © 1997-98, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Demo of new OOP system
   Lang: english
*/
#include "support.h"
#include <string.h>

VOID AddTail(struct List *list, struct Node *node)
{
    assert (node);
    assert (list);

    node->ln_Succ	       = (struct Node *)&list->lh_Tail;
    node->ln_Pred	       = list->lh_TailPred;

    list->lh_TailPred->ln_Succ = node;
    list->lh_TailPred	       = node;

}

struct Node *FindName(struct List *list, STRPTR name)
{
    struct Node * node;

    assert (list);
    assert (name);

    /* Look through the list */
    for (node=GetHead(list); node; node=GetSucc(node))
    {
	/* Only compare the names if this node has one. */
	if(node->ln_Name)
	{
	    /* Check the node. If we found it, stop. */
	    if (!strcmp (node->ln_Name, name))
		break;
	}
    }

    /*
	If we found a node, this will contain the pointer to it. If we
	didn't, this will be NULL (either because the list was
	empty or because we tried all nodes in the list)
    */
    return node;
}

VOID Remove(struct Node *node)
{
    node->ln_Pred->ln_Succ = node->ln_Succ;
    node->ln_Succ->ln_Pred = node->ln_Pred;
    
    return;
}
