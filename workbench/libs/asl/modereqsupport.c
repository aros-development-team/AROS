/*
    (C) 1995-97 AROS - The Amiga Research OS

    Desc:
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <graphics/modeid.h>
#include <graphics/displayinfo.h>
#include <graphics/monitor.h>
#include <stdio.h>
#include <string.h>
#include <clib/macros.h>

#include "asl_intern.h"
#include "modereqsupport.h"
#include "modereqhooks.h"
#include "layout.h"

#define SDEBUG 0
#define DEBUG 1
#define ADEBUG 1

#include <aros/debug.h>

/*****************************************************************************************/

static WORD SMCompareNodes(struct IntModeReq *imreq, struct DisplayMode *node1,
			   struct DisplayMode *node2, struct AslBase_intern *AslBase)
{
    return Stricmp(node1->dm_Node.ln_Name, node2->dm_Node.ln_Name);
}

/*****************************************************************************************/

LONG SMGetModes(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct SMUserData 	 *udata = (struct SMUserData *)ld->ld_UserData;	
    struct IntModeReq 	 *imreq = (struct IntModeReq *)ld->ld_IntReq;
    struct DisplayMode   *dispmode;
    struct DimensionInfo diminfo;
    struct DisplayInfo   dispinfo;
    struct NameInfo	 nameinfo;
    UBYTE		 name[DISPLAYNAMELEN + 1];    
    ULONG 		 displayid = INVALID_ID;
    
    while(INVALID_ID != (displayid = NextDisplayInfo(displayid)))
    {
        (bug("SMGetModes: DisplayID 0x%8x\n"));
	
	if ((GetDisplayInfoData(NULL, (APTR)&diminfo , sizeof(diminfo) , DTAG_DIMS, displayid) > 0) &&
	    (GetDisplayInfoData(NULL, (APTR)&dispinfo, sizeof(dispinfo), DTAG_DISP, displayid) > 0))
	{
	    D(bug("SMGetModes: Got DimensionInfo and DisplayInfo\n"));
	    
	    bug("SMGetModes: diminfo.displayid = 0x%8x\n", diminfo.Header.DisplayID);
	    
	    if (GetDisplayInfoData(NULL, (APTR)&nameinfo, sizeof(nameinfo), DTAG_NAME, displayid) > 0)
	    {
	        D(bug("SMGetModes: Got NameInfo. Name = \"%s\"\n", nameinfo.Name));
		
		strcpy(name, nameinfo.Name);
	    } else {
	        sprintf(name, "%ldx%ldx%ld",
			diminfo.Nominal.MaxX - diminfo.Nominal.MinX + 1,
			diminfo.Nominal.MaxY - diminfo.Nominal.MinY + 1,
			diminfo.MaxDepth);
		D(bug("SMGetModes: No NameInfo. Making by hand. Name = \"%s\"\n", name));
	    }
	    
	    if (diminfo.MaxDepth < imreq->ism_MinDepth) continue;
	    
	    if ((diminfo.MinRasterWidth  > imreq->ism_MaxWidth ) ||
	        (diminfo.MaxRasterWidth  < imreq->ism_MinWidth ) ||
		(diminfo.MinRasterHeight > imreq->ism_MaxHeight) ||
		(diminfo.MaxRasterHeight < imreq->ism_MinHeight)) continue;
		
	    if ((dispinfo.PropertyFlags   & imreq->ism_PropertyMask) !=
	        (imreq->ism_PropertyFlags & imreq->ism_PropertyMask)) continue;
	
	    if (imreq->ism_FilterFunc)
	    {
		if (CallHookPkt(imreq->ism_FilterFunc, ld->ld_Req, (APTR)displayid) == 0)
		    continue;     
	    }
	    
	    dispmode = AllocPooled(ld->ld_IntReq->ir_MemPool, sizeof(struct DisplayMode));
	    if (!dispmode) return ERROR_NO_FREE_STORE;
	    
	    dispmode->dm_DimensionInfo = diminfo;
	    dispmode->dm_PropertyFlags = dispinfo.PropertyFlags;
	    dispmode->dm_Node.ln_Name  = PooledCloneString(name, NULL, ld->ld_IntReq->ir_MemPool, AslBase);

	    SortInNode(imreq, &udata->ListviewList, &dispmode->dm_Node, (APTR)SMCompareNodes, AslBase);
	    
	} /* if diminfo and dispinfo could be retrieved */
	
    } /* while(INVALID_ID != (displayid = NextDisplayInfo(displayid))) */
    
    if (imreq->ism_CustomSMList)
    {
        struct DisplayMode *succ;
	
	ForeachNodeSafe(imreq->ism_CustomSMList, dispmode, succ)
	{
	    /* custom modes must have displayID from range 0xFFFF0000 .. 0xFFFFFFFF */
	    
	    if (succ->dm_DimensionInfo.Header.DisplayID < 0xFFFF0000) continue;
	    
	    Remove(&dispmode->dm_Node);
	    SortInNode(imreq, &udata->ListviewList, &dispmode->dm_Node, (APTR)SMCompareNodes, AslBase);
	}	
    }
    
    return IsListEmpty(&udata->ListviewList) ? ERROR_NO_MORE_ENTRIES : 0;
        
}


/*****************************************************************************************/

struct DisplayMode *SMGetActiveMode(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct SMUserData 	*udata = (struct SMUserData *)ld->ld_UserData;    
    IPTR		active;
    
    GetAttr(ASLLV_Active, udata->Listview, &active);
    
    return (struct DisplayMode *)FindListNode(&udata->ListviewList, active);
}

/*****************************************************************************************/

void SMChangeActiveLVItem(struct LayoutData *ld, WORD delta, UWORD quali, struct AslBase_intern *AslBase)
{
    struct SMUserData 	*udata = (struct SMUserData *)ld->ld_UserData;    
    IPTR 		active, total, visible;
   
    GetAttr(ASLLV_Active , udata->Listview, &active );
    GetAttr(ASLLV_Total  , udata->Listview, &total  );
    GetAttr(ASLLV_Visible, udata->Listview, &visible);
    
    if (quali & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
    {
        delta *= (visible - 1);
    }
    else if (quali & (IEQUALIFIER_LALT | IEQUALIFIER_RALT | IEQUALIFIER_CONTROL))
    {
        delta *= total;
    }

    active += delta;
    
    if (((LONG)active) < 0) active = 0;
    if (active >= total) active = total - 1;
    
    SMActivateMode(ld, active, AslBase);
    
}

/*****************************************************************************************/

UWORD SMGetOverscan(struct LayoutData *ld, struct DisplayMode *dispmode,
		    struct Rectangle **rect, struct AslBase_intern *AslBase)
{
    struct SMUserData 	*udata = (struct SMUserData *)ld->ld_UserData;    
    struct IntModeReq 	*imreq = (struct IntModeReq *)ld->ld_IntReq;
    UWORD		overscantype = imreq->ism_OverscanType;
    IPTR		val;
    
    if (imreq->ism_Flags & ISMF_DOOVERSCAN)
    {
        GetAttr(ASLCY_Active, udata->OverscanGadget, &val);
	overscantype = val + 1;	
    }
    
    if (rect)
    {
	*rect = &dispmode->dm_DimensionInfo.Nominal;

	if (overscantype == OSCAN_TEXT)
	{
            *rect = &dispmode->dm_DimensionInfo.TxtOScan;
	}
	else if (overscantype == OSCAN_STANDARD)
	{
            *rect = &dispmode->dm_DimensionInfo.StdOScan;
	}
	else if (overscantype == OSCAN_MAX)
	{
            *rect = &dispmode->dm_DimensionInfo.MaxOScan;
	}
	else if (overscantype == OSCAN_VIDEO)
	{
            *rect = &dispmode->dm_DimensionInfo.VideoOScan;
	}
    }
    
    return overscantype;
}

/*****************************************************************************************/

void SMFixValues(struct LayoutData *ld, struct DisplayMode *dispmode,
		 LONG *width, LONG *height, LONG *depth, struct AslBase_intern *AslBase)
{
    struct SMUserData 	*udata = (struct SMUserData *)ld->ld_UserData;    
    struct IntModeReq 	*imreq = (struct IntModeReq *)ld->ld_IntReq;
    struct Rectangle    *rect;
    struct TagItem	set_tags[] =
    {
        {STRINGA_LongVal , 0		},
	{TAG_DONE			}
    };
    
    SMGetOverscan(ld, dispmode, &rect, AslBase);
    
    if (width)
    {
        if (*width < imreq->ism_MinWidth)
	    *width = imreq->ism_MinWidth;
	else if (*width > imreq->ism_MaxWidth)
	    *width = imreq->ism_MaxWidth;
	
	if (*width < dispmode->dm_DimensionInfo.MinRasterWidth)
	    *width = dispmode->dm_DimensionInfo.MinRasterWidth;
	else if (*width > dispmode->dm_DimensionInfo.MaxRasterWidth)
	    *width = dispmode->dm_DimensionInfo.MaxRasterWidth;
	    
	if (imreq->ism_Flags & ISMF_DOWIDTH)
	{  
	    set_tags[0].ti_Data = *width;
	    
	    if (ld->ld_Window)
	    {
        	SetGadgetAttrsA((struct Gadget *)udata->WidthGadget, ld->ld_Window, NULL, set_tags);
	    } else {
        	SetAttrsA(udata->WidthGadget, set_tags);
	    }
	}
	
    } /* if (width) */

    if (height)
    {
        if (*height < imreq->ism_MinHeight)
	    *height = imreq->ism_MinHeight;
	else if (*height > imreq->ism_MaxHeight)
	    *height = imreq->ism_MaxHeight;

	if (*height < dispmode->dm_DimensionInfo.MinRasterHeight)
	    *height = dispmode->dm_DimensionInfo.MinRasterHeight;
	else if (*height > dispmode->dm_DimensionInfo.MaxRasterHeight)
	    *height = dispmode->dm_DimensionInfo.MaxRasterHeight;
	
	if (imreq->ism_Flags & ISMF_DOHEIGHT)
	{  
	    set_tags[0].ti_Data = *height;
	    
	    if (ld->ld_Window)
	    {
        	SetGadgetAttrsA((struct Gadget *)udata->HeightGadget, ld->ld_Window, NULL, set_tags);
	    } else {
        	SetAttrsA(udata->HeightGadget, set_tags);
	    }
	}
	
    } /* if (height) */
	
}

/*****************************************************************************************/

void SMActivateMode(struct LayoutData *ld, WORD which, struct AslBase_intern *AslBase)
{
    struct SMUserData 	*udata = (struct SMUserData *)ld->ld_UserData;    
    struct DisplayMode	*dispmode;
    struct Rectangle    *rect;
    struct TagItem 	set_tags[] =
    {
        {ASLLV_Active		, 0		},
	{ASLLV_MakeVisible	, 0		},
	{TAG_DONE				}
    };
    LONG		width, height;
    
    dispmode = (struct DisplayMode *)FindListNode(&udata->ListviewList, which);
    
    if (!dispmode) return; /* Should never happen */
    
    set_tags[0].ti_Data = set_tags[1].ti_Data = which;
    
    if (ld->ld_Window)
    {
        SetGadgetAttrsA((struct Gadget *)udata->Listview, ld->ld_Window, NULL, set_tags);
    } else {
        SetAttrsA(udata->Listview, set_tags);
    }
    
    SMGetOverscan(ld, dispmode, &rect,  AslBase);
    
    width  = rect->MaxX - rect->MinX + 1;
    height = rect->MaxY - rect->MinY + 1;
    
    SMFixValues(ld, dispmode, &width, &height, 0, AslBase);
    
}

/*****************************************************************************************/

void SMRestore(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct SMUserData 	*udata = (struct SMUserData *)ld->ld_UserData;    
    struct IntModeReq 	*imreq = (struct IntModeReq *)ld->ld_IntReq;
    struct DisplayMode	*dispmode;    
    LONG		i = 0, active = 0;
    
    ForeachNode(&udata->ListviewList, dispmode)
    {
        if (dispmode->dm_DimensionInfo.Header.DisplayID == imreq->ism_DisplayID)
	{
	    active = i;
	    break;
	}
	i++;
    }
    
    SMActivateMode(ld, active, AslBase);
}

/*****************************************************************************************/

LONG SMGetStringValue(struct LayoutData *ld, Object *obj, struct AslBase_intern *AslBase)
{
    IPTR val;
    
    ASSERT_VALID_PTR(obj);
    
    GetAttr(STRINGA_LongVal, obj, &val);
    
    return (LONG)val;
}

/*****************************************************************************************/

LONG SMGetWidth(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct SMUserData *udata = (struct SMUserData *)ld->ld_UserData;    
    
    ASSERT(udata->WidthGadget);
    
    return SMGetStringValue(ld, udata->WidthGadget, AslBase);
}

/*****************************************************************************************/

LONG SMGetHeight(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct SMUserData *udata = (struct SMUserData *)ld->ld_UserData;    
    
    ASSERT(udata->HeightGadget);
    
    return SMGetStringValue(ld, udata->HeightGadget, AslBase);
}

/*****************************************************************************************/

BOOL SMGetAutoScroll(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct SMUserData   *udata = (struct SMUserData *)ld->ld_UserData;  
    IPTR		active;
    
    ASSERT(udata->AutoScrollGadget);
    
    GetAttr(ASLCY_Active, udata->AutoScrollGadget, &active);
    
    return active ? TRUE : FALSE;
}

/*****************************************************************************************/
