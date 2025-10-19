/*
    Copyright (C) 2009-2025, The AROS Development Team. All rights reserved.
*/

#include <libraries/muiscreen.h>
#include <proto/intuition.h>
#include <intuition/screens.h>
#include <proto/intuition.h>
#include <proto/exec.h>
#include <exec/lists.h>
#include <string.h>
#define DEBUG 0
#include <aros/debug.h>

#include "muiscreen_intern.h"

/*****************************************************************************

    NAME */
#include <proto/muiscreen.h>

        AROS_LH1(BOOL, MUIS_ClosePubScreen,

/*  SYNOPSIS */
        AROS_LHA(char *, name,  A0),

/*  LOCATION */
        struct MUIScreenBase_intern *, MUIScreenBase, 8, MUIScreen)

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

    if (MUIScreenBase->muisb_def && !strcmp(MUIScreenBase->muisb_def, name)) {
        SetDefaultPubScreen("");
    }

    if(found)
    {
        struct Node *node, *tmpnode;

        PubScreenStatus(pubscrnode->psn_Screen, PSNF_PRIVATE);
        ObtainSemaphore(&MUIScreenBase->muisb_acLock);
        ForeachNodeSafe(&MUIScreenBase->muisb_autocScreens, node, tmpnode) {
            if (node->ln_Name == (char *)pubscrnode->psn_Screen) {
                Remove(node);
                FreeVec(node);
                break;
            }
        }
        ReleaseSemaphore(&MUIScreenBase->muisb_acLock);
        CloseScreen(pubscrnode->psn_Screen);

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
