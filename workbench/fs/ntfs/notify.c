/*
 * ntfs.handler - New Technology FileSystem handler
 *
 * Copyright © 2012 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id $
 */

#define AROS_ALMOST_COMPATIBLE

#include <exec/types.h>
#include <dos/dos.h>
#include <dos/notify.h>
#include <proto/exec.h>

#include "ntfs_fs.h"
#include "ntfs_protos.h"

#include "debug.h"

void SendNotify(struct NotifyRequest *nr)
{
    struct NotifyMessage *nm;

    D(bug("[NTFS] notifying for '%s'\n", nr->nr_FullName));

    /* signals are a doddle */
    if (nr->nr_Flags & NRF_SEND_SIGNAL) {
        D(bug("[NTFS] sending signal %ld to task 0x%08x\n", nr->nr_stuff.nr_Signal.nr_SignalNum, nr->nr_stuff.nr_Signal.nr_Task));

        Signal(nr->nr_stuff.nr_Signal.nr_Task, 1 << nr->nr_stuff.nr_Signal.nr_SignalNum);

        return;
    }

    /* if message isn't set, then they screwed up, and there's nothing to do */
    if (!(nr->nr_Flags & NRF_SEND_MESSAGE)) {
        D(bug("[NTFS] weird, request doesn't have SIGNAL or MESSAGE bits set, doing nothing\n"));
        return;
    }

    /* don't send if we're supposed to wait for them to reply and there's
     * still messages outstanding */
    if (nr->nr_Flags & NRF_WAIT_REPLY && nr->nr_MsgCount > 0) {
        D(bug("[NTFS] request has WAIT_REPLY set and there are %ld messages outstanding, doing nothing\n", nr->nr_MsgCount));
        return;
    }

    /* new message */
    nr->nr_MsgCount++;

    D(bug("[NTFS] request now has %ld messages outstanding\n", nr->nr_MsgCount));

    /* allocate and build the message */
    nm = AllocVec(sizeof(struct NotifyMessage), MEMF_PUBLIC | MEMF_CLEAR);
    nm->nm_ExecMessage.mn_ReplyPort = glob->notifyport;
    nm->nm_ExecMessage.mn_Length = sizeof(struct NotifyMessage);
    nm->nm_Class = NOTIFY_CLASS;
    nm->nm_Code = NOTIFY_CODE;
    nm->nm_NReq = nr;

    D(bug("[NTFS] sending notify message to port 0x%08x\n", nr->nr_stuff.nr_Msg.nr_Port));

    /* send it */
    PutMsg(nr->nr_stuff.nr_Msg.nr_Port, (struct Message *) nm);
}

/* send a notification for the file referenced by the passed global lock */
void SendNotifyByLock(struct FSData *sb, struct GlobalLock *gl)
{
    struct NotifyNode *nn;

    D(bug("[NTFS] notifying for lock (%ld/%ld)\n", gl->dir_cluster, gl->dir_entry));

    ForeachNode(&sb->info->notifies, nn)
        if (nn->gl == gl)
            SendNotify(nn->nr);
}

/* send a notification for the file referenced by the passed dir entry */
void SendNotifyByDirEntry(struct FSData *sb, struct DirEntry *de)
{
    struct NotifyNode *nn;
    struct DirHandle sdh;
    struct DirEntry sde;

    /* Inside the loop we may reuse the dirhandle, so here we explicitly mark it
       as uninitialised */
    sdh.ioh.data = NULL;

    D(bug("[NTFS] notifying for dir entry (%ld/%ld)\n", de->cluster, de->no));

    ForeachNode(&sb->info->notifies, nn)
        if (nn->gl != NULL) {
            if (nn->gl->dir_cluster == de->cluster && nn->gl->dir_entry == de->no)
                SendNotify(nn->nr);
        }

        else {
	    if (InitDirHandle(sb, &sdh, TRUE) != 0)
                continue;

            if (GetDirEntryByPath(&sdh, nn->nr->nr_FullName, strlen(nn->nr->nr_FullName), &sde) != 0)
                continue;

            if (sde.cluster == de->cluster && sde.no == de->no)
                SendNotify(nn->nr);

            ReleaseDirHandle(&sdh);
        }
}

/* handle returned notify messages */
void ProcessNotify(void)
{
    struct NotifyMessage *nm;

    while ((nm = (struct NotifyMessage *) GetMsg(glob->notifyport)) != NULL)

        if (nm->nm_Class == NOTIFY_CLASS && nm->nm_Code == NOTIFY_CODE) {
            nm->nm_NReq->nr_MsgCount--;
            if (nm->nm_NReq->nr_MsgCount < 0)
                nm->nm_NReq->nr_MsgCount = 0;

            D(bug("[NTFS] received notify message reply, %ld messages outstanding for this request\n", nm->nm_NReq->nr_MsgCount));

            FreeVec(nm);
        }

        else
            D(bug("[NTFS] non-notify message received, dropping it\n"));
}
