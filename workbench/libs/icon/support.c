/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Miscellaneous support functions.
*/

#include <string.h>
#include <stdio.h>

#include "icon_intern.h"
#include "support.h"

#define DEBUG 1
#   include <aros/debug.h>

extern const IPTR IconDesc[];

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
        STRPTR path   = AllocVecPooled(POOL, length);
        
        if(path != NULL)
        {
            strlcpy(path, name, length);
            strlcat(path, ".info", length);
            
            file = Open(path, mode);
            
            FreeVecPooled(POOL, path);
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

BPTR __OpenDefaultIcon_WB(CONST_STRPTR name, LONG mode, struct IconBase *IconBase)
{
    static const char * const  readPaths[]  = { "ENV:SYS", "ENVARC:SYS", NULL };
    static const char * const  writePaths[] = { "ENV:SYS", NULL }; 
    const char * const        *paths        = NULL;
    CONST_STRPTR               path         = NULL;
    BPTR                       file         = NULL;
    UBYTE                      i;
    
    /* Make sure it's a plain filename; paths are not allowed */
    if (strpbrk(name, "/:") != NULL) return NULL;
    
    if (mode == MODE_OLDFILE) paths = readPaths;
    else                      paths = writePaths;
    
    /* Attempt to open the icon from each path in turn */
    for (i = 0, path = paths[0]; path != NULL; i++, path = paths[i])
    {
        TEXT  buffer[256]; /* Filename buffer; should be more than large enough */
        ULONG copied;      /* Number of bytes copied */
        
        copied = snprintf(buffer, sizeof(buffer), "%s/def_%s.info", path, name);
        
        if (copied < sizeof(buffer)) /* check for truncation */
        {
            if ((file = Open(buffer, mode)) != NULL)
            {
                break;
            }
        }
    }
    
    return file;
}

BOOL __CloseDefaultIcon_WB(BPTR file, struct IconBase *IconBase)
{
    return Close(file);
}

struct DiskObject *__ReadIcon_WB(BPTR file, struct IconBase *IconBase)
{
    struct DiskObject *temp = NULL, /* Temporary icon data */
                      *icon = NULL; /* Final icon data */
            
    if (ReadStruct(&(LB(IconBase)->dsh), (APTR *) &temp, file, IconDesc))
    {
        // FIXME: consistency checks! (ie that WBDISK IS for a disk, WBDRAWER for a dir, WBTOOL for an executable)
        /*
            Duplicate the disk object so it can be freed with 
            FreeDiskObject(). 
        */
        // FIXME: is there any way to avoid this?
        icon = DupDiskObject
        (
            temp,
            ICONDUPA_JustLoadedFromDisk, TRUE,
            TAG_DONE
        );
    }
    
    // FIXME: Read/FreeStruct seem a bit broken in memory handling
    // FIXME: shouldn't ReadStruct deallocate memory if it fails?!?!
    if (temp != NULL) FreeStruct(temp, IconDesc);
    
    if (!icon)
    {
    	if (!PNGBase) PNGBase = OpenLibrary("datatypes/png.datatype", 0);
	
    	if (PNGBase && ReadIconPNG(&temp, file, IconBase))
	{
            /*
        	Duplicate the disk object so it can be freed with 
        	FreeDiskObject(). 
            */
            // FIXME: is there any way to avoid this?
            icon = DupDiskObject
            (
        	temp,
        	ICONDUPA_JustLoadedFromDisk, TRUE,
        	TAG_DONE
            );
	    
	    FreeIconPNG(temp, IconBase);
	}
	
    }
    
    return icon;
}

BOOL __WriteIcon_WB(BPTR file, struct DiskObject *icon, struct IconBase *IconBase)
{
    struct NativeIcon *nativeicon = GetNativeIcon(icon, IconBase);
    
    if (nativeicon && nativeicon->iconPNG.handle)
    {
    	return WriteIconPNG(file, icon, IconBase);
    }
    else
    {
    	return WriteStruct(&(LB(IconBase)->dsh), (APTR) icon, file, IconDesc);
    }
}

CONST_STRPTR GetDefaultIconName(LONG type)
{
    static const char * const defaultNames[] =
    {
        "Disk",         /* WBDISK    (1) */ 
        "Drawer",       /* WBDRAWER  (2) */
        "Tool",         /* WBTOOL    (3) */
        "Project",      /* WBPROJECT (4) */
        "Trashcan",     /* WBGARBAGE (5) */
        "Device",       /* WBDEVICE  (6) */
        "Kick",         /* WBKICK    (7) */
        "AppIcon"       /* WBAPPICON (8) */
    };
    
    if (type >= 1 && type <= 8)
    {
        return defaultNames[type - 1];
    }
    else
    {
        return NULL;
    }
}


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
    LONG i = 0;
    
    hash = CalcIconHash(dobj);
    
    ObtainSemaphoreShared(&IconBase->iconlistlock);
    ForeachNode((struct List *)&IconBase->iconlists[hash], icon)
    {
        i++;
    	if (dobj == &icon->dobj)
	{
	    reticon = icon;
	    break;
	}
    }
    ReleaseSemaphore(&IconBase->iconlistlock);   
    
    return reticon;
}
