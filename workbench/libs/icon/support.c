/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Miscellaneous support functions.
*/

#include <proto/dos.h>

#include "icon_intern.h"
#include "support.h"

#define DEBUG 1
#include <aros/debug.h>


BPTR __OpenIcon_WB(CONST_STRPTR name, LONG mode, struct IconBase *IconBase)
{
    BPTR  file       = NULL;
    ULONG nameLength = strlen(name);
    
    if (name[nameLength - 1] == ':')
    {
        BPTR lock = Lock(name, ACCESS_READ);
        
        if (lock != NULL)
        {
            BPTR cd = CurrentDir(lock);
            
            file = Open("Disk.info", mode);
            
            CurrentDir(cd);
            UnLock(lock);
        }
    }
    else
    {
        ULONG  length = nameLength + 5 /* strlen(".info") */ + 1 /* '\0' */;
        STRPTR path   = AllocVec(length, MEMF_ANY);
        
        if(path != NULL)
        {
            strlcpy(path, name, length);
            strlcat(path, ".info", length);
            
            file = Open(path, mode);
            
            FreeVec(path);
        }
        else
        {
            SetIoErr(ERROR_NO_FREE_STORE);
            return NULL;
        }
    }
    
    return file;
}

BOOL __CloseIcon_WB(BPTR file, struct IconBase *IconBase)
{
    return Close(file);
}


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

VOID AddIconToList(struct NativeIcon *icon, struct IconBase *IconBase)
{
    LONG hash;
    
    hash = CalcIconHash(&icon->dobj);
    
    ObtainSemaphore(&IconBase->iconlistlock);
    AddTail((struct List *)&IconBase->iconlists[hash], (struct Node *)&icon->node);
    ReleaseSemaphore(&IconBase->iconlistlock);
}

VOID RemoveIconFromList(struct NativeIcon *icon, struct IconBase *IconBase)
{
    ObtainSemaphore(&IconBase->iconlistlock);
    Remove((struct Node *)&icon->node);
    ReleaseSemaphore(&IconBase->iconlistlock);   
}

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
