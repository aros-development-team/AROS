/*
     Copyright © 1995-2001, The AROS Development Team. All rights reserved.
     $Id$

     Desc: Sort a list using a variant of the MergeSort algorithm.
     Lang: english
*/

#include <exec/lists.h>
#include <stddef.h>

/*****************************************************************************

    NAME */
        __attribute__((regparm(2)))
	static inline struct MinNode *Merge(

/*  SYNOPSIS */
        struct MinNode *l,
        int (*compare)(struct MinNode *n1, struct MinNode *n2, void *data),
	void *data)

/*  FUNCTION
        Given a list of ordered circular sublists, merge pairs of ordered sublists into one
	ordered circular sublist. 

    INPUTS
        l       - The first node of the first sublist. The sublists must be linked one
                  after the other one, and must be circular lists, that is their
		  first node's Pred pointer must point to their last node.
		  
		  I.e., the 2nd sublist will be at l->mln_Pred->mln_Succ, the 3rd will be at
		  l->mln_Pred->mln_Succ->mln_Pred->mln_Succ and so on.
		 
        compare - Pointer to the comparison function used to merge the 
                  sublists
		 
        data    - Pointer to user-defined data which will be passed untouched
                  to the comparison function.

    RESULT
        Pointer to the first node of the resulting list of sublists, with the same
	format of the input list, but with pairs of sublists merged into one.
    
******************************************************************************/
{   
    struct MinNode *l1, *last_l1, *l2, *last_l2, *next_l;
    struct MinNode *first = NULL, **first_ptr, **last_ptr = &first;
    
    l1 = l;
    
    /* l1 points to the 1st sublist, l2 points to the 2nd.
    
       Should there be no l2, we don't need to do anything special, as
       l1 will already be linked with the rest of the list AND it won't 
       obviously need to be merged with another list.  */
    while (l1 && (l2 = (last_l1 = l1->mln_Pred)->mln_Succ))
    {
        last_l2 = l2->mln_Pred;
        
	next_l  = last_l2->mln_Succ;	   
    
        /* This will make the below loop slightly faster, since there will only
           be tests against the constant NULL.  */
        last_l1->mln_Succ = NULL;
        last_l2->mln_Succ = NULL;
       
	/* Pointer to the beginning of the merged sublist */
	first_ptr = last_ptr;
        do
        {
            if (compare(l1, l2, data) < 0)
            {
	        l1->mln_Pred = (struct MinNode *)((char *)last_ptr - 
	                       offsetof(struct MinNode, mln_Succ));
                *last_ptr    = l1;
	        l1           = l1->mln_Succ;
            }
            else
            {
	        l2->mln_Pred = (struct MinNode *)((char *)last_ptr - 
	                       offsetof(struct MinNode, mln_Succ));
	        *last_ptr    = l2;
	        l2           = l2->mln_Succ;
            }
        
	    last_ptr = &(*last_ptr)->mln_Succ;
        } while (l1 && l2);

        if (l1)
        {
	    l1->mln_Pred = (struct MinNode *)((char *)last_ptr - 
	                   offsetof(struct MinNode, mln_Succ));
            
	    *last_ptr              = l1;
  	    (*first_ptr)->mln_Pred = last_l1;
	    last_ptr               = &last_l1->mln_Succ;
        }
        else
        if (l2)
        {
	    l2->mln_Pred = (struct MinNode *)((char *)last_ptr - 
	                   offsetof(struct MinNode, mln_Succ));
            
	    *last_ptr              = l2;
	    (*first_ptr)->mln_Pred = last_l2;
	    last_ptr               = &last_l2->mln_Succ;
        }
        else
        {
            (*first_ptr)->mln_Pred = (struct MinNode *)((char *)last_ptr - 
	                             offsetof(struct MinNode, mln_Succ));
        }
        
	l1 = *last_ptr = next_l;
    }
    
    return first;
}

/*****************************************************************************

    NAME */
#include <clib/alib_protos.h>       
        void MergeSortList(

/*  SYNOPSIS */
	struct MinList *l,
	int (*compare)(struct MinNode *n1, struct MinNode *n2, void *data),
	void *data)

/*  FUNCTION
        Sorts an Exec-style doubly linked list, by using a variant of the merge sorting
	algorithm, which is Theta(n log n ). No additional space is required other than
	the one needed for local variables in the function itself. The function is not
	recursive.

    INPUTS
       l       - The list to sort.
		 
       compare - Pointer to the comparison function which establishes the order
                 of the elements in the list
		 
       data    - Pointer to user-defined data which will be passed untouched
                 to the comparison function.

    RESULT
        The given list, sorted in place.
    
******************************************************************************/
{
    struct MinNode *head = (struct MinNode *)GetHead(l);
    struct MinNode *tail = (struct MinNode *)GetTail(l);
    
    struct MinNode *l1, *l2,
                   *first, **last_ptr;
    
    if (!head || head == tail)
        return;
	
    tail->mln_Succ = NULL;
    last_ptr = &first;

    /* The Merge() function requires a list of sublists, each of which
       has to be a circular list. Since the given list doesn't have these 
       properties, we need to divide the sorting algorithm in 2 parts:
        
           1) we first go trough the list once, making every node's Pred pointer
	      point to the node itself, so that the given list of n nodes is
	      transformed in a list of n circular sublists. Here we do the merging
	      "manually", without the help of the Merge() function, as we have to
	      deal with just couples of nodes, thus we can do some extra optimization.
	      
	   2) We then feed the resulting list to the Merge() function, as many times as
	      it takes to the Merge() function to give back just one circular list, rather
	      than a list of circular sublists: that will be our sorted list. */
       
    /* This is the first part.  */
    l1 = head;
    l2 = l1->mln_Succ;
    do
    {
        /* It can happen that the 2 nodes are already in the right,
           order and thus we only need to make a circular list out
	   of them, or their order needs to be reversed, but
	   in either case, the below line is necessary, because:
	   
	       1) In the first case, it serves to build the 
	          circular list.
		  
	       2) In the 2nd case, it does 1/4 of the job of
	          reversing the order of the nodes (the 
		  other 3/4 are done inside the if block).  */
        l1->mln_Pred = l2;
	
	if (compare(l1, l2, data) >= 0)
	{
	    /* l2 comes before l1, so rearrange them and
	       make a circular list out of them.  */
	    l1->mln_Succ = l2->mln_Succ;
	    l2->mln_Succ = l1;
	    l2->mln_Pred = l1;
	    
	    l1 = l2;
	}
	
	*last_ptr = l1;
	last_ptr  = &l1->mln_Pred->mln_Succ;
	l1        = *last_ptr;
    } while (l1 && (l2 = l1->mln_Succ));
       
    /* An orphan node? Add it at the end of the list of sublists and 
       make a circular list out of it.  */
    if (l1)
    {
        l1->mln_Pred = l1;
        *last_ptr = l1;
    }

    /* And this is the 2nd part.  */	
    while (first->mln_Pred->mln_Succ)
        first = Merge(first, compare, data);
    
    /* Now we fix up the list header */
    l->mlh_Head     = first;
    l->mlh_TailPred = first->mln_Pred;
    first->mln_Pred->mln_Succ = (struct MinNode *)&l->mlh_Tail;
    first->mln_Pred = (struct MinNode *)&l->mlh_Head;
}
