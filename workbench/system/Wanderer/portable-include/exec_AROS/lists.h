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

#endif /* EXEC_LISTS_AROS_H */
