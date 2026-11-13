#ifndef EXEC_AVL_H
#define EXEC_AVL_H

/*
   Copyright 2008, The AROS Development Team. All rights reserved.

   Structures for AVL balanced trees.
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#include <aros/asmcall.h>

/* The base node in an AVL tree.  Embed this within your object a-la exec ListNode. */
struct AVLNode {
    struct AVLNode *avl_link[2];
    struct AVLNode *avl_parent;
    LONG avl_balance;
};

/* The key type, it's content is only intepreted by the key comparison function */
typedef void *AVLKey;

/* Compare two nodes */
typedef AROS_UFP2(LONG, (*AVLNODECOMP),
	  AROS_UFPA(const struct AVLNode *, avlnode1,  A0),
	  AROS_UFPA(const struct AVLNode *, avlnode2,  A1));

/* Compare a node to a key */
typedef AROS_UFP2(LONG, (*AVLKEYCOMP),
	  AROS_UFPA(const struct AVLNode *, avlnode,  A0),
	  AROS_UFPA(AVLKey,		    avlkey,   A1));

#endif /* EXEC_AVL_H */ 
