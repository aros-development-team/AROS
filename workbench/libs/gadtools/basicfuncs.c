/*
    (C) 1997 AROS - The Amiga Replacement OS
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
#include <proto/utility.h>
#include <utility/tagitem.h>
#include <libraries/gadtools.h>
#include <aros/debug.h>
#include "gadtools_intern.h"



void freeitext(struct GadToolsBase_intern *GadToolsBase,
               struct IntuiText *itext)
{
    if (!itext)
        return;
    FreeVec(itext->ITextFont->ta_Name);
    FreeVec(itext->ITextFont);
    FreeVec(itext);
}

struct IntuiText *makeitext(struct GadToolsBase_intern *GadToolsBase,
			    struct NewGadget *ng)
{
    struct IntuiText *it;
    struct DrawInfo *dri = ((struct VisualInfo *)ng->ng_VisualInfo)->vi_dri;

    it = (struct IntuiText *)AllocVec(sizeof(struct IntuiText), MEMF_ANY);
    if (!it)
	return NULL;

    if (!(ng->ng_Flags & NG_HIGHLABEL))
	it->FrontPen = dri->dri_Pens[TEXTPEN];
    else
	it->FrontPen = dri->dri_Pens[HIGHLIGHTTEXTPEN];
    it->BackPen   = dri->dri_Pens[BACKGROUNDPEN];
    it->DrawMode  = JAM1;
    it->LeftEdge  = 0;
    it->TopEdge   = 0;
    it->ITextFont = AllocVec(sizeof(struct TextAttr), MEMF_ANY);
    if (!it->ITextFont)
    {
        FreeVec(it);
        return NULL;
    }
    if (ng->ng_TextAttr)
    {
        int len = strlen(ng->ng_TextAttr->ta_Name) + 1;

        it->ITextFont->ta_Name = AllocVec(len, MEMF_ANY);
        if (!it->ITextFont->ta_Name)
        {
            FreeVec(it->ITextFont);
            FreeVec(it);
            return NULL;
        }
        CopyMem(ng->ng_TextAttr->ta_Name, it->ITextFont->ta_Name, len);
        it->ITextFont->ta_YSize = ng->ng_TextAttr->ta_YSize;
        it->ITextFont->ta_Style = ng->ng_TextAttr->ta_Style;
        it->ITextFont->ta_Flags = ng->ng_TextAttr->ta_Flags;
    } else
    {
        int len = strlen(dri->dri_Font->tf_Message.mn_Node.ln_Name) + 1;

        it->ITextFont->ta_Name = AllocVec(len, MEMF_ANY);
        if (!it->ITextFont->ta_Name)
        {
            FreeVec(it->ITextFont);
            FreeVec(it);
            return NULL;
        }
        CopyMem(dri->dri_Font->tf_Message.mn_Node.ln_Name, it->ITextFont->ta_Name, len);
        it->ITextFont->ta_YSize = dri->dri_Font->tf_YSize;
        it->ITextFont->ta_Style = dri->dri_Font->tf_Style;
        it->ITextFont->ta_Flags = dri->dri_Font->tf_Flags;
    }
    it->IText     = ng->ng_GadgetText;
    it->NextText  = NULL;

    return it;
}

