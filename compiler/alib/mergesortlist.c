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
        Merge 2 ordered sublists into one. 

    INPUTS
        l       - The first node of the first sublist. The sublists are linked one
                  after the other one, and are both circular lists, that is their
		  first node's Pred pointer points to the last node in the list.
		  
		  The 2nd sublist will thus be at l->mln_Pred->mln_Succ.
		 
        compare - Pointer to the comparison function used to merge the 
                  sublists
		 
        data    - Pointer to user-defined data which will be passed untouched
                  to the comparison function.

    RESULT
        Pointer to the first node of the resulting list. 
        The list will be a circular list, that is its first node's
	Pred pointer will point to the last node in the list.
    
    NOTES
        Should 'l' point to more than 2 sublists, all the other ones
        will be kept linked with the resulting list.  
    
******************************************************************************/
{   
    struct MinNode *l1 = l, *last_l1 = l1->mln_Pred;
    struct MinNode *l2 = last_l1->mln_Succ, *last_l2 = l2->mln_Pred;
    struct MinNode *next_l = last_l2->mln_Succ;
    
    /* This will make the below loop slightly faster, since there will only
       be tests against the NULL constant.  */
    last_l1->mln_Succ = NULL;
    last_l2->mln_Succ = NULL;
    
    struct MinNode *first = NULL, **last_ptr = &first;
       
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
        *last_ptr = l1;
	
	first->mln_Pred   = last_l1;
	last_l1->mln_Succ = next_l;
    }
    else
    if (l2)
    {
	l2->mln_Pred = (struct MinNode *)((char *)last_ptr - 
	               offsetof(struct MinNode, mln_Succ));
        *last_ptr = l2;
	
	first->mln_Pred   = last_l2;
	last_l2->mln_Succ = next_l;
    }
    else
    {
        first->mln_Pred = (struct MinNode *)((char *)last_ptr - 
	                  offsetof(struct MinNode, mln_Succ));
        *last_ptr = next_l;
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

    /* The Merge function requires a list of sublists, each of which
       has to be a circular list. Since the given list doesn't have these 
       properties, we need to divide the sorting algorithm in 2 parts:
        
           1) we first go trough the list once, making every node's Pred pointer
	      point to the node itself, so that the given list of n nodes is
	      transformed in a list of n circular sublists, which we then merge,
	      2 at time.
	      
	   2) Then we go trough the list as many times as needed, until it's
	      completely sorted. 
       
       This is the first part.  */
    l1 = head;
    do
    {
	l2 = l1->mln_Succ;
	
        l1->mln_Pred = l1;
	l2->mln_Pred = l2;
	
	l1 = Merge(l1, compare, data);
	
	*last_ptr = l1;
	last_ptr  = &l1->mln_Pred->mln_Succ;
	l1        = *last_ptr;
    } while (l1 && l1->mln_Succ);
       
    if (l1)
    {
        l1->mln_Pred = l1;
        *last_ptr = l1;
    }

    /* And this is the 2nd part.  */	
    while (first->mln_Pred->mln_Succ)
    {
        l1       = first;
        last_ptr = &first;    
	
	do
	{
	    l1 = Merge(l1, compare, data);
	
	    *last_ptr = l1;
	    last_ptr  = &l1->mln_Pred->mln_Succ;
	    l1        = *last_ptr;
	} while (l1 && l1->mln_Pred->mln_Succ);
    
        if (l1)
            *last_ptr = l1;
    }
    
    /* Now we fix up the list header */
    l->mlh_Head     = first;
    l->mlh_TailPred = first->mln_Pred;
    first->mln_Pred->mln_Succ = (struct MinNode *)&l->mlh_Tail;
    first->mln_Pred = (struct MinNode *)&l->mlh_Head;
}
