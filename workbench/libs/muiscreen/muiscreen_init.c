/*
    Copyright (C) 2009-2025, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/intuition.h>

#include <exec/libraries.h>
#include <exec/lists.h>
#include <aros/symbolsets.h>
#include "muiscreen_intern.h"

#define MUIS_STACKSIZE 4096

static VOID GM_UNIQUENAME(pubscreenClose_Task)();

static int GM_UNIQUENAME(libInit)(LIBBASETYPEPTR LIBBASE)
{
    NEWLIST(&LIBBASE->clients);
    NEWLIST(&LIBBASE->muisb_autocScreens);

    LIBBASE->muisb_closeTask = AllocMem(sizeof (struct Task), MEMF_PUBLIC|MEMF_CLEAR);
    if (NULL != LIBBASE->muisb_closeTask) {
        APTR stack;

        NEWLIST(&LIBBASE->muisb_closeTask->tc_MemEntry);
        LIBBASE->muisb_closeTask->tc_Node.ln_Type = NT_TASK;
        LIBBASE->muisb_closeTask->tc_Node.ln_Name = "PUBSCREEN handler";
        LIBBASE->muisb_closeTask->tc_Node.ln_Pri = 0;

        stack=AllocMem(MUIS_STACKSIZE, MEMF_PUBLIC);
        if(NULL != stack) {
            LIBBASE->muisb_closeTask->tc_SPLower = stack;
            LIBBASE->muisb_closeTask->tc_SPUpper = (BYTE *)stack + MUIS_STACKSIZE;
            LIBBASE->muisb_closeTask->tc_UserData = LIBBASE;

#if AROS_STACK_GROWS_DOWNWARDS
            LIBBASE->muisb_closeTask->tc_SPReg = (BYTE *)LIBBASE->muisb_closeTask->tc_SPUpper - SP_OFFSET - sizeof(APTR);
#else
            LIBBASE->muisb_closeTask->tc_SPReg = (BYTE *)LIBBASE->muisb_closeTask->tc_SPLower - SP_OFFSET + sizeof(APTR);
#endif
            if (AddTask(LIBBASE->muisb_closeTask, GM_UNIQUENAME(pubscreenClose_Task), NULL) != NULL) {
                return TRUE;
            }
        } else {
            FreeMem(LIBBASE->muisb_closeTask, sizeof (struct Task));
            LIBBASE->muisb_closeTask = NULL;
        }
    }
    return FALSE;
}

static VOID GM_UNIQUENAME(pubscreenClose_Task)()
{
    struct Task *thistask = FindTask(NULL);
    struct MUIScreenBase_intern *MUIScreenBase = thistask->tc_UserData;

    D(
        bug("[MUIScreen] %s()\n", __func__);
        bug("[MUIScreen] %s: thisTask = 0x%p\n", __func__, thistask);
        bug("[MUIScreen] %s: MUIScreenBase = 0x%p\n", __func__, MUIScreenBase);
    )

    InitSemaphore(&MUIScreenBase->muisb_acLock);

    if ((MUIScreenBase->muisb_taskMsgPort = CreateMsgPort())) {
        do {
            ULONG sigs, signals = (1L << MUIScreenBase->muisb_taskMsgPort->mp_SigBit) | SIGBREAKF_CTRL_C;
            sigs = Wait(signals);
            if (sigs && (1L << MUIScreenBase->muisb_taskMsgPort->mp_SigBit)) {
                struct Node *autocNode, *tmp;

                D(bug("[MUIScreen] %s: msgport signal received\n", __func__);)

                ObtainSemaphore(&LIBBASE->muisb_acLock);
                ForeachNodeSafe(&LIBBASE->muisb_autocScreens, autocNode, tmp) {
                    struct Screen *screen = (struct Screen *)autocNode->ln_Name;
                    if (!screen->FirstWindow) {
                        D(bug("[MUIScreen] %s: closing Screen @ 0x%p\n", __func__, screen);)
                        Remove(autocNode);
                        CloseScreen(screen);
                        FreeVec(autocNode);
                    }
                }
                ReleaseSemaphore(&LIBBASE->muisb_acLock);
            }
            if (sigs & SIGBREAKF_CTRL_C) {
                break;
            }
        } while (1);
    }
    LIBBASE->muisb_closeTask = NULL;
}

int GM_UNIQUENAME(libExpunge)(LIBBASETYPEPTR LIBBASE)
{
    if (LIBBASE->muisb_taskMsgPort) {
        Signal(LIBBASE->muisb_closeTask, SIGBREAKF_CTRL_C);
    }
    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(libInit), 0);
ADD2EXPUNGELIB(GM_UNIQUENAME(libExpunge), 0);
