/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Help functions for aroslistclass.
    Lang: english
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include "aroslist_intern.h"

#ifndef TURN_OFF_DEBUG
#define DEBUG 1
#endif

#include <aros/debug.h>

/**************/
/* CountItems */
/**************/

ULONG CountItems(APTR *array)
{
/*    D(bug("aroslist:CountItems(array=%p)\n", array)); */
    
    register ULONG i;
    for (i = 0; *array ++; i ++) ;
    
/*    ReturnInt("aroslist:CountItems", ULONG, i); */
    return (i);
}

/****************/
/* AllocEntries */
/****************/

struct ListEntry **AllocEntries(ULONG			numnewentries,
				struct ListData		*data
)
{
    struct ListEntry *nodes = NULL;

    D(bug("aroslist:AllocEntries(numnewentries=%d, data=%p)\n", numnewentries, data));
    
    /* Allocate memory for ListEntrys.
     We need the first node for keeping the list of these node-arrays for
    later deallocation */
    
    numnewentries ++;
    
    nodes = AllocVec(UB(&nodes[numnewentries]) - UB(&nodes[0]), MEMF_ANY);
    if (nodes)
    {
	struct ListEntry **pointerarray = NULL;
	
	/* Allocate memory for expanded pointerarray */
    	numnewentries --;
    	pointerarray = AllocVec(UB(&pointerarray[numnewentries + data->ld_NumAllocated]) 
    						- UB(&pointerarray[0]),
    	    			MEMF_ANY);
    	if (pointerarray)
    	{
	    /* Add the nodearray to the internal list of entry arrays */
    	    AddLEHead(data->ld_EntryTables, nodes);
    	
    	    data->ld_NumAllocated += numnewentries;
    	
	    /* Skip the dummy node */
	    nodes ++;
	    for (; numnewentries --;)
    	    {
    	        AddLEHead(data->ld_UnusedEntries, nodes);
    	        nodes ++;
    	    }
    	    
    	    ReturnPtr ("aroslist:AllocItems", struct ListEntry **, pointerarray);
    	} /* if (pointerarray) */
    	FreeVec(nodes);
    	
    } /* if (nodes) */
    ReturnPtr ("aroslist:AllocItems", struct ListEntry **, NULL);
}

/***************/
/* InsertItems */
/***************/

ULONG InsertItems(APTR 		    *itemarray,
		struct ListEntry    **pointerarray,
		LONG		    pos,
		struct ListData     *data
)
{
    register struct ListEntry **leptr;
    register struct ListEntry *le;
    register ULONG numinserted = 0;
    
    		
    leptr = &pointerarray[pos];
    

    
    while(*itemarray)
    {

    	le = data->ld_UnusedEntries;
	
	if (data->ld_ConstructHook)
	{
	    le->le_Item = (APTR)CallHookPkt(data->ld_ConstructHook, 
					data->ld_Pool, 
					*itemarray);
	}
	else
	{
	    le->le_Item = *itemarray;
	} 
	itemarray ++;
						
	if (le->le_Item)
	{
	    /* If constructhook succeeded, remove entry 
	     * from list of unused entries.
	     */
    	    RemLEHead(data->ld_UnusedEntries);
    	    le->le_Flags = 0;
    	
    	    /* Point to new ListEntry in the pointerarray */
    	    *leptr ++ = le;
    	    
    	    numinserted ++;
        }
    }

    return (numinserted);
}
