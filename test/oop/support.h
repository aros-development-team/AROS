#ifndef SUPPORT_H
#define SUPPORT_H

/*
   Copyright © 1997-98, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Demo of new OOP system - support defs.
   Lang: english
*/

#ifndef TYPES_H
#  include "types.h"
#endif

#include <assert.h>


struct Node
{
    struct Node * ln_Succ,
		* ln_Pred;
    /* AROS: pointer should be 32bit aligned */
    char	* ln_Name;
    UBYTE	  ln_Type;
    BYTE	  ln_Pri;
};

struct MinNode
{
    struct MinNode * mln_Succ,
		   * mln_Pred;
};

struct List
{
    struct Node * lh_Head,
		* lh_Tail,
		* lh_TailPred;
    UBYTE	  lh_Type;
    UBYTE	  l_pad;
};

struct MinList
{
    struct MinNode * mlh_Head,
		   * mlh_Tail,
		   * mlh_TailPred;
};

#   define NEWLIST(l)       (((struct List *)l)->lh_TailPred \
				= (struct Node *)(l), \
			    ((struct List *)l)->lh_Tail = 0, \
			    ((struct List *)l)->lh_Head \
				= (struct Node *)\
				    &(((struct List *)l)->lh_Tail))

#   define ADDHEAD(l,n)     ((void)(\
	((struct Node *)n)->ln_Succ          = ((struct List *)l)->lh_Head, \
	((struct Node *)n)->ln_Pred          = (struct Node *)&((struct List *)l)->lh_Head, \
	((struct List *)l)->lh_Head->ln_Pred = ((struct Node *)n), \
	((struct List *)l)->lh_Head          = ((struct Node *)n)))

#   define ADDTAIL(l,n)     ((void)(\
	((struct Node *)n)->ln_Succ              = (struct Node *)&((struct List *)l)->lh_Tail, \
	((struct Node *)n)->ln_Pred              = ((struct List *)l)->lh_TailPred, \
	((struct List *)l)->lh_TailPred->ln_Succ = ((struct Node *)n), \
	((struct List *)l)->lh_TailPred          = ((struct Node *)n) ))

#   define REMOVE(n)        ((void)(\
	((struct Node *)n)->ln_Pred->ln_Succ = ((struct Node *)n)->ln_Succ,\
	((struct Node *)n)->ln_Succ->ln_Pred = ((struct Node *)n)->ln_Pred ))

#   define GetHead(l)       (void *)(((struct List *)l)->lh_Head->ln_Succ \
				? ((struct List *)l)->lh_Head \
				: (struct Node *)0)
#   define GetTail(l)       (void *)(((struct List *)l)->lh_TailPred->ln_Pred \
				? ((struct List *)l)->lh_TailPred \
				: (struct Node *)0)
#   define GetSucc(n)       (void *)(((struct Node *)n)->ln_Succ->ln_Succ \
				? ((struct Node *)n)->ln_Succ \
				: (struct Node *)0)
#   define GetPred(n)       (void *)(((struct Node *)n)->ln_Pred->ln_Pred \
				? ((struct Node *)n)->ln_Pred \
				: (struct Node *)0)
#   define ForeachNode(l,n) \
	for (n=(void *)(((struct List *)(l))->lh_Head); \
	    ((struct Node *)(n))->ln_Succ; \
	    n=(void *)(((struct Node *)(n))->ln_Succ))

#   define ForeachNodeSafe(l,n,n2) \
	for (n=(void *)(((struct List *)(l))->lh_Head); \
	    (n2=(void *)((struct Node *)(n))->ln_Succ); \
	    n=(void *)n2)

#   define SetNodeName(node,name)   \
	(((struct Node *)(node))->ln_Name = (char *)(name))
#   define GetNodeName(node)        \
	(((struct Node *)(node))->ln_Name

#   define ListLength(list,count)   \
	do {				    \
	    struct Node * n;		    \
	    count = 0;			    \
	    ForeachNode (list,n) count ++;  \
	} while (0)

#define IsListEmpty(l) \
	( (((struct List *)l)->lh_TailPred) == (struct Node *)(l) )


/* Prototypes */
VOID AddTail(struct List *, struct Node *);
struct Node *FindName(struct List *, STRPTR);
VOID Remove(struct Node *node);
#endif /* SUPPORT_H */
