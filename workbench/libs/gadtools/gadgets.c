/*
   (C) 1997 AROS - The Amiga Replacement OS
   $Id$

   Desc: GadTools gadget creation functions
   Lang: English
*/
#include <stdio.h>
#include <proto/exec.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/screens.h>
#include <intuition/icclass.h>
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <libraries/gadtools.h>
#include <gadgets/aroscheckbox.h>
#include <gadgets/aroscycle.h>
#include <gadgets/arosmx.h>
#include <gadgets/arospalette.h>
#include "gadtools_intern.h"

#define SDEBUG 1
#define DEBUG 1
#include <aros/debug.h>

/******************
**  makbutton()  **
******************/
struct Gadget *makebutton(struct GadToolsBase_intern *GadToolsBase,
			  struct TagItem stdgadtags[],
			  struct VisualInfo *vi,
			  struct TagItem *taglist)
{
    struct Gadget *obj;
    Class *cl;
    struct TagItem tags[] =
    {
	{GA_Disabled, FALSE},
	{GA_Immediate, FALSE},
	{GA_RelVerify, TRUE},
	{TAG_MORE, (IPTR) NULL}
    };

    cl = makebuttonclass(GadToolsBase);
    if (!cl)
	return NULL;

    tags[0].ti_Data = GetTagData(GA_Disabled, FALSE, taglist);
    tags[1].ti_Data = GetTagData(GA_Immediate, FALSE, taglist);
    tags[3].ti_Data = (IPTR) stdgadtags;

    obj = (struct Gadget *) NewObjectA(cl, NULL, tags);

    return obj;
}

/*********************
**  makecheckbox()  **
*********************/
struct Gadget *makecheckbox(struct GadToolsBase_intern *GadToolsBase,
			    struct TagItem stdgadtags[],
			    struct VisualInfo *vi,
			    struct TagItem *taglist)
{
    struct Gadget *obj;
    struct TagItem tags[] =
    {
	{GA_Disabled, FALSE},
	{GTCB_Checked, FALSE},
	{TAG_MORE, (IPTR) NULL}
    };

    if (!GadToolsBase->aroscbbase)
        GadToolsBase->aroscbbase = OpenLibrary("SYS:Classes/Gadgets/aroscheckbox.gadget", 0);
    if (!GadToolsBase->aroscbbase)
        return NULL;

    tags[0].ti_Data = GetTagData(GA_Disabled, FALSE, taglist);
    tags[1].ti_Data = GetTagData(GTCB_Checked, FALSE, taglist);
    tags[2].ti_Data = (IPTR) stdgadtags;

    if (!GetTagData(GTCB_Scaled, FALSE, taglist)) {
	stdgadtags[TAG_Width].ti_Data = CHECKBOX_WIDTH;
	stdgadtags[TAG_Height].ti_Data = CHECKBOX_HEIGHT;
    }
    obj = (struct Gadget *) NewObjectA(NULL, AROSCHECKBOXCLASS, tags);

    return obj;
}

/******************
**  makecycle()  **
******************/
struct Gadget *makecycle(struct GadToolsBase_intern *GadToolsBase,
                         struct TagItem stdgadtags[],
                         struct VisualInfo *vi,
                         struct TagItem *taglist)
{
    struct Gadget *obj;
    struct TagItem tags[] =
    {
	{GA_Disabled, FALSE},
	{GTCY_Labels, FALSE},
        {GTCY_Active, 0},
	{TAG_MORE, (IPTR) NULL}
    };

    if (!GadToolsBase->aroscybase)
        GadToolsBase->aroscybase = OpenLibrary("SYS:Classes/Gadgets/aroscycle.gadget", 0);
    if (!GadToolsBase->aroscybase)
        return NULL;

    tags[0].ti_Data = GetTagData(GA_Disabled, FALSE, taglist);
    tags[1].ti_Data = GetTagData(GTCY_Labels, FALSE, taglist);
    tags[2].ti_Data = GetTagData(GTCY_Active, 0, taglist);
    tags[3].ti_Data = (IPTR) stdgadtags;

    obj = (struct Gadget *) NewObjectA(NULL, AROSCYCLECLASS, tags);

    return obj;
}

/***************
**  makemx()  **
***************/
struct Gadget *makemx(struct GadToolsBase_intern *GadToolsBase,
		      struct TagItem stdgadtags[],
		      struct VisualInfo *vi,
		      struct TagItem *taglist)
{
    struct Gadget *gad;
    int labels = 0;
    STRPTR *labellist;
    struct TagItem *tag, tags[] =
    {
	{GA_Disabled, FALSE},
	{AROSMX_Labels, (IPTR) NULL},
	{AROSMX_Active, 0},
	{AROSMX_Spacing, 1},
        {AROSMX_TickHeight, MX_HEIGHT},
        {AROSMX_TickLabelPlace, GV_LabelPlace_Right},
	{TAG_MORE, (IPTR) NULL}
    };

    if (!GadToolsBase->arosmxbase)
        GadToolsBase->arosmxbase = OpenLibrary("SYS:Classes/Gadgets/arosmutualexclude.gadget", 0);
    if (!GadToolsBase->arosmxbase)
        return NULL;

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
    case PLACETEXT_LEFT:
        tags[5].ti_Data = GV_LabelPlace_Left;
        break;
    case PLACETEXT_ABOVE:
        tags[5].ti_Data = GV_LabelPlace_Above;
        break;
    case PLACETEXT_BELOW:
        tags[5].ti_Data = GV_LabelPlace_Below;
        break;
    }
    tags[6].ti_Data = (IPTR) stdgadtags;

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

    gad = (struct Gadget *) NewObjectA(NULL, AROSMXCLASS, tags);

    return gad;
}

/*******************
**  makepalette() **
*******************/
struct Gadget *makepalette(struct GadToolsBase_intern *GadToolsBase,
                         struct TagItem stdgadtags[],
                         struct VisualInfo *vi,
                         struct TagItem *taglist)
{
    struct Gadget *obj;

    struct TagItem *tag, tags[] =
    {
    	{GA_RelVerify,		TRUE},
	{GA_Disabled,		FALSE},
	{GTPA_Depth,		1},
        {GTPA_Color,		0},
        {GTPA_ColorOffset,	0},
        {GTPA_IndicatorWidth,	0},
        {GTPA_IndicatorHeight,	0},
        {GTPA_NumColors,	0},
        {GTPA_ColorTable,	0},
	{TAG_MORE, (IPTR) NULL}
    };
    
    /* Could use GetTagData(), but this is faster */
    while ((tag = NextTagItem(&taglist)))
    {
    	IPTR tidata = tag->ti_Data;
    	
    	switch (tag->ti_Tag)
    	{
    	case GA_Disabled:		tags[1].ti_Data = tidata; break;
    	case GTPA_Depth:		tags[2].ti_Data	= tidata; break;
    	case GTPA_Color:		tags[3].ti_Data	= tidata; break;
    	case GTPA_ColorOffset:		tags[4].ti_Data	= tidata; break;
    	case GTPA_IndicatorWidth:	tags[5].ti_Data	= tidata; break;
    	case GTPA_IndicatorHeight:	tags[6].ti_Data	= tidata; break;
    	case GTPA_NumColors:		tags[7].ti_Data	= tidata; break;
    	case GTPA_ColorTable:		tags[8].ti_Data	= tidata; break;
    	    
    	} /* switch() */
    	
    } /* while (iterate taglist) */

    tags[9].ti_Data = (IPTR)stdgadtags;

    if (!GadToolsBase->arospabase)
        GadToolsBase->arospabase = OpenLibrary(AROSPALETTECLASSPATH, 0);
    if (!GadToolsBase->arospabase)
        return NULL;

    obj = (struct Gadget *) NewObjectA(NULL, AROSPALETTECLASS, tags);

    return obj;
}

/*****************
**  maketext()  **
*****************/
struct Gadget *maketext(struct GadToolsBase_intern *GadToolsBase,
                         struct TagItem stdgadtags[],
                         struct VisualInfo *vi,
                         struct TextAttr *tattr,
                         struct TagItem *taglist)
{
    struct Gadget *obj;
    Class *cl;
    
    struct TagItem *tag, tags[] =
    {
    	{GTTX_Text,		(IPTR)"Blah"},
    	{GTTX_CopyText,		FALSE},
    	{GTTX_Clipped,		FALSE},
    	{GTTX_Border,		FALSE},
    	{GTTX_FrontPen,		TEXTPEN},
    	{GTTX_BackPen,		BACKGROUNDPEN},
    	{GTTX_Justification,	GTJ_LEFT},
    	{GTA_Text_Format,	(IPTR)"%s"},
    	{GA_TextAttr,		(IPTR)NULL},
	{TAG_MORE, (IPTR) NULL}
    };

    EnterFunc(bug("maketext(stdadgadtags=%p, vi=%p, tattr=%p, taglist=%p)\n",
    	stdgadtags, vi, tattr, taglist));
    
    /* Could use GetTagData(), but this is faster */
    while ((tag = NextTagItem(&taglist)))
    {
    	IPTR tidata = tag->ti_Data;
    	
    	switch (tag->ti_Tag)
    	{
    	case GTTX_Text:			tags[0].ti_Data = tidata; break;
    	case GTTX_CopyText:		tags[1].ti_Data	= tidata; break;
    	case GTTX_Clipped:		tags[2].ti_Data	= tidata; break;
    	case GTTX_Border:		tags[3].ti_Data	= tidata; break;
    	case GTTX_FrontPen:		tags[4].ti_Data	= tidata; break;
    	case GTTX_BackPen:		tags[5].ti_Data	= tidata; break;
    	case GTTX_Justification:	tags[6].ti_Data	= tidata; break;
    	    
    	}
    	
    } /* while (iterate taglist) */

    /* Be sure not to pass GA_TextAttr, NULL */
    if (tattr)
    	tags[8].ti_Data = (IPTR)tattr;
    else
    	tags[8].ti_Tag = TAG_IGNORE;
    tags[9].ti_Data = (IPTR)stdgadtags;

    cl = maketextclass(GadToolsBase);
    if (!cl)
    	return (NULL);
    obj = (struct Gadget *) NewObjectA(cl, NULL, tags);

    ReturnPtr ("maketext", struct Gadget *, obj);
}

/*******************
**  makenumber()  **
*******************/
struct Gadget *makenumber(struct GadToolsBase_intern *GadToolsBase,
                         struct TagItem stdgadtags[],
                         struct VisualInfo *vi,
                         struct TextAttr *tattr,
                         struct TagItem *taglist)
{
    struct Gadget *obj;
    Class *cl;

    struct TagItem *tag, tags[] =
    {
    	{GTNM_Number,		0},
    	{GTNM_Format,		(IPTR)"%ld"},
    	{GTNM_Clipped,		FALSE},
    	{GTNM_Border,		FALSE},
    	{GTNM_FrontPen,		TEXTPEN},
    	{GTNM_BackPen,		BACKGROUNDPEN},
    	{GTNM_Justification,	GTJ_CENTER},
    	{GTNM_MaxNumberLen,	100},
    	{GA_TextAttr,		(IPTR)NULL},
	{TAG_MORE, (IPTR) NULL}
    };
    
    EnterFunc(bug("makenumber(stdadgadtags=%p, vi=%p, tattr=%p, taglist=%p)\n",
    	stdgadtags, vi, tattr, taglist));
   
    /* Could use GetTagData(), but this is faster */
    while ((tag = NextTagItem(&taglist)))
    {
    	IPTR tidata = tag->ti_Data;
    	
    	switch (tag->ti_Tag)
    	{
    	case GTNM_Number:		tags[0].ti_Data = tidata; break;
    	case GTNM_Format:		tags[1].ti_Data	= tidata; break;
    	case GTNM_Clipped:		tags[2].ti_Data	= tidata; break;
    	case GTNM_Border:		tags[3].ti_Data	= tidata; break;
    	case GTNM_FrontPen:		tags[4].ti_Data	= tidata; break;
    	case GTNM_BackPen:		tags[5].ti_Data	= tidata; break;
    	case GTNM_Justification:	tags[6].ti_Data	= tidata; break;
    	case GTNM_MaxNumberLen:		tags[7].ti_Data	= tidata; break;
    	    
    	}
    	
    } /* while (iterate taglist) */

    /* Be sure not to pass GA_TextAttr, NULL */
    if (tattr)
    	tags[8].ti_Data = (IPTR)tattr;
    else
    	tags[8].ti_Tag = TAG_IGNORE;
    	
    tags[9].ti_Data = (IPTR)stdgadtags;

    cl = maketextclass(GadToolsBase);
    if (!cl)
    	return (NULL);
    obj = (struct Gadget *) NewObjectA(cl, NULL, tags);

    ReturnPtr ("maketest", struct Gadget *, obj);
}

/*******************
**  makeslider()  **
*******************/
struct Gadget *makeslider(struct GadToolsBase_intern *GadToolsBase,
                         struct TagItem stdgadtags[],
                         struct VisualInfo *vi,
                         struct TextAttr *tattr,
                         struct TagItem *taglist)
{

    struct TagItem *tag;
    
    struct TagItem stags[] =
    {
    	{GA_Disabled,	FALSE},
    	{GA_RelVerify,	FALSE},
    	{GA_Immediate,	FALSE},
    	{GTSL_Min,	0},
    	{GTSL_Max,	15},
    	{GTSL_Level,	0},
    	{PGA_Freedom,	LORIENT_HORIZ},
    	{TAG_MORE,	(IPTR)NULL}
    };
    
    struct TagItem ltags[] = 
    {
    	 {GA_Left,	0},
    	 {GA_Top,	0},
    	 {GA_Width,	0},
    	 {GA_Height,	0},
    	 {GA_TextAttr,	(IPTR)NULL},
    	 {GTNM_Format,	(IPTR)NULL},
    	 {GTNM_Justification, GTJ_LEFT},
	 {GTA_Text_DispFunc,	(IPTR)NULL},
	 {GA_Previous,	(IPTR)NULL},
	 {GA_DrawInfo,	(IPTR)NULL},
	 {GTNM_Number,	0},
	 {TAG_DONE,}
    };
    STRPTR lformat = NULL;
    WORD lmaxlen = 0;
    LONG lmaxpixellen = 0L;
    UBYTE lplace = PLACETEXT_LEFT;
    WORD level = 0;
    
    Class *slidercl, *textcl;
    struct Gadget *slidergad, *levelgad;
    
    EnterFunc(bug("makeslider(stdadgadtags=%p, vi=%p, tattr=%p, taglist=%p)\n",
    	stdgadtags, vi, tattr, taglist));

    /* open needed classes */
    slidercl = makesliderclass(GadToolsBase);
    if (!slidercl)
    	ReturnPtr("makeslider", struct Gadget *, NULL);

    textcl = maketextclass(GadToolsBase);
    if (!textcl)
    	ReturnPtr("makeslider", struct Gadget *, NULL);
    	
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
    	case GTSL_Min:		stags[3].ti_Data = tidata; break;
    	case GTSL_Max:		stags[4].ti_Data = tidata; break;
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
	case GTSL_LevelPlace:    lplace	      	  = (UBYTE)tidata;	break;
    	case GTSL_Justification: ltags[6].ti_Data = tidata;	  	break;
    	case GTSL_DispFunc:	 ltags[7].ti_Data = tidata;  		break;

    	} 
    	    
    } /* while (iterate taglist) */
    
    /* Create slider gadget */
    stags[7].ti_Data = (IPTR)stdgadtags;
    slidergad = NewObjectA(slidercl, NULL, stags);
    if (!slidergad)
    	ReturnPtr("makeslider", struct Gadget *, NULL);

    if (lformat || lmaxlen || lmaxpixellen)
    {
	WORD x, y;
	UWORD ysize;
	
	struct TagItem slider2level[] =
	{
	    {GTSL_Level,	GTNM_Number},
	    {TAG_DONE, }
	};
	
	struct TagItem lntags[] =
	{
	     {ICA_TARGET,	(IPTR)NULL},
	     {ICA_MAP,		(IPTR)NULL},
	     {TAG_DONE,}
	};
	    
    	/* Set som defaults */
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
            	x = slidergad->LeftEdge - lmaxpixellen - 4;
            	y = slidergad->TopEdge + (slidergad->Height - ysize) / 2 + 1;
    	    	break;
    	    case PLACETEXT_RIGHT:
            	x = slidergad->LeftEdge + slidergad->Width + 5;
            	y = slidergad->TopEdge  + (slidergad->Height - ysize) / 2 + 1;
    	    	break;
    	    case PLACETEXT_ABOVE:
            	x = slidergad->LeftEdge - (lmaxpixellen - slidergad->Width) / 2;
            	y = slidergad->TopEdge  - ysize - 2;
    	    	break;
    	    case PLACETEXT_BELOW:
            	x = slidergad->LeftEdge - (lmaxpixellen - slidergad->Width) / 2;
            	y = slidergad->TopEdge  + slidergad->Height + 3;
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
    	    
    	levelgad = (struct Gadget *)NewObjectA(textcl, NULL, ltags);
    	if (!levelgad)
    	{
    	    DisposeObject((Object *)slidergad);
    	    ReturnPtr("makeslider", struct Gadget *, NULL);
    	}
    	
    	/* Set up a notification from the slider to the level */
    	lntags[0].ti_Data = (IPTR)levelgad;
    	lntags[1].ti_Data = (IPTR)slider2level;
    	SetAttrsA((Object *)slidergad, lntags);
    	    
    } /* if (slider should have a level attached) */
    
    ReturnPtr("makeslider", struct Gadget *, slidergad);

}                         
