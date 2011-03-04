/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Miscellaneous support functions.
*/

#include <string.h>
#include <stdio.h>

#include "icon_intern.h"
#include "support.h"

#include <aros/debug.h>

extern const IPTR IconDesc[];

BPTR __OpenIcon_WB(CONST_STRPTR name, LONG mode, struct IconBase *IconBase)
{
    BPTR  file       = BNULL;
    ULONG nameLength = strlen(name);
       
    if (name[nameLength - 1] == ':')
    {
        BPTR lock = Lock(name, ACCESS_READ);
        
        if (lock != BNULL)
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
            return BNULL;
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
    BPTR                       file         = BNULL;
    UBYTE                      i;
    
    /* Make sure it's a plain filename; paths are not allowed */
    if (strpbrk(name, "/:") != NULL) return BNULL;
    
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
            if ((file = Open(buffer, mode)) != BNULL)
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
            
    if (ReadStruct(&(LB(IconBase)->dsh), (APTR *) &temp, (APTR)file, IconDesc))
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
    	if (!PNGBase) {
	    PNGBase = OpenLibrary("SYS:Classes/datatypes/png.datatype", 0);
	    D(bug("[ReadIcon] PNGBase is 0x%p\n", PNGBase));
	}
	
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
	    
	    D(bug("[ReadIcon] Freeing PNG icon\n"));
	    FreeIconPNG(temp, IconBase);
	}
	
    }
    
    D(bug("[ReadIcon] Returning 0x%p\n", icon));
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
    	return WriteStruct(&(LB(IconBase)->dsh), (APTR) icon, (APTR) file, IconDesc);
    }
}

BPTR __LockObject_WB(CONST_STRPTR name, LONG mode, struct IconBase *IconBase)
{
    BPTR lock = Lock(name, mode);
    
    if (lock == BNULL && strcasecmp(name + strlen(name) - 5, ":Disk") == 0)
    {
        // FIXME: perhaps allocate buffer from heap?
        TEXT  buffer[256];               /* Path buffer */
        ULONG length = strlen(name) - 3; /* Amount to copy + NULL */
        
        if (sizeof(buffer) >= length)
        {
            strlcpy(buffer, name, length);
            lock = Lock(buffer, mode);
        }
    }
    
    return lock;
}

VOID __UnLockObject_WB(BPTR lock, struct IconBase *IconBase)
{
    if (lock != BNULL) UnLock(lock);
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

/* Use the FNV-1 hash function over the object's pointer.
 * http://en.wikipedia.org/wiki/Fowler-Noll-Vo_hash_function
 */
LONG CalcIconHash(struct DiskObject *dobj)
{
    const ULONG FNV1_32_Offset = 2166136261UL;
    const ULONG FNV1_32_Prime  = 16777619UL;
    IPTR data = (IPTR)dobj;
    ULONG hash;
    int i;

    hash = FNV1_32_Offset;
    for (i = 0; i < AROS_SIZEOFPTR; i++) {
    	    hash *= FNV1_32_Prime;
    	    hash ^= data & 0xff;
    	    data >>= 8;
    }
    	
    return hash & (ICONLIST_HASHSIZE-1);
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
