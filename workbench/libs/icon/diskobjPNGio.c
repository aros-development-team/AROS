/*
    Copyright � 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/****************************************************************************************/

#include <datatypes/datatypes.h>
#include <intuition/intuition.h>

#include <proto/dos.h>
#include <proto/alib.h>
#include <proto/png.h>
#include <proto/datatypes.h>

#include <aros/bigendianio.h>
#include <aros/asmcall.h>

#include "icon_intern.h"

#define DEBUG 1
#include <aros/debug.h>

#define ATTR_ICONX   	    0x80001001
#define ATTR_ICONY   	    0x80001002
#define ATTR_DRAWERX 	    0x80001003
#define ATTR_DRAWERY 	    0x80001004
#define ATTR_DRAWERWIDTH    0x80001005
#define ATTR_DRAWERHEIGHT   0x80001006
#define ATTR_DRAWERFLAGS    0x80001007
#define ATTR_UNKNOWN	    0x80001008 /* probably toolwindow? */
#define ATTR_STACKSIZE	    0x80001009
#define ATTR_DEFAULTTOOL    0x8000100a
#define ATTR_TOOLTYPE	    0x8000100b
#define ATTR_VIEWMODES      0x8000100c  //OS4 PNG use that
#define ATTR_DD_CURRENTX    0x8000100d  //OS4 PNG use that
#define ATTR_DD_CURRENTY    0x8000100e  //OS4 PNG use that
#define ATTR_TYPE           0x8000100f  //OS4 PNG use that
#define ATTR_UNKNOWN2       0x80001011  //hexdump showed 4 bytes following
#define ATTR_UNKNOWN3       0x80001012  //hexdump showed 4 bytes following
#define ATTR_DRAWERFLAGS2   0x80001107  //writen from AFA to store needed dopus Magellan settings

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

/****************************************************************************************/

STATIC BOOL MakePlanarImage(struct NativeIcon *icon, struct Image **img,
    	    	    	    UBYTE *src, struct IconBase *IconBase)
{
    LONG width16 = (icon->iconPNG.width + 15) & ~15;
    LONG bpr = width16 / 8;
    LONG planesize = bpr * icon->iconPNG.height;
    LONG x, y;
    UWORD *p1, *p2;
    ULONG *s = (ULONG *)src;
    
    *img = (struct Image *)AllocPooled(icon->pool, sizeof(struct Image) + planesize * 2);
    if (*img == NULL) return FALSE;
    
    (*img)->Width     = icon->iconPNG.width;
    (*img)->Height    = icon->iconPNG.height;
    (*img)->Depth     = 2;
    (*img)->ImageData = (UWORD *)(*img + 1);
    (*img)->PlanePick = 3; 
    
    p1 = (UWORD *)(*img)->ImageData;
    p2 = p1 + planesize / 2;

    for(y = 0; y < icon->iconPNG.height; y++)
    {
    	ULONG pixelmask = 0x8000;
	UWORD plane1dat = 0;
	UWORD plane2dat = 0;
	
    	for(x = 0; x < icon->iconPNG.width; x++)
	{
	    ULONG pixel = *s++;
	    
	#if AROS_BIG_ENDIAN
	    if ((pixel & 0xFF000000) > 0x80000000)
	    {
	    	pixel = (((pixel & 0x00FF0000) >> 16) +
		    	 ((pixel & 0x0000FF00) >> 8)  +
			 ((pixel & 0x000000FF)) +
			 127) / 256;
	#else
	    if ((pixel & 0x000000FF) > 0x80)
    	    {
	    	pixel = (((pixel & 0x0000FF00) >> 8) +
		    	 ((pixel & 0x00FF0000) >> 16)  +
			 ((pixel & 0xFF000000) >> 24) +
			 127) / 256;
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
		
    } /* for(y = 0; y < icon->iconPNG.height; y++) */
    
    return TRUE;
    
}

/****************************************************************************************/

STATIC BOOL MakePlanarImages(struct NativeIcon *icon, struct IconBase *IconBase)
{
    if (!MakePlanarImage(icon,
    	    	    	 (struct Image **)&icon->dobj.do_Gadget.GadgetRender,
			 icon->iconPNG.img1,
			 IconBase))
    {
    	return FALSE;
    }
    
    icon->dobj.do_Gadget.Flags |= GFLG_GADGIMAGE;
    
    if (!icon->iconPNG.img2) return TRUE;
    
    if (MakePlanarImage(icon,
    	    	    	 (struct Image **)&icon->dobj.do_Gadget.SelectRender,
			 icon->iconPNG.img1,
			 IconBase))
    {
    	icon->dobj.do_Gadget.Flags |= GFLG_GADGHIMAGE;
    }
        
    return TRUE;
}

/****************************************************************************************/

BOOL ReadIconPNG(struct DiskObject **ret, BPTR file, struct IconBase *IconBase)
{
    static STRPTR chunknames[] =
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
    ULONG   	    	filesize;
    APTR    	    	pool;

    if (Seek(file, 0, OFFSET_END) < 0) return FALSE;
    if ((filesize = Seek(file, 0, OFFSET_BEGINNING)) < 0) return FALSE;

    pool = CreatePool(MEMF_ANY | MEMF_CLEAR, 1024, 1024);
    if (!pool) return FALSE;
    
    icon = AllocPooled(pool, sizeof(struct NativeIcon) + filesize);
    if (!icon)
    {
    	DeletePool(pool);
	return FALSE;
    }
    
    icon->pool = pool;
    
    icon->iconPNG.filebuffer = (UBYTE *)(icon + 1);
    icon->iconPNG.filebuffersize = filesize;
    
    /* Need a copy of whole file in memory for icon saving :-\ Because
       that should save file back as it was, only with modified or new
       icOn chunk. And it must also work when loading an icon and then
       saving it using another name. */
       
    if (Read(file, icon->iconPNG.filebuffer, filesize) != filesize)
    {
    	DeletePool(pool);
	return FALSE;
    }
    
    icon->iconPNG.handle = PNG_LoadImageMEM(icon->iconPNG.filebuffer, filesize,
    	    	    	    	    	    chunknames, chunkpointer, TRUE);

    if (!icon->iconPNG.handle)
    {
    	FreeIconPNG(&icon->dobj, IconBase);
	return FALSE;
    }
    
    {
    	LONG width, height;
	
	PNG_GetImageInfo(icon->iconPNG.handle, &width, &height, NULL, NULL);

	icon->iconPNG.width  = width;
	icon->iconPNG.height = height;
	
	PNG_GetImageData(icon->iconPNG.handle, (APTR *)&icon->iconPNG.img1, NULL);
	
	#define DO(x) (&x->dobj)
	
	DO(icon)->do_Magic    	    = WB_DISKMAGIC;
	DO(icon)->do_Version  	    = (WB_DISKVERSION << 8) | WB_DISKREVISION;
	DO(icon)->do_Type     	    = WBPROJECT;
	DO(icon)->do_CurrentX 	    = NO_ICON_POSITION;
	DO(icon)->do_CurrentY 	    = NO_ICON_POSITION;
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
	    
	    PNG_GetChunkInfo(chunkpointer[0], (APTR *)&chunkdata, &chunksize);
	    
	    while(chunksize >= 4)
	    {
	    	ULONG attr;
		IPTR  val = 0;
		BOOL  need_drawerdata = FALSE;

		attr = (chunkdata[0] << 24) | (chunkdata[1] << 16) | (chunkdata[2] << 8) | chunkdata[3];
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
		    case ATTR_VIEWMODES:
		    case ATTR_DD_CURRENTX:
		    case ATTR_DD_CURRENTY:
		    	need_drawerdata = TRUE;
			/* Fall through */
			
		    case ATTR_ICONX:
		    case ATTR_ICONY:
		    case ATTR_STACKSIZE:
		    case ATTR_TYPE:
		    case ATTR_UNKNOWN2:
		    case ATTR_UNKNOWN3:
		    	if (chunksize >= 4)
			{
			    val = (chunkdata[0] << 24) | (chunkdata[1] << 16) | (chunkdata[2] << 8) | chunkdata[3];
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
		    DO(icon)->do_DrawerData = AllocPooled(pool, sizeof(struct DrawerData));
		    if (!(DO(icon)->do_DrawerData))
		    {
		    	ok = FALSE;
			break;
		    }
		    
		    DO(icon)->do_DrawerData->dd_NewWindow.LeftEdge = 20;
		    DO(icon)->do_DrawerData->dd_NewWindow.TopEdge  = 20;
		    DO(icon)->do_DrawerData->dd_NewWindow.Width    = 300;
		    DO(icon)->do_DrawerData->dd_NewWindow.Height   = 200;		    
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
		    	DO(icon)->do_DefaultTool = AllocPooled(pool, strlen((char *)val) + 1);
			if (DO(icon)->do_DefaultTool)
			{
			    strcpy(DO(icon)->do_DefaultTool, (char *)val);
			}
			else
			{
			    ok = FALSE;
			}
			break;
			
		    case ATTR_TOOLTYPE:
		    	ttnum++;
			if (ttarraysize < ttnum + 1)
			{
			    STRPTR *old_tooltypes = DO(icon)->do_ToolTypes;
			    ULONG  old_ttarraysize = ttarraysize;
			    
			    ttarraysize += 10;
			    
			    DO(icon)->do_ToolTypes = AllocPooled(pool, ttarraysize * sizeof(APTR));
			    if (DO(icon)->do_ToolTypes)
			    {
			    	if (old_tooltypes)
				{
				    memcpy(DO(icon)->do_ToolTypes, old_tooltypes, (ttnum - 1) * sizeof(APTR));				    
				}
			    }
			    else
			    {
			    	ok = FALSE;
			    }
			    
			    if (old_tooltypes) FreePooled(pool, old_tooltypes, old_ttarraysize * sizeof(APTR));			    
			}
			
			if (!ok) break;
			
			DO(icon)->do_ToolTypes[ttnum - 1] = AllocPooled(pool, strlen((char *)val) + 1);
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
	    
	    PNG_FreeChunk(chunkpointer[0]);
	    
	    if (!ok)
	    {
    	    	D(bug("=== Failure during icOn chunk parsing ===\n"));
    	    	FreeIconPNG(&icon->dobj, IconBase);
	    	return FALSE;
	    }
	    
	    
	} /* if (chunkpointer[0]) */

	#undef DO
	
#warning "FIXME: Someone killed PNG Icon do_Type detection here which causes"
#warning "       following lines to always free DrawerData even when it"
#warning "       shouldn't be freed (only possible to know if do_Type is"
#warning "       known). So for now following lines disabled and DrawerData"
#warning "       is always kept (even when it shouldn't)."

#if 0	
	if (icon->dobj.do_DrawerData &&
	    (icon->dobj.do_Type != WBDISK) &&
	    (icon->dobj.do_Type != WBDRAWER) &&
	    (icon->dobj.do_Type != WBGARBAGE))
	{
	    FreePooled(pool, icon->dobj.do_DrawerData, sizeof(struct DrawerData));
	    icon->dobj.do_DrawerData = NULL;
	}
#endif
	
    } /**/
    
    /* Look for a possible 2nd PNG image attached onto the first one */
    {
    	UBYTE *filepos = icon->iconPNG.filebuffer + 8;
    	BOOL done = FALSE;
	
	while(!done)
	{
    	    ULONG chunksize = (filepos[0] << 24) | (filepos[1] << 16) |
	    	    	      (filepos[2] << 8) | filepos[3];
    	    ULONG chunktype = (filepos[4] << 24) | (filepos[5] << 16) |
	    	    	      (filepos[6] << 8) | filepos[7];

	    chunksize += 12;

	    if (chunktype == MAKE_ID('I', 'E', 'N', 'D'))
	    {
		done = TRUE;
	    }

	    filepos += chunksize;
	}
	
	if (filepos + 8 < icon->iconPNG.filebuffer + icon->iconPNG.filebuffersize)
	{
	    ULONG offset = filepos - icon->iconPNG.filebuffer;
	    
	    icon->iconPNG.handle2 = PNG_LoadImageMEM(filepos, filesize - offset, 0, 0, TRUE);
	    
	    if (icon->iconPNG.handle2)
	    {
    	    	LONG width, height;
	
	    	PNG_GetImageInfo(icon->iconPNG.handle2, &width, &height, NULL, NULL);
		
		if ((width == icon->iconPNG.width) &&
		    (height == icon->iconPNG.height))
		{
		    PNG_GetImageData(icon->iconPNG.handle2, (APTR *)&icon->iconPNG.img2, NULL);
		}
		else
		{
		    PNG_FreeImage(icon->iconPNG.handle2);
		    icon->iconPNG.handle2 = NULL;
		}
	    	
	    }
	}
    	
    } /**/
    
    /* If there's no image for selected-state, generate one */
    if (!icon->iconPNG.img2)
    {
    	ULONG size = icon->iconPNG.width * icon->iconPNG.height;
	
    	if ((icon->iconPNG.img2 = AllocPooled(pool, size * sizeof(ULONG))))
	{
	    ULONG *src = (ULONG *)icon->iconPNG.img1;
	    ULONG *dst = (ULONG *)icon->iconPNG.img2;
	    
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
    
    /* Make fallback planar images */
    
    if (!MakePlanarImages(icon, IconBase))
    {
    	D(bug("Planar image creation failed\n"));
    	FreeIconPNG(&icon->dobj, IconBase);
	return FALSE;
    }
        
    *ret = &icon->dobj;
    
    return TRUE;
    
}

/****************************************************************************************/

STATIC VOID MakeCRCTable(struct IconBase *IconBase)
{
    unsigned long c;
    int n, k;

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
    
    IconBase->ib_CRCTableComputed = TRUE;
    
}

/****************************************************************************************/

STATIC ULONG UpdateCRC(ULONG crc, UBYTE *buf, ULONG len, struct IconBase *IconBase)
{
    ULONG c = crc;
    ULONG n;

    ObtainSemaphore(&IconBase->iconlistlock);   
    if (!IconBase->ib_CRCTableComputed)
    {
	MakeCRCTable(IconBase);
    }
    ReleaseSemaphore(&IconBase->iconlistlock);

    for (n = 0; n < len; n++)
    {
	c = IconBase->ib_CRCTable[(c ^ buf[n]) & 0xff] ^ (c >> 8);
    }

    return c;
    
}

/****************************************************************************************/

STATIC BOOL WriteIconAttr(BPTR file, ULONG id, ULONG val, ULONG *chunksize,
    	    	    	  ULONG *crc, struct IconBase *IconBase)
{
    UBYTE buf[8];
    
    buf[0] = id >> 24;
    buf[1] = id >> 16;
    buf[2] = id >> 8;
    buf[3] = id;
    buf[4] = val >> 24;
    buf[5] = val >> 16;
    buf[6] = val >> 8;
    buf[7] = val;
    
    if (Write(file, buf, 8) != 8) return FALSE;

    *chunksize += 8;
    *crc = UpdateCRC(*crc, buf, 8, IconBase);    
    return TRUE;
}

/****************************************************************************************/

STATIC BOOL WriteIconStrAttr(BPTR file, ULONG id, char *val, ULONG *chunksize,
    	    	    	     ULONG *crc, struct IconBase *IconBase)
{
    UBYTE buf[4];
    ULONG len = strlen(val) + 1;
    
    buf[0] = id >> 24;
    buf[1] = id >> 16;
    buf[2] = id >> 8;
    buf[3] = id;
    
    if (Write(file, buf, 4) != 4) return FALSE;
    *crc = UpdateCRC(*crc, buf, 4, IconBase);
        
    if (Write(file, val, len) != len) return FALSE;
    *crc = UpdateCRC(*crc, val, len, IconBase);

    *chunksize += 4 + len;
    
    return TRUE;
}
 			  
/****************************************************************************************/

STATIC BOOL WriteIconChunk(BPTR file, struct DiskObject *dobj, struct IconBase *IconBase)
{
    ULONG crc = 0xFFFFFFFF;
    ULONG chunksize = 0;
    ULONG sizeseek = Seek(file, 0, OFFSET_CURRENT);
    UBYTE buf[] = {0x12, 0x34, 0x56, 0x78, 'i', 'c', 'O', 'n'};
    
    if (sizeseek < 0) return FALSE;

    /* Write Chunk size + chunk type */    
    if (Write(file, buf, 8) != 8) return FALSE;
    
    crc = UpdateCRC(crc, buf + 4, 4, IconBase); /* chunksize is excluded from CRC */

    /* Write Stack Size */

    if (!WriteIconAttr(file, ATTR_STACKSIZE, dobj->do_StackSize, &chunksize, &crc, IconBase))
    {
	return FALSE;
    }
    
    /* Write Icon X Position */
    if (dobj->do_CurrentX != NO_ICON_POSITION)
    {
    	if (!WriteIconAttr(file, ATTR_ICONX, dobj->do_CurrentX, &chunksize, &crc, IconBase))
	{
	    return FALSE;
	}
    }

    /* Write Icon Y Position */
    if (dobj->do_CurrentY != NO_ICON_POSITION)
    {
    	if (!WriteIconAttr(file, ATTR_ICONY, dobj->do_CurrentY, &chunksize, &crc, IconBase))
	{
	    return FALSE;
	}
    }
            
    if (dobj->do_DrawerData)
    {
    	if (!WriteIconAttr(file, ATTR_DRAWERX, dobj->do_DrawerData->dd_NewWindow.LeftEdge,
	    &chunksize, &crc, IconBase))
	{
	    return FALSE;
	}
	
    	if (!WriteIconAttr(file, ATTR_DRAWERY, dobj->do_DrawerData->dd_NewWindow.TopEdge,
	    &chunksize, &crc, IconBase))
	{
	    return FALSE;
	}

    	if (!WriteIconAttr(file, ATTR_DRAWERWIDTH, dobj->do_DrawerData->dd_NewWindow.Width,
	    &chunksize, &crc, IconBase))
	{
	    return FALSE;
	}

    	if (!WriteIconAttr(file, ATTR_DRAWERHEIGHT, dobj->do_DrawerData->dd_NewWindow.Height,
	    &chunksize, &crc, IconBase))
	{
	    return FALSE;
	}
	
    	if (!WriteIconAttr(file, ATTR_DRAWERFLAGS, dd_to_flags(dobj),
	    &chunksize, &crc, IconBase))
	{
	    return FALSE;
	}
	
    } /* if (dobj->do_DrawerData) */
    	    
    if (dobj->do_DefaultTool)
    {
    	if (!WriteIconStrAttr(file, ATTR_DEFAULTTOOL, dobj->do_DefaultTool,
	    &chunksize, &crc, IconBase))
	{
	    return FALSE;
	}
    	
    }

    if (dobj->do_ToolTypes)
    {
    	STRPTR *tt = (STRPTR *)dobj->do_ToolTypes;
	
	for(tt = (STRPTR *)dobj->do_ToolTypes; *tt; tt++)
	{
	    if (!WriteIconStrAttr(file, ATTR_TOOLTYPE, *tt, &chunksize,
	    	    	    	  &crc, IconBase))
	    {
	    	return FALSE;
	    }
	}
    }
        	    
    /* Write CRC */
        
    crc ^= 0xFFFFFFFF;
    buf[0] = crc >> 24;
    buf[1] = crc >> 16;
    buf[2] = crc >> 8;
    buf[3] = crc;
    if (Write(file, buf, 4) != 4) return FALSE;
    
    /* Write chunk's size */
    if (Seek(file, sizeseek, OFFSET_BEGINNING) < 0) return FALSE;

    buf[0] = chunksize >> 24;
    buf[1] = chunksize >> 16;
    buf[2] = chunksize >> 8;
    buf[3] = chunksize;
    if (Write(file, buf, 4) != 4) return FALSE;
        
    if (Seek(file, 0, OFFSET_END) < 0) return FALSE;
    
    return TRUE;
}

/****************************************************************************************/

BOOL WriteIconPNG(BPTR file, struct DiskObject *dobj, struct IconBase *IconBase)
{
    struct NativeIcon 	*nativeicon = NATIVEICON(dobj);
    struct IconPNG  	*iconpng = &nativeicon->iconPNG;
    UBYTE   	    	*mempos = iconpng->filebuffer;
    BOOL    	    	 done = FALSE;
    
    /* Write PNG header */
    if (Write(file, iconpng->filebuffer, 8) != 8) return FALSE;
    
    mempos += 8;
    
    while(!done)
    {
    	ULONG chunksize = (mempos[0] << 24) | (mempos[1] << 16) |
	    	    	  (mempos[2] << 8) | mempos[3];
    	ULONG chunktype = (mempos[4] << 24) | (mempos[5] << 16) |
	    	    	  (mempos[6] << 8) | mempos[7];
    	
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

    if (mempos < iconpng->filebuffer + iconpng->filebuffersize)
    {
    	ULONG size = iconpng->filebuffer + iconpng->filebuffersize - mempos;
	
    	/* 2nd PNG Image attached */
	
	if (Write(file, mempos, size) != size) return FALSE;
    }
        
    return TRUE;
}

/****************************************************************************************/

VOID FreeIconPNG(struct DiskObject *dobj, struct IconBase *IconBase)
{
    if (dobj)
    {
    	struct NativeIcon *nativeicon = NATIVEICON(dobj);
    
    	if (nativeicon->iconPNG.handle)
	{
	    PNG_FreeImage(nativeicon->iconPNG.handle);
	}

    	if (nativeicon->iconPNG.handle2)
	{
	    PNG_FreeImage(nativeicon->iconPNG.handle2);
	}
	
	if (nativeicon->pool) DeletePool(nativeicon->pool);
    }    
}

/****************************************************************************************/
