/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    AROS specific list class implementation.
*/

#include <exec/types.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include <intuition/classusr.h>
#include <aros/asmcall.h>
#include <string.h>

#include "aroslist_intern.h"
#include <gadgets/aroslist.h>

#define DEBUG 0

#include <aros/debug.h>

#undef AROSListBase
#define AROSListBase ((struct LVBase *)(cl->cl_UserData))


/*****************************************************************************/



#define SETFLAG(flagvar, boolvar, flag) \
    if (boolvar)			\
    	flagvar |= flag;		\
    else				\
    	flagvar &= ~flag;



/******************
**  List::Set()  **
******************/

STATIC VOID SetActive(LONG pos, struct ListData *data)
{
    if (data->ld_Active != AROSV_List_Active_None)
    {
    	data->ld_PointerArray[data->ld_Active]->le_Flags &= ~LEFLG_SELECTED;
    }

    if (pos != AROSV_List_Active_None)
    {
    	data->ld_PointerArray[pos]->le_Flags |= LEFLG_SELECTED;
    }
    data->ld_Active = pos;
    return;
}
STATIC _OM_SET(Class *cl, Object *o,struct opSet *msg)
{
    IPTR retval = (IPTR)0;
    
    const struct TagItem *tag, *tstate;
    struct ListData *data;
    
    data = INST_DATA(cl, o);
    tstate = msg->ops_AttrList;
    
    /* Set to 1 to signal visual changes */
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
    	switch (tag->ti_Tag)
    	{
    	
    	    case AROSA_List_ConstructHook:
    	    	data->ld_ConstructHook = (struct Hook *)tag->ti_Data;
    	    	break;
    	    	
    	    case AROSA_List_DestructHook:
    	    	data->ld_DestructHook  = (struct Hook *)tag->ti_Data;
    	    	break;
    	    
    	    	
    	    case AROSA_List_Active:
    	    	SetActive((LONG)tag->ti_Data, data);
    	    	break;
    	    	
	    default:
	    	break;
	    	
    	} /* switch (tag->ti_Tag) */
    	
    } /* while ((tag = NextTagItem(&tstate)) != NULL) */
    
    ReturnPtr("list_set", IPTR, retval);
}
IPTR AROSList__OM_SET(Class *cl, Object *o,struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, msg);
    retval += _OM_SET(cl, o, msg);
    return retval;
}
IPTR AROSList__OM_UPDATE(Class *cl, Object *o,struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, msg);
    retval += _OM_SET(cl, o, msg);
    return retval;
}

/*****************
** List::New()  **
*****************/
IPTR AROSList__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);

    if (o)
    {
    	struct ListData *data;
    	APTR *srcarray = NULL;
    	const struct TagItem *tag, *tstate;
    	
    	ULONG puddlesz = 2008, threshsz = 1024;
    	
    	D(bug("list, OM_NEW: obj returned from superclass\n"));
    	data = INST_DATA(cl, o);
	memset(data, 0, sizeof (struct ListData));

	/* Initialize the object's internal lists */
	NewLEList(data->ld_EntryTables);
	NewLEList(data->ld_UnusedEntries);
	
	/* Parse for some init only tags */
	tstate = msg->ops_AttrList;
	while ((tag = NextTagItem(&tstate)) != NULL)
	{
	    switch (tag->ti_Tag)
	    {
	    	case AROSA_List_SourceArray:
	    	    srcarray = (APTR *)tag->ti_Data;
	    	    break;
	    	    
	    	case AROSA_List_PoolPuddleSize:
	    	    puddlesz = (ULONG)tag->ti_Data;
	    	    break; 

	    	case AROSA_List_PoolThreshSize:
	    	    threshsz = (ULONG)tag->ti_Data;
	    	    break; 
	    	case AROSA_List_Pool:
	    	    data->ld_Pool = (APTR)tag->ti_Data;
	    	    break;
	    	    
	    } /* switch (tag->ti_Tag) */
	    
	} /* while (more tags in taglist) */
	
	
	/* User supplied source array ? */
	srcarray = (APTR *)GetTagData(AROSA_List_SourceArray, (IPTR) NULL, msg->ops_AttrList);
	
	/* Allocate a bunch of free listentries */
	data->ld_PointerArray = AllocEntries( (srcarray != NULL) 
						? CountItems(srcarray) 
						: NUMENTRIES_TO_ADD,
					    data
	);
					    
 	if (!data->ld_Pool)
 	{
 	    data->ld_Pool = CreatePool(MEMF_ANY, puddlesz, threshsz);
 	}
	
	D(bug("listclass, OM_NEW: pointerarray = %p, pool = %p\n", 
		data->ld_PointerArray, data->ld_Pool));
		
	if (!data->ld_PointerArray || !data->ld_Pool)
	{
	    CoerceMethod(cl, o, OM_DISPOSE);
	    return (IPTR) NULL;
	}
	
	/* As default there is no active entry */
	data->ld_Active = AROSV_List_Active_None;

    	/* Handle our special tags - overrides defaults. This must be done 
    	before the AROSM_List_Insert call, because we must get the ConstructHook and 
    	DestructHook tags */

   	_OM_SET(cl, o, msg);
    	
	if (srcarray)
	{
	    DoMethod(o, AROSM_List_Insert, (IPTR) srcarray, AROSV_List_Insert_Top);
	}

	
	    
    } /* if (object created) */
    return ((IPTR)o);

}

/*********************
** List::Dispose()  **
*********************/

VOID AROSList__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
#warning TODO: Call destructhook too

    struct ListData *data;
    struct ListEntry *entry, *next;
	    
    data = INST_DATA(cl, o);

    D(bug("Inside OM_DISPOSE\n"));
    DoMethod(o, AROSM_List_Clear);
    
    entry = data->ld_EntryTables;
    while (entry)
    {
    	next = entry->le_Next;
    	FreeVec(entry);
    	entry = next;
    }
	    
    if (data->ld_PointerArray)
    	FreeVec(data->ld_PointerArray);
    
    if (data->ld_Pool)
    	DeletePool(data->ld_Pool);
	    	
    DoSuperMethodA(cl, o, msg);
}

/*****************
** List::Get()  **
*****************/

IPTR AROSList__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    IPTR retval = 1UL;
    struct ListData *data;
    
    data = INST_DATA(cl, o);
    
    switch (msg->opg_AttrID)
    {
    	case AROSA_List_Entries:
    	    *(msg->opg_Storage) = (IPTR)data->ld_NumEntries;
    	    break;

    	case AROSA_List_Active:
    	    *(msg->opg_Storage) = (IPTR)data->ld_Active;
    	    break;
    	
    	default:
    	    retval = DoSuperMethodA(cl, o, (Msg)msg);
    	    break;
    }
    return (retval);
}

/********************
** List::Insert()  **
********************/
ULONG AROSList__AROSM_List_Insert(Class *cl, Object *o, struct AROSP_List_Insert *msg)
{
    struct ListData *data;
    
    register ULONG items2insert;
    register ULONG pos;
    
    ULONG numinserted;
    
    ULONG items_below_insertion;

    
    data = INST_DATA(cl, o);

    items2insert = CountItems(msg->ItemArray);
    pos = msg->Position;
    
    /* Special value ? */
    if (pos < 0)
    {
	switch (pos)
	{
	    case AROSV_List_Insert_Top:
	    	pos = 0L;
	    	break;
	    case AROSV_List_Insert_Bottom:
	    	pos = data->ld_NumEntries;
	    	break;
	    	
	    default:
	        pos = data->ld_NumEntries;
	        break;
	        
	} /* switch (pos) */
	
    } /* if (pos < 0) */
    
    else if (pos > data->ld_NumEntries)
    {
   	pos = data->ld_NumEntries;
    }

    items_below_insertion = data->ld_NumEntries - pos;

    /* To little space left ? */
    if (items2insert > data->ld_NumAllocated - data->ld_NumEntries)
    {
    	struct ListEntry **newptrarray;
    	
    	newptrarray = AllocEntries(items2insert, data);
    	if (!newptrarray)
    	    return (0UL);
    	
    	/* Copy all old entries BEFORE the new ones, into the new array */    
    	if (pos != AROSV_List_Insert_Top)
    	{
    	    CopyMem(	data->ld_PointerArray,
    	    		newptrarray, 
    	    		UB(newptrarray[pos]) - UB(newptrarray[0]));
	}    	    		
	
	/* Now insert the entries themselves */
	numinserted = InsertItems(msg->ItemArray, newptrarray, pos, data);
	
	/* Copy all the entries BELOW the inserted value */
	if (pos != data->ld_NumEntries)
	{
	    CopyMem(&(data->ld_PointerArray[pos]),
		&newptrarray[pos + numinserted],
		UB(newptrarray[items_below_insertion] ) - UB(newptrarray[0]));
	}
	/* Free the old buffer */
	FreeVec(data->ld_PointerArray);
	
	data->ld_PointerArray = newptrarray;

    }
    else
    {

    	/* Insert at end of array ? */
    	if (pos == data->ld_NumEntries)
    	{
    	    
    	    numinserted = InsertItems(msg->ItemArray,
    	    			data->ld_PointerArray,
    	    			pos,
    	    			data
    	    );
    	    D(bug("lins: inserted %d entries at end of list\n", numinserted));
    	}
    	else
    	{

    	    /* Insertion in the middle of the array.
    	     We have to move the some items down so we can
    	     to get space for inserting the items */
	    register ULONG i;
	    register struct ListEntry **ptr;
	       
	    ptr = &(data->ld_PointerArray[data->ld_NumEntries - 1]);
	    for (i = items_below_insertion; i; i -- )
	    {
	    	ptr[items2insert] = *ptr;
	    	ptr --;
	    }
	    
	    numinserted = InsertItems(msg->ItemArray, data->ld_PointerArray, pos, data);
	    if (numinserted < items2insert)
	    {
	    	/*
	    	 * Ouch ! Insertion of some (or all) items failed, and we have to move
	    	 * the below items up again
	    	 */
	    
		 ULONG delta = items2insert - numinserted;
	    	    
	    	 ptr = &(data->ld_PointerArray[pos + numinserted /* This is where the items got moved */]);
	    	 
	    	 for (i = items_below_insertion; i; i --)
	    	 {
	    	 
	    	     *ptr = ptr[delta];
	    	     ptr ++;
	    	     
	    	 } /* for (number of items to move) */

	    } /* if (not all entries inserted successfully) */
	    
	} /* if (Insert at end of arry or in middle) */
    
    } /* if (Enough entries left for insertion) */
    


    /* "Refresh" pos of the active entry */
    if (pos <= data->ld_Active && data->ld_Active != AROSV_List_Active_None)
    {
   	SetActive(data->ld_Active + numinserted, data);
    }

    data->ld_NumEntries += numinserted;

        
    return (numinserted);
}

/***************************
**  List::InsertSingle()  **
***************************/
IPTR AROSList__AROSM_List_InsertSingle(Class *cl, Object *o, struct AROSP_List_InsertSingle *msg)
{
    
    APTR ptrarray[2];
    struct AROSP_List_Insert insert_msg;
    
    ptrarray[0] = msg->Item;
    ptrarray[1] = NULL;
    
    insert_msg.MethodID	    = AROSM_List_Insert;
    insert_msg.ItemArray    = ptrarray;
    insert_msg.Position	    = msg->Position;
    
    return (IPTR)AROSList__AROSM_List_Insert(cl, o, &insert_msg);
}

/*********************
**  List::Remove()  **
*********************/
VOID AROSList__AROSM_List_Remove(Class *cl, Object *o, struct AROSP_List_Remove *msg)
{

    
    struct ListData *data;
    LONG pos;
    LONG lastpos;
    struct ListEntry **ptr;
    
    data = INST_DATA(cl, o);
    
    pos = msg->Position;
    lastpos = data->ld_NumEntries - 1;
    
    if (pos < 0) /* Special value ? */
    {
    	switch (pos)
    	{
    	    case AROSV_List_Remove_First:
    	    	pos = 0;
    	    	break;
    	    
    	    case AROSV_List_Remove_Last:
    	    	pos = lastpos;
    	    	break;
    	    	
    	    default:
    	    	/* No meaning */
    	    	return;
    	} /* switch (pos) */
    }
    
    if (pos > lastpos)
    {
    	/* Cannot remove a non-existing entry */
    	return;
    }

    
    /* Call destructhook for entry */
    if (data->ld_DestructHook)
    {
    	CallHookPkt(data->ld_DestructHook,
    		data->ld_Pool,
    		data->ld_PointerArray[pos]->le_Item);
    }		
    /* Add item to freelist */
    AddLEHead(data->ld_UnusedEntries, data->ld_PointerArray[pos]);
    data->ld_NumEntries --;

    /* "Refresh" pos of the active entry */
    if (    pos <= data->ld_Active 
    	&&  data->ld_Active != AROSV_List_Active_None
    	&&  data->ld_Active != 0)
    {
    	SetActive( ((data->ld_NumEntries) 
    			? data->ld_Active - 1 : AROSV_List_Active_None),
    		    data);
    }
   
    /* Skip the specialcase where we remove the last entry */
    if (pos < lastpos)
    {
    	register ULONG i;
    	
    	ptr = &(data->ld_PointerArray[pos]);
    	
    	/* We remove a entry in the middle of the array and must move
    	all lower items up to fill the gap */
    	
    	for (i = lastpos - pos; i; i --)
    	{
    	   *ptr = ptr[1];
    	   ptr ++;
    	}
    }
    return;
}

/********************
**  List::Clear()  **
********************/
VOID AROSList__AROSM_List_Clear(Class *cl, Object *o, Msg msg)
{
    register LONG pos;
    struct ListData *data = INST_DATA(cl, o);
    
D(bug("List::Clear()\n"));
    
    for (pos = 0; pos < data->ld_NumEntries; pos ++)
    {
D(bug("\tClearing entry at pos %d\n", pos));
	
	if (data->ld_DestructHook)
	{
    	    CallHookPkt(data->ld_DestructHook,
    		data->ld_Pool,
    		data->ld_PointerArray[pos]->le_Item);
    	}

D(bug("Addidng to freeentrylist\n"));    		
    	/* Add item to freelist */
    	AddLEHead(data->ld_UnusedEntries, data->ld_PointerArray[pos]);

    }
    data->ld_NumEntries = 0;
    /* "Refresh" pos of the active entry */
    SetActive(AROSV_List_Active_None, data);
    
    return;
}


/*********************
**  List::Select()  **
*********************/
VOID AROSList__AROSM_List_Select(Class *cl, Object *o, Msg msg)
{	
    /* We _could_ put the Select_All stuff together
       with the singlemode but that would slow down singlemode a little */
#undef S
#define S(msg) ((struct AROSP_List_Select *)msg)
    struct ListData *data = INST_DATA(cl, o);

    if (S(msg)->Position != AROSV_List_Select_All)
    {
	struct ListEntry *le; 
	    	
	le = data->ld_PointerArray[S(msg)->Position];
	    	
	switch (S(msg)->SelType)
	{
	case AROSV_List_Select_On:
	    le->le_Flags |= LEFLG_SELECTED;
	    break;
	    	    	
	case AROSV_List_Select_Off:
	    le->le_Flags &= ~LEFLG_SELECTED;
	    break;

	case AROSV_List_Select_Toggle:
	    le->le_Flags ^= LEFLG_SELECTED;
	    break;
	    	    	
	case AROSV_List_Select_Ask:
	    *(S(msg)->State) = ((le->le_Flags & LEFLG_SELECTED) != 0);
	    break;
	}
    }
    else
    {
	register LONG pos;
	register struct ListEntry *le;
	    	
	*(S(msg)->State) = 0;
	
	for (pos = 0; pos < data->ld_NumEntries; pos ++)
	{
	    le = data->ld_PointerArray[S(msg)->Position];
	    switch (S(msg)->SelType)
	    {
	    case AROSV_List_Select_On:
		le->le_Flags |= LEFLG_SELECTED;
		break;
	    	    	
	    case AROSV_List_Select_Off:
		le->le_Flags &= ~LEFLG_SELECTED;
		break;

	    case AROSV_List_Select_Toggle:
		le->le_Flags ^= LEFLG_SELECTED;
		break;
	    	    	
	    case AROSV_List_Select_Ask:
		if (le->le_Flags & LEFLG_SELECTED)
		{
		    *(S(msg)->State) += 1;
		}
		break;
	    }	    	
	}
    }
}


/***************************
**  List::NextSelected()  **
***************************/
VOID AROSList__AROSM_List_NextSelected(Class *cl, Object *o, Msg msg)
{
#undef NS
#define NS(msg) ((struct AROSP_List_NextSelected *)msg)
	    
    struct ListData *data = INST_DATA(cl, o);
    register LONG pos;
	    
    pos = *(NS(msg)->Position);
	    
    if (pos == AROSV_List_NextSelected_Start)
	pos = 0;
	    	
    for (; pos < data->ld_NumEntries; pos ++)
    {
	if (data->ld_PointerArray[pos]->le_Flags & LEFLG_SELECTED)
	{
	    *(NS(msg)->Position) = pos;
	    return;
	}
    }
	    
    *(NS(msg)->Position) = AROSV_List_NextSelected_End;
}


/***********************
**  List::GetEntry()  **
***********************/
VOID AROSList__AROSM_List_GetEntry(Class *cl, Object *o, Msg msg)
{
#undef GE
#define GE(msg) ((struct AROSP_List_GetEntry *)msg)
	    
    struct ListData *data;

    data = INST_DATA(cl, o);
	    
    if (GE(msg)->Position >= data->ld_NumEntries || GE(msg)->Position < 0)
	*(GE(msg)->ItemPtr) = NULL;
    else
	*(GE(msg)->ItemPtr) = data->ld_PointerArray[GE(msg)->Position]->le_Item;
}

