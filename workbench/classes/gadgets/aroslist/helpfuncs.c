/*
    (C) 1995-97 AROS - The Amiga Replacement OS
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
				struct ListData		*data,
				struct ListBase_intern	*AROSListBase)
{
    struct ListEntry *nodes;

    D(bug("aroslist:AllocEntries(numnewentries=%d, data=%p)\n", numnewentries, data));
    
    /* Allocate memory for ListEntrys.
     We need the first node for keeping the list of these node-arrays for
    later deallocation */
    
    numnewentries ++;
    
    nodes = AllocVec(UB(&nodes[numnewentries]) - UB(&nodes[0]), MEMF_ANY);
    if (nodes)
    {
	struct ListEntry **pointerarray;
	
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
		struct ListData     *data,
		struct ListBase_intern *AROSListBase)
{
    register struct ListEntry **leptr;
    register struct ListEntry *le;
    register ULONG numinserted = 0;
    
    D(bug("aroslist:InsertItems(itemarray=%p, pointerarray = %p, pos=%d, data=%p)\n",
    		itemarray, pointerarray, pos, data));
    		
    leptr = &pointerarray[pos];
    

    
    while(*itemarray)
    {

    	le = data->ld_UnusedEntries;

	le->le_Item = (APTR)CallHookPkt(data->ld_ConstructHook, 
					data->ld_Pool, 
					*itemarray ++);
	if (le->le_Item)
	{
	    /* If constructhook succeeded, remove entry 
	     * from list of unused entries.
	     */

	    D(bug("InsertItems: Got item %p\n",le->le_Item));
	    
    	    RemLEHead(data->ld_UnusedEntries);
    	    le->le_Flags = 0;
    	
    	    D(bug("leptr: %p, listentry: %p\n", leptr, le));
    	    /* Point to new ListEntry in the pointerarray */
    	    *leptr ++ = le;
    	    
    	    numinserted ++;
        }
    }

    return (numinserted);
}
