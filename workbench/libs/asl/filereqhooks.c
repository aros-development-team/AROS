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
#include <intuition/screens.h>
#include <intuition/icclass.h>
#include <graphics/gfx.h>
#include <gadgets/aroslistview.h>
#include <gadgets/aroslist.h>
#include <libraries/gadtools.h>
#include <workbench/startup.h>
#include <string.h>

#include "asl_intern.h"
#include "filereqhooks.h"
#include "layout.h"
#include "filereqsupport.h"
#include "coolimages.h"

#define SDEBUG 0
#define DEBUG 0

#include <aros/debug.h>

/*****************************************************************************************/

STATIC BOOL FRGadInit(struct LayoutData *, struct AslBase_intern *);
STATIC BOOL FRGadLayout(struct LayoutData *, struct AslBase_intern *);
STATIC VOID FRGadCleanup(struct LayoutData *, struct AslBase_intern *);
STATIC ULONG FRHandleEvents(struct LayoutData *, struct AslBase_intern *);

STATIC ULONG GetSelectedFiles(struct FRUserData *, struct LayoutData *, struct AslBase_intern *AslBase);

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

/************************
**  ASLFRRenderHook()  **
************************/
AROS_UFH3(IPTR, ASLFRRenderHook,
    AROS_UFHA(struct Hook *,            hook,     	A0),
    AROS_UFHA(struct ASLLVFileReqNode *,node,           A2),
    AROS_UFHA(struct ASLLVDrawMsg *,	msg,	        A1)
)
{
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
    	    	WORD numfit;
    	    	struct TextExtent te;
    	    
		SetAPen(rp, dri->dri_Pens[erasepen]);
     	    	RectFill(rp, min_x, min_y, max_x, max_y);
     	    	
		if (node)
		{
		    struct LayoutData *ld = ((struct LayoutData *)node->userdata);
    		    struct FRUserData *udata = (struct FRUserData *)ld->ld_UserData;	

		    WORD i;
		    
		    SetFont(rp, ld->ld_Font);
		    
		    min_x += BORDERLVITEMSPACINGX;
		    min_y += BORDERLVITEMSPACINGY;
		    
		    max_x -= BORDERLVITEMSPACINGX;
		    max_y -= BORDERLVITEMSPACINGY;
		    
		    for(i = 0; i < ASLLV_MAXCOLUMNS;i++)
		    {
		        WORD x;

		        if (node->text[i] == NULL) continue;
			
			switch(udata->LVColumnAlign[i])
			{
			    case ASLLV_ALIGN_RIGHT:
			        x = min_x + udata->LVColumnWidth[i] -
				    TextLength(rp, node->text[i], strlen(node->text[i]));
				break;
				
			    default:
			        x = min_x;
				break;
			}
			
			if (x > max_x) break;
						
    	    		numfit = TextFit(rp,
					 node->text[i],
					 strlen(node->text[i]),
    	    				 &te,
					 NULL,
					 1,
					 max_x - x + 1, 
					 max_y - min_y + 1);

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
}

/*****************************************************************************************/

#undef AslBase

/*****************************************************************************************/

/****************
**  FRTagHook  **
****************/

AROS_UFH3(VOID, FRTagHook,
    AROS_UFHA(struct Hook *,            hook,           A0),
    AROS_UFHA(struct ParseTagArgs *,    pta,            A2),
    AROS_UFHA(struct AslBase_intern *,  AslBase,        A1)
)
{
    struct TagItem *tag, *tstate;

    struct IntFileReq *ifreq;
    
    EnterFunc(bug("FRTagHook(hook=%p, pta=%p)\n", hook, pta));

    ifreq = (struct IntFileReq *)pta->pta_IntReq;

    tstate = pta->pta_Tags;
    while ((tag = NextTagItem((const struct TagItem **)&tstate)) != NULL)
    {

	switch (tag->ti_Tag)
	{
	    /* The tags that are put "in a row" are defined as the same value,
	    and therefor we only use one of them, but the effect is for all of them
	    */
	    case ASLFR_InitialDrawer:
	    /*	case ASL_Dir:  Obsolete */
		ifreq->ifr_Drawer = (STRPTR)tag->ti_Data;
		break;

	    case ASLFR_InitialFile:
	    /* case ASL_File:  Obsolete */
		ifreq->ifr_File = (STRPTR)tag->ti_Data;
		break;

	    case ASLFR_InitialPattern:
	    /*	case ASL_Pattern:  Obsolete */
		ifreq->ifr_Pattern = (STRPTR)tag->ti_Data;
		break;

	    case ASLFR_UserData:
		((struct FileRequester *)pta->pta_Req)->fr_UserData = (APTR)tag->ti_Data;
		break;

	    /* Options */

	    case ASLFR_Flags1:
		ifreq->ifr_Flags1 = (UBYTE)tag->ti_Data;
		/* Extract some flags that are common to all requester types and
		put them into IntReq->ir_Flags
		*/
		if (ifreq->ifr_Flags1 & FRF_PRIVATEIDCMP)
		    GetIR(ifreq)->ir_Flags |= IF_PRIVATEIDCMP;
		else
		    GetIR(ifreq)->ir_Flags &= ~IF_PRIVATEIDCMP;
		break;

	    case ASLFR_Flags2:
		ifreq->ifr_Flags2 = (UBYTE)tag->ti_Data;
		break;

	    case ASLFR_DoSaveMode:
		if (tag->ti_Data)
		    ifreq->ifr_Flags1 |= FRF_DOSAVEMODE;
		else
		    ifreq->ifr_Flags1 &= ~FRF_DOSAVEMODE;
		break;

	    case ASLFR_DoMultiSelect:
		if (tag->ti_Data)
		    ifreq->ifr_Flags1 |= FRF_DOMULTISELECT;
		else
		    ifreq->ifr_Flags1 &= ~FRF_DOMULTISELECT;
		break;

	    case ASLFR_DoPatterns:
		if (tag->ti_Data)
		    ifreq->ifr_Flags1 |= FRF_DOPATTERNS;
		else
		    ifreq->ifr_Flags1 &= ~FRF_DOPATTERNS;
		break;

	    case ASLFR_DrawersOnly:
		if (tag->ti_Data)
		    ifreq->ifr_Flags2 |= FRF_DRAWERSONLY;
		else
		    ifreq->ifr_Flags2 &= ~FRF_DRAWERSONLY;
		break;

	    case ASLFR_FilterFunc:
		ifreq->ifr_FilterFunc = (struct Hook *)tag->ti_Data;
		ifreq->ifr_Flags1 |= FRF_FILTERFUNC;
		break;

	    case ASLFR_RejectIcons:
		if (tag->ti_Data)
		    ifreq->ifr_Flags2 |= FRF_REJECTICONS;
		else
		    ifreq->ifr_Flags2 &= ~FRF_REJECTICONS;
		break;

	    case ASLFR_RejectPattern:
	        if (tag->ti_Data)
		    ifreq->ifr_RejectPattern = (STRPTR)tag->ti_Data;
		break;

	    case ASLFR_AcceptPattern:
	        if (tag->ti_Data)
		    ifreq->ifr_AcceptPattern = (STRPTR)tag->ti_Data;
		break;
		
	    case ASLFR_FilterDrawers:
		if (tag->ti_Data)
		    ifreq->ifr_Flags2 |= FRF_FILTERDRAWERS;
		else
		    ifreq->ifr_Flags2 &= ~FRF_FILTERDRAWERS;
		break;

	    case ASLFR_HookFunc:
	        ifreq->ifr_HookFunc = (APTR)tag->ti_Data;
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
}

/*****************************************************************************************/

/*********************
**  FRGadgetryHook  **
*********************/
AROS_UFH3(ULONG, FRGadgetryHook,
    AROS_UFHA(struct Hook *,            hook,           A0),
    AROS_UFHA(struct LayoutData *,      ld,             A2),
    AROS_UFHA(struct AslBase_intern *,  AslBase,        A1)
)
{
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
}

/*****************************************************************************************/


/****************
**  FRGadInit  **
****************/

struct ButtonInfo
{
    WORD gadid;  
    char *text;
    const struct CoolImage *coolimage;
    Object **objvar;
};

STATIC BOOL FRGadInit(struct LayoutData *ld, struct AslBase_intern *AslBase)
{    
    struct FRUserData *udata = ld->ld_UserData;
    struct IntFileReq *ifreq = (struct IntFileReq *)ld->ld_IntReq;
    const struct CoolImage *okimage = (GetIR(ifreq)->ir_Flags & IF_USER_POSTEXT) ? &cool_useimage :
    					((ifreq->ifr_Flags1 & FRF_DOSAVEMODE) ? &cool_saveimage :
					  &cool_loadimage);
    struct ButtonInfo bi[NUMBUTS] =
    {
        { ID_BUTOK	, GetIR(ifreq)->ir_PositiveText , okimage           , &udata->OKBut	 },
	{ ID_BUTVOLUMES , ifreq->ifr_VolumesText	, &cool_dotimage    , &udata->VolumesBut },
	{ ID_BUTPARENT  , ifreq->ifr_ParentText	        , &cool_dotimage    , &udata->ParentBut  },
	{ ID_BUTCANCEL  , GetIR(ifreq)->ir_NegativeText , &cool_cancelimage , &udata->CancelBut  }
    };

    Object *gad;
    STRPTR butstr[NUMBUTS];
    WORD gadrows, x, y, w, h, i, y2;
    
    NEWLIST(&udata->ListviewList);
    
    udata->ListviewHook.h_Entry = (APTR)AROS_ASMSYMNAME(ASLFRRenderHook);
    udata->ListviewHook.h_SubEntry = NULL;
    udata->ListviewHook.h_Data = AslBase;

    /* calc. min. size */
    
    w = 0;
    for(i = 0; i < NUMBUTS; i++)
    {
        x = TextLength(&ld->ld_DummyRP, bi[i].text, strlen(bi[i].text));

#if FREQ_COOL_BUTTONS
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
	    button_tags[3].ti_Data = (IPTR)bi[i].coolimage;

	    *(bi[i].objvar) = gad = NewObjectA(AslBase->aslbuttonclass, NULL, button_tags);
	    if (!gad) goto failure;
	}
	 	 
    }	 
    
    /* make labels */
        
    w = TextLength(&ld->ld_DummyRP, ifreq->ifr_DrawerText, strlen(ifreq->ifr_DrawerText)) + 
        LABELSPACINGX +
	ld->ld_Font->tf_XSize * 2; /* Frame symbol showing directory scan activity */

    i = 0;
    if (ifreq->ifr_Flags1 & FRF_DOPATTERNS) butstr[i++] = ifreq->ifr_PatternText;
    if (!(ifreq->ifr_Flags2 & FRF_DRAWERSONLY)) butstr[i++] = ifreq->ifr_FileText;

    if (i)
    {
	x = BiggestTextLength(butstr, i, &(ld->ld_DummyRP), AslBase);
        if (x > w) w = x;
    }
    
    x = ld->ld_WBorLeft + OUTERSPACINGX;
    y = -ld->ld_WBorBottom - OUTERSPACINGY - udata->ButHeight - 
    	(udata->ButHeight + GADGETSPACINGY) * (gadrows - 1) + 1;
    
    {
        struct LabelInfo
	{
	    BOOL doit;
	    char *text;
	    Object **objvar;
	} li [] =
	{
	    {TRUE, ifreq->ifr_PatternText, &udata->PatternLabel },
	    {TRUE, ifreq->ifr_DrawerText , &udata->DrawerLabel  },
	    {TRUE, ifreq->ifr_FileText   , &udata->FileLabel    }
	}; 

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

	if (!(ifreq->ifr_Flags1 & FRF_DOPATTERNS)) li[0].doit = FALSE;
	if (ifreq->ifr_Flags2 & FRF_DRAWERSONLY)   li[2].doit = FALSE;
	
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
	    WORD gadid;
	    char *text;
	    WORD maxchars;
	    struct Gadget **objvar;
	} si [] =
	{
	    {ID_STRPATTERN, ifreq->ifr_Pattern, MAX_PATTERN_LEN, &udata->PatternGad },
	    {ID_STRDRAWER , ifreq->ifr_Drawer , 257            , &udata->PathGad    },
	    {ID_STRFILE   , ifreq->ifr_File   , 257            , &udata->FileGad    },
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
	    
	    *(Object **)(si[i].objvar) = gad = NewObjectA(AslBase->aslstringclass, NULL, string_tags);
	    if (!gad) goto failure;
	    
	    y += udata->ButHeight + GADGETSPACINGY;
	    string_tags[1].ti_Data = y;
	}	
    }
    
    FRNewPath((STRPTR)ifreq->ifr_Drawer, ld, AslBase);
    
    SetAttrs(udata->Listview, ASLLV_Labels, (IPTR)&udata->ListviewList,
                              TAG_DONE);
    
    ld->ld_GList = (struct Gadget *)udata->Listview;							 
    
    /* Menus */
    {
        struct NewMenu nm[] =
	{
	    {NM_TITLE, ifreq->ifr_Menu_Control},
	     {NM_ITEM, ifreq->ifr_Item_Control_LastName + 2, ifreq->ifr_Item_Control_LastName},
	     {NM_ITEM, ifreq->ifr_Item_Control_NextName + 2, ifreq->ifr_Item_Control_NextName},
	     {NM_ITEM, NM_BARLABEL},
	     {NM_ITEM, ifreq->ifr_Item_Control_Restore + 2, ifreq->ifr_Item_Control_Restore},
	     {NM_ITEM, ifreq->ifr_Item_Control_Parent + 2, ifreq->ifr_Item_Control_Parent},
	     {NM_ITEM, ifreq->ifr_Item_Control_Volumes + 2, ifreq->ifr_Item_Control_Volumes},
	     {NM_ITEM, ifreq->ifr_Item_Control_Update + 2, ifreq->ifr_Item_Control_Update},
	     {NM_ITEM, NM_BARLABEL},
	     {NM_ITEM, ifreq->ifr_Item_Control_Delete + 2, ifreq->ifr_Item_Control_Delete},
	     {NM_ITEM, ifreq->ifr_Item_Control_CreateNewDrawer + 2, ifreq->ifr_Item_Control_CreateNewDrawer},
	     {NM_ITEM, ifreq->ifr_Item_Control_Rename + 2, ifreq->ifr_Item_Control_Rename},
	     {NM_ITEM, NM_BARLABEL},
	     {NM_ITEM, ifreq->ifr_Item_Control_Select + 2, ifreq->ifr_Item_Control_Select},
	     {NM_ITEM, NM_BARLABEL},
	     {NM_ITEM, ifreq->ifr_Item_Control_OK + 2, ifreq->ifr_Item_Control_OK},
	     {NM_ITEM, ifreq->ifr_Item_Control_Cancel + 2, ifreq->ifr_Item_Control_Cancel},
	    {NM_TITLE, ifreq->ifr_Menu_FileList},
	     {NM_ITEM, ifreq->ifr_Item_FileList_SortByName + 2, ifreq->ifr_Item_FileList_SortByName},
	     {NM_ITEM, ifreq->ifr_Item_FileList_SortByDate + 2, ifreq->ifr_Item_FileList_SortByDate},
	     {NM_ITEM, ifreq->ifr_Item_FileList_SortBySize + 2, ifreq->ifr_Item_FileList_SortBySize},
	     {NM_ITEM, NM_BARLABEL},
	     {NM_ITEM, ifreq->ifr_Item_FileList_AscendingOrder + 2, ifreq->ifr_Item_FileList_AscendingOrder},
	     {NM_ITEM, ifreq->ifr_Item_FileList_DescendingOrder + 2, ifreq->ifr_Item_FileList_DescendingOrder},
	     {NM_ITEM, NM_BARLABEL},
	     {NM_ITEM, ifreq->ifr_Item_FileList_ShowDrawersFirst + 2, ifreq->ifr_Item_FileList_ShowDrawersFirst},
	     {NM_ITEM, ifreq->ifr_Item_FileList_ShowDrawerWithFiles + 2, ifreq->ifr_Item_FileList_ShowDrawerWithFiles},
	     {NM_ITEM, ifreq->ifr_Item_FileList_ShowDrawersLast + 2, ifreq->ifr_Item_FileList_ShowDrawersLast},
	    {NM_END}
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
    
    ReturnBool ("FRGadInit", TRUE);
failure:

D(bug("failure\n"));

    FRGadCleanup(ld, ASLB(AslBase));

    ReturnBool ("FRGadInit", FALSE);

}

/*****************************************************************************************/

/******************
**  FRGadLayout  **
******************/

STATIC BOOL FRGadLayout(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData *udata = ld->ld_UserData;
    struct IntFileReq *ifreq = (struct IntFileReq *)ld->ld_IntReq;

    if (ifreq->ifr_Flags2 & FRF_DRAWERSONLY)
    {
        ActivateGadget((struct Gadget *)udata->PathGad, ld->ld_Window, NULL);
    } else {
        ActivateGadget((struct Gadget *)udata->FileGad, ld->ld_Window, NULL);
    }
    
    ReturnBool ("FRGadLayout", TRUE );
}

/*****************************************************************************************/


/*********************
**  FRHandleEvents  **
*********************/

STATIC ULONG FRHandleEvents(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct IntuiMessage *imsg;
    ULONG retval =GHRET_OK;
    struct FRUserData *udata;
    struct IntFileReq *ifreq;
    WORD gadid;

    EnterFunc(bug("FRHandleEvents: Class: %d\n", imsg->Class));

    udata = (struct FRUserData *)ld->ld_UserData;
    ifreq = (struct IntFileReq *)ld->ld_IntReq;
    
    imsg = ld->ld_Event;

    switch (imsg->Class)
    {
    case IDCMP_CLOSEWINDOW:
	retval = FALSE;
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
	    if (udata->Flags & FRFLG_SHOWING_VOLUMES)
	    {
	        UBYTE *dir;
		
		GetAttr(STRINGA_TextVal, (Object *)udata->PathGad, (IPTR *)&dir);
		FRGetDirectory(dir, ld, AslBase);
	    } else {
	        FRGetVolumes(ld, AslBase);
	    }
	    break;

	case ID_BUTPARENT:
	    FRParentPath(ld, AslBase);
	    break;

	case ID_STRFILE:
	    {
	        char filestring[257], checkstring[257], *file;
	        BOOL fall_through = (imsg->Code == 9) ? FALSE : TRUE;
		BOOL has_colon = FALSE;
		BOOL has_slash = FALSE;
		
		GetAttr(STRINGA_TextVal, (Object *)udata->FileGad, (IPTR *)&file);
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
			    
		            GetAttr(STRINGA_TextVal, (Object *)udata->PathGad, (IPTR *)&dir);
			    
			    strcpy(checkstring, dir);
			    AddPart(checkstring, filestring, 256);
			}
						
			if ((lock = Lock(checkstring, ACCESS_READ)))
			{
			    struct FileInfoBlock *fib;
			    BOOL isfile = FALSE;
			    
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
		
	    }
	    /* fall through */
	    
	case ID_BUTOK:
	    retval = GetSelectedFiles(udata, ld, ASLB(AslBase));
	    break;

        case ID_STRPATTERN:
	    if (imsg->Code != 9)
	    {
	        ActivateGadget((struct Gadget *)udata->PathGad, ld->ld_Window, NULL);
	    }
	    /* fall through */
	    
	case ID_STRDRAWER:
	    {
	        UBYTE *dir;
		
		if ((imsg->Code != 9) && (gadid == ID_STRDRAWER))
		{
		    if (ifreq->ifr_Flags2 & FRF_DRAWERSONLY)
		    {
		        retval = GetSelectedFiles(udata, ld, ASLB(AslBase));
			break;
		    } else {
		        ActivateGadget((struct Gadget *)udata->FileGad, ld->ld_Window, NULL);
		    }
		}
		GetAttr(STRINGA_TextVal, (Object *)udata->PathGad, (IPTR *)&dir);
		FRNewPath(dir, ld, AslBase);
	    }
	    break;
	    	
	case ID_LISTVIEW:
	    {
	        struct ASLLVFileReqNode *node;
		IPTR active;
		
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
				    retval = GetSelectedFiles(udata, ld, AslBase);
				}
			    }
			    break;
			    
		    } /* switch(node->type) */
		    
		} /* if ((node = (struct ASLLVFileReqNode *)FindNode(&udata->ListviewList, (WORD)active))) */
	    }
	    break;
	    
	} /* switch (gadget ID) */

	break; /* case IDCMP_GADGETUP: */

    } /* switch (imsg->Class) */

    ReturnInt ("FRHandleEvents", ULONG, retval);
}

/*****************************************************************************************/

/*******************
**  FRGadCleanup  **
*******************/
STATIC VOID FRGadCleanup(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData *udata;
    struct FileRequester *req;
    struct IntReq *intreq;
    
    EnterFunc(bug("FRGadCleanup(ld=%p)\n", ld));

    udata = (struct FRUserData *)ld->ld_UserData;
    req = (struct FileRequester *)ld->ld_Req;
    intreq = ld->ld_IntReq;

    if (udata->Listview)
	DisposeObject(udata->Listview);

    udata->Listview = NULL; /* don't remove */    
    FRFreeListviewList(ld, AslBase);

    if (udata->Prop)
	DisposeObject(udata->Prop);

    if (udata->OKBut)
	DisposeObject(udata->OKBut);

    if (udata->VolumesBut)
	DisposeObject(udata->VolumesBut);

    if (udata->ParentBut)
	DisposeObject(udata->ParentBut);

    if (udata->CancelBut)
	DisposeObject(udata->CancelBut);

    if (udata->PatternLabel)
        DisposeObject(udata->PatternLabel);
	
    if (udata->DrawerLabel)
        DisposeObject(udata->DrawerLabel);
	
    if (udata->FileLabel)
        DisposeObject(udata->FileLabel);
	
    if (udata->DirectoryScanSymbol)
        DisposeObject(udata->DirectoryScanSymbol);
	
    if (udata->PatternGad)
        DisposeObject((Object *)udata->PatternGad);
	
    if (udata->PathGad)
        DisposeObject((Object *)udata->PathGad);
	
    if (udata->FileGad)
        DisposeObject((Object *)udata->FileGad);

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

STATIC ULONG GetSelectedFiles(  struct FRUserData       *udata,
				struct LayoutData	*ld,
				struct AslBase_intern	*AslBase)
{
    struct IntReq *intreq = ld->ld_IntReq;
    struct IntFileReq *ifreq = (struct IntFileReq *)intreq;
    struct FileRequester *req = (struct FileRequester *)ld->ld_Req;
    char *name;
    
    ULONG retval = GHRET_OK;
    
    /* Kill possible old output variables from a previous AslRequest call
       on the same requester */
    
    StripRequester(req, ASL_FileRequest, AslBase);

    /* Save drawer string gadget text in fr_Drawer */
    
    GetAttr(STRINGA_TextVal, (Object *)udata->PathGad, (IPTR *)&name);    
    if (!(req->fr_Drawer = VecPooledCloneString(name, NULL, intreq->ir_MemPool, AslBase))) goto bye;
    ifreq->ifr_Drawer = req->fr_Drawer;
    
    /* Save file string gadget text in fr_File */
    
    if (!(ifreq->ifr_Flags2 & FRF_DRAWERSONLY))
    {
        GetAttr(STRINGA_TextVal, (Object *)udata->FileGad, (IPTR *)&name);
	
	if (!(req->fr_File = VecPooledCloneString(name, NULL, intreq->ir_MemPool, AslBase))) goto bye;
	ifreq->ifr_File = req->fr_File;
    }

    /* Save pattern string gadget text in fr_Patterns */

    if (ifreq->ifr_Flags1 & FRF_DOPATTERNS)
    {
        GetAttr(STRINGA_TextVal, (Object *)udata->PatternGad, (IPTR *)&name);
	
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
	    
	    if ((wbarg = AllocVecPooled(intreq->ir_MemPool, sizeof(struct WBArg) * numargs)))
	    {
	        struct ASLLVFileReqNode *node;
		WORD i = 0;
		
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
		    FreeVecPooled(req->fr_ArgList);
		    req->fr_ArgList = NULL;
		} else {
		    req->fr_NumArgs = i;		
		    lock = 0; /* clear lock to avoid that it is unlocked below */
		}
		
	    } /* if ((wbarg = AllocVecPooled(sizeof(struct WBArg) * numargs, MEMF_PUBLIC))) */
	    
	    if (lock) UnLock(lock);
	    
	} /* if ((lock = Lock(req->fr_Drawer, ACCESS_READ))) */
	
    } /* if (ifreq->ifr_Flags1 & FRF_DOMULTISELECT) */
    
    retval = GHRET_FINISHED_OK;
    
bye:    
    return (retval);
}

/*****************************************************************************************/
