/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
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
//    DTM_PRINT,
//    DTM_WRITE,

    PDTM_WRITEPIXELARRAY,
    PDTM_READPIXELARRAY,
    PDTM_SCALE,

    (~0)
};

/**************************************************************************************************/

IPTR Picture__OM_SET(struct IClass *cl, struct Gadget *g, struct opSet *msg);

/**************************************************************************************************/

STATIC ULONG NotifyAttrChanges(Object * o, VOID * ginfo, ULONG flags, ULONG tag1,...)
{
    return(DoMethod(o, OM_NOTIFY, (IPTR) &tag1, (IPTR) ginfo, flags));
}

/**************************************************************************************************/

struct Gadget *Picture__OM_NEW(struct IClass *cl, Object *o, struct opSet *msg)
{
    struct Gadget *g;
    const struct TagItem *attrs = msg->ops_AttrList;
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
    pd->DitherQuality = 4;
    pd->UseFriendBM = 1;
    pd->DestMode = 1;	/* needs to be changed to FALSE after Multiview adaption */

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

            case PDTA_DelayRead:
            	if(!pd->NoDelay)
                    pd->DelayRead = (BOOL) ti->ti_Data;
                DGS(bug("picture.datatype/OM_NEW: Tag ID PDTA_DelayRead: %ld\n", (long)pd->DelayRead));
                break;
        }
    }

    D(bug("picture.datatype/OM_NEW: Setting attributes\n"));
    Picture__OM_SET(cl, g, msg);

    return g;
}

/**************************************************************************************************/

IPTR Picture__OM_DISPOSE(struct IClass *cl, Object *o, Msg msg)
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

IPTR Picture__OM_SET(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    struct Picture_Data *pd;
    const struct TagItem *tl = msg->ops_AttrList;
    struct TagItem *ti;
    IPTR RetVal;
    struct RastPort *rp;

    pd=(struct Picture_Data *) INST_DATA(cl, g);
    RetVal=0;

    while((ti=NextTagItem(&tl)))
    {
        switch (ti->ti_Tag)
        {
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

	    case PDTA_DelayedRead:
                pd->DelayedRead = (BOOL) ti->ti_Data;
                DGS(bug("picture.datatype/OM_SET: Tag PDTA_DelayedRead: %ld\n", (long)pd->DelayedRead));
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
IPTR Picture__OM_UPDATE(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    return Picture__OM_SET(cl, g, msg);
}

/**************************************************************************************************/

IPTR Picture__OM_GET(struct IClass *cl, struct Gadget *g, struct opGet *msg)
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
	    *(msg->opg_Storage)=(ULONG) &pd->bmhd;
	    break;

	case PDTA_ClassBitMap:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_ClassBitMap: Handled as PDTA_BitMap\n"));
	case PDTA_BitMap:
	    if( !pd->SrcBM )
		ConvertChunky2Bitmap( pd );
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_BitMap: 0x%lx\n", (long)pd->SrcBM));
	    *(msg->opg_Storage)=(ULONG) pd->SrcBM;
	    break;

	case PDTA_DestBitMap:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_DestBitMap: 0x%lx\n", (long)pd->DestBM));
	    *(msg->opg_Storage)=(ULONG) pd->DestBM;
	    break;

	case PDTA_MaskPlane:
	    CreateMaskPlane( pd );
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_MaskPlane: 0x%lx\n", (long)pd->MaskPlane));
	    *(msg->opg_Storage)=(ULONG) pd->MaskPlane;
	    break;

	case PDTA_Screen:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_Screen: 0x%lx\n", (long)pd->DestScreen));
	    *(msg->opg_Storage)=(ULONG) pd->DestScreen;
	    break;

	case PDTA_ColorRegisters:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_ColorRegisters: 0x%lx\n", (long)&pd->ColMap));
	    *(msg->opg_Storage)=(ULONG) &pd->ColMap;
	    break;

	case PDTA_CRegs:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_CRegs: 0x%lx\n", (long)&pd->SrcColRegs));
	    *(msg->opg_Storage)=(ULONG) &pd->SrcColRegs;
	    break;

	case PDTA_GRegs:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_GRegs: 0x%lx\n", (long)&pd->DestColRegs));
	    *(msg->opg_Storage)=(ULONG) &pd->DestColRegs;
	    break;

	case PDTA_AllocatedPens:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_AllocatedPens: Handled by PDTA_ColorTable2\n"));
	case PDTA_ColorTable2:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_ColorTable2: Handled by PDTA_ColorTable\n"));
	case PDTA_ColorTable:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_ColorTable: 0x%lx\n", (long)&pd->ColTable));
	    *(msg->opg_Storage)=(ULONG) &pd->ColTable;
	    break;

	case PDTA_NumColors:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_NumColors: %ld\n", (long)pd->NumColors));
	    *(msg->opg_Storage)=(ULONG) pd->NumColors;
	    break;

	case PDTA_NumAlloc:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_NumAlloc: %ld\n", (long)pd->NumAlloc));
	    *(msg->opg_Storage)=(ULONG) pd->NumAlloc;
	    break;

	case PDTA_Grab:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_Grab: 0x%lx\n", (long)&pd->Grab));
	    *(msg->opg_Storage)=(ULONG) &pd->Grab;
	    break;

	case PDTA_SourceMode:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_SourceMode: 0x%lx\n", (long)PMODE_V43));
	    *(msg->opg_Storage)=(ULONG) PMODE_V43;
	    break;

	case PDTA_DestMode:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_DestMode: 0x%lx\n", (long)pd->DestMode));
	    *(msg->opg_Storage)=(ULONG) pd->DestMode;
	    break;

	case PDTA_FreeSourceBitMap:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_FreeSourceBitMap: 0x%lx\n", (long)pd->FreeSource));
	    *(msg->opg_Storage)=(ULONG) pd->FreeSource;
	    break;

	case PDTA_UseFriendBitMap:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_UseFriendBitMap: 0x%lx\n", (long)pd->UseFriendBM));
	    *(msg->opg_Storage)=(ULONG) pd->UseFriendBM;
	    break;

	case PDTA_MaxDitherPens:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_MaxDitherPens: 0x%lx\n", (long)pd->MaxDitherPens));
	    *(msg->opg_Storage)=(ULONG) pd->MaxDitherPens;
	    break;

	case PDTA_DitherQuality:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_DitherQuality: 0x%lx\n", (long)pd->DitherQuality));
	    *(msg->opg_Storage)=(ULONG) pd->DitherQuality;
	    break;

	case PDTA_ScaleQuality:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_ScaleQuality: 0x%lx\n", (long)pd->ScaleQuality));
	    *(msg->opg_Storage)=(ULONG) pd->ScaleQuality;
	    break;

	case PDTA_DelayedRead:
	    DGS(bug("picture.datatype/OM_GET: Tag PDTA_DelayedRead: 0x%lx\n", (long)pd->DelayedRead));
	    *(msg->opg_Storage)=(ULONG) pd->DelayedRead;
	    break;

	case DTA_Methods:
	    DGS(bug("picture.datatype/OM_GET: Tag DTA_Methods: 0x%lx\n", (long)SupportedMethods));
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

IPTR Picture__GM_RENDER(struct IClass *cl, struct Gadget *g, struct gpRender *msg)
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
    ReleaseSemaphore(&(si->si_Lock));

    return TRUE;
}

/**************************************************************************************************/

IPTR Picture__GM_GOACTIVE(struct IClass *cl, struct Gadget *g, struct gpInput *msg)
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

IPTR Picture__GM_HANDLEINPUT(struct IClass *cl, struct Gadget *g, struct gpInput *msg)
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

IPTR Picture__GM_LAYOUT(struct IClass *cl, struct Gadget *g, struct gpLayout *msg)
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

IPTR Picture__DTM_ASYNCLAYOUT(struct IClass *cl, struct Gadget *g, struct gpLayout *msg)
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
	if( pd->Remap )
	{
	    /* determine destination screen depth */
	    if( !pd->DestScreen )
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
	    if( pd->SrcDepth > 8 )
	    {
		D(bug("picture.datatype/DTM_ASYNCLAYOUT: Remap=FALSE option only for colormapped source !\n"));
		ReleaseSemaphore(&si->si_Lock);   /* unlock object data */
		return FALSE;
	    }
	    if( pd->Scale )
	    {
		D(bug("picture.datatype/DTM_ASYNCLAYOUT: Scaling doesn't work with Remap=FALSE !\n"));
		ReleaseSemaphore(&si->si_Lock);   /* unlock object data */
		return FALSE;
	    }
	    pd->DestScreen = NULL;
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

IPTR Picture__DTM_PROCLAYOUT(struct IClass *cl, struct Gadget *g, struct gpLayout *msg)
{
    IPTR RetVal;

    NotifyAttrChanges((Object *) g, msg->gpl_GInfo, 0,
   				 GA_ID, g->GadgetID,
   				 DTA_Busy, TRUE,
   				 TAG_DONE);

    RetVal=DoSuperMethodA(cl, (Object *) g, (Msg) msg);

    return Picture__DTM_ASYNCLAYOUT(cl, g, msg);
}

/**************************************************************************************************/

IPTR Picture__PDTM_WRITEPIXELARRAY(struct IClass *cl, struct Gadget *g, struct pdtBlitPixelArray *msg)
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

    pd->Layouted = FALSE;	/* re-layout required */
    return TRUE;
}

/**************************************************************************************************/

IPTR Picture__PDTM_READPIXELARRAY(struct IClass *cl, struct Gadget *g, struct pdtBlitPixelArray *msg)
{
    struct Picture_Data *pd;

    int pixelformat;
    int pixelbytes;

    pd = (struct Picture_Data *) INST_DATA(cl, g);

    /* Do some checks first */
    if( !pd->SrcBuffer || !pd->DestMode )
    {
        D(bug("picture.datatype/DTM_READPIXELARRAY: No source buffer or wrong DestMode\n"));
	return FALSE;
    }
    pixelformat = (long)msg->pbpa_PixelFormat;
    D(bug("picture.datatype/DTM_READPIXELARRAY: Source/Dest Pixelformat %d / %ld\n", pixelformat, pd->SrcPixelFormat));

    if ( pixelformat == pd->SrcPixelFormat )
    {
	/* Copy picture data, as source pixmode = dest pixmode */
        long line, lines;
        APTR srcstart;
        APTR deststart;
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
	UBYTE r=0, g=0, b=0, a;
        long line, x, col;
	int srcpixelformat;
        APTR srcstart;
	UBYTE *srcptr;
        APTR deststart;
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

	a = 0;
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

IPTR Picture__PDTM_SCALE(struct IClass *cl, struct Gadget *g, struct pdtScale *msg)
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
    if( pd->SrcWidth == pd->DestWidth && pd->SrcHeight == pd->DestHeight )
	pd->Scale = FALSE;
    else
	pd->Scale = TRUE;

    xscale = (pd->SrcWidth << 16) / pd->DestWidth;
    yscale = (pd->SrcHeight << 16) / pd->DestHeight;
    if( msg->ps_Flags & PScale_KeepAspect )
    {
	xscale = yscale = MAX(xscale, yscale);
	pd->DestWidth = (pd->SrcWidth << 16) / xscale;
	pd->DestHeight = (pd->SrcHeight << 16) / yscale;
    }
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

IPTR Picture__DTM_FRAMEBOX(struct IClass *cl, struct Gadget *g, struct dtFrameBox *msg)
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

IPTR Picture__DTM_OBTAINDRAWINFO(struct IClass *cl, struct Gadget *g, struct opSet *msg)
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

IPTR Picture__DTM_DRAW(struct IClass *cl, struct Gadget *g, struct dtDraw *msg)
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

        BltBitMapRastPort( pd->DestBM,
                          SrcX,
                          SrcY,
                          msg->dtd_RPort,
                          DestX,
                          DestY,
                          SizeX,
                          SizeY,
                          0xC0);
        D(bug("picture.datatype/DTM_DRAW: Switched to image mode\n"));
        RetVal = TRUE;
    }

    return RetVal;
}

/**************************************************************************************************/

IPTR Picture__DTM_RELEASEDRAWINFO(struct IClass *cl, struct Gadget *g, struct dtReleaseDrawInfo *msg)
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
            RetVal=(IPTR) Picture__OM_NEW(cl, o, (struct opSet *) msg);
            break;
        }

        case OM_GET:
        {
            // DGS(bug("picture.datatype/DT_Dispatcher: Method OM_GET\n"));
            RetVal=(IPTR) Picture__OM_GET(cl, (struct Gadget *) o, (struct opGet *) msg);
            break;
        }

        case OM_SET:
        case OM_UPDATE:
        {
            DGS(bug("picture.datatype/DT_Dispatcher: Method %s\n", (msg->MethodID==OM_UPDATE) ? "OM_UPDATE" : "OM_SET"));
            RetVal=(IPTR) Picture__OM_SET(cl, (struct Gadget *) o, (struct opSet *) msg);
            break;
        }

        case OM_DISPOSE:
        {
            D(bug("picture.datatype/DT_Dispatcher: Method OM_DISPOSE\n"));
            RetVal=(IPTR) Picture__OM_DISPOSE(cl, o, (Msg) msg);
            break;
        }

        case GM_LAYOUT:
        {
            D(bug("picture.datatype/DT_Dispatcher: Method GM_LAYOUT\n"));
            RetVal=(IPTR) Picture__GM_LAYOUT(cl, (struct Gadget *) o, (struct gpLayout *) msg);
            break;
        }

    	case GM_GOACTIVE:
	{
            D(bug("picture.datatype/DT_Dispatcher: Method GM_GOACTIVE\n"));
	    RetVal = Picture__GM_GOACTIVE(cl, (struct Gadget *)o, (struct gpInput *)msg);
	    break;
	}
	   
	case GM_HANDLEINPUT:
	{
            D(bug("picture.datatype/DT_Dispatcher: Method GM_HANDLEINPUT\n"));
	    RetVal = Picture__GM_HANDLEINPUT(cl, (struct Gadget *)o, (struct gpInput *)msg);
	    break;
	}
	   
        case DTM_PROCLAYOUT:
        {
            D(bug("picture.datatype/DT_Dispatcher: Method DTM_PROCLAYOUT\n"));
            RetVal=(IPTR) Picture__DTM_PROCLAYOUT(cl, (struct Gadget *) o, (struct gpLayout *) msg);
	    break;
        }

        case DTM_ASYNCLAYOUT:
        {
            D(bug("picture.datatype/DT_Dispatcher: Method DTM_ASYNCLAYOUT\n"));
            RetVal=(IPTR) Picture__DTM_ASYNCLAYOUT(cl, (struct Gadget *) o, (struct gpLayout *) msg);
            break;
        }

        case GM_RENDER:
        {
            D(bug("picture.datatype/DT_Dispatcher: Method GM_RENDER\n"));
            RetVal=(IPTR) Picture__GM_RENDER(cl, (struct Gadget *) o, (struct gpRender *) msg);
            break;
        }

        case DTM_FRAMEBOX:
        {
            D(bug("picture.datatype/DT_Dispatcher: Method DTM_FRAMEBOX\n"));
            RetVal=(IPTR) Picture__DTM_FRAMEBOX(cl, (struct Gadget *) o, (struct dtFrameBox *) msg);
            break;
        }

        case DTM_OBTAINDRAWINFO:
        {
            D(bug("picture.datatype/DT_Dispatcher: Method DTM_OBTAINDRAWINFO\n"));
            RetVal=(IPTR) Picture_DTM_OBTAINDRAWINFO(cl, (struct Gadget *) o, (struct opSet *) msg);
            break;
        }

        case DTM_DRAW:
        {
            D(bug("picture.datatype/DT_Dispatcher: Method DTM_DRAW\n"));
            RetVal=(IPTR) Picture__DTM_DRAW(cl, (struct Gadget *) o, (struct dtDraw *) msg);
            break;
        }

        case DTM_RELEASEDRAWINFO:
        {
            D(bug("picture.datatype/DT_Dispatcher: Method DTM_RELEASEDRAWINFO\n"));
            RetVal=(IPTR) Picture__DTM_RELEASEDRAWINFO(cl, (struct Gadget *) o, (struct dtReleaseDrawInfo *) msg);
            break;
        }

        case PDTM_WRITEPIXELARRAY:
        {
            // D(bug("picture.datatype/DT_Dispatcher: Method PDTM_WRITEPIXELARRAY\n"));
            RetVal=(IPTR) Picture__PDTM_WRITEPIXELARRAY(cl, (struct Gadget *) o, (struct pdtBlitPixelArray *) msg);
            break;
        }

        case PDTM_READPIXELARRAY:
        {
            // D(bug("picture.datatype/DT_Dispatcher: Method PDTM_READPIXELARRAY\n"));
            RetVal=(IPTR) Picture__PDTM_READPIXELARRAY(cl, (struct Gadget *) o, (struct pdtBlitPixelArray *) msg);
            break;
        }

        case PDTM_SCALE:
        {
            D(bug("picture.datatype/DT_Dispatcher: Method PDTM_SCALE\n"));
            RetVal=(IPTR) Picture__PDTM_SCALE(cl, (struct Gadget *) o, (struct pdtScale *) msg);
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
