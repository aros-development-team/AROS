/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef EXEC_LISTS_H
#define EXEC_LISTS_H

/******************************************************************************

    FILE
	$Id$

    DESCRIPTION
	Prototypes and macros for exec-lists.

******************************************************************************/

/**************************************
		Includes
**************************************/
#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif


/**************************************
	       Structures
**************************************/
/* Normal list */
struct List
{
    struct Node * lh_Head,
		* lh_Tail,
		* lh_TailPred;
    UBYTE	  lh_Type;
    UBYTE	  l_pad;
};

/* Minimal list */
struct MinList
{
    struct MinNode * mlh_Head,
		   * mlh_Tail,
		   * mlh_TailPred;
};


/**************************************
	       Makros
**************************************/
#define IsListEmpty(l) \
	( (((struct List *)l)->lh_TailPred) == (struct Node *)(l) )

#define IsMsgPortEmpty(mp) \
      ( (((struct MsgPort *)(mp))->mp_MsgList.lh_TailPred) \
	    == (struct Node *)(&(((struct MsgPort *)(mp))->mp_MsgList)) )

#define NEWLIST(_l)                              \
do                                               \
{                                                \
    struct List *__l = (struct List *)(_l);        \
                                                 \
    __l->lh_TailPred = (struct Node *)__l;           \
    __l->lh_Tail     = 0;                          \
    __l->lh_Head     = (struct Node *)&__l->lh_Tail; \
} while (0)

#define ADDHEAD(_l,_n)                                \
do                                                    \
{                                                     \
    struct Node *__n = (struct Node *)(_n);             \
    struct List *__l = (struct List *)(_l);             \
                                                      \
    __n->ln_Succ          = __l->lh_Head;                 \
    __n->ln_Pred          = (struct Node *)&__l->lh_Head; \
    __l->lh_Head->ln_Pred = __n;                          \
    __l->lh_Head          = __n;                          \
} while (0)

#define ADDTAIL(_l,_n)                                    \
do                                                        \
{                                                         \
    struct Node *__n = (struct Node *)(_n);                 \
    struct List *__l = (struct List *)(_l);                 \
                                                          \
    __n->ln_Succ              = (struct Node *)&__l->lh_Tail; \
    __n->ln_Pred              = __l->lh_TailPred;             \
    __l->lh_TailPred->ln_Succ = __n;                          \
    __l->lh_TailPred          = __n;                          \
} while (0)

#define REMOVE(_n)                            \
({                                            \
    struct Node *__n = (struct Node *)(_n);   \
                                              \
    __n->ln_Pred->ln_Succ = __n->ln_Succ;     \
    __n->ln_Succ->ln_Pred = __n->ln_Pred;     \
    __n;                                      \
})

#define GetHead(_l)                                         \
({                                                          \
   struct List *__l = (struct List *)(_l);                  \
                                                            \
   __l->lh_Head->ln_Succ ? __l->lh_Head : (struct Node *)0; \
})

#define GetTail(_l)                                                  \
({                                                                   \
    struct List *__l = (struct List *)(_l);                          \
                                                                     \
    __l->lh_TailPred->ln_Pred ? __l->lh_TailPred : (struct Node *)0; \
})

#define GetSucc(_n)                                          \
({                                                           \
    struct Node *__n = (struct Node *)(_n);                  \
                                                             \
    __n->ln_Succ->ln_Succ ? __n->ln_Succ : (struct Node *)0; \
})

#define GetPred(_n)                                          \
({                                                           \
    struct Node *__n = (struct Node *)(_n);                  \
                                                             \
    __n->ln_Pred->ln_Pred ? __n->ln_Pred : (struct Node *)0; \
})

#define REMHEAD(_l)                         \
({                                          \
    struct List *__l = (struct List *)(_l); \
                                            \
     __l->lh_Head->ln_Succ    ?             \
        REMOVE(__l->lh_Head) :              \
        (struct Node *)0                    \
    ;                                       \
})

#define REMTAIL(_l)                         \
({                                          \
    struct List *__l = (struct List *)(_l); \
                                            \
    __l->lh_TailPred->ln_Pred    ?          \
        REMOVE(__l->lh_TailPred) :          \
        (struct Node *)0                    \
    ;                                       \
})

#define ForeachNode(l,n)                       \
for                                            \
(                                              \
    n=(void *)(((struct List *)(l))->lh_Head); \
    ((struct Node *)(n))->ln_Succ;             \
    n=(void *)(((struct Node *)(n))->ln_Succ)  \
)

#define ForeachNodeSafe(l,n,n2)                 \
for                                             \
(                                               \
    n=(void *)(((struct List *)(l))->lh_Head);  \
    (n2=(void *)((struct Node *)(n))->ln_Succ); \
    n=(void *)n2                                \
)

#define SetNodeName(node,name)   \
    (((struct Node *)(node))->ln_Name = (char *)(name))
#define GetNodeName(node)        \
    (((struct Node *)(node))->ln_Name)

#define ListLength(list,count)     \
do {		                   \
    struct Node * __n;	           \
    count = 0;		           \
    ForeachNode (list,__n) count ++; \
} while (0)

/******************************************************************************
*****  ENDE exec/lists.h
******************************************************************************/

#endif /* EXEC_LISTS_H */
