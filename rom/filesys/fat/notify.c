/*
 * fat-handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007-2008 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#define AROS_ALMOST_COMPATIBLE

#include <exec/types.h>
#include <dos/dos.h>
#include <dos/notify.h>
#include <proto/exec.h>

#include "fat_fs.h"
#include "fat_protos.h"

#define DEBUG DEBUG_NOTIFY
#include "debug.h"

void SendNotify(struct NotifyRequest *nr) {
    struct NotifyMessage *nm;

    D(bug("[fat] notifying for '%s'\n", nr->nr_FullName));

    /* signals are a doddle */
    if (nr->nr_Flags & NRF_SEND_SIGNAL) {
        D(bug("[fat] sending signal %ld to task 0x%08x\n", nr->nr_stuff.nr_Signal.nr_SignalNum, nr->nr_stuff.nr_Signal.nr_Task));

        Signal(nr->nr_stuff.nr_Signal.nr_Task, 1 << nr->nr_stuff.nr_Signal.nr_SignalNum);

        return;
    }

    /* if message isn't set, then they screwed up, and there's nothing to do */
    if (!(nr->nr_Flags & NRF_SEND_MESSAGE)) {
        D(bug("[fat] weird, request doesn't have SIGNAL or MESSAGE bits set, doing nothing\n"));
        return;
    }

    /* don't send if we're supposed to wait for them to reply and there's
     * still messages outstanding */
    if (nr->nr_Flags & NRF_WAIT_REPLY && nr->nr_MsgCount > 0) {
        D(bug("[fat] request has WAIT_REPLY set and there are %ld messages outstanding, doing nothing\n", nr->nr_MsgCount));
        return;
    }

    /* new message */
    nr->nr_MsgCount++;

    D(bug("[fat] request now has %ld messages outstanding\n", nr->nr_MsgCount));

    /* allocate and build the message */
    nm = AllocVec(sizeof(struct NotifyMessage), MEMF_PUBLIC | MEMF_CLEAR);
    nm->nm_ExecMessage.mn_ReplyPort = glob->notifyport;
    nm->nm_ExecMessage.mn_Length = sizeof(struct NotifyMessage);
    nm->nm_Class = NOTIFY_CLASS;
    nm->nm_Code = NOTIFY_CODE;
    nm->nm_NReq = nr;

    D(bug("[fat] sending notify message to port 0x%08x\n", nr->nr_stuff.nr_Msg.nr_Port));

    /* send it */
    PutMsg(nr->nr_stuff.nr_Msg.nr_Port, (struct Message *) nm);
}

/* send a notification for the file referenced by the passed global lock */
void SendNotifyByLock(struct FSSuper *sb, struct GlobalLock *gl) {
    struct NotifyNode *nn;

    D(bug("[fat] notifying for lock (%ld/%ld)\n", gl->dir_cluster, gl->dir_entry));

    ForeachNode(&sb->info->notifies, nn)
        if (nn->gl == gl)
            SendNotify(nn->nr);
}

/* send a notification for the file referenced by the passed dir entry */
void SendNotifyByDirEntry(struct FSSuper *sb, struct DirEntry *de) {
    struct NotifyNode *nn;
    struct DirHandle sdh;
    struct DirEntry sde;

    /* Inside the loop we may reuse the dirhandle, so here we explicitly mark it
       as uninitialised */
    sdh.ioh.sb = NULL;

    D(bug("[fat] notifying for dir entry (%ld/%ld)\n", de->cluster, de->index));

    ForeachNode(&sb->info->notifies, nn)
        if (nn->gl != NULL) {
            if (nn->gl->dir_cluster == de->cluster && nn->gl->dir_entry == de->index)
                SendNotify(nn->nr);
        }

        else {
	    if (InitDirHandle(sb, 0, &sdh, TRUE) != 0)
                continue;

            if (GetDirEntryByPath(&sdh, nn->nr->nr_FullName, strlen(nn->nr->nr_FullName), &sde) != 0)
                continue;

            if (sde.cluster == de->cluster && sde.index == de->index)
                SendNotify(nn->nr);

            ReleaseDirHandle(&sdh);
        }
}

/* handle returned notify messages */
void ProcessNotify(void) {
    struct NotifyMessage *nm;

    while ((nm = (struct NotifyMessage *) GetMsg(glob->notifyport)) != NULL)

        if (nm->nm_Class == NOTIFY_CLASS && nm->nm_Code == NOTIFY_CODE) {
            nm->nm_NReq->nr_MsgCount--;
            if (nm->nm_NReq->nr_MsgCount < 0)
                nm->nm_NReq->nr_MsgCount = 0;

            D(bug("[fat] received notify message reply, %ld messages outstanding for this request\n", nm->nm_NReq->nr_MsgCount));

            FreeVec(nm);
        }

        else
            D(bug("[fat] non-notify message received, dropping it\n"));
}
