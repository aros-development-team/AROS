/*
    (C) 1997-2000 AROS - The Amiga Research OS
    $Id$

    Desc: Basic help functions needed by gadtools.library.
    Lang: English.
*/
#include <string.h>
#include <proto/exec.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <proto/graphics.h>
#include <graphics/rastport.h>
#include <graphics/rpattr.h>
#include <graphics/text.h>
#include <graphics/gfxmacros.h>
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <libraries/gadtools.h>
#include <aros/debug.h>
#include <intuition/gadgetclass.h>
#include "gadtools_intern.h"

/**********************************************************************************************/

#define HIGH_COLOR 1  /* instead of underscore use different color to highlight key */

#define EG(x) ((struct ExtGadget *)(x))

struct GTIText
{
    struct IntuiText it;
    struct IntuiText it2;
    struct TextAttr  ta;
    struct TextAttr  ta2;
    UBYTE	     text[0];
};

/**********************************************************************************************/

void freeitext(struct GadToolsBase_intern *GadToolsBase,
               struct IntuiText *itext)
{
    if (itext) FreeVec(itext);
}

/**********************************************************************************************/

/* Create a struct IntuiText accordings to a struct NewGadget */
struct IntuiText *makeitext(struct GadToolsBase_intern *GadToolsBase,
			    struct NewGadget *ng,
			    struct TagItem *taglist)
{
    struct GTIText 	*gtit;
    struct DrawInfo 	*dri = ((struct VisualInfo *)ng->ng_VisualInfo)->vi_dri;
    struct TextFont	*font = NULL;
    struct RastPort	temprp;
    STRPTR		underscorepos;
    STRPTR		fontname;
    UWORD		fontysize;
    UBYTE		fontstyle;
    UBYTE		fontflags;
    WORD		gadgettextlen;
    WORD		fontnamelen;
    WORD		underscorelen;
    WORD		alloclen;
    BOOL		fontopened = FALSE;
    UBYTE		underscore = 1; /* default for GT_Underscore = a char which hopefully a normal
    				           string never contains */

    underscore = (UBYTE)GetTagData(GT_Underscore, underscore, taglist);    
    underscorepos = strchr(ng->ng_GadgetText, underscore);
    gadgettextlen = strlen(ng->ng_GadgetText);
    
    if (ng->ng_TextAttr)
    {
        font = OpenFont(ng->ng_TextAttr);
	if (!font) return NULL;
	
	fontopened = TRUE;
	
        fontname  = ng->ng_TextAttr->ta_Name;
	fontysize = ng->ng_TextAttr->ta_YSize;
	fontstyle = ng->ng_TextAttr->ta_Style;
	fontflags = ng->ng_TextAttr->ta_Flags;
    } else {
        font = dri->dri_Font;
	
        fontname  = dri->dri_Font->tf_Message.mn_Node.ln_Name;
	fontysize = dri->dri_Font->tf_YSize;
	fontstyle = dri->dri_Font->tf_Style;
	fontflags = dri->dri_Font->tf_Flags;
    }
    
    if (!fontname) return NULL;
    
    fontnamelen = strlen(fontname);
    
    alloclen = sizeof(struct GTIText) + fontnamelen + 1 + gadgettextlen + 1 + 2; /* 2 for safety */
    		   
    gtit = (struct GTIText *)AllocVec(alloclen, MEMF_PUBLIC | MEMF_CLEAR);
    if (!gtit)
    {
        if (fontopened) CloseFont(font);
	return NULL;
    }

    CopyMem(fontname, gtit->text, fontnamelen);

    gtit->ta.ta_Name  = gtit->text;
    gtit->ta.ta_YSize = fontysize;
    gtit->ta.ta_Style = fontstyle;
    gtit->ta.ta_Flags = fontflags;
    
    gtit->it.FrontPen  = dri->dri_Pens[(ng->ng_Flags & NG_HIGHLABEL) ? HIGHLIGHTTEXTPEN : TEXTPEN];
    gtit->it.BackPen   = dri->dri_Pens[BACKGROUNDPEN];
    gtit->it.DrawMode  = JAM1;
    gtit->it.LeftEdge  = 0;
    gtit->it.TopEdge   = 0;
    gtit->it.ITextFont = &gtit->ta;
    
    if (!underscorepos)
    {
        gtit->it.IText = gtit->text + fontnamelen + 1;	
        gtit->it.NextText = NULL;
	
	if (gadgettextlen) CopyMem(ng->ng_GadgetText, gtit->it.IText, gadgettextlen);
    }
    else
    {	
        gadgettextlen--;
	underscorelen = underscorepos - ng->ng_GadgetText;
	
	gtit->it.IText = gtit->text + fontnamelen + 1;	
	if (underscorelen)
	{
	    CopyMem(ng->ng_GadgetText, gtit->it.IText, underscorelen);
	}
	if (gadgettextlen - underscorelen)
	{
	    CopyMem(underscorepos + 1, gtit->it.IText + underscorelen, gadgettextlen - underscorelen);
	}
		
	gtit->it.NextText = &gtit->it2;
	
    	gtit->it2 = gtit->it;
	gtit->it2.ITextFont = &gtit->ta2;
	gtit->it2.IText = gtit->it.IText + gadgettextlen + 1;
	gtit->it2.NextText = NULL;

	gtit->ta2 = gtit->ta;

#if HIGH_COLOR
	gtit->it2.FrontPen = dri->dri_Pens[(ng->ng_Flags & NG_HIGHLABEL) ? TEXTPEN : HIGHLIGHTTEXTPEN];
#else
	gtit->ta2.ta_Style |= FSF_UNDERLINED;
#endif
	

	if (!underscorelen)
	{
	    gtit->it2.LeftEdge = 0;
	}
	else
	{
	    InitRastPort(&temprp);
	    SetFont(&temprp, font);
	
	    gtit->it2.LeftEdge = TextLength(&temprp, ng->ng_GadgetText, underscorelen);
	    	
	    DeinitRastPort(&temprp);
	}
	
	gtit->it2.IText[0] = underscorepos[1];	
    }
    
    if (fontopened) CloseFont(font);
    
    return &gtit->it;
}

/**********************************************************************************************/

struct TextFont *preparefont(struct GadToolsBase_intern *GadToolsBase,
			     struct RastPort *rport, struct IntuiText *itext,
			     struct TextFont **oldfont)
{
    struct TextFont *font;

    if (itext->ITextFont)
    {
	*oldfont = rport->Font;
	font = OpenFont(itext->ITextFont);
	if (font)
	{
	    SetFont(rport, font);
	    SetSoftStyle(rport, itext->ITextFont->ta_Style, 0xffffffff);
	} else
	    font = rport->Font;
    } else
    {
	*oldfont = NULL;
	font = rport->Font;
    }
    SetABPenDrMd(rport, itext->FrontPen, itext->BackPen, itext->DrawMode);

    return font;
}

/**********************************************************************************************/

void closefont(struct GadToolsBase_intern *GadToolsBase,
	       struct RastPort *rport,
	       struct TextFont *font, struct TextFont *oldfont)
{
    if (oldfont)
    {
	SetFont(rport, oldfont);
	CloseFont(font);
    }
}

/**********************************************************************************************/

BOOL renderlabel(struct GadToolsBase_intern *GadToolsBase,
		 struct Gadget *gad, struct RastPort *rport, LONG labelplace)
{
    struct TextFont 	*font = NULL, *oldfont;
    struct TextExtent 	te;
    STRPTR 		text;
    int 		len = 0, x, y;
    UWORD 		width, height;
    WORD 		gadleft, gadtop, gadwidth, gadheight;
    
    if (EG(gad)->MoreFlags & GMORE_BOUNDS)
    {
    	gadleft   = EG(gad)->BoundsLeftEdge;
	gadtop    = EG(gad)->BoundsTopEdge;
	gadwidth  = EG(gad)->BoundsWidth;
	gadheight = EG(gad)->BoundsHeight;
    } else {
    	gadleft   = gad->LeftEdge;
	gadtop    = gad->TopEdge;
	gadwidth  = gad->Width;
	gadheight = gad->Height;
    }
    
    if (gad->GadgetText)
    {
        /* Calculate offsets. */
        if ((gad->Flags & GFLG_LABELSTRING))
            text = (STRPTR)gad->GadgetText;
        else if ((gad->Flags & GFLG_LABELIMAGE))
            text = NULL;
        else
        {
            /* GFLG_LABELITEXT */
            text = gad->GadgetText->IText;
            font = preparefont(GadToolsBase,
                               rport, gad->GadgetText, &oldfont);
            if (!font)
                return FALSE;
        }

        if (text)
        {
            len = strlen(text);
            TextExtent(rport, text, len, &te);
            width  = te.te_Width;
            height = te.te_Height;
        } else
        {
            width  = ((struct Image *)gad->GadgetText)->Width;
            height = ((struct Image *)gad->GadgetText)->Height;
        }

        if (labelplace == GV_LabelPlace_Right)
        {
            x = gadleft + gadwidth + 5;
            y = gadtop + (gadheight - height) / 2 + 1;
        } else if (labelplace == GV_LabelPlace_Above)
        {
            x = gadleft + (gadwidth - width) / 2;
            y = gadtop - height - 2;
        } else if (labelplace == GV_LabelPlace_Below)
        {
            x = gadleft + (gadwidth - width) / 2;
            y = gadtop + gadheight + 3;
        } else if (labelplace == GV_LabelPlace_In)
        {
            x = gadleft + (gadwidth - width) / 2;
            y = gadtop + (gadheight - height) / 2 + 1;
        } else /* GV_LabelPlace_Left */
        {
            x = gadleft - width - 4;
            y = gadtop + (gadheight - height) / 2 + 1;
        }

        if (gad->Flags & GFLG_LABELSTRING)
        {
            SetABPenDrMd(rport, 1, 0, JAM1);
            Move(rport, x, y);
            Text(rport, text, len);
        } else if (gad->Flags & GFLG_LABELIMAGE)
            DrawImage(rport, (struct Image *)gad->GadgetText, x, y);
        else
        { 
            PrintIText(rport, gad->GadgetText, x, y);
            closefont(GadToolsBase, rport, font, oldfont);
        }
    }
    return TRUE;
}

/**********************************************************************************************/

void DoDisabledPattern(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2,
		       WORD pen, struct GadToolsBase_intern *GadToolsBase)
{
    UWORD pattern[] = { 0x8888, 0x2222 };

    SetDrMd( rp, JAM1 );
    SetAPen( rp, pen );
    SetAfPt( rp, pattern, 1);

    /* render disable pattern */
    RectFill(rp, x1, y1, x2, y2);
    
    SetAfPt (rp, NULL, 0);
    
}

/**********************************************************************************************/
