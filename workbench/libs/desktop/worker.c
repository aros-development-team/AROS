/*
   Copyright © 1995-2003, The AROS Development Team. All rights reserved.
   $Id$ 
*/

#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/memory.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/intuition.h>

#include "worker.h"

#include "desktop_intern.h"

#include "desktop_intern_protos.h"

#include <stdarg.h>
#include <string.h>

/*
   FUTURE WORK: 1.  The worker should work inbetween messages from a
   requester. This will give faster results. 2.  The WorkerMessage structure
   is a bit bloated, perhaps have a seperate message for starts? 
 */

void scan(struct ScannerWorkerContext *swc)
{
    struct ExAllData *ead;
    struct SingleResult *sr = NULL;
    int             i = 0;
    UBYTE          *fullPath;
    BPTR            fileLock;
    struct FileInfoBlock *fib;
    ULONG           commentSize;

    swc->swc_More = ExAll
    (
        swc->swc_DirLock, (struct ExAllData *) swc->swc_Buffer,
        SCAN_BUFFER, ED_OWNER, swc->swc_EAC
    );

    if (swc->swc_EAC->eac_Entries)
    {
        ead = swc->swc_Buffer;
        sr = (struct SingleResult *) AllocVec
        (
            swc->swc_EAC->eac_Entries * sizeof(struct SingleResult), MEMF_ANY
        );
        for (i = 0; i < swc->swc_EAC->eac_Entries; i++)
        {
            ULONG length = strlen(swc->swc_DirName) + 1 /* strlen("/") */ 
                         + strlen(ead->ed_Name) + 1 /* '\0' */;
            
            sr[i].sr_Name = ead->ed_Name;

            fullPath = AllocVec(length, MEMF_ANY);
            
            strlcpy(fullPath, swc->swc_DirName, length);
            if (swc->swc_DirName[strlen(swc->swc_DirName) - 1] != ':')
            {
                strlcat(fullPath, "/", length);
            }
            strlcat(fullPath, ead->ed_Name, length);

            sr[i].sr_DiskObject = GetDiskObjectNew(fullPath);

            fileLock = Lock(fullPath, ACCESS_READ);
            if (fileLock)
            {
                fib = AllocDosObject(DOS_FIB, NULL);
                if (fib)
                {
                    if (Examine(fileLock, fib))
                    {
                        commentSize = strlen(fib->fib_Comment) + 1;
                        sr[i].sr_Comment = AllocVec(commentSize, MEMF_ANY);
                        CopyMem(fib->fib_Comment, sr[i].sr_Comment,
                                commentSize);
                        sr[i].sr_Script = fib->fib_Protection & FIBF_SCRIPT;
                        sr[i].sr_Pure = fib->fib_Protection & FIBF_PURE;
                        sr[i].sr_Archive = fib->fib_Protection & FIBF_ARCHIVE;
                        sr[i].sr_Read = fib->fib_Protection & FIBF_READ;
                        sr[i].sr_Write = fib->fib_Protection & FIBF_WRITE;
                        sr[i].sr_Execute = fib->fib_Protection & FIBF_EXECUTE;
                        sr[i].sr_Delete = fib->fib_Protection & FIBF_DELETE;
                        sr[i].sr_Type = fib->fib_DirEntryType;
                        sr[i].sr_Size = fib->fib_Size;
                        sr[i].sr_LastModified = fib->fib_Date;
                    }
                    FreeDosObject(DOS_FIB, fib);
                }
                UnLock(fileLock);
            }

            ead = ead->ed_Next;

            FreeVec(fullPath);
        }
    }

    ((struct WorkerScanRequest *) swc->swc_CurrentRequest)->wsr_Results =
        swc->swc_EAC->eac_Entries;
    ((struct WorkerScanRequest *) swc->swc_CurrentRequest)->wsr_ResultsArray =
        sr;
    ((struct WorkerScanRequest *) swc->swc_CurrentRequest)->wsr_More =
        swc->swc_More;
}

void startScan(struct ScannerWorkerContext *swc)
{
    UBYTE          *dirName;
    UWORD           bufferSize = 100;
    BOOL            success;

// a pointer to this buffer is given to the iconobserver class and
// will be freed when the class is destroyed
    swc->swc_Buffer = (STRPTR) AllocVec(SCAN_BUFFER, MEMF_ANY);
    swc->swc_DirLock =
        ((struct WorkerScanRequest *) swc->swc_CurrentRequest)->wsr_DirLock;
    swc->swc_EAC =
        (struct ExAllControl *) AllocDosObject(DOS_EXALLCONTROL, NULL);
    swc->swc_EAC->eac_LastKey = 0;

    dirName = (UBYTE *) AllocVec(bufferSize, MEMF_ANY);
    success = NameFromLock(swc->swc_DirLock, dirName, bufferSize);
    if (!success)
    {
        while (IoErr() == ERROR_LINE_TOO_LONG)
        {
            FreeVec(bufferSize);
            bufferSize += 50;
            dirName = (UBYTE *) AllocVec(bufferSize, MEMF_ANY);
            success = NameFromLock(swc->swc_DirLock, dirName, bufferSize);
            if (success)
                break;
        }
    }

    swc->swc_DirName = dirName;

    scan(swc);
}

void resumeScan(struct ScannerWorkerContext *swc)
{
    swc->swc_Buffer = (STRPTR) AllocVec(SCAN_BUFFER, MEMF_ANY);
    scan(swc);
}

void stopScan(struct ScannerWorkerContext *swc)
{
    ExAllEnd(swc->swc_DirLock, (struct ExAllData *) swc->swc_Buffer,
             SCAN_BUFFER, ED_OWNER, swc->swc_EAC);
}

void destroyScanWorker(struct ScannerWorkerContext *swc)
{
    UnLock(swc->swc_DirLock);
    FreeDosObject(DOS_EXALLCONTROL, swc->swc_EAC);
    FreeVec(swc->swc_DirName);
    FreeVec(swc);
}

ULONG workerEntry(void)
{
    BOOL            running = TRUE;
    struct MsgPort *port;
    struct WorkerMessage *msg;
    struct ScannerWorkerContext *swc = NULL;

    port = &((struct Process *) FindTask(NULL))->pr_MsgPort;

    while (running)
    {
        WaitPort(port);

        while ((msg = (struct WorkerMessage *) (GetMsg(port))))
        {
            switch (msg->w_Command)
            {
                case WM_START:
                    {
                        swc =
                            (struct ScannerWorkerContext *)
                            AllocVec(sizeof(struct ScannerWorkerContext),
                                     MEMF_ANY);
                        swc->swc_Context.workerAction = msg->w_Command;
                        swc->swc_Context.start = (APTR) startScan;
                        swc->swc_Context.resume = (APTR) resumeScan;
                        swc->swc_Context.stop = (APTR) stopScan;
                        swc->swc_EAC = NULL;
                        swc->swc_DirLock = NULL;
                        swc->swc_More = FALSE;
                        swc->swc_Buffer = NULL;
                        swc->swc_CurrentRequest =
                            (struct WorkerMessage *) msg;
                        swc->swc_Context.start(swc);
                        if (!swc->swc_More)
                        {
                            ((struct WorkerScanRequest *) swc->
                             swc_CurrentRequest)->wsr_More = FALSE;
                            running = FALSE;
                            destroyScanWorker(swc);
                        }
                        break;
                    }
                case WM_RESUME:
                    swc->swc_CurrentRequest = msg;
                    swc->swc_Context.resume(swc);
                    if (!swc->swc_More)
                    {
                        ((struct WorkerScanRequest *) swc->
                         swc_CurrentRequest)->wsr_More = FALSE;
                        running = FALSE;
                        destroyScanWorker(swc);
                    }
                    break;

                case WM_STOP:
                    if (msg)
                        swc->swc_Context.stop(swc);
                    destroyScanWorker(swc);
                    running = FALSE;
                    break;

                default:
                    break;
            }

            ReplyMsg((struct Message *) msg);
        }
    }

    return 0;
}

struct WorkerMessage *createWorkerScanMessage(ULONG workerCommand,
                                              ULONG workerAction,
                                              ULONG messageID,
                                              struct MsgPort *replyPort,
                                              BPTR dirLock)
{
    struct WorkerMessage *wm = NULL;
    struct WorkerScanRequest *sr;

    sr = AllocVec(sizeof(struct WorkerScanRequest), MEMF_ANY);
    sr->wsr_WMessage.w_Message.mn_Node.ln_Type = NT_MESSAGE;
    sr->wsr_WMessage.w_Message.mn_ReplyPort = replyPort;
    sr->wsr_WMessage.w_Message.mn_Length = sizeof(struct WorkerScanRequest);
    sr->wsr_WMessage.w_Command = workerCommand;

    sr->wsr_WMessage.w_Action = workerAction;
    sr->wsr_WMessage.w_ID = messageID;
    sr->wsr_DirLock = dirLock;
    wm = (struct WorkerMessage *) sr;

    return wm;
}

struct MsgPort *startScannerWorker(ULONG id, BPTR dirLock,
                                   struct MsgPort *replyPort)
{
    struct Process *process;
    struct ScannerWorkerMessage *msg;
    
    process = CreateNewProcTags
    (
        NP_Entry,     (IPTR) workerEntry,
        NP_Name,      (IPTR) "Worker_Scanner",
        NP_StackSize,        8192,
        TAG_DONE
    );
    
    if (process == NULL) return NULL;

    msg = createWorkerScanMessage
    (
        WM_START, WA_SCANNER, id, replyPort, dirLock
    );

    PutMsg(&process->pr_MsgPort, (struct Message *) msg);


    return &process->pr_MsgPort;
}
