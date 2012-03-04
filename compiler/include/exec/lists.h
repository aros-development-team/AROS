#ifndef EXEC_LISTS_H
#define EXEC_LISTS_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
        
    Structures and macros for exec lists.
*/


/**************************************
		Includes
**************************************/
#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif


/**************************************
	       Structures
**************************************/
#ifndef __GNUC__
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
#else
/* Normal list */
struct List __mayalias;
struct List
{
    struct Node * lh_Head,
                * lh_Tail;
    union
    {
        struct Node * lh_TailPred;
        struct List * lh_TailPred_;
    };
    UBYTE	  lh_Type;
    UBYTE	  l_pad;
};

/* Minimal list */
struct MinList __mayalias;
struct MinList
{
    struct MinNode * mlh_Head,
                   * mlh_Tail;
    union
    {
        struct MinNode * mlh_TailPred;
        struct MinList * mlh_TailPred_;
    };
};
#endif /* __GNUC__ */

/**************************************
	       Macros
**************************************/
#define IsListEmpty(l) \
	( (((struct List *)l)->lh_TailPred) == (struct Node *)(l) )

#define IsMinListEmpty(l) \
	( (((struct MinList *)l)->mlh_TailPred) == (struct MinList *)(l) )

#define IsMsgPortEmpty(mp) \
      ( (((struct MsgPort *)(mp))->mp_MsgList.lh_TailPred) \
	    == (struct Node *)(&(((struct MsgPort *)(mp))->mp_MsgList)) )

#ifndef __GNUC__
#define NEWLIST(_l)                                     \
do                                                      \
{                                                       \
    struct List *__aros_list_tmp = (struct List *)(_l), \
                *l = __aros_list_tmp;                   \
                                                        \
    l->lh_TailPred = (struct Node *)l;                  \
    l->lh_Tail     = 0;                                 \
    l->lh_Head     = (struct Node *)&l->lh_Tail;        \
} while (0)
#else /* __GNUC__ */
#define NEWLIST(_l)                                     \
do                                                      \
{                                                       \
    struct List *__aros_list_tmp = (struct List *)(_l), \
                *l = __aros_list_tmp;                   \
                                                        \
    l->lh_TailPred_= l;                                 \
    l->lh_Tail     = 0;                                 \
    l->lh_Head     = (struct Node *)&l->lh_Tail;        \
} while (0)
#endif /* __GNUC__ */

#define ADDHEAD(_l,_n)                                  \
do                                                      \
{                                                       \
    struct Node *__aros_node_tmp = (struct Node *)(_n), \
                *n = __aros_node_tmp;                   \
    struct List *__aros_list_tmp = (struct List *)(_l), \
                *l = __aros_list_tmp;                   \
                                                        \
    n->ln_Succ          = l->lh_Head;                   \
    n->ln_Pred          = (struct Node *)&l->lh_Head;   \
    l->lh_Head->ln_Pred = n;                            \
    l->lh_Head          = n;                            \
} while (0)

#define ADDTAIL(_l,_n)                                    \
do                                                        \
{                                                         \
    struct Node *__aros_node_tmp = (struct Node *)(_n),   \
                *n = __aros_node_tmp;                     \
    struct List *__aros_list_tmp = (struct List *)(_l),   \
                *l = __aros_list_tmp;                     \
                                                          \
    n->ln_Succ              = (struct Node *)&l->lh_Tail; \
    n->ln_Pred              = l->lh_TailPred;             \
    l->lh_TailPred->ln_Succ = n;                          \
    l->lh_TailPred          = n;                          \
} while (0)

#define REMOVE(_n)                                      \
({                                                      \
    struct Node *__aros_node_tmp = (struct Node *)(_n), \
                *n = __aros_node_tmp;                   \
                                                        \
    n->ln_Pred->ln_Succ = n->ln_Succ;                   \
    n->ln_Succ->ln_Pred = n->ln_Pred;                   \
                                                        \
    n;                                                  \
})

#define GetHead(_l)                                     \
({                                                      \
    struct List *__aros_list_tmp = (struct List *)(_l), \
                *l = __aros_list_tmp;                   \
                                                        \
   l->lh_Head->ln_Succ ? l->lh_Head : (struct Node *)0; \
})

#define GetTail(_l)                                              \
({                                                               \
    struct List *__aros_list_tmp = (struct List *)(_l),          \
                *l = __aros_list_tmp;                            \
                                                                 \
    l->lh_TailPred->ln_Pred ? l->lh_TailPred : (struct Node *)0; \
})

#define GetSucc(_n)                                      \
({                                                       \
    struct Node *__aros_node_tmp = (struct Node *)(_n),  \
                *n = __aros_node_tmp;                    \
                                                         \
    (n && n->ln_Succ && n->ln_Succ->ln_Succ) ? n->ln_Succ : (struct Node *)0; \
})

#define GetPred(_n)                                      \
({                                                       \
    struct Node *__aros_node_tmp = (struct Node *)(_n),  \
                *n = __aros_node_tmp;                    \
                                                         \
    (n && n->ln_Pred && n->ln_Pred->ln_Pred) ? n->ln_Pred : (struct Node *)0; \
})

#define REMHEAD(_l)                                               \
({                                                                \
    struct List *__aros_list_tmp = (struct List *)(_l),           \
                *l = __aros_list_tmp;                             \
                                                                  \
     l->lh_Head->ln_Succ ? REMOVE(l->lh_Head) : (struct Node *)0; \
})

#define REMTAIL(_l)                                                      \
({                                                                       \
    struct List *__aros_list_tmp = (struct List *)(_l),                  \
                *l = __aros_list_tmp;                                    \
                                                                         \
    l->lh_TailPred->ln_Pred ? REMOVE(l->lh_TailPred) : (struct Node *)0; \
})

#define ForeachNode(list, node)                        \
for                                                    \
(                                                      \
    node = (void *)(((struct List *)(list))->lh_Head); \
    ((struct Node *)(node))->ln_Succ;                  \
    node = (void *)(((struct Node *)(node))->ln_Succ)  \
)

#define ForeachNodeSafe(list, current, next)              \
for                                                       \
(                                                         \
    current = (void *)(((struct List *)(list))->lh_Head); \
    (next = (void *)((struct Node *)(current))->ln_Succ); \
    current = (void *)next                                \
)

#define ListLength(list,count)     \
do {		                   \
    struct Node * __n;	           \
    count = 0;		           \
    ForeachNode (list,__n) count ++; \
} while (0)

#endif /* EXEC_LISTS_H */
