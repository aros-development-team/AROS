/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Initialize a list
    Lang: english
*/


/*****************************************************************************

    NAME */
#include <exec/lists.h>

	void NewList (

/*  SYNOPSIS */
	struct List * list)

/*  FUNCTION
	Initialize a list. After that, you can use functions like
	AddHead(), AddTail() and Insert() on the list.

    INPUTS
	list - the list to be initialized

    RESULT
	None.

    NOTES
	You can also pass a struct MinList to this function.

    EXAMPLE
	See below.

    BUGS

    SEE ALSO
	NEWLIST() macro, exec.library/AddHead(), exec.library/AddTail(),
	exec.library/Insert(), exec.library/Enqueue(),
	exec.library/Remove(), exec.library/RemHead(), exec.library/RemTail()

    INTERNALS

    HISTORY
	28.11.96 digulla written

******************************************************************************/
{
    NEWLIST(list);
} /* NewList */

#ifdef TEST
#include <stdio.h>

int main (int argc, char ** argv)
{
    struct List list;
    struct Node node;
    struct Usage
    {
	struct Node node;
	int	    data;
    } usage;

    /* Initializing the list */
    NewList (&list);

    /* Adding a node to the list */
    AddHead (&list, &node);

    /*
	But most of the time, you will do something like this: The struct
	Usage contains a node as it's first field. Now you can collect any
	number of struct Usage structures in a list.
    */
    AddTail (&list, (struct Node *)&usage);

    /*
	If you want to avoid the cast, you can of course do this:

	    AddTail (&list, &usage.node);

	but sometimes you won't, because then you can write general
	functions to handle lists with all kinds of nodes in them.
    */

    return 0;
} /* main */
#endif /* TEST */
