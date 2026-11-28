/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: File system notification (ACTION_ADD_NOTIFY, ACTION_REMOVE_NOTIFY)
*/

#ifndef DEBUG
#define DEBUG 0
#endif

#include <proto/exec.h>
#include <proto/utility.h>

#include <string.h>

#include "os.h"
#include "afsblocks.h"
#include "cache.h"
#include "filehandles1.h"
#include "notify.h"

#include "baseredef.h"

/*
 * A request names its object by path (nr_FullName, "Volume:path"), and the
 * object need not exist. Nothing about the object is kept here: every change
 * looks the paths of all requests up again and compares header blocks. So a
 * request follows whatever has its name at that moment, on the volume that is
 * in the drive, through deletes, renames of the object or of a directory
 * above it, relabelling, formatting and disk changes.
 */
struct AfsNotify
{
    struct MinNode node;
    struct NotifyRequest *nr;
    BOOL pending;                   /* marked by afsNotifyTouch() */
};

/*
 * The header block of name, relative to dir, and of the directory it is in.
 * 0 when there is no such object, or its volume is not the one in the drive.
 */
static ULONG lookup(struct AFSBase *afsbase, struct AfsHandle *dir,
    CONST_STRPTR name, ULONG *parent)
{
    struct Volume *volume = dir->volume;
    struct BlockCache *blockbuffer;
    ULONG block;
    SIPTR error;

    if (volume->volumenode == NULL || volume->cachepending
            || !mediumPresent(&volume->ioh))
        return 0;
    /* a lock left over from a volume that was removed */
    if (dir->volumenode != NULL && dir->volumenode != volume->volumenode)
        return 0;
    blockbuffer = findBlock(afsbase, dir, name, &block, &error);
    if (blockbuffer == NULL)
        return 0;
    *parent = OS_BE2LONG(blockbuffer->buffer[BLK_PARENT(volume)]);
    return blockbuffer->blocknum;
}

/* The header block of the object a request watches, 0 if there is none */
static ULONG resolve(struct AFSBase *afsbase, struct Volume *volume,
    struct NotifyRequest *nr)
{
    CONST_STRPTR name = nr->nr_FullName;
    BSTR label = volume->devicelist.dl_Name;
    ULONG len, i, parent;

    if (name == NULL || volume->volumenode == NULL || label == BNULL)
        return 0;
    /* the request is for the volume it names, not for this drive */
    len = AROS_BSTR_strlen(label);
    for (i = 0; i < len; i++)
        if (ToUpper(name[i]) != ToUpper(AROS_BSTR_getchar(label, i)))
            return 0;
    if (name[len] != ':')
        return 0;
    return lookup(afsbase, &volume->ah, name + len + 1, &parent);
}

static void notify(struct AFSBase *afsbase, struct NotifyRequest *nr)
{
    struct NotifyMessage *nm;

    D(bug("[afs] notifying '%s'\n", nr->nr_FullName));
    if (nr->nr_Flags & NRF_SEND_SIGNAL)
    {
        Signal(nr->nr_stuff.nr_Signal.nr_Task,
            1L << nr->nr_stuff.nr_Signal.nr_SignalNum);
    }
    else if (nr->nr_Flags & NRF_SEND_MESSAGE)
    {
        /* one message at a time; the reply sends the one held back */
        if ((nr->nr_Flags & NRF_WAIT_REPLY) && nr->nr_MsgCount > 0)
        {
            nr->nr_Flags |= NRF_MAGIC;
            return;
        }
        nm = AllocMem(sizeof(struct NotifyMessage), MEMF_PUBLIC | MEMF_CLEAR);
        if (nm == NULL)
            return;
        nm->nm_ExecMessage.mn_ReplyPort = afsbase->notify_mp;
        nm->nm_ExecMessage.mn_Length = sizeof(struct NotifyMessage);
        nm->nm_Class = NOTIFY_CLASS;
        nm->nm_Code = NOTIFY_CODE;
        nm->nm_NReq = nr;
        nr->nr_MsgCount++;
        afsbase->notifymsgs++;
        PutMsg(nr->nr_stuff.nr_Msg.nr_Port, (struct Message *)nm);
    }
}

static struct AfsNotify *find(struct AFSBase *afsbase, struct NotifyRequest *nr)
{
    struct AfsNotify *an;

    ForeachNode(&afsbase->notifies, an)
        if (an->nr == nr)
            return an;
    return NULL;
}

LONG afsNotifyAdd(struct AFSBase *afsbase, struct Volume *volume,
    struct NotifyRequest *nr)
{
    struct AfsNotify *an;

    /* A request with neither a signal nor a message is accepted, as the
     * AmigaOS file systems do; it is never notified */
    if (nr == NULL || nr->nr_FullName == NULL)
        return ERROR_REQUIRED_ARG_MISSING;
    if (find(afsbase, nr) != NULL)
        return ERROR_OBJECT_IN_USE;
    an = AllocMem(sizeof(struct AfsNotify), MEMF_PUBLIC | MEMF_CLEAR);
    if (an == NULL)
        return ERROR_NO_FREE_STORE;
    an->nr = nr;
    AddTail((struct List *)&afsbase->notifies, (struct Node *)&an->node);
    if ((nr->nr_Flags & NRF_NOTIFY_INITIAL) && resolve(afsbase, volume, nr) != 0)
        notify(afsbase, nr);
    return 0;
}

LONG afsNotifyRemove(struct AFSBase *afsbase, struct NotifyRequest *nr)
{
    struct AfsNotify *an = find(afsbase, nr);

    if (an == NULL)
        return ERROR_OBJECT_NOT_FOUND;
    /* its messages stay counted in notifymsgs until they are replied */
    Remove((struct Node *)&an->node);
    FreeMem(an, sizeof(struct AfsNotify));
    return 0;
}

/*
 * Marks the requests that watch the object name (relative to dir) or the
 * directory it is in. Nothing is sent before afsNotifyFlush(): an operation
 * that removes the object, or takes it out of its directory, has to mark
 * before it is known to succeed.
 */
void afsNotifyTouch(struct AFSBase *afsbase, struct AfsHandle *dir,
    CONST_STRPTR name)
{
    struct AfsNotify *an;
    ULONG block, parent, watched;

    if (IsListEmpty((struct List *)&afsbase->notifies))
        return;
    block = lookup(afsbase, dir, name, &parent);
    if (block == 0)
        return;
    ForeachNode(&afsbase->notifies, an)
    {
        watched = resolve(afsbase, dir->volume, an->nr);
        if (watched != 0 && (watched == block || watched == parent))
            an->pending = TRUE;
    }
}

/* Notifies the marked requests, or only unmarks them */
void afsNotifyFlush(struct AFSBase *afsbase, BOOL send)
{
    struct AfsNotify *an;

    ForeachNode(&afsbase->notifies, an)
    {
        if (an->pending && send)
            notify(afsbase, an->nr);
        an->pending = FALSE;
    }
}

/* Takes the replied notification messages off the port */
void afsNotifyReplies(struct AFSBase *afsbase)
{
    struct NotifyMessage *nm;
    struct NotifyRequest *nr;

    while ((nm = (struct NotifyMessage *)GetMsg(afsbase->notify_mp)) != NULL)
    {
        nr = nm->nm_NReq;
        FreeMem(nm, sizeof(struct NotifyMessage));
        afsbase->notifymsgs--;
        /* EndNotify() replies what is left itself; its request is gone */
        if (find(afsbase, nr) != NULL)
        {
            nr->nr_MsgCount--;
            if (nr->nr_Flags & NRF_MAGIC)
            {
                nr->nr_Flags &= ~NRF_MAGIC;
                notify(afsbase, nr);
            }
        }
    }
}
