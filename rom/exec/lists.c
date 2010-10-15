/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.

#Description: @AROS.Exec.LISTS@

<chapter title="Exec Lists">

<p>Exec offers a standard way to create lists of structures. All you
need to do is to put <code>struct Node node;</code>
as the first
field into any structure you want to collect in a list. You can
then use the macro <code>NEWLIST()</code> to initialize a structure
of the type <code>struct List</code> and insert the nodes
with <code>AddHead()</code>, <code>AddTail()</code>, 
<code>Enqueue()</code> or <code>Insert()</code>.<p>

<p>FIXME Explain other functions, how to use the macros,
give some example programs and explain how lists work.<p>

</chapter>

*/
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
	Insert Node <code>node</code> as the first node of the list.

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
	@AROS.Exec.LISTS@

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
	void AddTail (

/*  SYNOPSIS */
	struct List * list, /*#A0 
	    The list to insert the node into */
	struct Node * node) /*#A1
	    This node is to be inserted */

/*  LOCATION
	Exec (41), BaseNotNecessary

    FUNCTION
	Insert Node node at the end of a list.

    RESULT

    NOTES

    EXAMPLE
	struct List * list;
	struct Node * pred;

	// Insert Node at end of the list
	AddTail (list, node);

    BUGS

    SEE ALSO
	@AROS.Exec.LISTS@

    INTERNALS

******************************************************************************/
{
//  ASSERT_VALID_PTR(node); argh! TypeOfMem() doesn't know about the data segment!
    ASSERT_VALID_PTR(list);

    /*
	Make the node point to the head of the list. Our predecessor is the
	previous last node of the list.
    */
    node->ln_Succ	       = (struct Node *)&list->lh_Tail;
    node->ln_Pred	       = list->lh_TailPred;

    /*
	Now we are the last now. Make the old last node point to us
	and the pointer to the last node, too.
    */
    list->lh_TailPred->ln_Succ = node;
    list->lh_TailPred	       = node;
} /* AddTail */

/*#AROS_LH********************************************************************

    NAME */
	void Enqueue (

/*  SYNOPSIS */
	struct List * list, /*#A0 
	    Insert into this list. The list has to be in descending
	    order in respect to the field ln_Pri of all nodes. */
	struct Node * node) /*#A1
	    This node is to be inserted. Note that this has to
	    be a complete node and not a MinNode ! */

/*  LOCATION
	Exec (45), BaseNotNecessary

    FUNCTION
	Sort a node into a list. The sort-key is the field 
	<code>node->ln_Pri</code>.
	The node will be inserted into the list before the first node
	with lower priority. This creates a FIFO queue for nodes with
	the same priority.

    RESULT
	The new node will be inserted before nodes with lower
	priority.

    NOTES
	The list has to be in descending order in respect to the field
	<code>ln_Pri</code> of all nodes.

    EXAMPLE
	struct List * list;
	struct Node * node;

	node->ln_Pri = 5;

	// Sort the node at the correct place into the list
	Enqueue (list, node);

    BUGS

    SEE ALSO
	@AROS.Exec.LISTS@

    INTERNALS

******************************************************************************/
{
    struct Node * next;

    assert (list);
    assert (node);

    /* Look through the list */
    ForeachNode (list, next)
    {
	/*
	    Look for the first node with a lower pri as the node
	    we have to insert into the list.
	*/
	if (node->ln_Pri > next->ln_Pri)
	    break;
    }

    /* Insert the node before(!) next. The situation looks like this:

	    A<->next<->B *<-node->*

	ie. next->ln_Pred points to A, A->ln_Succ points to next,
	next->ln_Succ points to B, B->ln_Pred points to next.
	ln_Succ and ln_Pred of node contain illegal pointers.
    */

    /* Let node point to A: A<-node */
    node->ln_Pred	   = next->ln_Pred;

    /* Let A point to node: A->node */
    next->ln_Pred->ln_Succ = node;

    /* Make next point to node: A<->node<-next<->B */
    next->ln_Pred	   = node;

    /* Make node point to next: A<->node->next<->B */
    node->ln_Succ	   = next;

} /* Enqueue */

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

	When supplied with a NULL list argument, defaults to the exec port list.

    EXAMPLE
	struct List * list;
	struct Node * node;

	// Look for a node with the name "Hello"
	node = FindName (list, "Hello");

    BUGS

    SEE ALSO
	@AROS.Exec.LISTS@

    INTERNALS

******************************************************************************/
{
    struct Node * node;

/* FIX
	FindNode when supplied with a NULL list searches the exec ports list.
	per ampurtle, reference RKM 1.0
	Hmmm... why is there also a separate FindName.c
	Both files changed....
*/
    if( !list )
	list = &SysBase->PortList;

    /* assert (list); */
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
	didn't, this will be <code>NULL</code> (either because the list was
	empty or because we tried all nodes in the list)
    */
    return node;
} /* FindName */

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
	Insert Node <code>node</code> after <code>pred</code> in list.

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
	@AROS.Exec.LISTS@

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
	    add at the top of the list. I do not use <code>AddHead()</code>
	    here but write the code twice for two reasons: 1. The code is small
	    and therefore errors are unlikely and 2. If I would call 
	    <code>AddHead()</code>, it would take almost as long to call the
	    function as the execution would take yielding 100% overhead.
	*/
	node->ln_Succ	       = list->lh_Head;
	node->ln_Pred	       = (struct Node *)&list->lh_Head;
	list->lh_Head->ln_Pred = node;
	list->lh_Head	       = node;
    }
} /* Insert */

/*#AROS_LH********************************************************************

    NAME */
	struct Node * RemHead (

/*  SYNOPSIS */
	struct List * list) /*#A0
	    The list to remove the node from */

/*  LOCATION
	Exec (43), BaseNotNecessary

    FUNCTION
	Remove the first node from a list.

    RESULT
	The node that has been removed.

    NOTES

    EXAMPLE
	struct List * list;
	struct Node * head;

	// Remove node and return it
	head = RemHead (list);

    BUGS

    SEE ALSO
	@AROS.Exec.LISTS@

    INTERNALS

******************************************************************************/
{
    struct Node * node;

    assert (list);
    /*
	Unfortunately, there is no (quick) check that the node
	is in a list
    */

    /* Get the address of the first node or NULL */
    node = list->lh_Head->ln_Succ;
    if (node)
    {
	node->ln_Pred = (struct Node *)list;
	node = list->lh_Head;
	list->lh_Head = node->ln_Succ;
    }

    /* Return the address or NULL */
    return node;
} /* RemHead */

/*#AROS_LH********************************************************************

    NAME */
	void Remove (

/*  SYNOPSIS */
	struct Node * node) /*#A1
	    Remove this node from the list it is currently in. */

/*  LOCATION
	Exec (42), BaseNotNecessary

    FUNCTION
	Remove a node from a list.

    RESULT

    NOTES
	There is no need to specify the list but the node must be in
	a list !

    EXAMPLE
	struct Node * node;

	// Remove node
	Remove (node);

    BUGS

    SEE ALSO
	@AROS.Exec.LISTS@

    INTERNALS

******************************************************************************/
{
    assert (node);
    /*
	Unfortunately, there is no (quick) check that the node
	is in a list.
    */

    /*
	Just bend the pointers around the node, ie. we make our
	predecessor point to our successor and vice versa
    */
    node->ln_Pred->ln_Succ = node->ln_Succ;
    node->ln_Succ->ln_Pred = node->ln_Pred;
} /* Remove */

/*#AROS_LH********************************************************************

    NAME */
	struct Node * RemTail (

/*  SYNOPSIS */
	struct List * list) /*#A0
	    The list to remove the node from */

/*  LOCATION
	Exec (44), BaseNotNecessary

    FUNCTION
	Remove the last node from a list.

    RESULT
	The node that has been removed.

    NOTES

    EXAMPLE
	struct List * list;
	struct Node * tail;

	// Remove node and return it
	tail = RemTail (list);

    BUGS

    SEE ALSO
	@AROS.Exec.LISTS@

    INTERNALS

******************************************************************************/
{
    struct Node * node;

    assert (list);
    /*
	Unfortunately, there is no (quick) check that the node
	is in a list.
    */

    /* Get the last node of the list */
    if ( (node = GetTail (list)) )
    {
	/* normal code to remove a node if there is one */
	node->ln_Pred->ln_Succ = node->ln_Succ;
	node->ln_Succ->ln_Pred = node->ln_Pred;
    }

    /* return it's address or NULL if there was no node */
    return node;
} /* RemTail */

