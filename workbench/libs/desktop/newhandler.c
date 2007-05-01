/*
   Copyright © 1995-2007, The AROS Development Team. All rights reserved.
   $Id$ 
 */

#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/classes.h>
#include <libraries/mui.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/icon.h>

#include "desktop_intern.h"
#include "support.h"
#include "worker.h"

#include "desktop_intern_protos.h"
#include "worker_protos.h"
#include "iconcontainerobserver.h"

#include <string.h>

// Handler states
#define HS_STARTING 1
#define HS_RUNNING  2
#define HS_STOPPING 3

void startedMessage(void)
{
    struct Message *m;

    m = (struct Message *) AllocVec(sizeof(struct Message), MEMF_ANY);
    m->mn_Node.ln_Type = NT_MESSAGE;
    m->mn_ReplyPort = DesktopBase->db_HandlerPort;
    m->mn_Length = sizeof(struct Message);

    PutMsg((struct MsgPort *) ((struct Process *) FindTask(NULL))->pr_Task.
           tc_UserData, m);
}

ULONG desktopHandler(void)
{
    struct DesktopInternMsg *msg,
                   *finalMsg;
    ULONG           handlerState = HS_STARTING;
    BOOL            replyNow = TRUE,
        running = TRUE;
    ULONG           userCount = 0,
        appCount = 0;
    ULONG           idCount = 0;
    struct MinList  workingMessages;

    NEWLIST((struct List *) &workingMessages);

// The library's OPEN vector gets a mutex on the library
// base.  Trouble will come our way if someone started this
// handler elsewhere
    DesktopBase->db_HandlerPort = CreateMsgPort();

    kprintf("--- starting desktop handler\n");

// let the creater know that it's now safe to send the handler
// messages
    startedMessage();

    kprintf("--- desktop handler accepting messages\n");

    while (running)
    {
        WaitPort(DesktopBase->db_HandlerPort);
        while ((msg =
                ((struct DesktopInternMsg *)
                 GetMsg(DesktopBase->db_HandlerPort))))
        {
            if (msg->di_Message.mn_Node.ln_Type == NT_MESSAGE)
            {
                if (handlerState == HS_STARTING)
                {
                    switch (msg->di_Command)
                    {
                        case DIMC_ADDUSER:
                            userCount++;
                            replyNow = TRUE;
                            handlerState = HS_RUNNING;
                            break;
                        default:
                            break;
                    }
                }
                else if (handlerState == HS_STOPPING)
                {
                    switch (msg->di_Command)
                    {
                        case DIMC_ADDUSER:
                            userCount++;
                            replyNow = TRUE;
                            handlerState = HS_RUNNING;
                            break;
                        default:
                            break;
                    }
                }
                else if (handlerState == HS_RUNNING)
                {
                    switch (msg->di_Command)
                    {
                        case DIMC_ADDUSER:
                            {
                                userCount++;
                                replyNow = TRUE;
                                break;
                            }
                        case DIMC_SUBUSER:
                            {
                                kprintf("--- DIMC_SUBUSER\n");
                                userCount--;
                                if (userCount == 0 && appCount == 0)
                                {
                                    kprintf("--- time to exit...\n");
                                    handlerState = HS_STOPPING;
                                    replyNow = TRUE;
                                    kprintf("--- attempting semaphore\n");
                                    if (AttemptSemaphore
                                        (&DesktopBase->db_BaseMutex))
                                    {
                                        if (AttemptSemaphore
                                            (&DesktopBase->db_HandlerSafety))
                                        {
                                            running = FALSE;
                                            replyNow = FALSE;
                                            finalMsg = msg;
                                        }
                                        else
                                            ReleaseSemaphore(&DesktopBase->
                                                             db_BaseMutex);
                                    }
                                }
                                break;
                            }
                        case DIMC_SCANDIRECTORY:
                            {
                                struct HandlerScanRequest *scanMsg =
                                    (struct HandlerScanRequest *) msg;
                                struct WorkingMessageNode *wmn;

                                wmn =
                                    AllocVec(sizeof
                                             (struct WorkingMessageNode),
                                             MEMF_ANY);
                                wmn->wm_Working =
                                    (struct DesktopInternMsg *) scanMsg;
                                wmn->wm_ID = ++idCount;
                                AddTail((struct List *) &workingMessages,
                                        (struct Node *) wmn);

                                wmn->wm_Port =
                                    startScannerWorker(idCount,
                                                       scanMsg->hsr_DirLock,
                                                       DesktopBase->
                                                       db_HandlerPort);

                                replyNow = TRUE;

                                break;
                            }
                        case DIMC_TOPLEVEL:
                            {
                                struct TempNode
                                {
                                    struct Node     t_Node;
                                    UBYTE          *t_Name;
                                };
                                struct HandlerTopLevelRequest *htl =
                                    (struct HandlerTopLevelRequest *) msg;
                                struct DosList *dl;
                                struct TempNode *tn;
                                struct List     tnList;
                                UWORD           i = 0,
                                    j = 0;
                                struct SingleResult *sr;
                                UBYTE          *fullPath;

                                NEWLIST(&tnList);

                                dl = LockDosList(htl->htl_Types | LDF_READ);
                                while
                                (
                                    (dl = NextDosEntry(dl, htl->htl_Types))
                                )
                                {
                                    ULONG length = strlen(dl->dol_Ext.dol_AROS.dol_DevName) + 1;
                                    
                                    tn = (struct TempNode *) AllocVec
                                    (
                                        sizeof(struct TempNode), MEMF_ANY
                                    );
                                    tn->t_Name = AllocVec
                                    (
                                        length + 1, MEMF_ANY
                                    );
                                    strlcpy(tn->t_Name, dl->dol_Ext.dol_AROS.dol_DevName, length);
                                    AddTail(&tnList, (struct Node *) tn);
                                    i++;
                                }

                                UnLockDosList(htl->htl_Types | LDF_READ);

                                sr = (struct SingleResult *)
                                    AllocVec(sizeof(struct SingleResult) * i,
                                             MEMF_ANY);
                                tn = tnList.lh_Head;
                                while (tn->t_Node.ln_Succ)
                                {
                                    ULONG length = strlen(tn->t_Name) + 2;
                                    
                                    sr[j].sr_Name = tn->t_Name;
                                    
                                    fullPath = AllocVec(length + 2, MEMF_ANY);
                                    
                                    strlcpy(fullPath, tn->t_Name, length);
                                    strlcat(fullPath, ":", length);
                                    
                                    sr[j].sr_DiskObject = GetDiskObjectNew
                                    (
                                        fullPath
                                    );
                                    tn = (struct TempNode *) tn->t_Node.ln_Succ;
                                    j++;
                                }

                                DoMethod
                                (
                                    htl->htl_Application,
                                    MUIM_Application_PushMethod,
                                    htl->htl_CallBack, 3,
                                    ICOM_AddIcons, i, sr
                                );

                                break;
                            }
                        default:
                            break;
                    }
                }

                if (replyNow)
                    ReplyMsg((struct Message *) msg);
            }
            else if (msg->di_Message.mn_Node.ln_Type == NT_REPLYMSG)
            {
                struct WorkerMessage *wm = (struct WorkerMessage *) msg;

                switch (wm->w_Action)
                {
                    case WA_SCANNER:
                        {
                            struct WorkerMessage *newMsg;
                            struct WorkingMessageNode *wmn;
                            struct WorkerScanRequest *wsr =
                                (struct WorkerScanRequest *) wm;

                            wmn =
                                findWorkedMessage(&workingMessages,
                                                  wsr->wsr_WMessage.w_ID);

                            DoMethod(((struct HandlerScanRequest *) wmn->
                                      wm_Working)->hsr_Application,
                                     MUIM_Application_PushMethod,
                                     ((struct HandlerScanRequest *) wmn->
                                      wm_Working)->hsr_CallBack, 3,
                                     ICOM_AddIcons, wsr->wsr_Results,
                                     wsr->wsr_ResultsArray);
                            DoMethod(((struct HandlerScanRequest *) wmn->
                                      wm_Working)->hsr_Application,
                                     MUIM_Application_PushMethod,
                                     ((struct HandlerScanRequest *) wmn->
                                      wm_Working)->hsr_CallBack, 2,
                                     OM_FreeList_Add, wsr->wsr_ExAllBuffer);

                            if (wsr->wsr_More)
                            {
                                newMsg =
                                    createWorkerScanMessage(WM_RESUME,
                                                            WA_SCANNER,
                                                            wmn->wm_ID,
                                                            DesktopBase->
                                                            db_HandlerPort,
                                                            wsr->wsr_DirLock);
                                PutMsg(wmn->wm_Port,
                                       (struct Message *) newMsg);
                            }
                            else
                            {
                                Remove((struct Node *) wmn);
                                ReplyMsg((struct Message *) wmn->wm_Working);
                            }

                            FreeVec(wsr);

                            break;
                        }
                }
            // FreeVec(msg);
            }
        }
    }

    kprintf("deleting port\n");
    DeleteMsgPort(DesktopBase->db_HandlerPort);
    DesktopBase->db_HandlerPort = NULL;

    kprintf("releasing semaphores\n");
    ReleaseSemaphore(&DesktopBase->db_HandlerSafety);
    ReleaseSemaphore(&DesktopBase->db_BaseMutex);

    kprintf("replying to msg\n");
    ReplyMsg((struct Message *) finalMsg);

    kprintf("--- shutting down desktop handler\n");

    return 0;
}
