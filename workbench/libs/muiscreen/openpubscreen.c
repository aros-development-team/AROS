/*
    Copyright (C) 2009-2025, The AROS Development Team. All rights reserved.
*/

#include <libraries/muiscreen.h>
#include <utility/hooks.h>
#include <intuition/screens.h>
#include <proto/intuition.h>
#include <proto/exec.h>
#include <exec/lists.h>
#define DEBUG 0
#include <aros/debug.h>

#include "muiscreen_intern.h"

/*****************************************************************************

    NAME */
#include <proto/muiscreen.h>

        AROS_LH1(char *, MUIS_OpenPubScreen,

/*  SYNOPSIS */
        AROS_LHA(struct MUI_PubScreenDesc *, desc,  A0),

/*  LOCATION */
        struct MUIScreenBase_intern *, MUIScreenBase, 7, MUIScreen)

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

    struct Node *autocNode;
    struct Hook *backfillHook = NULL;
    char *ret = NULL;

    D(bug("[MUIScreen] %s(%p) name %s\n", __func__, desc, desc->Name));

    // TODO desc->CloseGadget
    // TODO desc->SystemPens
    // TODO desc->Palette

   if (desc->AutoClose && MUIScreenBase->muisb_taskMsgPort) {
       autocNode = AllocVec(sizeof(struct Node), MEMF_CLEAR);
   } else
       autocNode = NULL;

    struct Screen *screen = OpenScreenTags(NULL,
            SA_Type, (IPTR) PUBLICSCREEN,
            SA_PubName, desc->Name,
            SA_Title, desc->Title,
            (backfillHook) ? SA_BackFill : TAG_IGNORE,
                backfillHook,
            (autocNode) ? SA_PubTask : TAG_IGNORE,
                MUIScreenBase->muisb_closeTask,
            (autocNode) ? SA_PubSig : TAG_IGNORE,
                (autocNode) ? MUIScreenBase->muisb_taskMsgPort->mp_SigBit : -1,
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
        if ((PubScreenStatus(screen, 0) & 1) == 0)
        {
            D(bug("[MUIScreen] %s: Can't make screen public\n", __func__));
            if (autocNode)
                FreeVec(autocNode);
            CloseScreen(screen);
        }
        else
        {
            ret = desc->Name;

            if (autocNode) {
                autocNode->ln_Name = (char *)screen;
                ObtainSemaphore(&LIBBASE->muisb_acLock);
                AddTail(&LIBBASE->muisb_autocScreens, autocNode);
                ReleaseSemaphore(&LIBBASE->muisb_acLock);
            }

            if (desc->SysDefault) {
                MUIScreenBase->muisb_def = ret;
                SetDefaultPubScreen((UBYTE *)MUIScreenBase->muisb_def);
            }

            struct Node *node;
            ForeachNode(&MUIScreenBase->clients, node)
            {
                struct MUIS_InfoClient *client = (struct MUIS_InfoClient*) node;
                Signal(client->task, client->sigbit);
            }
        }
    } else {
        if (autocNode)
            FreeVec(autocNode);
    }

    return ret;

    AROS_LIBFUNC_EXIT
}
