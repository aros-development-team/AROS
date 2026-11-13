/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/log.h>
#include <proto/dos.h>

#include <resources/log.h>
#include <exec/ports.h>

#include <stdio.h>
#include <string.h>

#include "logtest.h"

struct Library *LogResBase = NULL;

#define ARG_TEMPLATE "ALL/S"

enum
{
    ARG_ALL = 0,
    NOOFARGS
};

int main(void)
{
    IPTR args[NOOFARGS] =
    {
               FALSE    // ARG_ALL
    };
    struct RDArgs *rda;
    rda = ReadArgs(ARG_TEMPLATE, args, NULL);

    LogResBase = OpenResource("log.resource");
    if (LogResBase)
    {
        struct MsgPort *MsgPort;
        MsgPort = CreateMsgPort();
        if (!MsgPort)
            return 0;
        APTR eventListener;
        if ((eventListener= logAddListener(MsgPort, LOGF_Flag_Type_All | EHMF_ALLEVENTS)) != NULL)
        {
            BOOL running = TRUE;
            printf("Listening for log.resource events...\n");
            while (running)
            {
                struct LogResBroadcastMsg *lrbMsg = NULL;
                ULONG signals = Wait((1 << MsgPort->mp_SigBit) | SIGBREAKF_CTRL_C);
                if (signals & SIGBREAKF_CTRL_C)
                {
                    running = FALSE;
                    continue;
                }
                else if (signals & (1 << MsgPort->mp_SigBit))
                    while ((lrbMsg = (struct LogResBroadcastMsg *)GetMsg(MsgPort)) != NULL)
                    {
                        if (args[ARG_ALL])
                            printf("Event received...\n");

                        switch (lrbMsg->lrbm_MsgType)
                        {
                            case EHMF_START:
                                if (args[ARG_ALL])
                                    printf(" - START event\n");
                                break;
                            case EHMF_STOP:
                                if (args[ARG_ALL])
                                    printf(" - STOP event\n");
                                break;
                            case EHMF_ADDENTRY:
                                {
                                    if (!args[ARG_ALL])
                                    {
                                        char *compStr = NULL;
                                        struct TagItem entryAttribs[] = {
                                            { LOGMA_Component,      (IPTR)&compStr      },
                                            { TAG_DONE, 0 }
                                        };
                                        logGetEntryAttrs(lrbMsg->lrbm_Target, entryAttribs);
                                        if ((compStr) && (!strcmp(compStr, LOGTEST_COMPONENT)))
                                        {
                                            printf(" - ADD entry 0x%p\n", lrbMsg->lrbm_Target);
                                        }
                                    }
                                    else
                                        printf(" - ADD entry 0x%p\n", lrbMsg->lrbm_Target);
                                }
                                break;
                            case EHMF_REMENTRY:
                                if (args[ARG_ALL])
                                    printf(" - REM entry 0x%p\n", lrbMsg->lrbm_Target);
                                break;
                            default:
                                if (args[ARG_ALL])
                                    printf(" Unknown event type (!?!)\n");
                                break;
                        }
                        logDeleteBroadcast(eventListener, lrbMsg);
                    }
            }
            logRemListener(eventListener);
        }
        DeleteMsgPort(MsgPort);
    }

    return 20;
}
