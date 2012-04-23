/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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
        STRPTR path   = AllocVec(length, MEMF_PUBLIC);
        
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

static const struct ColorRegister ehb_palette[] = 
{
    { 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x7f },
    { 0x00, 0x7f, 0x00 },
    { 0x00, 0x7f, 0x7f },
    { 0x7f, 0x00, 0x00 },
    { 0x7f, 0x00, 0x7f },
    { 0x7f, 0x7f, 0x00 },
    { 0x7f, 0x7f, 0x7f },
    { 0xb0, 0xb0, 0xb0 },
    { 0x00, 0x00, 0xf0 },
    { 0x00, 0xf0, 0x00 },
    { 0x00, 0xf0, 0xf0 },
    { 0xf0, 0x00, 0x00 },
    { 0xf0, 0x00, 0xf0 },
    { 0xf0, 0xf0, 0x00 },
    { 0xf0, 0xf0, 0xf0 },
    { 0x00, 0x00, 0x00 },       /* 'Transparent' */
};

VOID __FetchIconARGB_WB(struct DiskObject *icon, int id, struct IconBase *IconBase)
{
    struct NativeIcon *ni = NATIVEICON(icon);
    struct NativeIconImage *image;

    image = &ni->ni_Image[id];

    if (image->ARGB || ni->ni_Extra.Data == NULL)
        return;

    if (ni->ni_Extra.PNG[id].Offset >= 0) {
        image->ARGB = ReadMemPNG(icon, ni->ni_Extra.Data + ni->ni_Extra.PNG[id].Offset,
                                     &ni->ni_Face.Width, &ni->ni_Face.Height,
                                     NULL, NULL, IconBase);
    }

    /* Prepare ARGB selected imagery, if needed */
    if (image->ARGB == NULL && ni->ni_Image[0].ARGB != NULL) {
        /* Synthesize a selected ARGB icon */
        ULONG area = ni->ni_Face.Width * ni->ni_Face.Height;
        image->ARGB = AllocMemIcon(icon, area * sizeof(UBYTE) * 4, MEMF_PUBLIC);
        if (image->ARGB) {
            CopyMem(ni->ni_Image[0].ARGB, (APTR)image->ARGB, area * sizeof(UBYTE) * 4);
            UBYTE *cp;
            for (cp = (APTR)image->ARGB; area > 0; area--, cp += 4) {
                struct ColorRegister *cr = (APTR)&cp[1];
                ChangeToSelectedIconColor(cr);
            }
        }
    }

    return;
}

/* If we have ARGB imagery, but no Palettized imagery,
 * synthesize some. Use an EHB palette, similar to the
 * VGA-16 color palette.
 *
 * The goal here is to be fast & usable, not slow and accurate.
 */
VOID __FetchIconImage_WB(struct DiskObject *icon, int id, struct IconBase *IconBase)
{
    struct NativeIcon *ni = NATIVEICON(icon);
    struct NativeIconImage *image;
    UBYTE *pixel, *idata;
    ULONG count;

    image = &ni->ni_Image[id];

    if (image->ImageData)
        return;

    FetchIconARGB(icon, id);

    /* If no ARGB, and we're the selected image... */
    if (!image->ARGB) {
        if (id == 1 &&
            image->Palette == NULL &&
            image->ImageData == NULL &&
            ni->ni_Image[0].Palette != NULL &&
            ni->ni_Image[0].ImageData != NULL) {

            ULONG pens = ni->ni_Image[0].Pens;
            struct ColorRegister *newpal;

            newpal = AllocMemIcon(icon, sizeof(struct ColorRegister)*pens, MEMF_PUBLIC);
            if (newpal) {
                ULONG i;

                image->Pens = pens;
                image->TransparentColor = ni->ni_Image[0].TransparentColor;
                image->ImageData = ni->ni_Image[0].ImageData;
                CopyMem(ni->ni_Image[0].Palette, newpal, sizeof(struct ColorRegister)*image->Pens);
                for (i = 0; i < image->Pens; i++) {
                    if (i == image->TransparentColor)
                        continue;
                    ChangeToSelectedIconColor(&newpal[i]);
                }
                image->Palette = newpal;
            }
        }
    } else if (image->ARGB) {
        image->ImageData = AllocMemIcon(icon, ni->ni_Face.Width * ni->ni_Face.Height, MEMF_PUBLIC);
        image->Pens = 17;
        image->Palette = ehb_palette;
        image->TransparentColor = 17;
        pixel = (UBYTE *)image->ARGB;
        idata = (UBYTE *)image->ImageData;
        for (count = 0; count < ni->ni_Face.Width * ni->ni_Face.Height; count++, pixel+=4, idata++) {
            if (pixel[0] == 0) {
                *idata = image->TransparentColor;
            } else {
                UBYTE i,r,g,b;

                r = pixel[1];
                g = pixel[2];
                b = pixel[3];

                /* Determine max intensity */
                i = (r > g) ? r : g;
                i = (i > b) ? i : b;

                /* Rescale by the intensity */
                if (i == 0) {
                    *idata = 0;
                } else {
                    r = (r > (i/4*3)) ? 1 : 0;
                    g = (g > (i/4*3)) ? 1 : 0;
                    b = (b > (i/4*3)) ? 1 : 0;
                    i = (i > 0xd0) ? 1 : 0;

                    *idata = (i << 3) | (r << 2) | (g << 1) | (b << 0);
                }
            }
        }
    }
}


/* Any last-minute changes to the Icon go here before it
 * is returned.
 */
VOID __PrepareIcon_WB(struct DiskObject *icon, struct IconBase *IconBase)
{
    /* Ensure that do_DrawerData exists for WBDISK and WBDRAWER objects */
    if ((icon->do_Type == WBDISK || icon->do_Type == WBDRAWER) &&
        icon->do_DrawerData == NULL) {
        icon->do_DrawerData = AllocMemIcon(icon, sizeof(struct DrawerData), MEMF_PUBLIC | MEMF_CLEAR);
        icon->do_DrawerData->dd_NewWindow.LeftEdge = 50;
        icon->do_DrawerData->dd_NewWindow.TopEdge = 50;
        icon->do_DrawerData->dd_NewWindow.Width = 400;
        icon->do_DrawerData->dd_NewWindow.Height = 150;
    }

    /* Clean out dangling pointers that should no long be
     * present
     */
    if (icon->do_DrawerData) {
        icon->do_DrawerData->dd_NewWindow.FirstGadget = NULL;
        icon->do_DrawerData->dd_NewWindow.Screen = NULL;
        icon->do_DrawerData->dd_NewWindow.BitMap = NULL;
    }

    return;
}



struct DiskObject *__ReadIcon_WB(BPTR file, struct IconBase *IconBase)
{
    struct DiskObject *icon;

    icon = NewDiskObject(0);
    if (icon == NULL)
        return NULL;

    Seek(file, 0, OFFSET_BEGINNING);
    if (ReadStruct(&(LB(IconBase)->dsh), (APTR *) &icon, (APTR)file, IconDesc))
    {
        D(bug("[ReadIcon] Return classic icon %p\n", icon));
        return icon;
    }
    FreeDiskObject(icon);

    if (CyberGfxBase) {
        /* PNG icons don't have any fallback renders, so if we
         * don't have CyberGfxBase, we couldn't draw them on-screen
         * anyway.
         */
        icon = NewDiskObject(0);
        if (icon == NULL)
            return NULL;

        if (ReadIconPNG(icon, file, IconBase))
        {
            D(bug("[ReadIcon] Return PNG icon %p\n", icon));
            return icon;
        }
        
        FreeDiskObject(icon);
    }

    D(bug("[ReadIcon] No icon found\n"));

    return NULL;
}

/* Absolute offsets for the 'only update position' mode */
#define OFFSET_DO_CURRENTX      0x3a
#define OFFSET_DO_CURRENTY      0x3e

#define is_png(ni) (ni && ni->ni_Extra.Data && ni->ni_Extra.PNG[0].Size && ni->ni_Extra.PNG[0].Offset == 0)
#define is_aos(ni) (!is_png(ni))

BOOL __WriteIcon_WB(BPTR file, struct DiskObject *icon, struct TagItem *tags, struct IconBase *IconBase)
{
    struct NativeIcon *ni;
    struct DiskObject *itmp, *oldicon = NULL;
    BOOL success;

    D(bug("[%s] icon=%p\n", __func__, icon));

    ni = GetNativeIcon(icon, IconBase);
   /* Fast position update, for non-PNG icons  */
   if (is_aos(ni) && GetTagData(ICONPUTA_OnlyUpdatePosition, FALSE, tags)) {
        LONG tmp;
        D(bug("[%s] FastUpdate x,y\n", __func__, icon));
        Seek(file, OFFSET_DO_CURRENTX, OFFSET_BEGINNING);
        tmp = AROS_LONG2BE((LONG)icon->do_CurrentX);
        if (Write(file, &tmp, sizeof(tmp)) == sizeof(tmp)) {
            Seek(file, OFFSET_DO_CURRENTY, OFFSET_BEGINNING);
            tmp = AROS_LONG2BE((LONG)icon->do_CurrentY);
            if (Write(file, &tmp, sizeof(tmp)) == sizeof(tmp)) {
                return TRUE;
            }
        }
        return FALSE;
    }

    itmp = DupDiskObject(icon, ICONDUPA_DuplicateImages, TRUE,
                               ICONDUPA_DuplicateImageData, TRUE,
                               TAG_END);

    ni = GetNativeIcon(itmp, IconBase);

    if (GetTagData(ICONPUTA_DropPlanarIconImage, FALSE, tags)) {
        itmp->do_Gadget.Flags &= ~(GFLG_GADGIMAGE | GFLG_GADGHIMAGE);
        itmp->do_Gadget.GadgetRender = NULL;
        itmp->do_Gadget.SelectRender = NULL;
    } else if (is_aos(ni)) {
        if (itmp->do_Gadget.GadgetRender == NULL) {
            SetIoErr(ERROR_OBJECT_WRONG_TYPE);
            return FALSE;
        }
    }

    if (ni && GetTagData(ICONPUTA_DropChunkyIconImage, FALSE, tags)) {
        ni->ni_Image[0].ImageData = NULL;
        ni->ni_Image[1].ImageData = NULL;
    }

    if (itmp->do_ToolTypes && GetTagData(ICONPUTA_DropNewIconToolTypes, FALSE, tags)) {
        int i;
        for (i = 0; itmp->do_ToolTypes[i] != NULL; i++) {
            if (strcmp(itmp->do_ToolTypes[i],"*** DON'T EDIT THE FOLLOWING LINES!! ***") == 0) {
                itmp->do_ToolTypes[i] = NULL;
                break;
            }
        }
    }

    if (is_aos(ni) && GetTagData(ICONPUTA_OptimizeImageSpace, FALSE, tags)) {
        /* TODO: Compress the palette for the icon */
    }

    if (is_aos(ni) && GetTagData(ICONPUTA_PreserveOldIconImages, TRUE, tags)) {
        oldicon = ReadIcon(file);
        Seek(file, 0, OFFSET_BEGINNING);
        if (oldicon) {
            itmp->do_Gadget.GadgetRender = oldicon->do_Gadget.GadgetRender;
            itmp->do_Gadget.SelectRender = oldicon->do_Gadget.SelectRender;
        }
    }

    if (is_png(ni)) 
    {
    	D(bug("[%s] Write as PNG\n", __func__));
    	success = WriteIconPNG(file, itmp, IconBase);
    }
    else
    {
    	D(bug("[%s] Write as AOS 3.5\n", __func__));
    	success = WriteStruct(&(LB(IconBase)->dsh), (APTR) itmp, (APTR) file, IconDesc);
    }

    if (oldicon)
        FreeDiskObject(oldicon);

    FreeDiskObject(itmp);

    return success;
}

BPTR __LockObject_WB(CONST_STRPTR name, LONG mode, struct IconBase *IconBase)
{
    BPTR lock;
   
    if (name == NULL)
        return BNULL;

    lock = Lock(name, mode);
    
    if (lock == BNULL && (strlen(name) >= 5) && strcasecmp(name + strlen(name) - 5, ":Disk") == 0)
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
    
    hash = CalcIconHash(&icon->ni_DiskObject);
    
    ObtainSemaphore(&IconBase->iconlistlock);
    AddTail((struct List *)&IconBase->iconlists[hash], (struct Node *)&icon->ni_Node);
    ReleaseSemaphore(&IconBase->iconlistlock);
}

VOID RemoveIconFromList(struct NativeIcon *icon, struct IconBase *IconBase)
{
    ObtainSemaphore(&IconBase->iconlistlock);
    Remove((struct Node *)&icon->ni_Node);
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
    	if (dobj == &icon->ni_DiskObject)
	{
	    reticon = icon;
	    break;
	}
    }
    ReleaseSemaphore(&IconBase->iconlistlock);   
    
    return reticon;
}



