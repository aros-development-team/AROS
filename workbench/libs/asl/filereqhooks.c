/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    File requester specific code.
*/


#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <intuition/screens.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <graphics/gfx.h>
#include <devices/rawkeycodes.h>
#include <libraries/gadtools.h>
#include <workbench/startup.h>
#include <string.h>

#include "asl_intern.h"
#include "filereqhooks.h"
#include "layout.h"
#include "filereqsupport.h"
#include "specialreq.h"

#if USE_SHARED_COOLIMAGES
#include <libraries/coolimages.h>
#include <proto/coolimages.h>
#else
#include "coolimages.h"
#endif

#define CATCOMP_NUMBERS
#include "strings.h"

#define SDEBUG 0
#define DEBUG 0

#include <aros/debug.h>

/*****************************************************************************************/

STATIC BOOL  FRGadInit(struct LayoutData *, struct AslBase_intern *);
STATIC BOOL  FRGadLayout(struct LayoutData *, struct AslBase_intern *);
STATIC VOID  FRGadCleanup(struct LayoutData *, struct AslBase_intern *);
STATIC ULONG FRHandleEvents(struct LayoutData *, struct AslBase_intern *);
STATIC ULONG FRGetSelectedFiles(struct LayoutData *, struct AslBase_intern *AslBase);

/*****************************************************************************************/

#define ID_BUTOK	ID_MAINBUTTON_OK
#define ID_BUTVOLUMES	ID_MAINBUTTON_MIDDLELEFT
#define ID_BUTPARENT	ID_MAINBUTTON_MIDDLERIGHT
#define ID_BUTCANCEL	ID_MAINBUTTON_CANCEL

#define ID_LISTVIEW	1
#define ID_STRDRAWER    2
#define ID_STRPATTERN   3
#define ID_STRFILE	4

#undef NUMBUTS
#define NUMBUTS 4L

#define CLASS_ASLBASE ((struct AslBase_intern *)cl->cl_UserData)
#define HOOK_ASLBASE  ((struct AslBase_intern *)hook->h_Data)

#define AslBase HOOK_ASLBASE

/*****************************************************************************************/

AROS_UFH3(IPTR, ASLFRRenderHook,
    AROS_UFHA(struct Hook *,            hook,     	A0),
    AROS_UFHA(struct ASLLVFileReqNode *,node,           A2),
    AROS_UFHA(struct ASLLVDrawMsg *,	msg,	        A1)
)
{
    AROS_USERFUNC_INIT

    IPTR retval;

    if (msg->lvdm_MethodID == LV_DRAW)
    {
    	struct DrawInfo *dri = msg->lvdm_DrawInfo;
    	struct RastPort *rp  = msg->lvdm_RastPort;
    	
    	WORD min_x = msg->lvdm_Bounds.MinX;
    	WORD min_y = msg->lvdm_Bounds.MinY;
    	WORD max_x = msg->lvdm_Bounds.MaxX;
    	WORD max_y = msg->lvdm_Bounds.MaxY;

        UWORD erasepen = BACKGROUNDPEN;
	UWORD textpen = TEXTPEN;

	if (node) switch(node->type)
	{
	    case ASLLV_FRNTYPE_DIRECTORY:
	        if (node->subtype > 0) textpen = SHINEPEN;
	        break;
	    case ASLLV_FRNTYPE_VOLUMES:
	        switch(node->subtype)
		{
		    case DLT_DIRECTORY:
		    case DLT_LATE:
		    case DLT_NONBINDING:
		        textpen = SHINEPEN;
			break;
		}
	        break;
	}
	
     	SetDrMd(rp, JAM1);
     	    
     	switch (msg->lvdm_State)
     	{
     	    case ASLLVR_SELECTED:
		erasepen = FILLPEN;
		textpen = FILLTEXTPEN;
		
		/* Fall through */
		
     	    case ASLLVR_NORMAL:
	    {
    	    	WORD 			numfit;
    	    	struct TextExtent 	te;
    	    
		SetAPen(rp, dri->dri_Pens[erasepen]);
     	    	RectFill(rp, min_x, min_y, max_x, max_y);
     	    	
		if (node)
		{
		    struct LayoutData 	*ld = ((struct LayoutData *)node->userdata);
    		    struct FRUserData 	*udata = (struct FRUserData *)ld->ld_UserData;	
		    WORD 		i;
		    
		    SetFont(rp, ld->ld_Font);
		    
		    min_x += BORDERLVITEMSPACINGX;
		    min_y += BORDERLVITEMSPACINGY;
		    
		    max_x -= BORDERLVITEMSPACINGX;
		    max_y -= BORDERLVITEMSPACINGY;
		    
		    for(i = 0; i < ASLLV_MAXCOLUMNS;i++)
		    {
		        WORD x;
    	    	    	UWORD len;
			
		        if (node->text[i] == NULL) continue;
			
			len = strlen(node->text[i]);
			
			switch(udata->LVColumnAlign[i])
			{
			    case ASLLV_ALIGN_RIGHT:
			        x = min_x + udata->LVColumnWidth[i] -
				    TextLength(rp, node->text[i], len);
				break;
				
			    default:
			        x = min_x;
				break;
			}
			
			if (x > max_x) break;
						
    	    		numfit = TextFit(rp,
					 node->text[i],
					 len,
    	    				 &te,
					 NULL,
					 1,
					 max_x - x + 1, 
					 max_y - min_y + 1);

    	    	    	if (numfit < len) numfit++;
			
			if (numfit < 1) break;
			
	    		SetAPen(rp, dri->dri_Pens[textpen]);

    	    		/* Render text */
    	    		Move(rp, x, min_y + rp->Font->tf_Baseline);
    	    		Text(rp, node->text[i], numfit);
			
			min_x += udata->LVColumnWidth[i] + rp->TxWidth * 2;
			
		    } /* for(i = 0; i < ASLLV_MAXCOLUMNS;i++) */		    

	    	} /* if (node) */
     	    	
     	    } break;

       	} /* switch (msg->lvdm_State) */
     	
     	retval = ASLLVCB_OK;
	
     } /* if (msg->lvdm_MethodID == LV_DRAW) */
     else
     {
     	retval = ASLLVCB_UNKNOWN;
     }
     	
     return retval;

     AROS_USERFUNC_EXIT
}

/*****************************************************************************************/

#undef AslBase

/*****************************************************************************************/

AROS_UFH3(VOID, FRTagHook,
    AROS_UFHA(struct Hook *,            hook,           A0),
    AROS_UFHA(struct ParseTagArgs *,    pta,            A2),
    AROS_UFHA(struct AslBase_intern *,  AslBase,        A1)
)
{
    AROS_USERFUNC_INIT

    struct TagItem 	*tag, *tstate;
    struct IntFileReq 	*ifreq;
    IPTR		tidata;
    
    EnterFunc(bug("FRTagHook(hook=%p, pta=%p)\n", hook, pta));

    ifreq = (struct IntFileReq *)pta->pta_IntReq;

    tstate = pta->pta_Tags;
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
	tidata = tag->ti_Data;
	
	switch (tag->ti_Tag)
	{
	    /* The tags that are put "in a row" are defined as the same value,
	    and therefor we only use one of them, but the effect is for all of them
	    */
	    case ASLFR_InitialDrawer:
	    /*	case ASL_Dir:  Obsolete */
		if (tidata)
		    ifreq->ifr_Drawer = (STRPTR)tidata;
		break;

	    case ASLFR_InitialFile:
	    /* case ASL_File:  Obsolete */
		if (tidata)
		    ifreq->ifr_File = (STRPTR)tidata;
		break;

	    case ASLFR_InitialPattern:
	    /*	case ASL_Pattern:  Obsolete */
	    	if (tidata)
		    ifreq->ifr_Pattern = (STRPTR)tidata;
		break;

	    case ASLFR_UserData:
		((struct FileRequester *)pta->pta_Req)->fr_UserData = (APTR)tidata;
		break;

	    /* Options */

	    case ASLFR_Flags1:
		ifreq->ifr_Flags1 = (UBYTE)tidata;
		/* Extract some flags that are common to all requester types and
		put them into IntReq->ir_Flags
		*/
		if (ifreq->ifr_Flags1 & FRF_PRIVATEIDCMP)
		    GetIR(ifreq)->ir_Flags |= IF_PRIVATEIDCMP;
		else
		    GetIR(ifreq)->ir_Flags &= ~IF_PRIVATEIDCMP;
		break;

	    case ASLFR_Flags2:
		ifreq->ifr_Flags2 = (UBYTE)tidata;
		break;

	    case ASLFR_DoSaveMode:
		if (tidata)
		    ifreq->ifr_Flags1 |= FRF_DOSAVEMODE;
		else
		    ifreq->ifr_Flags1 &= ~FRF_DOSAVEMODE;
		break;

	    case ASLFR_DoMultiSelect:
		if (tidata)
		    ifreq->ifr_Flags1 |= FRF_DOMULTISELECT;
		else
		    ifreq->ifr_Flags1 &= ~FRF_DOMULTISELECT;
		break;

	    case ASLFR_DoPatterns:
		if (tidata)
		    ifreq->ifr_Flags1 |= FRF_DOPATTERNS;
		else
		    ifreq->ifr_Flags1 &= ~FRF_DOPATTERNS;
		break;

	    case ASLFR_DrawersOnly:
		if (tidata)
		    ifreq->ifr_Flags2 |= FRF_DRAWERSONLY;
		else
		    ifreq->ifr_Flags2 &= ~FRF_DRAWERSONLY;
		break;

	    case ASLFR_FilterFunc:
		ifreq->ifr_FilterFunc = (struct Hook *)tidata;
		ifreq->ifr_Flags1 |= FRF_FILTERFUNC;
		break;

	    case ASLFR_RejectIcons:
		if (tidata)
		    ifreq->ifr_Flags2 |= FRF_REJECTICONS;
		else
		    ifreq->ifr_Flags2 &= ~FRF_REJECTICONS;
		break;

	    case ASLFR_RejectPattern:
	        if (tidata)
		    ifreq->ifr_RejectPattern = (STRPTR)tidata;
		break;

	    case ASLFR_AcceptPattern:
	        if (tidata)
		    ifreq->ifr_AcceptPattern = (STRPTR)tidata;
		break;
		
	    case ASLFR_FilterDrawers:
		if (tidata)
		    ifreq->ifr_Flags2 |= FRF_FILTERDRAWERS;
		else
		    ifreq->ifr_Flags2 &= ~FRF_FILTERDRAWERS;
		break;

	    case ASLFR_HookFunc:
	        ifreq->ifr_HookFunc = (APTR)tidata;
	        break;
		
	    case ASLFR_SetSortBy:
	    	ifreq->ifr_SortBy = tidata;
		break;
		
	    case ASLFR_GetSortBy:
	        ifreq->ifr_GetSortBy = (ULONG *)tidata;
		break;
		
	    case ASLFR_SetSortOrder:
	        ifreq->ifr_SortOrder = tidata;
		break;
		
	    case ASLFR_GetSortOrder:
	        ifreq->ifr_GetSortOrder = (ULONG *)tidata;
		break;
		
	    case ASLFR_SetSortDrawers:
	        ifreq->ifr_SortDrawers = tidata;
		break;
		
	    case ASLFR_GetSortDrawers:
	        ifreq->ifr_GetSortDrawers = (ULONG *)tidata;
		break;
	
	    case ASLFR_InitialShowVolumes:
	        ifreq->ifr_InitialShowVolumes = tidata ? TRUE : FALSE;
		break;
			
	    default:
		break;
		
	} /* switch (tag->ti_Tag) */

    } /* while ((tag = NextTagItem(&tstate)) != 0) */

    /* DrawersOnly excludes multiselect */
    
    if (ifreq->ifr_Flags2 & FRF_DRAWERSONLY)
    {
        ifreq->ifr_Flags1 &= ~FRF_DOMULTISELECT;
    }
    
    ReturnVoid("FRTagHook");

    AROS_USERFUNC_EXIT
}

/*****************************************************************************************/

AROS_UFH3(ULONG, FRGadgetryHook,
    AROS_UFHA(struct Hook *,            hook,           A0),
    AROS_UFHA(struct LayoutData *,      ld,             A2),
    AROS_UFHA(struct AslBase_intern *,  AslBase,        A1)
)
{
    AROS_USERFUNC_INIT

    ULONG retval;

    switch (ld->ld_Command)
    {
	case LDCMD_INIT:
	    retval = (ULONG)FRGadInit(ld, ASLB(AslBase));
	    break;

	case LDCMD_LAYOUT:
	    retval = (ULONG)FRGadLayout(ld, ASLB(AslBase));
	    break;

	case LDCMD_HANDLEEVENTS:
	    retval = (ULONG)FRHandleEvents(ld, ASLB(AslBase));
	    break;

	case LDCMD_CLEANUP:
	    FRGadCleanup(ld, ASLB(AslBase));
	    retval = GHRET_OK;
	    break;

	default:
	    retval = GHRET_FAIL;
	    break;
    }

    return (retval);

    AROS_USERFUNC_EXIT
}

/*****************************************************************************************/

struct ButtonInfo
{
    WORD 			gadid;  
    char 			*text;
#if USE_SHARED_COOLIMAGES
    ULONG   	    	    	coolid;
    Object  	    	    	**objvar;
    const struct CoolImage  	*coolimage;
#else
    const struct CoolImage 	*coolimage;
    Object 			**objvar;
#endif
};

/*****************************************************************************************/

STATIC BOOL FRGadInit(struct LayoutData *ld, struct AslBase_intern *AslBase)
{    
    struct FRUserData 		*udata = ld->ld_UserData;
    struct IntFileReq 		*ifreq = (struct IntFileReq *)ld->ld_IntReq;
#if USE_SHARED_COOLIMAGES
    ULONG	    	    	okid = (GetIR(ifreq)->ir_Flags & IF_USER_POSTEXT) ? COOL_USEIMAGE_ID :
    					    ((ifreq->ifr_Flags1 & FRF_DOSAVEMODE) ? COOL_SAVEIMAGE_ID :
					     COOL_LOADIMAGE_ID);
    struct ButtonInfo 		bi[NUMBUTS] =
    {
        { ID_BUTOK	, GetIR(ifreq)->ir_PositiveText , okid               	, &udata->OKBut	     },
	{ ID_BUTVOLUMES , NULL      	    	    	, COOL_DOTIMAGE_ID  	, &udata->VolumesBut },
	{ ID_BUTPARENT  , NULL      	    	    	, COOL_DOTIMAGE_ID    	, &udata->ParentBut  },
	{ ID_BUTCANCEL  , GetIR(ifreq)->ir_NegativeText , COOL_CANCELIMAGE_ID 	, &udata->CancelBut  }
    };
#else
    const struct CoolImage 	*okimage = (GetIR(ifreq)->ir_Flags & IF_USER_POSTEXT) ? &cool_useimage :
    					    ((ifreq->ifr_Flags1 & FRF_DOSAVEMODE) ? &cool_saveimage :
					     &cool_loadimage);
    struct ButtonInfo 		bi[NUMBUTS] =
    {
        { ID_BUTOK	, GetIR(ifreq)->ir_PositiveText , okimage           , &udata->OKBut	 },
	{ ID_BUTVOLUMES , NULL      	    	    	, &cool_dotimage    , &udata->VolumesBut },
	{ ID_BUTPARENT  , NULL      	    	    	, &cool_dotimage    , &udata->ParentBut  },
	{ ID_BUTCANCEL  , GetIR(ifreq)->ir_NegativeText , &cool_cancelimage , &udata->CancelBut  }
    };
#endif
    Object 			*gad;
    STRPTR 			butstr[NUMBUTS];
    LONG			error = ERROR_NO_FREE_STORE;
    WORD 			gadrows, x, y, w, h, i, y2;
    
    NEWLIST(&udata->ListviewList);
    
    udata->StringEditHook.h_Entry    = (APTR)AROS_ASMSYMNAME(StringEditFunc);
    udata->StringEditHook.h_SubEntry = NULL;
    udata->StringEditHook.h_Data     = AslBase;
    
    udata->ListviewHook.h_Entry      = (APTR)AROS_ASMSYMNAME(ASLFRRenderHook);
    udata->ListviewHook.h_SubEntry   = NULL;
    udata->ListviewHook.h_Data       = AslBase;

    /* calc. min. size */
    
    if (!bi[0].text) bi[0].text = GetString(MSG_FILEREQ_POSITIVE_GAD, GetIR(ifreq)->ir_Catalog, AslBase);    
    bi[1].text = GetString(MSG_FILEREQ_VOLUMES_GAD, GetIR(ifreq)->ir_Catalog, AslBase);
    bi[2].text = GetString(MSG_FILEREQ_PARENT_GAD, GetIR(ifreq)->ir_Catalog, AslBase);
    if (!bi[3].text) bi[3].text = GetString(MSG_FILEREQ_NEGATIVE_GAD, GetIR(ifreq)->ir_Catalog, AslBase);
    
    
    w = 0;
    for(i = 0; i < NUMBUTS; i++)
    {
        x = TextLength(&ld->ld_DummyRP, bi[i].text, strlen(bi[i].text));

#if FREQ_COOL_BUTTONS
#if USE_SHARED_COOLIMAGES
    	if (CoolImagesBase)
	{
	    bi[i].coolimage = (const struct CoolImage *)COOL_ObtainImageA(bi[i].coolid, NULL);
	}
	
	if (CoolImagesBase)
#endif
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
        
#if FREQ_COOL_BUTTONS

#if USE_SHARED_COOLIMAGES
    if (CoolImagesBase)
    {
#endif
	y  = BUTTONEXTRAHEIGHT + ld->ld_Font->tf_YSize;
	if (ld->ld_TrueColor)
	{
            y2 = IMAGEBUTTONEXTRAHEIGHT + DEF_COOLIMAGEHEIGHT;
	} else {
            y2 = 0;
	}
	udata->ButHeight = (y > y2) ? y : y2;
#if USE_SHARED_COOLIMAGES
    }
    else
    {
    	udata->ButHeight = BUTTONEXTRAHEIGHT + ld->ld_Font->tf_YSize;
    }
#endif

#else
    udata->ButHeight = BUTTONEXTRAHEIGHT + ld->ld_Font->tf_YSize;
#endif
    
    gadrows = 3; /* button row + file string + drawer string */
    if (ifreq->ifr_Flags1 & FRF_DOPATTERNS) gadrows++;
    if (ifreq->ifr_Flags2 & FRF_DRAWERSONLY) gadrows--;
    
    ld->ld_MinWidth =  OUTERSPACINGX * 2 +
		       GADGETSPACINGX * 3 +
		       udata->ButWidth * NUMBUTS;

    ld->ld_MinHeight = OUTERSPACINGY * 2 +
		       (GADGETSPACINGY + udata->ButHeight) * gadrows +
		       BORDERLVSPACINGY * 2 +
		       (ld->ld_Font->tf_YSize + BORDERLVITEMSPACINGY * 2) * FREQ_MIN_VISIBLELINES;

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
	    {ASLLV_CallBack	, (IPTR)&udata->ListviewHook			},
	    {ASLLV_DoMultiSelect, (ifreq->ifr_Flags1 & FRF_DOMULTISELECT)	},
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
#if FREQ_COOL_BUTTONS
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
	    
	#if USE_SHARED_COOLIMAGES
	    if (CoolImagesBase == NULL) button_tags[3].ti_Tag = TAG_IGNORE;
	#endif
	    button_tags[3].ti_Data = (IPTR)bi[i].coolimage;

	    *(bi[i].objvar) = gad = NewObjectA(AslBase->aslbuttonclass, NULL, button_tags);
	    if (!gad) goto failure;
	}
	 	 
    }	 
    
    /* make labels */
    
    {    
        struct LabelInfo
	{
	    BOOL 	doit;
	    STRPTR  	text;
	    Object 	**objvar;
	} li [] =
	{
	    {TRUE, (STRPTR)MSG_FILEREQ_PATTERN_LABEL, &udata->PatternLabel },
	    {TRUE, (STRPTR)MSG_FILEREQ_DRAWER_LABEL , &udata->DrawerLabel  },
	    {TRUE, (STRPTR)MSG_FILEREQ_FILE_LABEL   , &udata->FileLabel    }
	}; 

        struct TagItem label_tags[] =
	{
	    {GA_Left		, 0			},
	    {GA_RelBottom	, 0			},
	    {GA_Width		, 0			},
	    {GA_Height		, udata->ButHeight	},
	    {GA_Text		, 0			},
	    {GA_Previous	, (IPTR)gad		},
	    {GA_UserData	, (IPTR)ld		},
	    {GA_Disabled	, TRUE			},
	    {TAG_DONE					}
	};

    	for(i = 0; i < 3; i++)
	{
	    li[i].text = GetString((LONG)li[i].text, GetIR(ifreq)->ir_Catalog, AslBase);
	}
	
	/* Drawer label is always there */
	
	w = TextLength(&ld->ld_DummyRP, li[1].text, strlen(li[1].text)) + 
            LABELSPACINGX +
	    ld->ld_Font->tf_XSize * 2; /* Frame symbol showing directory scan activity */

	i = 0;
	if (ifreq->ifr_Flags1 & FRF_DOPATTERNS)
	{
	    butstr[i++] = li[0].text;
	}
	else
	{
	    li[0].doit = FALSE;
	}
	
	if (!(ifreq->ifr_Flags2 & FRF_DRAWERSONLY))
	{
	    butstr[i++] = li[2].text;
	}
	else
	{
	    li[2].doit = FALSE;
	}

	if (i)
	{
	    x = BiggestTextLength(butstr, i, &(ld->ld_DummyRP), AslBase);
            if (x > w) w = x;
	}

	x = ld->ld_WBorLeft + OUTERSPACINGX;
	y = -ld->ld_WBorBottom - OUTERSPACINGY - udata->ButHeight - 
    	    (udata->ButHeight + GADGETSPACINGY) * (gadrows - 1) + 1;
    
    	label_tags[1].ti_Data = y;
	
	for(i = 0; i < 3;i++)
	{
	    if (!li[i].doit) continue;
	    	    
	    if (i == 1) y2 = y;
	    
	    label_tags[2].ti_Data = TextLength(&ld->ld_DummyRP, li[i].text, strlen(li[i].text));
	    label_tags[0].ti_Data = x + w - label_tags[2].ti_Data;
	    label_tags[4].ti_Data = (IPTR)li[i].text;
	    label_tags[5].ti_Data = (IPTR)gad;
	    
	    *(li[i].objvar) = gad = NewObjectA(AslBase->aslbuttonclass, NULL, label_tags);
	    if (!gad) goto failure;
	    
	    y += udata->ButHeight + GADGETSPACINGY;
	    label_tags[1].ti_Data = y;
	}	
    }

    /* Directory Scan Symbol */
    
    {
        struct TagItem sym_tags[] =
	{
	    {GA_Left		, x				},
	    {GA_RelBottom	, y2 + 1			},
	    {GA_Width		, ld->ld_Font->tf_XSize * 2	},
	    {GA_Height		, udata->ButHeight - 2		},
	    {GA_Image		, 0				}, /* means we want a frame */
	    {GA_Previous	, (IPTR)gad			},
	    {GA_Disabled	, TRUE				},
	    {GA_UserData	, (IPTR)ld			},
	    {TAG_DONE						}
	};
	
	udata->DirectoryScanSymbol = gad = NewObjectA(AslBase->aslbuttonclass, NULL, sym_tags);
	if (!udata->DirectoryScanSymbol) goto failure;
    }
    
    /* make string gadgets */
        
    y = -ld->ld_WBorBottom - OUTERSPACINGY - udata->ButHeight - 
    	(udata->ButHeight + GADGETSPACINGY) * (gadrows - 1) + 1;
    x = ld->ld_WBorLeft + OUTERSPACINGX + w + LABELSPACINGX;
    
    w = -ld->ld_WBorLeft - ld->ld_WBorRight - OUTERSPACINGX * 2 -
        w - LABELSPACINGX;

    {
        struct StrInfo
	{
	    WORD 	gadid;
	    char 	*text;
	    WORD 	maxchars;
	    Object 	**objvar;
	} si [] =
	{
	    {ID_STRPATTERN, ifreq->ifr_Pattern, MAX_PATTERN_LEN, &udata->PatternGad },
	    {ID_STRDRAWER , ifreq->ifr_Drawer , MAX_PATH_LEN   , &udata->PathGad    },
	    {ID_STRFILE   , ifreq->ifr_File   , MAX_FILE_LEN   , &udata->FileGad    },
	}; 
	
        struct TagItem string_tags[] =
	{
	    {GA_Left		, x				},
	    {GA_RelBottom	, y				},
	    {GA_RelWidth	, w				},
	    {GA_Height		, udata->ButHeight		},
	    {GA_Previous	, (IPTR)gad			},
	    {STRINGA_TextVal	, (IPTR)ifreq->ifr_Pattern	},
	    {STRINGA_MaxChars	, MAX_PATTERN_LEN		},
	    {GA_ID		, ID_STRPATTERN			},
	    {GA_RelVerify	, TRUE				},
	    {GA_UserData	, (IPTR)ld			},
	    {GA_TabCycle	, TRUE				},
	    {STRINGA_EditHook   , (IPTR)&udata->StringEditHook	},
	    {STRINGA_Font   	, (IPTR)ld->ld_Font 	    	},
	    {TAG_DONE						}
	};

	if (!(ifreq->ifr_Flags1 & FRF_DOPATTERNS)) si[0].gadid = 0;
	if (ifreq->ifr_Flags2 & FRF_DRAWERSONLY) si[2].gadid = 0;
	
	for(i = 0;i < 3; i++)
	{
	    if (si[i].gadid == 0) continue;
	    
	    string_tags[4].ti_Data = (IPTR)gad;
	    string_tags[5].ti_Data = (IPTR)si[i].text;
	    string_tags[6].ti_Data = si[i].maxchars;
	    string_tags[7].ti_Data = si[i].gadid;
	    
	    *(si[i].objvar) = gad = NewObjectA(AslBase->aslstringclass, NULL, string_tags);
	    if (!gad) goto failure;
	    
	    y += udata->ButHeight + GADGETSPACINGY;
	    string_tags[1].ti_Data = y;
	}	
    }

#if AVOID_FLICKER
    {
    	struct TagItem eraser_tags[] =
	{
	    {GA_Previous, (IPTR)gad},
	    {TAG_DONE}
	};
	
	udata->EraserGad = gad = NewObjectA(AslBase->asleraserclass, NULL, eraser_tags);
	/* Doesn't matter if this failed */
    }
#endif
    
    if (ifreq->ifr_InitialShowVolumes)
    {
	FRGetVolumes(ld, AslBase);
    } else {
        FRNewPath((STRPTR)ifreq->ifr_Drawer, ld, AslBase);
    }
    
    SetAttrs(udata->Listview, ASLLV_Labels, (IPTR)&udata->ListviewList,
                              TAG_DONE);
    
    ld->ld_GList = (struct Gadget *)udata->Listview;							 
    
    /* Menus */
    {
        struct NewMenu nm[] =
	{
	    {NM_TITLE, (STRPTR)MSG_FILEREQ_MEN_CONTROL											}, /*  0 */ 
	     {NM_ITEM, (STRPTR)MSG_FILEREQ_MEN_CONTROL_LASTNAME		, 0 , 0		, 0		, (APTR)FRMEN_LASTNAME		}, /*  1 */
	     {NM_ITEM, (STRPTR)MSG_FILEREQ_MEN_CONTROL_NEXTNAME		, 0, 0		, 0		, (APTR)FRMEN_NEXTNAME		}, /*  2 */
	     {NM_ITEM, NM_BARLABEL													}, /*  3 */
	     {NM_ITEM, (STRPTR)MSG_FILEREQ_MEN_CONTROL_RESTORE		, 0, 0		, 0		, (APTR)FRMEN_RESTORE		}, /*  4 */
	     {NM_ITEM, (STRPTR)MSG_FILEREQ_MEN_CONTROL_PARENT		, 0, 0		, 0		, (APTR)FRMEN_PARENT		}, /*  5 */
	     {NM_ITEM, (STRPTR)MSG_FILEREQ_MEN_CONTROL_VOLUMES		, 0, 0		, 0		, (APTR)FRMEN_VOLUMES		}, /*  6 */
	     {NM_ITEM, (STRPTR)MSG_FILEREQ_MEN_CONTROL_UPDATE	    	, 0, 0		, 0		, (APTR)FRMEN_UPDATE		}, /*  7 */
	     {NM_ITEM, NM_BARLABEL													}, /*  8 */
	     {NM_ITEM, (STRPTR)MSG_FILEREQ_MEN_CONTROL_DELETE		, 0, 0		, 0		, (APTR)FRMEN_DELETE		}, /*  9 */
	     {NM_ITEM, (STRPTR)MSG_FILEREQ_MEN_CONTROL_CREATEDRAWER	, 0, 0		, 0		, (APTR)FRMEN_NEWDRAWER		}, /* 10 */
	     {NM_ITEM, (STRPTR)MSG_FILEREQ_MEN_CONTROL_RENAME		, 0, 0		, 0		, (APTR)FRMEN_RENAME		}, /* 11 */
	     {NM_ITEM, NM_BARLABEL													}, /* 12 */
	     {NM_ITEM, (STRPTR)MSG_FILEREQ_MEN_CONTROL_SELECT		, 0, 0		, 0		, (APTR)FRMEN_SELECT		}, /* 13 */
	     {NM_ITEM, NM_BARLABEL													}, /* 14 */
	     {NM_ITEM, (STRPTR)MSG_FILEREQ_MEN_CONTROL_OK		, 0, 0		, 0		, (APTR)FRMEN_OK		}, /* 15 */
	     {NM_ITEM, (STRPTR)MSG_FILEREQ_MEN_CONTROL_CANCEL		, 0, 0		, 0		, (APTR)FRMEN_CANCEL		}, /* 16 */
	    {NM_TITLE, (STRPTR)MSG_FILEREQ_MEN_FILELIST											}, /* 17 */
	     {NM_ITEM, (STRPTR)MSG_FILEREQ_MEN_FILELIST_SORTNAME	, 0, CHECKIT   	, 2 + 4		, (APTR)FRMEN_BYNAME		}, /* 18 */
	     {NM_ITEM, (STRPTR)MSG_FILEREQ_MEN_FILELIST_SORTDATE	, 0, CHECKIT	, 1 + 4		, (APTR)FRMEN_BYDATE		}, /* 19 */
	     {NM_ITEM, (STRPTR)MSG_FILEREQ_MEN_FILELIST_SORTSIZE	, 0, CHECKIT	, 1 + 2		, (APTR)FRMEN_BYSIZE		}, /* 20 */
	     {NM_ITEM, NM_BARLABEL													}, /* 21 */
	     {NM_ITEM, (STRPTR)MSG_FILEREQ_MEN_FILELIST_SORTUP	    	, 0, CHECKIT 	, 32		, (APTR)FRMEN_ASCENDING		}, /* 22 */
	     {NM_ITEM, (STRPTR)MSG_FILEREQ_MEN_FILELIST_SORTDOWN	, 0, CHECKIT	, 16		, (APTR)FRMEN_DESCENDING	}, /* 23 */
	     {NM_ITEM, NM_BARLABEL													}, /* 24 */
	     {NM_ITEM, (STRPTR)MSG_FILEREQ_MEN_FILELIST_SORTDRAWERFIRST	, 0, CHECKIT	, 256 + 512	, (APTR)FRMEN_DRAWERSFIRST	}, /* 25 */
	     {NM_ITEM, (STRPTR)MSG_FILEREQ_MEN_FILELIST_SORTDRAWERSAME	, 0, CHECKIT	, 128 + 512	, (APTR)FRMEN_DRAWERSMIX	}, /* 26 */
	     {NM_ITEM, (STRPTR)MSG_FILEREQ_MEN_FILELIST_SORTDRAWERLAST	, 0, CHECKIT	, 128 + 256	, (APTR)FRMEN_DRAWERSLAST	}, /* 27 */
	    {NM_END															}  /* 28 */
	};

	struct TagItem menu_tags[] =
	{
	    {GTMN_NewLookMenus  , TRUE  	    	    	    },
	    {GTMN_TextAttr	, (IPTR)GetIR(ifreq)->ir_TextAttr   },
	    {TAG_DONE   	    	    	    	    	    }
	};
	
	if (menu_tags[1].ti_Data == 0) menu_tags[1].ti_Tag = TAG_IGNORE;

	LocalizeMenus(nm, GetIR(ifreq)->ir_Catalog, AslBase);
	
	nm[18 + ifreq->ifr_SortBy     ].nm_Flags |= CHECKED;
	nm[22 + ifreq->ifr_SortOrder  ].nm_Flags |= CHECKED;
	nm[25 + ifreq->ifr_SortDrawers].nm_Flags |= CHECKED;

        /* Show "Select" menu item only if this is a multiselect file requester.
	   The orig Amiga asl.library disables (ghosts) the item, but why
	   show it if it cannot be used anyway. */
	
	if (!(ifreq->ifr_Flags1 & FRF_DOMULTISELECT))
	{
	    nm[13].nm_Type = NM_IGNORE;
	    nm[14].nm_Type = NM_IGNORE;
	}

	/* No Rename in drawersonly requesters */
	
	if (ifreq->ifr_Flags2 & FRF_DRAWERSONLY)
	{
	    nm[11].nm_Type = NM_IGNORE;
	}
	
	/* Don't fail, if menus cannot be created/layouted, because a requester
	   without menus is still better than no requester at all */
	   
	if ((ld->ld_Menu = CreateMenusA(nm, NULL)))
	{	    
	    if (!LayoutMenusA(ld->ld_Menu, ld->ld_VisualInfo, menu_tags))
	    {
	        FreeMenus(ld->ld_Menu);ld->ld_Menu = NULL;
	    }
	}
    }
    
    SetIoErr(0);

    ReturnBool ("FRGadInit", TRUE);

failure:
    D(bug("failure\n"));

    FRGadCleanup(ld, ASLB(AslBase));

    SetIoErr(error);

    ReturnBool ("FRGadInit", FALSE);

}

/*****************************************************************************************/

STATIC BOOL FRGadLayout(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    FRActivateMainStringGadget(ld, AslBase);
    
    ReturnBool ("FRGadLayout", TRUE );
}

/*****************************************************************************************/

STATIC VOID FRClickOnVolumes(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData *udata = (struct FRUserData *)ld->ld_UserData;
    
    if (udata->Flags & FRFLG_SHOWING_VOLUMES)
    {
	UBYTE *dir;

	GetAttr(STRINGA_TextVal, udata->PathGad, (IPTR *)&dir);
	FRGetDirectory(dir, ld, AslBase);
    } else {
	FRGetVolumes(ld, AslBase);
    }
}

/*****************************************************************************************/

STATIC ULONG FRHandleEvents(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct IntuiMessage *imsg;
    ULONG 		retval = GHRET_OK;
    struct FRUserData 	*udata;
    struct IntFileReq 	*ifreq;
    WORD 		gadid;

    EnterFunc(bug("FRHandleEvents: Class: %d\n", imsg->Class));

    udata = (struct FRUserData *)ld->ld_UserData;
    ifreq = (struct IntFileReq *)ld->ld_IntReq;
    
    imsg = ld->ld_Event;

    switch (imsg->Class)
    {
	case IDCMP_CLOSEWINDOW:
	    retval = FALSE;
	    break;

	case IDCMP_MOUSEBUTTONS:
	    FRActivateMainStringGadget(ld, AslBase);
	    break;
	    
        case IDCMP_RAWKEY:
	    switch (imsg->Code)
	    {
	        case CURSORUP:
		    FRChangeActiveLVItem(ld, -1, imsg->Qualifier, 0, AslBase);
		    break;
		    
		case RAWKEY_PAGEUP:
		    FRChangeActiveLVItem(ld, -1, IEQUALIFIER_LSHIFT, 0, AslBase);
		    break;
		    
		case RAWKEY_HOME:
		    FRChangeActiveLVItem(ld, -1, IEQUALIFIER_LALT, 0, AslBase);
		    break;
		    
		case RAWKEY_NM_WHEEL_UP:
		    FRChangeActiveLVItem(ld, -3, imsg->Qualifier, 0, AslBase);		    
		    break;
		    
		case CURSORDOWN:
		    FRChangeActiveLVItem(ld, 1, imsg->Qualifier, 0, AslBase);
		    break;
		    
		case RAWKEY_PAGEDOWN:	
		    FRChangeActiveLVItem(ld, 1, IEQUALIFIER_LSHIFT, 0, AslBase);
		    break;

		case RAWKEY_END:
		    FRChangeActiveLVItem(ld, 1, IEQUALIFIER_LALT, 0, AslBase);
		    break;
		
		case RAWKEY_NM_WHEEL_DOWN:
		    FRChangeActiveLVItem(ld, 3, imsg->Qualifier, 0, AslBase);
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

		case ID_BUTVOLUMES:
	            FRClickOnVolumes(ld, AslBase);
		    break;

		case ID_BUTPARENT:
		    FRParentPath(ld, AslBase);
		    break;

		case ID_STRFILE:
		    if (imsg->Code == STRINGCODE_CURSORUP)
		    {
			FRChangeActiveLVItem(ld, -1, imsg->Qualifier, (struct Gadget *)udata->FileGad, AslBase);
			break;
		    }
		    else if (imsg->Code == STRINGCODE_CURSORDOWN)
		    {
			FRChangeActiveLVItem(ld, 1, imsg->Qualifier, (struct Gadget *)udata->FileGad, AslBase);
			break;
		    }
		    else if ((imsg->Code == 0) || (imsg->Code == 9))
		    {
	        	char filestring[MAX_FILE_LEN], checkstring[MAX_FILE_LEN], *file;
	        	BOOL fall_through = (imsg->Code == 9) ? FALSE : TRUE;
			BOOL has_colon = FALSE;
			BOOL has_slash = FALSE;

			GetAttr(STRINGA_TextVal, udata->FileGad, (IPTR *)&file);
			strcpy(filestring, file);

			has_colon = strchr(filestring, ':') ? TRUE : FALSE;
			has_slash = strchr(filestring, '/') ? TRUE : FALSE;

			if (has_colon || has_slash)
			{
			    fall_through = FALSE;

			    if (strcmp(filestring, ":") == 0)
			    {
		        	FRNewPath(filestring, ld, AslBase);
				FRSetFile("", ld, AslBase);
			    } else if (stricmp(filestring,  "/") == 0)
			    {
		        	FRParentPath(ld, AslBase);
				FRSetFile("", ld, AslBase);
			    } else {
				BPTR lock;
				BOOL isfile = TRUE;

				if (has_colon)
				{
				    strcpy(checkstring, filestring);
				} else {
				    char *dir;

		        	    GetAttr(STRINGA_TextVal, udata->PathGad, (IPTR *)&dir);

				    strcpy(checkstring, dir);
				    AddPart(checkstring, filestring, MAX_FILE_LEN);
				}

				if ((lock = Lock(checkstring, ACCESS_READ)))
				{
				    struct FileInfoBlock 	*fib;
				    BOOL 			isfile = FALSE;

				    if ((fib = AllocDosObject(DOS_FIB, NULL)))
				    {
			        	if (Examine(lock, fib))
					{
					    if (fib->fib_DirEntryType > 0) isfile = FALSE;
					}
					FreeDosObject(DOS_FIB, fib);
				    }
				    UnLock(lock);
				}

				if (isfile)
				{
				    char *fp = FilePart(checkstring);
				    char fpc = *fp;

				    *fp = '\0';
				    FRNewPath(checkstring, ld, AslBase);

				    *fp = fpc;
				    FRSetFile(fp, ld, AslBase);

				} else {
				   FRNewPath(filestring, ld, AslBase);
				   FRSetFile("", ld, AslBase);
				}
			    }

			    ActivateGadget((struct Gadget *)udata->FileGad, ld->ld_Window, NULL);

			} /* has colon or slash */	

			if (!fall_through) break;

		    } /* if ((imsg->Code == 0) || (imsg->Code == 9)) */
		    else
		    {
			break;
		    }

		    /* fall through */

		case ID_BUTOK:
		    retval = FRGetSelectedFiles(ld, ASLB(AslBase));
		    break;

        	case ID_STRPATTERN:
		    if (imsg->Code == STRINGCODE_CURSORUP)
		    {
			FRChangeActiveLVItem(ld, -1, imsg->Qualifier, (struct Gadget *)udata->PatternGad, AslBase);
		        break;
		    }
		    else if (imsg->Code == STRINGCODE_CURSORDOWN)
		    {
			FRChangeActiveLVItem(ld, 1, imsg->Qualifier, (struct Gadget *)udata->PatternGad, AslBase);
		        break;
		    }
		    else if ((imsg->Code == 0) || (imsg->Code == 9))
		    {
	        	if (imsg->Code == 0) ActivateGadget((struct Gadget *)udata->PathGad, ld->ld_Window, NULL);
			/* fall through to ID_STRDRAWER */
	    	    }
		    else
		    {
		        break;
		    }
		    /* no break here!! */

		case ID_STRDRAWER:
		    if (imsg->Code == STRINGCODE_CURSORUP)
		    {
			FRChangeActiveLVItem(ld, -1, imsg->Qualifier, (struct Gadget *)udata->PathGad, AslBase);
		    }
		    else if (imsg->Code == STRINGCODE_CURSORDOWN)
		    {
			FRChangeActiveLVItem(ld, 1, imsg->Qualifier, (struct Gadget *)udata->PathGad, AslBase);
		    }
		    else if ((imsg->Code == 0) || (imsg->Code == 9))
		    {
	        	UBYTE *dir;

			if ((imsg->Code == 0) && (gadid == ID_STRDRAWER))
			{
			    if (ifreq->ifr_Flags2 & FRF_DRAWERSONLY)
			    {
		        	retval = FRGetSelectedFiles(ld, ASLB(AslBase));
				break;
			    } else {
		        	ActivateGadget((struct Gadget *)udata->FileGad, ld->ld_Window, NULL);
			    }
			}
			GetAttr(STRINGA_TextVal, udata->PathGad, (IPTR *)&dir);
			FRNewPath(dir, ld, AslBase);
		    }
		    break;

		case ID_LISTVIEW:
		    {
	        	struct ASLLVFileReqNode *node;
			IPTR 			active;

			GetAttr(ASLLV_Active, udata->Listview, &active);

			if ((node = (struct ASLLVFileReqNode *)FindListNode(&udata->ListviewList, (WORD)active)))
			{
			    switch(node->type)
			    {
		        	case ASLLV_FRNTYPE_VOLUMES:
				    FRNewPath((STRPTR)node->text[0], ld, AslBase);
				    break;

				case ASLLV_FRNTYPE_DIRECTORY:
				    if (node->subtype > 0)
				    {
			        	FRAddPath((STRPTR)node->text[0], ld, AslBase);
				    } else {
			        	FRSetFile((STRPTR)node->text[0], ld, AslBase);

					if (imsg->Code) /* TRUE if double clicked */
					{
					    retval = FRGetSelectedFiles(ld, AslBase);
					}
				    }
				    break;

			    } /* switch(node->type) */

			} /* if ((node = (struct ASLLVFileReqNode *)FindNode(&udata->ListviewList, (WORD)active))) */

    			FRActivateMainStringGadget(ld, AslBase);

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
		    struct MenuItem 	*item;
		    BOOL	    	resort = FALSE;
		    
		    if ((item = ItemAddress(ld->ld_Menu, men)))
		    {
			switch((IPTR)GTMENUITEM_USERDATA(item))
			{
			    /* Control menu */
			    
			    case FRMEN_LASTNAME:
		    		FRChangeActiveLVItem(ld, -1, 0, 0, AslBase);
			        break;
				
			    case FRMEN_NEXTNAME:
		    		FRChangeActiveLVItem(ld, 1, 0, 0, AslBase);
			        break;
				
			    case FRMEN_RESTORE:
			        if (ifreq->ifr_Flags1 & FRF_DOPATTERNS)
				{
				    FRSetPattern(ifreq->ifr_Pattern, ld, AslBase);
				}
				if (!(ifreq->ifr_Flags2 & FRF_DRAWERSONLY))
				{
				    FRSetFile(ifreq->ifr_File, ld, AslBase);
				}
				FRNewPath(ifreq->ifr_Drawer, ld, AslBase);
			        break;
				
			    case FRMEN_PARENT:
				FRParentPath(ld, AslBase);
			        break;
				
			    case FRMEN_VOLUMES:
			        FRClickOnVolumes(ld, AslBase);
			        break;
				
			    case FRMEN_UPDATE:
			        /* WARNING: a bit hacky */
				udata->Flags ^= FRFLG_SHOWING_VOLUMES;
				FRClickOnVolumes(ld, AslBase);
			        break;
				
			    case FRMEN_DELETE:
			        FRDeleteRequester(ld, AslBase);		
			        break;
				
			    case FRMEN_NEWDRAWER:
			    	FRNewDrawerRequester(ld, AslBase);
			        break;
				
			    case FRMEN_RENAME:
			        FRRenameRequester(ld, AslBase);
			        break;
				
			    case FRMEN_SELECT:
				FRSelectRequester(ld, AslBase);
			        break;
					
			    case FRMEN_OK:
			    	retval = FRGetSelectedFiles(ld, ASLB(AslBase));
				break;

			    case FRMEN_CANCEL:
			        retval = FALSE;
				break;
			    
			    /* File list menu */
			    
			    case FRMEN_BYNAME:
			        ifreq->ifr_SortBy = ASLFRSORTBY_Name;
				resort = TRUE;
			        break;
				
			    case FRMEN_BYDATE:
			        ifreq->ifr_SortBy = ASLFRSORTBY_Date;
				resort = TRUE;
			        break;
				
			    case FRMEN_BYSIZE:
			        ifreq->ifr_SortBy = ASLFRSORTBY_Size;
				resort = TRUE;
			        break;
				
			    case FRMEN_ASCENDING:
			        ifreq->ifr_SortOrder = ASLFRSORTORDER_Ascend;
				resort = TRUE;
			        break;
				
			    case FRMEN_DESCENDING:
			        ifreq->ifr_SortOrder = ASLFRSORTORDER_Descend;
				resort = TRUE;
			        break;
				
			    case FRMEN_DRAWERSFIRST:
			    	ifreq->ifr_SortDrawers = ASLFRSORTDRAWERS_First;
				resort = TRUE;
			        break;
				
			    case FRMEN_DRAWERSMIX:
			        ifreq->ifr_SortDrawers = ASLFRSORTDRAWERS_Mix;
				resort = TRUE;
			        break;
				
			    case FRMEN_DRAWERSLAST:
			        ifreq->ifr_SortDrawers = ASLFRSORTDRAWERS_Last;
				resort = TRUE;
			        break;
				
			} /* switch id */
			
			if (resort)
			{
			    FRReSortListview(ld, AslBase);
			}
			
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

    ReturnInt ("FRHandleEvents", ULONG, retval);
}

/*****************************************************************************************/

STATIC VOID FRGadCleanup(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData 		*udata;
    struct FileRequester 	*req;
    struct IntReq 		*intreq;
    
    EnterFunc(bug("FRGadCleanup(ld=%p)\n", ld));

    udata = (struct FRUserData *)ld->ld_UserData;
    req = (struct FileRequester *)ld->ld_Req;
    intreq = ld->ld_IntReq;

    if (ld->ld_Window && ld->ld_GList)
    {
        RemoveGList(ld->ld_Window, ld->ld_GList, -1);
    }
    
    FreeObjects(&FREQ_FIRST_OBJECT(udata), &FREQ_LAST_OBJECT(udata), AslBase);

    killscrollergadget(&udata->ScrollGad, AslBase);
    
    FRFreeListviewList(ld, AslBase);

    if (ld->ld_Window)
    {
	req->fr_LeftEdge = intreq->ir_LeftEdge = ld->ld_Window->LeftEdge;
	req->fr_TopEdge  = intreq->ir_TopEdge  = ld->ld_Window->TopEdge;
	req->fr_Width    = intreq->ir_Width    = ld->ld_Window->Width;
	req->fr_Height   = intreq->ir_Height   = ld->ld_Window->Height;
    }
    	
    ReturnVoid("FRGadCleanup");
}

/*****************************************************************************************/

STATIC ULONG FRGetSelectedFiles(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData 		*udata = (struct FRUserData *)ld->ld_UserData;	
    struct IntReq 		*intreq = ld->ld_IntReq;
    struct IntFileReq 		*ifreq = (struct IntFileReq *)intreq;
    struct FileRequester 	*req = (struct FileRequester *)ld->ld_Req;
    char 			*name;
    
    ULONG 			retval = GHRET_OK;
    
    /* Kill possible old output variables from a previous AslRequest call
       on the same requester */
    
    #undef GetFR
    #define GetFR(r) ((struct FileRequester *)r)
    /*
     * must be done here and NOT in StripRequester
     */
    MyFreeVecPooled(GetFR(req)->fr_Drawer, AslBase);
    GetFR(req)->fr_Drawer = NULL;

    MyFreeVecPooled(GetFR(req)->fr_File, AslBase);
    GetFR(req)->fr_File = NULL;

    MyFreeVecPooled(GetFR(req)->fr_Pattern, AslBase);
    GetFR(req)->fr_Pattern = NULL;

    StripRequester(req, ASL_FileRequest, AslBase);

    /* Save drawer string gadget text in fr_Drawer */
    
    GetAttr(STRINGA_TextVal, udata->PathGad, (IPTR *)&name);    
    if (!(req->fr_Drawer = VecPooledCloneString(name, NULL, intreq->ir_MemPool, AslBase))) goto bye;
    D(bug("FRGetSelectedFiles: fr_Drawer 0x%lx <%s>\n",req->fr_Drawer,req->fr_Drawer));
    ifreq->ifr_Drawer = req->fr_Drawer;
    
    /* Save file string gadget text in fr_File */
    
    if (!(ifreq->ifr_Flags2 & FRF_DRAWERSONLY))
    {
        GetAttr(STRINGA_TextVal, udata->FileGad, (IPTR *)&name);
	
	if (!(req->fr_File = VecPooledCloneString(name, NULL, intreq->ir_MemPool, AslBase))) goto bye;
	D(bug("FRGetSelectedFiles: fr_File 0x%lx <%s>\n",req->fr_File,req->fr_File));
	ifreq->ifr_File = req->fr_File;
    }

    /* Save pattern string gadget text in fr_Patterns */

    if (ifreq->ifr_Flags1 & FRF_DOPATTERNS)
    {
        GetAttr(STRINGA_TextVal, udata->PatternGad, (IPTR *)&name);
	
	if (!(req->fr_Pattern = VecPooledCloneString(name, NULL, intreq->ir_MemPool, AslBase))) goto bye;
	ifreq->ifr_Pattern = req->fr_Pattern;
    }
    
    /* Create ArgList in case of ASLFR_DoMultiSelect requesters */
    
    req->fr_NumArgs = 0;

    if (ifreq->ifr_Flags1 & FRF_DOMULTISELECT)
    {
        struct WBArg *wbarg;
        BPTR lock;
	
	if ((lock = Lock(req->fr_Drawer, ACCESS_READ)))
	{
	    WORD numargs, numselected = 0;
	    
            if (!(udata->Flags & FRFLG_SHOWING_VOLUMES))
	    {
		numselected = CountNodes(&udata->ListviewList, NODEPRIF_SELECTED | NODEPRIF_MULTISEL);
	    }
	    numargs = numselected > 0 ? numselected : 1;
	    
	    if ((wbarg = MyAllocVecPooled(intreq->ir_MemPool, sizeof(struct WBArg) * numargs, AslBase)))
	    {
	        struct ASLLVFileReqNode *node;
		WORD 			i = 0;
		
	        req->fr_ArgList = wbarg;
		
		ForeachNode(&udata->ListviewList, node)
		{
		    if (i == numselected) break;
		    
		    if ((node->node.ln_Pri & (NODEPRIF_SELECTED | NODEPRIF_MULTISEL)) == (NODEPRIF_SELECTED | NODEPRIF_MULTISEL))
		    {
		        char *filename = node->text[0] ? node->text[0] : "";
			
			if ((wbarg->wa_Name = VecPooledCloneString(filename, NULL, intreq->ir_MemPool, AslBase)))
			{
			    wbarg->wa_Lock = lock;
			    wbarg++;
			    i++;
			}
		    }
		    
		} /* ForeachNode(&udata->ListviewList, node) */
		
		if (i == 0)
		{
		    if ((wbarg->wa_Name = VecPooledCloneString(req->fr_File, NULL, intreq->ir_MemPool, AslBase)))
		    {
		        wbarg->wa_Lock = lock;
			i++;
		    }
		}
		
		if (i == 0)
		{
		    MyFreeVecPooled(req->fr_ArgList, AslBase);
		    req->fr_ArgList = NULL;
		} else {
		    req->fr_NumArgs = i;		
		    lock = 0; /* clear lock to avoid that it is unlocked below */
		}
		
	    } /* if ((wbarg = MyAllocVecPooled(intreq->ir_MemPool, sizeof(struct WBArg) * numargs, AslBase))) */
	    
	    if (lock) UnLock(lock);
	    
	} /* if ((lock = Lock(req->fr_Drawer, ACCESS_READ))) */
	
    } /* if (ifreq->ifr_Flags1 & FRF_DOMULTISELECT) */
    
    if (ifreq->ifr_GetSortBy)      *ifreq->ifr_GetSortBy      = ifreq->ifr_SortBy;
    if (ifreq->ifr_GetSortOrder)   *ifreq->ifr_GetSortOrder   = ifreq->ifr_SortOrder;
    if (ifreq->ifr_GetSortDrawers) *ifreq->ifr_GetSortDrawers = ifreq->ifr_SortDrawers;
    
    retval = GHRET_FINISHED_OK;
    
bye:    
    return retval;
}

/*****************************************************************************************/
