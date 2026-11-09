/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

/****************************************************************************************/

#define DTT(x)

#include <datatypes/datatypes.h>
#include <intuition/intuition.h>

#include <proto/dos.h>
#include <proto/alib.h>
#include <proto/pngdt.h>
#include <proto/datatypes.h>

#include <aros/bigendianio.h>
#include <aros/asmcall.h>

#include "icon_intern.h"

#include <aros/debug.h>

#include <png.h>

#define ATTR_ICONX          0x80001001
#define ATTR_ICONY          0x80001002
#define ATTR_DRAWERX        0x80001003
#define ATTR_DRAWERY        0x80001004
#define ATTR_DRAWERWIDTH    0x80001005
#define ATTR_DRAWERHEIGHT   0x80001006
#define ATTR_DRAWERFLAGS    0x80001007
#define ATTR_TOOLWINDOW     0x80001008  //OS4: STRPTR, tool window string, length including the tag
                                        //must be a multiple of 8
#define ATTR_STACKSIZE      0x80001009
#define ATTR_DEFAULTTOOL    0x8000100a
#define ATTR_TOOLTYPE       0x8000100b
#define ATTR_VIEWMODES      0x8000100c  //OS4 PNG use that
#define ATTR_DD_CURRENTX    0x8000100d  //OS4 ULONG, drawer view X offset
#define ATTR_DD_CURRENTY    0x8000100e  //OS4 ULONG, drawer view Y offset
#define ATTR_TYPE           0x8000100f  //OS4 icon type (WBDISK...WBKICK)
#define ATTR_FRAMELESS      0x80001010  //OS4 ULONG, frameless property
#define ATTR_DRAWERFLAGS3   0x80001011  //OS4 ULONG, drawer flags
#define ATTR_VIEWMODES2     0x80001012  //OS4 ULONG, drawer view modes
#define ATTR_DRAWERFLAGS2   0x80001107  //written from AFA to store needed dopus Magellan settings

#define EFFECT_NONE      (0)
#define EFFECT_LIGHTEN   (1)
#define EFFECT_TINT_BLUE (2)
#define EFFECT_XOR       (3)

#define EFFECT EFFECT_LIGHTEN

/****************************************************************************************/

/*
     ATTR_DRAWERFLAGS: AAABC
     
     C  : 1-bit flag  : 0 = showonlyicons 1 = showallfiles
     B  : 1-bit flag  : 0 = viewastext    1 = view as icons
     AA : 2-bit value : 0 = viewbyname, 1 = viewbydata, 2 = viewbysize, 3 = viewbytype
*/

static ULONG flags_to_ddflags(ULONG flags)
{
    ULONG ret = 0;
    
    if (flags & 1)
    {
        ret = DDFLAGS_SHOWALL;
    }
    else
    {
        ret = DDFLAGS_SHOWICONS;
    }

    return ret;
}

static ULONG flags_to_ddviewmodes(ULONG flags)
{
    ULONG ret = 0;
    
    if (flags & 2)
    {
        ret = DDVM_BYICON;
    }
    else
    {
        ret = (flags >> 2) + DDVM_BYNAME;
    }
    
    return ret;
}

static ULONG dd_to_flags(struct DiskObject *dobj)
{
    ULONG drawerflags = 0;
    
    if (dobj->do_DrawerData->dd_Flags & DDFLAGS_SHOWALL)
    {
        drawerflags |= 1;
    }

    if (dobj->do_DrawerData->dd_ViewModes == DDVM_BYICON)
    {
        drawerflags |= 2;
    }
    else
    {
        drawerflags |= ((dobj->do_DrawerData->dd_ViewModes - 2) << 2);
    }
        
    return drawerflags;
}

/* Returns an ARGB image.
 * Set &width == -1 and &height == -1 to get the size.
 * Otherwise, sets the image size of width & height
 */
ULONG *ReadMemPNG(struct DiskObject *icon, APTR stream, LONG *width, LONG *height, const CONST_STRPTR *chunknames, APTR *chunkpointer, struct IconBase *IconBase)
{
    APTR PNGBase = OpenLibrary("SYS:Classes/datatypes/png.datatype", 41);
    APTR handle;
    ULONG *argb = NULL;

    if (!PNGBase) {
        D(bug("[%s] Can't open png.datatype\n", __func__));
        return NULL;
    }

    handle = PNG_LoadImageMEM(stream, -1, chunknames, chunkpointer, TRUE);
    if (handle) {
        ULONG *src, *dst;
        LONG w = 0, h = 0;
        LONG x,y, xoff, yoff;

        PNG_GetImageInfo(handle, &w, &h, NULL, NULL);
        D(bug("[%s] Dest (%d x %d), Image (%d x %d)\n", __func__,
                    *width, *height, w, h));
        if (*width == -1 && *height == -1) {
            *width = w;
            *height = h;
        }

        PNG_GetImageData(handle, (APTR *)&src, NULL);

        argb = AllocMemIcon(icon, (*width) * (*height) * sizeof(ULONG),
            MEMF_PUBLIC, IconBase);
        if (argb) {
            xoff = ((*width) - w)/2;
            yoff = ((*height) - h)/2;

            dst = argb;
            for (y = 0; y < *height; y++) {
                LONG sy = y + yoff;

                if (sy < 0)
                    sy = 0;

                if (sy >= h)
                    sy = h-1;

                for (x = 0; x < *width; x++, dst++) {
                    LONG sx = x + xoff;

                    if (sx < 0)
                        sx = 0;

                    if (sx >= w)
                        sx = w-1;

                    *dst = src[sy*w+sx];
                }
            }
        }

        PNG_FreeImage(handle);
    } else {
        D(bug("[%s] PNG datatype can't parse data\n", __func__));
    }

    CloseLibrary(PNGBase);
    return argb;
}


/****************************************************************************************/

/* A big endian 32 bit value from a byte stream, at any alignment */
static ULONG GetBE32(const UBYTE *p)
{
    return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

static void GetChunkInfo(APTR stream, APTR *chunkdata, ULONG *chunksize)
{
    png_unknown_chunkp chunkp = stream;
    *chunksize = chunkp->size;
    *chunkdata = chunkp->data;
}

/****************************************************************************************/

STATIC BOOL MakePlanarImage(struct NativeIcon *icon, struct Image **img, UBYTE *src, struct IconBase *IconBase)
{
    LONG width16 = (icon->ni_DiskObject.do_Gadget.Width + 15) & ~15;
    LONG bpr = width16 / 8;
    LONG planesize = bpr * icon->ni_DiskObject.do_Gadget.Height;
    LONG x, y;
    UWORD *p1, *p2;
    ULONG *s = (ULONG *) src;

    *img = (struct Image *) AllocMemIcon(&icon->ni_DiskObject,
        sizeof(struct Image) + planesize * 2, MEMF_PUBLIC | MEMF_CLEAR,
        IconBase);
    if (*img == NULL)
        return FALSE;

    (*img)->Width = icon->ni_DiskObject.do_Gadget.Width;
    (*img)->Height = icon->ni_DiskObject.do_Gadget.Height;
    (*img)->Depth = 2;
    (*img)->ImageData = (UWORD *) (*img + 1);
    (*img)->PlanePick = 3;

    p1 = (UWORD *) (*img)->ImageData;
    p2 = p1 + planesize / 2;

    for (y = 0; y < (*img)->Height; y++)
    {
        ULONG pixelmask = 0x8000;
        UWORD plane1dat = 0;
        UWORD plane2dat = 0;

        for (x = 0; x < (*img)->Width; x++)
        {
            ULONG pixel = *s++;

#if AROS_BIG_ENDIAN
            if ((pixel & 0xFF000000) > 0x80000000)
            {
                pixel = (((pixel & 0x00FF0000) >> 16) +
                        ((pixel & 0x0000FF00) >> 8) +
                        ((pixel & 0x000000FF)) +
                        127) / 256;
#else
            if ((pixel & 0x000000FF) > 0x80)
            {
                pixel = (((pixel & 0x0000FF00) >> 8) + ((pixel & 0x00FF0000) >> 16) + ((pixel & 0xFF000000) >> 24) + 127) / 256;
#endif

                if (pixel == 3)
                {
                    /* Col 2: White */
                    plane2dat |= pixelmask;
                }
                else if ((pixel == 2) || (pixel == 1))
                {
                    /* Col 3: Amiga Blue */
                    plane1dat |= pixelmask;
                    plane2dat |= pixelmask;
                }
                else
                {
                    /* Col 1: Black */
                    plane1dat |= pixelmask;
                }
            }

            pixelmask >>= 1;
            if (!pixelmask)
            {
                pixelmask = 0x8000;
                *p1++ = AROS_WORD2BE(plane1dat);
                *p2++ = AROS_WORD2BE(plane2dat);

                plane1dat = plane2dat = 0;

            }

        }

        if (pixelmask != 0x8000)
        {
            *p1++ = AROS_WORD2BE(plane1dat);
            *p2++ = AROS_WORD2BE(plane2dat);

        }

    } /* for(y = 0; y < icon->ni_Height; y++) */

    return TRUE;

}

/****************************************************************************************/

STATIC BOOL MakePlanarImages(struct NativeIcon *icon, struct IconBase *IconBase)
{
    if (!MakePlanarImage(icon, (struct Image **) &icon->ni_DiskObject.do_Gadget.GadgetRender,
            (UBYTE *)icon->ni_Image[0].ARGB, IconBase))
    {
        return FALSE;
    }

    icon->ni_DiskObject.do_Gadget.Flags |= GFLG_GADGIMAGE;

    if (!icon->ni_Image[1].ARGB)
        return TRUE;

    if (MakePlanarImage(icon, (struct Image **) &icon->ni_DiskObject.do_Gadget.SelectRender,
            (UBYTE *)icon->ni_Image[1].ARGB, IconBase))
    {
        icon->ni_DiskObject.do_Gadget.Flags &= ~GFLG_GADGHIGHBITS;
        icon->ni_DiskObject.do_Gadget.Flags |= GFLG_GADGHIMAGE;
    }

    return TRUE;
}

/****************************************************************************************/

BOOL ReadIconPNG(struct DiskObject *dobj, BPTR file, struct IconBase *IconBase)
{
    static CONST_STRPTR const chunknames[] =
    {
        "icOn",
        NULL
    };
    APTR chunkpointer[] =
    {
        NULL,
        NULL
    };
    
    struct NativeIcon  *icon;
    ULONG               filesize;
    APTR                data;

    icon = NATIVEICON(dobj);

    D(bug("%s: File stream %p\n", __func__, file));

    if (Seek(file, 0, OFFSET_END) < 0) return FALSE;
    if ((filesize = Seek(file, 0, OFFSET_BEGINNING)) < 0) return FALSE;

    D(bug("[%s] Inspecting a %d byte file\n", __func__, filesize));

    /* Need a copy of whole file in memory for icon saving :-\ Because
       that should save file back as it was, only with modified or new
       icOn chunk. And it must also work when loading an icon and then
       saving it using another name. */

    data = AllocMemIcon(&icon->ni_DiskObject, filesize, MEMF_PUBLIC,
        IconBase);
    if (!data)
        return FALSE;

    if (Read(file, data, filesize) != filesize)
    {
        D(bug("[%s] Can't read from file\n", __func__));
        return FALSE;
    }

    icon->ni_Extra.Data = data;
    icon->ni_Extra.Size = filesize;
    
    icon->ni_Extra.PNG[0].Offset = 0;
    icon->ni_Extra.PNG[0].Size = filesize;
    {
        ULONG width = ~0, height = ~0;
        
        icon->ni_Image[0].ARGB = ReadMemPNG(&icon->ni_DiskObject, icon->ni_Extra.Data + icon->ni_Extra.PNG[0].Offset, &width, &height, chunknames, chunkpointer, IconBase);
        if (icon->ni_Image[0].ARGB == NULL) {
            D(bug("[%s] Can't parse PNG image at 0\n", __func__));
            return FALSE;
        }

        icon->ni_Face.Width  = width;
        icon->ni_Face.Height = height;
        icon->ni_Face.Aspect = ICON_ASPECT_RATIO_UNKNOWN;
        
        #define DO(x) (&x->ni_DiskObject)
        
        DO(icon)->do_Magic          = WB_DISKMAGIC;
        DO(icon)->do_Version        = (WB_DISKVERSION << 8) | WB_DISKREVISION;
        DO(icon)->do_Type           = 0;  /* Invalid */
        DO(icon)->do_CurrentX       = NO_ICON_POSITION;
        DO(icon)->do_CurrentY       = NO_ICON_POSITION;
        DO(icon)->do_Gadget.Width   = width;
        DO(icon)->do_Gadget.Height  = height;
        DO(icon)->do_StackSize      = AROS_STACKSIZE;

        if (chunkpointer[0])
        {
            UBYTE *chunkdata;
            ULONG  chunksize;
            ULONG  ttnum = 0;
            ULONG  ttarraysize = 0;
            BOOL   ok = TRUE;
            
            GetChunkInfo(chunkpointer[0], (APTR *)&chunkdata, &chunksize);
            
            while(chunksize >= 4)
            {
                ULONG attr;
                IPTR  val = 0;
                BOOL  need_drawerdata = FALSE;

                attr = GetBE32(chunkdata);
                chunksize -=4;
                chunkdata += 4;
                
                switch(attr)
                {
                    case ATTR_DRAWERX:
                    case ATTR_DRAWERY:
                    case ATTR_DRAWERWIDTH:
                    case ATTR_DRAWERHEIGHT:
                    case ATTR_DRAWERFLAGS:
                    case ATTR_DRAWERFLAGS2:
                    case ATTR_DRAWERFLAGS3:
                    case ATTR_VIEWMODES:
                    case ATTR_VIEWMODES2:
                    case ATTR_DD_CURRENTX:
                    case ATTR_DD_CURRENTY:
                        need_drawerdata = TRUE;
                        /* Fall through */
                        
                    case ATTR_ICONX:
                    case ATTR_ICONY:
                    case ATTR_STACKSIZE:
                    case ATTR_TYPE:
                    case ATTR_FRAMELESS:
                        if (chunksize >= 4)
                        {
                            val = GetBE32(chunkdata);
                            chunksize -=4;
                            chunkdata += 4;
                        }
                        else
                        {
                            ok = FALSE;
                        }
                        break;
                        
                    /* case ATTR_UNKNOWN: */
                    case ATTR_DEFAULTTOOL:
                    case ATTR_TOOLTYPE:
                        val = (IPTR)chunkdata;
                        chunksize -= strlen((char *)val) + 1;
                        chunkdata += strlen((char *)val) + 1;
                        
                        if (chunksize < 0)
                        {
                            ok = FALSE;
                        }
                        break;
                        
                    default:
                        /* Unknown attribute/tag. Impossible to handle correctly
                           if we don't know if it's a string attribute or not. */
                        ok = FALSE;
                        break;
                                            
                } /* switch(attr) */
                
                if (!ok) break;
                
                if (need_drawerdata && !(DO(icon)->do_DrawerData))
                {
                    DO(icon)->do_DrawerData =
                        AllocMemIcon(DO(icon), sizeof(struct DrawerData),
                            MEMF_PUBLIC | MEMF_CLEAR, IconBase);
                    if (!(DO(icon)->do_DrawerData))
                    {
                        ok = FALSE;
                        break;
                    }
                    
                    DO(icon)->do_DrawerData->dd_NewWindow.LeftEdge = 20;
                    DO(icon)->do_DrawerData->dd_NewWindow.TopEdge  = 20;
                    DO(icon)->do_DrawerData->dd_NewWindow.Width    = 300;
                    DO(icon)->do_DrawerData->dd_NewWindow.Height   = 200;
            DO(icon)->do_Gadget.UserData = (APTR)1; /* See DupDiskObject logic */
                }
                
                switch(attr)
                {
                    case ATTR_ICONX:
                        DO(icon)->do_CurrentX = val;
                        break;
                        
                    case ATTR_ICONY:
                        DO(icon)->do_CurrentY = val;
                        break;
                        
                    case ATTR_STACKSIZE:
                        DO(icon)->do_StackSize = val;
                        break;
                        
                    case ATTR_DRAWERX:
                        DO(icon)->do_DrawerData->dd_NewWindow.LeftEdge = (WORD)val;
                        break;

                    case ATTR_DRAWERY:
                        DO(icon)->do_DrawerData->dd_NewWindow.TopEdge = (WORD)val;
                        break;

                    case ATTR_DRAWERWIDTH:
                        DO(icon)->do_DrawerData->dd_NewWindow.Width = (WORD)val;
                        break;

                    case ATTR_DRAWERHEIGHT:
                        DO(icon)->do_DrawerData->dd_NewWindow.Height = (WORD)val;
                        break;
                        
                    case ATTR_DRAWERFLAGS:
                        DO(icon)->do_DrawerData->dd_Flags     = flags_to_ddflags(val);
                        DO(icon)->do_DrawerData->dd_ViewModes = flags_to_ddviewmodes(val);
                        break;

                    case ATTR_DEFAULTTOOL:
                        DO(icon)->do_DefaultTool =
                            AllocMemIcon(DO(icon), strlen((char *)val) + 1,
                                MEMF_PUBLIC | MEMF_CLEAR, IconBase);
                        if (DO(icon)->do_DefaultTool)
                        {
                            strcpy(DO(icon)->do_DefaultTool, (char *)val);
                        }
                        else
                        {
                            ok = FALSE;
                        }
                        break;
                    case ATTR_FRAMELESS:
                        icon->ni_Frameless = val ? TRUE : FALSE;
                        break;

                    case ATTR_TOOLTYPE:
                        ttnum++;

                        D(bug("[Icon.PNG] Got tooltype number %u : %s\n", ttnum - 1, val));

                        if (ttarraysize < ttnum + 1)
                        {
                            STRPTR *old_tooltypes = DO(icon)->do_ToolTypes;

                            ttarraysize += 10;

                            DO(icon)->do_ToolTypes =
                                AllocMemIcon(DO(icon),
                                    ttarraysize * sizeof(APTR),
                                    MEMF_PUBLIC | MEMF_CLEAR, IconBase);
                            if (DO(icon)->do_ToolTypes)
                            {
                                D(bug("[Icon.PNG] Allocated array of %u entries @ 0x%p (old 0x%p)\n", ttarraysize, DO(icon)->do_ToolTypes, old_tooltypes));
                                if (old_tooltypes)
                                {
                                    CopyMemQuick(old_tooltypes, DO(icon)->do_ToolTypes, (ttnum - 1) * sizeof(APTR));

                                    /* TODO: Free old array */
                                }
                            }
                            else
                            {
                                ok = FALSE;
                            }
                        }

                        if (!ok) break;
                        
                        DO(icon)->do_ToolTypes[ttnum - 1] =
                            AllocMemIcon(DO(icon), strlen((char *)val) + 1,
                            MEMF_PUBLIC | MEMF_CLEAR, IconBase);
                        if (DO(icon)->do_ToolTypes[ttnum - 1])
                        {
                            strcpy(DO(icon)->do_ToolTypes[ttnum - 1], (char *)val);
                        }
                        else
                        {
                            ok = FALSE;
                        }
                        break;
                        
                } /* switch(attr) */
                                
                if (!ok) break;
                
            } /* while(chunksize >= 4) */
            
            if (!ok)
            {
                D(bug("=== Failure during icOn chunk parsing ===\n"));
                FreeIconPNG(&icon->ni_DiskObject, IconBase);
                return FALSE;
            }
            
            
        } /* if (chunkpointer[0]) */

        #undef DO

        /*
         * FIXME: Someone killed PNG Icon do_Type detection here which causes
         *        following lines to always free DrawerData even when it
         *        shouldn't be freed (only possible to know if do_Type is
         *        known). So for now following lines disabled and DrawerData
         *        is always kept (even when it shouldn't).
         */

#if 0
        if (icon->ni_DiskObject.do_DrawerData &&
            (icon->ni_DiskObject.do_Type != WBDISK) &&
            (icon->ni_DiskObject.do_Type != WBDRAWER) &&
            (icon->ni_DiskObject.do_Type != WBGARBAGE))
        {
            FreePooled(pool, icon->ni_DiskObject.do_DrawerData, sizeof(struct DrawerData));
            icon->ni_DiskObject.do_DrawerData = NULL;
        }
#endif
        
    } /**/
    
    /* Look for a possible 2nd PNG image attached onto the first one */
    {
        UBYTE *filepos = icon->ni_Extra.Data + icon->ni_Extra.PNG[0].Offset + 8;
        BOOL done = FALSE;
        
        while(!done && filepos < ((UBYTE *)icon->ni_Extra.Data + icon->ni_Extra.Size))
        {
            ULONG chunksize = GetBE32(filepos);
            ULONG chunktype = GetBE32(filepos + 4);

            chunksize += 12;

            if (chunktype == MAKE_ID('I', 'E', 'N', 'D'))
            {
                done = TRUE;
            }

            filepos += chunksize;
        }
        
        if (filepos + 8 < (UBYTE *)icon->ni_Extra.Data + icon->ni_Extra.Size)
        {
            ULONG offset = filepos - (UBYTE *)icon->ni_Extra.Data;
            
            icon->ni_Extra.PNG[0].Size = offset;
            icon->ni_Extra.PNG[1].Offset = offset;
            icon->ni_Extra.PNG[1].Size = (filesize - icon->ni_Extra.PNG[0].Size);
            icon->ni_Image[1].ARGB = ReadMemPNG(&icon->ni_DiskObject, filepos, &icon->ni_Face.Width, &icon->ni_Face.Height, NULL, NULL, IconBase);
        }
        
    } /**/

    /* If there's no image for selected-state, generate one */
    if (!icon->ni_Image[1].ARGB)
    {
        ULONG size = icon->ni_Face.Width * icon->ni_Face.Height;
        
        if ((icon->ni_Image[1].ARGB =
            AllocMemIcon(&icon->ni_DiskObject, size * sizeof(ULONG),
                MEMF_PUBLIC, IconBase)))
        {
            ULONG *src = (ULONG *)icon->ni_Image[0].ARGB;
            ULONG *dst = (ULONG *)icon->ni_Image[1].ARGB;
            
            while(size--)
            {
                ULONG pixel = *src++;
                
                /* Effects like in changetoselectediconcolor.c */
                
        #if EFFECT == EFFECT_LIGHTEN
            #if AROS_BIG_ENDIAN
                pixel = (pixel & 0xFF000000) +
                        ((pixel >> 1) & 0x007F7F7F) +
                        0x00808080;
            #else
                pixel = (pixel & 0x000000FF) +
                        ((pixel >> 1) & 0x7F7F7F00) +
                        0x80808000;
            #endif
        #elif EFFECT == EFFECT_TINT_BLUE
            #if AROS_BIG_ENDIAN
                pixel = (pixel & 0xFF000000) +
                        ((pixel >> 1) & 0x007F7F7F) +
                        0x00000080;
            #else
                pixel = (pixel & 0x000000FF) +
                        ((pixel >> 1) & 0x7F7F7F00) +
                        0x80000000;
            #endif
            
        #elif EFFECT == EFFECT_XOR
            #if AROS_BIG_ENDIAN
                pixel = (pixel & 0xFF000000) +
                        ((pixel & 0x00FFFFFF) ^ 0x00FFFFFF);
            #else
                pixel = (pixel & 0x000000FF) +
                        ((pixel & 0xFFFFFF00) ^ 0xFFFFFF00);
            #endif
        #endif
                *dst++ = pixel;
            }
        }
    }

    /*
     * Support old-style AROS PNG icons. 3rd party applications
     * will still be using them.
     */

    if (!MakePlanarImages(icon, IconBase))
    {
        D(bug("Planar image creation failed\n"));
        FreeIconPNG(&icon->ni_DiskObject, IconBase);
        return FALSE;
    }

    return TRUE;
    
}

/****************************************************************************************/

STATIC VOID MakeCRCTable(struct IconBase *IconBase)
{
    unsigned long c;
    int n, k;

    IconBase->ib_CRCTable = AllocMem(256 * sizeof(ULONG), MEMF_ANY);
    if (!IconBase->ib_CRCTable)
        return;
    for (n = 0; n < 256; n++)
    {
        c = (unsigned long) n;
        for (k = 0; k < 8; k++)
        {
            if (c & 1)
                c = 0xedb88320L ^ (c >> 1);
            else
                c = c >> 1;
        }
        IconBase->ib_CRCTable[n] = c;
    }
}

/****************************************************************************************/

STATIC ULONG UpdateCRC(ULONG crc, UBYTE *buf, ULONG len, struct IconBase *IconBase)
{
    ULONG c = crc;
    ULONG n;

    for (n = 0; n < len; n++)
    {
        c = IconBase->ib_CRCTable[(c ^ buf[n]) & 0xff] ^ (c >> 8);
    }

    return c;
    
}

/****************************************************************************************/

/* State of the icOn chunk being written: the CRC and the size cover its data */
struct IconChunkWriter
{
    BPTR             file;
    ULONG            chunksize;
    ULONG            crc;
    struct IconBase *IconBase;
};

STATIC BOOL WriteIconData(struct IconChunkWriter *w, CONST_APTR data, ULONG len)
{
    struct IconBase *IconBase = w->IconBase;

    if (Write(w->file, data, len) != len) return FALSE;
    w->crc = UpdateCRC(w->crc, (UBYTE *)data, len, IconBase);
    w->chunksize += len;
    return TRUE;
}

/****************************************************************************************/

STATIC BOOL WriteIconAttr(struct IconChunkWriter *w, ULONG id, ULONG val)
{
    ULONG buf[2];

    buf[0] = AROS_LONG2BE(id);
    buf[1] = AROS_LONG2BE(val);

    return WriteIconData(w, buf, 8);
}

/****************************************************************************************/

STATIC BOOL WriteIconStrAttr(struct IconChunkWriter *w, ULONG id, char *val)
{
    id = AROS_LONG2BE(id);

    if (!WriteIconData(w, &id, 4)) return FALSE;

    return WriteIconData(w, val, strlen(val) + 1);
}
                          
/****************************************************************************************/

STATIC BOOL WriteIconChunk(BPTR file, struct DiskObject *dobj, struct IconBase *IconBase)
{
    struct IconChunkWriter w;
    ULONG sizeseek = Seek(file, 0, OFFSET_CURRENT);
    UBYTE buf[] = {0x12, 0x34, 0x56, 0x78, 'i', 'c', 'O', 'n'};
    ULONG be;
    
    if (sizeseek < 0) return FALSE;

    w.file      = file;
    w.chunksize = 0;
    w.crc       = 0xFFFFFFFF;
    w.IconBase  = IconBase;

    /* Write Chunk size + chunk type */
    if (Write(file, buf, 8) != 8) return FALSE;
    
    w.crc = UpdateCRC(w.crc, buf + 4, 4, IconBase); /* chunksize is excluded from CRC */

    /* Write Frameless */
    if (!WriteIconAttr(&w, ATTR_FRAMELESS, NATIVEICON(dobj)->ni_Frameless))
    {
        return FALSE;
    }

    /* Write Stack Size */

    if (!WriteIconAttr(&w, ATTR_STACKSIZE, dobj->do_StackSize))
    {
        return FALSE;
    }
    
    /* Write Icon X Position */
    if (dobj->do_CurrentX != NO_ICON_POSITION)
    {
        if (!WriteIconAttr(&w, ATTR_ICONX, dobj->do_CurrentX))
        {
            return FALSE;
        }
    }

    /* Write Icon Y Position */
    if (dobj->do_CurrentY != NO_ICON_POSITION)
    {
        if (!WriteIconAttr(&w, ATTR_ICONY, dobj->do_CurrentY))
        {
            return FALSE;
        }
    }
            
    if (dobj->do_DrawerData)
    {
        /* Write the drawer window box: ATTR_DRAWERX, ATTR_DRAWERY,
           ATTR_DRAWERWIDTH and ATTR_DRAWERHEIGHT follow each other, as do
           LeftEdge, TopEdge, Width and Height in the NewWindow */
        WORD *box = &dobj->do_DrawerData->dd_NewWindow.LeftEdge;
        ULONG i;

        _Static_assert(ATTR_DRAWERY == ATTR_DRAWERX + 1 && ATTR_DRAWERWIDTH == ATTR_DRAWERX + 2 &&
                       ATTR_DRAWERHEIGHT == ATTR_DRAWERX + 3, "drawer box attributes");
        _Static_assert(offsetof(struct NewWindow, TopEdge) == offsetof(struct NewWindow, LeftEdge) + sizeof(WORD) &&
                       offsetof(struct NewWindow, Width) == offsetof(struct NewWindow, LeftEdge) + 2 * sizeof(WORD) &&
                       offsetof(struct NewWindow, Height) == offsetof(struct NewWindow, LeftEdge) + 3 * sizeof(WORD),
                       "drawer box fields");

        for (i = 0; i < 4; i++)
        {
            if (!WriteIconAttr(&w, ATTR_DRAWERX + i, box[i]))
            {
                return FALSE;
            }
        }
        
        if (!WriteIconAttr(&w, ATTR_DRAWERFLAGS, dd_to_flags(dobj)))
        {
            return FALSE;
        }
        
    } /* if (dobj->do_DrawerData) */
            
    if (dobj->do_DefaultTool)
    {
        if (!WriteIconStrAttr(&w, ATTR_DEFAULTTOOL, dobj->do_DefaultTool))
        {
            return FALSE;
        }
        
    }

    if (dobj->do_ToolTypes)
    {
        STRPTR *tt = (STRPTR *)dobj->do_ToolTypes;
        
        for(tt = (STRPTR *)dobj->do_ToolTypes; *tt; tt++)
        {
            if (!WriteIconStrAttr(&w, ATTR_TOOLTYPE, *tt))
            {
                return FALSE;
            }
        }
    }
                    
    /* Write CRC */
        
    be = AROS_LONG2BE(w.crc ^ 0xFFFFFFFF);
    if (Write(file, &be, 4) != 4) return FALSE;
    
    /* Trim any garbage at end of file */
    if (SetFileSize(file, 0, OFFSET_CURRENT) < 0) return FALSE;

    /* Write chunk's size */
    if (Seek(file, sizeseek, OFFSET_BEGINNING) < 0) return FALSE;

    be = AROS_LONG2BE(w.chunksize);
    if (Write(file, &be, 4) != 4) return FALSE;
        
    if (Seek(file, 0, OFFSET_END) < 0) return FALSE;
    
    return TRUE;
}

/****************************************************************************************/

BOOL WriteIconPNG(BPTR file, struct DiskObject *dobj, struct IconBase *IconBase)
{
    struct NativeIcon   *nativeicon = NATIVEICON(dobj);
    UBYTE               *mempos = nativeicon->ni_Extra.Data + nativeicon->ni_Extra.PNG[0].Offset;
    BOOL                 done = FALSE;

    D(bug("%s: ni=%p, ni->ni_Extra.Data = %p\n", __func__, nativeicon, nativeicon->ni_Extra.Data));
    if (nativeicon->ni_Extra.Data == NULL)
        return FALSE;

    ObtainSemaphore(&IconBase->iconlistlock);
    if (!IconBase->ib_CRCTable)
        MakeCRCTable(IconBase);
    ReleaseSemaphore(&IconBase->iconlistlock);
    if (!IconBase->ib_CRCTable)
        return FALSE;

    /* Write PNG header */
    if (Write(file, mempos, 8) != 8) return FALSE;
    
    mempos += 8;
    
    while(!done)
    {
        ULONG chunksize = GetBE32(mempos);
        ULONG chunktype = GetBE32(mempos + 4);
        
        chunksize += 12;
        
        if (chunktype == MAKE_ID('I', 'E', 'N', 'D'))
        {
            if (!WriteIconChunk(file, dobj, IconBase)) return FALSE;
            done = TRUE;
        }
        
        if (chunktype != MAKE_ID('i', 'c', 'O', 'n'))
        {
            if (Write(file, mempos, chunksize) != chunksize)
            {
                return FALSE;
            }
        }
        
        mempos += chunksize;
        
    }

    if (nativeicon->ni_Extra.PNG[1].Offset > 0)
    {
        ULONG size = nativeicon->ni_Extra.PNG[1].Size;

        /* 2nd PNG Image attached */

        if (Write(file, nativeicon->ni_Extra.Data + nativeicon->ni_Extra.PNG[1].Offset, size) != size) return FALSE;
    }
        
    return TRUE;
}

/****************************************************************************************/

VOID FreeIconPNG(struct DiskObject *dobj, struct IconBase *IconBase)
{
}

/****************************************************************************************/
