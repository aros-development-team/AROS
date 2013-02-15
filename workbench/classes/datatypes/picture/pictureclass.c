/*
    Copyright � 1995-2012, The AROS Development Team. All rights reserved.
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

#ifndef __AROS__
#include "compilerspecific.h"
#else
#ifdef STATIC
#undef STATIC
#endif
#define STATIC
#endif
#include "debug.h"
#include "pictureclass.h"
#include "prefs.h"
#include "colorhandling.h"

#ifdef MYDEBUG
#include "methods.h"
#endif

#define DGS(x) D(x)
//#define DGS(x)

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

    DTM_OBTAINDRAWINFO,
    DTM_DRAW,
    DTM_RELEASEDRAWINFO,

//    DTM_SELECT,
//    DTM_CLEARSELECTED,
//    DTM_COPY,
    DTM_PRINT,
//    DTM_WRITE,

    PDTM_WRITEPIXELARRAY,
    PDTM_READPIXELARRAY,
    PDTM_SCALE,

    (~0)
};

/**************************************************************************************************/

STATIC IPTR DT_SetMethod(struct IClass *cl, struct Gadget *g, struct opSet *msg);

/**************************************************************************************************/

IPTR NotifyAttrChanges(Object * o, VOID * ginfo, ULONG flags, Tag tag1, ...)
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = DoMethod(o, OM_NOTIFY, AROS_SLOWSTACKTAGS_ARG(tag1), (IPTR) ginfo, flags);
    AROS_SLOWSTACKTAGS_POST
}

/**************************************************************************************************/

STATIC struct Gadget *DT_NewMethod(struct IClass *cl, Object *o, struct opSet *msg)
{
    struct Gadget *g;
    struct TagItem *attrs = msg->ops_AttrList;
    struct TagItem *ti;
    struct Picture_Data *pd;

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
    pd->DitherQuality = 0;
    pd->UseFriendBM = 1;
    pd->DestMode = 1;	/* needs to be changed to FALSE after Multiview adaptation */

    /* Prefs overrides default, but application overrides Prefs */
    ReadPrefs(pd);
    while((ti=NextTagItem(&attrs)))
    {
        switch (ti->ti_Tag)
        {
            case OBP_Precision:
                pd->Precision = (ULONG) ti->ti_Data;
                DGS(bug("picture.datatype/OM_NEW: Tag ID OBP_Precision: %ld\n", (long)pd->Precision));
                break;

            case PDTA_Remap:
                pd->Remap = (BOOL) ti->ti_Data;
                DGS(bug("picture.datatype/OM_NEW: Tag ID PDTA_Remap: %ld\n", (long)pd->Remap));
                break;

            case PDTA_NumSparse:
                pd->NumSparse = (UWORD) ti->ti_Data;
                DGS(bug("picture.datatype/OM_NEW: Tag ID PDTA_NumSparse: %ld\n", (long)pd->NumSparse));
                break;

            case PDTA_SparseTable:
                DGS(bug("picture.datatype/OM_NEW: Tag ID PDTA_SparseTable\n"));
                if(!(pd->NumSparse && ti->ti_Data))
                {
                    break;
                }
                CopyMem((APTR) ti->ti_Data, (APTR) pd->SparseTable, pd->NumSparse);
                break;

#ifdef __AROS__
            case PDTA_DelayRead:
            	if(!pd->NoDelay)
                    pd->DelayRead = (BOOL) ti->ti_Data;
                DGS(bug("picture.datatype/OM_NEW: Tag ID PDTA_DelayRead: %ld\n", (long)pd->DelayRead));
                break;
#endif
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
    struct TagItem *tl = msg->ops_AttrList;
    struct TagItem *ti;
    IPTR RetVal;
    struct RastPort *rp;

    pd=(struct Picture_Data *) INST_DATA(cl, g);
    RetVal=0;

    while((ti=NextTagItem(&tl)))
    {
        switch (ti->ti_Tag)
        {
	    case DTA_VisibleHoriz:
	    case DTA_VisibleVert:
	    	RetVal = 1;
		break;
		
            case PDTA_ModeID:
                pd->ModeID = (ULONG) ti->ti_Data;
                DGS(bug("picture.datatype/OM_SET: Tag PDTA_ModeID: 0x%lx\n", (long)pd->ModeID));
                break;

            case PDTA_ClassBitMap:
                pd->KeepSrcBM = TRUE;
	        DGS(bug("picture.datatype/OM_GET: Tag PDTA_ClassBitMap: Handled as PDTA_BitMap\n"));
            case PDTA_BitMap:
                pd->SrcBM = (struct BitMap *) ti->ti_Data;
                DGS(bug("picture.datatype/OM_SET: Tag PDTA_BitMap: 0x%lx\n", (long)pd->SrcBM));
                break;

            case PDTA_Screen:
                pd->DestScreen = (struct Screen *) ti->ti_Data;
                DGS(bug("picture.datatype/OM_SET: Tag PDTA_Screen: 0x%lx\n", (long)pd->DestScreen));
                break;

            case PDTA_NumColors:
                pd->NumColors = (UWORD) ti->ti_Data;
                DGS(bug("picture.datatype/OM_SET: Tag PDTA_NumColors: %ld\n", (long)pd->NumColors));
                break;

            case PDTA_Grab:
            {
                Point *ThePoint;

                DGS(bug("picture.datatype/OM_SET: Tag PDTA_Grab\n"));
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
                DGS(bug("picture.datatype/OM_SET: Tag PDTA_SourceMode (ignored): %ld\n", (long)ti->ti_Data));
	        break;

	    case PDTA_DestMode:
		pd->DestMode = (BOOL) ti->ti_Data;
                DGS(bug("picture.datatype/OM_SET: Tag PDTA_DestMode: %ld\n", (long)pd->DestMode));
	        break;

            case PDTA_FreeSourceBitMap:
                pd->FreeSource = (BOOL) ti->ti_Data;
                DGS(bug("picture.datatype/OM_SET: Tag PDTA_FreeSourceBitMap: %ld\n", (long)pd->FreeSource));
                break;

	    case PDTA_UseFriendBitMap:
                pd->UseFriendBM = (BOOL) ti->ti_Data;
                DGS(bug("picture.datatype/OM_SET: Tag PDTA_UseFriendBitMap: %ld\n", (long)pd->UseFriendBM));
	        break;

	    case PDTA_MaxDitherPens:
                pd->MaxDitherPens = (UWORD) ti->ti_Data;
                DGS(bug("picture.datatype/OM_SET: Tag PDTA_MaxDitherPens: %ld\n", (long)pd->MaxDitherPens));
	        break;

	    case PDTA_DitherQuality:
                pd->DitherQuality = (UWORD) ti->ti_Data;
                DGS(bug("picture.datatype/OM_SET: Tag PDTA_DitherQuality: %ld\n", (long)pd->DitherQuality));
	        break;

	    case PDTA_ScaleQuality:
                pd->ScaleQuality = (UWORD) ti->ti_Data;
                DGS(bug("picture.datatype/OM_SET: Tag PDTA_ScaleQuality: %ld\n", (long)pd->ScaleQuality));
	        break;

    	    case PDTA_Remap:                
                pd->Remap = (BOOL) ti->ti_Data;
                DGS(bug("picture.datatype/OM_SET: Tag ID PDTA_Remap: %ld\n", (long)pd->Remap));
                break;    
		
#ifdef __AROS__
	    case PDTA_DelayedRead:
                pd->DelayedRead = (BOOL) ti->ti_Data;
                DGS(bug("picture.datatype/OM_SET: Tag PDTA_DelayedRead: %ld\n", (long)pd->DelayedRead));
	        break;
#endif

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

               DGS(bug("picture.datatype/OM_SET: Tag %s: 0x%lx (%ld)\n", AttribNames[i], (long)ti->ti_Data, (long)ti->ti_Data));
              }
             }

             if(!Known)
             {
              DGS(bug("picture.datatype/OM_SET: Tag ID 0x%lx: 0x%lx\n", (long)ti->ti_Tag, (long)ti->ti_Data));
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
                DoMethod((Object *) g, GM_RENDER, (IPTR) msg->ops_GInfo, (IPTR) rp, GREDRAW_UPDATE);
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
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_ModeID: 0x%lx\n", (long)pd->ModeID));
	    *(msg->opg_Storage)=pd->ModeID;
	    break;

	case PDTA_BitMapHeader:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_BitMapHeader: 0x%lx\n", (long)&pd->bmhd));
	    *(msg->opg_Storage)=(IPTR) &pd->bmhd;
	    break;

	case PDTA_ClassBitMap:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_ClassBitMap: Handled as PDTA_BitMap\n"));
	case PDTA_BitMap:
	    if( !pd->SrcBM )
		ConvertChunky2Bitmap( pd );
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_BitMap: 0x%lx\n", (long)pd->SrcBM));
	    *(msg->opg_Storage)=(IPTR) pd->SrcBM;
	    break;

	case PDTA_DestBitMap:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_DestBitMap: 0x%lx\n", (long)pd->DestBM));
	    *(msg->opg_Storage)=(IPTR) pd->DestBM;
	    break;

	case PDTA_MaskPlane:
	    CreateMaskPlane( pd );
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_MaskPlane: 0x%lx\n", (long)pd->MaskPlane));
	    *(msg->opg_Storage)=(IPTR) pd->MaskPlane;
	    break;

	case PDTA_Screen:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_Screen: 0x%lx\n", (long)pd->DestScreen));
	    *(msg->opg_Storage)=(IPTR) pd->DestScreen;
	    break;

	case PDTA_ColorRegisters:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_ColorRegisters: 0x%lx\n", (long)&pd->ColMap));
	    *(msg->opg_Storage)=(IPTR) &pd->ColMap;
	    break;

	case PDTA_CRegs:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_CRegs: 0x%lx\n", (long)&pd->SrcColRegs));
	    *(msg->opg_Storage)=(IPTR) &pd->SrcColRegs;
	    break;

	case PDTA_GRegs:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_GRegs: 0x%lx\n", (long)&pd->DestColRegs));
	    *(msg->opg_Storage)=(IPTR) &pd->DestColRegs;
	    break;

	case PDTA_AllocatedPens:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_AllocatedPens: Handled by PDTA_ColorTable2\n"));
	case PDTA_ColorTable2:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_ColorTable2: Handled by PDTA_ColorTable\n"));
	case PDTA_ColorTable:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_ColorTable: 0x%lx\n", (long)&pd->ColTable));
	    *(msg->opg_Storage)=(IPTR) &pd->ColTable;
	    break;

	case PDTA_NumColors:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_NumColors: %ld\n", (long)pd->NumColors));
	    *(msg->opg_Storage)=(IPTR) pd->NumColors;
	    break;

	case PDTA_NumAlloc:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_NumAlloc: %ld\n", (long)pd->NumAlloc));
	    *(msg->opg_Storage)=(IPTR) pd->NumAlloc;
	    break;

	case PDTA_Grab:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_Grab: 0x%lx\n", (long)&pd->Grab));
	    *(msg->opg_Storage)=(IPTR) &pd->Grab;
	    break;

	case PDTA_SourceMode:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_SourceMode: 0x%lx\n", (long)PMODE_V43));
	    *(msg->opg_Storage)=(IPTR) PMODE_V43;
	    break;

	case PDTA_DestMode:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_DestMode: 0x%lx\n", (long)pd->DestMode));
	    *(msg->opg_Storage)=(IPTR) pd->DestMode;
	    break;

	case PDTA_FreeSourceBitMap:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_FreeSourceBitMap: 0x%lx\n", (long)pd->FreeSource));
	    *(msg->opg_Storage)=(IPTR) pd->FreeSource;
	    break;

	case PDTA_UseFriendBitMap:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_UseFriendBitMap: 0x%lx\n", (long)pd->UseFriendBM));
	    *(msg->opg_Storage)=(IPTR) pd->UseFriendBM;
	    break;

	case PDTA_MaxDitherPens:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_MaxDitherPens: 0x%lx\n", (long)pd->MaxDitherPens));
	    *(msg->opg_Storage)=(IPTR) pd->MaxDitherPens;
	    break;

	case PDTA_DitherQuality:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_DitherQuality: 0x%lx\n", (long)pd->DitherQuality));
	    *(msg->opg_Storage)=(IPTR) pd->DitherQuality;
	    break;

	case PDTA_ScaleQuality:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_ScaleQuality: 0x%lx\n", (long)pd->ScaleQuality));
	    *(msg->opg_Storage)=(IPTR) pd->ScaleQuality;
	    break;

#ifdef __AROS__
	case PDTA_DelayedRead:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_DelayedRead: 0x%lx\n", (long)pd->DelayedRead));
	    *(msg->opg_Storage)=(IPTR) pd->DelayedRead;
	    break;
#endif

	case DTA_Methods:
	    DGS(bug("picture.datatype/OM_GET: Tag DTA_Methods: 0x%lx\n", (long)SupportedMethods));
	    *(msg->opg_Storage)=(IPTR) SupportedMethods;
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

	      DGS(bug("picture.datatype/OM_GET: Tag ID: %s\n", AttribNames[i]));
	     }
	    }

	    if(!Known)
	    {
	     DGS(bug("picture.datatype/OM_GET: Tag ID: 0x%lx\n", msg->opg_AttrID));
	    }
#endif /* MYDEBUG */

	    return(DoSuperMethodA(cl, (Object *) g, (Msg) msg));
	}
    } /* switch AttrID */

    return TRUE;
}
/**************************************************************************************************/
static void render_on_rastport(struct Picture_Data *pd, struct Gadget *g, LONG SrcX, LONG SrcY, struct RastPort * destRP, 
    LONG DestX, LONG DestY, LONG SizeX, LONG SizeY)
{
    ULONG depth;
    struct BitMapHeader * bmhd;

    depth = (ULONG) GetBitMapAttr(destRP->BitMap, BMA_DEPTH);
    GetDTAttrs((Object *) g, PDTA_BitMapHeader, (IPTR)&bmhd, TAG_DONE);

    if ((depth >= 15) && (bmhd->bmh_Masking == mskHasAlpha))
    {
        /* Transparency on high color rast port with alpha channel in picture */
        struct RastPort srcRP;
        ULONG * img = (ULONG *) AllocVec(SizeX * SizeY * 4, MEMF_ANY);
        if (img)
        {
            InitRastPort(&srcRP);
            srcRP.BitMap = pd->DestBM;

            ReadPixelArray(img, 0, 0, SizeX * 4, &srcRP, SrcX, SrcY, SizeX, SizeY, RECTFMT_ARGB);

            WritePixelArrayAlpha(img, 0, 0, SizeX * 4, destRP, DestX, DestY, SizeX, SizeY, 0xffffffff);
            FreeVec((APTR) img);
        }
    }
    else
    {   
        if ((bmhd->bmh_Masking == mskHasMask) || (bmhd->bmh_Masking == mskHasTransparentColor))
        {
            /* Transparency with mask */
            APTR mask = NULL;

            GetDTAttrs((Object *) g, PDTA_MaskPlane, (IPTR)&mask, TAG_DONE);

            if (mask) 
                BltMaskBitMapRastPort(pd->DestBM, 
                                    SrcX,
                                    SrcY,
                                    destRP,
                                    DestX,
                                    DestY,
                                    SizeX,
                                    SizeY,
                                    0xE0,
                                    (PLANEPTR)mask);
        }
        else
        {
            /* All other cases */
            BltBitMapRastPort( pd->DestBM,
                              SrcX,
                              SrcY,
                              destRP,
                              DestX,
                              DestY,
                              SizeX,
                              SizeY,
                              0xC0);
        }
    }

}
/**************************************************************************************************/

STATIC IPTR DT_Render(struct IClass *cl, struct Gadget *g, struct gpRender *msg)
{
    struct Picture_Data *pd;
    struct DTSpecialInfo *si;

    struct IBox *domain;
    IPTR TopVert, TopHoriz;

    long SrcX, SrcY, DestX, DestY, SizeX, SizeY;

    pd = (struct Picture_Data *) INST_DATA(cl, g);
    si = (struct DTSpecialInfo *) g->SpecialInfo;

    if(!pd->Layouted)
    {
        D(bug("picture.datatype/GM_RENDER: No layout done yet !\n"));
        return FALSE;
    }

    if(si->si_Flags & DTSIF_LAYOUT)
    {
        D(bug("picture.datatype/GM_RENDER: In layout process !\n"));
        return FALSE;
    }

    if(!(GetDTAttrs((Object *) g, DTA_Domain,    (IPTR) &domain,
   			       DTA_TopHoriz,     (IPTR) &TopHoriz,
   			       DTA_TopVert,      (IPTR) &TopVert,
   			       TAG_DONE) == 3))
    {
        D(bug("picture.datatype/GM_RENDER: Couldn't get dimensions\n"));
        return FALSE;
    }

    ObtainSemaphore(&(si->si_Lock));
    D(bug("picture.datatype/GM_RENDER: Domain: left %ld top %ld width %ld height %ld\n", domain->Left, domain->Top, domain->Width, domain->Height));
    D(bug("picture.datatype/GM_RENDER: TopHoriz %ld TopVert %ld Width %ld Height %ld\n", (long)TopHoriz, (long)TopVert, (long)pd->DestWidth, (long)pd->DestHeight));

    if( pd->DestBM )
    {
	SrcX = MIN( TopHoriz, pd->DestWidth );
	SrcY = MIN( TopVert, pd->DestHeight );
	DestX = domain->Left;
	DestY = domain->Top;
	SizeX = MIN( pd->DestWidth - SrcX, domain->Width );
	SizeY = MIN( pd->DestHeight - SrcY, domain->Height );
	D(bug("picture.datatype/GM_RENDER: SizeX/Y %ld/%ld\n SrcX/Y %ld/%ld DestX/Y %ld/%ld\n",
	    SizeX, SizeY, SrcX, SrcY, DestX, DestY));

	render_on_rastport(pd, g, SrcX, SrcY, msg->gpr_RPort, DestX, DestY, SizeX, SizeY);
    }
    else
    {
        D(bug("picture.datatype/GM_RENDER: No destination picture present !\n"));
        return FALSE;
    }
    ReleaseSemaphore(&(si->si_Lock));

    return TRUE;
}

/**************************************************************************************************/

STATIC IPTR DT_GoActiveMethod(struct IClass *cl, struct Gadget *g, struct gpInput *msg)
{
    struct DTSpecialInfo *dtsi = (struct DTSpecialInfo *)g->SpecialInfo;
    struct Picture_Data  *pd = INST_DATA(cl, g);
    IPTR    	    	  retval = GMR_NOREUSE;
    
    if (!AttemptSemaphore(&dtsi->si_Lock))
    {
	return GMR_NOREUSE;
    }
    
    if (dtsi->si_Flags & DTSIF_DRAGSELECT)
    {
    	ReleaseSemaphore(&dtsi->si_Lock);
	
    	return DoSuperMethodA(cl, (Object *)g, (Msg)msg);
    }
    else if (msg->gpi_IEvent && !(dtsi->si_Flags & DTSIF_LAYOUT))
    {
    	IPTR toph, topv;
	
	GetDTAttrs((Object *)g, DTA_TopVert, (IPTR)&topv,
	    	    	    	DTA_TopHoriz, (IPTR)&toph,
				TAG_DONE);
				
    	pd->ClickX = msg->gpi_Mouse.X + (LONG)toph;
	pd->ClickY = msg->gpi_Mouse.Y + (LONG)topv;
	
    	retval = GMR_MEACTIVE;
    }
    
    ReleaseSemaphore(&dtsi->si_Lock);
    
    return retval;
}

STATIC IPTR DT_HandleInputMethod(struct IClass *cl, struct Gadget *g, struct gpInput *msg)
{
    struct DTSpecialInfo *dtsi = (struct DTSpecialInfo *)g->SpecialInfo;
    struct Picture_Data  *pd = INST_DATA(cl, g);
    IPTR    	    	  retval = GMR_MEACTIVE;
    
    if (!AttemptSemaphore(&dtsi->si_Lock))
    {
	return GMR_NOREUSE;
    }

    if (dtsi->si_Flags & DTSIF_DRAGSELECT)
    {
    	ReleaseSemaphore(&dtsi->si_Lock);
    	return DoSuperMethodA(cl, (Object *)g, (Msg)msg);
    }
    
    if (dtsi->si_Flags & DTSIF_LAYOUT)
    {
    	ReleaseSemaphore(&dtsi->si_Lock);
	return GMR_NOREUSE;
    }
    
    switch(msg->gpi_IEvent->ie_Class)
    {
    	case IECLASS_RAWMOUSE:
	    switch(msg->gpi_IEvent->ie_Code)
	    {
	    	case SELECTUP:
		    retval = GMR_NOREUSE;
		    break;
		    
		case IECODE_NOBUTTON:
		{
		    IPTR toph, totalh, visibleh;
		    IPTR topv, totalv, visiblev;
		    LONG newtoph, newtopv;
		    
		    GetDTAttrs((Object *)g, DTA_TopVert     , (IPTR) &topv,
		    	    	    	    DTA_TotalVert   , (IPTR) &totalv,
					    DTA_VisibleVert , (IPTR) &visiblev,
					    DTA_TopHoriz    , (IPTR) &toph,
					    DTA_TotalHoriz  , (IPTR) &totalh,
					    DTA_VisibleHoriz, (IPTR) &visibleh,
					    TAG_DONE);
				
		    newtoph = pd->ClickX - msg->gpi_Mouse.X;
		    newtopv = pd->ClickY - msg->gpi_Mouse.Y;
		    
		    if (newtoph + (LONG)visibleh > (LONG)totalh) newtoph = (LONG)totalh - (LONG)visibleh;
		    if (newtoph < 0) newtoph = 0;
		    
		    if (newtopv + (LONG)visiblev > (LONG)totalv) newtopv = (LONG)totalv - (LONG)visiblev;
		    if (newtopv < 0) newtopv = 0;
		    
		    if ((newtoph != (LONG)toph) || (newtopv != (LONG)topv))
		    {
		    	NotifyAttrChanges((Object *) g, msg->gpi_GInfo, 0,
					   GA_ID, g->GadgetID,
					   DTA_TopHoriz, newtoph,
					   DTA_TopVert, newtopv,
					   TAG_DONE);
		    }

		    break;
		}
	    }
	    
	    break;
    }
    
    ReleaseSemaphore(&dtsi->si_Lock);
    
    return retval;
}

/**************************************************************************************************/

STATIC IPTR DT_Layout(struct IClass *cl, struct Gadget *g, struct gpLayout *msg)
{
    IPTR RetVal;

    D(bug("picture.datatype/GM_LAYOUT: Initial %d\n", (int)msg->gpl_Initial));
    NotifyAttrChanges((Object *) g, msg->gpl_GInfo, 0,
   				 GA_ID, g->GadgetID,
   				 DTA_Busy, TRUE,
   				 TAG_DONE);

    RetVal=DoSuperMethodA(cl, (Object *) g, (Msg) msg);

    RetVal += (IPTR) DoAsyncLayout((Object *) g, msg);

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
    D(bug("picture.datatype/DTM_ASYNCLAYOUT: Masking %d Transparent %d Initial %d Layouted %d\n", (int)pd->bmhd.bmh_Masking, (int)pd->bmhd.bmh_Transparent, (int)msg->gpl_Initial, (int)pd->Layouted));

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
    if( msg->gpl_Initial | !pd->Layouted )   /* we need to do it just once at startup or after scaling */
    {
        FreeDest( pd );
	if( pd->Remap )
	{
	    /* determine destination screen depth */
	    if( !pd->DestScreen && msg->gpl_GInfo )
	    {
		pd->DestScreen = msg->gpl_GInfo->gi_Screen;
	    }
	    if( !pd->DestScreen )
	    {
		D(bug("picture.datatype/DTM_ASYNCLAYOUT: No screen given !\n"));
		ReleaseSemaphore(&si->si_Lock);   /* unlock object data */
		return FALSE;
	    }
	    pd->DestDepth = GetBitMapAttr( pd->DestScreen->RastPort.BitMap, BMA_DEPTH );
	}
	else
	{
	#if 0  // stegerg: CHECKME
	    if( pd->SrcDepth > 8 )
	    {
		D(bug("picture.datatype/DTM_ASYNCLAYOUT: Remap=FALSE option only for colormapped source !\n"));
		ReleaseSemaphore(&si->si_Lock);   /* unlock object data */
		return FALSE;
	    }
	#endif
	    if( pd->Scale )
	    {
		D(bug("picture.datatype/DTM_ASYNCLAYOUT: Scaling doesn't work with Remap=FALSE !\n"));
		ReleaseSemaphore(&si->si_Lock);   /* unlock object data */
		return FALSE;
	    }
	#if 0 // stegerg: CHECKME
	    pd->DestScreen = NULL;
	#endif
	    pd->DestDepth = SrcDepth;
	}

        if( pd->DestDepth > 8 )
	{
            pd->TrueColorDest = TRUE;
	    if( !pd->DestMode )
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

	/* allocate destination Bitmap */
	FreeDest( pd );
	if( !pd->Scale )
	{
	    pd->DestWidth = SrcWidth;
	    pd->DestHeight = SrcHeight;
	}
	D(bug("picture.datatype/DTM_ASYNCLAYOUT: Destination Width %ld Height %ld Depth %ld\n", pd->DestWidth, pd->DestHeight, (long)pd->DestDepth));
	if( !AllocDestBM( pd ) )
	{
            D(bug("picture.datatype/DTM_ASYNCLAYOUT: Didn't get dest BM !\n"));
	    ReleaseSemaphore(&si->si_Lock);   /* unlock object data */
	    return FALSE;
	}

	/* remap picture depending on the source/dest color case */
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
	
	/* free source, if asked */
	if( pd->FreeSource )
	{
	    CreateMaskPlane( pd );
	    FreeSource( pd );
	}
	
	/* layout done */
        pd->Layouted = TRUE;
        D(bug("picture.datatype/DTM_ASYNCLAYOUT: Initial layout done\n"));
    } /* if( msg->gpl_Initial | !pd->Layouted ) */

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
	 *  get attributes
	 */
	if(!(GetDTAttrs((Object *) g, DTA_Domain, (IPTR) &domain,
				   DTA_ObjName, (IPTR) &Title,
				   DTA_NominalHoriz, (IPTR) &Width,
				   DTA_NominalVert, (IPTR) &Height,
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
    
	NotifyAttrChanges((Object *) g, msg->gpl_GInfo, 0,
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
        D(bug("picture.datatype/DTM_ASYNCLAYOUT: NotifyAttrChanges done, Layouted %d\n", (int)pd->Layouted));
    }
    return TRUE;
}

/**************************************************************************************************/

STATIC IPTR DT_ProcLayout(struct IClass *cl, struct Gadget *g, struct gpLayout *msg)
{
    NotifyAttrChanges((Object *) g, msg->gpl_GInfo, 0,
   				 GA_ID, g->GadgetID,
   				 DTA_Busy, TRUE,
   				 TAG_DONE);

    DoSuperMethodA(cl, (Object *) g, (Msg) msg);

    return DT_AsyncLayout(cl, g, msg);
}

/**************************************************************************************************/

STATIC IPTR PDT_WritePixelArray(struct IClass *cl, struct Gadget *g, struct pdtBlitPixelArray *msg)
{
    struct Picture_Data *pd;

    int pixelformat;
    int pixelbytes;

    pd = (struct Picture_Data *) INST_DATA(cl, g);

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
                case PBPAFMT_RGBA:
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

    /* Copy picture data */
    {
        LONG line, lines;
        STRPTR srcstart;
        STRPTR deststart;
        ULONG srcwidth, numbytes;
        ULONG srcmod, destmod;

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

    pd->Layouted = FALSE;	/* re-layout required */
    return TRUE;
}

/**************************************************************************************************/

STATIC IPTR PDT_ReadPixelArray(struct IClass *cl, struct Gadget *g, struct pdtBlitPixelArray *msg)
{
    struct Picture_Data *pd;

    int pixelformat;
    int pixelbytes;

    pd = (struct Picture_Data *) INST_DATA(cl, g);

    /* Do some checks first */
    if (!pd->DestMode)
    {
        D(bug("picture.datatype/DTM_READPIXELARRAY: Wrong DestMode\n"));
	return FALSE;
    }
    
    if( !pd->SrcBuffer)
    {
    	ConvertBitmap2Chunky(pd);
    }
    
    if (!pd->SrcBuffer)
    {
        D(bug("picture.datatype/DTM_READPIXELARRAY: No source buffer\n"));
	return FALSE;
    }
    pixelformat = (long)msg->pbpa_PixelFormat;
    D(bug("picture.datatype/DTM_READPIXELARRAY: Source/Dest Pixelformat %d / %ld\n", pixelformat, pd->SrcPixelFormat));

    if ( pixelformat == pd->SrcPixelFormat )
    {
	/* Copy picture data, as source pixmode = dest pixmode */
        long line, lines;
        STRPTR srcstart;
        STRPTR deststart;
        long srcmod, destmod;
        long destwidth, numbytes;

        /* Now copy the new source data to the ChunkyBuffer line by line */
        pixelbytes = pd->SrcPixelBytes;
        srcmod = pd->SrcWidthBytes;
        srcstart = pd->SrcBuffer + msg->pbpa_Left * pixelbytes + msg->pbpa_Top * srcmod;
        destmod = msg->pbpa_PixelArrayMod;
        deststart = msg->pbpa_PixelData;
        destwidth = msg->pbpa_Width;
        lines = msg->pbpa_Height;
        numbytes = destwidth * pixelbytes;

	/* simply copy data */
	for( line=0; line<lines; line++ )
	{
	    // D(bug("picture.datatype/DTM_READPIXELARRAY: COPY src 0x%lx dest 0x%lx bytes %ld\n", (long)srcstart, (long)deststart, numbytes));
	    CopyMem((APTR) srcstart, (APTR) deststart, numbytes);
	    srcstart += srcmod;
	    deststart += destmod;
	}
    }
    else if ( pixelformat == PBPAFMT_RGB || pixelformat == PBPAFMT_RGBA || pixelformat == PBPAFMT_ARGB )
    {
	/* Copy picture data pixel by pixel (this is not fast, but compatible :-) */
	UBYTE r=0, g=0, b=0, a=0xff;
        long line, x, col;
	int srcpixelformat;
        STRPTR srcstart;
	UBYTE *srcptr;
        STRPTR deststart;
	UBYTE *destptr;
        long srcmod, destmod;
	ULONG * colregs;

        /* Now copy the new source data to the ChunkyBuffer line by line */
	srcpixelformat = pd->SrcPixelFormat;
        srcmod = pd->SrcWidthBytes;
        srcstart = pd->SrcBuffer + msg->pbpa_Left * pd->SrcPixelBytes + msg->pbpa_Top * srcmod;
        destmod = msg->pbpa_PixelArrayMod;
        deststart = msg->pbpa_PixelData;
	colregs = pd->SrcColRegs;

	for( line=0; line<msg->pbpa_Height; line++ )
	{
	    srcptr = srcstart;
	    destptr = deststart;
	    for( x=0; x<pd->SrcWidth; x++ )
	    {
		switch( srcpixelformat )
		{
		    case PBPAFMT_GREY8:
		    case PBPAFMT_LUT8:
			col = 3 * (*srcptr++);
			r = colregs[col++] >> 24;
			g = colregs[col++] >> 24;
			b = colregs[col] >> 24;
			break;
		    case PBPAFMT_ARGB:
			a = *srcptr++;
		    case PBPAFMT_RGB:
			r = *srcptr++;
			g = *srcptr++;
			b = *srcptr++;
			break;
    	            case PBPAFMT_RGBA:
    	                r = *srcptr++;
    	                g = *srcptr++;
    	                b = *srcptr++;   
    	                a = *srcptr++;
    	                break;
		}
		if( pixelformat == PBPAFMT_ARGB )
		    *destptr++ = a;
		*destptr++ = r;
		*destptr++ = g;
		*destptr++ = b;
		if( pixelformat == PBPAFMT_RGBA )
		    *destptr++ = a;
	    }
	    srcstart += srcmod;
	    deststart += destmod;
	}
    }
    else
    {
        D(bug("picture.datatype/DTM_READPIXELARRAY: Source/Dest Pixelformat mismatch (not yet supported)\n"));
	return FALSE;
    }
    return TRUE;
}

/**************************************************************************************************/

STATIC IPTR PDT_Scale(struct IClass *cl, struct Gadget *g, struct pdtScale *msg)
{
    struct Picture_Data *pd;
    ULONG xscale, yscale;
    struct DTSpecialInfo *si;

    pd = (struct Picture_Data *) INST_DATA(cl, g);
    si = (struct DTSpecialInfo *) g->SpecialInfo;

    ObtainSemaphore( &(si->si_Lock) );	/* lock object data */
    D(bug("picture.datatype/PDTM_SCALE: newwidth %ld newheight %ld flags %08lx\n", msg->ps_NewWidth, msg->ps_NewHeight, msg->ps_Flags));

    pd->DestWidth = msg->ps_NewWidth;
    pd->DestHeight = msg->ps_NewHeight;
    
    /* FIXME: PDT_Scale() set bmh_Width/bmh_Height to new size yes or no? */
    pd->bmhd.bmh_Width = msg->ps_NewWidth;   
    pd->bmhd.bmh_Height = msg->ps_NewHeight;   

    if( pd->SrcWidth == pd->DestWidth && pd->SrcHeight == pd->DestHeight )
	pd->Scale = FALSE;
    else
	pd->Scale = TRUE;

    xscale = (pd->SrcWidth << 16) / pd->DestWidth;
    yscale = (pd->SrcHeight << 16) / pd->DestHeight;
#ifdef __AROS__
    if( msg->ps_Flags & PScale_KeepAspect )
    {
	xscale = yscale = MAX(xscale, yscale);
	pd->DestWidth = (pd->SrcWidth << 16) / xscale;
	pd->DestHeight = (pd->SrcHeight << 16) / yscale;
    }
#endif
    pd->XScale = xscale;
    pd->YScale = yscale;
    D(bug("picture.datatype/PDTM_SCALE: srcwidth %ld srcheight %ld destwidth %ld destheight %ld xscale %06lx yscale %06lx\n", pd->SrcWidth, pd->SrcHeight, pd->DestWidth, pd->DestHeight, pd->XScale, pd->YScale));
    
    SetDTAttrs((Object *) g, NULL, NULL,
			DTA_NominalHoriz, pd->DestWidth,
			DTA_NominalVert , pd->DestHeight,
			TAG_DONE);
    pd->Layouted = FALSE;	/* re-layout required */

    ReleaseSemaphore( &si->si_Lock );   /* unlock object data */
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

    /* FIXME: DT_FrameBox implementation may need to be checked */

    /* It is not really clear/documented what's the correct thing to do
       here. And what effect FRAMEF_SPECIFY has *here*. The demo sources on 
       the Amiga Dev CD 2.1 are conflicting. 
       
       ClipView source on Amiga Dev CD 2.1 uses ContentsInfo and FRAMEF_SPECIFY
       and (!) uninitialized FrameInfo. So accessing FrameInfo here would crash.
       
       Most other sources on the Dev CD set both ContentsInfo and FrameInfo
       to the same struct. Without using FRAMEF_SPECIFY.
       
       Another source (Reference/Amiga_Mail_Vol2/IV-101/dtpic.c) uses FrameInfo
       and NULLs ContentsInfo and no FRAMEF_SPECIFY. */
       
#if 1
    if(msg->dtf_ContentsInfo)
    {
        msg->dtf_ContentsInfo->fri_Dimensions.Height = Height;
        msg->dtf_ContentsInfo->fri_Dimensions.Width  = Width;
        msg->dtf_ContentsInfo->fri_Dimensions.Depth  = Depth;
        msg->dtf_ContentsInfo->fri_Flags             = FIF_SCROLLABLE;

        RetVal = 1;
    }
#else
    if(msg->dtf_FrameInfo)
    {
        msg->dtf_FrameInfo->fri_Dimensions.Height = Height;
        msg->dtf_FrameInfo->fri_Dimensions.Width  = Width;
        msg->dtf_FrameInfo->fri_Dimensions.Depth  = Depth;
        msg->dtf_FrameInfo->fri_Flags             = FIF_SCROLLABLE;

        RetVal = 1;
    }
#endif

    return(RetVal);
}

/**************************************************************************************************/

STATIC IPTR DT_ObtainDrawInfo(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    struct Picture_Data *pd;
    IPTR RetVal;

    pd = (struct Picture_Data *) INST_DATA(cl, g);

    RetVal = FALSE;

    if( !pd->UseAsImage )
    {
    	RetVal = DoMethod( (Object *)g, DTM_PROCLAYOUT, (IPTR) NULL, 1L );
    	if( RetVal )
	{
    	    pd->UseAsImage = TRUE;
            D(bug("picture.datatype/DTM_OBTAINDRAWINFO: Switched to image mode\n"));
        }
    }

    return RetVal;
}

/**************************************************************************************************/

STATIC IPTR DT_Draw(struct IClass *cl, struct Gadget *g, struct dtDraw *msg)
{
    struct Picture_Data *pd;
    IPTR RetVal;

    pd=(struct Picture_Data *) INST_DATA(cl, g);

    RetVal = FALSE;

    if( pd->UseAsImage && pd->DestBM )
    {
    long SrcX, SrcY, DestX, DestY, SizeX, SizeY;

	SrcX = MIN( msg->dtd_TopHoriz, pd->DestWidth );
	SrcY = MIN( msg->dtd_TopVert, pd->DestHeight );
	DestX = msg->dtd_Left;
	DestY = msg->dtd_Top;
	SizeX = MIN( pd->DestWidth - SrcX, msg->dtd_Width );
	SizeY = MIN( pd->DestHeight - SrcY, msg->dtd_Height );
	D(bug("picture.datatype/DTM_DRAW: SizeX/Y %ld/%ld SrcX/Y %ld/%ld DestX/Y %ld/%ld\n",
	    SizeX, SizeY, SrcX, SrcY, DestX, DestY));

	render_on_rastport(pd, g, SrcX, SrcY, msg->dtd_RPort, DestX, DestY, SizeX, SizeY);
        D(bug("picture.datatype/DTM_DRAW: Switched to image mode\n"));
        RetVal = TRUE;
    }

    return RetVal;
}

/**************************************************************************************************/

STATIC IPTR DT_Print(struct IClass *cl, Object *o, struct dtPrint *msg)
{
    IPTR RetVal;
    struct IODRPReq *pio = &msg->dtp_PIO->iodrp;
    struct RastPort *rp = NULL;
    struct TagItem *tag, *tags = msg->dtp_AttrList;
    IPTR w = 0, h = 0, th = 0, tw = 0;
    struct GadgetInfo *gi = msg->dtp_GInfo;

    GetDTAttrs(o, DTA_NominalHoriz, &w, DTA_NominalVert, &h, TAG_DONE);
    GetDTAttrs(o, DTA_TopHoriz, &tw, DTA_TopVert, &th, TAG_DONE);

    if (w == 0 || h == 0)
        return 0;

    RetVal = PDERR_CANCEL;

    pio->io_Command = PRD_DUMPRPORT;
    pio->io_SrcX = 0;
    pio->io_SrcY = 0;
    pio->io_SrcWidth = w;
    pio->io_SrcHeight = h;
    pio->io_DestCols = 0;
    pio->io_DestRows = 0;
    pio->io_Special = 0;

    while ((tag = NextTagItem(&tags))) {
        switch (tag->ti_Tag) {
        case DTA_DestCols: pio->io_DestCols = (LONG)tag->ti_Data; break;
        case DTA_DestRows: pio->io_DestRows = (LONG)tag->ti_Data; break;
        case DTA_RastPort: rp = (struct RastPort *)tag->ti_Data; break;
        case DTA_Special:  pio->io_Special = (UWORD)tag->ti_Data; break;
        default: break;
        }
    }
    tags = msg->dtp_AttrList;

    if (rp) {
        /* Print from supplied (non colormap) rastport */
        D(bug("%s: Print from RastPort %p\n", __func__, rp));
        pio->io_RastPort = rp;
        pio->io_ColorMap = NULL;
        pio->io_Modes = INVALID_ID;
        RetVal = DoIO((struct IORequest *)pio);
    } else if (gi) {
        /* Print as Gadget */
        struct Screen *s;
        
        D(bug("%s: Print from Gadget %p on Screen %p\n", __func__, gi, gi->gi_Screen));
        if ((s = gi->gi_Screen)) {
            if ((pio->io_Modes = GetVPModeID(&s->ViewPort)) != INVALID_ID) {
                pio->io_ColorMap = s->ViewPort.ColorMap;
                if ((pio->io_RastPort = ObtainGIRPort(gi))) {
                    RetVal = DoIO((struct IORequest *)pio);
                    ReleaseGIRPort(pio->io_RastPort);
                } else {
                    D(bug("%s:   Can't obtain GI RastPort\n", __func__));
                }
            } else {
                D(bug("%s:   No valid Screen mode\n", __func__));
            }
        }
    } else {
        /* Print as a 24-bit color image */
        struct RastPort baseRP;
        struct BitMap *bm;

        D(bug("%s: Printing as image\n", __func__));

        rp = &baseRP;

        pio->io_ColorMap = NULL;
        pio->io_Modes = INVALID_ID;
        pio->io_RastPort = rp;
        InitRastPort(rp);
        /* Maybe we should do strip printing to save memory?
         *
         * Too bad native AOS's PostScript driver doesn't
         * support that.
         *
         * Well, if 24 bit color fails, try 12 bit (4096 color),
         * and if that fails, just do black & white.
         */
        if ((bm = AllocBitMap(w, h, 24, 0, NULL)) ||
            (bm = AllocBitMap(w, h, 12, 0, NULL)) ||
            (bm = AllocBitMap(w, h,  1, 0, NULL))) {
            struct Layer_Info *li;
            rp->BitMap = bm;
            D(bug("%s:   Printing with Depth %d bitmap\n", __func__, GetBitMapAttr(bm, BMA_DEPTH)));
            if ((li = NewLayerInfo())) {
                struct Layer *layer;
                if ((layer = CreateUpfrontLayer(li, bm, 0, 0, w, h, 0, NULL))) {
                    APTR drawInfo;
                    rp->Layer = layer;
                    if ((drawInfo = ObtainDTDrawInfo(o, tags))) {
                        if (DrawDTObject(rp, o, 0, 0, w, h, th, tw, tags)) {
                            RetVal = DoIO((struct IORequest *)pio);
                        } else {
                            D(bug("%s:   Can't draw object to printer RastPort\n", __func__));
                        }
                        ReleaseDTDrawInfo(o, drawInfo);
                    } else {
                        D(bug("%s:   Can't obtain DTDrawInfo\n", __func__));
                    }
                    DeleteLayer(0, layer);
                } else {
                    D(bug("%s:   Can't allocate %dx%d layer\n", __func__, w, h));
                }
                DisposeLayerInfo(li);
            } else {
                D(bug("%s:   Can't allocate LayerInfo\n", __func__));
            }
            FreeBitMap(bm);
        } else {
            D(bug("%s:   Can't allocate a bitmap\n", __func__));
        }
    }

    return RetVal;
}

/**************************************************************************************************/

STATIC IPTR DT_ReleaseDrawInfo(struct IClass *cl, struct Gadget *g, struct dtReleaseDrawInfo *msg)
{
    struct Picture_Data *pd;
    IPTR RetVal;

    pd = (struct Picture_Data *) INST_DATA(cl, g);

    RetVal = FALSE;

    if( pd->UseAsImage )
    {
	pd->UseAsImage = FALSE;
	RetVal = TRUE;
    }

    return RetVal;
}

/**************************************************************************************************/

#ifndef __AROS__
ASM ULONG DT_Dispatcher(register __a0 struct IClass *cl, register __a2 Object *o, register __a1 Msg msg)
{
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
            // DGS(bug("picture.datatype/DT_Dispatcher: Method OM_GET\n"));
            RetVal=(IPTR) DT_GetMethod(cl, (struct Gadget *) o, (struct opGet *) msg);
            break;
        }

        case OM_SET:
        case OM_UPDATE:
        {
            DGS(bug("picture.datatype/DT_Dispatcher: Method %s\n", (msg->MethodID==OM_UPDATE) ? "OM_UPDATE" : "OM_SET"));
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

    	case GM_GOACTIVE:
	{
            D(bug("picture.datatype/DT_Dispatcher: Method GM_GOACTIVE\n"));
	    RetVal = DT_GoActiveMethod(cl, (struct Gadget *)o, (struct gpInput *)msg);
	    break;
	}
	   
	case GM_HANDLEINPUT:
	{
            D(bug("picture.datatype/DT_Dispatcher: Method GM_HANDLEINPUT\n"));
	    RetVal = DT_HandleInputMethod(cl, (struct Gadget *)o, (struct gpInput *)msg);
	    break;
	}
	   
        case DTM_PROCLAYOUT:
        {
            D(bug("picture.datatype/DT_Dispatcher: Method DTM_PROCLAYOUT\n"));
            RetVal=(IPTR) DT_ProcLayout(cl, (struct Gadget *) o, (struct gpLayout *) msg);
	    break;
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

        case DTM_OBTAINDRAWINFO:
        {
            D(bug("picture.datatype/DT_Dispatcher: Method DTM_OBTAINDRAWINFO\n"));
            RetVal=(IPTR) DT_ObtainDrawInfo(cl, (struct Gadget *) o, (struct opSet *) msg);
            break;
        }

        case DTM_PRINT:
        {
            D(bug("picture.datatype/DT_Dispatcher: Method DTM_PRINT\n"));
            RetVal=(IPTR) DT_Print(cl, o, (struct dtPrint *) msg);
            break;
        }

        case DTM_DRAW:
        {
            D(bug("picture.datatype/DT_Dispatcher: Method DTM_DRAW\n"));
            RetVal=(IPTR) DT_Draw(cl, (struct Gadget *) o, (struct dtDraw *) msg);
            break;
        }

        case DTM_RELEASEDRAWINFO:
        {
            D(bug("picture.datatype/DT_Dispatcher: Method DTM_RELEASEDRAWINFO\n"));
            RetVal=(IPTR) DT_ReleaseDrawInfo(cl, (struct Gadget *) o, (struct dtReleaseDrawInfo *) msg);
            break;
        }

        case PDTM_WRITEPIXELARRAY:
        {
            // D(bug("picture.datatype/DT_Dispatcher: Method PDTM_WRITEPIXELARRAY\n"));
            RetVal=(IPTR) PDT_WritePixelArray(cl, (struct Gadget *) o, (struct pdtBlitPixelArray *) msg);
            break;
        }

        case PDTM_READPIXELARRAY:
        {
            // D(bug("picture.datatype/DT_Dispatcher: Method PDTM_READPIXELARRAY\n"));
            RetVal=(IPTR) PDT_ReadPixelArray(cl, (struct Gadget *) o, (struct pdtBlitPixelArray *) msg);
            break;
        }

        case PDTM_SCALE:
        {
            D(bug("picture.datatype/DT_Dispatcher: Method PDTM_SCALE\n"));
            RetVal=(IPTR) PDT_Scale(cl, (struct Gadget *) o, (struct pdtScale *) msg);
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
}

/**************************************************************************************************/

struct IClass *DT_MakeClass(struct Library *picturebase)
{
    struct IClass *cl = MakeClass("picture.datatype", DATATYPESCLASS, NULL, sizeof(struct Picture_Data), 0);

    if (cl)
    {
	cl->cl_Dispatcher.h_Entry = (HOOKFUNC) DT_Dispatcher;
	cl->cl_Dispatcher.h_SubEntry = (HOOKFUNC) getreg(REG_A4);
	cl->cl_UserData = (IPTR) picturebase;  /* Required by datatypes */
    }

    return cl;
}
#endif /* !__AROS__ */

/**************************************************************************************************/

