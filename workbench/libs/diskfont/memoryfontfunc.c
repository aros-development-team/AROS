/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Hook for getting font descriptions from memory font list in mem
    Lang: English.
*/

#include <string.h>

#include <proto/graphics.h>
#include <proto/dos.h>
#include <graphics/text.h>
#include <graphics/gfxbase.h>
#include "diskfont_intern.h"

#include <aros/debug.h>

/****************************************************************************************/

/* Userdata needed by the MemoryFontHook */
struct MFData
{
    /* Pointer to the current font in the memory font list */
    struct TextFont *CurrentFont;
    struct TTextAttr currattr;
};

/****************************************************************************************/

/*******************/
/* MF_IteratorInit */
/*******************/

/****************************************************************************************/

APTR MF_IteratorInit(struct DiskfontBase_intern *DiskfontBase)
{
    struct MFData *mfdata;

    mfdata = AllocVec(sizeof(struct MFData), MEMF_ANY|MEMF_CLEAR);
    if (mfdata == NULL)
	return NULL;
    
    /* To prevent race conditions */
    Forbid();
					
    /* Get the first font */
    mfdata->CurrentFont = (struct TextFont*)GetHead(&GfxBase->TextFonts);

    return (APTR)mfdata;
}

/****************************************************************************************/

/**********************/
/* MF_IteratorGetNext */
/**********************/

/****************************************************************************************/

struct TTextAttr *MF_IteratorGetNext(APTR iterator, struct DiskfontBase_intern *DiskfontBase)
{
    struct MFData *mfdata = (struct MFData *)iterator;
    struct TextFont *currfont;
    
    if (mfdata==NULL || mfdata->CurrentFont==NULL)
	return NULL;
    
    currfont = mfdata->CurrentFont;
    
    mfdata->currattr.tta_Tags = NULL;
    mfdata->currattr.tta_Name = currfont->tf_Message.mn_Node.ln_Name;
    mfdata->currattr.tta_YSize = currfont->tf_YSize;
    mfdata->currattr.tta_Style = currfont->tf_Style;
    mfdata->currattr.tta_Flags = currfont->tf_Flags;
    
    if (ExtendFont(currfont, 0L))
    {
	mfdata->currattr.tta_Tags = TFE(currfont->tf_Extension)->tfe_Tags;
	if (mfdata->currattr.tta_Tags) mfdata->currattr.tta_Style |= FSF_TAGGED;
    }

    /* Go to next font */
    mfdata->CurrentFont = (struct TextFont *)GetSucc(mfdata->CurrentFont);
    
    return &mfdata->currattr;
}

/****************************************************************************************/

/*******************/
/* MF_IteratorFree */
/*******************/

/****************************************************************************************/

VOID MF_IteratorFree(APTR iterator, struct DiskfontBase_intern *DiskfontBase)
{
    struct MFData *mfdata = (struct MFData *)iterator;
    
    FreeVec(mfdata);
    Permit();
}

/****************************************************************************************/
