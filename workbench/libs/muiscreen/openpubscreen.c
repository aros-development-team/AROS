/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <libraries/muiscreen.h>
#include <utility/hooks.h>
#include <intuition/screens.h>
#include <proto/intuition.h>
#include <proto/exec.h>
#include <exec/lists.h>
#define DEBUG 1
#include <aros/debug.h>

#include "muiscreen_intern.h"

/*****************************************************************************

    NAME */
#include <proto/muiscreen.h>

        AROS_LH1(char *, MUIS_OpenPubScreen,

/*  SYNOPSIS */
	AROS_LHA(struct MUI_PubScreenDesc *, desc,  A0),

/*  LOCATION */
	struct MUIScreenBase_intern *, MUIScreenBase, 0x42, MUIScreen)

/*  FUNCTION

    INPUTS

    RESULT
    
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/

{
    AROS_LIBFUNC_INIT
    
    struct TextAttr *font = NULL;
    struct Hook *backfillHook = NULL;
    char *ret = NULL;

    D(bug("MUIS_OpenPubScreen(%p)\n", desc));

    // TODO desc->SysDefault
    // TODO desc->AutoClose
    // TODO desc->CloseGadget
    // TODO desc->SystemPens
    // TODO desc->Palette
    
    struct Screen *screen = OpenScreenTags(NULL,
	    SA_Type, (IPTR) PUBLICSCREEN,
	    SA_PubName, desc->Name,
	    SA_Title, desc->Title,
	    SA_Font, font,
	    (backfillHook ? SA_BackFill : TAG_IGNORE), backfillHook,
	    SA_DisplayID, (IPTR) desc->DisplayID,
	    SA_Width, (IPTR) desc->DisplayWidth,
	    SA_Height, (IPTR) desc->DisplayHeight,
	    SA_Depth, (IPTR) desc->DisplayDepth,
	    SA_Overscan, (IPTR) desc->OverscanType,
	    SA_AutoScroll, (IPTR) desc->AutoScroll,
	    SA_Draggable, (IPTR) !desc->NoDrag,
	    SA_Exclusive, (IPTR) desc->Exclusive,
	    SA_Interleaved, (IPTR) desc->Interleaved,
	    SA_Behind, (IPTR) desc->Behind,
            TAG_DONE);
    
    if(screen)
    {
	ret = desc->Name;
	struct Node *node;
	ForeachNode(&MUIScreenBase->clients, node)
	{
	    struct MUIS_InfoClient *client = (struct MUIS_InfoClient*) node;
	    Signal(client->task, client->sigbit);
	}
    }
    
    return ret;

    AROS_LIBFUNC_EXIT
}
