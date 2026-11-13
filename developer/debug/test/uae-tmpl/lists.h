#ifndef EXEC_LISTS_H
#define EXEC_LISTS_H
/* Copyright © 1995, The AROS Development Team. All rights reserved. */

#ifndef EXEC_NODES_H
#   include "nodes.h"
#endif

typedef be_ptr<struct List> ListPtr;
typedef be_ptr<struct MinList> MinListPtr;

/* Normal list */
struct List
{
    NodePtr lh_Head,
	    lh_Tail,
	    lh_TailPred;
    UBYTE   lh_Type;
    UBYTE   l_pad;
};

/* Minimal list */
struct MinList
{
    MinNodePtr mlh_Head,
	       mlh_Tail,
	       mlh_TailPred;
};

#define IsListEmpty(l) \
	( (((struct List *)l)->lh_TailPred) == (struct Node *)(l) )
#define IsMsgPortEmpty(mp) \
      ( (((struct MsgPort *)mp)->mp_MsgList.lh_TailPred) \
	    == (struct Node *)(&(((struct MsgPort *)mp)->mp_MsgList)) )

#   define NEWLIST(l)       (((struct List *)l)->lh_TailPred \
				= (NodePtr)(l), \
			    ((struct List *)l)->lh_Tail = 0, \
			    ((struct List *)l)->lh_Head \
				= (NodePtr)\
				    &(((struct List *)l)->lh_Tail))

#   define ADDHEAD(l,n)     ((void)(\
	((struct Node *)n)->ln_Succ          = ((struct List *)l)->lh_Head, \
	((struct Node *)n)->ln_Pred          = (struct Node *)&((struct List *)l)->lh_Head, \
	((struct Node *)((struct List *)l)->lh_Head)->ln_Pred = ((struct Node *)n), \
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


#endif /* EXEC_LISTS_H */
