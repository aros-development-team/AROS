/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: File requester specific code.
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE 1

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/boopsi.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <intuition/screens.h>
#include <intuition/icclass.h>
#include <graphics/displayinfo.h>
#include <graphics/modeid.h>
#include <graphics/monitor.h>

#include <graphics/gfx.h>
#include <libraries/gadtools.h>
#include <workbench/startup.h>
#include <string.h>
#include <stdio.h>

#include "asl_intern.h"
#include "modereqhooks.h"
#include "modereqsupport.h"
#include "layout.h"
#include "coolimages.h"

#define SDEBUG 0
#define DEBUG 0

#include <aros/debug.h>

/*****************************************************************************************/

STATIC BOOL  SMGadInit(struct LayoutData *, struct AslBase_intern *);
STATIC BOOL  SMGadLayout(struct LayoutData *, struct AslBase_intern *);
STATIC VOID  SMGadCleanup(struct LayoutData *, struct AslBase_intern *);
STATIC ULONG SMHandleEvents(struct LayoutData *, struct AslBase_intern *);
STATIC ULONG SMGetSelectedMode(struct LayoutData *, struct AslBase_intern *AslBase);

/*****************************************************************************************/

#define ID_BUTOK	ID_MAINBUTTON_OK
#define ID_BUTCANCEL	ID_MAINBUTTON_CANCEL

#define ID_LISTVIEW	1
#define ID_OVERSCAN     2
#define ID_WIDTH	3
#define ID_HEIGHT       4
#define ID_COLORS       5
#define ID_AUTOSCROLL   6

#undef NUMBUTS
#define NUMBUTS 2L

#define CLASS_ASLBASE ((struct AslBase_intern *)cl->cl_UserData)
#define HOOK_ASLBASE  ((struct AslBase_intern *)hook->h_Data)


/*****************************************************************************************/

AROS_UFH3(VOID, SMTagHook,
    AROS_UFHA(struct Hook *,            hook,           A0),
    AROS_UFHA(struct ParseTagArgs *,    pta,            A2),
    AROS_UFHA(struct AslBase_intern *,  AslBase,        A1)
)
{
    struct TagItem 	*tag, *tstate;
    struct IntModeReq 	*imreq;
    
    EnterFunc(bug("SMTagHook(hook=%p, pta=%p)\n", hook, pta));

    imreq = (struct IntModeReq *)pta->pta_IntReq;

    tstate = pta->pta_Tags;
    while ((tag = NextTagItem((const struct TagItem **)&tstate)) != NULL)
    {
	IPTR tidata = tag->ti_Data;
	
	switch (tag->ti_Tag)
	{
	    case ASLSM_CustomSMList:
	        imreq->ism_CustomSMList = (struct List *)tidata;
		break;

	    case ASLSM_FilterFunc:
	        imreq->ism_FilterFunc = (struct Hook *)tidata;
		break;
				
            case ASLSM_DoAutoScroll:
	        if (tidata)
		    imreq->ism_Flags |= ISMF_DOAUTOSCROLL;
		else
		    imreq->ism_Flags &= ~ISMF_DOAUTOSCROLL;
		break;

	    case ASLSM_DoDepth:
	        if (tidata)
		    imreq->ism_Flags |= ISMF_DODEPTH;
		else
		    imreq->ism_Flags &= ~ISMF_DODEPTH;
		break;

            case ASLSM_DoWidth:
	        if (tidata)
		    imreq->ism_Flags |= ISMF_DOWIDTH;
		else
		    imreq->ism_Flags &= ~ISMF_DOWIDTH;
		break;
		
            case ASLSM_DoHeight:
	        if (tidata)
		    imreq->ism_Flags |= ISMF_DOHEIGHT;
		else
		    imreq->ism_Flags &= ~ISMF_DOHEIGHT;
		break;

            case ASLSM_DoOverscanType:
	        if (tidata)
		    imreq->ism_Flags |= ISMF_DOOVERSCAN;
		else
		    imreq->ism_Flags &= ~ISMF_DOOVERSCAN;
		break;
		
	    case ASLSM_UserData:
		((struct ScreenModeRequester *)pta->pta_Req)->sm_UserData = (APTR)tidata;		break;

	    case ASLSM_InitialAutoScroll:
	        imreq->ism_AutoScroll = tidata ? TRUE : FALSE;
		break;
		
	    case ASLSM_InitialDisplayDepth:
	        imreq->ism_DisplayDepth = tidata;
		break;
	
	    case ASLSM_InitialDisplayWidth:
	        imreq->ism_DisplayWidth = tidata;
		break;

            case ASLSM_InitialDisplayHeight:
	        imreq->ism_DisplayHeight = tidata;
		break;
	
	    case ASLSM_InitialDisplayID:
	        imreq->ism_DisplayID = tidata;
		break;
		
  	    case ASLSM_InitialInfoLeftEdge:
	        imreq->ism_InfoLeftEdge = tidata;
		break;
		
	    case ASLSM_InitialInfoTopEdge:
	        imreq->ism_InfoTopEdge = tidata;
		break;
		
	    case ASLSM_InitialInfoOpened:
	        imreq->ism_InfoOpened = tidata ? TRUE : FALSE;
		break;
	
	    case ASLSM_InitialOverscanType:
	        imreq->ism_OverscanType = (((LONG)tidata >= OSCAN_TEXT) &&
					   ((LONG)tidata <= OSCAN_VIDEO)) ? tidata: OSCAN_TEXT;
		break;
	
	    case ASLSM_MinWidth:
	        imreq->ism_MinWidth = tidata;
		break;
		
	    case ASLSM_MaxWidth:
	        imreq->ism_MaxWidth = tidata;
		break;
		
	    case ASLSM_MinHeight:
	        imreq->ism_MinHeight = tidata;
		break;
	
	    case ASLSM_MaxHeight:
	        imreq->ism_MaxHeight = tidata;
		break;
	
	    case ASLSM_MinDepth:
	        imreq->ism_MinDepth = tidata;
		break;
		
	    case ASLSM_MaxDepth:
	        imreq->ism_MaxDepth = tidata;
		break;
		
	    case ASLSM_PropertyFlags:
	        imreq->ism_PropertyFlags = tidata;
		break;
		
	    case ASLSM_PropertyMask:
	        imreq->ism_PropertyMask = tidata;
		break;
		
	    default:
		break;
		
	} /* switch (tag->ti_Tag) */

    } /* while ((tag = NextTagItem(&tstate)) != 0) */

    if (imreq->ism_MinDepth < 1) imreq->ism_MinDepth = 1;

    if (imreq->ism_MaxDepth < imreq->ism_MinDepth)
        imreq->ism_MaxDepth = imreq->ism_MinDepth;
	    
    if (imreq->ism_DisplayDepth < imreq->ism_MinDepth)
        imreq->ism_DisplayDepth = imreq->ism_MinDepth;
	
    if (imreq->ism_DisplayDepth > imreq->ism_MaxDepth)
        imreq->ism_DisplayDepth = imreq->ism_MaxDepth;
	
    ReturnVoid("SMTagHook");
}

/*****************************************************************************************/

AROS_UFH3(ULONG, SMGadgetryHook,
    AROS_UFHA(struct Hook *,            hook,           A0),
    AROS_UFHA(struct LayoutData *,      ld,             A2),
    AROS_UFHA(struct AslBase_intern *,  AslBase,        A1)
)
{
    ULONG retval;

    switch (ld->ld_Command)
    {
	case LDCMD_INIT:
	    retval = (ULONG)SMGadInit(ld, ASLB(AslBase));
	    break;

	case LDCMD_LAYOUT:
	    retval = (ULONG)SMGadLayout(ld, ASLB(AslBase));
	    break;

	case LDCMD_HANDLEEVENTS:
	    retval = (ULONG)SMHandleEvents(ld, ASLB(AslBase));
	    break;

	case LDCMD_CLEANUP:
	    SMGadCleanup(ld, ASLB(AslBase));
	    retval = GHRET_OK;
	    break;

	default:
	    retval = GHRET_FAIL;
	    break;
    }

    return (retval);
}

/*****************************************************************************************/

struct ButtonInfo
{
    WORD 			gadid;  
    char 			*text;
    const struct CoolImage 	*coolimage;
    Object 			**objvar;
};

STATIC BOOL SMGadInit(struct LayoutData *ld, struct AslBase_intern *AslBase)
{    
    struct SMUserData 	*udata = ld->ld_UserData;
    struct IntModeReq 	*imreq = (struct IntModeReq *)ld->ld_IntReq;
    STRPTR 		str[6];
    struct ButtonInfo 	bi[NUMBUTS] =
    {
        { ID_BUTOK	, GetIR(imreq)->ir_PositiveText , &cool_monitorimage, &udata->OKBut	 },
	{ ID_BUTCANCEL  , GetIR(imreq)->ir_NegativeText , &cool_cancelimage , &udata->CancelBut  }
    };
    Object 		*gad;
    LONG		error;
    WORD 		gadrows, x, y, w, h, i, y2;
    
    
    NEWLIST(&udata->ListviewList);

    error = SMGetModes(ld, AslBase);
    if (error) goto failure;
    
    error = ERROR_NO_FREE_STORE;
    
    /* calc. min. size */
    
    w = 0;
    for(i = 0; i < NUMBUTS; i++)
    {
        x = TextLength(&ld->ld_DummyRP, bi[i].text, strlen(bi[i].text));

#if SREQ_COOL_BUTTONS
	if (ld->ld_TrueColor)
	{
	    x += IMAGEBUTTONEXTRAWIDTH + bi[i].coolimage->width;
	}
#endif

	if (x > w) w = x;	
    }
    
    udata->ButWidth = w + BUTTONEXTRAWIDTH;

    ld->ld_ButWidth = udata->ButWidth;
    ld->ld_NumButtons = 4;
    
#if SREQ_COOL_BUTTONS
    y  = BUTTONEXTRAHEIGHT + ld->ld_Font->tf_YSize;
    if (ld->ld_TrueColor)
    {
        y2 = IMAGEBUTTONEXTRAHEIGHT + DEF_COOLIMAGEHEIGHT;
    } else {
        y2 = 0;
    }
    udata->ButHeight = (y > y2) ? y : y2;
#else
    udata->ButHeight = BUTTONEXTRAHEIGHT + ld->ld_Font->tf_YSize;
#endif
    
    gadrows = 1; /* button row  */
    if (imreq->ism_Flags & ISMF_DOOVERSCAN) gadrows++;
    if (imreq->ism_Flags & ISMF_DOWIDTH) gadrows++;
    if (imreq->ism_Flags & ISMF_DOHEIGHT) gadrows++;
    if (imreq->ism_Flags & ISMF_DODEPTH) gadrows++;
    if (imreq->ism_Flags & ISMF_DOAUTOSCROLL) gadrows++;
    
    ld->ld_MinWidth =  OUTERSPACINGX * 2 +
		       GADGETSPACINGX * 1 +
		       udata->ButWidth * NUMBUTS;

    ld->ld_MinHeight = OUTERSPACINGY * 2 +
		       (GADGETSPACINGY + udata->ButHeight) * gadrows +
		       BORDERLVSPACINGY * 2 +
		       (ld->ld_Font->tf_YSize + BORDERLVITEMSPACINGY * 2) * SREQ_MIN_VISIBLELINES;

    /* make listview gadget */
    
    x = ld->ld_WBorLeft + OUTERSPACINGX;
    y = ld->ld_WBorTop + OUTERSPACINGY;
    w = -ld->ld_WBorRight - ld->ld_WBorLeft - OUTERSPACINGX * 2 - PROPSIZE;
    h = -ld->ld_WBorBottom - ld->ld_WBorTop - OUTERSPACINGY * 2 -
    	udata->ButHeight * gadrows -
	GADGETSPACINGY * gadrows;
    
    {
        struct TagItem lv_tags[] = 
	{
	    {GA_Left		, x						},
	    {GA_Top		, y						},
	    {GA_RelWidth	, w						},
	    {GA_RelHeight	, h						},
	    {GA_UserData	, (IPTR)ld					},
	    {GA_ID		, ID_LISTVIEW					},
	    {GA_RelVerify	, TRUE						},
	    {ASLLV_Labels	, (IPTR)&udata->ListviewList			},
	    {TAG_DONE								}
	};
	
	udata->Listview = gad = NewObjectA(AslBase->asllistviewclass, NULL, lv_tags);
	if (!udata->Listview) goto failure;

    }
    
    /* make scroller gadget for listview */
    		       
    x = -ld->ld_WBorRight - OUTERSPACINGX - PROPSIZE + 1;
    y = ld->ld_WBorTop + OUTERSPACINGY;
    w = PROPSIZE;
    h = -ld->ld_WBorBottom - ld->ld_WBorTop - OUTERSPACINGY * 2 -
    	udata->ButHeight * gadrows -
	GADGETSPACINGY * gadrows;
    {
	struct TagItem scroller_tags[] =
	{
    	    {GA_RelRight	, x		},
	    {GA_Top		, y		},
	    {GA_Width		, w		},
	    {GA_RelHeight	, h		},
	    {GA_ID		, ID_LISTVIEW	},
	    {PGA_NewLook	, TRUE		},
	    {PGA_Borderless 	, TRUE		},
	    {PGA_Freedom	, FREEVERT	},
	    {PGA_Top		, 0		},
	    {PGA_Total		, 20		},
	    {PGA_Visible	, 1		},
	    {GA_Previous	, (IPTR)gad	},
	    {TAG_DONE				}
	};

	if (!makescrollergadget(&udata->ScrollGad, ld, scroller_tags, AslBase)) goto failure;
	gad = udata->ScrollGad.arrow2;
    }

    connectscrollerandlistview(&udata->ScrollGad, udata->Listview, AslBase);
    
    /* make button row */
    
    y = -ld->ld_WBorBottom - OUTERSPACINGY - udata->ButHeight + 1;
    
    {
        struct TagItem button_tags[] =
	{
	    {GA_Text		, 0			},
	    {GA_Previous	, 0			},
	    {GA_ID		, 0			},
#if SREQ_COOL_BUTTONS
	    {ASLBT_CoolImage	, 0			},
#else
	    {TAG_IGNORE		, 0			},
#endif	    
	    {GA_UserData	, (IPTR)ld		},
	    {GA_Left		, 0			},
	    {GA_RelBottom	, y			},
	    {GA_Width		, udata->ButWidth	},
	    {GA_Height		, udata->ButHeight	},
	    {GA_RelVerify	, TRUE			},
	    {GA_Image		, 0			}, /* means we want a frame */
	    {TAG_DONE					}
	};

	for(i = 0; i < NUMBUTS; i++)
	{
	    button_tags[0].ti_Data = (IPTR)bi[i].text;
	    button_tags[1].ti_Data = (IPTR)gad;
	    button_tags[2].ti_Data = bi[i].gadid;
	    button_tags[3].ti_Data = (IPTR)bi[i].coolimage;

	    *(bi[i].objvar) = gad = NewObjectA(AslBase->aslbuttonclass, NULL, button_tags);
	    if (!gad) goto failure;
	}
	 	 
    }	 
    
    /* make labels */
        
    i = 0;
    if (imreq->ism_Flags & ISMF_DOOVERSCAN)   str[i++] = imreq->ism_OverscanText;
    if (imreq->ism_Flags & ISMF_DOWIDTH)      str[i++] = imreq->ism_WidthText;
    if (imreq->ism_Flags & ISMF_DOHEIGHT)     str[i++] = imreq->ism_HeightText;
    if (imreq->ism_Flags & ISMF_DODEPTH)      str[i++] = imreq->ism_ColorsText;
    if (imreq->ism_Flags & ISMF_DOAUTOSCROLL) str[i++] = imreq->ism_AutoScrollText;

    x = ld->ld_WBorLeft + OUTERSPACINGX;
    y = -ld->ld_WBorBottom - OUTERSPACINGY - udata->ButHeight - 
    	(udata->ButHeight + GADGETSPACINGY) * (gadrows - 1) + 1;

    if (i)
    {
        #define FSET(x) ((imreq->ism_Flags & x) ? TRUE : FALSE)
	
        struct LabelInfo
	{
	    BOOL doit;
	    char *text;
	    Object **objvar;
	} li [] =
	{
	    {FSET(ISMF_DOOVERSCAN)  , imreq->ism_OverscanText  , &udata->OverscanLabel  },
	    {FSET(ISMF_DOWIDTH)     , imreq->ism_WidthText     , &udata->WidthLabel     },
	    {FSET(ISMF_DOHEIGHT)    , imreq->ism_HeightText    , &udata->HeightLabel    },
	    {FSET(ISMF_DODEPTH)     , imreq->ism_ColorsText    , &udata->DepthLabel     },
	    {FSET(ISMF_DOAUTOSCROLL), imreq->ism_AutoScrollText, &udata->AutoScrollLabel}
	}; 

        #undef FSET
	
        struct TagItem label_tags[] =
	{
	    {GA_Left		, 0			},
	    {GA_RelBottom	, y			},
	    {GA_Width		, 0			},
	    {GA_Height		, udata->ButHeight	},
	    {GA_Text		, 0			},
	    {GA_Previous	, (IPTR)gad		},
	    {GA_UserData	, (IPTR)ld		},
	    {GA_Disabled	, TRUE			},
	    {TAG_DONE					}
	};

	w = BiggestTextLength(str, i, &(ld->ld_DummyRP), AslBase);
            
	for(i = 0; i < 5;i++)
	{
	    if (!li[i].doit) continue;
	    
	    label_tags[2].ti_Data = TextLength(&ld->ld_DummyRP, li[i].text, strlen(li[i].text));
	    label_tags[0].ti_Data = x + w - label_tags[2].ti_Data;
	    label_tags[4].ti_Data = (IPTR)li[i].text;
	    label_tags[5].ti_Data = (IPTR)gad;
	    
	    *(li[i].objvar) = gad = NewObjectA(AslBase->aslbuttonclass, NULL, label_tags);
	    if (!gad) goto failure;
	    
	    y += udata->ButHeight + GADGETSPACINGY;
	    label_tags[1].ti_Data = y;
	}	

	y = -ld->ld_WBorBottom - OUTERSPACINGY - udata->ButHeight - 
    	    (udata->ButHeight + GADGETSPACINGY) * (gadrows - 1) + 1;
	x = ld->ld_WBorLeft + OUTERSPACINGX + w + LABELSPACINGX;

	w = -ld->ld_WBorLeft - ld->ld_WBorRight - OUTERSPACINGX * 2 -
            w - LABELSPACINGX;
	
	/* Make Overscan gadget */
	
	if (imreq->ism_Flags & ISMF_DOOVERSCAN)
	{
	    struct TagItem cycle_tags[] =
	    {
	        {GA_Previous		, (IPTR)gad			 },
		{GA_Left		, x				 },
		{GA_RelBottom		, y				 },
		{GA_RelWidth		, w				 },
		{GA_Height		, udata->ButHeight		 },
		{GA_RelVerify		, TRUE				 },
		{GA_UserData		, (IPTR)ld			 },
		{GA_ID			, ID_OVERSCAN			 },
		{ASLCY_Labels		, (IPTR)&imreq->ism_Overscan1Text},
		{ASLCY_Active		, imreq->ism_OverscanType - 1	 },
		{TAG_DONE						 }
		
	    };
	    
	    udata->OverscanGadget = gad = NewObjectA(AslBase->aslcycleclass, NULL, cycle_tags);
	    if (!gad) goto failure;
	    
	    y += udata->ButHeight + GADGETSPACINGY;
	    
	} /* if (imreq->ism_Flags & ISMF_DOOVERSCAN) */
	
	{
	    struct TagItem string_tags[] =
	    {
	        {GA_Previous		, (IPTR)gad			 },
		{GA_ID			, ID_WIDTH			 },
		{STRINGA_LongVal	, imreq->ism_DisplayWidth	 },
		{GA_RelBottom		, y				 },
		{GA_Left		, x				 },
		{GA_RelWidth		, w				 },
		{GA_Height		, udata->ButHeight		 },
		{GA_RelVerify		, TRUE				 },
		{GA_UserData		, (IPTR)ld			 },
		{GA_TabCycle		, TRUE				 },
		{STRINGA_MaxChars	, 8				 },
		{TAG_DONE						 }
	    };
	    
	    /* Make width gadget */
	    
	    if (imreq->ism_Flags & ISMF_DOWIDTH)
	    {
	        udata->WidthGadget = gad = NewObjectA(AslBase->aslstringclass, NULL, string_tags);
		if (!gad) goto failure;
		
		y += udata->ButHeight + GADGETSPACINGY;
	    }
	   
	    /* Make height gadget */
	    
	    if (imreq->ism_Flags & ISMF_DOHEIGHT)
	    {
	        string_tags[0].ti_Data = (IPTR)gad;
		string_tags[1].ti_Data = ID_HEIGHT;
		string_tags[2].ti_Data = imreq->ism_DisplayHeight;
		string_tags[3].ti_Data = y;
		
		udata->HeightGadget = gad = NewObjectA(AslBase->aslstringclass, NULL, string_tags);
		if (!gad) goto failure;
		
		y += udata->ButHeight + GADGETSPACINGY;
	    }	    
	}
	
	{
	    struct TagItem cycle_tags[] =
	    {
	        {GA_Previous		, (IPTR)gad					 },
		{GA_ID			, ID_COLORS					 },
		{ASLCY_Labels		, (IPTR)udata->colorarray			 },
		{ASLCY_Active		, imreq->ism_DisplayDepth - imreq->ism_MinDepth	 },
		{GA_RelBottom		, y						 },
		{GA_Left		, x						 },
		{GA_RelWidth		, w						 },
		{GA_Height		, udata->ButHeight				 },
		{GA_RelVerify		, TRUE						 },
		{GA_UserData		, (IPTR)ld					 },
		{TAG_DONE								 }
		
	    };

	    /* Make Colors gadget */
	    
	    if (imreq->ism_Flags & ISMF_DODEPTH)
	    {
	        STRPTR *array = udata->colorarray;
		STRPTR text = udata->colortext;
		
		for(i = imreq->ism_MinDepth; i <= imreq->ism_MaxDepth; i++)
		{
		    sprintf(text, "%ld", 1L << i);
		    *array++ = text;
		    
		    text += strlen(text) + 1;
		}
		*array++ = NULL;
				
		udata->DepthGadget = gad = NewObjectA(AslBase->aslcycleclass, NULL, cycle_tags);
		if (!gad) goto failure;
		
		y += udata->ButHeight + GADGETSPACINGY;		
	    }
	    
	    /* Make AutoScroll gadget */
	    
	    if (imreq->ism_Flags & ISMF_DOAUTOSCROLL)
	    {
	        cycle_tags[0].ti_Data = (IPTR)gad;
		cycle_tags[1].ti_Data = ID_AUTOSCROLL;
		cycle_tags[2].ti_Data = (IPTR)&imreq->ism_AutoScrollOFFText;
		cycle_tags[3].ti_Data = imreq->ism_AutoScroll;
		cycle_tags[4].ti_Data = y;
		
		udata->AutoScrollGadget = gad = NewObjectA(AslBase->aslcycleclass, NULL, cycle_tags);
		if (!gad) goto failure;
		
		y += udata->ButHeight + GADGETSPACINGY;				
	    }
	    
	}
	
    } /* if (i) */
         
    ld->ld_GList = (struct Gadget *)udata->Listview;							 
    
    /* Menus */
    {
        struct NewMenu nm[] =
	{
	    {NM_TITLE, imreq->ism_Menu_Control													},
	     {NM_ITEM, imreq->ism_Item_Control_LastMode + 2	, imreq->ism_Item_Control_LastMode	, 0, 0, (APTR)SMMEN_LASTMODE		},
	     {NM_ITEM, imreq->ism_Item_Control_NextMode + 2	, imreq->ism_Item_Control_NextMode	, 0, 0, (APTR)SMMEN_NEXTMODE 		},
	     {NM_ITEM, NM_BARLABEL														},
	     {NM_ITEM, imreq->ism_Item_Control_PropertyList + 2	, imreq->ism_Item_Control_PropertyList	, 0, 0, (APTR)SMMEN_PROPERTYLIST	},
	     {NM_ITEM, imreq->ism_Item_Control_Restore + 2	, imreq->ism_Item_Control_Restore	, 0, 0, (APTR)SMMEN_RESTORE		},
	     {NM_ITEM, NM_BARLABEL														},
	     {NM_ITEM, imreq->ism_Item_Control_OK + 2		, imreq->ism_Item_Control_OK		, 0, 0, (APTR)SMMEN_OK			},
	     {NM_ITEM, imreq->ism_Item_Control_Cancel + 2	, imreq->ism_Item_Control_Cancel	, 0, 0, (APTR)SMMEN_CANCEL		},
	    {NM_END																}
	};

	struct TagItem menu_tags[] =
	{
	    {GTMN_NewLookMenus	, TRUE		},
	    {TAG_DONE				}
	};
	
	/* Don't fail, if menus cannot be created/layouted, because a requester
	   without menus is still better than no requester at all */
	   
	if ((ld->ld_Menu = CreateMenusA(nm, menu_tags)))
	{
	    if (!LayoutMenusA(ld->ld_Menu, ld->ld_VisualInfo, menu_tags))
	    {
	        FreeMenus(ld->ld_Menu);ld->ld_Menu = NULL;
	    }
	}
    }
    
    SMRestore(ld, AslBase);
    
    SetIoErr(0);
    ReturnBool ("SMGadInit", TRUE);
    
failure:
    SetIoErr(error);
    
D(bug("failure\n"));

    SMGadCleanup(ld, ASLB(AslBase));

    ReturnBool ("SMGadInit", FALSE);

}

/*****************************************************************************************/

STATIC BOOL SMGadLayout(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    ReturnBool ("SMGadLayout", TRUE );
}

/*****************************************************************************************/

STATIC ULONG SMHandleEvents(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct IntuiMessage 	*imsg;
    struct SMUserData 		*udata;
    struct IntModeReq 		*imreq;
    WORD 			gadid;
    ULONG 			retval = GHRET_OK;

    EnterFunc(bug("SMHandleEvents: Class: %d\n", imsg->Class));

    udata = (struct SMUserData *)ld->ld_UserData;
    imreq = (struct IntModeReq *)ld->ld_IntReq;
    
    imsg = ld->ld_Event;

    switch (imsg->Class)
    {
	case IDCMP_CLOSEWINDOW:
	    retval = FALSE;
	    break;

        case IDCMP_RAWKEY:
	    switch (imsg->Code)
	    {
	        case CURSORUP:
		    SMChangeActiveLVItem(ld, -1, imsg->Qualifier, AslBase);
		    break;
		    
		case CURSORDOWN:
		    SMChangeActiveLVItem(ld, 1, imsg->Qualifier, AslBase);
		    break;
	    }
	    break;

	case IDCMP_VANILLAKEY:
	    switch(imsg->Code)
	    {
	        case 27:
		    retval = FALSE;
		    break;
	    }
	    break;
	
	case IDCMP_GADGETUP:
	    gadid = ((struct Gadget *)imsg->IAddress)->GadgetID;

	    D(bug("GADGETUP! gadgetid=%d\n", gadid));

	    switch (gadid)
	    {
		case ID_BUTCANCEL:
		    retval = FALSE;
		    break;

		case ID_BUTOK:
		    retval = SMGetSelectedMode(ld, AslBase);
		    break;

		case ID_LISTVIEW:		
		    {
	        	struct DisplayMode 	*dispmode;
			IPTR 			active;

			GetAttr(ASLLV_Active, udata->Listview, &active);

			if ((dispmode = (struct DisplayMode *)FindListNode(&udata->ListviewList, (WORD)active)))
			{
			    SMActivateMode(ld, active, AslBase);
			
			    if (imsg->Code) /* TRUE if double clicked */
			    {
				retval = SMGetSelectedMode(ld, AslBase);
			    }
			}
		    }
		    break;
		
		case ID_OVERSCAN:
		    SMChangeActiveLVItem(ld, 0, 0, AslBase);
		    break;
		
		case ID_WIDTH:
		    {
		        struct DisplayMode 	*dispmode;
			LONG			width;
			
			dispmode = SMGetActiveMode(ld, AslBase);
			ASSERT_VALID_PTR(dispmode);
			
			width  = SMGetWidth (ld, AslBase);
			 
			SMFixValues(ld, dispmode, &width, 0, 0, AslBase);			
		    }
		    break;
		    		         
		case ID_HEIGHT:
		    {
		        struct DisplayMode 	*dispmode;
			LONG			height;
			
			dispmode = SMGetActiveMode(ld, AslBase);
			ASSERT_VALID_PTR(dispmode);
			
			height  = SMGetWidth (ld, AslBase);
			 
			SMFixValues(ld, dispmode, 0, &height, 0, AslBase);			
		    }
		    break;
		    		         
	    } /* switch (gadget ID) */

	    break; /* case IDCMP_GADGETUP: */

	case IDCMP_MENUPICK:
	    if (ld->ld_Menu)
	    {
	        UWORD men = imsg->Code;

		while(men != MENUNULL)
		{
		    struct MenuItem *item;
		    
		    if ((item = ItemAddress(ld->ld_Menu, men)))
		    {
			switch((IPTR)GTMENUITEM_USERDATA(item))
			{
			    /* Control menu */
			    
			    case SMMEN_LASTMODE:
		    		SMChangeActiveLVItem(ld, -1, 0, AslBase);
			        break;
				
			    case SMMEN_NEXTMODE:
		    		SMChangeActiveLVItem(ld, 1, 0, AslBase);
			        break;
			
			    case SMMEN_PROPERTYLIST:
			        break;
				
			    case SMMEN_RESTORE:
			        SMRestore(ld, AslBase);
			        break;
				
			    case SMMEN_OK:
			        retval = SMGetSelectedMode(ld, AslBase);
				break;

			    case SMMEN_CANCEL:
			        retval = FALSE;
				break;
			    
			} /* switch id */

		        men = item->NextSelect;
		    } /* if ((item = ItemAddress(ld->ld_Menu, men))) */
		    else
		    {
		        men = MENUNULL;
		    }
		    
		} /* while(men != MENUNULL) */
		
	    } /* if (ld->ld_Menu) */
	    
	    break; /* case IDCMP_MENUPICK: */

    } /* switch (imsg->Class) */

    ReturnInt ("SMHandleEvents", ULONG, retval);
}

/*****************************************************************************************/

STATIC VOID SMGadCleanup(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct SMUserData 		*udata;
    struct ScreenModeRequester 	*req;
    struct IntReq 	*intreq;
    
    EnterFunc(bug("SMGadCleanup(ld=%p)\n", ld));

    udata = (struct SMUserData *)ld->ld_UserData;
    req = (struct ScreenModeRequester *)ld->ld_Req;
    intreq = ld->ld_IntReq;

    if (ld->ld_Window && ld->ld_GList)
    {
        RemoveGList(ld->ld_Window, ld->ld_GList, -1);
    }
    
    killscrollergadget(&udata->ScrollGad, AslBase);

    FreeObjects(&SREQ_FIRST_OBJECT(udata), &SREQ_LAST_OBJECT(udata), AslBase);
    		
    if (ld->ld_Window)
    {
	req->sm_LeftEdge = intreq->ir_LeftEdge = ld->ld_Window->LeftEdge;
	req->sm_TopEdge  = intreq->ir_TopEdge  = ld->ld_Window->TopEdge;
	req->sm_Width    = intreq->ir_Width    = ld->ld_Window->Width;
	req->sm_Height   = intreq->ir_Height   = ld->ld_Window->Height;
    }
    	
    ReturnVoid("SMGadCleanup");
}

/*****************************************************************************************/

STATIC ULONG SMGetSelectedMode(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct SMUserData 		*udata = (struct SMUserData *)ld->ld_UserData;	
    struct IntReq 		*intreq = ld->ld_IntReq;
    struct IntModeReq 		*imreq = (struct IntModeReq *)intreq;
    struct ScreenModeRequester 	*req = (struct ScreenModeRequester *)ld->ld_Req;
    struct DisplayMode		*dispmode;
    struct Rectangle		*rect;
    LONG			width, height;
    
    dispmode = SMGetActiveMode(ld, AslBase);    
    ASSERT_VALID_PTR(dispmode);
    
    imreq->ism_DisplayID =
    req->sm_DisplayID = dispmode->dm_DimensionInfo.Header.DisplayID;

    /* OverscanType: This must be before width/height because of rect variable!
    **               SMGetOverscan() can handle the case when ASLSM_DoOverscanType
    **		     is not set to TRUE
    **/
    
    imreq->ism_OverscanType =
    req->sm_OverscanType = SMGetOverscan(ld, dispmode, &rect, AslBase);
    
    /* Width */
    
    if (imreq->ism_Flags & ISMF_DOWIDTH)
    {
        width = SMGetWidth(ld, AslBase);
    } else {
        width = rect->MaxX - rect->MinX + 1;
    }    
    
    SMFixValues(ld, dispmode, &width, 0, 0, AslBase);    
    
    imreq->ism_DisplayWidth = 
    req->sm_DisplayWidth    = width;
    
    /* Height */
    
    if (imreq->ism_Flags & ISMF_DOHEIGHT)
    {
        height = SMGetHeight(ld, AslBase);
    } else {
        height = rect->MaxY - rect->MinY + 1;
    }
    
    SMFixValues(ld, dispmode, 0, &height, 0, AslBase);
    
    imreq->ism_DisplayHeight =
    req->sm_DisplayHeight   = height;
        
    /* AutoScroll */
    if (imreq->ism_Flags & ISMF_DOAUTOSCROLL)
    {
        imreq->ism_AutoScroll = SMGetAutoScroll(ld, AslBase);
    }
    req->sm_AutoScroll = imreq->ism_AutoScroll;
    
    return GHRET_FINISHED_OK;    
}

/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/

