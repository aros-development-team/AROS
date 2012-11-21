/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Execute a loaded command synchronously
    Lang: english
*/

#include <aros/debug.h>
#include <exec/memory.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <proto/arossupport.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <string.h>

#include "dos_intern.h"

/* Under AOS, BCPL handlers expect to receive a pointer to their
 * startup packet in D1.
 *
 * This wrapper is here to support that.
 */
void BCPL_RunHandler(void);

struct MsgPort *RunHandler(struct DeviceNode *deviceNode, const char *path, struct DosLibrary *DOSBase)
{
        D(struct FileSysStartupMsg *fssm = NULL;)
        struct DosPacket *dp;
        struct MsgPort *reply_port;
        struct Process *process = NULL;
        BSTR bpath;
        ULONG len;
        CONST_STRPTR handler;
        APTR entry;

        handler = AROS_BSTR_ADDR(deviceNode->dn_Handler);

        /* No possible way to continue? */
        if (deviceNode->dn_SegList == BNULL && handler == NULL)
            return NULL;

        if (deviceNode->dn_SegList == BNULL) {
            struct Segment *seg = NULL;

            /* Try to find in the Resident Segment list */
            Forbid();
            D(bug("[RunHandler] Looking for handler '%s' in resident list\n",
                handler));
            seg = FindSegment(handler, NULL, TRUE);
            Permit();

            deviceNode->dn_SegList = seg ? seg->seg_Seg : BNULL;
        }

        if (deviceNode->dn_SegList == BNULL) {
            D(bug("[RunHandler] LoadSeg(\"%s\")\n", handler));

            deviceNode->dn_SegList = LoadSeg(handler);
        }

        if (deviceNode->dn_SegList == BNULL) {
            CONST_STRPTR cp = FilePart(handler);

            if (cp != NULL) {
                BPTR dir;
                dir = Lock("L:", SHARED_LOCK);
                if (dir != BNULL) {
                    BPTR olddir;
                    olddir = CurrentDir(dir);
                    D(bug("[RunHandler] LoadSeg(\"L:%s\")\n", cp));
                    deviceNode->dn_SegList = LoadSeg(cp);
                    CurrentDir(olddir);
                }
            }
        }

        if (deviceNode->dn_SegList == BNULL) {
                D(bug("[RunHandler] name '%b' seglist=NULL?\n", deviceNode->dn_Name));
                return NULL;
        }

        if (path)
        {
            bpath = CreateBSTR(path);
            if (bpath == BNULL)
                return NULL;
        }
        else
        {
            path  = AROS_BSTR_ADDR(deviceNode->dn_Name);
            len   = AROS_BSTR_strlen(deviceNode->dn_Name);
            bpath = MKBADDR(AllocVec(AROS_BSTR_MEMSIZE4LEN(len + 1), MEMF_PUBLIC));
            if (bpath == BNULL)
                return NULL;

            CopyMem(path, AROS_BSTR_ADDR(bpath), len);
            AROS_BSTR_ADDR(bpath)[len++] = ':';
            AROS_BSTR_setstrlen(bpath, len);
        }

        D(bug("[RunHandler] in open by Task '%s'\n", FindTask(NULL)->tc_Node.ln_Name));

        D(if ((IPTR)deviceNode->dn_Startup >= 64))    /* really an FSSM? */
            D(fssm = (struct FileSysStartupMsg *)BADDR(deviceNode->dn_Startup);)

        D(bug("[RunHandler] devicenode=%08lx path='%b' devicename '%b' unit %d dosname '%b' handler=%x seg=%08lx startup=%08lx\n",
            deviceNode,
            bpath,
            fssm ? fssm->fssm_Device : BNULL,
            fssm ? fssm->fssm_Unit : 0,
            deviceNode->dn_Name,
            deviceNode->dn_Handler,
            deviceNode->dn_SegList,
            deviceNode->dn_Startup));

        D(bug("RunHandler: %b has GlobalVec = %d\n", deviceNode->dn_Name, (SIPTR)deviceNode->dn_GlobalVec));

#ifdef __mc68000
        /* BCPL file-handler support */
        if (deviceNode->dn_GlobalVec != (BPTR)-1 && deviceNode->dn_GlobalVec != (BPTR)-2)
            entry = BCPL_RunHandler;
        else
#endif
            entry = BADDR(deviceNode->dn_SegList)+sizeof(IPTR);

        D(bug("[RunHandler] stacksize %d priority %d\n",
            deviceNode->dn_StackSize,
            deviceNode->dn_Priority));

        /* start it up */
        process = CreateNewProcTags(
                NP_Entry,   (IPTR)entry,
                NP_Seglist, (IPTR)deviceNode->dn_SegList,
                NP_FreeSeglist, (IPTR)FALSE,
                NP_Name,  AROS_BSTR_ADDR(deviceNode->dn_Name), /* GB: always NUL terminated */
                NP_StackSize, deviceNode->dn_StackSize,
                NP_Priority,  deviceNode->dn_Priority,
                TAG_DONE);
  
        D(bug("[RunHandler] started, process structure is 0x%08x\n", process));
        reply_port = CreateMsgPort();
        if (!reply_port) {
            FreeVec(BADDR(bpath));
            return NULL;
        }

        /* build the startup packet */
        dp = (struct DosPacket *) AllocDosObject(DOS_STDPKT, NULL);
        if (!dp) {
            DeleteMsgPort(reply_port);
            FreeVec(BADDR(bpath));
            return NULL;
        }
        dp->dp_Arg1 = (SIPTR)bpath;
        dp->dp_Arg2 = (SIPTR)deviceNode->dn_Startup;
        dp->dp_Arg3 = (SIPTR)MKBADDR(deviceNode);
        dp->dp_Port = reply_port;

        /* A handler can add volumes during startup, so we have to be fully functional before it
           replies the startup packet */

        D(bug("[RunHandler] sending startup packet port=%x\n", &(process->pr_MsgPort)));
        PutMsg(&(process->pr_MsgPort), dp->dp_Link);
        WaitPort(reply_port);
        GetMsg(reply_port);

        DeleteMsgPort(reply_port);
        FreeVec(BADDR(bpath));

        if (dp->dp_Res1 == DOSFALSE)
        {
            D(bug("[RunHandler] handler failed startup [%d]\n", dp->dp_Res2));
            process = NULL;
            deviceNode->dn_Task = NULL; /* Some handlers (e.g. SFS) don't clear dn_Task upon failure. */
        }
            D(else bug("[RunHandler] '%b' now online, dn_Task 0x%p, pr_MsgPort 0x%p\n", deviceNode->dn_Name, deviceNode->dn_Task, &process->pr_MsgPort);)

        FreeDosObject(DOS_STDPKT, dp);

        return process ? &process->pr_MsgPort : NULL;
}
