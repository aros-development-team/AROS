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

#ifdef AROS_ALMOST_COMPATIBLE
#   define NEWLIST(_l)                               \
    do                                               \
    {                                                \
        struct List *l = (struct List *)(_l);        \
                                                     \
        l->lh_TailPred = (struct Node *)l;           \
        l->lh_Tail     = 0;                          \
        l->lh_Head     = (struct Node *)&l->lh_Tail; \
    } while (0)

#   define ADDHEAD(_l,_n)                                 \
    do                                                    \
    {                                                     \
        struct Node *n = (struct Node *)(_n);             \
        struct List *l = (struct List *)(_l);             \
                                                          \
        n->ln_Succ          = l->lh_Head;                 \
	n->ln_Pred          = (struct Node *)&l->lh_Head; \
	l->lh_Head->ln_Pred = n;                          \
	l->lh_Head          = n;                          \
    } while (0)

#   define ADDTAIL(_l,_n)                                     \
    do                                                        \
    {                                                         \
        struct Node *n = (struct Node *)(_n);                 \
        struct List *l = (struct List *)(_l);                 \
                                                              \
        n->ln_Succ              = (struct Node *)&l->lh_Tail; \
	n->ln_Pred              = l->lh_TailPred;             \
	l->lh_TailPred->ln_Succ = n;                          \
	l->lh_TailPred          = n;                          \
    } while (0)

#   define REMOVE(_n)                         \
    do                                        \
    {                                         \
	struct Node *n = (struct Node *)(_n); \
                                              \
        n->ln_Pred->ln_Succ = n->ln_Succ;     \
	n->ln_Succ->ln_Pred = n->ln_Pred;     \
    } while (0)

#   ifdef __GNUC__
#       define GetHead(_l)                                       \
        ({                                                       \
            struct List *l = (struct List *)(_l);                \
                                                                 \
            l->lh_Head->ln_Succ ? l->lh_Head : (struct Node *)0; \
        })

#       define GetTail(_l)                                               \
        ({                                                               \
            struct List *l = (struct List *)(_l);                        \
                                                                         \
            l->lh_TailPred->ln_Pred ? l->lh_TailPred : (struct Node *)0; \
        })

#       define GetSucc(_n)                                       \
        ({                                                       \
	    struct Node *n = (struct Node *)(_n);                \
                                                                 \
            n->ln_Succ->ln_Succ ? n->ln_Succ : (struct Node *)0; \
        })

#       define GetPred(_n)                                       \
        ({                                                       \
	    struct Node *n = (struct Node *)(_n);                \
                                                                 \
            n->ln_Pred->ln_Pred ? n->ln_Pred : (struct Node *)0; \
        })

#    else

#       define GetHead(l)                                                                            \
        (                                                                                            \
            ((struct List *)(l))->lh_Head->ln_Succ ? ((struct List *)l)->lh_Head : (struct Node *)0; \
        )

#       define GetTail(l)                                                                                      \
        (                                                                                                      \
            ((struct List *)(l))->lh_TailPred->ln_Pred ? ((struct List *)(l))->lh_TailPred : (struct Node *)0; \
        )

#       define GetSucc(n)                                                                          \
        (                                                                                          \
            ((struct Node *)n)->ln_Succ->ln_Succ ? ((struct Node *)n)->ln_Succ : (struct Node *)0; \
        }

#       define GetPred(n)                                                                          \
        (                                                                                          \
            ((struct Node *)n)->ln_Pred->ln_Pred ? ((struct Node *)n)->ln_Pred : (struct Node *)0; \
        }
#    endif

#   define ForeachNode(l,n)                        \
    for                                            \
    (                                              \
        n=(void *)(((struct List *)(l))->lh_Head); \
        ((struct Node *)(n))->ln_Succ;             \
	n=(void *)(((struct Node *)(n))->ln_Succ)  \
    )

#   define ForeachNodeSafe(l,n,n2)                  \
    for                                             \
    (                                               \
        n=(void *)(((struct List *)(l))->lh_Head);  \
        (n2=(void *)((struct Node *)(n))->ln_Succ); \
        n=(void *)n2                                \
    )

#   define SetNodeName(node,name)   \
	(((struct Node *)(node))->ln_Name = (char *)(name))
#   define GetNodeName(node)        \
	(((struct Node *)(node))->ln_Name)

#   define ListLength(list,count)      \
    do {		               \
	struct Node * n;	       \
	count = 0;		       \
	ForeachNode (list,n) count ++; \
    } while (0)

#endif


/******************************************************************************
*****  ENDE exec/lists.h
******************************************************************************/

#endif /* EXEC_LISTS_H */
