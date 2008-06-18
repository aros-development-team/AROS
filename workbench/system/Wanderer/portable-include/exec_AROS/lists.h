#ifndef EXEC_LISTS_AROS_H
#define EXEC_LISTS_AROS_H

/*
    Copyright � 1995-2005, The AROS Development Team. All rights reserved.
        
    Structures and macros for exec lists.
*/
#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif


/**************************************
         Makros
**************************************/


#define NEW_LIST(_l)                                     \
do                                                      \
{                                                       \
    struct List *__aros_list_tmp = (struct List *)(_l), \
                *l = __aros_list_tmp;                   \
                                                        \
    l->lh_TailPred = (struct Node *)l;                  \
    l->lh_Tail     = 0;                                 \
    l->lh_Head     = (struct Node *)&l->lh_Tail;        \
} while (0)



#define Foreach_Node(list, node)                        \
for                                                    \
(                                                      \
    node = (void *)(((struct List *)(list))->lh_Head); \
    ((struct Node *)(node))->ln_Succ;                  \
    node = (void *)(((struct Node *)(node))->ln_Succ)  \
)

#define Foreach_NodeSafe(l,n,n2) \
  for (n=(void *)(((struct List *)(l))->lh_Head); \
      (n2=(void *)((struct Node *)(n))->ln_Succ); \
      n=(void *)n2)

/*
#ifdef AROS_ALMOST_COMPATIBLE

#define GetHead(_l)  \
({ struct List *l = (struct List *)(_l);  \
	l->lh_Head->ln_Succ ? l->lh_Head : (struct Node *)0;  \
})

#define GetTail(_l)  \
({ struct List *l = (struct List *)(_l);  \
	l->lh_TailPred->ln_Pred ? l->lh_TailPred : (struct Node *)0;  \
})

#define GetSucc(_n)  \
({ struct Node *n = (struct Node *)(_n);  \
	n->ln_Succ->ln_Succ ? n->ln_Succ : (struct Node *)0;  \
})

#define GetPred(_n)  \
({ struct Node *n = (struct Node *)(_n);  \
	n->ln_Pred->ln_Pred ? n->ln_Pred : (struct Node *)0;  \
})

#define ForeachNode(l,n)  \
for (  \
	n = (void *)(((struct List *)(l))->lh_Head);  \
	((struct Node *)(n))->ln_Succ;  \
	n = (void *)(((struct Node *)(n))->ln_Succ)  \
)

#define ForeachNodeSafe(l,n,n2)  \
for (  \
	n = (void *)(((struct List *)(l))->lh_Head);  \
	(n2 = (void *)((struct Node *)(n))->ln_Succ);  \
	n = (void *)n2  \
)

#define SetNodeName(node,name)  (((struct Node *)(node))->ln_Name = (char *)(name))
#define GetNodeName(node)       (((struct Node *)(node))->ln_Name)

#define ListLength(list,count)  \
do {  \
	struct Node *n;  \
	count = 0;  \
	ForeachNode(list,n) count++;  \
} while (0)

#endif /* AROS_ALMOST_COMPATIBLE */


#endif /* EXEC_LISTS_AROS_H */
