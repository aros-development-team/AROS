#include <exec/libraries.h>
#include <graphics/gfxbase.h>
#include <graphics/modeid.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include "dosboot_intern.h"

struct Screen *NoBootMediaScreen(struct DOSBootBase *DOSBootBase)
{
    struct Screen *scr = NULL;

    GfxBase = (void *)OpenLibrary("graphics.library", 36);
    if (GfxBase) {
	IntuitionBase = (void *)OpenLibrary("intuition.library", 36);
	if (IntuitionBase) {
	    /* The same as in menu.c/initScreen() */
	    ULONG mode = BestModeID(BIDTAG_DesiredWidth, 640, BIDTAG_DesiredHeight, 480,
				    BIDTAG_Depth, 4, TAG_DONE);

	    if (mode != INVALID_ID) {
		scr = OpenScreenTags(NULL, SA_DisplayID, mode, SA_Draggable, FALSE, 
				     SA_Quiet, TRUE, TAG_DONE);
		if (scr) {

		    /* TODO: Display a picture here */
		    SetAPen(&scr->RastPort, 1);
		    Move(&scr->RastPort, 215, 120);
		    Text(&scr->RastPort, "No bootable media found...", 26);

		    return scr;
		}
	    }
	    CloseLibrary((struct Library *)IntuitionBase);
	}
	CloseLibrary((struct Library *)GfxBase);
    }
    return NULL;
}

void CloseBootScreen(struct Screen *scr, struct DOSBootBase *DOSBootBase)
{
    if (scr) {
	CloseScreen(scr);
	CloseLibrary(&IntuitionBase->LibNode);
	CloseLibrary(&GfxBase->LibNode);
    }
}
