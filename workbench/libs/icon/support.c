/*
    Copyright � 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Miscellaneous support functions.
*/

#include "icon_intern.h"

/****************************************************************************************/

/****************************************/
/* Copy the deficon name of type	*/
/* supplied into the mem supplied	*/
/* (GetDefDiskObject & PutDefDiskObject */
/****************************************/

VOID GetDefIconName (LONG def_type, UBYTE * deficonname)
{
    UBYTE * extname = NULL;

    strcpy (deficonname,"ENV:Sys/def_");

    switch(def_type)
    {
    case WBDISK   : extname = "Disk";     break;
    case WBDRAWER : extname = "Drawer";   break;
    case WBTOOL   : extname = "Tool";     break;
    case WBPROJECT: extname = "Project";  break;
    case WBGARBAGE: extname = "Trashcan"; break;
    case WBKICK   : extname = "Kick";     break;
    default: extname = "Unknown"; break; /* Avoid segmentation faults */
    }

    strcat (deficonname, extname);
} /* GetDefIconName */

/****************************************************************************************/

LONG CalcIconHash(struct DiskObject *dobj)
{
    LONG l1, l2, l3, l4, hash;
    
    /* FIXME: Probably sucks. I have no clue about this hash stuff */
    
    l1 = ((LONG)dobj) & 0xFF;
    l2 = (((LONG)dobj) >> 8) & 0xFF;
    l3 = (((LONG)dobj) >> 16) & 0xFF;
    l4 = (((LONG)dobj) >> 24) & 0xFF;
   
    hash = (l1 + l2 + l3 + l4) % ICONLIST_HASHSIZE;

    return hash;
}

/****************************************************************************************/

VOID AddIconToList(struct NativeIcon *icon, struct IconBase *IconBase)
{
    LONG hash;
    
    hash = CalcIconHash(&icon->dobj);
    
    ObtainSemaphore(&IconBase->iconlistlock);
    AddTail((struct List *)&IconBase->iconlists[hash], (struct Node *)&icon->node);
    ReleaseSemaphore(&IconBase->iconlistlock);
}

/****************************************************************************************/

VOID RemoveIconFromList(struct NativeIcon *icon, struct IconBase *IconBase)
{
    ObtainSemaphore(&IconBase->iconlistlock);
    Remove((struct Node *)&icon->node);
    ReleaseSemaphore(&IconBase->iconlistlock);   
}

/****************************************************************************************/

struct NativeIcon *GetNativeIcon(struct DiskObject *dobj, struct IconBase *IconBase)
{
    struct NativeIcon *icon, *reticon = NULL;
    LONG    	       hash;
    
    hash = CalcIconHash(dobj);
    
    ObtainSemaphoreShared(&IconBase->iconlistlock);
    ForeachNode((struct List *)&IconBase->iconlists[hash], icon)
    {
    	if (dobj == &icon->dobj)
	{
	    reticon = icon;
	    break;
	}
    }
    ReleaseSemaphore(&IconBase->iconlistlock);   
    
    return reticon;
}

/****************************************************************************************/
