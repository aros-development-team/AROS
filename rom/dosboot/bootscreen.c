#include <aros/debug.h>
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
    UWORD height, depth;
    ULONG mode;

    GfxBase = (void *)TaggedOpenLibrary(TAGGEDOPEN_GRAPHICS);
    IntuitionBase = (void *)TaggedOpenLibrary(TAGGEDOPEN_INTUITION);

    if ((!IntuitionBase) || (!GfxBase))
	/* We failed to open one of system libraries. AROS is in utterly broken state */
	Alert(AT_DeadEnd|AN_BootStrap|AG_OpenLib);

    height = 480;
    mode = BestModeID(BIDTAG_DesiredWidth, 640, BIDTAG_DesiredHeight, height,
			    BIDTAG_Depth, 8, TAG_DONE);
    if (mode != INVALID_ID) {
	/* if we got depth=8 mode, we must have fast enough hardware for 4 planes too,
	 * either it is non-Amiga(tm) hardware or AGA chipset */
	depth = 4;
    } else {
	/* we probably have OCS or ECS chipset, select 2 planes because 4 planes OCS/ECS hires is very slow */
	depth = 2;
    }
    /* set PAL or NTSC default height if we are running on Amiga(tm) hardware
     * we are using interlaced screen height because boot screen assumes 1:1 pixels */
    if (GfxBase->DisplayFlags & NTSC)
        height = 200 * 2;
    else if (GfxBase->DisplayFlags & PAL)
        height = 256 * 2;

    /* We want the screen to occupy the whole display, so we find best maching
       mode ID and then open a screen with that mode */
    mode = BestModeID(BIDTAG_DesiredWidth, 640, BIDTAG_DesiredHeight, height,
			    BIDTAG_Depth, depth, TAG_DONE);
    if (mode != INVALID_ID)
    {
	struct Screen *scr = OpenScreenTags(NULL, SA_DisplayID, mode, SA_Draggable, FALSE, 
					    SA_Quiet, TRUE, SA_Depth, depth, TAG_DONE);

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
    CloseScreen(scr);

    CloseLibrary(&IntuitionBase->LibNode);
    CloseLibrary(&GfxBase->LibNode);
}
