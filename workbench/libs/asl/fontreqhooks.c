/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Font requester specific code.
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
#include <intuition/imageclass.h>
#include <graphics/gfx.h>
#include <devices/rawkeycodes.h>
#include <libraries/gadtools.h>
#include <workbench/startup.h>
#include <string.h>
#include <stdio.h>

#include "asl_intern.h"
#include "fontreqhooks.h"
#include "fontreqsupport.h"
#include "layout.h"

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

STATIC BOOL  FOGadInit(struct LayoutData *, struct AslBase_intern *);
STATIC VOID  FOWindowOpened(struct LayoutData *, struct AslBase_intern *);
STATIC BOOL  FOGadLayout(struct LayoutData *, struct AslBase_intern *);
STATIC VOID  FOGadCleanup(struct LayoutData *, struct AslBase_intern *);
STATIC ULONG FOHandleEvents(struct LayoutData *, struct AslBase_intern *);
STATIC ULONG FOGetSelectedFont(struct LayoutData *, struct AslBase_intern *AslBase);

/*****************************************************************************************/

#define ID_BUTOK	ID_MAINBUTTON_OK
#define ID_BUTCANCEL	ID_MAINBUTTON_CANCEL

#define ID_NAMELISTVIEW	1
#define ID_SIZELISTVIEW 2
#define ID_NAMESTRING  	3
#define ID_SIZESTRING	4
#define ID_PREVIEW  	5
#define ID_DRAWMODE 	6
#define ID_STYLE    	7
#define ID_FRONTPEN 	8
#define ID_BACKPEN  	9

#undef NUMBUTS
#define NUMBUTS     	2L

#define FONTPREVIEWHEIGHT 40

#define CLASS_ASLBASE 	((struct AslBase_intern *)cl->cl_UserData)
#define HOOK_ASLBASE  	((struct AslBase_intern *)hook->h_Data)

#define AslBase     	HOOK_ASLBASE

/*****************************************************************************************/

AROS_UFH3(IPTR, SizeListviewRenderFunc,
    AROS_UFHA(struct Hook *,            hook,     	A0),
    AROS_UFHA(struct Node *,    	node,           A2),
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

     	SetDrMd(rp, JAM1);
     	    
    	
     	switch (msg->lvdm_State)
     	{
     	    case ASLLVR_SELECTED:
		erasepen = FILLPEN;
		textpen = FILLTEXTPEN;

		/* Fall through */
		
     	    case ASLLVR_NORMAL:
	    {
    	    	struct TextExtent   te;
    	    	WORD 	    	    numfit;
    	    	UBYTE 	    	    s[10];
		
		SetAPen(rp, dri->dri_Pens[erasepen]);
     	    	RectFill(rp, min_x, min_y, max_x, max_y);
     	    	
		if (node)
		{
		    sprintf(s, "%ld", (LONG)node->ln_Name);

    	    	    numfit = TextFit(rp,
				     s,
				     strlen(s),
    	    			     &te,
				     NULL,
				     1,
				     max_x - min_x + 1 - BORDERLVITEMSPACINGX * 2, 
				     max_y - min_y + 1);

	    	    SetAPen(rp, dri->dri_Pens[textpen]);

    	    	    /* Render text */
    	    	    Move(rp, min_x + BORDERLVITEMSPACINGX,
			     min_y + BORDERLVITEMSPACINGY + rp->Font->tf_Baseline);
    	    	    Text(rp, s, numfit);
		}
    	    	
     	    } break;
       	}
     	
     	retval = ASLLVCB_OK;
     }
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

AROS_UFH3(VOID, FOTagHook,
    AROS_UFHA(struct Hook *,            hook,           A0),
    AROS_UFHA(struct ParseTagArgs *,    pta,            A2),
    AROS_UFHA(struct AslBase_intern *,  AslBase,        A1)
)
{
    AROS_USERFUNC_INIT

    struct TagItem 	     *tag;
    const struct TagItem *tstate;
    struct IntFontReq 	 *iforeq;

    EnterFunc(bug("FOTagHook(hook=%p, pta=%p)\n", hook, pta));

    iforeq = (struct IntFontReq *)pta->pta_IntReq;

    tstate = pta->pta_Tags;
    while ((tag = NextTagItem(&tstate)) != NULL)
    {
	IPTR tidata = tag->ti_Data;
	
	switch (tag->ti_Tag)
	{
    	    case ASLFO_InitialName:
	    	if (tidata)
		    iforeq->ifo_TextAttr.ta_Name = (STRPTR)tidata;
		break;
	
	    case ASLFO_InitialSize:
	    	iforeq->ifo_TextAttr.ta_YSize = (UWORD)tidata;
		break;
	
	    case ASLFO_InitialStyle:
	    	iforeq->ifo_TextAttr.ta_Style = (UBYTE)tidata;
		break;
		
	    case ASLFO_InitialFlags:
	    	iforeq->ifo_TextAttr.ta_Flags = (UBYTE)tidata;
		break;

	    case ASLFO_InitialFrontPen:
	    	iforeq->ifo_FrontPen = (UBYTE)tidata;
		break;
		
	    case ASLFO_InitialBackPen:
	    	iforeq->ifo_BackPen = (UBYTE)tidata;
		break;
	
	    case ASLFO_InitialDrawMode:
	    	iforeq->ifo_DrawMode = (UBYTE)tidata;
		break;
	
	    case ASLFO_Flags:
	    	iforeq->ifo_Flags = (UBYTE)tidata;
		break;
		
	    case ASLFO_DoFrontPen:
	    	if (tidata)
		    iforeq->ifo_Flags |= FOF_DOFRONTPEN;
		else
		    iforeq->ifo_Flags &= ~FOF_DOFRONTPEN;
	    	break;

	    case ASLFO_DoBackPen:
	    	if (tidata)
		    iforeq->ifo_Flags |= FOF_DOBACKPEN;
		else
		    iforeq->ifo_Flags &= ~FOF_DOBACKPEN;
	    	break;
	
	    case ASLFO_DoStyle:
	    	if (tidata)
		    iforeq->ifo_Flags |= FOF_DOSTYLE;
		else
		    iforeq->ifo_Flags &= ~FOF_DOSTYLE;
	    	break;

	    case ASLFO_DoDrawMode:
	    	if (tidata)
		    iforeq->ifo_Flags |= FOF_DODRAWMODE;
		else
		    iforeq->ifo_Flags &= ~FOF_DODRAWMODE;
	    	break;
	    
	    case ASLFO_FixedWidthOnly:
	    	if (tidata)
		    iforeq->ifo_Flags |= FOF_FIXEDWIDTHONLY;
		else
		    iforeq->ifo_Flags &= ~FOF_FIXEDWIDTHONLY;
	    	break;
	    
	    case ASLFO_MinHeight:
	    	iforeq->ifo_MinHeight = (UWORD)tidata;
		break;
		
	    case ASLFO_MaxHeight:
	    	iforeq->ifo_MaxHeight = (UWORD)tidata;
		break;
	    
	    case ASLFO_FilterFunc:
	    	iforeq->ifo_FilterFunc = (struct Hook *)tidata;
		break;
	
	    case ASLFO_HookFunc:
	    	iforeq->ifo_HookFunc = (APTR)tidata;
		break;
	
	    case ASLFO_MaxFrontPen:
	    	iforeq->ifo_MaxFrontPen = (UWORD)tidata;
		break;
		
	    case ASLFO_MaxBackPen:
	    	iforeq->ifo_MaxBackPen = (UWORD)tidata;
		break;
	    
	    case ASLFO_ModeList:
    	    	iforeq->ifo_ModeList = (STRPTR *)tidata;
		break;
	    
	    case ASLFO_FrontPens:
	    	iforeq->ifo_FrontPens = (UBYTE *)tidata;
		break;
		
	    case ASLFO_BackPens:
	    	iforeq->ifo_BackPens = (UBYTE *)tidata;
		break;
		
	    case ASLFO_SampleText:
	    	iforeq->ifo_SampleText = (STRPTR)tidata;
		break;
		
	    default:
		break;
		
	} /* switch (tag->ti_Tag) */

    } /* while ((tag = NextTagItem(&tstate)) != 0) */

    ReturnVoid("FOTagHook");

    AROS_USERFUNC_EXIT
}

/*****************************************************************************************/

AROS_UFH3(ULONG, FOGadgetryHook,
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
	    retval = (ULONG)FOGadInit(ld, ASLB(AslBase));
	    break;

	case LDCMD_WINDOWOPENED:
	    FOWindowOpened(ld, ASLB(AslBase));
	    break;

	case LDCMD_LAYOUT:
	    retval = (ULONG)FOGadLayout(ld, ASLB(AslBase));
	    break;

	case LDCMD_HANDLEEVENTS:
	    retval = (ULONG)FOHandleEvents(ld, ASLB(AslBase));
	    break;

	case LDCMD_CLEANUP:
	    FOGadCleanup(ld, ASLB(AslBase));
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
    STRPTR  	    	    	text;
    LONG    	    	    	deftextid;
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

STATIC BOOL FOGadInit(struct LayoutData *ld, struct AslBase_intern *AslBase)
{    
    struct FOUserData 	*udata = ld->ld_UserData;
    struct IntFontReq 	*iforeq = (struct IntFontReq *)ld->ld_IntReq;
    STRPTR 		str[6];
#if USE_SHARED_COOLIMAGES
    struct ButtonInfo 	bi[NUMBUTS] =
    {
        { ID_BUTOK	, GetIR(iforeq)->ir_PositiveText , MSG_FONTREQ_POSITIVE_GAD, COOL_USEIMAGE_ID    , &udata->OKBut	  },
	{ ID_BUTCANCEL  , GetIR(iforeq)->ir_NegativeText , MSG_FONTREQ_NEGATIVE_GAD, COOL_CANCELIMAGE_ID , &udata->CancelBut  }
    };
#else
    struct ButtonInfo 	bi[NUMBUTS] =
    {
        { ID_BUTOK	, GetIR(iforeq)->ir_PositiveText , MSG_FONTREQ_POSITIVE_GAD, &cool_useimage    , &udata->OKBut	  },
	{ ID_BUTCANCEL  , GetIR(iforeq)->ir_NegativeText , MSG_FONTREQ_NEGATIVE_GAD, &cool_cancelimage , &udata->CancelBut  }
    };
#endif
    Object 		*gad;
    LONG		error;
    WORD 		gadrows, x, y, w, h, i, y2;
    WORD		sizelvwidth, labelwidth = 0, maxgadcolwidth = 0;
    
    NEWLIST(&udata->NameListviewList);

    udata->SizeListviewRenderHook.h_Entry      = (APTR)AROS_ASMSYMNAME(SizeListviewRenderFunc);
    udata->SizeListviewRenderHook.h_SubEntry   = NULL;
    udata->SizeListviewRenderHook.h_Data       = AslBase;

    udata->StringEditHook.h_Entry    = (APTR)AROS_ASMSYMNAME(StringEditFunc);
    udata->StringEditHook.h_SubEntry = NULL;
    udata->StringEditHook.h_Data     = AslBase;

    FOGetFonts(ld, AslBase);
    
    error = ERROR_NO_FREE_STORE;
    
    /* calc. min. size */

    w = 0;
    for(i = 0; i < NUMBUTS; i++)
    {
    	if(!bi[i].text) bi[i].text = GetString(bi[i].deftextid, GetIR(iforeq)->ir_Catalog, AslBase);

        x = TextLength(&ld->ld_DummyRP, bi[i].text, strlen(bi[i].text));

#if FOREQ_COOL_BUTTONS
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
    
#if FOREQ_COOL_BUTTONS

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
    
    gadrows = 2; /* button row + string gadgets under listviews */
    if (iforeq->ifo_Flags & FOF_DODRAWMODE) gadrows++;
    if (iforeq->ifo_Flags & FOF_DOSTYLE) gadrows++;
    if (iforeq->ifo_Flags & (FOF_DOFRONTPEN | FOF_DOBACKPEN)) gadrows++;
    
    ld->ld_MinWidth =  OUTERSPACINGX * 2 +
		       GADGETSPACINGX * 1 +
		       udata->ButWidth * NUMBUTS;

    ld->ld_MinHeight = OUTERSPACINGY * 2 +
		       (GADGETSPACINGY + udata->ButHeight) * gadrows +
		       BORDERLVSPACINGY * 2 +
		       (ld->ld_Font->tf_YSize + BORDERLVITEMSPACINGY * 2) * FOREQ_MIN_VISIBLELINES +
		       FONTPREVIEWHEIGHT + GADGETSPACINGY -
		       GADGETSPACINGY; /* because the string gadgets are attached to listview gadgets */

    /* make listview gadgets */
    
    sizelvwidth = PROPSIZE +
    	    	  FOREQ_VISIBILE_SIZE_CHARS * ld->ld_Font->tf_XSize +
		  BORDERLVSPACINGX * 2 +
		  BORDERLVITEMSPACINGX * 2;
    
    x = ld->ld_WBorLeft + OUTERSPACINGX;
    y = ld->ld_WBorTop + OUTERSPACINGY;
    w = -ld->ld_WBorRight - ld->ld_WBorLeft - OUTERSPACINGX * 2 - PROPSIZE - GADGETSPACINGX - sizelvwidth;
    h = -ld->ld_WBorBottom - ld->ld_WBorTop - OUTERSPACINGY * 2 -
    	udata->ButHeight * gadrows -
	GADGETSPACINGY * gadrows -
	(FONTPREVIEWHEIGHT + GADGETSPACINGY) +
	GADGETSPACINGY; /* because the string gadgets are attached to listview gadgets */
    
    {
        struct TagItem lv_tags[] = 
	{
	    {GA_Left		, x						},
	    {GA_Top		, y						},
	    {GA_RelWidth	, w						},
	    {GA_RelHeight	, h						},
	    {GA_UserData	, (IPTR)ld					},
	    {GA_ID		, ID_NAMELISTVIEW				},
	    {GA_RelVerify	, TRUE						},
	    {ASLLV_Labels	, (IPTR)&udata->NameListviewList		},
	    {TAG_IGNORE    	, 0 	    	    	    	    	    	},
	    {TAG_IGNORE     	, (IPTR)&udata->SizeListviewRenderHook	    	},
	    {ASLLV_Font     	, (IPTR)ld->ld_Font 	    	    	    	},
	    {TAG_DONE								}
	};
	
	udata->NameListview = gad = NewObjectA(AslBase->asllistviewclass, NULL, lv_tags);
	if (!gad) goto failure;

    	lv_tags[0].ti_Tag  = GA_RelRight;
	lv_tags[0].ti_Data = -ld->ld_WBorRight - OUTERSPACINGX - sizelvwidth + 1;
	lv_tags[2].ti_Tag  = GA_Width;
	lv_tags[2].ti_Data = sizelvwidth - PROPSIZE;
	lv_tags[5].ti_Data = ID_SIZELISTVIEW;
	lv_tags[7].ti_Data = 0;
	lv_tags[8].ti_Tag  = GA_Previous;
	lv_tags[8].ti_Data = (IPTR)gad;
	lv_tags[9].ti_Tag  = ASLLV_CallBack;
	
	udata->SizeListview = gad = NewObjectA(AslBase->asllistviewclass, NULL, lv_tags);
	if (!gad) goto failure;
	
    }
    
    /* make scroller gadgets for listviews */
    		       
    x = -ld->ld_WBorRight - OUTERSPACINGX - PROPSIZE - sizelvwidth - GADGETSPACINGX + 1;
    y = ld->ld_WBorTop + OUTERSPACINGY;
    w = PROPSIZE;
    h = -ld->ld_WBorBottom - ld->ld_WBorTop - OUTERSPACINGY * 2 -
    	udata->ButHeight * gadrows -
	GADGETSPACINGY * gadrows -
	(FONTPREVIEWHEIGHT + GADGETSPACINGY) +
	GADGETSPACINGY;
    {
	struct TagItem scroller_tags[] =
	{
    	    {GA_RelRight	, x		    },
	    {GA_Top		, y		    },
	    {GA_Width		, w		    },
	    {GA_RelHeight	, h		    },
	    {GA_ID		, ID_NAMELISTVIEW   },
	    {PGA_NewLook	, TRUE		    },
	    {PGA_Borderless 	, TRUE		    },
	    {PGA_Freedom	, FREEVERT	    },
	    {PGA_Top		, 0		    },
	    {PGA_Total		, 20		    },
	    {PGA_Visible	, 1		    },
	    {GA_Previous	, (IPTR)gad	    },
	    {TAG_DONE				    }
	};

	if (!makescrollergadget(&udata->NameScrollGad, ld, scroller_tags, AslBase)) goto failure;
	gad = udata->NameScrollGad.arrow2;

    	scroller_tags[0].ti_Data  = x + sizelvwidth + GADGETSPACINGX;
	scroller_tags[1].ti_Data  = y;
	scroller_tags[2].ti_Data  = w;
	scroller_tags[3].ti_Data  = h;
	scroller_tags[4].ti_Data  = ID_SIZELISTVIEW;
	scroller_tags[11].ti_Data = (IPTR)gad;
	
	if (!makescrollergadget(&udata->SizeScrollGad, ld, scroller_tags, AslBase)) goto failure;
	gad = udata->SizeScrollGad.arrow2;
	
    }

    connectscrollerandlistview(&udata->NameScrollGad, udata->NameListview, AslBase);
    connectscrollerandlistview(&udata->SizeScrollGad, udata->SizeListview, AslBase);

    /* make preview gadget */

    x = ld->ld_WBorLeft + OUTERSPACINGX;
    y = -ld->ld_WBorBottom - OUTERSPACINGY - udata->ButHeight - 
    	GADGETSPACINGY - FONTPREVIEWHEIGHT;
    w = -ld->ld_WBorRight - ld->ld_WBorLeft - OUTERSPACINGX * 2;

    {
    	struct TagItem preview_tags[] =
	{
	    {GA_Left	    	, x 	    	    	    	    },
	    {GA_RelBottom   	, y 	    	    	    	    },
	    {GA_RelWidth    	, w 	    	    	    	    },
	    {GA_Height	    	, FONTPREVIEWHEIGHT  	    	    },
	    {GA_Previous    	, (IPTR)gad 	    	    	    },
	    {GA_ID  	    	, ID_PREVIEW     	    	    },
	    {ASLFP_SampleText	, (IPTR)iforeq->ifo_SampleText	    },
	    {ASLFP_APen     	, iforeq->ifo_FrontPen	    	    },
	    {ASLFP_BPen     	, iforeq->ifo_BackPen	    	    },
	    {TAG_DONE	    	    	    	    	    	    }
	};

	udata->Preview = gad = NewObjectA(AslBase->aslfontpreviewclass, NULL, preview_tags);
	if (!gad) goto failure;
    }
    
    /* make string gadgets */

    x = ld->ld_WBorLeft + OUTERSPACINGX;
    y = -ld->ld_WBorBottom - OUTERSPACINGY - udata->ButHeight - 
    	(udata->ButHeight + GADGETSPACINGY) * (gadrows - 1) -
	(FONTPREVIEWHEIGHT + GADGETSPACINGY) + 1;
		
    w = -ld->ld_WBorRight - ld->ld_WBorLeft - OUTERSPACINGX * 2 - GADGETSPACINGX - sizelvwidth;
    
    {
    	struct TagItem string_tags[] =
	{
	    {GA_Left	    	, x 	    	    	    	    },
	    {GA_RelBottom   	, y 	    	    	    	    },
	    {GA_RelWidth    	, w 	    	    	    	    },
	    {GA_Height	    	, udata->ButHeight  	    	    },
	    {GA_Previous    	, (IPTR)gad 	    	    	    },
	    {STRINGA_TextVal	, (IPTR)""     	    	    	    },
	    {STRINGA_MaxChars	, MAXFONTNAME 	    	    	    },
	    {STRINGA_EditHook	, (IPTR)&udata->StringEditHook	    },
	    {GA_ID  	    	, ID_NAMESTRING     	    	    },
	    {GA_RelVerify   	, TRUE	    	    	    	    },
	    {GA_UserData    	, (IPTR)ld  	    	    	    },
	    {GA_TabCycle    	, TRUE	    	    	    	    },
	    {STRINGA_Font   	, (IPTR)ld->ld_Font 	    	    },
	    {TAG_DONE	    	    	    	    	    	    }
	};

	udata->NameString = gad = NewObjectA(AslBase->aslstringclass, NULL, string_tags);
	if (!gad) goto failure;
	
   	string_tags[0].ti_Tag  = GA_RelRight;
	string_tags[0].ti_Data = -ld->ld_WBorRight - OUTERSPACINGX - sizelvwidth + 1;
	string_tags[2].ti_Tag  = GA_Width;
	string_tags[2].ti_Data = sizelvwidth;
	string_tags[4].ti_Data = (IPTR)gad;
	string_tags[5].ti_Tag  = STRINGA_LongVal;
	string_tags[5].ti_Data = iforeq->ifo_TextAttr.ta_YSize;
	string_tags[6].ti_Data = 6;
	string_tags[8].ti_Data = ID_SIZESTRING;

	udata->SizeString = gad = NewObjectA(AslBase->aslstringclass, NULL, string_tags);
	if (!gad) goto failure;
	
    }
    
    /* make button row */
    
    y = -ld->ld_WBorBottom - OUTERSPACINGY - udata->ButHeight + 1;
    
    {
        struct TagItem button_tags[] =
	{
	    {GA_Text		, 0			},
	    {GA_Previous	, 0			},
	    {GA_ID		, 0			},
#if FOREQ_COOL_BUTTONS
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
        
    i = 0;

    x = ld->ld_WBorLeft + OUTERSPACINGX;
    y = -ld->ld_WBorBottom - OUTERSPACINGY - udata->ButHeight - 
    	(udata->ButHeight + GADGETSPACINGY) * (gadrows - 2) -
	(FONTPREVIEWHEIGHT + GADGETSPACINGY) + 1;

    if (iforeq->ifo_Flags & (FOF_DODRAWMODE | FOF_DOSTYLE | FOF_DOFRONTPEN | FOF_DOBACKPEN))
    {
        #define FSET(x) ((iforeq->ifo_Flags & x) ? TRUE : FALSE)
	
        struct LabelInfo
	{
	    BOOL doit;
	    char *text;
	    Object **objvar;
	} li [] =
	{
	    {FSET(FOF_DOSTYLE)	    , (STRPTR)MSG_FONTREQ_STYLE_LABEL 	, &udata->StyleLabel     },
	    {FALSE  	    	    , (STRPTR)MSG_FONTREQ_COLOR_LABEL_FG, &udata->ColorLabel	 },
	    {FSET(FOF_DODRAWMODE)   , (STRPTR)MSG_FONTREQ_MODE_LABEL  	, &udata->DrawModeLabel  }
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
    	WORD i2;
	
	if (iforeq->ifo_Flags & (FOF_DOFRONTPEN | FOF_DOBACKPEN))
	{
	    li[1].doit = TRUE;
	    
	    switch(iforeq->ifo_Flags & (FOF_DOFRONTPEN | FOF_DOBACKPEN))
	    {
	    	case FOF_DOFRONTPEN:
		    break;
		    
		case FOF_DOBACKPEN:
		    li[1].text = (STRPTR)MSG_FONTREQ_COLOR_LABEL_BG;
		    break;
		    
		case FOF_DOFRONTPEN | FOF_DOBACKPEN:
		    li[1].text = (STRPTR)MSG_FONTREQ_COLOR_LABEL_FGBG;
		    break;
	    }
	    
	} /* if (iforeq->ifo_Flags & (FOF_DOFRONTPEN | FOF_DOBACKPEN)) */
    	for(i = 0, i2 = 0; i < 3; i++)
	{
	    if (li[i].doit)
	    {
	    	if ((i == 2) && (iforeq->ifo_ModeList))
		{
		    li[i].text = iforeq->ifo_ModeList[0];
		}
		else
		{
	    	    li[i].text = GetString((LONG)li[i].text, GetIR(iforeq)->ir_Catalog, AslBase);
		}
		str[i2++] = li[i].text;
	    }
	}
		
	w = labelwidth = BiggestTextLength(str, i2, &(ld->ld_DummyRP), AslBase);
            
	for(i = 0; i < 3;i++)
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
    	    (udata->ButHeight + GADGETSPACINGY) * (gadrows - 2) -
	    (FONTPREVIEWHEIGHT + GADGETSPACINGY) + 1;
	    
	x = ld->ld_WBorLeft + OUTERSPACINGX + w + LABELSPACINGX;

    	/* Make Style gadget */
	
	if (iforeq->ifo_Flags & FOF_DOSTYLE)
	{
	    STRPTR stylestrings[3];
	    
	    struct TagItem style_tags[] =
	    {
	        {GA_Previous		, (IPTR)gad			  },
		{GA_Left		, x				  },
		{GA_RelBottom		, y				  },
		{GA_Width		, 0				  },
		{GA_Height		, udata->ButHeight		  },
		{GA_RelVerify		, TRUE				  },
		{GA_UserData		, (IPTR)ld			  },
		{GA_ID			, ID_STYLE			  },
		{ASLFS_LabelArray    	, (IPTR)stylestrings	    	  },
		{ASLFS_Style		, iforeq->ifo_TextAttr.ta_Style	  },
		{TAG_DONE						  }
	    };

    	    stylestrings[0] = GetString(MSG_FONTREQ_STYLE_BOLD, GetIR(iforeq)->ir_Catalog, AslBase);
    	    stylestrings[1] = GetString(MSG_FONTREQ_STYLE_ITALIC, GetIR(iforeq)->ir_Catalog, AslBase);
    	    stylestrings[2] = GetString(MSG_FONTREQ_STYLE_UNDERLINED, GetIR(iforeq)->ir_Catalog, AslBase);
	    
	    w = BiggestTextLength(stylestrings, 3, &(ld->ld_DummyRP), AslBase);
	    w *= 2;
	    w *= 3;
	    
	    style_tags[3].ti_Data = w;
	    
	    if (w > maxgadcolwidth) maxgadcolwidth = w;
	    
	    udata->StyleGadget = gad = NewObjectA(AslBase->aslfontstyleclass, NULL, style_tags);
	    if (!gad) goto failure;
	    
	    y += udata->ButHeight + GADGETSPACINGY;
	    
	}
	
	w = udata->ButHeight * 12 / 10 + 19; /* CYCLEIMAGEWIDTH = 19 */
	
	/* Make FrontPen gadget */
	
	if (iforeq->ifo_Flags & FOF_DOFRONTPEN)
	{
	    struct TagItem cp_tags[] =
	    {
	        {GA_Previous		, (IPTR)gad			  },
		{GA_Left		, x				  },
		{GA_RelBottom		, y				  },
		{GA_Width		, w				  },
		{GA_Height		, udata->ButHeight		  },
		{GA_RelVerify		, TRUE				  },
		{GA_UserData		, (IPTR)ld			  },
		{GA_ID			, ID_FRONTPEN			  },
		{ASLCP_Color	    	, iforeq->ifo_FrontPen	    	  },
		{ASLCP_ColorTable   	, (IPTR)iforeq->ifo_FrontPens     },
		{ASLCP_NumColors    	, iforeq->ifo_MaxFrontPen   	  },
		{TAG_DONE						  }
		
	    };
	    
	    udata->FGColorGadget = gad = NewObjectA(AslBase->aslcolorpickerclass, NULL, cp_tags);
	    if (!gad) goto failure;
	    
	    x += w + GADGETSPACINGX;
	}

    	/* Make BackPen gadget */
	
	if (iforeq->ifo_Flags & FOF_DOBACKPEN)
	{
	    struct TagItem cp_tags[] =
	    {
	        {GA_Previous		, (IPTR)gad			  },
		{GA_Left		, x				  },
		{GA_RelBottom		, y				  },
		{GA_Width		, w				  },
		{GA_Height		, udata->ButHeight		  },
		{GA_RelVerify		, TRUE				  },
		{GA_UserData		, (IPTR)ld			  },
		{GA_ID			, ID_BACKPEN			  },
		{ASLCP_Color	    	, iforeq->ifo_BackPen	    	  },
		{ASLCP_ColorTable   	, (IPTR)iforeq->ifo_BackPens      },
		{ASLCP_NumColors    	, iforeq->ifo_MaxBackPen   	  },
		{TAG_DONE						  }
		
	    };
	    
	    udata->BGColorGadget = gad = NewObjectA(AslBase->aslcolorpickerclass, NULL, cp_tags);
	    if (!gad) goto failure;
	    
	}

    	if (iforeq->ifo_Flags & (FOF_DOFRONTPEN | FOF_DOBACKPEN))
	{
	    if ((iforeq->ifo_Flags & (FOF_DOFRONTPEN | FOF_DOBACKPEN)) == (FOF_DOFRONTPEN | FOF_DOBACKPEN))
	    {
	    	w += GADGETSPACINGX + w;
	    }
	    
	    if (w > maxgadcolwidth) maxgadcolwidth = w;
	    
	    y += udata->ButHeight + GADGETSPACINGY;
	}
	
	
    	/* Make DrawMode gadget */

	x = ld->ld_WBorLeft + OUTERSPACINGX + labelwidth + LABELSPACINGX;

	w = -ld->ld_WBorLeft - ld->ld_WBorRight - OUTERSPACINGX * 2 -
            labelwidth - LABELSPACINGX;

	
	if (iforeq->ifo_Flags & FOF_DODRAWMODE)
	{
	    struct TagItem cycle_tags[] =
	    {
	        {GA_Previous		, (IPTR)gad			  },
		{GA_Left		, x				  },
		{GA_RelBottom		, y				  },
		{GA_RelWidth		, w				  },
		{GA_Height		, udata->ButHeight		  },
		{GA_RelVerify		, TRUE				  },
		{GA_UserData		, (IPTR)ld			  },
		{GA_ID			, ID_DRAWMODE			  },
		{ASLCY_Labels		, 0 	    	    	    	  },
		{ASLCY_Active		, iforeq->ifo_DrawMode	    	  },
		{ASLCY_Font 	    	, (IPTR)ld->ld_Font 	    	  },
		{TAG_DONE						  }
		
	    };
	    static LONG labelids[] =
	    {
		MSG_FONTREQ_MODE_TEXT,
		MSG_FONTREQ_MODE_TEXTANDFIELD,
		MSG_FONTREQ_MODE_COMPLEMENT,
	    };
	    STRPTR *labels;
	    
	    
	    if (iforeq->ifo_ModeList)
	    {
	    	labels = &iforeq->ifo_ModeList[1];
	    }
	    else
	    {
    	    	labels = (STRPTR *)&iforeq->ifo_DrawModeJAM1Text;
	    	
    		for(i = 0; i < 3; i++)
		{
	    	    labels[i] = GetString(labelids[i], GetIR(iforeq)->ir_Catalog, AslBase);
		}
	    }
	    
	    cycle_tags[8].ti_Data = (IPTR)labels;
	    
	    i = CYCLEEXTRAWIDTH +  BiggestTextLength(labels,
	    					     0x7FFF,
						     &(ld->ld_DummyRP),
						     AslBase);						     
	    if (i > maxgadcolwidth) maxgadcolwidth = i;
	    
	    udata->DrawModeGadget = gad = NewObjectA(AslBase->aslcycleclass, NULL, cycle_tags);
	    if (!gad) goto failure;
	    
	    y += udata->ButHeight + GADGETSPACINGY;
	    
	} /* if (iforeq->ifo_Flags & FOF_DODRAWMODE) */
		
    } /* if (iforeq->ifo_Flags & (FOF_DODRAWMODE | FOF_DOSTYLE | FOF_DOFRONTPEN | FOF_DOBACKPEN)) */

#if AVOID_FLICKER
    {
    	struct TagItem eraser_tags[] =
	{
	    {GA_Previous, (IPTR)gad},
	    {TAG_DONE}
	};
	
	udata->EraserGadget = gad = NewObjectA(AslBase->asleraserclass, NULL, eraser_tags);
	/* Doesn't matter if this failed */
    }
#endif
    
    w = OUTERSPACINGX + labelwidth + LABELSPACINGX + maxgadcolwidth + OUTERSPACINGX;
    if (w > ld->ld_MinWidth) ld->ld_MinWidth = w;
    
    ld->ld_GList = (struct Gadget *)udata->NameListview;							 
    
    /* Menus */
    {
        struct NewMenu nm[] =
	{
	    {NM_TITLE, (STRPTR)MSG_FONTREQ_MEN_CONTROL							},
	     {NM_ITEM, (STRPTR)MSG_FONTREQ_MEN_CONTROL_LASTFONT , 0, 0, 0, (APTR)FOMEN_LASTFONT		},
	     {NM_ITEM, (STRPTR)MSG_FONTREQ_MEN_CONTROL_NEXTFONT , 0, 0, 0, (APTR)FOMEN_NEXTFONT 	},
	     {NM_ITEM, NM_BARLABEL									},
	     {NM_ITEM, (STRPTR)MSG_FONTREQ_MEN_CONTROL_RESTORE	, 0, 0, 0, (APTR)FOMEN_RESTORE 		},
	     {NM_ITEM, (STRPTR)MSG_FONTREQ_MEN_CONTROL_RESCAN	, 0, 0, 0, (APTR)FOMEN_RESCAN		},
	     {NM_ITEM, NM_BARLABEL									},
	     {NM_ITEM, (STRPTR)MSG_FONTREQ_MEN_CONTROL_OK   	, 0, 0, 0, (APTR)FOMEN_OK		},
	     {NM_ITEM, (STRPTR)MSG_FONTREQ_MEN_CONTROL_CANCEL	, 0, 0, 0, (APTR)FOMEN_CANCEL		},
	    {NM_END																}
	};

	struct TagItem menu_tags[] =
	{
	    {GTMN_NewLookMenus  , TRUE  	    	    	    },
	    {GTMN_TextAttr	, (IPTR)GetIR(iforeq)->ir_TextAttr  },
	    {TAG_DONE   	    	    	    	    	    }
	};
	
	if (menu_tags[1].ti_Data == 0) menu_tags[1].ti_Tag = TAG_IGNORE;

	LocalizeMenus(nm, GetIR(iforeq)->ir_Catalog, AslBase);

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
    
    FORestore(ld, iforeq->ifo_TextAttr.ta_Name, iforeq->ifo_TextAttr.ta_YSize,  AslBase);
    
    SetIoErr(0);
    ReturnBool ("FOGadInit", TRUE);
    
failure:
    SetIoErr(error);
    
    D(bug("failure\n"));

    FOGadCleanup(ld, ASLB(AslBase));

    ReturnBool ("FOGadInit", FALSE);

}

/*****************************************************************************************/

STATIC VOID FOWindowOpened(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
//    struct IntFontReq 		*iforeq = (struct IntFontReq *)ld->ld_IntReq;
}

/*****************************************************************************************/

STATIC BOOL FOGadLayout(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    ReturnBool ("FOGadLayout", TRUE );
}

/*****************************************************************************************/

STATIC ULONG FOHandleEvents(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct IntuiMessage 	*imsg = ld->ld_Event;
    struct FOUserData 		*udata = (struct FOUserData *)ld->ld_UserData;
    struct IntFontReq 		*iforeq = (struct IntFontReq *)ld->ld_IntReq;
    WORD 			gadid;
    ULONG 			retval = GHRET_OK;

    EnterFunc(bug("FOHandleEvents: Class: %d\n", imsg->Class));
    
    switch (imsg->Class)
    {
	case IDCMP_CLOSEWINDOW:
	    retval = FALSE;
	    break;

        case IDCMP_RAWKEY:
	    switch (imsg->Code)
	    {
	        case CURSORUP:
		    FOChangeActiveFont(ld, -1, imsg->Qualifier, FALSE, AslBase);
		    break;
		    
		case RAWKEY_PAGEUP:
		    FOChangeActiveFont(ld, -1, IEQUALIFIER_LSHIFT, FALSE, AslBase);
		    break;
		    
		case RAWKEY_HOME:
		    FOChangeActiveFont(ld, -1, IEQUALIFIER_LALT, FALSE, AslBase);
		    break;
		    
		case RAWKEY_NM_WHEEL_UP:
		    FOChangeActiveFont(ld, -3, imsg->Qualifier, FALSE, AslBase);
		    break;
		
		case CURSORDOWN:
		    FOChangeActiveFont(ld, 1, imsg->Qualifier, FALSE, AslBase);
		    break;
		    
		case RAWKEY_PAGEDOWN:
		    FOChangeActiveFont(ld, 1, IEQUALIFIER_LSHIFT, FALSE, AslBase);
		    break;
		    
		case RAWKEY_END:
		    FOChangeActiveFont(ld, 1, IEQUALIFIER_LALT, FALSE, AslBase);
		    break;
		    
		case RAWKEY_NM_WHEEL_DOWN:
		    FOChangeActiveFont(ld, 3, imsg->Qualifier, FALSE, AslBase);
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

	    D(bug("GADGETUP! gadgetid=%d code=%d\n", gadid, imsg->Code));

	    switch (gadid)
	    {
		case ID_BUTCANCEL:
		    retval = FALSE;
		    break;

		case ID_BUTOK:
		    retval = FOGetSelectedFont(ld, AslBase);
		    break;

		case ID_NAMELISTVIEW:		
		    {
	        	struct ASLLVFontReqNode	*fontnode;
			IPTR 			active;
    	    	    	IPTR	    	    	size;
			
			GetAttr(ASLLV_Active, udata->NameListview, &active);
    	    	    	GetAttr(STRINGA_LongVal, udata->SizeString, &size);
			
			if ((fontnode = (struct ASLLVFontReqNode *)FindListNode(&udata->NameListviewList, (WORD)active)))
			{
			    FOActivateFont(ld, active, (LONG)size, AslBase);
			
			    if (imsg->Code) /* TRUE if double clicked */
			    {
				retval = FOGetSelectedFont(ld, AslBase);
			    }
			}
			ActivateGadget((struct Gadget *)udata->NameString, ld->ld_Window, NULL);
		    }
		    break;
		
		case ID_SIZELISTVIEW:
		    if (udata->ActiveFont)
		    {
	        	struct Node	*node;
			IPTR 	    active;

			GetAttr(ASLLV_Active, udata->SizeListview, &active);

			if ((node = FindListNode(&udata->ActiveFont->SizeList, (WORD)active)))
			{
			    FOSetSizeString((LONG)node->ln_Name, ld, AslBase);
			    FOUpdatePreview(ld, AslBase);
			    
			    if (imsg->Code) /* TRUE if double clicked */
			    {
				retval = FOGetSelectedFont(ld, AslBase);
			    }
			}
			ActivateGadget((struct Gadget *)udata->SizeString, ld->ld_Window, NULL);
		    }
		    break;
		    
		case ID_NAMESTRING:
		    if (imsg->Code == STRINGCODE_CURSORUP)
		    {
		    	FOChangeActiveFont(ld, -1, imsg->Qualifier, TRUE, AslBase);
			ActivateGadget((struct Gadget *)udata->NameString, ld->ld_Window, NULL);
			break;
		    }
		    else if (imsg->Code == STRINGCODE_CURSORDOWN)
		    {
		    	FOChangeActiveFont(ld, 1, imsg->Qualifier, TRUE, AslBase);
			ActivateGadget((struct Gadget *)udata->NameString, ld->ld_Window, NULL);
			break;
		    }
		    else if ((imsg->Code == 0) || (imsg->Code == 9))
		    {
    	    	    	FOUpdatePreview(ld, AslBase);
			break;
		    }
		    break;
		
		case ID_SIZESTRING:
		    if (imsg->Code == STRINGCODE_CURSORUP)
		    {
		    	FOChangeActiveSize(ld, -1, imsg->Qualifier, AslBase);
			ActivateGadget((struct Gadget *)udata->SizeString, ld->ld_Window, NULL);
			break;
		    }
		    else if (imsg->Code == STRINGCODE_CURSORDOWN)
		    {
		    	FOChangeActiveSize(ld, 1, imsg->Qualifier, AslBase);
			ActivateGadget((struct Gadget *)udata->SizeString, ld->ld_Window, NULL);
			break;
		    }
		    else if ((imsg->Code == 0) || (imsg->Code == 9))
		    {
		    	IPTR val;
			LONG size;
			
		    	GetAttr(STRINGA_LongVal, udata->SizeString, (IPTR *)&val);
    	    	    	size = (LONG)val;
			
			if ((size < iforeq->ifo_MinHeight) || (size > iforeq->ifo_MaxHeight))
			{
			    if (size < iforeq->ifo_MinHeight) size = iforeq->ifo_MinHeight;
			    if (size > iforeq->ifo_MaxHeight) size = iforeq->ifo_MaxHeight;			    
			    FOSetSizeString(size, ld, AslBase);
			}
			
			FOActivateSize(ld, -size, AslBase);
			
		    	break;
		    }
		    break;
		 
		 case ID_STYLE:
		 case ID_FRONTPEN:
		 case ID_BACKPEN:
		    FOUpdatePreview(ld, AslBase);
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
			    
			    case FOMEN_LASTFONT:
			    	FOChangeActiveFont(ld, -1, 0, FALSE, AslBase);
			        break;
				
			    case FOMEN_NEXTFONT:
		    		FOChangeActiveFont(ld, 1, 0, FALSE, AslBase);
			        break;
			
			    case FOMEN_RESTORE:
			        FORestore(ld, iforeq->ifo_TextAttr.ta_Name, iforeq->ifo_TextAttr.ta_YSize, AslBase);
			        break;

			    case FOMEN_RESCAN:
			    	FOGetFonts(ld, AslBase);
			        break;
				
			    case FOMEN_OK:
			        retval = FOGetSelectedFont(ld, AslBase);
				break;

			    case FOMEN_CANCEL:
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

    ReturnInt ("FOHandleEvents", ULONG, retval);
}

/*****************************************************************************************/

STATIC VOID FOGadCleanup(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FOUserData 		*udata = (struct FOUserData *)ld->ld_UserData;
    struct FontRequester 	*req = (struct FontRequester *)ld->ld_Req;
    struct IntReq 		*intreq = ld->ld_IntReq;
//  struct IntFontReq 		*iforeq = (struct IntFontReq *)intreq;
    
    EnterFunc(bug("FOGadCleanup(ld=%p)\n", ld));

    if (ld->ld_Window && ld->ld_GList)
    {
        RemoveGList(ld->ld_Window, ld->ld_GList, -1);
    }
    
    killscrollergadget(&udata->NameScrollGad, AslBase);
    killscrollergadget(&udata->SizeScrollGad, AslBase);

    FreeObjects(&FOREQ_FIRST_OBJECT(udata), &FOREQ_LAST_OBJECT(udata), AslBase);
    
    FOFreeFonts(ld, AslBase);
    		
    if (udata->PreviewFont) CloseFont(udata->PreviewFont);
    
    if (ld->ld_Window)
    {
	req->fo_LeftEdge = intreq->ir_LeftEdge = ld->ld_Window->LeftEdge;
	req->fo_TopEdge  = intreq->ir_TopEdge  = ld->ld_Window->TopEdge;
	req->fo_Width    = intreq->ir_Width    = ld->ld_Window->Width;
	req->fo_Height   = intreq->ir_Height   = ld->ld_Window->Height;
    }
        
    ReturnVoid("FOGadCleanup");
}

/*****************************************************************************************/

STATIC ULONG FOGetSelectedFont(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FOUserData 	    *udata = (struct FOUserData *)ld->ld_UserData;
    struct IntReq 	    *intreq = ld->ld_IntReq;
    struct IntFontReq 	    *iforeq = (struct IntFontReq *)intreq;
    struct FontRequester    *req = (struct FontRequester *)ld->ld_Req;
    STRPTR  	    	    name;
    IPTR    	    	    val;
    ULONG   	    	    retval = GHRET_OK;
    
    GetAttr(STRINGA_TextVal, udata->NameString, (IPTR *)&name);
    if (!(req->fo_TAttr.tta_Name = VecPooledCloneString(name, ".font", intreq->ir_MemPool, AslBase))) goto bye;
    iforeq->ifo_TextAttr.ta_Name = req->fo_TAttr.tta_Name;
    
    GetAttr(STRINGA_LongVal, udata->SizeString, &val);
    req->fo_TAttr.tta_YSize = iforeq->ifo_TextAttr.ta_YSize = (UWORD)val;
        
    /* DrawMode */
    
    if (iforeq->ifo_Flags & FOF_DODRAWMODE)
    {
    	iforeq->ifo_DrawMode = FOGetDrawMode(ld, AslBase);
    }
    req->fo_DrawMode = iforeq->ifo_DrawMode;
    
    /* Style */
    
    
    if (iforeq->ifo_Flags & FOF_DOSTYLE)
    {
    	iforeq->ifo_TextAttr.ta_Style = FOGetStyle(ld, AslBase);
    }
    req->fo_TAttr.tta_Style = iforeq->ifo_TextAttr.ta_Style;
    
    if (iforeq->ifo_Flags & FOF_DOFRONTPEN)
    {
    	iforeq->ifo_FrontPen = FOGetFGColor(ld, AslBase);
    }
    req->fo_FrontPen = iforeq->ifo_FrontPen;
    
    if (iforeq->ifo_Flags & FOF_DOBACKPEN)
    {
    	iforeq->ifo_BackPen = FOGetBGColor(ld, AslBase);
    }
    req->fo_BackPen = iforeq->ifo_BackPen;
    
    /* Hmm ... there is also a struct TextAttr fo_Attr in
       FontRequester structure. Just put the same values in!? */
       
    req->fo_Attr.ta_Name  = req->fo_TAttr.tta_Name;
    req->fo_Attr.ta_YSize = req->fo_TAttr.tta_YSize;
    req->fo_Attr.ta_Style = req->fo_TAttr.tta_Style;
    req->fo_Attr.ta_Flags = req->fo_TAttr.tta_Flags;

    retval = GHRET_FINISHED_OK;

bye:
    return retval;
}

/*****************************************************************************************/

