/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dostags.h>
#include <graphics/gfxbase.h>
#include <graphics/rpattr.h>
#include <intuition/imageclass.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/iffparse.h>
#include <proto/datatypes.h>

#include "compilerspecific.h"
#include "debug.h"

#include "methods.h"

/**************************************************************************************************/

struct FileBitMapHeader
{
    UBYTE        bmh_Width[2];
    UBYTE        bmh_Height[2];
    UBYTE        bmh_Left[2];
    UBYTE        bmh_Top[2];
    UBYTE        bmh_Depth;
    UBYTE        bmh_Masking;
    UBYTE        bmh_Compression;
    UBYTE        bmh_Pad;
    UBYTE        bmh_Transparent[2];
    UBYTE        bmh_XAspect;
    UBYTE        bmh_YAspect;
    UBYTE        bmh_PageWidth[2];
    UBYTE        bmh_PageHeight[2];
};

/**************************************************************************************************/

static LONG propchunks[] =
{
    ID_ILBM, ID_BMHD,
    ID_ILBM, ID_CMAP,
    ID_ILBM, ID_CAMG
};

/**************************************************************************************************/

static UBYTE *UnpackByteRun1(UBYTE *source, UBYTE *dest, LONG unpackedsize)
{
    UBYTE r;
    BYTE c;

    for(;;)
    {
	c = (BYTE)(*source++);
	if (c >= 0)
	{
    	    while(c-- >= 0)
	    {
		*dest++ = *source++;
		if (--unpackedsize <= 0) return source;
	    }
	}
	else if (c != -128)
	{
    	    c = -c;
	    r = *source++;

	    while(c-- >= 0)
	    {
		*dest++ = r;
		if (--unpackedsize <= 0) return source;
	    }
	}
    }   
}

/**************************************************************************************************/

static BOOL MakeBitMap(Object *o, struct IFFHandle *handle, struct BitMapHeader *bmhd,
    	    	       struct FileBitMapHeader *file_bmhd, struct ContextNode *body_cn)
{
    struct BitMap *bm;
    UBYTE   	  *src, *body, *uncompress_buf;
    LONG    	   y, p, w16, bm_bpr, body_bpr, copy_bpr, totdepth;
    
    totdepth = bmhd->bmh_Depth;
    if (file_bmhd->bmh_Masking == mskHasMask) totdepth++;
    
    w16      = (bmhd->bmh_Width + 15) & ~15;
    body_bpr = w16 / 8;

    p = body_cn->cn_Size;
    if ((file_bmhd->bmh_Compression == cmpByteRun1))
    {
    	p += body_bpr * totdepth;
    }
    
    body = AllocVec(p, MEMF_ANY);
    if (!body)
    {
    	SetIoErr(ERROR_NO_FREE_STORE);
    	return FALSE;
    }
    
    if (ReadChunkBytes(handle, body, body_cn->cn_Size) != body_cn->cn_Size)
    {
    	FreeVec(body);
	SetIoErr(ERROR_UNKNOWN);
	return FALSE;
    }
    
    bm = AllocBitMap(bmhd->bmh_Width, bmhd->bmh_Height, bmhd->bmh_Depth, BMF_CLEAR, NULL);
    if (!bm)
    {
    	FreeVec(body);
	SetIoErr(ERROR_NO_FREE_STORE);
	return FALSE;
    }
    
    bm_bpr   = bm->BytesPerRow;
    copy_bpr = (body_bpr < bm_bpr) ? body_bpr : bm_bpr;
    
    switch(file_bmhd->bmh_Compression)
    {
    	case cmpNone:
	    src = body;
	    for(y = 0; y < bmhd->bmh_Height; y++)
	    {
	    	for(p = 0; p < bmhd->bmh_Depth; p++)
		{
		    UBYTE *dest;
		    
		    dest = bm->Planes[p] + y * bm_bpr;
		    
		    CopyMem(src, dest, copy_bpr);
		    src += body_bpr;
		}
		
		if (file_bmhd->bmh_Masking == mskHasMask) src += body_bpr;
	    }
	    break;
	    
	case cmpByteRun1:
	    uncompress_buf = body + body_cn->cn_Size;
	    src = body;
	    
	    for(y = 0; y < bmhd->bmh_Height; y++)
	    {
	    	UBYTE *copysrc = uncompress_buf;
		
	    	src = UnpackByteRun1(src, uncompress_buf, body_bpr * totdepth);
		
		for(p = 0; p < bmhd->bmh_Depth; p++)
		{
		    UBYTE *dest;

		    dest = bm->Planes[p] + y * bm_bpr;
		    
		    CopyMem(copysrc, dest, copy_bpr);
		    copysrc += body_bpr;
		}
		
	    }
	    break;
	    
    } /* switch(file_bmhd->bmh_Compression) */
    
    SetDTAttrs(o, NULL, NULL, DTA_NominalHoriz, bmhd->bmh_Width ,
    	    	    	      DTA_NominalVert , bmhd->bmh_Height,
			      PDTA_BitMap     , (IPTR)bm    	,
			      TAG_DONE);
			      
    FreeVec(body);
    SetIoErr(0);
    
    return TRUE;
}

/**************************************************************************************************/

static BOOL ReadILBM(Object *o)
{
    struct FileBitMapHeader *file_bmhd;
    struct BitMapHeader     *bmhd;
    struct IFFHandle	    *handle;
    struct StoredProperty   *bmhd_prop, *cmap_prop, *camg_prop;
    struct ContextNode	    *cn;
    struct ColorRegister    *colorregs;
    ULONG    	    	    *cregs;
    ULONG   	    	    numcolors;
    IPTR    	    	    sourcetype;
    LONG    	    	    error;
 
    D(bug("ilbm.datatype/ReadILBM()\n"));
       
    if (GetDTAttrs(o, DTA_SourceType	, (IPTR)&sourcetype ,
    	    	      DTA_Handle    	, (IPTR)&handle     , 
		      PDTA_BitMapHeader , (IPTR)&bmhd	    ,
		      TAG_DONE	    	    	    	     ) != 3)
    {
    	SetIoErr(ERROR_OBJECT_NOT_FOUND);
	return FALSE;
    }
    
    if ((sourcetype != DTST_FILE) && (sourcetype != DTST_CLIPBOARD))
    {
    	SetIoErr(ERROR_OBJECT_NOT_FOUND);
	return FALSE;
    }
    
    if (!handle || !bmhd)
    {
    	SetIoErr(ERROR_OBJECT_NOT_FOUND);
	return FALSE;
    }

    if (PropChunks(handle, propchunks, 3) != 0)
    {
    	SetIoErr(ERROR_NO_FREE_STORE);
	return FALSE;
    }
   
    if (StopChunk(handle, ID_ILBM, ID_BODY) != 0)
    {
    	SetIoErr(ERROR_NO_FREE_STORE);
	return FALSE;
    }
    
    error = ParseIFF(handle, IFFPARSE_SCAN);
    if (error)
    {
    	SetIoErr(ERROR_OBJECT_WRONG_TYPE);
	return FALSE;
    }
    
    bmhd_prop = FindProp(handle, ID_ILBM, ID_BMHD);
    cmap_prop = FindProp(handle, ID_ILBM, ID_CMAP);
    camg_prop = FindProp(handle, ID_ILBM, ID_CAMG);

    cn = CurrentChunk(handle);
    if ((cn->cn_Type != ID_ILBM) ||
    	(cn->cn_ID != ID_BODY) ||
	(bmhd_prop == NULL) ||
	(cmap_prop == NULL))
    {
    	SetIoErr(ERROR_REQUIRED_ARG_MISSING);
	return FALSE;
    }

    file_bmhd = (struct FileBitMapHeader *)bmhd_prop->sp_Data;
    
    if ((file_bmhd->bmh_Depth > 8) ||
    	(file_bmhd->bmh_Compression > 1))
    {
    	SetIoErr(ERROR_NOT_IMPLEMENTED);
	return FALSE;
    }
    
    bmhd->bmh_Width  = bmhd->bmh_PageWidth  = file_bmhd->bmh_Width [0] * 256 + file_bmhd->bmh_Width [1];
    bmhd->bmh_Height = bmhd->bmh_PageHeight = file_bmhd->bmh_Height[0] * 256 + file_bmhd->bmh_Height[1];
    bmhd->bmh_Depth  = file_bmhd->bmh_Depth;
   
    numcolors = cmap_prop->sp_Size / 3;

    SetDTAttrs(o, NULL, NULL, PDTA_NumColors, numcolors, TAG_DONE);
    
    if (GetDTAttrs(o, PDTA_ColorRegisters   , (IPTR *)&colorregs,
    	    	      PDTA_CRegs    	    , (IPTR *)&cregs	,
		      TAG_DONE	    	    	    	    	 ) == 2)
    {
    	if (colorregs && cregs)
	{
	    UBYTE *src = (UBYTE *)cmap_prop->sp_Data;
	    LONG   i;
	    
	    for(i = 0; i < numcolors; i++)
	    {
	    	colorregs->red   = *src++;
		colorregs->green = *src++;
		colorregs->blue  = *src++;
		
		*cregs++ = ((ULONG)colorregs->red)   * 0x01010101;
		*cregs++ = ((ULONG)colorregs->green) * 0x01010101;
		*cregs++ = ((ULONG)colorregs->blue)  * 0x01010101;
		
		colorregs++;
	    }
	    
	}
    }

    {
    	IPTR name = NULL;
	
	GetDTAttrs(o, DTA_Name, (IPTR)&name, TAG_DONE);
	
    	SetDTAttrs(o, NULL, NULL, DTA_ObjName, name, TAG_DONE);
    }
    
    return MakeBitMap(o, handle, bmhd, file_bmhd, cn);
}

/**************************************************************************************************/

static IPTR ILBM_New(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;
    
    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
    	if (!ReadILBM((Object *)retval))
	{
	    CoerceMethod(cl, (Object *)retval, OM_DISPOSE);
	    retval = 0;
	}
    }
    
    return retval;
}

/**************************************************************************************************/

#ifdef __AROS__
AROS_UFH3S(IPTR, DT_Dispatcher,
	   AROS_UFHA(Class *, cl, A0),
	   AROS_UFHA(Object *, o, A2),
	   AROS_UFHA(Msg, msg, A1))
#else
ASM IPTR DT_Dispatcher(register __a0 struct IClass *cl, register __a2 Object * o, register __a1 Msg msg)
#endif
{
#ifdef __AROS__
    AROS_USERFUNC_INIT
#endif

    IPTR retval;

    putreg(REG_A4, (long) cl->cl_Dispatcher.h_SubEntry);        /* Small Data */

    D(bug("ilbm.datatype/DT_Dispatcher: Entering\n"));

    switch(msg->MethodID)
    {
	case OM_NEW:
	    D(bug("ilbm.datatype/DT_Dispatcher: Method OM_NEW\n"));
	    retval = ILBM_New(cl, o, (struct opSet *)msg);
	    break;

	default:
	    retval = DoSuperMethodA(cl, o, msg);
	    break;

    } /* switch(msg->MethodID) */

    D(bug("ilbm.datatype/DT_Dispatcher: Leaving\n"));

    return retval;
    
#ifdef __AROS__
    AROS_USERFUNC_EXIT
#endif
}

/**************************************************************************************************/

struct IClass *DT_MakeClass(struct Library *ilbmbase)
{
    struct IClass *cl;
    
    cl = MakeClass("ilbm.datatype", "picture.datatype", 0, 0, 0);

    D(bug("ilbm.datatype/DT_MakeClass: DT_Dispatcher 0x%lx\n", (unsigned long) DT_Dispatcher));

    if (cl)
    {
#ifdef __AROS__
	cl->cl_Dispatcher.h_Entry = (HOOKFUNC) AROS_ASMSYMNAME(DT_Dispatcher);
#else
	cl->cl_Dispatcher.h_Entry = (HOOKFUNC) DT_Dispatcher;
#endif
	cl->cl_Dispatcher.h_SubEntry = (HOOKFUNC) getreg(REG_A4);
	cl->cl_UserData = (IPTR)ilbmbase; /* Required by datatypes (see disposedtobject) */
    }

    return cl;
}

/**************************************************************************************************/

