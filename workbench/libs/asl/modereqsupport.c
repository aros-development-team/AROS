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

void SMGetModes(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct SMUserData 	 *udata = (struct SMUserData *)ld->ld_UserData;	
    struct IntModeReq 	 *imreq = (struct IntModeReq *)ld->ld_IntReq;
    struct DisplayMode   *dispmode;
    struct DimensionInfo diminfo;
    struct DisplayInfo   dispinfo;
    struct NameInfo	 nameinfo;
    UBYTE		 name[DISPLAYNAMELEN + 1];
    
    ULONG 		displayid = INVALID_ID;
    
    while(INVALID_ID != (displayid = NextDisplayInfo(displayid)))
    {
        D(bug("SMGetModes: DisplayID 0x%8x\n"));
	
	if ((GetDisplayInfoData(NULL, (APTR)&diminfo , sizeof(diminfo) , DTAG_DIMS, displayid) > 0) &&
	    (GetDisplayInfoData(NULL, (APTR)&dispinfo, sizeof(dispinfo), DTAG_DISP, displayid) > 0))
	{
	    D(bug("SMGetModes: Got DimensionInfo and DisplayInfo\n"));
	    
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
	    
	    dispmode = AllocPooled(ld->ld_IntReq->ir_MemPool, sizeof(struct DisplayMode));
	    if (dispmode)
	    {
	        dispmode->dm_DimensionInfo = diminfo;
		dispmode->dm_PropertyFlags = dispinfo.PropertyFlags;
		dispmode->dm_Node.ln_Name  = PooledCloneString(name, NULL, ld->ld_IntReq->ir_MemPool, AslBase);
		SortInNode(imreq, &udata->ListviewList, &dispmode->dm_Node, (APTR)SMCompareNodes, AslBase);
	    }
	    
	} /* if diminfo and dispinfo could be retrieved */
	
    } /* while(INVALID_ID != (displayid = NextDisplayInfo(displayid))) */
    
    SetAttrs(udata->Listview, ASLLV_Labels, (IPTR)&udata->ListviewList,
                              TAG_DONE);

    if (!IsListEmpty(&udata->ListviewList))
    {
        SetAttrs(udata->Listview, ASLLV_Active, 0,
				  TAG_DONE);
    }
        
}


/*****************************************************************************************/

