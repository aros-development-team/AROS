/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "global.h"

#include <proto/iffparse.h>
#include <proto/locale.h>

#include <libraries/iffparse.h>		// IFFHandle structure

#include "stdlib.h"			// exit()

UBYTE version[] = "$VER: Font 0.14 (14.1.2002)";

struct libInfo
{
 APTR		lT_Library;
 STRPTR		lT_Name;
 ULONG		lT_Version;
};

struct Catalog *catalogPtr;
extern struct RDArgs *readArgs;

extern struct FontPrefs *fontPrefs[3];	// init.c

STRPTR getCatalog(struct Catalog *catalogPtr, ULONG id)
{
    STRPTR string;
    
    if(catalogPtr)
	string = GetCatalogStr(catalogPtr, id, CatCompArray[id].cca_Str);
    else
	string = CatCompArray[id].cca_Str;

    return(string);
}

STRPTR formatErrorMsg(STRPTR message)
{
    static UBYTE bufferMem[128];	// We better not override the current space
    
    sprintf(bufferMem, getCatalog(catalogPtr, MSG_CANT_ALLOCATE_MEM), message);

    return(bufferMem);
}

void displayError(STRPTR errorMsg)
{
    printf("%s\n", errorMsg);
}

void quitApp(STRPTR errorMsg, UBYTE errorCode)
{
    UBYTE a;
    extern struct libInfo libTable;	// init.c
    extern struct IFFHandle *iffHandle;	// handleiff.c
    struct libInfo *tmpLibInfo = &libTable;
    
    extern struct RDArgs *readArgs;
    if(errorMsg)
	displayError(errorMsg);

    if(LocaleBase)
    {
	CloseCatalog(catalogPtr); // Passing NULL is valid!
	CloseLibrary((struct Library *)LocaleBase);
    }

    if(appGUIData)
    {
	if(appGUIData->agd_VisualInfo)
	    FreeVisualInfo(appGUIData->agd_VisualInfo);
	    
	if(appGUIData->agd_Screen)
	{
	    // We don't have to check for agd_DrawInfo - passing NULL is valid!
	    FreeScreenDrawInfo(appGUIData->agd_Screen, appGUIData->agd_DrawInfo);
		
	    //UnlockPubScreen(NULL, appGUIData->agd_Screen); // CloseWindow should do this for us (petah)
	}

	if(appGUIData->agd_GadgetList)
	    FreeGadgets(appGUIData->agd_GadgetList);

	if(appGUIData->agd_Menu)
	{  
	    ClearMenuStrip(appGUIData->agd_Window); // Really necessary?
	    FreeMenus(appGUIData->agd_Menu);
	}

	freeRegisterTabs();

	if(appGUIData->agd_Window)
	    CloseWindow(appGUIData->agd_Window);

	FreeMem(appGUIData, sizeof(struct AppGUIData));
    }

    // We don't have to check for a TRUE value. However, there is no reason
    // why not to - it might turn out useful in the future.
    if(readArgs)
	FreeArgs(readArgs);

    if(iffHandle)
	FreeIFF(iffHandle);
    else
	kprintf("No iffHandle?!\n");

    for(a = 0; a <= 2; a++)
	if(fontPrefs[a])
	    FreeMem(fontPrefs[a], sizeof(struct FontPrefs));

    while(tmpLibInfo->lT_Name)
    {
	if((*(struct Library **)tmpLibInfo->lT_Library))
	{
	    CloseLibrary((*(struct Library **)tmpLibInfo->lT_Library));
	    kprintf("Closed %s\n", tmpLibInfo->lT_Name);
	}

	tmpLibInfo++;
    }

    exit(errorCode);
}

int main(void)
{
    extern struct AppGUIData *appGUIData; // init.c

    if(!(LocaleBase = (struct LocaleBase *)OpenLibrary("locale.library", 0L)))
	printf("Warning: Can't open locale.library!"); // Non runtime critical error

    // NULL is not runtime critical but should be dealt with in a smarter fashion!
    // The default language (= built-in) language is english and doesn't need to be set (Introduced 0.13)
    if(!(catalogPtr = OpenCatalog(NULL, "Sys/fontprefs.catalog", OC_Version, REQ_CAT_VERSION, TAG_DONE)))
	PrintFault(IoErr(), NULL); // If invalid catalog --> "unknown error"?

    if(!(initLibs()))
	quitApp(NULL, RETURN_FAIL);

    if(!(initPrefMem()))
	quitApp(formatErrorMsg("fontPrefs"), RETURN_FAIL);

    printf("Welcome to Font Preferences for AROS!\nThis is alpha class software: read CAVEATS before you continue!\n");

    // Check for shell (Desktop?) arguments
    switch(processArguments())
    {
	case APP_STOP:
	    quitApp(NULL, RETURN_OK);
	break;

	case APP_FAIL:
	    quitApp(NULL, RETURN_FAIL);
	break;
    }

    if((appGUIData = initAppGUIDataMem(catalogPtr)))
    {
	if(createRegisterTabs())
	{

	    if((appGUIData = createGadgets()))
	    {
		// Open the application window and start the main I/O event loop!

		if((appGUIData = openAppWindow()))
		    quitApp(NULL, inputLoop());
		    
		else
		    quitApp(getCatalog(catalogPtr, MSG_CANT_CREATE_WIN), RETURN_FAIL); // Don't call quitApp(), change this!
	    }
	    else
		quitApp(getCatalog(catalogPtr, MSG_CANT_CREATE_GADGET), RETURN_FAIL);
	}
	else
	    quitApp(formatErrorMsg("Registertabs"), RETURN_FAIL);
    }
    else
	quitApp(NULL, RETURN_FAIL);

    quitApp(NULL, RETURN_OK);
    
    exit(0); // Suppresses GCC warning
}
