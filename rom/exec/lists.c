/*
    (C) 1995-2001 AROS - The Amiga Research OS

    Desc: Exec lists
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE
#include <string.h>
#include <exec/lists.h> /*#ALL*/
#include <proto/exec.h> /*#ALL*/

/*#AROS_LH********************************************************************

    NAME */
	void AddHead (

/*  SYNOPSIS */
	struct List * list, /*#A0 
	    The list to insert the node into */
	struct Node * node) /*#A1
	    This node is to be inserted */

/*  LOCATION
	Exec (40), BaseNotNecessary

    FUNCTION
	Insert Node node as the first node of the list.

    RESULT
	None.

    NOTES

    EXAMPLE
	struct List * list;
	struct Node * pred;

	// Insert Node at top
	AddHead (list, node);

    BUGS

    SEE ALSO
	@LISTS@

    INTERNALS

******************************************************************************/
{
    ASSERT_VALID_PTR(node);
    ASSERT_VALID_PTR(list);

    /*
	Make the node point to the old first node in the list and to the
	head of the list.
    */
    node->ln_Succ	   = list->lh_Head;
    node->ln_Pred	   = (struct Node *)&list->lh_Head;

    /*
	New we come before the old first node which must now point to us
	and the same applies to the pointer to-the-first-node in the
	head of the list.
    */
    list->lh_Head->ln_Pred = node;
    list->lh_Head	   = node;
} /* AddHead */

/*#AROS_LH********************************************************************

    NAME */
	void Insert (

/*  SYNOPSIS */
	struct List * list, /*#A0
	    The list to insert the node into */
	struct Node * node, /*#A1
	    This node is to be inserted */
	struct Node * pred) /*#A2
	    Insert after this node. If this is NULL, node is inserted
	    as the first node (same as AddHead()) */

/*  LOCATION
	Exec (39), BaseNotNecessary

    FUNCTION
	Insert Node node after pred in list.

    RESULT

    NOTES

    EXAMPLE
	struct List * list;
	struct Node * pred, * node;

	// Insert Node node as second node in list
	pred = GetHead (list);
	Insert (list, node, pred);

    BUGS

    SEE ALSO
	@LISTS@

    INTERNALS

******************************************************************************/
{
    assert (node);
    assert (list);

    /* If we have a node to insert behind... */
    if (pred)
    {
	/* Is this the last node in the list ? */
	if (pred->ln_Succ) /* Normal node ? */
	{
	    /*
		Our successor is the successor of the node we add ourselves
		behind and our predecessor is just the node itself.
	    */
	    node->ln_Succ = pred->ln_Succ;
	    node->ln_Pred = pred;

	    /*
		We are the predecessor of the successor of our predecessor
		(What ? blblblb... ;) and of our predecessor itself.
		Note that here the sequence is quite important since
		we need ln_Succ in the first expression and change it in
		the second.
	    */
	    pred->ln_Succ->ln_Pred = node;
	    pred->ln_Succ = node;
	}
	else /* last node */
	{
	    /*
		Add the node at the end of the list.
		Make the node point to the head of the list. Our
		predecessor is the previous last node of the list.
	    */
	    node->ln_Succ	       = (struct Node *)&list->lh_Tail;
	    node->ln_Pred	       = list->lh_TailPred;

	    /*
		Now we are the last now. Make the old last node point to us
		and the pointer to the last node, too.
	    */
	    list->lh_TailPred->ln_Succ = node;
	    list->lh_TailPred	       = node;
	}
    }
    else
    {
	/*
	    add at the top of the list. I do not use AddHead() here but
	    write the code twice for two reasons: 1. The code is small and
	    therefore errors are unlikely and 2. If I would call AddHead(),
	    it would take almost as long to call the function as the execution
	    would take yielding 100% overhead.
	*/
	node->ln_Succ	       = list->lh_Head;
	node->ln_Pred	       = (struct Node *)&list->lh_Head;
	list->lh_Head->ln_Pred = node;
	list->lh_Head	       = node;
    }
} /* Insert */

/*#AROS_LH********************************************************************

    NAME */
	struct Node * FindName (

/*  SYNOPSIS */
	struct List * list, /*#A0
	    Search this list. */
	UBYTE       * name) /*#A1
	    This is the name to look for. */

/*  LOCATION
	Exec (46), BaseNotNecessary

    FUNCTION
	Look for a node with a certain name in a list.

    RESULT

    NOTES
	The search is case-sensitive, so "Hello" will not find a node
	named "hello".

	The list must contain complete Nodes and no MinNodes.

    EXAMPLE
	struct List * list;
	struct Node * node;

	// Look for a node with the name "Hello"
	node = FindName (list, "Hello");

    BUGS

    SEE ALSO
	@LISTS@

    INTERNALS

******************************************************************************/
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
} /* FindName */

