/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/


#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include <exec/initializers.h>
#include <dos/dos.h>
#include <graphics/modeid.h>
#include <graphics/displayinfo.h>
#include <graphics/monitor.h>
#include <intuition/gadgetclass.h>
#include <stdio.h>
#include <string.h>
#include <clib/macros.h>

#include "asl_intern.h"
#include "modereqsupport.h"
#include "modereqhooks.h"
#include "layout.h"

#define CATCOMP_NUMBERS
#include "strings.h"

#define SDEBUG 0
#define DEBUG 0
//#define ADEBUG 0

#include <aros/debug.h>

/*****************************************************************************************/

static WORD SMCompareNodes(struct IntSMReq *ismreq, struct DisplayMode *node1,
			   struct DisplayMode *node2, struct AslBase_intern *AslBase)
{
    return Stricmp(node1->dm_Node.ln_Name, node2->dm_Node.ln_Name);
}

/*****************************************************************************************/

LONG SMGetModes(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct SMUserData 	 *udata = (struct SMUserData *)ld->ld_UserData;	
    struct IntSMReq 	 *ismreq = (struct IntSMReq *)ld->ld_IntReq;
    struct DisplayMode   *dispmode;
    struct DimensionInfo diminfo;
    struct DisplayInfo   dispinfo;
    struct NameInfo	 nameinfo;
    UBYTE		 name[DISPLAYNAMELEN + 1];    
    ULONG 		 displayid = INVALID_ID;
    
    while(INVALID_ID != (displayid = NextDisplayInfo(displayid)))
    {
        D(bug("SMGetModes: DisplayID 0x%8x\n", displayid));
	
	if ((GetDisplayInfoData(NULL, (APTR)&diminfo , sizeof(diminfo) , DTAG_DIMS, displayid) > 0) &&
	    (GetDisplayInfoData(NULL, (APTR)&dispinfo, sizeof(dispinfo), DTAG_DISP, displayid) > 0))
	{
	    D(bug("SMGetModes: Got DimensionInfo and DisplayInfo\n"));
	    
	    D(bug("SMGetModes: diminfo.displayid = 0x%8x\n", diminfo.Header.DisplayID));
	    
	    if (GetDisplayInfoData(NULL, (APTR)&nameinfo, sizeof(nameinfo), DTAG_NAME, displayid) > 0)
	    {
	        D(bug("SMGetModes: Got NameInfo. Name = \"%s\"\n", nameinfo.Name));
		
		strcpy(name, nameinfo.Name);
	    } else {
	        sprintf(name, "%dx%dx%d",
			diminfo.Nominal.MaxX - diminfo.Nominal.MinX + 1,
			diminfo.Nominal.MaxY - diminfo.Nominal.MinY + 1,
			diminfo.MaxDepth);
		D(bug("SMGetModes: No NameInfo. Making by hand. Name = \"%s\"\n", name));
	    }

	    if ((dispinfo.PropertyFlags   & ismreq->ism_PropertyMask) !=
	        (ismreq->ism_PropertyFlags & ismreq->ism_PropertyMask)) continue;
	    
	    if (diminfo.MaxDepth < ismreq->ism_MinDepth) continue;
	    if (diminfo.MaxDepth > 8)
	    {
	        if (ismreq->ism_MaxDepth < diminfo.MaxDepth) continue;
	    }
	    else
	    {
	        if (dispinfo.PropertyFlags & (DIPF_IS_HAM | DIPF_IS_EXTRAHALFBRITE))
		{
		    if (ismreq->ism_MaxDepth < 6) continue;
		}
	    }
	    
	    if ((diminfo.MinRasterWidth  > ismreq->ism_MaxWidth ) ||
	        (diminfo.MaxRasterWidth  < ismreq->ism_MinWidth ) ||
		(diminfo.MinRasterHeight > ismreq->ism_MaxHeight) ||
		(diminfo.MaxRasterHeight < ismreq->ism_MinHeight)) continue;
			
	    if (ismreq->ism_FilterFunc)
	    {
#ifdef __MORPHOS__
		{
		    ULONG ret;

		    REG_A4 = (ULONG)ismreq->ism_IntReq.ir_BasePtr;	/* Compatability */
		    REG_A0 = (ULONG)ismreq->ism_FilterFunc;
		    REG_A2 = (ULONG)ld->ld_Req;
		    REG_A1 = (ULONG)displayid;
		    ret = (*MyEmulHandle->EmulCallDirect68k)(ismreq->ism_FilterFunc->h_Entry);

		    if (ret == 0)
			continue;
		}
#else
		if (CallHookPkt(ismreq->ism_FilterFunc, ld->ld_Req, (APTR)displayid) == 0)
		    continue;     
#endif
	    }
	    
	    dispmode = AllocPooled(ld->ld_IntReq->ir_MemPool, sizeof(struct DisplayMode));
	    if (!dispmode) return ERROR_NO_FREE_STORE;
	    
	    diminfo.Header.DisplayID = displayid; /* AROS bug? */
	    
	    dispmode->dm_DimensionInfo = diminfo;
	    dispmode->dm_PropertyFlags = dispinfo.PropertyFlags;
	    dispmode->dm_Node.ln_Name  = PooledCloneString(name, NULL, ld->ld_IntReq->ir_MemPool, AslBase);

	    SortInNode(ismreq, &udata->ListviewList, &dispmode->dm_Node, (APTR)SMCompareNodes, AslBase);
	    
	} /* if diminfo and dispinfo could be retrieved */
	
    } /* while(INVALID_ID != (displayid = NextDisplayInfo(displayid))) */
    
    if (ismreq->ism_CustomSMList)
    {
        struct DisplayMode *succ;
	
	ForeachNodeSafe(ismreq->ism_CustomSMList, dispmode, succ)
	{
	    /* custom modes must have displayID from range 0xFFFF0000 .. 0xFFFFFFFF */
	    
	    if (dispmode->dm_DimensionInfo.Header.DisplayID < 0xFFFF0000) continue;
	    
	    Remove(&dispmode->dm_Node);
	    SortInNode(ismreq, &udata->ListviewList, &dispmode->dm_Node, (APTR)SMCompareNodes, AslBase);
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
    
    SMActivateMode(ld, active, 0, AslBase);
    
}

/*****************************************************************************************/

UWORD SMGetOverscan(struct LayoutData *ld, struct DisplayMode *dispmode,
		    struct Rectangle **rect, struct AslBase_intern *AslBase)
{
    struct SMUserData 	*udata = (struct SMUserData *)ld->ld_UserData;    
    struct IntSMReq 	*ismreq = (struct IntSMReq *)ld->ld_IntReq;
    UWORD		overscantype = ismreq->ism_OverscanType;
    IPTR		val;
    
    if (ismreq->ism_Flags & ISMF_DOOVERSCAN)
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
    struct IntSMReq 	*ismreq = (struct IntSMReq *)ld->ld_IntReq;
    struct Rectangle    *rect;
    struct TagItem	set_tags[] =
    {
        {STRINGA_LongVal , 0		},
	{TAG_DONE			}
    };
    
    SMGetOverscan(ld, dispmode, &rect, AslBase);
    
    if (width)
    {
        if (*width < ismreq->ism_MinWidth)
	    *width = ismreq->ism_MinWidth;
	else if (*width > ismreq->ism_MaxWidth)
	    *width = ismreq->ism_MaxWidth;
	
	if (*width < dispmode->dm_DimensionInfo.MinRasterWidth)
	    *width = dispmode->dm_DimensionInfo.MinRasterWidth;
	else if (*width > dispmode->dm_DimensionInfo.MaxRasterWidth)
	    *width = dispmode->dm_DimensionInfo.MaxRasterWidth;
	    
	if (ismreq->ism_Flags & ISMF_DOWIDTH)
	{  
	    set_tags[0].ti_Data = *width;
	    
            SetGadgetAttrsA((struct Gadget *)udata->WidthGadget, ld->ld_Window, NULL, set_tags);
	}
	
    } /* if (width) */

    if (height)
    {
        if (*height < ismreq->ism_MinHeight)
	    *height = ismreq->ism_MinHeight;
	else if (*height > ismreq->ism_MaxHeight)
	    *height = ismreq->ism_MaxHeight;

	if (*height < dispmode->dm_DimensionInfo.MinRasterHeight)
	    *height = dispmode->dm_DimensionInfo.MinRasterHeight;
	else if (*height > dispmode->dm_DimensionInfo.MaxRasterHeight)
	    *height = dispmode->dm_DimensionInfo.MaxRasterHeight;
	
	if (ismreq->ism_Flags & ISMF_DOHEIGHT)
	{  
	    set_tags[0].ti_Data = *height;
	    
            SetGadgetAttrsA((struct Gadget *)udata->HeightGadget, ld->ld_Window, NULL, set_tags);
	}
	
    } /* if (height) */
	
}

/*****************************************************************************************/

void SMActivateMode(struct LayoutData *ld, WORD which, LONG depth, struct AslBase_intern *AslBase)
{
    struct SMUserData 	*udata = (struct SMUserData *)ld->ld_UserData;    
    struct IntSMReq 	*ismreq = (struct IntSMReq *)ld->ld_IntReq;
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
    
    SetGadgetAttrsA((struct Gadget *)udata->Listview, ld->ld_Window, NULL, set_tags);
     
    if (ismreq->ism_Flags & ISMF_DODEPTH)
    {   
        STRPTR	*array = udata->Colorarray;
	STRPTR 	text = udata->Colortext;
	WORD	i = 0, min, max, bestmatch;
	
	set_tags[0].ti_Tag  = ASLCY_Labels;
	set_tags[0].ti_Data = 0;
	set_tags[1].ti_Tag  = ASLCY_Active;
	set_tags[2].ti_Data = 0;

        if (depth == 0) depth = SMGetDepth(ld, 0, AslBase);
	
        SetGadgetAttrsA((struct Gadget *)udata->DepthGadget, ld->ld_Window, NULL, set_tags);
	
	if (dispmode->dm_DimensionInfo.MaxDepth > 8)
	{
	    sprintf(text, "%ld", 1L << dispmode->dm_DimensionInfo.MaxDepth);
	    *array++ = text;
	    
	    udata->ColorDepth[0]     =
	    udata->RealColorDepth[0] = dispmode->dm_DimensionInfo.MaxDepth;
	    
	    udata->NumColorEntries = 1;
	    
	}
	else if (dispmode->dm_PropertyFlags & DIPF_IS_EXTRAHALFBRITE)
	{
	    *array++ = "64";

	    udata->ColorDepth[0]     =
	    udata->RealColorDepth[0] = 6;
	    
	    udata->NumColorEntries = 1;	    
	}
	else if (dispmode->dm_PropertyFlags & DIPF_IS_HAM)
	{
	    udata->NumColorEntries = 0;
	    
	    if (ismreq->ism_MinDepth <= 6)
	    {
		*array++ = "4096";
		
		udata->ColorDepth[udata->NumColorEntries]     = 6;
		udata->RealColorDepth[udata->NumColorEntries] = 12;
		
		udata->NumColorEntries++;
	    }
	    
	    if ((dispmode->dm_DimensionInfo.MaxDepth == 8) &&
	        (ismreq->ism_MaxDepth >= 8))
	    {
	        *array++ = "16777216";
		
		udata->ColorDepth[udata->NumColorEntries]     = 8;
		udata->RealColorDepth[udata->NumColorEntries] = 24;
		
		udata->NumColorEntries++;
	    }
	}
	else
	{
	    max = ismreq->ism_MaxDepth;
	    if (dispmode->dm_DimensionInfo.MaxDepth < max)
	        max = dispmode->dm_DimensionInfo.MaxDepth;
		
	    min = ismreq->ism_MinDepth;
	    if (min > max) max = min;
	    
	    for(i = min; i <= max; i++)
	    {
	        sprintf(text, "%ld", 1L << i);
		*array++ = text;
		
		udata->ColorDepth[i - min]     =
		udata->RealColorDepth[i - min] = i;
		
		text += strlen(text) + 1;
	    }
	    
	    udata->NumColorEntries = max - min + 1;
	    
	}
	
	*array++ = NULL;
	
	set_tags[0].ti_Data = (IPTR)udata->Colorarray;
	
	bestmatch = 1000;
	for(i = 0; i < udata->NumColorEntries; i++)
	{
	    WORD match = depth - udata->ColorDepth[i];
	    
	    if (match < 0) match = -match;
	    
	    if (match < bestmatch)
	    {
	        bestmatch = match;
		set_tags[1].ti_Data = i;
	    }
	}
	
        SetGadgetAttrsA((struct Gadget *)udata->DepthGadget, ld->ld_Window, NULL, set_tags);
	
    } /* if (ismreq->ism_Flags & ISMF_DODEPTH) */
    
    SMGetOverscan(ld, dispmode, &rect,  AslBase);

    width  = rect->MaxX - rect->MinX + 1;
    height = rect->MaxY - rect->MinY + 1;
    
    SMFixValues(ld, dispmode, &width, &height, 0, AslBase);
    
    SMRefreshPropertyWindow(ld, dispmode, AslBase);
}

/*****************************************************************************************/

void SMRestore(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct SMUserData 	*udata = (struct SMUserData *)ld->ld_UserData;    
    struct IntSMReq 	*ismreq = (struct IntSMReq *)ld->ld_IntReq;
    struct DisplayMode	*dispmode;
    LONG		i = 0, active = 0;
    LONG		width = ismreq->ism_DisplayWidth;
    LONG		height = ismreq->ism_DisplayHeight;
    
    /* the order of the function calls is important! */
    
    if (ismreq->ism_Flags & ISMF_DOOVERSCAN)
    {
        SMSetOverscan(ld, ismreq->ism_OverscanType, AslBase);
    }
    
    if (ismreq->ism_Flags & ISMF_DOAUTOSCROLL)
    {
        SMSetAutoScroll(ld, ismreq->ism_AutoScroll, AslBase);
    }
    
    ForeachNode(&udata->ListviewList, dispmode)
    {
        if (dispmode->dm_DimensionInfo.Header.DisplayID == ismreq->ism_DisplayID)
	{
	    active = i;
	    break;
	}
	i++;
    }
    
    SMActivateMode(ld, active, ismreq->ism_DisplayDepth, AslBase);

    dispmode = SMGetActiveMode(ld, AslBase);    
    ASSERT_VALID_PTR(dispmode);
        
    SMFixValues(ld, dispmode, &width, &height, 0, AslBase);
  
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

LONG SMGetDepth(struct LayoutData *ld, LONG *realdepth, struct AslBase_intern *AslBase)
{
    struct SMUserData   *udata = (struct SMUserData *)ld->ld_UserData;  
    IPTR		active;
    
    ASSERT(udata->DepthGadget);
    
    GetAttr(ASLCY_Active, udata->DepthGadget, &active);
    
    if (realdepth) *realdepth = udata->RealColorDepth[active];
    
    return udata->ColorDepth[active];
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

void SMSetDepth(struct LayoutData *ld, UWORD id, struct AslBase_intern *AslBase)
{
    struct SMUserData   *udata = (struct SMUserData *)ld->ld_UserData;  
    struct TagItem	set_tags[] =
    {
        {ASLCY_Active	, id		},
	{TAG_DONE		   	}
    };
    
    ASSERT(udata->DepthGadget);
    
    SetGadgetAttrsA((struct Gadget *)udata->DepthGadget, ld->ld_Window, NULL, set_tags);
    
}

/*****************************************************************************************/

void SMSetOverscan(struct LayoutData *ld, UWORD oscan, struct AslBase_intern *AslBase)
{
    struct SMUserData   *udata = (struct SMUserData *)ld->ld_UserData;  
    struct TagItem	set_tags[] =
    {
        {ASLCY_Active	, oscan - 1	},
	{TAG_DONE		   	}
    };
    
    ASSERT(udata->OverscanGadget);
    
    SetGadgetAttrsA((struct Gadget *)udata->OverscanGadget, ld->ld_Window, NULL, set_tags);
    
}

/*****************************************************************************************/

void SMSetAutoScroll(struct LayoutData *ld, BOOL onoff, struct AslBase_intern *AslBase)
{
    struct SMUserData   *udata = (struct SMUserData *)ld->ld_UserData;  
    struct TagItem	set_tags[] =
    {
        {ASLCY_Active	, onoff		},
	{TAG_DONE		   	}
    };
    
    ASSERT(udata->AutoScrollGadget);
    
    SetGadgetAttrsA((struct Gadget *)udata->AutoScrollGadget, ld->ld_Window, NULL, set_tags);
    
}

/*****************************************************************************************/

void SMOpenPropertyWindow(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    static WORD propertyids[] =
    {
    	MSG_MODEREQ_PROPERTIES_NOTWB,
    	MSG_MODEREQ_PROPERTIES_NOTGENLOCK,
    	MSG_MODEREQ_PROPERTIES_NOTDRAG,
    	MSG_MODEREQ_PROPERTIES_HAM,
    	MSG_MODEREQ_PROPERTIES_EHB,
    	MSG_MODEREQ_PROPERTIES_LACE,
    	MSG_MODEREQ_PROPERTIES_ECS,
    	MSG_MODEREQ_PROPERTIES_WB,
    	MSG_MODEREQ_PROPERTIES_GENLOCK,
    	MSG_MODEREQ_PROPERTIES_DRAG,
    	MSG_MODEREQ_PROPERTIES_DPFPRI2,
    	MSG_MODEREQ_PROPERTIES_REFRESHRATE,
	-1
    };
    
    struct SMUserData 	*udata = (struct SMUserData *)ld->ld_UserData;    
    struct IntSMReq 	*ismreq = (struct IntSMReq *)ld->ld_IntReq;
    struct DisplayMode	*dispmode;
    STRPTR  	    	*properties = (STRPTR *)&ismreq->ism_PropertyList_NotWB;
    WORD		i, x, y, w, h;
      
    if (ld->ld_Window2) return;
    
    for(i = 0; propertyids[i] != -1; i++)
    {
    	properties[i] = GetString(propertyids[i], GetIR(ismreq)->ir_Catalog, AslBase);
    }
    
    x = ld->ld_Window->LeftEdge + ismreq->ism_InfoLeftEdge;
    y = ld->ld_Window->TopEdge  + ismreq->ism_InfoTopEdge;
    
    i = (&SREQ_LAST_PROPERTY_ITEM(ismreq) - &SREQ_FIRST_PROPERTY_ITEM(ismreq)) /
        sizeof(STRPTR) + 1;

    
    w = BiggestTextLength(&SREQ_FIRST_PROPERTY_ITEM(ismreq),
			  i,
			  &(ld->ld_DummyRP),
			  AslBase);
    
    w += OUTERSPACINGX * 2 + BORDERLVSPACINGX * 2 + BORDERLVITEMSPACINGX * 2;
    
    h = SREQ_MAX_PROPERTIES * (ld->ld_Font->tf_YSize + BORDERLVITEMSPACINGY * 2);
    
    h += OUTERSPACINGY * 2 + BORDERLVSPACINGY * 2;
    
    {
	struct TagItem	lv_tags[] =
	{
	    {GA_Left		, ld->ld_WBorLeft + OUTERSPACINGX	},
	    {GA_Top		, ld->ld_WBorTop + OUTERSPACINGY	},
	    {GA_Width		, w - OUTERSPACINGX * 2			},
	    {GA_Height		, h - OUTERSPACINGY * 2			},
	    {GA_UserData	, (IPTR)ld				},
	    {ASLLV_ReadOnly	, TRUE					},
	    {ASLLV_Font     	, (IPTR)ld->ld_Font 	    	    	},
	    {TAG_DONE							}
	};
	
	udata->PropertyGadget = NewObjectA(AslBase->asllistviewclass, NULL, lv_tags);
	if (!udata->PropertyGadget) return;
	
    }
    
    {
    	struct TagItem	win_tags[] =
	{
	    {WA_CustomScreen	, (IPTR)ld->ld_Screen	    	},
	    {WA_Title		, 0 	    		    	},
	    {WA_Left		, x			    	},
	    {WA_Top		, y			    	},
	    {WA_InnerWidth	, w			    	},
	    {WA_InnerHeight	, h			    	},
	    {WA_AutoAdjust	, TRUE			    	},
	    {WA_CloseGadget	, TRUE			    	},
	    {WA_DepthGadget	, TRUE			    	},
	    {WA_DragBar		, TRUE			    	},
	    {WA_SimpleRefresh	, TRUE			    	},
	    {WA_NoCareRefresh	, TRUE			    	},
	    {WA_IDCMP		, 0			    	},
	    {WA_Gadgets		, (IPTR)udata->PropertyGadget	},
	    {TAG_DONE						}
	};
	
	win_tags[1].ti_Data = (IPTR)GetString(MSG_MODEREQ_PROPERTIES_TITLE, GetIR(ismreq)->ir_Catalog, AslBase);

	ld->ld_Window2 = OpenWindowTagList(0, win_tags);
	if (!ld->ld_Window2)
	{
	     DisposeObject(udata->PropertyGadget);
	     udata->PropertyGadget = 0;
	     return;
	}
	
	ld->ld_Window2->UserPort = ld->ld_Window->UserPort;	
	ModifyIDCMP(ld->ld_Window2, IDCMP_CLOSEWINDOW | IDCMP_VANILLAKEY);
    }
    
    dispmode = SMGetActiveMode(ld, AslBase);
    ASSERT_VALID_PTR(dispmode);
    
    SMRefreshPropertyWindow(ld, dispmode, AslBase);
}

/*****************************************************************************************/

void SMClosePropertyWindow(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct SMUserData 	*udata = (struct SMUserData *)ld->ld_UserData;    
    struct IntSMReq 	*ismreq = (struct IntSMReq *)ld->ld_IntReq;

    if (!ld->ld_Window2) return;
    
    RemoveGadget(ld->ld_Window2, (struct Gadget *)udata->PropertyGadget);
    DisposeObject(udata->PropertyGadget);
    udata->PropertyGadget = 0;

    ismreq->ism_InfoLeftEdge = ld->ld_Window2->LeftEdge - ld->ld_Window->LeftEdge;
    ismreq->ism_InfoTopEdge  = ld->ld_Window2->TopEdge  - ld->ld_Window->TopEdge;
    
    CloseWindowSafely(ld->ld_Window2, AslBase);
    
    ld->ld_Window2 = 0;
}

/*****************************************************************************************/

ULONG SMHandlePropertyEvents(struct LayoutData *ld, struct IntuiMessage *imsg, struct AslBase_intern *AslBase)
{ 
    switch(imsg->Class)
    {
        case IDCMP_VANILLAKEY:
	    if (imsg->Code != 27) break;
	    /* fall through */
	    
	case IDCMP_CLOSEWINDOW:
	    SMClosePropertyWindow(ld, AslBase);
	    break;
	    
    } /* switch(imsg->Class) */
    
    return GHRET_OK;   
}

/*****************************************************************************************/

void SMRefreshPropertyWindow(struct LayoutData *ld, struct DisplayMode *dispmode, struct AslBase_intern *AslBase)
{
    struct SMUserData 	*udata = (struct SMUserData *)ld->ld_UserData;    
    struct IntSMReq 	*ismreq = (struct IntSMReq *)ld->ld_IntReq;
    struct TagItem	set_tags[] =
    {
        {ASLLV_Labels,	0	},
	{TAG_DONE		}
    };
    struct Node 	*node = udata->PropertyNodes;

    #define OF(x) (ULONG)(OFFSET(IntSMReq, ism_PropertyList_ ## x))
    
    static struct propertyinfo
    {
        ULONG mask;
	ULONG flags;
	ULONG offset;
    } *pi, pitable [] = 
    {
        {DIPF_IS_WB		, 0			, OF(NotWB)		},
	{DIPF_IS_GENLOCK	, 0			, OF(NotGenlock)	},
	{DIPF_IS_DRAGGABLE	, 0			, OF(NotDraggable)	},
	{DIPF_IS_HAM		, DIPF_IS_HAM		, OF(HAM)		},
	{DIPF_IS_EXTRAHALFBRITE , DIPF_IS_EXTRAHALFBRITE, OF(EHB)		},
	{DIPF_IS_LACE		, DIPF_IS_LACE		, OF(Interlace)		},
	{DIPF_IS_ECS		, DIPF_IS_ECS		, OF(ECS)		},
	{DIPF_IS_WB		, DIPF_IS_WB		, OF(WB)		},
	{DIPF_IS_GENLOCK	, DIPF_IS_GENLOCK	, OF(Genlock)		},
	{DIPF_IS_DRAGGABLE	, DIPF_IS_DRAGGABLE	, OF(Draggable)		},
	{DIPF_IS_PF2PRI		, DIPF_IS_PF2PRI	, OF(DPFPri2)		},
	{0			, 0			, 0			}	
    };
    
    if (!ld->ld_Window2) return;
    
    SetGadgetAttrsA((struct Gadget *)udata->PropertyGadget, ld->ld_Window2, NULL, set_tags);
    
    NEWLIST(&udata->PropertyList);
    
    for(pi = pitable; pi->mask != 0; pi++)
    {
        if ((dispmode->dm_PropertyFlags & pi->mask) == pi->flags)
	{
	    node->ln_Name = *(STRPTR *)(((UBYTE *)ismreq) + pi->offset);
	    AddTail(&udata->PropertyList, node);
	    node++;
	}
    }
    set_tags[0].ti_Data = (IPTR)&udata->PropertyList;
    
    SetGadgetAttrsA((struct Gadget *)udata->PropertyGadget, ld->ld_Window2, NULL, set_tags);
    
}

/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/











