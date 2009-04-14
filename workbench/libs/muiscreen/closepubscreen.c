/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <libraries/muiscreen.h>
#include <proto/intuition.h>
#include <intuition/screens.h>
#include <proto/intuition.h>
#include <proto/exec.h>
#include <exec/lists.h>
#include <string.h>
#define DEBUG 1
#include <aros/debug.h>

#include "muiscreen_intern.h"

/*****************************************************************************

    NAME */
#include <proto/muiscreen.h>

        AROS_LH1(BOOL, MUIS_ClosePubScreen,

/*  SYNOPSIS */
	AROS_LHA(char *, name,  A0),

/*  LOCATION */
	struct MUIScreenBase_intern *, MUIScreenBase, 0x48, MUIScreen)

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

    struct List *pubscrlist;
    struct PubScreenNode *pubscrnode;
    BOOL found = FALSE;
    BOOL retval = FALSE;

    D(bug("MUIS_ClosePubScreen(%s)\n", name));

    pubscrlist = LockPubScreenList();
    ForeachNode(pubscrlist, pubscrnode)
    {
	if(strcmp(pubscrnode->psn_Node.ln_Name, name) == 0)
	{
	    found = TRUE;
	    break;
	}
    }
    UnlockPubScreenList();
    
    if(found)
    {
	CloseScreen(pubscrnode->psn_Screen);

	struct Node *node;
	ForeachNode(&MUIScreenBase->clients, node)
	{
	    struct MUIS_InfoClient *client = (struct MUIS_InfoClient*) node;
	    Signal(client->task, client->sigbit);
	}

	retval = TRUE;
    }

    return retval;

    AROS_LIBFUNC_EXIT
}
