/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Support files for purify (incomplete)
    Lang: english
*/

/* Local prototypes */
#include <exec/lists.h>
#include <exec/execbase.h>
#include <string.h>
#include <stdlib.h>
#include <proto/arossupport.h>
#include <proto/exec.h>
#include <aros/purify.h>
#include <aros/rt.h>

struct PNode
{
    struct Node Node;
    UBYTE     * Memory;
    ULONG	Size;
    UBYTE	State[0];
};

static const char * pmsNames[] =
{
    "free", "empty", "init.", "ro"
};

static struct List P_Memory;
static int InitWasCalled;

static struct PNode * FindNode (APTR memPtr);
static int FindNextNodes (APTR memPtr, struct PNode **, struct PNode **);

extern void RT_ShowRTStack (void);

/*****************************************************************************

    NAME */
	#include <aros/purify.h>

	void Purify_Init (

/*  SYNOPSIS */
	void)

/*  FUNCTION
	Initialize purify.

    INPUTS
	none

    RESULT
	none

    NOTES
	This function is not part of any library and may thus be called at
	any time.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    NEWLIST (&P_Memory);
} /* Purify_Init */


/*****************************************************************************

    NAME */
	#include <aros/purify.h>

	void Purify_AddMemory (

/*  SYNOPSIS */
	APTR  memPtr,
	ULONG size)

/*  FUNCTION
	Add size bytes of memory at memPtr to watch. Any access to this
	memory will be checked after this call.

    INPUTS
	memPtr - Start of the memory block
	size - The size of the memory block

    RESULT
	none

    NOTES
	This function is not part of any library and may thus be called at
	any time.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    struct PNode * node;

    if (!InitWasCalled)
	Purify_Init ();

    if (!(node = malloc (sizeof (struct PNode) + size)) )
	return;

    node->Memory = memPtr;
    node->Size	 = size;
    memset (node->State, PMS_FREE, size);

    AddHead (&P_Memory, (struct Node *)node);
} /* Purify_AddMemory */


/*****************************************************************************

    NAME */
	#include <aros/purify.h>

	void Purify_SetState (

/*  SYNOPSIS */
	APTR  memPtr,
	ULONG size,
	ULONG state)

/*  FUNCTION
	Brings a block of memory into a certain state (eg. empty, initialized,
	read-only). memPtr and size must be within a block beforehand
	declared with Purify_AddMemory().

    INPUTS
	memPtr - Where to start
	size - How many bytes after memPtr
	state - The new state of the memory:
		PMS_EMPTY - The memory may be read from and written to,
			but any read before the first write will yield an
			error (read from uninitialized memory).
		PMS_INITIALIZED - The memory has been initialized and may
			be read and writen to.
		PMS_READONLY - The memory may be read but not written to.
		PMS_FREE - The memory may not be read from or written to.

    RESULT
	none

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    struct PNode * node, * nodeBefore, * nodeAfter;
    UBYTE * mem;

    if (!InitWasCalled)
	Purify_Init ();

    /* Look for a node which contains memPtr */
    node = FindNode (memPtr);

    mem = memPtr;

    /* Is there one ? */
    if (!node)
    {
	/*  Look for the node we know about which comes least before and
	    right next after memPtr. This will _not_ return any node
	    which contains memPtr because there is none. If there are
	    none or the area we look for is in none of them, print a
	    message and exit. */
	if (!FindNextNodes (memPtr, &nodeBefore, &nodeAfter)
	    || mem+size <= nodeAfter->Memory
	)
	{
	    kprintf ("Purify: Tried to set state %s for unpurifed memory at %p:%ld\n"
		, pmsNames[state]
		, memPtr
		, size
	    );
	    return;
	}

	/*  If we get here, we have found a node which comes after memPtr
	    but memPtr is not inside the node */
    }
    else
    {
	if (mem+size <= node->Memory+node->Size)
	    memset (&node->State[mem - node->Memory], state, size);
	else
	    kprintf ("Purify: %p:%ld exceeds PNode %p:%ld\n"
		, mem
		, size
		, node->Memory
		, node->Size
	    );
    }
} /* Purify_SetState */


/*****************************************************************************

    NAME */
	#include <aros/purify.h>

	void Purify_CheckAccess (

/*  SYNOPSIS */
	APTR  memPtr,
	ULONG size,
	ULONG type)

/*  FUNCTION
	Checks a specific kind of access to memPtr[0...size-1].

    INPUTS
	memPtr - Where the access happens
	size - How many bytes are accessed
	type - Kind of access (PMA_READ, PMA_WRITE or PMA_MODIFY)

    RESULT
	none

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    struct PNode * node;
    UBYTE * mem;

    if (!InitWasCalled)
	Purify_Init ();

    /* Look for a node which contains memPtr */
    node = FindNode (memPtr);

    mem = memPtr;

    /* Is there one ? */
    if (!node)
    {
	kprintf ("Purify: Illegal access %p:%ld\n"
	    , mem
	    , size
	);
	RT_ShowRTStack ();
    }
    else
    {
	if (mem+size > node->Memory+node->Size)
	    kprintf ("Purify: Access %p:%ld beyond bounds %p:%ld\n"
		, mem
		, size
		, node->Memory
		, node->Size
	    );
	else
	{
	    switch (type)
	    {
	    case PMA_READ:
		mem = &node->State[mem - node->Memory];

		for ( ; size; size --)
		{
		    switch (*mem)
		    {
		    case PMS_EMPTY:
			kprintf ("Purify: Read of undefined memory at %p (%ld bytes from the beginning of the block)\n"
			    , node->Memory - node->State + mem
			    , mem - node->State
			);
			RT_ShowRTStack ();
			size = 1;
			break;

		    case PMS_INITIALIZED:
			break;

		    case PMS_READONLY:
			break;

		    case PMS_FREE:
			kprintf ("Purify: Read of freed memory at %p (%ld bytes from the beginning of the block)\n"
			    , node->Memory - node->State + mem
			    , mem - node->State
			);
			RT_ShowRTStack ();
			size = 1;
			break;
		    } /* switch */
		} /* for */

		break;

	    case PMA_WRITE:
		mem = &node->State[mem - node->Memory];

		for ( ; size; size --)
		{
		    switch (*mem)
		    {
		    case PMS_EMPTY:
			*mem = PMS_INITIALIZED;
			break;

		    case PMS_INITIALIZED:
			break;

		    case PMS_READONLY:
			kprintf ("Purify: Write to readonly memory at %p (%ld bytes from the beginning of the block)\n"
			    , node->Memory - node->State + mem
			    , mem - node->State
			);
			RT_ShowRTStack ();
			size = 1;
			break;
			break;

		    case PMS_FREE:
			kprintf ("Purify: Write to freed memory at %p (%ld bytes from the beginning of the block)\n"
			    , node->Memory - node->State + mem
			    , mem - node->State
			);
			RT_ShowRTStack ();
			size = 1;
			break;
		    } /* switch */
		}

		break;

	    case PMA_MODIFY:
		mem = &node->State[mem - node->Memory];

		for ( ; size; size --)
		{
		    switch (*mem)
		    {
		    case PMS_EMPTY:
			kprintf ("Purify: Modify of undefined memory at %p (%ld bytes from the beginning of the block)\n"
			    , node->Memory - node->State + mem
			    , mem - node->State
			);
			RT_ShowRTStack ();
			size = 1;
			break;

		    case PMS_INITIALIZED:
			break;

		    case PMS_READONLY:
			kprintf ("Purify: Modify of readonly memory at %p (%ld bytes from the beginning of the block)\n"
			    , node->Memory - node->State + mem
			    , mem - node->State
			);
			RT_ShowRTStack ();
			size = 1;
			break;

		    case PMS_FREE:
			kprintf ("Purify: Modify of freed memory at %p (%ld bytes from the beginning of the block)\n"
			    , node->Memory - node->State + mem
			    , mem - node->State
			);
			RT_ShowRTStack ();
			size = 1;
			break;
		    } /* switch */
		}

		break;

	    } /* switch (access type) */
	} /* Complete within bounds ? */
    } /* Node with memPtr found ? */
} /* Purify_CheckAccess */


/*****************************************************************************

    NAME */
	static struct PNode * FindNode (

/*  SYNOPSIS */
	APTR memPtr)

/*  FUNCTION
	Searches for the PNode which contains memPtr.

    INPUTS
	memPtr - A pointer into a piece of memory previously made known
		with Purify_AddMemory.

    RESULT
	A pointer to a PNode which contains the memPtr or NULL if there
	is no such pointer. No error will be printed.

    NOTES
	Must not be called before Purify_Init().

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    struct PNode * node;

    for (node=(struct PNode *)GetHead(&P_Memory); node; node=(struct PNode *)GetSucc(node))
    {
	if (node->Memory <= (UBYTE *)memPtr
	    || (UBYTE *)memPtr < node->Memory+node->Size
	)
	    break;
    }

    return node;
} /* FindNode */


/*****************************************************************************

    NAME */
	static int FindNextNodes (

/*  SYNOPSIS */
	APTR		memPtr,
	struct PNode ** before,
	struct PNode ** after)

/*  FUNCTION
	Returns the addresses of the PNodes right before and right right
	after memPtr.

    INPUTS
	memPtr - The address to look for
	before - Pointer to a pointer to PNode where the address of
		the node right before memPtr will be stored.
	after - Pointer to a pointer to PNode where the address of
		the node right after memPtr will be stored.

    RESULT
	The number of found PNodes. *before will contain a pointer to
	the PNode which is before memPtr or which contains memPtr or NULL
	if there is no node before PNode. *after will contain a pointer
	to the first PNode which comes right after memPtr or NULL if no
	PNode follows memPtr.

    NOTES
	Must not be called before Purify_Init().

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
    int found = 0;
    struct PNode * node;

    *before = NULL;
    *after  = NULL;

    for (node=(struct PNode *)GetHead(&P_Memory); node; node=(struct PNode *)GetSucc(node))
    {
	if (!*before)
	{
	    if (node->Memory < (UBYTE *)memPtr)
	    {
		found |= 1;
		*before = node;
	    }
	}
	else
	{
	    if (node->Memory < (UBYTE *)memPtr
		&& (*before)->Memory < node->Memory
	    )
	    {
		found |= 1;
		*before = node;
	    }
	}

	if (!*after)
	{
	    if (node->Memory > (UBYTE *)memPtr)
	    {
		found |= 2;
		*after = node;
	    }
	}
	else
	{
	    if (node->Memory > (UBYTE *)memPtr
		&& (*after)->Memory > node->Memory
	    )
	    {
		found |= 2;
		*after = node;
	    }
	}
    }

    if (found == 2)
	found = 1;
    else if (found == 3)
	found = 2;

    return found;
} /* FindNextNodes */
