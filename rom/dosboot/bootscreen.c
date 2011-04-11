#include <exec/alerts.h>
#include <exec/libraries.h>
#include <graphics/gfxbase.h>
#include <graphics/modeid.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include "dosboot_intern.h"

struct Screen *OpenBootScreen(struct DOSBootBase *DOSBootBase)
{   
    ULONG mode;

    GfxBase = (void *)OpenLibrary("graphics.library", 36);
    IntuitionBase = (void *)OpenLibrary("intuition.library", 36);

    if ((!IntuitionBase) || (!GfxBase))
	/* We failed to open one of system libraries. AROS is in utterly broken state */
	Alert(AT_DeadEnd|AN_BootStrap|AG_OpenLib);

    /* We want the screen to occupy the whole display, so we find best maching
       mode ID and then open a screen with that mode */
    mode = BestModeID(BIDTAG_DesiredWidth, 640, BIDTAG_DesiredHeight, 480,
			    BIDTAG_Depth, 4, TAG_DONE);

    if (mode != INVALID_ID)
    {
	struct Screen *scr = OpenScreenTags(NULL, SA_DisplayID, mode, SA_Draggable, FALSE, 
					    SA_Quiet, TRUE, TAG_DONE);

	if (scr)
	    return scr;
    }
    /* We can't open a screen. Likely there are no display modes in the database at all */
    Alert(AN_SysScrnType);
    return NULL;
}



struct Screen *NoBootMediaScreen(struct DOSBootBase *DOSBootBase)
{
    struct Screen *scr = OpenBootScreen(DOSBootBase);

    if (!anim_Init(scr, DOSBootBase))
    {
    	SetAPen(&scr->RastPort, 1);
    	Move(&scr->RastPort, 215, 120);
    	Text(&scr->RastPort, "No bootable media found...", 26);
    }

    return scr;
}

void CloseBootScreen(struct Screen *scr, struct DOSBootBase *DOSBootBase)
{
    anim_Stop(DOSBootBase);
    CloseScreen(scr);

    CloseLibrary(&IntuitionBase->LibNode);
    CloseLibrary(&GfxBase->LibNode);
}
