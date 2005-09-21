/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    GadTools gadget creation functions
*/

/****************************************************************************************/

#include <stdio.h>
#include <proto/exec.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <proto/intuition.h>
#include <graphics/gfxmacros.h>
#include <intuition/intuition.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/screens.h>
#include <intuition/icclass.h>
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <libraries/gadtools.h>

#if 0
#define SDEBUG 0
#define DEBUG 0
#endif
#include <aros/debug.h>

#include "gadtools_intern.h"

/****************************************************************************************/

#define EG(x) ((struct ExtGadget *)(x))

/****************************************************************************************/

struct Gadget *makebutton(struct GadToolsBase_intern *GadToolsBase,
			  struct TagItem stdgadtags[],
			  struct VisualInfo *vi,
			  struct TagItem *taglist)
{
    struct Gadget *obj;
    struct TagItem tags[] =
    {
	{GA_Disabled	, FALSE		},
	{GA_Immediate	, FALSE		},
	{GA_RelVerify	, TRUE		},
	{TAG_MORE	, (IPTR) NULL	}
    };

    tags[0].ti_Data = GetTagData(GA_Disabled, FALSE, taglist);
    tags[1].ti_Data = GetTagData(GA_Immediate, FALSE, taglist);
    tags[3].ti_Data = (IPTR) stdgadtags;

    obj = (struct Gadget *) NewObjectA(GadToolsBase->buttonclass, NULL, tags);
    if (obj)
    {
	obj->GadgetType |= GTYP_GADTOOLS;
    }
    return obj;
}

/****************************************************************************************/

struct Gadget *makecheckbox(struct GadToolsBase_intern *GadToolsBase,
			    struct TagItem stdgadtags[],
			    struct VisualInfo *vi,
			    struct TagItem *taglist)
{
    struct Gadget *obj;
    struct TagItem tags[] =
    {
	{GA_Disabled	, FALSE		},
	{GTCB_Checked	, FALSE		},
	{TAG_MORE	, (IPTR)NULL	}
    };

    EnterFunc(bug("makecheckbox()\n"));

    tags[0].ti_Data = GetTagData(GA_Disabled, FALSE, taglist);
    tags[1].ti_Data = GetTagData(GTCB_Checked, FALSE, taglist);
    tags[2].ti_Data = (IPTR) stdgadtags;

    if (!GetTagData(GTCB_Scaled, FALSE, taglist)) {
        stdgadtags[TAG_Width].ti_Data = CHECKBOX_WIDTH;
        stdgadtags[TAG_Height].ti_Data = CHECKBOX_HEIGHT;
    }

    obj = (struct Gadget *) NewObjectA(GadToolsBase->checkboxclass, NULL, tags);
    if (obj)
    {
	obj->GadgetType |= GTYP_GADTOOLS;
    }

    ReturnPtr("makecheckbox()", struct Gadget *, obj);
}

/****************************************************************************************/

struct Gadget *makecycle(struct GadToolsBase_intern *GadToolsBase,
                         struct TagItem stdgadtags[],
                         struct VisualInfo *vi,
		      	 struct TextAttr *tattr,
                         struct TagItem *taglist)
{
    struct Gadget *obj;
    struct TagItem tags[] =
    {
	{GA_Disabled	, FALSE		}, /* 0 */
	{GTCY_Labels	, FALSE		}, /* 1 */
        {GTCY_Active	, 0		}, /* 2 */
	{GA_RelVerify	, TRUE		}, /* 3 */
	{GA_TextAttr	, 0 	    	}, /* 4 */
	{TAG_MORE	, (IPTR)NULL	}  /* 5 */
    };

    EnterFunc(bug("makecycle()\n"));

    tags[0].ti_Data = GetTagData(GA_Disabled, FALSE, taglist);
    tags[1].ti_Data = GetTagData(GTCY_Labels, FALSE, taglist);
    tags[2].ti_Data = GetTagData(GTCY_Active, 0, taglist);

    /* Be sure not to pass GA_TextAttr, NULL */
    if (tattr)
    	tags[4].ti_Data = (IPTR)tattr;
    else
    	tags[4].ti_Tag = TAG_IGNORE;

    tags[5].ti_Data = (IPTR) stdgadtags;

    obj = (struct Gadget *) NewObjectA(GadToolsBase->cycleclass, NULL, tags);

    if (obj)
    {
	obj->GadgetType |= GTYP_GADTOOLS;
    }

    ReturnPtr("makecycle()", struct Gadget *, obj);
}

/****************************************************************************************/

struct Gadget *makemx(struct GadToolsBase_intern *GadToolsBase,
		      struct TagItem stdgadtags[],
		      struct VisualInfo *vi,
		      struct TextAttr *tattr,
		      struct TagItem *taglist)
{
    struct Gadget *obj;
    int labels = 0;
    STRPTR *labellist;
    struct TagItem *tag, tags[] =
    {
	{GA_Disabled		, FALSE			}, /* 0 */
	{GTMX_Labels		, (IPTR) NULL		}, /* 1 */
	{GTMX_Active		, 0			}, /* 2 */
	{GTMX_Spacing		, 1			}, /* 3 */
        {GTMX_TickHeight	, MX_HEIGHT		}, /* 4 */
        {GTMX_TickLabelPlace	, GV_LabelPlace_Right	}, /* 5 */
	{GA_TextAttr	    	, 0 	    	    	}, /* 6 */
	{TAG_MORE		, (IPTR) NULL		}  /* 7 */
    };

    tags[0].ti_Data = GetTagData(GA_Disabled, FALSE, taglist);
    labellist = (STRPTR *) GetTagData(GTMX_Labels, (IPTR) NULL, taglist);
    if (!labellist)
	return NULL;
    tags[1].ti_Data = (IPTR) labellist;
    tags[2].ti_Data = GetTagData(GTMX_Active, 0, taglist);
    tags[3].ti_Data = GetTagData(GTMX_Spacing, 1, taglist);
    if (GetTagData(GTMX_Scaled, FALSE, taglist))
        tags[4].ti_Data = stdgadtags[TAG_Height].ti_Data;
    else
        stdgadtags[TAG_Width].ti_Data = MX_WIDTH;
    switch (stdgadtags[TAG_LabelPlace].ti_Data & 0x1f)
    {
    case GV_LabelPlace_Left:
        tags[5].ti_Data = GV_LabelPlace_Left;
        break;
    case GV_LabelPlace_Above:
        tags[5].ti_Data = GV_LabelPlace_Above;
        break;
    case GV_LabelPlace_Below:
        tags[5].ti_Data = GV_LabelPlace_Below;
        break;
    }

    /* Be sure not to pass GA_TextAttr, NULL */
    if (tattr)
    	tags[6].ti_Data = (IPTR)tattr;
    else
    	tags[6].ti_Tag = TAG_IGNORE;

    tags[7].ti_Data = (IPTR) stdgadtags;

    tag = FindTagItem(GTMX_TitlePlace, taglist);
    if (tag)
    {
        switch (tag->ti_Data)
        {
        case PLACETEXT_LEFT:
            stdgadtags[TAG_LabelPlace].ti_Data = GV_LabelPlace_Left;
            break;
        case PLACETEXT_RIGHT:
            stdgadtags[TAG_LabelPlace].ti_Data = GV_LabelPlace_Right;
            break;
        case PLACETEXT_ABOVE:
            stdgadtags[TAG_LabelPlace].ti_Data = GV_LabelPlace_Above;
            break;
        case PLACETEXT_BELOW:
            stdgadtags[TAG_LabelPlace].ti_Data = GV_LabelPlace_Below;
            break;
        default:
            freeitext(GadToolsBase,
                      (struct IntuiText *)stdgadtags[TAG_IText].ti_Data);
            stdgadtags[TAG_IText].ti_Data = (IPTR)NULL;
            break;
        }
    } else
    {
        freeitext(GadToolsBase,
                  (struct IntuiText *)stdgadtags[TAG_IText].ti_Data);
        stdgadtags[TAG_IText].ti_Data = (IPTR)NULL;
    }

    while (labellist[labels])
	labels++;

    obj = (struct Gadget *) NewObjectA(GadToolsBase->mxclass, NULL, tags);

    if (obj)
    {
	obj->GadgetType |= GTYP_GADTOOLS;
    }

    return obj;
}

/****************************************************************************************/

struct Gadget *makepalette(struct GadToolsBase_intern *GadToolsBase,
                         struct TagItem stdgadtags[],
                         struct VisualInfo *vi,
                         struct TagItem *taglist)
{
    struct Gadget *obj = NULL;

    struct TagItem *tag, tags[] =
    {
        {GA_RelVerify,		TRUE	}, /* 0 */
        {GA_Disabled,		FALSE	}, /* 1 */
        {GTPA_Depth,		1		}, /* 2 */
        {GTPA_Color,		0		}, /* 3 */
        {GTPA_ColorOffset,	0		}, /* 4 */
        {GTPA_IndicatorWidth,	0	}, /* 5 */
        {GTPA_IndicatorHeight,	0	}, /* 6 */
        {GTPA_NumColors,	2		}, /* 7 */
        {GTPA_ColorTable,	0		}, /* 8 */
        {TAG_MORE, 		(IPTR)NULL	}
    };
    
    /* Could use GetTagData(), but this is faster */
    while ((tag = NextTagItem(&taglist)))
    {
    	IPTR tidata = tag->ti_Data;
    	
	/* Note: GTPA_NumColors overrides GTPA_Depth tag! */
	
    	switch (tag->ti_Tag)
    	{
    	case GA_Disabled:		tags[1].ti_Data = tidata; break;
    	case GTPA_Depth:		tags[2].ti_Data	= tidata; tags[7].ti_Data = 1L << tidata;break;
    	case GTPA_Color:		tags[3].ti_Data	= tidata; break;
    	case GTPA_ColorOffset:		tags[4].ti_Data	= tidata; break;
    	case GTPA_IndicatorWidth:	tags[5].ti_Data	= tidata; break;
    	case GTPA_IndicatorHeight:	tags[6].ti_Data	= tidata; break;
    	case GTPA_NumColors:		tags[7].ti_Data	= tidata; break;
    	case GTPA_ColorTable:		tags[8].ti_Data	= tidata; break;
    	    
    	} /* switch() */
    	
    } /* while (iterate taglist) */

    tags[9].ti_Data = (IPTR)stdgadtags;

    obj = (struct Gadget *) NewObjectA(GadToolsBase->paletteclass, NULL, tags);

    if (obj)
    {
	obj->GadgetType |= GTYP_GADTOOLS;
    }

    return obj;
}

/****************************************************************************************/

struct Gadget *maketext(struct GadToolsBase_intern *GadToolsBase,
                         struct TagItem stdgadtags[],
                         struct VisualInfo *vi,
                         struct TextAttr *tattr,
                         struct TagItem *taglist)
{
    struct Gadget *obj = NULL;
    Class *cl;
    BOOL cliptag_found = FALSE;
    
    struct TagItem *tag, tags[] =
    {
    	{GTTX_Text		, 0			},
    	{GTTX_CopyText		, FALSE			},
    	{GTTX_Clipped		, FALSE			},
    	{GTTX_Border		, FALSE			},
    	{GTTX_FrontPen		, TEXTPEN		},
    	{GTTX_BackPen		, BACKGROUNDPEN		},
    	{GTTX_Justification	, GTJ_LEFT		},
    	{GTA_Text_Format	, (IPTR)"%s"		},
    	{GA_TextAttr		, (IPTR)NULL		},
	{GTA_GadgetKind		, TEXT_KIND		},
	{TAG_MORE		, (IPTR) NULL		}
    };

    /* Could use GetTagData(), but this is faster */
    while ((tag = NextTagItem(&taglist)))
    {
    	IPTR tidata = tag->ti_Data;
    	
    	switch (tag->ti_Tag)
    	{
    	    case GTTX_Text:		tags[0].ti_Data = tidata; break;
    	    case GTTX_CopyText:		tags[1].ti_Data	= tidata; break;
    	    case GTTX_Clipped:		tags[2].ti_Data	= tidata; cliptag_found = TRUE;break;
    	    case GTTX_Border:		tags[3].ti_Data	= tidata; break;
    	    case GTTX_FrontPen:		tags[4].ti_Data	= tidata; break;
    	    case GTTX_BackPen:		tags[5].ti_Data	= tidata; break;
    	    case GTTX_Justification:	tags[6].ti_Data	= tidata; break;    	    
    	}
    	
    } /* while (iterate taglist) */

    /* if GTTX_Clipped was not specified then the default value is
       the GTTX_Border value */
    
    if (!cliptag_found) tags[2].ti_Data = tags[3].ti_Data;
    
    /* Be sure not to pass GA_TextAttr, NULL */
    if (tattr)
    	tags[8].ti_Data = (IPTR)tattr;
    else
    	tags[8].ti_Tag = TAG_IGNORE;
    tags[10].ti_Data = (IPTR)stdgadtags;

    obj = (struct Gadget *) NewObjectA(GadToolsBase->textclass, NULL, tags);

    if (obj)
    {
	obj->GadgetType |= GTYP_GADTOOLS;
    }

    return (obj);
}

/****************************************************************************************/

struct Gadget *makenumber(struct GadToolsBase_intern *GadToolsBase,
                         struct TagItem stdgadtags[],
                         struct VisualInfo *vi,
                         struct TextAttr *tattr,
                         struct TagItem *taglist)
{
    struct Gadget *obj = NULL;
    Class *cl;
    BOOL cliptag_found = FALSE;
    
    struct TagItem *tag, tags[] =
    {
    	{GTNM_Number		, 0		},
    	{GTNM_Format		, (IPTR)"%ld"	},
    	{GTNM_Clipped		, FALSE		},
    	{GTNM_Border		, FALSE		},
    	{GTNM_FrontPen		, TEXTPEN	},
    	{GTNM_BackPen		, BACKGROUNDPEN	},
    	{GTNM_Justification	, GTJ_CENTER	},
    	{GTNM_MaxNumberLen	, 100		},
    	{GA_TextAttr		, (IPTR)NULL	},
	{GTA_GadgetKind		, NUMBER_KIND	},
	{TAG_MORE		, (IPTR) NULL	}
    };
    
   
    /* Could use GetTagData(), but this is faster */
    while ((tag = NextTagItem(&taglist)))
    {
    	IPTR tidata = tag->ti_Data;
    	
    	switch (tag->ti_Tag)
    	{
    	    case GTNM_Number:		tags[0].ti_Data = tidata; break;
    	    case GTNM_Format:		tags[1].ti_Data	= tidata; break;
    	    case GTNM_Clipped:		tags[2].ti_Data	= tidata; cliptag_found = TRUE;break;
    	    case GTNM_Border:		tags[3].ti_Data	= tidata; break;
    	    case GTNM_FrontPen:		tags[4].ti_Data	= tidata; break;
    	    case GTNM_BackPen:		tags[5].ti_Data	= tidata; break;
    	    case GTNM_Justification:	tags[6].ti_Data	= tidata; break;
    	    case GTNM_MaxNumberLen:	tags[7].ti_Data	= tidata; break;    	    
    	}
    	
    } /* while (iterate taglist) */

    /* if GTNM_Clipped was not specified then the default value is
       the GTNM_Border value */
    
    if (!cliptag_found) tags[2].ti_Data = tags[3].ti_Data;

    /* Be sure not to pass GA_TextAttr, NULL */
    if (tattr)
    	tags[8].ti_Data = (IPTR)tattr;
    else
    	tags[8].ti_Tag = TAG_IGNORE;
    	
    tags[10].ti_Data = (IPTR)stdgadtags;

    obj = (struct Gadget *) NewObjectA(GadToolsBase->textclass, NULL, tags);

    if (obj)
    {
	obj->GadgetType |= GTYP_GADTOOLS;
    }

    return  (obj);
}


/****************************************************************************************/

/* This MUST be global, since the gadgetclass doesn't copy ICA_MAPPINGs */
const struct TagItem slider2level[] =
{
    {GTSL_Level,	GTNM_Number},
    {TAG_DONE }
};

/****************************************************************************************/

struct Gadget *makeslider(struct GadToolsBase_intern *GadToolsBase,
                         struct TagItem stdgadtags[],
                         struct VisualInfo *vi,
                         struct TextAttr *tattr,
                         struct TagItem *taglist)
{

    struct TagItem *tag;
    struct IBox bbox;
    
    struct TagItem stags[] =
    {
    	{GA_Disabled	    , FALSE		},
    	{GA_RelVerify	    , TRUE		},	/* Georg S.: was false */
    	{GA_Immediate	    , TRUE		},	/* Georg S.: was false */
    	{GTSL_Min	    , 0		    	},
    	{GTSL_Max	    , 15		},
    	{GTSL_Level	    , 0		    	},
    	{PGA_Freedom	    , FREEHORIZ	    	},
	{PGA_Borderless	    , TRUE		},
#ifdef __MORPHOS__
	{PGA_NewLook	    , FALSE		},
#else
	{PGA_NewLook	    , TRUE		},
#endif
	{GA_Bounds	    , (IPTR)&bbox	},
	{GA_FollowMouse	    , TRUE		},
	{PGA_NotifyBehaviour, PG_BEHAVIOUR_NICE },
	{PGA_RenderBehaviour, PG_BEHAVIOUR_NICE },
    	{TAG_MORE	    , (IPTR)NULL	}
    };
    
    struct TagItem ltags[] = 
    {
    	 {GA_Left		, 0		},
    	 {GA_Top		, 0		},
    	 {GA_Width		, 0		},
    	 {GA_Height		, 0		},
    	 {GA_TextAttr		, (IPTR)tattr	},
    	 {GTNM_Format		, (IPTR)NULL	},
    	 {GTNM_Justification	, GTJ_LEFT	},
	 {GTA_Text_DispFunc	, (IPTR)NULL	},
	 {GA_Next		, (IPTR)NULL	},
	 {GA_DrawInfo		, (IPTR)NULL	},
	 {GTNM_Number		, 0		},
	 {GTA_GadgetKind	, SLIDER_KIND	},
	 {GA_Previous		, (IPTR)NULL	},
	 {TAG_DONE				}
    };
    STRPTR lformat = NULL;
    WORD lmaxlen = 0;
    LONG lmaxpixellen = 0L;
    UBYTE lplace = PLACETEXT_LEFT;
    WORD level = 0;
    
    struct Gadget *slidergad = NULL, *levelgad = NULL;

    /* Parse tags */
    while ((tag = NextTagItem(&taglist)))
    {
    	IPTR tidata = tag->ti_Data;
    	    
    	switch (tag->ti_Tag)
    	{
    	
    	    /* Slider tags */
    	    case GA_Disabled:	stags[0].ti_Data = tidata; break;
    	    case GA_RelVerify:	stags[1].ti_Data = tidata; break;
    	    case GA_Immediate:	stags[2].ti_Data = tidata; break;
    	    case GTSL_Min:	stags[3].ti_Data = tidata; break;
    	    case GTSL_Max:	stags[4].ti_Data = tidata; break;
    	    case GTSL_Level:	level = stags[5].ti_Data = tidata; break;
    	    case PGA_Freedom:
    		if (tidata == LORIENT_HORIZ)
    	    	    stags[6].ti_Data = FREEHORIZ;
    		else
    	    	    stags[6].ti_Data = FREEVERT;
    		break;

    	    /* Level tags */    
    	    case GTSL_LevelFormat:   lformat          = (STRPTR)tidata;	break;
    	    case GTSL_MaxLevelLen:   lmaxlen          = (UWORD)tidata;	break;
	    case GTSL_MaxPixelLen:   lmaxpixellen     = (ULONG)tidata; 	break;
	    case GTSL_LevelPlace:    lplace	      = (UBYTE)tidata;	break;
    	    case GTSL_Justification: ltags[6].ti_Data = tidata;	  	break;
    	    case GTSL_DispFunc:	     ltags[7].ti_Data = tidata;  	break;

    	} 
    	    
    } /* while (iterate taglist) */
    
    /* if there is a bounding box the label position
       will be calculated based on this box and not
       the gadget coords */

    bbox.Left   = GetTagData(GA_Left, 0, stdgadtags);
    bbox.Top    = GetTagData(GA_Top,  0, stdgadtags);
    bbox.Width  = GetTagData(GA_Width, 0, stdgadtags);
    bbox.Height = GetTagData(GA_Height, 0, stdgadtags);

    /* There are always GA_Left, GA_Top, GA_Width and GA_Height tags
       thanks to creategadgeta.c! */
      
    FindTagItem(GA_Left,stdgadtags)->ti_Data   += BORDERPROPSPACINGX;
    FindTagItem(GA_Top,stdgadtags)->ti_Data    += BORDERPROPSPACINGY;
    FindTagItem(GA_Width,stdgadtags)->ti_Data  -= BORDERPROPSPACINGX * 2;
    FindTagItem(GA_Height,stdgadtags)->ti_Data -= BORDERPROPSPACINGY * 2;

    /* Create slider gadget */
    stags[13].ti_Data = (IPTR)stdgadtags;
    slidergad = NewObjectA(GadToolsBase->sliderclass, NULL, stags);

    if (!slidergad)
    	return (NULL);

    slidergad->GadgetType |= GTYP_GADTOOLS;

    if (lformat || lmaxlen || lmaxpixellen)
    {
	WORD x = 0, y = 0;
	UWORD ysize = 8;
	Class *textcl;
	
	struct TagItem lntags[] =
	{
	     {ICA_TARGET, (IPTR)NULL	},
	     {ICA_MAP	, (IPTR)NULL	},
	     {TAG_DONE			}
	};
	    
    	/* Set some defaults */
    	if (!lformat)
    	    lformat = "%ld";
    	if (!lmaxlen)
    	    lmaxlen = 2;
    	if (!lmaxpixellen)
    	{
    	    struct TextFont *font;
    	    UWORD xsize;
    	    
    	    ysize = vi->vi_dri->dri_Font->tf_YSize;
    	    xsize = vi->vi_dri->dri_Font->tf_XSize;

	    if (tattr)
	    {
    	    	font = OpenFont(tattr);
    	    	if (font)
    	   	{
    	   	    ysize = font->tf_YSize;
    	   	    xsize = font->tf_XSize;
    	   	    CloseFont(font);
    	   	}
    	   	else /* If no valid tattr */
    	   	    ltags[4].ti_Tag = TAG_IGNORE;
    	    }
    	    else /* If no valid tattr */
    	        ltags[4].ti_Tag = TAG_IGNORE;

    	    lmaxpixellen = lmaxlen * xsize;
    	        
    	    ltags[4].ti_Data = (ULONG)tattr;
    	    	
    	} /* if (!lmaxpixellen) */
    	
    	switch (lplace)
    	{
    	    case PLACETEXT_LEFT:
            	x = slidergad->LeftEdge - BORDERPROPSPACINGX - lmaxpixellen - 4;
            	y = slidergad->TopEdge + (slidergad->Height - ysize) / 2 + 1;
    	    	break;
    	    case PLACETEXT_RIGHT:
            	x = slidergad->LeftEdge + slidergad->Width + BORDERPROPSPACINGX + 5;
            	y = slidergad->TopEdge  + (slidergad->Height - ysize) / 2 + 1;
    	    	break;
    	    case PLACETEXT_ABOVE:
            	x = slidergad->LeftEdge - (lmaxpixellen - slidergad->Width) / 2;
            	y = slidergad->TopEdge  - BORDERPROPSPACINGY - ysize - 2;
    	    	break;
    	    case PLACETEXT_BELOW:
            	x = slidergad->LeftEdge - (lmaxpixellen - slidergad->Width) / 2;
            	y = slidergad->TopEdge  + slidergad->Height + BORDERPROPSPACINGY + 3;
 	    	break;
    	}

        /* Create the levelobj */
    	
    	ltags[0].ti_Data = (IPTR)x;
    	ltags[1].ti_Data = (IPTR)y;
    	ltags[2].ti_Data = (IPTR)lmaxpixellen;
    	ltags[3].ti_Data = (IPTR)ysize;
    	ltags[5].ti_Data = (IPTR)lformat;
    	ltags[8].ti_Data = (IPTR)slidergad;
    	ltags[9].ti_Data = (IPTR)vi->vi_dri;
    	ltags[10].ti_Data = (IPTR)level;
	ltags[12].ti_Data = (IPTR)GetTagData(GA_Previous, 0, stdgadtags);

    	levelgad = (struct Gadget *)NewObjectA(GadToolsBase->textclass, NULL, ltags);
    	if (!levelgad)
    	{
    	    DisposeObject((Object *)slidergad);
    	    return (NULL);
    	}
	levelgad->GadgetType |= GTYP_GADTOOLS;
    	
    	/* Set up a notification from the slider to the level */
    	lntags[0].ti_Data = (IPTR)levelgad;
    	lntags[1].ti_Data = (IPTR)slider2level;
    	SetAttrsA((Object *)slidergad, lntags);

	return (levelgad);
    	    
    } /* if (slider should have a level attached) */
    
    return (slidergad);

}                         

/****************************************************************************************/

#ifdef SDEBUG
#   undef SDEBUG
#endif
#ifdef DEBUG
#   undef DEBUG
#endif
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/

struct Gadget *makescroller(struct GadToolsBase_intern *GadToolsBase,
                         struct TagItem stdgadtags[],
                         struct VisualInfo *vi,
                         struct TagItem *taglist)
{
    struct Gadget *scroller = NULL,
    		  *arrow_dec = NULL,
    		  *arrow_inc = NULL ;
    struct IBox bbox;

    struct TagItem *tag, stags[] =
    {
    	{GTSC_Top	    , 0		    	},
    	{GTSC_Total	    , 0		    	},
    	{GTSC_Visible	    , 2		    	},
    	{PGA_Freedom	    , FREEHORIZ	    	},
    	{GA_Disabled	    , FALSE		},
    	{GA_RelVerify	    , TRUE		},
    	{GA_Immediate	    , TRUE		},
	{GTA_GadgetKind	    , SCROLLER_KIND	},
	{PGA_Borderless	    , TRUE		},
#ifdef __MORPHOS__
	{PGA_NewLook	    , FALSE		},
#else
	{PGA_NewLook	    , TRUE		},
#endif
	{GA_Bounds	    , (IPTR) &bbox	},
	{GA_FollowMouse	    , TRUE		},
	{PGA_NotifyBehaviour, PG_BEHAVIOUR_NICE },
	{PGA_RenderBehaviour, PG_BEHAVIOUR_NICE },
	{TAG_MORE	    , (IPTR)NULL	}
    };
    
    struct TagItem *scr_dim_tagitem;
    
 
    
    UWORD freedom = stags[3].ti_Data; /* default */
    WORD arrowdim = 0 /* -1*/, arrowkind = SCROLLER_KIND;
    BOOL relverify, immediate;
    ULONG scr_dim_tag;
    
    EnterFunc(bug("makescroller(stdgadtags=%p, vi=%p, taglist = %p)\n",
    		stdgadtags, vi, taglist));
    /* Could use GetTagData(), but this is faster */
    while ((tag = NextTagItem(&taglist)))
    {
    	IPTR tidata = tag->ti_Data;
    	
    	switch (tag->ti_Tag)
    	{
    	    case GTSC_Top:	stags[0].ti_Data = tidata; break;
    	    case GTSC_Total:	stags[1].ti_Data = tidata; break;
    	    case GTSC_Visible:	stags[2].ti_Data = tidata; break;
    	    case PGA_Freedom:
    		if (tidata == LORIENT_HORIZ)
    	    	    freedom = stags[3].ti_Data = FREEHORIZ;
    		else
    	    	    freedom = stags[3].ti_Data = FREEVERT;
    		break;
    	    case GA_Disabled:	stags[4].ti_Data = tidata; break;
    	    case GA_RelVerify:	relverify = stags[5].ti_Data = tidata; break;
    	    case GA_Immediate:	immediate = stags[6].ti_Data = tidata; break;

    	    case GTSC_Arrows:	arrowdim = (WORD)tidata; break;
	    case GTA_Scroller_ArrowKind: arrowkind = (WORD)tidata; break;
	    case GTA_Scroller_ScrollerKind: stags[7].ti_Data = tidata; break;
    	    
    	}
    	
    } /* while (iterate taglist) */
    
    /* if there is a bounding box the label position
       will be calculated based on this box and not
       the gadget coords */

    DEBUG_CREATESCROLLER(bug("makescroller: arrowdim %ld\n", arrowdim));
       
    bbox.Left   = GetTagData(GA_Left, 0, stdgadtags);
    bbox.Top    = GetTagData(GA_Top,  0, stdgadtags);
    bbox.Width  = GetTagData(GA_Width, 0, stdgadtags);
    bbox.Height = GetTagData(GA_Height, 0, stdgadtags);

    DEBUG_CREATESCROLLER(bug("makescroller: left %ld top %ld width %ld height %ld\n", bbox.Left, bbox.Top, bbox.Width, bbox.Height));

    stags[14].ti_Data = (IPTR)stdgadtags;
    
    /* Substract the arrow's total size from the sroller's size */

    if (arrowdim == -1)
    {
 	DEBUG_CREATESCROLLER(bug("makescroller: default arrowdim\n"));
   	if (freedom == FREEVERT)
	{
	    arrowdim = GetTagData(GA_Width, 16, stdgadtags);
	    DEBUG_CREATESCROLLER(bug("makescroller: freevert arrowdim %ld\n", arrowdim));
	} else {
	    arrowdim = GetTagData(GA_Height, 16, stdgadtags);
	    DEBUG_CREATESCROLLER(bug("makescroller: freehoriz arrowdim %ld\n", arrowdim));
	}
	DEBUG_CREATESCROLLER(bug("makescroller: arrowdim %ld\n", arrowdim));
    }
    
    scr_dim_tag = ((freedom == FREEVERT) ? GA_Height : GA_Width);
    
    scr_dim_tagitem = FindTagItem(scr_dim_tag, stdgadtags);
    scr_dim_tagitem->ti_Data -= 2 * arrowdim;

    /* There are always GA_Left, GA_Top, GA_Width and GA_Height tags
       thanks to creategadgeta.c! */
      
    FindTagItem(GA_Left,stdgadtags)->ti_Data   += BORDERPROPSPACINGX;
    FindTagItem(GA_Top,stdgadtags)->ti_Data    += BORDERPROPSPACINGY;
    FindTagItem(GA_Width,stdgadtags)->ti_Data  -= BORDERPROPSPACINGX * 2;
    FindTagItem(GA_Height,stdgadtags)->ti_Data -= BORDERPROPSPACINGY * 2;

    scroller = (struct Gadget *) NewObjectA(GadToolsBase->scrollerclass, NULL, stags);

    if (!scroller)
    	return (NULL);

    scroller->GadgetType |= GTYP_GADTOOLS;

    DEBUG_CREATESCROLLER(bug("makescroller: scroller gadget 0x%lx scroller Width %ld Height %ld\n",scroller,scroller->Width,scroller->Height));
    DEBUG_CREATESCROLLER(bug("makescroller: scroller nextgadget 0x%lx\n",scroller->NextGadget));
    
    if (arrowdim) /* Scroller has arrows ? */
    {
    	struct TagItem atags[] =
    	{
    	    {GA_Left		, 0		},
    	    {GA_Top		, 0		},
    	    {GA_Width		, 0		},
    	    {GA_Height		, 0		},
    	    {GTA_Arrow_Type	, 0		},
    	    {GA_DrawInfo	, (IPTR)NULL	},
    	    {GA_Next	    	, (IPTR)NULL	},
    	    {GTA_Arrow_Scroller	, (IPTR)NULL	},
    	    {GA_RelVerify	, TRUE		},
    	    {GA_Immediate	, TRUE		},
    	    {GA_ID		, 0		},
	    {GTA_GadgetKind	, arrowkind	},
	    {GA_Previous	, (IPTR)NULL	},
    	    {TAG_DONE				}
    	};
    	struct TagItem tellscrollertags[] =
	{
	    {GTA_Scroller_Arrow1, 0		},
	    {GTA_Scroller_Arrow2, 0		},
	    {TAG_DONE				}
	};
	
    	atags[5].ti_Data = (IPTR)vi->vi_dri;    /* Set GA_DrawInfo */
    	atags[6].ti_Data = (IPTR)scroller;	/* Set GA_Previous */
    	atags[7].ti_Data = (IPTR)scroller;	/* Set GTA_Arrow_Scroller */
	atags[12].ti_Data = (IPTR)GetTagData(GA_Previous, 0, stdgadtags);

    	/* These must be the same as for scroller */
/*    	atags[8].ti_Data = (IPTR)relverify;
    	atags[9].ti_Data = (IPTR)immediate;*/
    	atags[10].ti_Data = (IPTR)GetTagData(GA_ID, 0, stdgadtags); 
    	
    	if (freedom == FREEVERT)
    	{
	    DEBUG_CREATESCROLLER(bug("makescroller: Freedom=FREEVERT\n"));
    	    atags[0].ti_Data = scroller->LeftEdge - BORDERPROPSPACINGX;
    	    atags[1].ti_Data = scroller->TopEdge + BORDERPROPSPACINGY + scroller->Height;
    	    atags[2].ti_Data = scroller->Width + BORDERPROPSPACINGX * 2;
    	    atags[3].ti_Data = arrowdim;
    	    atags[4].ti_Data = UPIMAGE;
    	    
	    DEBUG_CREATESCROLLER(bug("makescroller: Width %ld Height %ld\n",atags[2].ti_Data,atags[3].ti_Data));

    	    arrow_dec = NewObjectA(GadToolsBase->arrowclass, NULL, atags);
    	    if (!arrow_dec)
    	    	goto failure;

	    arrow_dec->GadgetType |= GTYP_GADTOOLS;

	    DEBUG_CREATESCROLLER(bug("makescroller: arrow_dec gadget 0x%lx\n",arrow_dec));
	    DEBUG_CREATESCROLLER(bug("makescroller: arrow_dec nextgadget 0x%lx\n",arrow_dec->NextGadget));
    	    
    	    ((ULONG)atags[1].ti_Data) += arrowdim;
    	    atags[4].ti_Data = DOWNIMAGE;
    	    atags[6].ti_Data = (IPTR)arrow_dec;
	
    	    arrow_inc = NewObjectA(GadToolsBase->arrowclass, NULL, atags);
    	    if (!arrow_inc)
    	    	goto failure;

	    arrow_inc->GadgetType |= GTYP_GADTOOLS;

	    DEBUG_CREATESCROLLER(bug("makescroller: arrow_inc gadget 0x%lx\n",arrow_inc));
	    DEBUG_CREATESCROLLER(bug("makescroller: arrow_inc nextgadget 0x%lx\n",arrow_inc->NextGadget));
    	    
    	}
    	else
    	{
	    DEBUG_CREATESCROLLER(bug("makescroller: Freedom=FREEHORIZ\n"));

    	    atags[0].ti_Data = scroller->LeftEdge + scroller->Width + BORDERPROPSPACINGX;
    	    atags[1].ti_Data = scroller->TopEdge - BORDERPROPSPACINGY;
    	    atags[2].ti_Data = arrowdim;
    	    atags[3].ti_Data = scroller->Height + BORDERPROPSPACINGY * 2;
    	    atags[4].ti_Data = LEFTIMAGE;

	    DEBUG_CREATESCROLLER(bug("makescroller: Width %ld Height %ld\n",atags[3].ti_Data,atags[2].ti_Data));
    	    
    	    arrow_dec = NewObjectA(GadToolsBase->arrowclass, NULL, atags);
    	    if (!arrow_dec)
    	    	goto failure;

	    arrow_dec->GadgetType |= GTYP_GADTOOLS;

	    DEBUG_CREATESCROLLER(bug("makescroller: arrow_dec gadget 0x%lx\n",arrow_dec));
    	    
    	    ((ULONG)atags[0].ti_Data) += arrowdim;
    	    atags[4].ti_Data = RIGHTIMAGE;
    	    atags[6].ti_Data = (IPTR)arrow_dec;

    	    arrow_inc = NewObjectA(GadToolsBase->arrowclass, NULL, atags);
    	    if (!arrow_inc)
    	    	goto failure;
    	    	
	    arrow_inc->GadgetType |= GTYP_GADTOOLS;

	    DEBUG_CREATESCROLLER(bug("makescroller: arrow_inc gadget 0x%lx\n",arrow_inc));
    	    	
    	} /* if (scroller is FREEVERT or FREEHORIZ) */
    	
	tellscrollertags[0].ti_Data = (IPTR)arrow_dec;
	tellscrollertags[1].ti_Data = (IPTR)arrow_inc;
	
	DEBUG_CREATESCROLLER(bug("makescroller: tell scroller about arrows\n"));
	SetAttrsA(scroller, tellscrollertags);

	ReturnPtr ("makescroller", struct Gadget *, arrow_inc);
	
    } /* if (scroller should have arrows attached) */
    
    ReturnPtr ("makescroller", struct Gadget *, scroller);
    
failure:
   if (scroller)
   	DisposeObject((Object *)scroller);

   if (arrow_dec)
   	DisposeObject((Object *)arrow_dec);

   if (arrow_inc)
   	DisposeObject((Object *)arrow_inc);
   
   ReturnPtr("makescroller", struct Gadget *, NULL);
}

/****************************************************************************************/

struct Gadget *makestring(struct GadToolsBase_intern *GadToolsBase,
                         struct TagItem stdgadtags[],
                         struct VisualInfo *vi,
                         struct TextAttr *tattr,
                         struct TagItem *taglist)
{
    struct Gadget *obj = NULL;
    struct IBox bbox;
    
    Class *cl;

    struct TagItem *tag, tags[] =
    {
    	{GA_Disabled		, FALSE			}, /* 0 */
    	{GA_Immediate		, FALSE			}, /* 1 */
	{GA_RelVerify		, TRUE			}, /* 2 */
    	{GA_TabCycle		, TRUE			}, /* 3 */
    	{GTST_String		, (IPTR)NULL		}, /* 4 */
    	{GTST_MaxChars		, 64UL			}, /* 5 */ 
    	{GTST_EditHook		, (IPTR)NULL		}, /* 6 */
    	{STRINGA_ExitHelp	, FALSE			}, /* 7 */
    	{STRINGA_Justification	, GACT_STRINGLEFT	}, /* 8 */
    	{STRINGA_ReplaceMode	, FALSE			}, /* 9 */
    	{GA_TextAttr		, (IPTR)NULL		}, /* 10 */
	{GTA_GadgetKind		, STRING_KIND		}, /* 11 */
	{GA_Bounds		, (IPTR)&bbox		}, /* 12 */
	{TAG_MORE		, (IPTR)NULL		}  /* 13 */
    };
    
   
    /* Could use GetTagData(), but this is faster */
    while ((tag = NextTagItem(&taglist)))
    {
    	IPTR tidata = tag->ti_Data;
    	
    	switch (tag->ti_Tag)
    	{
    	    case GA_Disabled:		tags[0].ti_Data = tidata; break;
    	    case GA_Immediate:		tags[1].ti_Data	= tidata; break;
	    case GA_RelVerify:        	tags[2].ti_Data = tidata; break;
    	    case GA_TabCycle:		tags[3].ti_Data	= tidata; break;
    	    case GTST_String:		tags[4].ti_Data	= tidata; break;
    	    case GTST_MaxChars:		tags[5].ti_Data	= tidata; break;
    	    case GTST_EditHook:		tags[6].ti_Data	= tidata; break;
    	    case STRINGA_ExitHelp:	tags[7].ti_Data	= tidata; break;
    	    case STRINGA_Justification:	tags[8].ti_Data	= tidata; break;
    	    case STRINGA_ReplaceMode:	tags[9].ti_Data	= tidata; break;
    	}
    	
    } /* while (iterate taglist) */
    
    if (tattr) /* Text Attr supplied ? */
	tags[10].ti_Data = (IPTR)tattr; 
    else
    	tags[10].ti_Tag = TAG_IGNORE; /* Don't pass GA_TextAttr, NULL */
	
    /* if there is a bounding box the label position
       will be calculated based on this box and not
       the gadget coords */
       
    bbox.Left   = GetTagData(GA_Left, 0, stdgadtags);
    bbox.Top    = GetTagData(GA_Top,  0, stdgadtags);
    bbox.Width  = GetTagData(GA_Width, 0, stdgadtags);
    bbox.Height = GetTagData(GA_Height, 0, stdgadtags);

    /* There are always GA_Left, GA_Top, GA_Width and GA_Height tags
       thanks to creategadgeta.c! */
      
    FindTagItem(GA_Left,stdgadtags)->ti_Data   += BORDERSTRINGSPACINGX;
    FindTagItem(GA_Top,stdgadtags)->ti_Data    += BORDERSTRINGSPACINGY;
    FindTagItem(GA_Width,stdgadtags)->ti_Data  -= BORDERSTRINGSPACINGX * 2;
    FindTagItem(GA_Height,stdgadtags)->ti_Data -= BORDERSTRINGSPACINGY * 2;

    tags[13].ti_Data = (IPTR)stdgadtags;

    obj = (struct Gadget *) NewObjectA(GadToolsBase->stringclass, NULL, tags);

    if (obj)
    {
	obj->GadgetType |= GTYP_GADTOOLS;
    }
    return  (obj);
}

/****************************************************************************************/

struct Gadget *makeinteger(struct GadToolsBase_intern *GadToolsBase,
                         struct TagItem stdgadtags[],
                         struct VisualInfo *vi,
                         struct TextAttr *tattr,
                         struct TagItem *taglist)
{
    struct Gadget *obj = NULL;
    struct IBox bbox;
    Class *cl;

    struct TagItem *tag, tags[] =
    {
    	{GA_Disabled		, FALSE			}, /* 0 */
    	{GA_Immediate		, FALSE			}, /* 1 */
	{GA_RelVerify		, TRUE			}, /* 2 */
    	{GA_TabCycle		, TRUE			}, /* 3 */
    	{GTIN_Number		, 0L			}, /* 4 */
    	{GTIN_MaxChars		, 10L			}, /* 5 */
    	{GTIN_EditHook		, (IPTR)NULL		}, /* 6 */
    	{STRINGA_ExitHelp	, FALSE			}, /* 7 */
    	{STRINGA_Justification	, GACT_STRINGLEFT	}, /* 8 */
    	{STRINGA_ReplaceMode	, FALSE			}, /* 9 */
    	{GA_TextAttr		, (IPTR)NULL		}, /* 10 */
	{GTA_GadgetKind		, INTEGER_KIND		}, /* 11 */
	{GA_Bounds		, (IPTR)&bbox		}, /* 12 */
	{TAG_MORE		, (IPTR)NULL		}  /* 13 */
    };
    
   
    /* Could use GetTagData(), but this is faster */
    while ((tag = NextTagItem(&taglist)))
    {
    	IPTR tidata = tag->ti_Data;
    	
    	switch (tag->ti_Tag)
    	{
    	    case GA_Disabled:		tags[0].ti_Data = tidata; break;
    	    case GA_Immediate:		tags[1].ti_Data	= tidata; break;
	    case GA_RelVerify:		tags[2].ti_Data = tidata; break;
    	    case GA_TabCycle:		tags[3].ti_Data	= tidata; break;
    	    case GTIN_Number:		tags[4].ti_Data	= tidata; break;
    	    case GTIN_MaxChars:		tags[5].ti_Data	= tidata; break;
    	    case GTIN_EditHook:		tags[6].ti_Data	= tidata; break;
    	    case STRINGA_ExitHelp:	tags[7].ti_Data	= tidata; break;
    	    case STRINGA_Justification:	tags[8].ti_Data	= tidata; break;
    	    case STRINGA_ReplaceMode:	tags[9].ti_Data	= tidata; break;
    	}
    	
    } /* while (iterate taglist) */

    if (tattr) /* Text Attr supplied ? */
	tags[10].ti_Data = (IPTR)tattr; 
    else
    	tags[10].ti_Tag = TAG_IGNORE; /* Don't pass GA_TextAttr, NULL */

    /* if there is a bounding box the label position
       will be calculated based on this box and not
       the gadget coords */
       
    bbox.Left   = GetTagData(GA_Left, 0, stdgadtags);
    bbox.Top    = GetTagData(GA_Top,  0, stdgadtags);
    bbox.Width  = GetTagData(GA_Width, 0, stdgadtags);
    bbox.Height = GetTagData(GA_Height, 0, stdgadtags);

    /* There are always GA_Left, GA_Top, GA_Width and GA_Height tags
       thanks to creategadgeta.c! */
      
    FindTagItem(GA_Left,stdgadtags)->ti_Data   += BORDERSTRINGSPACINGX;
    FindTagItem(GA_Top,stdgadtags)->ti_Data    += BORDERSTRINGSPACINGY;
    FindTagItem(GA_Width,stdgadtags)->ti_Data  -= BORDERSTRINGSPACINGX * 2;
    FindTagItem(GA_Height,stdgadtags)->ti_Data -= BORDERSTRINGSPACINGY * 2;

    tags[13].ti_Data = (IPTR)stdgadtags;

    obj = (struct Gadget *) NewObjectA(GadToolsBase->stringclass, NULL, tags);
    if (obj)
    {
	obj->GadgetType |= GTYP_GADTOOLS;
    }
    return  (obj);
}


/****************************************************************************************/

const struct TagItem scroller2lv[] =
{
    {PGA_Top	,	GTLV_Top},
    {TAG_DONE	, 		}
};

/****************************************************************************************/

/* Spacing between scroller and listview */
#define SCROLLER_SPACING 2

/****************************************************************************************/

struct Gadget *makelistview(struct GadToolsBase_intern *GadToolsBase,
                         struct TagItem stdgadtags[],
                         struct VisualInfo *vi,
                         struct TextAttr *tattr,
                         struct TagItem *taglist)
{
    struct Gadget *lvgad = NULL, *showselgad = LV_SHOWSELECTED_NONE;
    struct Gadget *scrollergad;
    struct IBox bbox;


    WORD scroller_width = 16; /* default */
    
    struct TagItem *lv_width_tag, *lv_height_tag;
    WORD lv_width, lv_height;
#if CORRECT_LISTVIEWHEIGHT
    WORD viewheight;
#endif
    WORD ysize, totalitemheight = 0;
    
    struct TagItem *tag, lvtags[] =
    {
    	{GA_Disabled		, FALSE			}, /* 0  */
    	{GTLV_Top		, 0L			}, /* 1  */
    	{GTLV_MakeVisible	, 0L			}, /* 2  */
    	{GTLV_Labels		, (IPTR)NULL		}, /* 3  */
    	{GTLV_Selected		, ~0L			}, /* 4  */
    	{GTLV_ItemHeight	, 0L			}, /* 5  */
    	{GTLV_CallBack		, (IPTR)NULL		}, /* 6  */
    	{GTLV_MaxPen		, 0L			}, /* 7  */
    	{GTLV_ReadOnly		, 0L			}, /* 8  */
    	{LAYOUTA_Spacing	, 0L			}, /* 9  */    	
    	{GA_TextAttr		, (IPTR)NULL		}, /* 10 */
	{GA_RelVerify		, TRUE			}, /* 11 */
	{GA_Bounds		, (IPTR)&bbox		}, /* 12 */
	{GTLV_ShowSelected	, (IPTR)showselgad	}, /* 13 */
	{TAG_MORE		, (IPTR)NULL		}  /* 14 */
    };
    
    EnterFunc(bug("makelistview()\n"));
    
    /* Could use GetTagData(), but this is faster */
    while ((tag = NextTagItem(&taglist)))
    {
    	IPTR tidata = tag->ti_Data;
    	
    	switch (tag->ti_Tag)
    	{
    	    case GA_Disabled:		lvtags[0].ti_Data = tidata; break;
    	    case GTLV_Top:		lvtags[1].ti_Data = tidata; break;
    	    case GTLV_MakeVisible:	lvtags[2].ti_Data = tidata; break;
    	    case GTLV_Labels:		lvtags[3].ti_Data = tidata; break;
    	    case GTLV_Selected:		lvtags[4].ti_Data = tidata; break;
    	    case GTLV_ItemHeight:	lvtags[5].ti_Data = tidata; break;
    	    case GTLV_CallBack:		lvtags[6].ti_Data = tidata; break;
    	    case GTLV_MaxPen:		lvtags[7].ti_Data = tidata; break;
    	    case GTLV_ReadOnly:		lvtags[8].ti_Data = tidata; break;
    	    case LAYOUTA_Spacing:	lvtags[9].ti_Data = tidata; break;

    	    case GTLV_ShowSelected:
	        showselgad  = (struct Gadget *)tidata;
		lvtags[13].ti_Data = (IPTR)showselgad;
		break;
		
    	    case GTLV_ScrollWidth:	scroller_width = (UWORD)tidata;
    	}
    	
    } /* while (iterate taglist) */

    /* if there is a bounding box the label position
       will be calculated based on this box and not
       the gadget coords */
       
    bbox.Left   = GetTagData(GA_Left, 0, stdgadtags);
    bbox.Top    = GetTagData(GA_Top,  0, stdgadtags);
    bbox.Width  = GetTagData(GA_Width, 0, stdgadtags);
    bbox.Height = GetTagData(GA_Height, 0, stdgadtags);

    /* Callback supplied ? */
    if (!lvtags[6].ti_Data)
    	lvtags[6].ti_Tag = TAG_IGNORE; /* Don't pass GTLV_Callback, NULL */

    if (tattr) /* Text Attr supplied ? */
    {
	lvtags[10].ti_Data = (IPTR)tattr; 
	ysize = tattr->ta_YSize;
    }
    else
    {
    	lvtags[10].ti_Tag = TAG_IGNORE; /* Don't pass GA_TextAttr, NULL */
    	
    	ysize = vi->vi_dri->dri_Font->tf_YSize;
    }
    
    /* If not set allready, set ItemHeight */
    if (lvtags[5].ti_Data == 0)
    	lvtags[5].ti_Data = ysize;
    	
    /* If not set allready, set Spacing */
    if (lvtags[9].ti_Data == 0)
    	lvtags[9].ti_Data = LV_DEF_INTERNAL_SPACING;

    DEBUG_CREATELISTVIEW(bug("makelistview: item ysize %ld\n", ysize));
    	     
    /* Find the dimension specific tags */
    lv_width_tag  = FindTagItem(GA_Width,  stdgadtags);
    lv_height_tag = FindTagItem(GA_Height, stdgadtags);
    
    lv_width  = (WORD)lv_width_tag->ti_Data;
    lv_height = (WORD)lv_height_tag->ti_Data;
    
    	
    /* Adjust the listview width according to scroller width + some spacing */
    lv_width -= (scroller_width + SCROLLER_SPACING);
    
    /* Adjust the listview height according to showsel gadget height */
    if ((showselgad) && (showselgad != LV_SHOWSELECTED_NONE))
    {
        /* The showselected string gadget must have the same width as the
	   listview gadget, otherwise fail (AmigaOS does the same). Fail
	   also if the showselected string gadget is too high*/
	   
        if ((EG(showselgad)->BoundsWidth != bbox.Width) ||
	    (EG(showselgad)->BoundsHeight >= lv_height))
	{	
            ReturnPtr ("makelistview", struct Gadget *, NULL);
        }
	
	/* the showselected string gadget shrinks the height of the listview */
	
    	lv_height -= EG(showselgad)->BoundsHeight;
	
	/* the showselected string gadget will get its position automatically
	   fixed to be under the listview gadget */
	   
	EG(showselgad)->BoundsLeftEdge = bbox.Left;
	EG(showselgad)->BoundsTopEdge  = bbox.Top + lv_height;
	EG(showselgad)->LeftEdge       = EG(showselgad)->BoundsLeftEdge + BORDERSTRINGSPACINGX;
	EG(showselgad)->TopEdge        = EG(showselgad)->BoundsTopEdge  + BORDERSTRINGSPACINGY;
	
	D(bug("makelistview: Showselected gadget specified"));

    }
    
			/* GTLV_ItemHeight + LAYOUTA_Spacing */
    totalitemheight = lvtags[5].ti_Data + lvtags[9].ti_Data;

#if CORRECT_LISTVIEWHEIGHT    
 /* stegerg: I think it looks better without this adjustment. Think of
    GTLV_ShowSelected and aligning with other gadgets */
    
    /* Adjust listview height so that an exact number of items fits in it */
    viewheight = lv_height - (2 * LV_BORDER_Y);
    lv_height -= (viewheight % totalitemheight);
    
#endif
    
    /* Reinsert the modified dimension attrs into the listview taglist */
    
    D(bug("makelistview: Dimension passed to listview: w=%d, h=%d\n", lv_width, lv_height));

    lv_width_tag->ti_Data  = (IPTR)lv_width;
    lv_height_tag->ti_Data = (IPTR)lv_height;
        
    lvtags[14].ti_Data = (IPTR)stdgadtags;
    
    D(bug("makelistview: Listview class opened\n"));
    lvgad = (struct Gadget *)NewObjectA(GadToolsBase->listviewclass, NULL, lvtags);
    if (lvgad)
    {
	struct TagItem scr_stdtags[] =
	{
	    {GA_Left	, 0L		},
	    {GA_Top	, 0L		},
	    {GA_Width	, 0L		},
	    {GA_Height	, 0L		},
	    {GA_Next	, (IPTR)NULL	},
	    {GA_ID	, 0L		},
	    {GA_DrawInfo, (IPTR)NULL	},
	    {GA_Previous, (IPTR)NULL	},
	    {TAG_DONE			}
	};
        
	struct TagItem scr_specialtags[] =
	{
	    /* The listview will initialize the scrollers top, visible & total,
	     ** in its GM_LAYOUT method
	     */
	    {GTSC_Arrows			, scroller_width	},
	    {PGA_Freedom			, LORIENT_VERT		},
	    {GTA_Scroller_ArrowKind		, LISTVIEW_KIND		},
	    {GTA_Scroller_ScrollerKind		, LISTVIEW_KIND		},
	    {TAG_DONE							}
	};

	lvgad->GadgetType |= GTYP_GADTOOLS;
        
	D(bug("makelistview: Listview gadget created: %p\n", lvgad));
	D(bug("makelistview: scroller_width %ld\n",scroller_width));

	/* Create a scroller object to use with the listviev */
	scr_stdtags[0].ti_Data = lvgad->LeftEdge + lvgad->Width  - 1 + SCROLLER_SPACING;
	scr_stdtags[1].ti_Data = lvgad->TopEdge;
	scr_stdtags[2].ti_Data = scroller_width;
	scr_stdtags[3].ti_Data = lvgad->Height;
	scr_stdtags[4].ti_Data = (IPTR)lvgad;
	scr_stdtags[5].ti_Data = lvgad->GadgetID;
	scr_stdtags[6].ti_Data = (IPTR)vi->vi_dri;
	scr_stdtags[7].ti_Data = (IPTR)GetTagData(GA_Previous, 0, stdgadtags);
	
	scrollergad = makescroller(GadToolsBase, scr_stdtags, vi, scr_specialtags);
	if (scrollergad)
	{
	    struct TagItem lvset_tags[] =
	    {
		{GTA_Listview_Scroller, (IPTR)NULL	},
		{TAG_DONE,				}
	    };
	    struct TagItem scrnotify_tags[] =
	    {
		{ICA_TARGET	, (IPTR)NULL	},
		{ICA_MAP	, (IPTR)NULL	},
		{TAG_DONE	 		}
	    };
	    struct Gadget *prop_of_scrollergad;
	    
	    D(bug("makelistview: Scroller gadget created: %p\n", scrollergad));
                
	    /* the scrollergadget is a multigadget gadget: arrowinc->arrowdec->prop */
#warning This relies on scroller gadget to always contain arrow gadgets
#warning If this ever changes the line below must be updated.
	    prop_of_scrollergad = scrollergad->NextGadget->NextGadget;
		
	    scrollergad->Activation &= ~GACT_FOLLOWMOUSE;
		    	    
	    /* Tell the listview about the scroller and the showselgad */
	    DEBUG_CREATELISTVIEW(bug("makelistview: tell listview about scroller\n"));
	    lvset_tags[0].ti_Data = (IPTR)prop_of_scrollergad;
	    SetAttrsA((Object *)lvgad, lvset_tags);
    	    	
	    /* Tell the scroller to notify the listview when its PGA_Top attribute changes */
	    DEBUG_CREATELISTVIEW(bug("makelistview: tell scroller about notification\n"));
	    scrnotify_tags[0].ti_Data = (IPTR)lvgad;
	    scrnotify_tags[1].ti_Data = (IPTR)scroller2lv;
    	    	
	    SetAttrsA((Object *)prop_of_scrollergad, scrnotify_tags);
    	    	
	    ReturnPtr ("makelistview", struct Gadget *, scrollergad);
	}

	DisposeObject(lvgad);
   	    
    } /* if (lvgad created) */
    	
    ReturnPtr ("makelistview", struct Gadget *, NULL);
}

/****************************************************************************************/

struct Gadget *makegeneric(struct GadToolsBase_intern *GadToolsBase,
		      	  struct TagItem stdgadtags[],
		      	  struct VisualInfo *vi,
		      	  struct TextAttr *tattr,
		      	  struct TagItem *taglist)
{
    struct GT_GenericGadget *gad;
    struct Gadget	    *prevgad;
    
    gad = AllocMem(sizeof(struct GT_GenericGadget), MEMF_PUBLIC | MEMF_CLEAR);
    if (gad)
    {
        gad->gad.LeftEdge 	= stdgadtags[TAG_Left].ti_Data;
	gad->gad.TopEdge 	= stdgadtags[TAG_Top].ti_Data;
	gad->gad.Width 		= stdgadtags[TAG_Width].ti_Data;
	gad->gad.Height 	= stdgadtags[TAG_Height].ti_Data;
	gad->gad.Flags 		= GFLG_EXTENDED;
	gad->gad.Activation 	= 0;
	gad->gad.GadgetType 	= GTYP_GADTOOLS;
	gad->gad.GadgetRender 	= NULL;
	gad->gad.SelectRender 	= NULL;
	gad->gad.GadgetText 	= (struct IntuiText *)stdgadtags[TAG_IText].ti_Data;
	gad->gad.MutualExclude 	= 0;
	gad->gad.SpecialInfo 	= NULL;
	gad->gad.GadgetID 	= stdgadtags[TAG_ID].ti_Data;
	gad->gad.UserData 	= (APTR)stdgadtags[TAG_UserData].ti_Data;
	gad->gad.MoreFlags 	= GMORE_BOUNDS | GMORE_GADGETHELP;
	gad->gad.BoundsLeftEdge = stdgadtags[TAG_Left].ti_Data;
	gad->gad.BoundsTopEdge 	= stdgadtags[TAG_Top].ti_Data;
	gad->gad.BoundsWidth 	= stdgadtags[TAG_Width].ti_Data;
	gad->gad.BoundsHeight 	= stdgadtags[TAG_Height].ti_Data;
	gad->magic 		= GENERIC_MAGIC;
	gad->magic2 		= GENERIC_MAGIC2;
	gad->itext 		= (struct IntuiText *)stdgadtags[TAG_IText].ti_Data;
	
	prevgad = (struct Gadget *)stdgadtags[TAG_Previous].ti_Data;
	prevgad->NextGadget = (struct Gadget *)gad;
    }
    
    ReturnPtr ("makegeneric", struct Gadget *, (struct Gadget *)gad);
}

/****************************************************************************************/
