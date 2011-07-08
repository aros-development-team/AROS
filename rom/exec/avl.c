/* indent -blC -bli0 -npcs -ncs -i4 -bad -bap avl.c */
/*
    Copyright (c) 2008, The AROS Development Team. All rights reserved.

#Description: @AROS.Exec.AVL@

<chapter title="Exec AVL Trees">

<p>FIXME explain it</p>

</chapter>

*/

#include <proto/exec.h>
#include <exec/avl.h>

/* don't think this works inside exec.library */
#define D(x)

#define LEFT (0)
#define RIGHT (1)

/* internal functions */
/* Re-Links the parent of subroot, properly handling the root-node.
   The old parent is supplied in parent */
static inline int
link_parent(struct AVLNode **root, const struct AVLNode *scan,
	    struct AVLNode *subroot, struct AVLNode *parent)
{
    int dir;

    if (parent == NULL)
    {
	*root = subroot;
	subroot->avl_parent = NULL;
	dir = -1;
    }
    else
    {
	dir = parent->avl_link[1] == scan;
	parent->avl_link[dir] = subroot;
	subroot->avl_parent = parent;
    }

    return dir;
}

/* Perform a single rotation about next in the given direction.
   Note that this will be inlined with hard-coded dirs */
static inline struct AVLNode *
rotate_single(struct AVLNode *scan, struct AVLNode *next, const int dir)
{
    struct AVLNode *subroot = next;
    const int other = dir == 0;

    D(printf("single %s about %p\n", dir == 0 ? "left" : "right", subroot));

    scan->avl_link[other] = next->avl_link[dir];
    if (next->avl_link[dir] != NULL)
	next->avl_link[dir]->avl_parent = scan;
    next->avl_link[dir] = scan;
    scan->avl_parent = next;

    return subroot;
}

/* Peform a double rotation about scan and then next in a given direction
   Note that this will be in-lined with hard-coded dirs */
static inline struct AVLNode *
rotate_double(struct AVLNode *scan, struct AVLNode *next, const int dir)
{
    struct AVLNode *subroot = next->avl_link[dir];
    const int other = dir == 0;

    D(printf("double %s about %p\n", dir == 0 ? "left" : "right", subroot));

    next->avl_link[dir] = subroot->avl_link[other];
    if (subroot->avl_link[other] != NULL)
	subroot->avl_link[other]->avl_parent = next;
    subroot->avl_link[other] = next;
    scan->avl_link[other] = subroot->avl_link[dir];
    if (subroot->avl_link[dir] != NULL)
	subroot->avl_link[dir]->avl_parent = scan;
    subroot->avl_link[dir] = scan;

    next->avl_parent = subroot;
    scan->avl_parent = subroot;

    return subroot;
}

/*****************************************************************************

    NAME */

    AROS_LH3I(struct AVLNode *, AVL_AddNode,

/*  SYNOPSIS */
	  AROS_LHA(struct AVLNode **, root, A0),
	  AROS_LHA(struct AVLNode *, node, A1),
	  AROS_LHA(AVLNODECOMP, func, A2),
/*  LOCATION */
	  struct ExecBase *, SysBase, 139, Exec)
/*  FUNCTION
	Add a new node to an AVL tree.

    INPUTS
	Root  -	The root node of the tree to which the new node will be added.
		Initially and if the tree is empty, this will be NULL.

	Node  - The new node to add.  Any key information must alread be
		initialised.

	Func  - A key comparison function.  It will be passed 2 nodes,
		node1 and node2 to compare and must return <0, 0 or >0 to
		reflect node1 < node2, node1 == node, or node1 > node2
		respectively.

    RESULT
	NULL if the node was added to the tree, or a pointer to a node with the
	same key which already exists in the tree.

    NOTES
	AVL trees are balanced binary search trees.  This implementation is
	based on embedding the struct AVLNode within your own data object
	and providing custom comparison functions wherever they are needed.

	Two comparison functions are needed for different cases.  It is entirely
	up to the application as to how it interprets the nodes and compares
	the keys.

	AVLNODECOMP is used to compare the keys of two nodes.

	typedef LONG *AVLNODECOMP(const struct AVLNode *avlnode1,
				  const struct AVLNode *avlnode2);
                D0                A0, A1

	AVLKEYCOMP is used to compare a key to a node.

	typedef LONG *AVLKEYCOMP(const struct AVLNode *avlnode,
				 AVLKey key);
	        D0               A0, A1

	These functions must return the same sense for the same key values.
	That is,
	    <0  if key of avlnode1 <  key of avlnode2
                if key of avlnode  <  key
             0  if key of avlnode1 == key of avlnode2
                if key of avlnode  == key
	    >0  if key of avlnode1 >  key of avlnode2
                if key of avlnode  <  key

	Since this function returns the existing node if the keys match,
	this function can be used to efficiently add items to the tree or
	update existing items without requiring additional lookups.

    EXAMPLE
        This is a fragment which counts the occurances of every word in a file.
	Also note that this example embeds the struct AVLNode data at
	the start of the structure, so simple casts suffice for translating
	AVLNode to ExampleNode addresses.

	struct ExampleNode {
	    struct AVLNode avl;
	    STRPTR key;
	    ULONG count;
	};

	static LONG ExampleNodeComp(const struct AVLNode *a1, const struct AVLNode *a2)
	{
	    const struct ExampleNode *e1 = (const struct ExampleNode *)a1;
	    const struct ExampleNode *e2 = (const struct ExampleNode *)e2;

	    return strcmp(a1->key, a2->key);
	}

	static LONG ExampleKeyComp(const struct AVLNode *a1, AVLKey key)
	{
	    const struct ExampleNode *e1 = (const struct ExampleNode *)a1;
	    char *skey = key;

	    return strcmp(a1->key, skey);
	}

	void CountWords(wordfile)
	{
	    struct ExampleNode *node;
	    struct AVLNode *root = NULL, *check;

	    node = AllocMem(sizeof(struct ExampleNode), 0);
	    node->count = 1;

	    while (node->key = read_word(wordfile)) {
	    	check = AVL_AddNode(&root, &node->avl, ExampleNodecomp);

	    	if (check != NULL) {
		    struct ExampleNode *work = (struct ExampleNode *)check;

		    check->count += 1;
		} else {
		    free(node->key);
		    node = AllocMem(sizeof(struct ExampleNode), 0);
		    node->count = 1;
		}
	    }
	    FreeMem(node, sizeof(struct ExampleNode));
	}

    BUGS

    SEE ALSO
    	AVL_FindNode(), AVL_FindNextNodeByKey(), AVL_FindPrevNodeByKey(),
	AVL_RemNodeByAddress(), AVL_RemNodeByKey()

    INTERNALS
******************************************************************************/
{
    AROS_LIBFUNC_INIT;

    int dir;
    struct AVLNode *next = *root;
    struct AVLNode *scan;
    struct AVLNode *child;

    /* Simple case - empty tree */
    if (next == NULL)
    {
	*root = node;
	node->avl_link[0] = node->avl_link[1] = node->avl_parent = NULL;
	node->avl_balance = 0;
	return NULL;
    }

    /* find insertion point */
    do
    {
	LONG cmp;

	scan = next;
	cmp = AROS_UFC2(LONG, func,
			AROS_UFCA(const struct AVLNode *, scan, A0),
			AROS_UFCA(const struct AVLNode *, node, A1));
	if (cmp == 0)
	    return (struct AVLNode *)scan;

	dir = cmp < 0;
	next = scan->avl_link[dir];
    }
    while (next != NULL);

    /* insert */
    node->avl_parent = scan;
    scan->avl_link[dir] = node;
    node->avl_link[0] = node->avl_link[1] = NULL;
    node->avl_balance = 0;

    /* update balance factors */
    while (1)
    {
	LONG bal = (dir * 2 - 1) + scan->avl_balance;
	struct AVLNode *parent;
	struct AVLNode *subroot;

	switch (bal)
	{
	case 0:
	    scan->avl_balance = bal;
	    return NULL;

	case 2:
	    parent = scan->avl_parent;
	    next = scan->avl_link[1];

	    switch (next->avl_balance)
	    {
	    case -1:
		subroot = rotate_double(scan, next, LEFT);

		if (subroot->avl_balance == 1)
		{
		    scan->avl_balance = -1;
		    next->avl_balance = 0;
		}
		else if (subroot->avl_balance == -1)
		{
		    scan->avl_balance = 0;
		    next->avl_balance = 1;
		}
		else
		{
		    scan->avl_balance = next->avl_balance = 0;
		}

		subroot->avl_balance = 0;
		break;
	    default:
		subroot = rotate_single(scan, next, LEFT);

		scan->avl_balance = next->avl_balance = 0;
		break;
	    }

	    link_parent(root, scan, subroot, parent);
	    return NULL;

	case -2:
	    parent = scan->avl_parent;
	    next = scan->avl_link[0];

	    switch (next->avl_balance)
	    {
	    case 1:
		subroot = rotate_double(scan, next, RIGHT);

		if (subroot->avl_balance == -1)
		{
		    scan->avl_balance = 1;
		    next->avl_balance = 0;
		}
		else if (subroot->avl_balance == 1)
		{
		    scan->avl_balance = 0;
		    next->avl_balance = -1;
		}
		else
		{
		    scan->avl_balance = next->avl_balance = 0;
		}

		subroot->avl_balance = 0;
		break;
	    default:
		subroot = rotate_single(scan, next, RIGHT);

		scan->avl_balance = next->avl_balance = 0;
		break;
	    }

	    link_parent(root, scan, subroot, parent);
	    return NULL;

	default:
	    scan->avl_balance = bal;

	    child = scan;
	    scan = scan->avl_parent;
	    if (scan == NULL)
		return NULL;

	    dir = scan->avl_link[1] == child;
	    break;
	}
    }

    return NULL;

    AROS_LIBFUNC_EXIT;
}				/* AVL_AddNode */

/*****************************************************************************

    NAME */

    AROS_LH2I(struct AVLNode *, AVL_RemNodeByAddress,

/*  SYNOPSIS */
	  AROS_LHA(struct AVLNode **, Root, A0),
	  AROS_LHA(struct AVLNode *, Node, A1),
/*  LOCATION */
	  struct ExecBase *, SysBase, 140, Exec)
/*  FUNCTION
	Remove a given node from the tree.

    INPUTS
	Root - A pointer to a pointer to the root node of the tree.

	Node - A struct AVLNode to remove.  The node must be present
	       in the tree.

    RESULT
	Node is returned.

    NOTES
	Removing a node may require multiple rebalancing steps
	in the tree.  Each step however runs in constant time
	and no more than 1.44log(n) steps will be required.

    EXAMPLE
	See AVL_FindNextNodeByAddress(), AVL_FindPrevNodeByAddress()

    BUGS

    SEE ALSO
	AVL_AddNode(), AVL_FindNode(), AVL_FindNextNodeByAddress(),
	AVL_FindPrevNodeByAddress()

    INTERNALS
******************************************************************************/
{
    AROS_LIBFUNC_INIT;

    struct AVLNode *scan;
    struct AVLNode *node = (struct AVLNode *)Node;
    struct AVLNode **root = (struct AVLNode **)Root;
    int dir;

    if (node->avl_link[0] == NULL)
    {
	if (node->avl_link[1] == NULL)
	{
	    if (node->avl_parent == NULL)
	    {
		/* removed last node */
		*root = NULL;
		return Node;
	    }
	    else
	    {
		/* removed a leaf */
		scan = node->avl_parent;
		dir = scan->avl_link[1] == node;
		scan->avl_link[dir] = NULL;
		D(printf("removed leaf dir = %d\n", dir));
	    }
	}
	else
	{
	    if (node->avl_parent == NULL)
	    {
		/* removed root left-leaf node */
		*root = node->avl_link[1];
		node->avl_link[1]->avl_parent = NULL;
		return Node;
	    }
	    else
	    {
		/* removed a left-leaf */
		scan = node->avl_parent;
		dir = scan->avl_link[1] == node;
		scan->avl_link[dir] = node->avl_link[1];
		node->avl_link[1]->avl_parent = scan;
		D(printf("removed left leaf dir =%d\n", dir));
	    }
	}
    }
    else
    {
	if (node->avl_link[1] == NULL)
	{
	    if (node->avl_parent == NULL)
	    {
		/* removed root right-leaf node */
		*root = node->avl_link[0];
		node->avl_link[0]->avl_parent = NULL;
		return Node;
	    }
	    else
	    {
		/* removed a right-leaf */
		scan = node->avl_parent;
		dir = scan->avl_link[1] == node;
		scan->avl_link[dir] = node->avl_link[0];
		node->avl_link[0]->avl_parent = scan;
		D(printf("removed right leaf dir=%d\n", dir));
	    }
	}
	else
	{
	    /* removing a non-leaf node - swap it with a leaf node and 'remove' that instead */
	    struct AVLNode *toremove = (struct AVLNode *)AVL_FindPrevNodeByAddress(Node);	// do we do prev/next based on balance factor? */

	    D(printf("removing non-leaf key ... %p\n", toremove));

	    /* remove the leaf node */
	    scan = toremove->avl_parent;

	    if (scan == node)
	    {
		/* special case - immediate child of what we're removing */
		toremove->avl_link[1] = node->avl_link[1];
		if (node->avl_link[1] != NULL)
		    node->avl_link[1]->avl_parent = toremove;
		dir = 0;
		scan = toremove;
		D(printf("  removed immediate child\n"));
	    }
	    else
	    {
		dir = toremove->avl_parent->avl_link[1] == toremove;
		toremove->avl_parent->avl_link[dir] = toremove->avl_link[0];
		if (toremove->avl_link[0] != NULL)
		    toremove->avl_link[0]->avl_parent = toremove->avl_parent;

		/* swap it with the removed node */
		toremove->avl_link[0] = node->avl_link[0];
		if (node->avl_link[0] != NULL)
		    node->avl_link[0]->avl_parent = toremove;
		toremove->avl_link[1] = node->avl_link[1];
		if (node->avl_link[1] != NULL)
		    node->avl_link[1]->avl_parent = toremove;
		D(printf("  removed distant child\n"));
	    }
	    toremove->avl_balance = node->avl_balance;
	    toremove->avl_parent = node->avl_parent;
	    if (node->avl_parent == NULL)
		*root = toremove;
	    else
		node->avl_parent->avl_link[node->avl_parent->avl_link[1] ==
					   node] = toremove;

	    D(printf("removed key, tree is now:\n"));
	    D(dumptree(*root, 0));
	}
    }

    /* rebalance from 'scan' up */
    while (1)
    {
	int bal = scan->avl_balance - (dir * 2 - 1);
	struct AVLNode *next;
	struct AVLNode *parent;
	struct AVLNode *subroot = NULL;

	switch (bal)
	{
	case 0:
	    scan->avl_balance = bal;

	    parent = scan->avl_parent;
	    if (parent == NULL)
		return Node;

	    dir = parent->avl_link[1] == scan;
	    scan = parent;
	    break;

	case -1:
	case 1:
	    scan->avl_balance = bal;
	    return Node;

	case 2:
	    parent = scan->avl_parent;
	    next = scan->avl_link[1];

	    switch (next->avl_balance)
	    {
	    case 0:
		subroot = rotate_single(scan, next, LEFT);
		scan->avl_balance = 1;
		next->avl_balance = -1;
		// stop
		link_parent(root, scan, subroot, parent);
		return Node;
	    case 1:
		subroot = rotate_single(scan, next, LEFT);
		scan->avl_balance = 0;
		next->avl_balance = 0;
		// keep going
		break;
	    case -1:
		subroot = rotate_double(scan, next, LEFT);

		if (subroot->avl_balance == 1)
		{
		    scan->avl_balance = -1;
		    next->avl_balance = 0;
		}
		else if (subroot->avl_balance == -1)
		{
		    scan->avl_balance = 0;
		    next->avl_balance = 1;
		}
		else
		{
		    scan->avl_balance = 0;
		    next->avl_balance = 0;
		}

		subroot->avl_balance = 0;
		// keep going
		break;
	    }

	    dir = link_parent(root, scan, subroot, parent);
	    if (dir == -1)
		return Node;

	    scan = parent;
	    break;

	case -2:
	    parent = scan->avl_parent;
	    next = scan->avl_link[0];

	    switch (next->avl_balance)
	    {
	    case 0:
		subroot = rotate_single(scan, next, RIGHT);
		scan->avl_balance = -1;
		next->avl_balance = 1;
		// stop
		link_parent(root, scan, subroot, parent);
		return Node;
	    case -1:
		subroot = rotate_single(scan, next, RIGHT);
		scan->avl_balance = next->avl_balance = 0;
		// keep going
		break;
	    case +1:
		subroot = rotate_double(scan, next, RIGHT);

		if (subroot->avl_balance == -1)
		{
		    scan->avl_balance = 1;
		    next->avl_balance = 0;
		}
		else if (subroot->avl_balance == 1)
		{
		    scan->avl_balance = 0;
		    next->avl_balance = -1;
		}
		else
		{
		    scan->avl_balance = 0;
		    next->avl_balance = 0;
		}

		subroot->avl_balance = 0;
		// keep going
		break;
	    }

	    dir = link_parent(root, scan, subroot, parent);
	    if (dir == -1)
		return Node;

	    scan = parent;
	    break;
	}
    }

    AROS_LIBFUNC_EXIT;
}				/* AVL_RemNodeByAddress */

/*****************************************************************************

    NAME */

    AROS_LH3I(struct AVLNode *, AVL_RemNodeByKey,

/*  SYNOPSIS */
	  AROS_LHA(struct AVLNode **, root, A0),
	  AROS_LHA(AVLKey, key, A1), AROS_LHA(AVLKEYCOMP, func, A2),
/*  LOCATION */
	  struct ExecBase *, SysBase, 141, Exec)
/*  FUNCTION
	Looks up a node in the tree by key, and removes it if it
	is found.

    INPUTS
	Root - A pointer to a pointer to the root node of the AVL tree.

	key  - The key to search for.

	func - An AVLKEYCOMP function used to compare the key and nodes
	       in the tree.

    RESULT
	If the key was present in the tree, then a pointer to the node
	for that key.  Otherwise NULL if no such key existed in the
	tree.

    NOTES
	See AVL_RemNodeByAddress().

    EXAMPLE

    BUGS

    SEE ALSO
	AVL_AddNode(), AVL_RemNodeByAddress()

    INTERNALS
******************************************************************************/
{
    AROS_LIBFUNC_INIT;

    struct AVLNode *node;

    node = AVL_FindNode(*root, key, func);

    if (node != NULL)
	node = AVL_RemNodeByAddress(root, node);

    return node;

    AROS_LIBFUNC_EXIT;
}				/* AVL_RemNodeByKey */

/*****************************************************************************

    NAME */

    AROS_LH3I(struct AVLNode *, AVL_FindNode,

/*  SYNOPSIS */
	  AROS_LHA(const struct AVLNode *, Root, A0),
	  AROS_LHA(AVLKey, key, A1), AROS_LHA(AVLKEYCOMP, func, A2),
/*  LOCATION */
	  struct ExecBase *, SysBase, 142, Exec)
/*  FUNCTION
	Find an entry in the AVL tree by key.

    INPUTS
	Root - The root of the AVL tree.

	key  - The key to search.

	func - The AVLKEYCOMP key comparision function for this tree.

    RESULT
	A pointer to the node matching key if it exists in the
	tree, or NULL if no such node exists.

    NOTES

    EXAMPLE
	node = (struct ExampleNode *)AVL_FindNode(root, "aros", ExampleKeyComp);
	if (node)
	    printf(" `%s' occurs %d times\n", node->key, node->count);

    BUGS

    SEE ALSO
	AVL_AddNode(), AVL_RemNodeByAddress(), AVL_FindNextNodeByAddress(),
	AVL_FindPrevNodeByAddress()

    INTERNALS
******************************************************************************/
{
    AROS_LIBFUNC_INIT;

    struct AVLNode *node = (struct AVLNode *)Root;
    LONG cmp;

    while (node != NULL &&
	   (cmp = AROS_UFC2(LONG, func,
			AROS_UFCA(const struct AVLNode *, node, A0),
			AROS_UFCA(AVLKey,                 key,  A1))) != 0)
    {
	node = node->avl_link[cmp < 0];
    }

    return (struct AVLNode *)node;

    AROS_LIBFUNC_EXIT;
}				/* AVL_FindNode */

/*****************************************************************************

    NAME */

    AROS_LH1I(struct AVLNode *, AVL_FindPrevNodeByAddress,

/*  SYNOPSIS */
	  AROS_LHA(const struct AVLNode *, Node, A0),
/*  LOCATION */
	  struct ExecBase *, SysBase, 143, Exec)
/*  FUNCTION
	Perform an inverse-order traversal to the previous node in the tree.

    INPUTS
	Node - The current node.  The node must be present in the tree.

    RESULT
	The next-least node in the tree, or NULL if Node was already
	the lowest valued node in the tree.

    NOTES
	This implementation uses a parent pointer to avoid needing
	a stack or state to track it's current position in the tree.

	Although this operation is typically O(1), in the worst case it
	iterate the full depth of the tree - approximately 1.44Log(N).

    EXAMPLE
	... inverse-order traversal of all nodes
	node = (struct ExampleNode *)AVL_FindLastNode(root);
	while (node) {
	    printf(" %s\n", node->key);
	    node = (struct ExampleNode *)AVL_FindPrevNodeByAddress(node);
	}

	... inverse-order traversal of all nodes - with safe deletion
	node = (struct ExampleNode *)AVL_FindLastNode(root);
	prev = (struct ExampleNode *)AVL_FindPrevNodeByAddress(node);
	while (node) {
	    printf(" %s\n", node->key);

	    if (DELETE_NODE(node))
	        AVL_RemNodeByAddress(&root, node);

	    node = prev;
	    prev = (struct ExampleNode *)AVL_FindPrevNodeByAddress(node);
	}

    BUGS

    SEE ALSO
	AVL_AddNode(), AVL_RemNodeByAddress(), AVL_FindLastNode()

    INTERNALS
******************************************************************************/
{
    AROS_LIBFUNC_INIT;

    const struct AVLNode *node = (const struct AVLNode *)Node;
    const struct AVLNode *prev;

    prev = node->avl_link[0];
    if (prev != NULL)
    {
	do
	{
	    node = prev;
	    prev = prev->avl_link[1];
	}
	while (prev);
    }
    else
    {
	do
	{
	    prev = node;
	    node = node->avl_parent;
	}
	while (node != NULL && node->avl_link[1] != prev);
    }

    return (struct AVLNode *)node;

    AROS_LIBFUNC_EXIT;
}				/* AVL_FindPrevNodeByAddress */

/*****************************************************************************

    NAME */

    AROS_LH3I(struct AVLNode *, AVL_FindPrevNodeByKey,

/*  SYNOPSIS */
	  AROS_LHA(const struct AVLNode *, root, A0),
	  AROS_LHA(AVLKey, key, A1), AROS_LHA(AVLKEYCOMP, func, A2),
/*  LOCATION */
	  struct ExecBase *, SysBase, 144, Exec)
/*  FUNCTION
	Find the node matching the key, or if such a node does not exist,
	then the node with the next-lowest value.

    INPUTS
	Root - The root of the AVL tree.

	key  - The key to search.

	func - The AVLKEYCOMP key comparision function for this tree.

    RESULT
	A pointer to the struct AVLNode which matches this key, or
	if that key is not present in the tree, the next lowest node.
	Or NULL if key is lower than the key of any node in the tree.

    NOTES
	This is only O(1.44log(n)) in the worst case.

    EXAMPLE

    BUGS

    SEE ALSO
	AVL_AddNode(), AVL_FindNextNodeByAddress(), AVL_FindLastNode()

    INTERNALS
******************************************************************************/
{
    AROS_LIBFUNC_INIT;

    struct AVLNode *node = (struct AVLNode *)root;
    struct AVLNode *lesser = NULL;
    LONG cmp;

    while (node != NULL &&
	   (cmp = AROS_UFC2(LONG, func,
			AROS_UFCA(const struct AVLNode *, node, A0),
			AROS_UFCA(AVLKey,                 key,  A1))) != 0)
    {
	if (cmp < 0)
	    lesser = node;
	node = node->avl_link[cmp < 0];
    }

    return (struct AVLNode *)(node != NULL ? node : lesser);

    AROS_LIBFUNC_EXIT;
}				/* AVL_FindPrevNodeByKey */

/*****************************************************************************

    NAME */

    AROS_LH1I(struct AVLNode *, AVL_FindNextNodeByAddress,

/*  SYNOPSIS */
	  AROS_LHA(const struct AVLNode *, Node, A0),
/*  LOCATION */
	  struct ExecBase *, SysBase, 145, Exec)
/*  FUNCTION
	Perform an in-order traversal to the next node in the tree.

    INPUTS
	Node - The current node.  The node must be present in the tree.

    RESULT
	The next-greatest node in the tree, or NULL if Node was already
	the highest valued node in the tree.

    NOTES
	This implementation uses a parent pointer to avoid needing
	a stack or state to track it's current position in the tree.

	Although this operation is typically O(1), in the worst case it
	iterate the full depth of the tree - approximately 1.44Log(N).

    EXAMPLE
	... in-order traversal of all nodes
	node = (struct ExampleNode *)AVL_FindFirstNode(root);
	while (node) {
	    printf(" %s\n", node->key);
	    node = (struct ExampleNode *)AVL_FindNextNodeByAddress(node);
	}

	... in-order traversal of all nodes - with safe deletion
	node = (struct ExampleNode *)AVL_FindFirstNode(root);
	next = (struct ExampleNode *)AVL_FindNextNodeByAddress(node);
	while (node) {
	    printf(" %s\n", node->key);

	    if (DELETE_NODE(node))
	        AVL_RemNodeByAddress(&root, node);

	    node = next;
	    next = (struct ExampleNode *)AVL_FindNextNodeByAddress(node);
	}

    BUGS

    SEE ALSO
	AVL_AddNode(), AVL_RemNodeByAddress(), AVL_FindFirstNode()

    INTERNALS
******************************************************************************/
{
    AROS_LIBFUNC_INIT;

    const struct AVLNode *node = (const struct AVLNode *)Node;
    const struct AVLNode *next;

    next = node->avl_link[1];
    if (next != NULL)
    {
	do
	{
	    node = next;
	    next = node->avl_link[0];
	}
	while (next != NULL);
    }
    else
    {
	do
	{
	    next = node;
	    node = node->avl_parent;
	}
	while (node != NULL && node->avl_link[1] == next);
    }

    return (struct AVLNode *)node;

    AROS_LIBFUNC_EXIT;
}				/* AVL_FindNextNodeByAddress */

/*****************************************************************************

    NAME */

    AROS_LH3I(struct AVLNode *, AVL_FindNextNodeByKey,

/*  SYNOPSIS */
	  AROS_LHA(const struct AVLNode *, Root, A0),
	  AROS_LHA(AVLKey, key, A1), AROS_LHA(AVLKEYCOMP, func, A2),
/*  LOCATION */
	  struct ExecBase *, SysBase, 146, Exec)
/*  FUNCTION
	Find the node matching the key, or if such a node does not exist,
	then the node with the next-highest value.

    INPUTS
	Root - The root of the AVL tree.

	key  - The key to search.

	func - The AVLKEYCOMP key comparision function for this tree.

    RESULT
	A pointer to the struct AVLNode which matches this key, or
	if that key is not present in the tree, the next highest node.
	Or NULL if key is higher than the key of any node in the tree.

    NOTES
	This is only O(1.44log(n)) in the worst case.

    EXAMPLE

    BUGS

    SEE ALSO
	AVL_AddNode(), AVL_FindNextNodeByAddress(), AVL_FindLastNode()

    INTERNALS
******************************************************************************/
{
    AROS_LIBFUNC_INIT;

    struct AVLNode *node = (struct AVLNode *)Root;
    struct AVLNode *greater = NULL;
    LONG cmp;

    while (node != NULL &&
	   (cmp = AROS_UFC2(LONG, func,
			AROS_UFCA(const struct AVLNode *, node, A0),
			AROS_UFCA(AVLKey,                 key,  A1))) != 0)
    {
	if (cmp > 0)
	    greater = node;
	node = node->avl_link[cmp < 0];
    }

    return (struct AVLNode *)(node != NULL ? node : greater);

    AROS_LIBFUNC_EXIT;
}				/* AVL_FindNextNodeByKey */

/*****************************************************************************

    NAME */

    AROS_LH1I(struct AVLNode *, AVL_FindFirstNode,

/*  SYNOPSIS */
	  AROS_LHA(const struct AVLNode *, Root, A0),
/*  LOCATION */
	  struct ExecBase *, SysBase, 147, Exec)
/*  FUNCTION
	Find the smallest node in an AVL tree.

    INPUTS
	Root - The root node of the AVL tree.

    RESULT
	NULL if the tree is empty (i.e. Root is NULL), or the node
	which contains the least value in the tree as determined by
	the comparision function used to add the node.

    NOTES
	AVL trees are balanced but not minimal.  This operation
	must iterate the full depth of the tree on one side -
	approximately 1.44Log(N).

    EXAMPLE
	See AVL_FindNextNodeByAddress()

    BUGS

    SEE ALSO
	AVL_AddNode(), AVL_FindNextNodeByAddress()

    INTERNALS
******************************************************************************/
{
    AROS_LIBFUNC_INIT;

    struct AVLNode *node = (struct AVLNode *)Root;

    if (node != NULL)
	while (node->avl_link[0] != NULL)
	    node = node->avl_link[0];

    return (struct AVLNode *)node;

    AROS_LIBFUNC_EXIT;
}				/* AVL_FindFirstNode */

/*****************************************************************************

    NAME */

    AROS_LH1I(struct AVLNode *, AVL_FindLastNode,

/*  SYNOPSIS */
	  AROS_LHA(const struct AVLNode *, Root, A0),
/*  LOCATION */
	  struct ExecBase *, SysBase, 148, Exec)
/*  FUNCTION
	Find the largest node in an AVL tree.

    INPUTS
	Root - The root node of the AVL tree.

    RESULT
	NULL if the tree is empty (i.e. Root is NULL), or the node
	which contains the greatest value in the tree as determined by
	the comparision function used to add the node.

    NOTES
	AVL trees are balanced but not minimal.  This operation
	must iterate the full depth of the tree on one side -
	approximately 1.44Log(N).

    EXAMPLE
	See AVL_FindPrevNodeByAddress()

    BUGS

    SEE ALSO
	AVL_AddNode(), AVL_FindPrevNodeByAddress()

    INTERNALS
******************************************************************************/
{
    AROS_LIBFUNC_INIT;

    struct AVLNode *node = (struct AVLNode *)Root;

    if (node != NULL)
	while (node->avl_link[1] != NULL)
	    node = node->avl_link[1];

    return (struct AVLNode *)node;

    AROS_LIBFUNC_EXIT;
}				/* AVL_FindLastNode */
