/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "global.h"

#include <graphics/gfxbase.h>
#include <intuition/intuitionbase.h>

#include <string.h>	// strcpy()

struct AppGUIData *appGUIData;
struct FontPrefs *fontPrefs[3];
extern IPTR argArray[NUM_ARGS];

void initDefaultPrefs(struct FontPrefs **fontPrefsPtr)
{
    UBYTE a;

    for(a = 0; a <= 2; a++)
    {
	fontPrefs[a]->fp_Type = a;	/* Is this 0, 1, 2 or 1, 2, 3? Look it up! */
	fontPrefs[a]->fp_FrontPen = 0;	/* Is this (really) default? Look it up! */
	fontPrefs[a]->fp_BackPen = 0;	/* Is this (really) default? Look it up! */
	fontPrefs[a]->fp_DrawMode = 0;	/* Is this (really) default? Look it up! */

	fontPrefs[a]->fp_TextAttr.ta_YSize = 8; /* Is this (really) default? Look it up! */
	fontPrefs[a]->fp_TextAttr.ta_Style = FS_NORMAL;
 	fontPrefs[a]->fp_TextAttr.ta_Flags = FPB_DISKFONT; /* Is this (really) default? Look it up! */

	strcpy(fontPrefs[a]->fp_Name, "topaz.font"); /* Is this (really) default? Check it up! */
	fontPrefs[a]->fp_TextAttr.ta_Name = fontPrefs[a]->fp_Name;
    }
}

BOOL initPrefMem(void)
{
    UBYTE a;

    for(a = 0; a <= 2; a++)
	if(!(fontPrefs[a] = AllocMem(sizeof(struct FontPrefs), MEMF_ANY | MEMF_CLEAR)))
	    return(FALSE); /* Some structures may have been allocated */

    initDefaultPrefs(fontPrefs);
 
    return(TRUE);
}

struct AppGUIData *initAppGUIDataMem(struct Catalog *catalogPtr)
{
    if((appGUIData = AllocMem(sizeof(struct AppGUIData), (MEMF_ANY | MEMF_CLEAR))))
    {
	if((appGUIData->agd_Screen = LockPubScreen((UBYTE *)argArray[ARG_PUBSCREEN])))
	{
	    if((appGUIData->agd_VisualInfo = GetVisualInfo(appGUIData->agd_Screen, NULL)))
	    {
		appGUIData->addGUIItems[0] = addGadgets;
		appGUIData->addGUIItems[1] = addPreview;
		appGUIData->removeGUIItems[0] = removeGadgets;
		appGUIData->removeGUIItems[1] = removePreview;
		
		appGUIData->agd_DrawInfo = GetScreenDrawInfo(appGUIData->agd_Screen); // Can't fail!
	    }
	    else
	    {
		displayError(getCatalog(catalogPtr, MSG_CANT_GET_VI));
		return(FALSE);
	    }
	}
	else
	{
	    displayError(getCatalog(catalogPtr, MSG_CANT_LOCK_SCR));
	    return(FALSE);
	}
    }
    else
    {
	displayError(formatErrorMsg("appGUIData"));
	return(FALSE);
    }
	
    return(appGUIData);
}

