/*
   Copyright © 1995-2003, The AROS Development Team. All rights reserved.
   $Id$ 
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <intuition/classes.h>
#include <libraries/desktop.h>
#include <libraries/gadtools.h>
#include <utility/tagitem.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include "desktop_intern.h"
#include "support.h"

#include "desktop_intern_protos.h"

#define DEBUG 1
#include <aros/debug.h>

BOOL startDesktopHandler(void)
{
    /*
        NOTE: the OPEN vector (the only caller of this function)
        already has a mutex on the library base at this point
    */
    struct Process *process = NULL;
    struct Message *msg     = NULL;
    struct MsgPort *port    = (struct MsgPort *) CreateMsgPort();
    
    if (port == NULL)
    {
        D(bug("[desktop handler starter] ERROR: Could not create message port!\n"));
        return FALSE;
    }
    
    D(bug("*** Starting desktop handler\n"));
    process = CreateNewProcTags
    (
        NP_Entry,     (IPTR) desktopHandler,
        NP_Name,      (IPTR) "Desktop Handler",
        NP_StackSize,        8192,
        NP_UserData,  (IPTR) port,
        
        TAG_DONE
    );
    
    if (process == NULL)
    {
        // FIXME: deallocate msgport??
        D(bug("[desktop handler starter] ERROR: Could not start desktop handler process!\n")); 
        return FALSE;
    }
    
    WaitPort(port);
    msg = GetMsg(port);
    ReplyMsg(msg);

    D(bug("*** Desktop Handler started OK\n"));

    return TRUE;
}

BOOL handlerAddUser(void)
{
    struct MsgPort *port;
    struct DesktopInternMsg msg;

    kprintf("/// Attempting to obtain semaphore\n");
    if (!AttemptSemaphoreShared(&DesktopBase->db_HandlerSafety))
        return FALSE;

    port = CreateMsgPort();
    if (port)
    {
        msg.di_Message.mn_Node.ln_Type = NT_MESSAGE;
        msg.di_Message.mn_ReplyPort = port;
        msg.di_Message.mn_Length = sizeof(struct DesktopInternMsg);
        msg.di_Command = DIMC_ADDUSER;

        PutMsg(DesktopBase->db_HandlerPort, (struct Message *) &msg);

        kprintf("/// addmsg: awaitng reply from handler\n");
        WaitPort(port);
        GetMsg(port);
    }
    kprintf("/// addmsg: got reply, releasing semaphore\n");

    ReleaseSemaphore(&DesktopBase->db_HandlerSafety);

    return TRUE;
}

BOOL handlerSubUser(void)
{
    struct MsgPort *port;
    struct DesktopInternMsg msg;

    port = CreateMsgPort();
    if (port)
    {
        msg.di_Message.mn_Node.ln_Type = NT_MESSAGE;
        msg.di_Message.mn_ReplyPort = port;
        msg.di_Message.mn_Length = sizeof(struct DesktopInternMsg);
        msg.di_Command = DIMC_SUBUSER;

        PutMsg(DesktopBase->db_HandlerPort, (struct Message *) &msg);

        WaitPort(port);
        GetMsg(port);
    }

    return TRUE;
}

struct HandlerScanRequest *createScanMessage(ULONG command,
                                             struct MsgPort *replyPort,
                                             BPTR dirLock, Object * callback,
                                             Object * app)
{
    struct HandlerScanRequest *hsr;

    hsr =
        (struct HandlerScanRequest *)
        AllocVec(sizeof(struct HandlerScanRequest), MEMF_ANY);
    hsr->hsr_Message.di_Message.mn_Length = sizeof(struct HandlerScanRequest);
    hsr->hsr_Message.di_Message.mn_Node.ln_Type = NT_MESSAGE;
    hsr->hsr_Message.di_Message.mn_ReplyPort = replyPort;
    hsr->hsr_Message.di_Command = command;
    hsr->hsr_CallBack = callback;
    hsr->hsr_DirLock = dirLock;
    hsr->hsr_Application = app;

    return hsr;
}

struct HandlerTopLevelRequest *createTLScanMessage(ULONG command,
                                                   struct MsgPort *replyPort,
                                                   ULONG types,
                                                   Object * callback,
                                                   Object * app)
{
    struct HandlerTopLevelRequest *htl;

    htl =
        (struct HandlerTopLevelRequest *)
        AllocVec(sizeof(struct HandlerTopLevelRequest), MEMF_ANY);
    htl->htl_Message.di_Message.mn_Length = sizeof(struct HandlerScanRequest);
    htl->htl_Message.di_Message.mn_Node.ln_Type = NT_MESSAGE;
    htl->htl_Message.di_Message.mn_ReplyPort = replyPort;
    htl->htl_Message.di_Command = command;
    htl->htl_Types = types;
    htl->htl_CallBack = callback;
    htl->htl_Application = app;

    return htl;
}


struct WorkingMessageNode *findWorkedMessage(struct MinList *list, ULONG id)
{
    struct WorkingMessageNode *wmn;
    BOOL            found = FALSE;

    wmn = (struct WorkingMessageNode *)list->mlh_Head;
    while (!found && wmn->wm_Node.mln_Succ)
    {
        if (wmn->wm_ID == id)
            found = TRUE;
        else
            wmn = (struct WorkingMessageNode *)wmn->wm_Node.mln_Succ;
    }

    return wmn;
}


BOOL findOperationItem(LONG menuNumber, struct DesktopOperationItem * doi,
                       struct NewMenu * menuDat, LONG * i)
{
    LONG            j = 0,
        k;
    BOOL            found = FALSE;


    while (!found && doi[j].doi_Code != 0)
    {
    // find menu number in doi
        if (doi[j].doi_Number == menuNumber)
        {
        // find matching entry in menudat
            k = 0;
            while (!found && menuDat[k].nm_Type != NM_END)
            {
                if (menuDat[k].nm_UserData == doi[j].doi_Code)
                {
                // kprintf("found mutex to exclude : %s\n", doi[j].doi_Name);
                    found = TRUE;
                    *i = k;
                }
                k++;
            }
        }

        if (doi[j].doi_SubItems)
        {
            LONG            m = 0;

            while (!found && doi[j].doi_SubItems[m].doi_Code != 0)
            {
                if (doi[j].doi_SubItems[m].doi_Number == menuNumber)
                {
                // find matching entry in menudat
                    k = 0;
                    while (!found && menuDat[k].nm_Type != NM_END)
                    {
                        if (menuDat[k].nm_UserData ==
                            doi[j].doi_SubItems[m].doi_Code)
                        {
                            found = TRUE;
                            *i = k;
                        }
                        k++;
                    }
                }
                m++;
            }
        }

        j++;
    }

    return found;
}

LONG getItemPosition(struct NewMenu * menuDat, LONG i)
{
    LONG            j = i - 1;
    UBYTE           menuType;

    while (menuDat[j].nm_Type == menuDat[i].nm_Type)
        j--;

    return i - j - 1;
}

void doExclude(struct DesktopOperationItem *doi, struct NewMenu *menuDat,
               LONG n)
{
    LONG            i = 0,
        j = 0;
    LONG            bitNum;

    while (doi[j].doi_Code != 0)
    {
        if (doi[j].doi_MutualExclude)
        {
            bitNum = 0;
            while (bitNum < 32)
            {
                if (bitNum & doi[j].doi_MutualExclude)
                {
                    if (findOperationItem(bitNum, doi, menuDat, &i))
                        menuDat[n].nm_MutualExclude |=
                            (1 << getItemPosition(menuDat, i));
                }

                bitNum++;
            }
        }

        n++;
        if (doi[j].doi_SubItems)
        {
            LONG            k = 0;

            while (doi[j].doi_SubItems[k].doi_Code != 0)
            {
                if (doi[j].doi_SubItems[k].doi_MutualExclude)
                {
                    bitNum = 0;
                    while (bitNum < 32)
                    {
                        if ((1 << bitNum) & doi[j].doi_SubItems[k].
                            doi_MutualExclude)
                        {
                            if (findOperationItem(bitNum, doi, menuDat, &i))
                                menuDat[n].nm_MutualExclude |=
                                    (1 << getItemPosition(menuDat, i));
                        }

                        bitNum++;
                    }
                    k++;
                }
                n++;
            }
        }

        j++;
        n++;
    }
}

void processOperationItem(LONG * reali, LONG * realj,
                          struct DesktopOperationItem *doi,
                          struct NewMenu *menuDat)
{
    LONG            i = *reali,
        j = *realj;

    menuDat[i].nm_Type = NM_ITEM;
    menuDat[i].nm_Label = doi[j].doi_Name;
    menuDat[i].nm_CommKey = 0;
    menuDat[i].nm_Flags = 0;
    menuDat[i].nm_MutualExclude = 0;

    if (doi[j].doi_Flags & DOIF_CHECKABLE)
    {
        menuDat[i].nm_Flags |= CHECKIT;
        if (doi[j].doi_Flags & DOIF_CHECKED)
            menuDat[i].nm_Flags |= CHECKED;
    }

    menuDat[i].nm_UserData = doi[j].doi_Code;
    i++;

    if (doi[j].doi_SubItems)
    {
        ULONG           k = 0;

        while (doi[j].doi_SubItems[k].doi_Code != 0
               && doi[j].doi_SubItems[k].doi_Name != NULL)
        {
            menuDat[i].nm_Type = NM_SUB;
            menuDat[i].nm_Label = doi[j].doi_SubItems[k].doi_Name;
            menuDat[i].nm_CommKey = 0;
            menuDat[i].nm_Flags = 0;
            menuDat[i].nm_MutualExclude = 0;

            if (doi[j].doi_SubItems[k].doi_Flags & DOIF_CHECKABLE)
            {
                menuDat[i].nm_Flags |= CHECKIT;
                if (doi[j].doi_SubItems[k].doi_Flags & DOIF_CHECKED)
                    menuDat[i].nm_Flags |= CHECKED;
            }

            menuDat[i].nm_UserData = doi[j].doi_SubItems[k].doi_Code;
            i++;
            k++;
        }
    }
    j++;

    *reali = i;
    *realj = j;
}
