/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

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
#include <cybergraphx/cybergraphics.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/iffparse.h>
#include <proto/layers.h>

#include <proto/datatypes.h>
#include <proto/cybergraphics.h>

#include "compilerspecific.h"
#include "debug.h"
#include "pictureclass.h"
#include "colorhandling.h"

#include "methods.h"

/**************************************************************************************************/

const IPTR SupportedMethods[] =
{
 OM_NEW,
 OM_GET,
 OM_SET,
 OM_UPDATE,
 OM_DISPOSE,

 GM_LAYOUT,
 GM_HITTEST,
 GM_GOACTIVE,
 GM_HANDLEINPUT,
 GM_RENDER,

 DTM_PROCLAYOUT,
 DTM_ASYNCLAYOUT,
 DTM_FRAMEBOX,
 DTM_SELECT,
 DTM_CLEARSELECTED,
 DTM_COPY,
 DTM_PRINT,
 DTM_WRITE,
 
 PDTM_WRITEPIXELARRAY,

 (~0)
};

/**************************************************************************************************/

STATIC IPTR DT_SetMethod(struct IClass *cl, struct Gadget *g, struct opSet *msg);

/**************************************************************************************************/

STATIC ULONG NotifyAttrChanges(Object * o, VOID * ginfo, ULONG flags, ULONG tag1,...)
{
    return(DoMethod(o, OM_NOTIFY, &tag1, ginfo, flags));
}

/**************************************************************************************************/

STATIC struct Gadget *DT_NewMethod(struct IClass *cl, Object *o, struct opSet *msg)
{
    struct Gadget *g;
    struct TagItem *attrs;
    struct TagItem *ti;
    struct Picture_Data *pd;

    attrs = msg->ops_AttrList;

#if 0	/* DTA_SourceType is handled by subclasses */
    IPTR sourcetype;
    IPTR handle;

    sourcetype = GetTagData(DTA_SourceType, DTST_FILE, attrs);
    handle = GetTagData(DTA_Handle, NULL, attrs);

    if( sourcetype == DTST_RAM && handle == NULL )
    {
	D(bug("picture.datatype/OM_NEW: Creating an empty object\n"));
    }
    else if( !(sourcetype==DTST_CLIPBOARD || sourcetype==DTST_FILE) )
    {
        D(bug("picture.datatype/OM_NEW: wrong DTA_SourceType\n"));
        SetIoErr(ERROR_OBJECT_WRONG_TYPE);
        return FALSE;
    }
#endif /* 0 */

    g = (struct Gadget *) DoSuperMethodA(cl, o, (Msg) msg);
    if( !g )
    {
        return FALSE;
    }
    D(bug("picture.datatype/OM_NEW: Created object 0x%lx\n", (long)g));

    pd = (struct Picture_Data *) INST_DATA(cl, g);
    memset(pd, 0, sizeof(struct Picture_Data));

    pd->Precision = PRECISION_IMAGE;
    pd->ModeID = INVALID_ID;
    pd->Remap = TRUE;
    pd->BitmapMode = TRUE;
    pd->PixelFormat = -1;
   
    while((ti=NextTagItem(&attrs)))
    {
        switch (ti->ti_Tag)
        {
            case OBP_Precision:
            {
                pd->Precision = ti->ti_Data;
                D(bug("picture.datatype/OM_NEW: Tag ID OBP_Precision: %ld\n", (long)pd->Precision));
                break;
            }
         
            case PDTA_Remap:
            {
                pd->Remap = ti->ti_Data;
                D(bug("picture.datatype/OM_NEW: Tag ID PDTA_Remap: %ld\n", (long)pd->Remap));
                break;
            }
         
            case PDTA_NumSparse:
            {
                pd->NumSparse = ti->ti_Data;
                D(bug("picture.datatype/OM_NEW: Tag ID PDTA_NumSparse: %ld\n", (long)pd->NumSparse));
                break;
            }
         
            case PDTA_SparseTable:
            {
                D(bug("picture.datatype/OM_NEW: Tag ID PDTA_SparseTable\n"));
                if(!(pd->NumSparse && ti->ti_Data))
                {
                    break;
                }
                CopyMem((APTR) ti->ti_Data, (APTR) pd->SparseTable, pd->NumSparse);
                break;
            }
        }
    }
   
    D(bug("picture.datatype/OM_NEW: Setting attributes\n"));
    DT_SetMethod(cl, g, msg);
   
    return(g);
}

/**************************************************************************************************/

STATIC IPTR DT_DisposeMethod(struct IClass *cl, Object *o, Msg msg)
{
    struct Picture_Data *pd;
    IPTR RetVal;
    register int i;
   
    RetVal=1;
   
    pd=(struct Picture_Data *) INST_DATA(cl, o);
   
    if(pd)
    {
        if(pd->ChunkyBuffer)
        {
            FreeVec((void *) pd->ChunkyBuffer);
        }
      
        if(pd->NumAlloc)
        {
            for(i=0; i<pd->NumAlloc; i++)
            {
                ReleasePen(pd->TheScreen->ViewPort.ColorMap, pd->ColTable[i]);
            }
      
            pd->NumAlloc=0;
        }
      
        if(pd->DestBM && (pd->DestBM != pd->bm))
        {
            FreeBitMap(pd->DestBM);
        }
      
        if(pd->bm)
        {
            FreeBitMap((void *) pd->bm);
        }
    }
   
    RetVal += DoSuperMethodA(cl, o, msg);
   
    return(RetVal);
}

/**************************************************************************************************/

STATIC IPTR DT_SetMethod(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    struct Picture_Data *pd;
    struct TagItem *tl;
    struct TagItem *ti;
    IPTR RetVal;
    struct RastPort *rp;
   
    pd=(struct Picture_Data *) INST_DATA(cl, g);
    tl=msg->ops_AttrList;
    RetVal=1;
   
    while((ti=NextTagItem(&tl)))
    {
        switch (ti->ti_Tag)
        {
            case PDTA_ModeID:
            {
                pd->ModeID = ti->ti_Data;
                D(bug("picture.datatype/OM_SET: Tag PDTA_ModeID: 0x%lx\n", (long)pd->ModeID));
                break;
            }
         
            case PDTA_BitMap:
            {
                pd->bm = (struct BitMap *) ti->ti_Data;
                D(bug("picture.datatype/OM_SET: Tag PDTA_BitMap: 0x%lx\n", (long)pd->bm));
                break;
            }
         
            case PDTA_NumColors:
            {
                pd->NumColors = ti->ti_Data;
                D(bug("picture.datatype/OM_SET: Tag PDTA_NumColors: %ld\n", (long)pd->NumColors));
                break;
            }
         
            case PDTA_Screen:
            {
                pd->TheScreen = (struct Screen *) ti->ti_Data;
                D(bug("picture.datatype/OM_SET: Tag PDTA_Screen: 0x%lx\n", (long)pd->TheScreen));
                break;
            }
         
            case PDTA_FreeSourceBitMap:
            {
                pd->FreeSourceBitMap = (BOOL) ti->ti_Data;
                D(bug("picture.datatype/OM_SET: Tag PDTA_FreeSourceBitMap: %ld\n", (long)pd->FreeSourceBitMap));
                break;
            }
         
            case PDTA_Grab:
            {
                Point *ThePoint;
            
                D(bug("picture.datatype/OM_SET: Tag PDTA_Grab\n"));
                ThePoint = (Point *) ti->ti_Data;
                if(!ThePoint)
                {
                 break;
                }
                pd->Grab.x = ThePoint->x;
                pd->Grab.y = ThePoint->y;
                break;
            }
         
            case PDTA_ClassBitMap:
            {
                pd->ClassBM = (struct BitMap *) ti->ti_Data;
                D(bug("picture.datatype/OM_SET: Tag PDTA_ClassBitMap: 0x%lx\n", (long)pd->ClassBM));
                break;
            }

#ifdef MYDEBUG
            default:
            {
             register int i;
             int Known;
         
             Known=FALSE;
         
             for(i=0; i<NumAttribs; i++)
             {
              if(ti->ti_Tag==KnownAttribs[i])
              {
               Known=TRUE;
         
               D(bug("picture.datatype/OM_SET: Tag %s: 0x%lx (%ld)\n", AttribNames[i], (long)ti->ti_Data, (long)ti->ti_Data));
              }
             }
         
             if(!Known)
             {
              D(bug("picture.datatype/OM_SET: Tag ID 0x%lx: 0x%lx\n", (long)ti->ti_Tag, (long)ti->ti_Data));
             }
            }
#endif /* MYDEBUG */
        }
    }

#if 0
    if(msg->ops_GInfo)
    {
        DoMethod((Object *) g, GM_LAYOUT, msg->ops_GInfo, TRUE);
    }
#endif

    /* Do not call the SuperMethod if you come from OM_NEW! */
    if(!(msg->MethodID == OM_NEW))
    {
        RetVal += (IPTR) DoSuperMethodA(cl, (Object *) g, (Msg) msg);
    }
   
    if(msg->ops_GInfo)
    {
#if 1
        if (RetVal)
#else
        if(OCLASS((Object *) g) == cl)
#endif
        {
            rp=ObtainGIRPort(msg->ops_GInfo);
            if(rp)
            {
                DoMethod((Object *) g, GM_RENDER, msg->ops_GInfo, rp, GREDRAW_UPDATE);
                ReleaseGIRPort (rp);
            }
        }
   
#if 0 /* stegerg: ?? */
        if(msg->MethodID == OM_UPDATE)
        {
             DoMethod((Object *) g, OM_NOTIFY, msg->ops_AttrList, msg->ops_GInfo, 0);
        }
#endif
    }
   
    return(RetVal);
}

/**************************************************************************************************/

STATIC IPTR DT_GetMethod(struct IClass *cl, struct Gadget *g, struct opGet *msg)
{
    struct Picture_Data *pd;
    
    pd=(struct Picture_Data *) INST_DATA(cl, g);
    
    switch(msg->opg_AttrID)
    {
	case PDTA_ModeID:
	{
	    D(bug("picture.datatype/OM_GET: Tag PDTA_ModeID: 0x%lx\n", (long)pd->ModeID));
	    *(msg->opg_Storage)=pd->ModeID;
	    break;
	}

	case PDTA_BitMapHeader:
	{
	    D(bug("picture.datatype/OM_GET: Tag PDTA_BitMapHeader: 0x%lx\n", (long)&pd->bmhd));
	    *(msg->opg_Storage)=(ULONG) &pd->bmhd;
	    break;
	}

	case PDTA_BitMap:
	{
	    D(bug("picture.datatype/OM_GET: Tag PDTA_BitMap: 0x%lx\n", (long)pd->bm));
	    *(msg->opg_Storage)=(ULONG) pd->bm;
	    break;
	}

	case PDTA_ColorRegisters:
	{
	    D(bug("picture.datatype/OM_GET: Tag PDTA_ColorRegisters: 0x%lx\n", (long)&pd->ColMap));
	    *(msg->opg_Storage)=(ULONG) &pd->ColMap;
	    break;
	}

	case PDTA_CRegs:
	{
	    D(bug("picture.datatype/OM_GET: Tag PDTA_CRegs: 0x%lx\n", (long)&pd->CRegs));
	    *(msg->opg_Storage)=(ULONG) &pd->CRegs;
	    break;
	}

	case PDTA_GRegs:
	{
	    D(bug("picture.datatype/OM_GET: Tag PDTA_GRegs: 0x%lx\n", (long)&pd->GRegs));
	    *(msg->opg_Storage)=(ULONG) &pd->GRegs;
	    break;
	}

	case PDTA_ColorTable:
	{
	    D(bug("picture.datatype/OM_GET: Tag PDTA_ColorTable: 0x%lx\n", (long)&pd->ColTable));
	    *(msg->opg_Storage)=(ULONG) &pd->ColTable;
	    break;
	}

	case PDTA_ColorTable2:
	{
	    D(bug("picture.datatype/OM_GET: Tag PDTA_ColorTable2: 0x%lx\n", (long)&pd->ColTable2));
	    *(msg->opg_Storage)=(ULONG) &pd->ColTable2;
	    break;
	}

	case PDTA_Allocated:
	{
	    D(bug("picture.datatype/OM_GET: Tag PDTA_Allocated: %ld\n", (long)pd->Allocated));
	    *(msg->opg_Storage)=(ULONG) pd->Allocated;
	    break;
	}

	case PDTA_NumColors:
	{
	    D(bug("picture.datatype/OM_GET: Tag PDTA_NumColors: %ld\n", (long)pd->NumColors));
	    *(msg->opg_Storage)=(ULONG) pd->NumColors;
	    break;
	}

	case PDTA_NumAlloc:
	{
	    D(bug("picture.datatype/OM_GET: Tag PDTA_NumAlloc: %ld\n", (long)pd->NumAlloc));
	    *(msg->opg_Storage)=(ULONG) pd->NumAlloc;
	    break;
	}

	case PDTA_Grab:
	{
	    D(bug("picture.datatype/OM_GET: Tag PDTA_Grab: 0x%lx\n", (long)&pd->Grab));
	    *(msg->opg_Storage)=(ULONG) &pd->Grab;
	    break;
	}

	case PDTA_DestBitMap:
	{
	    D(bug("picture.datatype/OM_GET: Tag PDTA_DestBitMap: 0x%lx\n", (long)pd->DestBM));
	    *(msg->opg_Storage)=(ULONG) pd->DestBM;
	    break;
	}

	case PDTA_ClassBitMap:
	{
	    D(bug("picture.datatype/OM_GET: Tag PDTA_ClassBitMap: 0x%lx\n", (long)pd->ClassBM));
	    *(msg->opg_Storage)=(ULONG) pd->ClassBM;
	    break;
	}

	case DTA_Methods:
	{
	    D(bug("picture.datatype/OM_GET: Tag DTA_Methods: 0x%lx\n", (long)SupportedMethods));
	    *(msg->opg_Storage)=(ULONG) SupportedMethods;
	    break;
	}

	default:
	{

#ifdef MYDEBUG
	    register int i;
	    int Known;
	
	    Known=FALSE;
	
	    for(i=0; i<NumAttribs; i++)
	    {
	     if(msg->opg_AttrID==KnownAttribs[i])
	     {
	      Known=TRUE;
	
	      D(bug("picture.datatype/OM_GET: Tag ID: %s\n", AttribNames[i]));
	     }
	    }
	
	    if(!Known)
	    {
	     D(bug("picture.datatype/OM_GET: Tag ID: 0x%lx\n", msg->opg_AttrID));
	    }
#endif /* MYDEBUG */

	    return(DoSuperMethodA(cl, (Object *) g, (Msg) msg));
	}
    } /* switch AttrID */

    return TRUE;
}

/**************************************************************************************************/

STATIC IPTR DT_Render(struct IClass *cl, struct Gadget *g, struct gpRender *msg)
{
    struct Picture_Data *pd;
    struct DTSpecialInfo *si;
   
    struct IBox *domain;
    IPTR TopVert, TopHoriz, NominalWidth, NominalHeight;
   
    long SrcX, SrcY, DestX, DestY, SizeX, SizeY;
   
    pd=(struct Picture_Data *) INST_DATA(cl, g);

    if(pd->Remap && !pd->Remapped)
    {
        D(bug("picture.datatype/GM_RENDER: No layout done yet !\n"));
        return FALSE;
    }

    si=(struct DTSpecialInfo *) g->SpecialInfo;
    if(si->si_Flags & DTSIF_LAYOUT)
    {
        D(bug("picture.datatype/GM_RENDER: In layout process !\n"));
        return FALSE;
    }
   
    if(pd->BitmapMode && !pd->DestBM)
    {
        D(bug("picture.datatype/GM_RENDER: No DestBM set for planar mode !\n"));
        return FALSE;
    }
   
    if(!(GetDTAttrs((Object *) g, DTA_Domain, &domain,
   			       DTA_TopHoriz, &TopHoriz,
   			       DTA_TopVert, &TopVert,
   			       DTA_NominalHoriz, &NominalWidth,
   			       DTA_NominalVert, &NominalHeight,
   			       TAG_DONE) == 5))
    {
        D(bug("picture.datatype/GM_RENDER: Couldn't get dimensions\n"));
        return FALSE;
    }
   
#if 1
    D(bug("picture.datatype/GM_RENDER: Domain: left %ld top %ld width %ld height %ld\n", domain->Left, domain->Top, domain->Width, domain->Height));
    D(bug("picture.datatype/GM_RENDER: TopHoriz     : %lu\n", (unsigned long) TopHoriz));
    D(bug("picture.datatype/GM_RENDER: TopVert      : %lu\n", (unsigned long) TopVert));
    D(bug("picture.datatype/GM_RENDER: Width        : %lu\n", (unsigned long) NominalWidth));
    D(bug("picture.datatype/GM_RENDER: Height       : %lu\n", (unsigned long) NominalHeight));
#endif
   
#if 0
    ObtainSemaphore(&(si->si_Lock));
#endif
   
    SrcX = TopHoriz;
    SrcY = TopVert;
    DestX = domain->Left;
    DestY = domain->Top;
   
    SizeX = MIN(NominalWidth - TopHoriz, domain->Width);
    SizeY = MIN(NominalHeight - TopVert, domain->Height);
    D(bug("picture.datatype/GM_RENDER: Size X/Y: %ld %ld\n", SizeX, SizeY));

    if( pd->TrueColorSrc )
    {
    	/* true color source */
        D(bug("picture.datatype/GM_RENDER: TrueColor source, PixelFormat %ld\n", pd->PixelFormat));
        if( !WritePixelArray(pd->ChunkyBuffer,
			    SrcX,
			    SrcY,
			    pd->CBWidthBytes,
			    msg->gpr_RPort,
			    DestX,
			    DestY,
			    SizeX,
			    SizeY,
			    pd->PixelFormat) )
	{
	    D(bug("picture.datatype/GM_RENDER: WritePixelArray failed !\n"));
	    return FALSE;
	}
    }
    else
    {
    	/* planar or chunky source */
        D(bug("picture.datatype/GM_RENDER: Planar/chunky source, NumColors %ld\n", (long)pd->NumColors));
        BltBitMapRastPort(pd->DestBM,
                          SrcX,
                          SrcY,
                          msg->gpr_RPort,
                          DestX,
                          DestY,
                          SizeX,
                          SizeY,
                          0xC0);
    }

#if 0
    ReleaseSemaphore(&(si->si_Lock));
#endif
   
    return TRUE;
}

/**************************************************************************************************/

STATIC IPTR DT_Layout(struct IClass *cl, struct Gadget *g, struct gpLayout *msg)
{
 IPTR RetVal;

 NotifyAttrChanges((Object *) g, msg->gpl_GInfo, NULL,
				 GA_ID, g->GadgetID,
				 DTA_Busy, TRUE,
				 TAG_DONE);

 RetVal=DoSuperMethodA(cl, (Object *) g, (Msg) msg);

 RetVal += (IPTR) DoAsyncLayout((Object *) g, msg);

 return(RetVal);
}

/**************************************************************************************************/

STATIC IPTR DT_ProcLayout(struct IClass *cl, struct Gadget *g, struct gpLayout *msg)
{
 IPTR RetVal;

 NotifyAttrChanges((Object *) g, msg->gpl_GInfo, NULL,
				 GA_ID, g->GadgetID,
				 DTA_Busy, TRUE,
				 TAG_DONE);

 RetVal=DoSuperMethodA(cl, (Object *) g, (Msg) msg);

 return(RetVal);
}

/**************************************************************************************************/

STATIC IPTR DT_AsyncLayout(struct IClass *cl, struct Gadget *g, struct gpLayout *msg)
{
 IPTR RetVal;
 struct Picture_Data *pd;
 struct DTSpecialInfo *si;

 struct IBox *domain;
 IPTR Width, Height;
 ULONG SrcWidth, SrcHeight;
 unsigned int SrcDepth;

 STRPTR Title;

 RetVal=1;
 pd=(struct Picture_Data *) INST_DATA(cl, g);
 si=(struct DTSpecialInfo *) g->SpecialInfo;

 SrcWidth = pd->bmhd.bmh_Width;
 SrcHeight = pd->bmhd.bmh_Height;
 SrcDepth = pd->bmhd.bmh_Depth;
 D(bug("picture.datatype/DTM_ASYNCLAYOUT: Source Width %ld Height %ld Depth %ld\n", SrcWidth, SrcHeight, (long)SrcDepth));
 if( !SrcWidth || !SrcHeight || !SrcDepth )
 {
  D(bug("picture.datatype/DTM_ASYNCLAYOUT: Neccessary fields in BitMapHeader not set !\n"));
  return FALSE;
 }
 if( SrcDepth > 8 )
     pd->TrueColorSrc = TRUE;
 else
     pd->TrueColorSrc = FALSE;

 /*
  *  BitMap noch nicht gesetzt!
  */
 if( pd->BitmapMode && !pd->bm )
 {
  D(bug("picture.datatype/DTM_ASYNCLAYOUT: Planar source mode, but source bitmap not set !\n"));
  return FALSE;
 }

 /*
  *  Attribute holen
  */
 if(!(GetDTAttrs((Object *) g, DTA_Domain, (IPTR) &domain,
			       DTA_ObjName, (IPTR) &Title,
			       DTA_NominalHoriz, &Width,
			       DTA_NominalVert, &Height,
			       TAG_DONE) == 4))
 {
  return FALSE;
 }

 /*
  *  Sperren
  */
 ObtainSemaphore(&(si->si_Lock));

 /*
  *  Wir brauchen nur einmal taetig zu werden.
  */
 if(msg->gpl_Initial)
 {
  /*
   *  Wir muessen das Bild remappen
   */
  if(pd->Remap)
  {
   unsigned int DestDepth;

   /*
    *  Groessen bestimmen
    */
   if(!pd->TheScreen)
   {
    pd->TheScreen=msg->gpl_GInfo->gi_Screen;
   }

   DestDepth=GetBitMapAttr(pd->TheScreen->RastPort.BitMap, BMA_DEPTH);
   D(bug("picture.datatype/DTM_ASYNCLAYOUT: Destination Depth %ld\n", (long)DestDepth));

   if(pd->TrueColorSrc)
   {
    if(pd->BitmapMode)
    {
     D(bug("picture.datatype/DTM_ASYNCLAYOUT: Bitmap source only possible with up to 8 bits !\n"));
     return FALSE;
    }
    if(DestDepth > 8)
    {
     D(bug("picture.datatype/DTM_ASYNCLAYOUT: TrueColor src/dest mode; no remapping required\n"));
     pd->TrueColorDest = TRUE;
    }
    else
    {
     D(bug("picture.datatype/DTM_ASYNCLAYOUT: TrueColor src, Colormapped dest mode\n"));
     pd->TrueColorDest = FALSE;
     D(bug("picture.datatype/DTM_ASYNCLAYOUT: ... sorry, not implemented yet !\n"));
     return FALSE;
    }
   }
   else
   {
    unsigned int BM_Width, BM_Height, BM_Width16;
    unsigned int DestNumColors;
    struct RastPort SrcRP, DestRP;
    int i, j;

    if(DestDepth > 8)
    {
     D(bug("picture.datatype/DTM_ASYNCLAYOUT: Colormapped src, TrueColor dest mode\n"));
     pd->TrueColorDest = TRUE;
     /* For now, we render it in colormapped dest mode, so there is no difference */
     DestDepth=8;
     DestNumColors=256;
    }
    else
    {
     D(bug("picture.datatype/DTM_ASYNCLAYOUT: Colormapped src, Colormapped dest mode\n"));
     pd->TrueColorDest = FALSE;
     DestNumColors=1<<DestDepth;
    }

    /*
     *  Aufraeumen was vielleicht uebriggeblieben ist
     */
    if(pd->DestBM && (pd->DestBM != pd->bm))
    {
     FreeBitMap(pd->DestBM);
     pd->DestBM=NULL;
    }
 
    if(pd->NumAlloc)
    {
     for(i=0; i<pd->NumAlloc; i++)
     {
      ReleasePen(pd->TheScreen->ViewPort.ColorMap, pd->ColTable[i]);
     }
     pd->NumAlloc=0;
    }
 
    if(pd->ChunkyBuffer && pd->BitmapMode)
    {
     FreeVec((void *) pd->ChunkyBuffer);
    }
 
    /*
     *  pd->GRegs vorbelegen
     */
    memset(pd->GRegs, 0xFF, 768*sizeof(ULONG));
 
    /*
     *  SparseTable ausfuellen
     */
    memset(pd->SparseTable, 0x0, 256);
    pd->NumSparse=pd->NumColors;
 
    /*
     *  Destination BM neu allozieren
     */
    BM_Width = pd->bmhd.bmh_Width;
    BM_Width16 = (BM_Width + 15) & ~15; /* multiple of 16 */
    BM_Height = pd->bmhd.bmh_Height;
    pd->DestBM=AllocBitMap(BM_Width, BM_Height, DestDepth,
 			  (BMF_CLEAR | BMF_INTERLEAVED | BMF_MINPLANES),
 			  pd->TheScreen->RastPort.BitMap);
    if(!pd->DestBM)
    {
     ReleaseSemaphore(&si->si_Lock);
     return FALSE;;
    }
 
    /*
     *  Do planar->chunky conversion only, if WRITEPIXELARRAY wasn't used,
     *  because WRITEPIXELARRAY fills the ChunkyBuffer with source data
     */
    if(pd->BitmapMode)
    {
     pd->ChunkyBuffer=AllocVec(BM_Width16*BM_Height, MEMF_ANY | MEMF_CLEAR);
     if(!pd->ChunkyBuffer)
     {
      ReleaseSemaphore(&si->si_Lock);
      return(0);
     }
 
     InitRastPort(&SrcRP);
     SrcRP.BitMap=pd->bm;
 
     /*
      *  planare BitMap in einen chunky Buffer auslesen
      */
#ifdef __AROS__
     for(i=0; i<BM_Height; i++)
     {
       /* AROS ReadPixelLine/Array8 does not need a temprp */
       ReadPixelLine8(&SrcRP, 0, i, BM_Width, &pd->ChunkyBuffer[i * BM_Width16], NULL);
     }
     DeinitRastPort(&SrcRP);
#else
     for(i=0; i<BM_Height; i++)
     {
      for(j=0; j<BM_Width; j++)
      {
       pd->ChunkyBuffer[i*BM_Width16+j]=ReadPixel(&SrcRP, j, i);
      }
     }
#endif
    } /* if(pd->BitmapMode) */
     
    /*
     *  ColorMap bestimmen
     */
    if(DestNumColors >= pd->NumColors)
    {
     CopyMem(pd->CRegs, pd->GRegs, pd->NumColors*3*sizeof(ULONG));
    }
    else
    {
     struct HistEntry TheHist[256];
 
     /*
      *  Farben im Histogramm ausfuellen
      */
     for(i=0; i<pd->NumColors; i++)
     {
      TheHist[i].Count=0;
      TheHist[i].Red   = pd->CRegs[i*3+0];
      TheHist[i].Green = pd->CRegs[i*3+1];
      TheHist[i].Blue  = pd->CRegs[i*3+2];
     }
 
     /*
      *  Farbanzahl im Histogramm ermitteln
      */
     
     { 
     	UBYTE *cb = pd->ChunkyBuffer;
     
 	for(i = 0; i < BM_Height;i++)
 	{
 	    for(j = 0; j < BM_Width; j++)
 	    {
 	    	TheHist[cb[j]].Count++;
 	    }
 	    cb += BM_Width16;
 	}
     }
     
     /*
      *  Duplikate im Histogramm ausmerzen
      */
     for(i=0; i<(pd->NumColors-1); i++)
     {
      for(j=i+1; j<pd->NumColors; j++)
      {
       if((TheHist[j].Red == TheHist[i].Red) &&
 	 (TheHist[j].Green == TheHist[i].Green) &&
 	 (TheHist[j].Blue == TheHist[i].Blue))
       {
        TheHist[i].Count+=TheHist[j].Count;
        TheHist[j].Count=0;
       }
      }
     }
 
     /*
      *  Histogramm nach Haeufigkeit sortieren
      */
     qsort((void *) TheHist, pd->NumColors, sizeof(struct HistEntry), HistSort);
 
     /*
      *  Es werden die DestNumColors meistvorhandenen Farben benutzt
      */
     for(i=0; i<DestNumColors; i++)
     {
      pd->GRegs[i*3+0] = TheHist[i].Red;
      pd->GRegs[i*3+1] = TheHist[i].Green;
      pd->GRegs[i*3+2] = TheHist[i].Blue;
     }
    } /* else(DestNumColors >= pd->NumColors) */
 
    /*
     *  Pens fuer GRegs obtainen
     */
    for(i=0; i<DestNumColors; i++)
    {
     pd->ColTable[i]=ObtainBestPen(pd->TheScreen->ViewPort.ColorMap,
 				  pd->GRegs[i*3+0], pd->GRegs[i*3+1], pd->GRegs[i*3+2],
 				  OBP_Precision, pd->Precision,
 				  OBP_FailIfBad, FALSE,
 				  TAG_DONE);
 
     pd->NumAlloc++;
    }
    D(bug("picture.datatype/DTM_ASYNCLAYOUT: NumColors: %ld DestNumColors: %ld NumAlloc: %ld\n", (long)pd->NumColors, (long)DestNumColors, (long)pd->NumAlloc));
 
    /*
     *  Die wirklichen Farben der Pens holen
     */
    for(i=0; i<DestNumColors; i++)
    {
     GetRGB32(pd->TheScreen->ViewPort.ColorMap, pd->ColTable[i], 1, pd->GRegs+(3*i));
    }
 
    /*
     *  SparseTable nach der "Geringster Abstand" Methode bestimmen
     */
    for(i=0; i<pd->NumColors; i++)
    {
     unsigned int Diff, LastDiff;
     short CRed, GRed, CGreen, GGreen, CBlue, GBlue;
 
     LastDiff=0xFFFFFFFF;
 
     CRed   = pd->CRegs[i*3+0]>>17;
     CGreen = pd->CRegs[i*3+1]>>17;
     CBlue  = pd->CRegs[i*3+2]>>17;
 
     for(j=0; j<DestNumColors; j++)
     {
      GRed   = pd->GRegs[j*3+0]>>17;
      GGreen = pd->GRegs[j*3+1]>>17;
      GBlue  = pd->GRegs[j*3+2]>>17;
 
      Diff=abs(CRed - GRed) +
 	  abs(CGreen - GGreen) +
 	  abs(CBlue - GBlue);
 
      if(Diff <= LastDiff)
      {
       pd->SparseTable[i]=pd->ColTable[j];
 
       LastDiff=Diff;
      }
 
      if(LastDiff==0)
      {
       break;
      }
     }
    }
 
    /*
     *  ChunkyBuffer remappen
     */
     
    { 
     	UBYTE *cb = pd->ChunkyBuffer;
     
 	for(i = 0; i < BM_Height;i++)
 	{
 	    for(j = 0; j < BM_Width; j++)
 	    {
 	    	cb[j] = pd->SparseTable[cb[j]];
 	    }
 	    cb += BM_Width16;
 	}
    }
 
    /*
     *  C2P vom ChunkyBuffer auf DestBM
     */
    InitRastPort(&DestRP);
    DestRP.BitMap=pd->DestBM;
 
    WriteChunkyPixels(&DestRP, 0, 0, BM_Width-1, BM_Height-1, pd->ChunkyBuffer, BM_Width16);
 
#ifdef __AROS__
    DeinitRastPort(&DestRP);
#endif
   } /* else(pd->TrueColorSrc) */
   pd->Remapped = TRUE;
  } /* if(pd->Remap) */
  else
  {
   /*
    *  Bild soll nicht remapped werden
    */
   D(bug("picture.datatype/DTM_ASYNCLAYOUT: Remapping disabled\n"));
   if (!pd->bm)
   {
    D(bug("picture.datatype/DTM_ASYNCLAYOUT: No bitmap set !\n"));
    return FALSE;
   }
   pd->DestBM = pd->bm;
  } /* else(pd->Remap) */
 } /* if(msg->gpl_Initial) */

 /*
  *  Wieder entsperren
  */
 ReleaseSemaphore(&si->si_Lock);

#ifdef __AROS__
si->si_VertUnit = 1;
si->si_VisVert = domain->Height;
si->si_TotVert = Height;

si->si_HorizUnit = 1;
si->si_VisHoriz = domain->Width;
si->si_TotHoriz = Width;
#endif

 NotifyAttrChanges((Object *) g, msg->gpl_GInfo, NULL,
				 GA_ID, g->GadgetID,

				 DTA_VisibleVert, domain->Height,
				 DTA_TotalVert, Height,
				 DTA_NominalVert, Height,
				 DTA_VertUnit, 1,

				 DTA_VisibleHoriz, domain->Width,
				 DTA_TotalHoriz, Width,
				 DTA_NominalHoriz, Width,
				 DTA_HorizUnit, 1,

				 DTA_Title, (IPTR) Title,
				 DTA_Busy, TRUE,
				 DTA_Sync, TRUE,
				 TAG_DONE);

 return(RetVal);
}

/**************************************************************************************************/

STATIC IPTR PDT_WritePixelArray(struct IClass *cl, struct Gadget *g, struct pdtBlitPixelArray *msg)
{
    struct Picture_Data *pd;
    struct DTSpecialInfo *si;
   
    long line, lines, numbytes;
    int pixelformat;
    int pixelsize;
    APTR srcstart;
    APTR deststart;
    long srcmod;
    long destmod;
   
    pd = (struct Picture_Data *) INST_DATA(cl, g);
    si = (struct DTSpecialInfo *) g->SpecialInfo;

    if( !pd->bmhd.bmh_Width || !pd->bmhd.bmh_Height || !pd->bmhd.bmh_Depth )
    {
        D(bug("picture.datatype/DTM_WRITEPIXELARRAY: BitMapHeader not set !\n"));
        return FALSE;
    }
    
    /* Do some checks first */
    pixelformat = (long)msg->pbpa_PixelFormat;
    if ( pixelformat != pd->PixelFormat )
    {
        if( pd->BitmapMode )
        {
            /* Initial call: Set new pixel format and allocate Chunky or RGB buffer */
            pd->BitmapMode = FALSE;
            pd->PixelFormat = pixelformat;
            switch(pixelformat)
            {
                case PBPAFMT_LUT8:
                case PBPAFMT_GREY8:
                    pixelsize = 1;
                    break;
                case PBPAFMT_RGB:
                    pixelsize = 3;
                    break;
                case PBPAFMT_RGBA:
                case PBPAFMT_ARGB:
                    pixelsize = 4;
                    break;
                default:
                    D(bug("picture.datatype/DTM_WRITEPIXELARRAY: Unknown PixelFormat mode !\n"));
                    return FALSE;
            }
            pd->PixelSize = pixelsize;
            pd->CBWidth = pd->bmhd.bmh_Width;
            pd->CBWidthBytes = (pd->CBWidth * pixelsize + 15) & ~15; /* multiple of 16 */
            pd->CBHeight = pd->bmhd.bmh_Height;
            pd->ChunkyBuffer = AllocVec(pd->CBWidthBytes * pd->CBHeight, MEMF_ANY | MEMF_CLEAR);
#if 0 /* fill chunky buffer with something colorful, works only with PBPAFMT_RGB */
	    if( pixelformat == PBPAFMT_RGB )
	    {
		long x, y;
		long Width = pd->CBWidth;
		long WidthBytes = pd->CBWidthBytes;
		long Height = pd->CBHeight;

		for (y=0; y<Height; y++)
		{
		    for (x=0; x<Width; x++)
		    {
			pd->ChunkyBuffer[x*pixelsize+y*WidthBytes+0] = x*256/Width;
			pd->ChunkyBuffer[x*pixelsize+y*WidthBytes+1] = y*256/Height;
			pd->ChunkyBuffer[x*pixelsize+y*WidthBytes+2] = ((Width-x)*256/Width+(Height-y)*256/Height)/2;
		    }
		}
	    }
#endif
            if(!pd->ChunkyBuffer)
            {
                D(bug("picture.datatype/DTM_WRITEPIXELARRAY: Chunky buffer allocation failed !\n"));
                return FALSE;
            }
            D(bug("picture.datatype/DTM_WRITEPIXELARRAY: Initialized ChunkyBuffer 0x%lx, PixelFormat %ld\n", (long)pd->ChunkyBuffer, pd->PixelFormat));
            D(bug("picture.datatype/DTM_WRITEPIXELARRAY: Width %ld WidthBytes %ld Height %ld\n", pd->CBWidth, pd->CBWidthBytes, pd->CBHeight));
        }
        else
        {
            D(bug("picture.datatype/DTM_WRITEPIXELARRAY: PixelFormat mismatch !\n"));
            return FALSE;
        }
    }

    /* Now copy the new source data to the ChunkyBuffer line by line */
    pixelsize = pd->PixelSize;
    srcmod = msg->pbpa_PixelArrayMod;
    srcstart = msg->pbpa_PixelData;
    destmod = pd->CBWidthBytes;
    deststart = pd->ChunkyBuffer + msg->pbpa_Left * pixelsize + msg->pbpa_Top * destmod;
    lines = msg->pbpa_Height;
    numbytes = msg->pbpa_Width * pixelsize;
    
    for( line=0; line<lines; line++ )
    {
	D(bug("picture.datatype/DTM_WRITEPIXELARRAY: src 0x%lx dest 0x%lx bytes %ld\n", (long)srcstart, (long)deststart, numbytes));
        CopyMem((APTR) srcstart, (APTR) deststart, numbytes);
        srcstart += srcmod;
        deststart += destmod;
    }
    return TRUE;
}

/**************************************************************************************************/

#if 0
STATIC IPTR DT_FrameBox(struct IClass *cl, struct Gadget *g, struct dtFrameBox *msg)
{
 struct Picture_Data *pd;
 ULONG Width, Height, Depth;
 IPTR RetVal;

 pd=(struct Picture_Data *) INST_DATA(cl, g);

 RetVal=0;

 Width=pd->bmhd.bmh_Width;
 Height=pd->bmhd.bmh_Height;
 Depth=pd->bmhd.bmh_Depth;

 D(bug("picture.datatype/DTM_FRAMEBOX: Width %ld\n", (long) Width));
 D(bug("picture.datatype/DTM_FRAMEBOX: Height %ld\n", (long) Height));
 D(bug("picture.datatype/DTM_FRAMEBOX: Depth %ld\n", (long) Depth));

 if(msg->dtf_FrameInfo)
 {
  msg->dtf_FrameInfo->fri_Dimensions.Height = Height;
  msg->dtf_FrameInfo->fri_Dimensions.Width  = Width;
  msg->dtf_FrameInfo->fri_Dimensions.Depth  = Depth;
  msg->dtf_FrameInfo->fri_Flags             = FIF_SCROLLABLE;

  RetVal = 1;
 }

 return(RetVal);
}
#endif


/**************************************************************************************************/

#ifdef __AROS__
AROS_UFH3S(IPTR, DT_Dispatcher,
	   AROS_UFHA(Class *, cl, A0),
	   AROS_UFHA(Object *, o, A2),
	   AROS_UFHA(Msg, msg, A1))
{
    AROS_USERFUNC_INIT
#else
ASM ULONG DT_Dispatcher(register __a0 struct IClass *cl, register __a2 Object *o, register __a1 Msg msg)
{
#endif
    IPTR RetVal;
#ifdef MYDEBUG
    register int i;
    int Known;
   
    Known=FALSE;
#endif /* MYDEBUG */
   
    putreg(REG_A4, (long) cl->cl_Dispatcher.h_SubEntry);   /* Small Data */
   
    RetVal=(IPTR) 0;
   
    switch(msg->MethodID)
    {
        case OM_NEW:
        {
            D(bug("picture.datatype/DT_Dispatcher: Method OM_NEW\n"));
            RetVal=(IPTR) DT_NewMethod(cl, o, (struct opSet *) msg);
            break;
        }
      
        case OM_GET:
        {
            D(bug("picture.datatype/DT_Dispatcher: Method OM_GET\n"));
            RetVal=(IPTR) DT_GetMethod(cl, (struct Gadget *) o, (struct opGet *) msg);
            break;
        }
      
        case OM_SET:
        case OM_UPDATE:
        {
            D(bug("picture.datatype/DT_Dispatcher: Method %s\n", (msg->MethodID==OM_UPDATE) ? "OM_UPDATE" : "OM_SET"));
            RetVal=(IPTR) DT_SetMethod(cl, (struct Gadget *) o, (struct opSet *) msg);
            break;
        }
      
        case OM_DISPOSE:
        {
            D(bug("picture.datatype/DT_Dispatcher: Method OM_DISPOSE\n"));
            RetVal=(IPTR) DT_DisposeMethod(cl, o, (Msg) msg);
            break;
        }
      
        case GM_LAYOUT:
        {
            D(bug("picture.datatype/DT_Dispatcher: Method GM_LAYOUT\n"));
            RetVal=(IPTR) DT_Layout(cl, (struct Gadget *) o, (struct gpLayout *) msg);
            break;
        }
      
        case DTM_PROCLAYOUT:
        {
            D(bug("picture.datatype/DT_Dispatcher: Method DTM_PROCLAYOUT\n"));
            RetVal=(IPTR) DT_ProcLayout(cl, (struct Gadget *) o, (struct gpLayout *) msg);
            /*
             *  Yes, here is no break!
             */
        }
      
        case DTM_ASYNCLAYOUT:
        {
            D(bug("picture.datatype/DT_Dispatcher: Method DTM_ASYNCLAYOUT\n"));
            RetVal=(IPTR) DT_AsyncLayout(cl, (struct Gadget *) o, (struct gpLayout *) msg);
            break;
        }
      
        case GM_RENDER:
        {
            D(bug("picture.datatype/DT_Dispatcher: Method GM_RENDER\n"));
            RetVal=(IPTR) DT_Render(cl, (struct Gadget *) o, (struct gpRender *) msg);
            break;
        }
      
#if 0
        case DTM_FRAMEBOX:
        {
            D(bug("picture.datatype/DT_Dispatcher: Method DTM_FRAMEBOX\n"));
            RetVal=(IPTR) DT_FrameBox(cl, (struct Gadget *) o, (struct dtFrameBox *) msg);
            break;
        }
#endif
      
        case PDTM_WRITEPIXELARRAY:
        {
            // D(bug("picture.datatype/DT_Dispatcher: Method PDTM_WRITEPIXELARRAY\n"));
            RetVal=(IPTR) PDT_WritePixelArray(cl, (struct Gadget *) o, (struct pdtBlitPixelArray *) msg);
            break;
        }
      
        default:
        {
#ifdef MYDEBUG
            for(i=0; i<NumMethods; i++)
            {
             if(msg->MethodID==KnownMethods[i])
             {
              Known=TRUE;
         
              D(bug("picture.datatype/DT_Dispatcher: Method %s\n", MethodNames[i]));
             }
            }
         
            if(!Known)
            {
             D(bug("picture.datatype/DT_Dispatcher: Method 0x%lx\n", (unsigned long) msg->MethodID));
            }
#endif /* MYDEBUG */
            RetVal=DoSuperMethodA(cl, o, (Msg) msg);
            break;
        }
    }
       
    return(RetVal);

#ifdef __AROS__
    AROS_USERFUNC_EXIT
#endif
}

/**************************************************************************************************/

struct IClass *DT_MakeClass(struct Library *picturebase)
{
    struct IClass *cl = MakeClass("picture.datatype", DATATYPESCLASS, NULL, sizeof(struct Picture_Data), NULL);

    if (cl)
    {
#ifdef __AROS__
	cl->cl_Dispatcher.h_Entry = (HOOKFUNC) AROS_ASMSYMNAME(DT_Dispatcher);
#else
	cl->cl_Dispatcher.h_Entry = (HOOKFUNC) DT_Dispatcher;
#endif
	cl->cl_Dispatcher.h_SubEntry = (HOOKFUNC) getreg(REG_A4);
	cl->cl_UserData = (IPTR) picturebase;  /* Required by datatypes */
    }

    return cl;
}

/**************************************************************************************************/
