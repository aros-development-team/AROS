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

// Our local library bases. They don't need to be set to NULL. David Gerber has pointed out that
// ANSI-C requires automated NULL initialization when nothing else is specified. However, if the
// library pointers are declared as "extern" it seems that some AROS link module will initialize
// them automatically which is a bad thing as our Open/CloseLibrary() pair will override whatever
// data already present in the library pointers.

struct IntuitionBase *IntuitionBase = NULL;
struct GfxBase *GfxBase = NULL;
struct Library *GadToolsBase = NULL;
struct LocaleBase *LocaleBase = NULL;
struct Library *IFFParseBase = NULL;
struct Library *AslBase = NULL;
struct Library *DiskFontBase = NULL;

struct libInfo
{
 APTR		lT_Library;
 STRPTR		lT_Name;
 ULONG		lT_Version;
};

// For now, library version 37 are required. It should be sufficient, but it hasn't been
// more thoroughly checked!
struct libInfo libTable[] =
{
 { &IntuitionBase,	"intuition.library",	37L},
 { &GadToolsBase,	"gadtools.library",	37L},
 { &GfxBase,		"graphics.library",	37L},
 { &IFFParseBase,	"iffparse.library",	37L},
 { &AslBase,		"asl.library",		37L},
 { &DiskFontBase,	"diskfont.library",	37L},
 { NULL }
};

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

BOOL initLibs(void)
{
    struct libInfo *tmpLibInfo = libTable;
    UBYTE tmpString[128]; // What if library name plus error message exceeds 128 bytes?

    kprintf("Opening libs...\n");

    while(tmpLibInfo->lT_Library)
    {
	if(!((*(struct Library **)tmpLibInfo->lT_Library = OpenLibrary(tmpLibInfo->lT_Name, tmpLibInfo->lT_Version))))
	{
	    sprintf(tmpString, getCatalog(catalogPtr, MSG_CANT_OPEN_LIB), tmpLibInfo->lT_Name,tmpLibInfo->lT_Version);
	    return(FALSE);
	}

    else
	kprintf("Opened %s!\n", tmpLibInfo->lT_Name);

	tmpLibInfo++;
    }

    return(TRUE);
}
