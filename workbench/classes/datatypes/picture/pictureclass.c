/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/* Supported Attributes (Init (new), Set, Get)
I.. ULONG                 Precision;
ISG ULONG                 ModeID;
..G struct BitMapHeader   bmhd;
ISG struct BitMap        *SrcBM;
ISG struct BitMap        *ClassBM;
..G ULONG                 CRegs[768];
..G struct BitMap        *DestBM;
..G ULONG                 GRegs[768];
IS. struct Screen        *DestScreen;
..G struct ColorRegister  ColMap[256];
..G UBYTE                 ColTable[256];
..G UBYTE                 ColTable2[256];
ISG UWORD                 NumColors;
..G UWORD                 NumAlloc;
..G ULONG                 Allocated;
I.. UBYTE                 SparseTable[256];
I.. UWORD                 NumSparse;
ISG Point                 Grab;
ISG BOOL                  FreeSource;
I.. BOOL                  Remap;
ISG BOOL                  UseFriendBM;
ISG BOOL                  DestMode
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
#include "prefs.h"
#include "colorhandling.h"

#include "methods.h"
#define PDTA_ScaleQuality 12345

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

    /* initialize our class structure with zeros (or FALSE for BOOL's) */
    pd = (struct Picture_Data *) INST_DATA(cl, g);
    memset(pd, 0, sizeof(struct Picture_Data));

    /* initialize some non-zero values */
    pd->Precision = PRECISION_IMAGE;
    pd->ModeID = INVALID_ID;
    pd->Remap = TRUE;
    pd->SrcPixelFormat = -1;
    pd->DitherQuality = 4;
    pd->UseFriendBM = 1;

    /* Prefs overrides default, but application overrides Prefs */
    ReadPrefs(pd);

    while((ti=NextTagItem(&attrs)))
    {
        switch (ti->ti_Tag)
        {
            case OBP_Precision:
                pd->Precision = ti->ti_Data;
                D(bug("picture.datatype/OM_NEW: Tag ID OBP_Precision: %ld\n", (long)pd->Precision));
                break;

            case PDTA_Remap:
                pd->Remap = ti->ti_Data;
                D(bug("picture.datatype/OM_NEW: Tag ID PDTA_Remap: %ld\n", (long)pd->Remap));
                break;

            case PDTA_NumSparse:
                pd->NumSparse = ti->ti_Data;
                D(bug("picture.datatype/OM_NEW: Tag ID PDTA_NumSparse: %ld\n", (long)pd->NumSparse));
                break;

            case PDTA_SparseTable:
                D(bug("picture.datatype/OM_NEW: Tag ID PDTA_SparseTable\n"));
                if(!(pd->NumSparse && ti->ti_Data))
                {
                    break;
                }
                CopyMem((APTR) ti->ti_Data, (APTR) pd->SparseTable, pd->NumSparse);
                break;
        }
    }

    D(bug("picture.datatype/OM_NEW: Setting attributes\n"));
    DT_SetMethod(cl, g, msg);

    return g;
}

/**************************************************************************************************/

STATIC IPTR DT_DisposeMethod(struct IClass *cl, Object *o, Msg msg)
{
    struct Picture_Data *pd;
    IPTR RetVal;

    RetVal = 1;

    pd = (struct Picture_Data *) INST_DATA(cl, o);

    if(pd)
    {
	FreeDest(pd);
	FreeSource(pd);
    }

    RetVal += DoSuperMethodA(cl, o, msg);

    return RetVal;
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
                pd->ModeID = ti->ti_Data;
                D(bug("picture.datatype/OM_SET: Tag PDTA_ModeID: 0x%lx\n", (long)pd->ModeID));
                break;

            case PDTA_ClassBitMap:
                pd->KeepSrcBM = TRUE;
	        D(bug("picture.datatype/OM_GET: Tag PDTA_ClassBitMap: Handled as PDTA_BitMap\n"));
            case PDTA_BitMap:
                pd->SrcBM = (struct BitMap *) ti->ti_Data;
                D(bug("picture.datatype/OM_SET: Tag PDTA_BitMap: 0x%lx\n", (long)pd->SrcBM));
                break;

            case PDTA_Screen:
                pd->DestScreen = (struct Screen *) ti->ti_Data;
                D(bug("picture.datatype/OM_SET: Tag PDTA_Screen: 0x%lx\n", (long)pd->DestScreen));
                break;

            case PDTA_NumColors:
                pd->NumColors = ti->ti_Data;
                D(bug("picture.datatype/OM_SET: Tag PDTA_NumColors: %ld\n", (long)pd->NumColors));
                break;

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

	    case PDTA_SourceMode:
                D(bug("picture.datatype/OM_SET: Tag PDTA_SourceMode (ignored): %ld\n", (long)ti->ti_Data));
	        break;

	    case PDTA_DestMode:
		pd->DestMode = (BOOL) ti->ti_Data;
                D(bug("picture.datatype/OM_SET: Tag PDTA_DestMode: %ld\n", (long)pd->DestMode));
	        break;

            case PDTA_FreeSourceBitMap:
                pd->FreeSource = (BOOL) ti->ti_Data;
                D(bug("picture.datatype/OM_SET: Tag PDTA_FreeSourceBitMap: %ld\n", (long)pd->FreeSource));
                break;

	    case PDTA_UseFriendBitMap:
                pd->UseFriendBM = (BOOL) ti->ti_Data;
                D(bug("picture.datatype/OM_SET: Tag PDTA_UseFriendBitMap: %ld\n", (long)pd->UseFriendBM));
	        break;

	    case PDTA_MaxDitherPens:
                pd->MaxDitherPens = (UWORD) ti->ti_Data;
                D(bug("picture.datatype/OM_SET: Tag PDTA_MaxDitherPens: %ld\n", (long)pd->MaxDitherPens));
	        break;

	    case PDTA_DitherQuality:
                pd->DitherQuality = (UWORD) ti->ti_Data;
                D(bug("picture.datatype/OM_SET: Tag PDTA_DitherQuality: %ld\n", (long)pd->DitherQuality));
	        break;

	    case PDTA_ScaleQuality:
                pd->ScaleQuality = (UWORD) ti->ti_Data;
                D(bug("picture.datatype/OM_SET: Tag PDTA_ScaleQuality: %ld\n", (long)pd->ScaleQuality));
	        break;

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
	    D(bug("picture.datatype/OM_GET: Tag PDTA_ModeID: 0x%lx\n", (long)pd->ModeID));
	    *(msg->opg_Storage)=pd->ModeID;
	    break;

	case PDTA_BitMapHeader:
	    D(bug("picture.datatype/OM_GET: Tag PDTA_BitMapHeader: 0x%lx\n", (long)&pd->bmhd));
	    *(msg->opg_Storage)=(ULONG) &pd->bmhd;
	    break;

	case PDTA_ClassBitMap:
	    D(bug("picture.datatype/OM_GET: Tag PDTA_ClassBitMap: Handled as PDTA_BitMap\n"));
	case PDTA_BitMap:
	    if( !pd->SrcBM )
		ConvertChunky2Bitmap( pd );
	    D(bug("picture.datatype/OM_GET: Tag PDTA_BitMap: 0x%lx\n", (long)pd->SrcBM));
	    *(msg->opg_Storage)=(ULONG) pd->SrcBM;
	    break;

	case PDTA_DestBitMap:
	    D(bug("picture.datatype/OM_GET: Tag PDTA_DestBitMap: 0x%lx\n", (long)pd->DestBM));
	    *(msg->opg_Storage)=(ULONG) pd->DestBM;
	    break;

	case PDTA_MaskPlane:
	    CreateMaskPlane( pd );
	    D(bug("picture.datatype/OM_GET: Tag PDTA_MaskPlane: 0x%lx\n", (long)pd->MaskPlane));
	    *(msg->opg_Storage)=(ULONG) pd->MaskPlane;
	    break;

	case PDTA_Screen:
	    D(bug("picture.datatype/OM_GET: Tag PDTA_Screen: 0x%lx\n", (long)pd->DestScreen));
	    *(msg->opg_Storage)=(ULONG) pd->DestScreen;
	    break;

	case PDTA_ColorRegisters:
	    D(bug("picture.datatype/OM_GET: Tag PDTA_ColorRegisters: 0x%lx\n", (long)&pd->ColMap));
	    *(msg->opg_Storage)=(ULONG) &pd->ColMap;
	    break;

	case PDTA_CRegs:
	    D(bug("picture.datatype/OM_GET: Tag PDTA_CRegs: 0x%lx\n", (long)&pd->SrcColRegs));
	    *(msg->opg_Storage)=(ULONG) &pd->SrcColRegs;
	    break;

	case PDTA_GRegs:
	    D(bug("picture.datatype/OM_GET: Tag PDTA_GRegs: 0x%lx\n", (long)&pd->DestColRegs));
	    *(msg->opg_Storage)=(ULONG) &pd->DestColRegs;
	    break;

	case PDTA_AllocatedPens:
	    D(bug("picture.datatype/OM_GET: Tag PDTA_AllocatedPens: Handled by PDTA_ColorTable2\n"));
	case PDTA_ColorTable2:
	    D(bug("picture.datatype/OM_GET: Tag PDTA_ColorTable2: Handled by PDTA_ColorTable\n"));
	case PDTA_ColorTable:
	    D(bug("picture.datatype/OM_GET: Tag PDTA_ColorTable: 0x%lx\n", (long)&pd->ColTable));
	    *(msg->opg_Storage)=(ULONG) &pd->ColTable;
	    break;

	case PDTA_NumColors:
	    D(bug("picture.datatype/OM_GET: Tag PDTA_NumColors: %ld\n", (long)pd->NumColors));
	    *(msg->opg_Storage)=(ULONG) pd->NumColors;
	    break;

	case PDTA_NumAlloc:
	    D(bug("picture.datatype/OM_GET: Tag PDTA_NumAlloc: %ld\n", (long)pd->NumAlloc));
	    *(msg->opg_Storage)=(ULONG) pd->NumAlloc;
	    break;

	case PDTA_Grab:
	    D(bug("picture.datatype/OM_GET: Tag PDTA_Grab: 0x%lx\n", (long)&pd->Grab));
	    *(msg->opg_Storage)=(ULONG) &pd->Grab;
	    break;

	case PDTA_SourceMode:
	    D(bug("picture.datatype/OM_GET: Tag PDTA_SourceMode: 0x%lx\n", (long)PMODE_V43));
	    *(msg->opg_Storage)=(ULONG) PMODE_V43;
	    break;

	case PDTA_DestMode:
	    D(bug("picture.datatype/OM_GET: Tag PDTA_DestMode: 0x%lx\n", (long)pd->DestMode));
	    *(msg->opg_Storage)=(ULONG) pd->DestMode;
	    break;

	case PDTA_FreeSourceBitMap:
	    D(bug("picture.datatype/OM_GET: Tag PDTA_FreeSourceBitMap: 0x%lx\n", (long)pd->FreeSource));
	    *(msg->opg_Storage)=(ULONG) pd->FreeSource;
	    break;

	case PDTA_UseFriendBitMap:
	    D(bug("picture.datatype/OM_GET: Tag PDTA_UseFriendBitMap: 0x%lx\n", (long)pd->UseFriendBM));
	    *(msg->opg_Storage)=(ULONG) pd->UseFriendBM;
	    break;

	case PDTA_MaxDitherPens:
	    D(bug("picture.datatype/OM_GET: Tag PDTA_MaxDitherPens: 0x%lx\n", (long)pd->MaxDitherPens));
	    *(msg->opg_Storage)=(ULONG) pd->MaxDitherPens;
	    break;

	case PDTA_DitherQuality:
	    D(bug("picture.datatype/OM_GET: Tag PDTA_DitherQuality: 0x%lx\n", (long)pd->DitherQuality));
	    *(msg->opg_Storage)=(ULONG) pd->DitherQuality;
	    break;

	case PDTA_ScaleQuality:
	    D(bug("picture.datatype/OM_GET: Tag PDTA_ScaleQuality: 0x%lx\n", (long)pd->ScaleQuality));
	    *(msg->opg_Storage)=(ULONG) pd->ScaleQuality;
	    break;

	case DTA_Methods:
	    D(bug("picture.datatype/OM_GET: Tag DTA_Methods: 0x%lx\n", (long)SupportedMethods));
	    *(msg->opg_Storage)=(ULONG) SupportedMethods;
	    break;

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

    if(!pd->Layouted)
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

    D(bug("picture.datatype/GM_RENDER: Domain: left %ld top %ld width %ld height %ld\n", domain->Left, domain->Top, domain->Width, domain->Height));
    D(bug("picture.datatype/GM_RENDER: TopHoriz %ld TopVert %ld Width %ld Height %ld\n", (long)TopHoriz, (long)TopVert, (long)NominalWidth, (long)NominalHeight));

#if 0
    ObtainSemaphore(&(si->si_Lock));
#endif

    SrcX = TopHoriz;
    SrcY = TopVert;
    DestX = domain->Left;
    DestY = domain->Top;

    SizeX = MIN(NominalWidth - TopHoriz, domain->Width);
    SizeY = MIN(NominalHeight - TopVert, domain->Height);
    D(bug("picture.datatype/GM_RENDER: Size X %ld Y %ld\n", SizeX, SizeY));

    if( pd->DestBM )
    {
        BltBitMapRastPort( pd->DestBM,
                          SrcX,
                          SrcY,
                          msg->gpr_RPort,
                          DestX,
                          DestY,
                          SizeX,
                          SizeY,
                          0xC0);
    }
    else /* if(pd->DestBuffer) || if(pd->DestBM) */
    {
        D(bug("picture.datatype/GM_RENDER: No destination picture present !\n"));
        return FALSE;
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
    struct Picture_Data *pd;
    struct DTSpecialInfo *si;
    ULONG SrcWidth, SrcHeight;
    unsigned int SrcDepth;
    BOOL success;

    pd = (struct Picture_Data *) INST_DATA(cl, g);
    si = (struct DTSpecialInfo *) g->SpecialInfo;

    SrcWidth = pd->bmhd.bmh_Width;
    SrcHeight = pd->bmhd.bmh_Height;
    SrcDepth = pd->bmhd.bmh_Depth;
    D(bug("picture.datatype/DTM_ASYNCLAYOUT: Source Width %ld Height %ld Depth %ld\n", SrcWidth, SrcHeight, (long)SrcDepth));
    D(bug("picture.datatype/DTM_ASYNCLAYOUT: Masking %d Transparent %d\n", (int)pd->bmhd.bmh_Masking, (int)pd->bmhd.bmh_Transparent));
    if( !SrcWidth || !SrcHeight || !SrcDepth )
    {
        D(bug("picture.datatype/DTM_ASYNCLAYOUT: Neccessary fields in BitMapHeader not set !\n"));
        return FALSE;
    }

    if( !pd->SrcBuffer && !pd->SrcBM )
    {
        D(bug("picture.datatype/DTM_ASYNCLAYOUT: No source picture given !\n"));
        return FALSE;
    }

    ObtainSemaphore( &(si->si_Lock) );   /* lock object data */

    success = TRUE;
    if( msg->gpl_Initial )   /* we need to do it just once */
    {
        /* determine destination screen depth */
        if( !pd->DestScreen )
        {
            pd->DestScreen = msg->gpl_GInfo->gi_Screen;
        }
        pd->DestDepth = GetBitMapAttr( pd->DestScreen->RastPort.BitMap, BMA_DEPTH );
        if( pd->DestDepth > 8 )
	{
            pd->TrueColorDest = TRUE;
	    if( pd->UseCM )
	    {
		D(bug("picture.datatype/DTM_ASYNCLAYOUT: Forcing colormapped dest depth of 8 instead of %ld\n", (long)pd->DestDepth));
		pd->DestDepth = 8;
		pd->TrueColorDest = FALSE;
	    }
	}
        else
	{
            pd->TrueColorDest = FALSE;
	}
        D(bug("picture.datatype/DTM_ASYNCLAYOUT: Destination Depth %ld\n", (long)pd->DestDepth));

	FreeDest( pd );
	if( !AllocDestBM( pd, SrcWidth, SrcHeight, pd->DestDepth ) )
	{
	    ReleaseSemaphore(&si->si_Lock);   /* unlock object data */
	    return FALSE;
	}

	if( pd->TrueColorSrc )
        {
            if( !pd->SrcBuffer )
            {
                D(bug("picture.datatype/DTM_ASYNCLAYOUT: Bitmap source only possible with up to 8 bits !\n"));
                ReleaseSemaphore(&si->si_Lock);   /* unlock object data */
                return FALSE;
            }
            if( pd->TrueColorDest )
            {
                success = ConvertTC2TC( pd );
            }
            else
            {
                success = ConvertTC2CM( pd );
            }
        }
        else /* if(pd->TrueColorSrc) */
        {
	    if( !pd->SrcBuffer )
	    {
		if( !ConvertBitmap2Chunky( pd ) )
		{
		    ReleaseSemaphore(&si->si_Lock);   /* unlock object data */
		    return FALSE;
		}
	    }
            if( pd->TrueColorDest )
            {
                success = ConvertCM2TC( pd );
            }
            else
            {
                success = ConvertCM2CM( pd );
            }
        } /* else(pd->TrueColorSrc) */
	if( pd->FreeSource )
	{
	    CreateMaskPlane( pd );
	    FreeSource( pd );
	}
        pd->Layouted = TRUE;
        D(bug("picture.datatype/DTM_ASYNCLAYOUT: Initial layout done\n"));
    } /* if(msg->gpl_Initial) */

    ReleaseSemaphore( &si->si_Lock );   /* unlock object data */
    if( !success )
    {
        D(bug("picture.datatype/DTM_ASYNCLAYOUT: Layout failed during remapping !\n"));
	return FALSE;
    }

    {
	struct IBox *domain;
	IPTR Width, Height;
	STRPTR Title;
    
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
    }
    return TRUE;
}

/**************************************************************************************************/

STATIC IPTR PDT_WritePixelArray(struct IClass *cl, struct Gadget *g, struct pdtBlitPixelArray *msg)
{
    struct Picture_Data *pd;
    struct DTSpecialInfo *si;

    int pixelformat;
    int pixelbytes;

    pd = (struct Picture_Data *) INST_DATA(cl, g);
    si = (struct DTSpecialInfo *) g->SpecialInfo;

    /* Do some checks first */
    pixelformat = (long)msg->pbpa_PixelFormat;
    if ( pixelformat != pd->SrcPixelFormat )	/* This also checks for pd->SrcBuffer */
    {
        if( !pd->SrcBuffer )
        {
            /* Initial call: Set new pixel format and allocate Chunky or RGB buffer */
	    if( pd->SrcBM )
	    {
		D(bug("picture.datatype/DTM_WRITEPIXELARRAY: Not possible in bitmap mode !\n"));
		return FALSE;
	    }
	    if( !pd->bmhd.bmh_Width || !pd->bmhd.bmh_Height || !pd->bmhd.bmh_Depth )
	    {
		D(bug("picture.datatype/DTM_WRITEPIXELARRAY: BitMapHeader not set !\n"));
		return FALSE;
	    }

            switch( pixelformat )
            {
                case PBPAFMT_GREY8:
		    InitGreyColTable( pd );
                    pixelbytes = 1;
                    break;
                case PBPAFMT_LUT8:
                    pixelbytes = 1;
                    break;
                case PBPAFMT_RGB:
		    InitRGBColTable( pd );
                    pixelbytes = 3;
		    pd->TrueColorSrc = TRUE;
                    break;
                case PBPAFMT_ARGB:
		    InitRGBColTable( pd );
                    pixelbytes = 4;
		    pd->TrueColorSrc = TRUE;
                    break;
                default:
                    D(bug("picture.datatype/DTM_WRITEPIXELARRAY: Unknown PixelFormat mode %d !\n", pixelformat));
                    return FALSE;
            }
	    if( !AllocSrcBuffer( pd, pd->bmhd.bmh_Width, pd->bmhd.bmh_Height, pixelformat, pixelbytes ) )
		return FALSE;
            D(bug("picture.datatype/DTM_WRITEPIXELARRAY: Initialized SrcBuffer 0x%lx, PixelFormat %ld\n", (long)pd->SrcBuffer, pd->SrcPixelFormat));
            D(bug("picture.datatype/DTM_WRITEPIXELARRAY: Width %ld WidthBytes %ld Height %ld\n", pd->SrcWidth, pd->SrcWidthBytes, pd->SrcHeight));

#if 0 /* fill chunky buffer with something colorful, works only with PBPAFMT_RGB */
	    if( pixelformat == PBPAFMT_RGB )
	    {
		long x, y;
		long Width = pd->SrcWidth;
		long WidthBytes = pd->SrcWidthBytes;
		long Height = pd->SrcHeight;

		for (y=0; y<Height; y++)
		{
		    for (x=0; x<Width; x++)
		    {
			pd->SrcBuffer[x*pixelbytes+y*WidthBytes+0] = x*256/Width;
			pd->SrcBuffer[x*pixelbytes+y*WidthBytes+1] = y*256/Height;
			pd->SrcBuffer[x*pixelbytes+y*WidthBytes+2] =
			    ( (Width-x)*256/Width + (Height-y)*256/Height )/2;
		    }
		}
	    }
#endif

        }
        else /* if(!pd->SrcBuffer) */
        {
            D(bug("picture.datatype/DTM_WRITEPIXELARRAY: PixelFormat mismatch !\n"));
            return FALSE;
        }
    } /* if(pixelformat != pd->SrcPixelFormat) */

    {
        long line, lines;
        APTR srcstart;
        APTR deststart;
        long srcwidth, numbytes;
        long srcmod, destmod;

        /* Now copy the new source data to the ChunkyBuffer line by line */
        pixelbytes = pd->SrcPixelBytes;
        srcmod = msg->pbpa_PixelArrayMod;
        srcstart = msg->pbpa_PixelData;
        srcwidth = msg->pbpa_Width;
        destmod = pd->SrcWidthBytes;
        deststart = pd->SrcBuffer + msg->pbpa_Left * pixelbytes + msg->pbpa_Top * destmod;
        lines = msg->pbpa_Height;
        numbytes = srcwidth * pixelbytes;

	/* simply copy data */
	for( line=0; line<lines; line++ )
	{
	    // D(bug("picture.datatype/DTM_WRITEPIXELARRAY: COPY src 0x%lx dest 0x%lx bytes %ld\n", (long)srcstart, (long)deststart, numbytes));
	    CopyMem((APTR) srcstart, (APTR) deststart, numbytes);
	    srcstart += srcmod;
	    deststart += destmod;
	}
    }
    return TRUE;
}

/**************************************************************************************************/

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
            // D(bug("picture.datatype/DT_Dispatcher: Method OM_GET\n"));
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

        case DTM_FRAMEBOX:
        {
            D(bug("picture.datatype/DT_Dispatcher: Method DTM_FRAMEBOX\n"));
            RetVal=(IPTR) DT_FrameBox(cl, (struct Gadget *) o, (struct dtFrameBox *) msg);
            break;
        }

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
