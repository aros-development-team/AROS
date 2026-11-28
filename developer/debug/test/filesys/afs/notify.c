/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: File system notification on OFS and FFS volumes.

          A notification request is sent when its object changes, and a
          request on a directory also when something in that directory
          changes. A file is notified when it is closed after it was
          created, emptied, written or resized, not for each write. Delete,
          rename, create directory, and setting the date, protection or
          comment notify at once. The object need not exist when the
          request is made.

          The test watches, by signal, a directory W, the volume root, a
          file W/f that does not exist yet (named in another case) and a
          directory W/Later that does not exist yet. It does each of those
          operations and checks which watches signalled, and that changes
          in a sibling or a subdirectory signal nothing. It renames W away
          and back, so that the requests have to follow the name. Then it
          checks notification by message: one message per change,
          NRF_WAIT_REPLY, NRF_NOTIFY_INITIAL, EndNotify() with a message
          still outstanding, and that ACTION_DIE is refused while a request
          is active.

          It runs on RAM: first, for comparison (differences there are
          noted, not failed), then on an OFS and an FFS image it writes to
          RAM:afsnotify, mounts through fdsk.device as AFSNTFY: and
          formats. Each image is dismounted and deleted afterwards.

    Usage: notify [HANDLER <path>]

          HANDLER mounts the images with that handler file instead of the
          system's handler for the DosType.
*/

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/notify.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <stdio.h>
#include <string.h>

#define DIR         "RAM:afsnotify"
#define IMAGE       DIR "/Unit0"
#define MOUNTLIST   DIR "/Mountlist"
#define DEVICE      "AFSNTFY:"
#define LABEL       "NotifyTest"
#define REFERENCE   DIR "/ram/"
#define CMDLOG      "RAM:afsnotify.cmdlog"

#define BLOCKS      1760                /* a DD floppy */

struct Format
{
    CONST_STRPTR name;
    ULONG dos_type;
};

static const struct Format formats[] =
{
    { "OFS", ID_DOS_DISK },
    { "FFS", ID_FFS_DISK }
};

static LONG checks, fails;
static BOOL reference;                  /* differences are noted, not failed */

static BOOL exists(CONST_STRPTR name)
{
    BPTR lock = Lock(name, SHARED_LOCK);

    if (lock)
        UnLock(lock);
    return lock != BNULL;
}

/* Runs a command; when it fails, shows what it printed */
static LONG run(CONST_STRPTR command)
{
    BPTR out = Open(CMDLOG, MODE_NEWFILE);
    LONG rc = SystemTags(command, SYS_Output, out, TAG_END);
    char line[160];

    if (out)
        Close(out);
    if (rc != 0)
    {
        printf("NOTE: '%s' returned %ld\n", (const char *)command, (long)rc);
        if ((out = Open(CMDLOG, MODE_OLDFILE)) != BNULL)
        {
            while (FGets(out, (STRPTR)line, sizeof(line)))
                printf("    %s", line);
            Close(out);
        }
    }
    DeleteFile(CMDLOG);
    return rc;
}

static void check(BOOL ok, CONST_STRPTR how, CONST_STRPTR what)
{
    checks++;
    if (!ok)
    {
        printf("%s: %s %s\n", reference ? "NOTE" : "FAIL", (const char *)how, (const char *)what);
        if (!reference)
            fails++;
    }
}

/* Image ------------------------------------------------------------------ */

static BOOL write_image(void)
{
    static UBYTE zero[512 * 16];
    BPTR fh = Open(IMAGE, MODE_NEWFILE);
    LONG i;
    BOOL ok = TRUE;

    if (!fh)
        return FALSE;
    for (i = 0; ok && i < BLOCKS; i += 16)
        ok = Write(fh, zero, sizeof(zero)) == sizeof(zero);
    return Close(fh) && ok;
}

static BOOL write_mountlist(CONST_STRPTR handler, ULONG dos_type)
{
    BPTR fh = Open(MOUNTLIST, MODE_NEWFILE);
    BOOL ok;

    if (!fh)
        return FALSE;
    ok = FPuts(fh, DEVICE "\n") == 0;
    if (ok && handler)
        ok = FPrintf(fh, "    FileSystem     = %s\n", (IPTR)handler) >= 0;
    ok = ok && FPrintf(fh,
        "    Device         = fdsk.device\n"
        "    Unit           = 0\n"
        "    DosType        = 0x%08lx\n"
        "    SectorSize     = 512\n"
        "    Surfaces       = 2\n"
        "    BlocksPerTrack = 11\n"
        "    Reserved       = 2\n"
        "    LowCyl         = 0\n"
        "    HighCyl        = 79\n"
        "    Buffers        = 20\n"
        "    StackSize      = 16384\n"
        "    Activate       = 1\n"
        "#\n", (IPTR)dos_type) >= 0;
    return Close(fh) && ok;
}

/* Asks the handler to stop; 0 when it did */
static LONG die(void)
{
    struct DevProc *dvp = GetDeviceProc(DEVICE, NULL);
    LONG err;

    if (!dvp)
        return IoErr();
    err = DoPkt(dvp->dvp_Port, ACTION_DIE, 0, 0, 0, 0, 0) ? 0 : IoErr();
    FreeDeviceProc(dvp);
    return err;
}

static void unmount(void)
{
    LONG i, err = 0;

    for (i = 0; i < 10; i++)
    {
        err = die();
        if (!err)
            break;
        Delay(25);
    }
    if (err)
        printf("NOTE: the handler did not stop: error %ld\n", (long)err);
    run("Assign " DEVICE " DISMOUNT");
}

/* Watches by signal ------------------------------------------------------ */

#define WATCHES 4
#define WD 1                            /* the directory W */
#define RT 2                            /* the root of the volume */
#define FL 4                            /* the file W/f */
#define LT 8                            /* the directory W/Later */

static const char *const watch_names[WATCHES] = { "W", "root", "W/f", "W/Later" };
static struct NotifyRequest requests[WATCHES];
static BYTE signals[WATCHES] = { -1, -1, -1, -1 };
static BOOL watching[WATCHES];
static ULONG all_signals;

static BOOL start_watch(LONG w, CONST_STRPTR name)
{
    struct NotifyRequest *nr = &requests[w];

    memset(nr, 0, sizeof(*nr));
    nr->nr_Name = (STRPTR)name;
    nr->nr_Flags = NRF_SEND_SIGNAL;
    nr->nr_stuff.nr_Signal.nr_Task = FindTask(NULL);
    nr->nr_stuff.nr_Signal.nr_SignalNum = signals[w];
    watching[w] = StartNotify(nr);
    if (!watching[w])
    {
        printf("FAIL: StartNotify(%s): error %ld\n", (const char *)name, (long)IoErr());
        fails++;
    }
    return watching[w];
}

static void end_watches(void)
{
    LONG w;

    for (w = 0; w < WATCHES; w++)
        if (watching[w])
        {
            EndNotify(&requests[w]);
            watching[w] = FALSE;
        }
    SetSignal(0, all_signals);
}

/*
 * The watches that signalled since the last call, as a bit per watch. The
 * handlers notify before they reply to the packet; a short wait still lets
 * a late signal count, and a missing one show up.
 */
static ULONG signalled(void)
{
    ULONG got, bits = 0;
    LONG w;

    Delay(2);
    got = SetSignal(0, all_signals) & all_signals;
    for (w = 0; w < WATCHES; w++)
        if (signals[w] >= 0 && (got & (1UL << signals[w])))
            bits |= 1 << w;
    return bits;
}

/* Operations ------------------------------------------------------------- */

static char path_buf[4][128];

/* base + name in one of four rotating buffers */
static CONST_STRPTR p(CONST_STRPTR base, CONST_STRPTR name)
{
    static LONG next;
    char *b = path_buf[next++ & 3];

    snprintf(b, sizeof(path_buf[0]), "%s%s", (const char *)base, (const char *)name);
    return (CONST_STRPTR)b;
}

static BOOL put(CONST_STRPTR name, CONST_STRPTR text, LONG mode)
{
    BPTR fh = Open(name, mode);
    LONG len = text ? strlen((const char *)text) : 0;
    BOOL ok;

    if (!fh)
        return FALSE;
    ok = len == 0 || Write(fh, (APTR)text, len) == len;
    return Close(fh) && ok;
}

static BOOL append(CONST_STRPTR name, CONST_STRPTR text)
{
    BPTR fh = Open(name, MODE_READWRITE);
    LONG len = strlen((const char *)text);
    BOOL ok;

    if (!fh)
        return FALSE;
    ok = Seek(fh, 0, OFFSET_END) >= 0 && Write(fh, (APTR)text, len) == len;
    return Close(fh) && ok;
}

static BOOL read_only(CONST_STRPTR name)
{
    BPTR fh = Open(name, MODE_OLDFILE);
    UBYTE c;

    if (!fh)
        return FALSE;
    Read(fh, &c, 1);
    return Close(fh);
}

static BOOL resize(CONST_STRPTR name, LONG size)
{
    BPTR fh = Open(name, MODE_READWRITE);
    BOOL ok;

    if (!fh)
        return FALSE;
    ok = SetFileSize(fh, size, OFFSET_BEGINNING) == size;
    return Close(fh) && ok;
}

static BOOL make_dir(CONST_STRPTR name)
{
    BPTR lock = CreateDir(name);

    if (lock)
        UnLock(lock);
    return lock != BNULL;
}

static BOOL set_date(CONST_STRPTR name)
{
    struct DateStamp ds;

    DateStamp(&ds);
    ds.ds_Days -= 1;
    return SetFileDate(name, &ds);
}

/*
 * Checks which watches signalled after an operation: want has a bit per watch
 * that must signal; every other active watch must not, except those in
 * either, which may.
 */
static void expect2(BOOL done, ULONG want, ULONG either, CONST_STRPTR what, CONST_STRPTR how)
{
    ULONG got = signalled(), active = 0, wrong;
    LONG w;

    for (w = 0; w < WATCHES; w++)
        if (watching[w])
            active |= 1 << w;
    want &= active;
    got &= active;
    wrong = (got ^ want) & ~either;
    checks++;
    if (!done)
    {
        printf("FAIL: %s %s: the operation failed: error %ld\n", (const char *)how,
            (const char *)what, (long)IoErr());
        fails++;
    }
    else if (wrong)
    {
        printf("%s: %s %s:", reference ? "NOTE" : "FAIL", (const char *)how, (const char *)what);
        for (w = 0; w < WATCHES; w++)
            if (wrong & (1 << w))
                printf(" %s %s;", watch_names[w],
                    (want & (1 << w)) ? "not notified" : "notified wrongly");
        printf("\n");
        if (!reference)
            fails++;
    }
}

#define expect(done, want, what, how) expect2(done, want, 0, what, how)

static BOOL make_tree(CONST_STRPTR base)
{
    return make_dir(p(base, "W")) && make_dir(p(base, "W/Sub")) && make_dir(p(base, "Other"));
}

/*
 * Every change, with W, Other and the watched names below base. root says
 * whether base is the volume root (then changes there notify the root watch).
 */
static void changes(CONST_STRPTR base, CONST_STRPTR how, BOOL root)
{
    char w[100], other[100];
    ULONG in_base = root ? RT : 0;
    BPTR fh;

    snprintf(w, sizeof(w), "%sW/", (const char *)base);
    snprintf(other, sizeof(other), "%sOther/", (const char *)base);

    /* A file: notified when it is closed */
    expect(put(p(w, "f"), NULL, MODE_NEWFILE), WD | FL, "create and close an empty file", how);
    expect(put(p(w, "f"), "data", MODE_READWRITE), WD | FL, "write and close a file", how);
    expect(read_only(p(w, "f")), 0, "read and close a file", how);
    expect(put(p(w, "f"), NULL, MODE_READWRITE), 0, "open a file for update and close it unwritten", how);
    expect(append(p(w, "f"), "more"), WD | FL, "append to a file", how);
    expect(resize(p(w, "f"), 2), WD | FL, "set a file's size", how);
    expect(put(p(w, "f"), NULL, MODE_NEWFILE), WD | FL, "empty a file without writing", how);

    fh = Open(p(w, "f"), MODE_READWRITE);
    expect(fh && Write(fh, "one", 3) == 3, 0, "write to a file that stays open", how);
    expect(fh && Write(fh, "two", 3) == 3, 0, "write to it again", how);
    expect(fh && Close(fh), WD | FL, "close it", how);

    /* Immediately */
    expect(SetProtection(p(w, "f"), FIBF_ARCHIVE), WD | FL, "set a file's protection", how);
    expect(SetComment(p(w, "f"), "comment"), WD | FL, "set a file's comment", how);
    expect(set_date(p(w, "f")), WD | FL, "set a file's date", how);
    expect(Rename(p(w, "f"), p(w, "g")), WD | FL, "rename the watched file to another name", how);
    expect(SetProtection(p(w, "g"), 0), WD, "change it under its other name", how);
    expect(Rename(p(w, "g"), p(w, "F")), WD | FL, "rename it back to the watched name", how);
    expect(Rename(p(w, "f"), p(other, "moved")), WD | FL, "rename a file out of W", how);
    expect(Rename(p(other, "moved"), p(w, "f")), WD | FL, "rename a file into W", how);
    expect(DeleteFile(p(w, "f")), WD | FL, "delete a file", how);
    expect(!DeleteFile(p(w, "f")), 0, "fail to delete a file that is gone", how);
    expect(!Rename(p(w, "none"), p(w, "f")), 0, "fail to rename a file that does not exist", how);

    /* A directory that did not exist when its request was made */
    expect(make_dir(p(w, "Later")), WD | LT, "create a watched directory", how);
    expect(put(p(w, "Later/x"), "x", MODE_NEWFILE), LT, "write a file in it", how);
    expect(!DeleteFile(p(w, "Later")), 0, "fail to delete a directory that is not empty", how);
    expect(DeleteFile(p(w, "Later/x")), LT, "delete the file in it", how);
    expect(Rename(p(w, "Later"), p(base, "Later")), WD | LT | in_base, "move a directory to W's parent", how);
    expect(Rename(p(base, "Later"), p(w, "Later")), WD | LT | in_base, "move a directory back into W", how);
    expect(DeleteFile(p(w, "Later")), WD | LT, "delete a directory", how);

    /* Not in W */
    expect(put(p(w, "Sub/inner"), "x", MODE_NEWFILE), 0, "write a file in W's subdirectory", how);
    expect(DeleteFile(p(w, "Sub/inner")), 0, "delete a file in W's subdirectory", how);
    expect(put(p(other, "file"), "x", MODE_NEWFILE), 0, "write a file in W's sibling", how);
    expect(DeleteFile(p(other, "file")), 0, "delete a file in W's sibling", how);
    expect(put(p(base, "top"), "x", MODE_NEWFILE), in_base, "write a file next to W", how);
    expect(DeleteFile(p(base, "top")), in_base, "delete a file next to W", how);

    /*
     * The requests name W: when W is renamed they watch nothing, and when it
     * is renamed back they watch it and its contents again. Whether the
     * request on the file inside is told that its path came or went is left
     * open.
     */
    expect(put(p(w, "f"), "x", MODE_NEWFILE), WD | FL, "write the watched file again", how);
    expect2(Rename(p(base, "W"), p(base, "Away")), WD | in_base, FL, "rename W away", how);
    expect(put(p(base, "Away/f"), "y", MODE_NEWFILE), 0, "write the file in the renamed directory", how);
    expect(put(p(base, "Away/new"), "y", MODE_NEWFILE), 0, "create a file in the renamed directory", how);
    expect2(Rename(p(base, "Away"), p(base, "W")), WD | in_base, FL, "rename the directory back to W", how);
    expect(put(p(w, "f"), "z", MODE_NEWFILE), WD | FL, "write the watched file after that", how);
    expect(DeleteFile(p(w, "new")), WD, "delete the other file", how);
    expect(DeleteFile(p(w, "f")), WD | FL, "delete the watched file", how);
}

static void watch_and_change(CONST_STRPTR base, CONST_STRPTR how, BOOL root)
{
    /* the file is named in another case than it gets created with */
    if (start_watch(0, p(base, "W")) && (!root || start_watch(1, base))
            && start_watch(2, p(base, "w/F")) && start_watch(3, p(base, "W/Later")))
        changes(base, how, root);
    end_watches();
}

/* Notification by message ------------------------------------------------ */

static LONG messages(struct MsgPort *port, struct NotifyRequest *nr, BOOL reply, BOOL *valid)
{
    struct NotifyMessage *nm;
    struct Node *n;
    LONG count = 0;

    Delay(2);
    if (!reply)
    {
        Forbid();
        for (n = port->mp_MsgList.lh_Head; n->ln_Succ; n = n->ln_Succ)
            count++;
        Permit();
        return count;
    }
    while ((nm = (struct NotifyMessage *)GetMsg(port)) != NULL)
    {
        if (nm->nm_Class != NOTIFY_CLASS || nm->nm_Code != NOTIFY_CODE || nm->nm_NReq != nr)
            *valid = FALSE;
        ReplyMsg((struct Message *)nm);
        count++;
    }
    return count;
}

static BOOL start_message(struct NotifyRequest *nr, struct MsgPort *port, CONST_STRPTR name, ULONG flags)
{
    memset(nr, 0, sizeof(*nr));
    nr->nr_Name = (STRPTR)name;
    nr->nr_Flags = NRF_SEND_MESSAGE | flags;
    nr->nr_stuff.nr_Msg.nr_Port = port;
    if (StartNotify(nr))
        return TRUE;
    printf("FAIL: StartNotify(%s) by message: error %ld\n", (const char *)name, (long)IoErr());
    fails++;
    return FALSE;
}

static void by_message(CONST_STRPTR base, CONST_STRPTR how, BOOL volume)
{
    struct MsgPort *port = CreateMsgPort();
    struct NotifyRequest nr;
    CONST_STRPTR file = p(base, "W/m");
    BOOL valid = TRUE;
    LONG i, err;

    if (!port)
    {
        check(FALSE, how, "no message port");
        return;
    }
    if (start_message(&nr, port, file, 0))
    {
        check(messages(port, &nr, FALSE, &valid) == 0, how, "a request on a file that does not exist sent a message at once");
        for (i = 0; i < 3; i++)
            put(file, "x", MODE_NEWFILE);
        check(messages(port, &nr, FALSE, &valid) == 3, how, "three changes did not send three messages");
        check(nr.nr_MsgCount == 3, how, "nr_MsgCount is not 3 with three messages outstanding");
        check(messages(port, &nr, TRUE, &valid) == 3 && valid, how, "the messages are not NotifyMessages for the request");
        Delay(2);
        check(nr.nr_MsgCount == 0, how, "nr_MsgCount is not 0 after the replies");
        EndNotify(&nr);
    }

    if (start_message(&nr, port, file, NRF_WAIT_REPLY))
    {
        for (i = 0; i < 3; i++)
            put(file, "x", MODE_NEWFILE);
        check(messages(port, &nr, FALSE, &valid) == 1, how, "NRF_WAIT_REPLY: not one message for three changes");
        check(messages(port, &nr, TRUE, &valid) == 1, how, "NRF_WAIT_REPLY: no message to reply");
        /* what changed while the first message was out is told once */
        check(messages(port, &nr, TRUE, &valid) == 1, how, "NRF_WAIT_REPLY: the changes made meanwhile were not notified after the reply");
        check(messages(port, &nr, TRUE, &valid) == 0, how, "NRF_WAIT_REPLY: a message without a change");
        put(file, "x", MODE_NEWFILE);
        check(messages(port, &nr, FALSE, &valid) == 1, how, "NRF_WAIT_REPLY: no message for a change after the replies");

        /* ACTION_DIE has to wait for the request, and for its message */
        if (volume)
        {
            err = die();
            check(err == ERROR_OBJECT_IN_USE, how, "ACTION_DIE was not refused with ERROR_OBJECT_IN_USE while a request was active");
            if (err == 0)
            {
                /* the handler is gone, and the request with it */
                DeleteMsgPort(port);
                return;
            }
        }
        /* with the message still on the port */
        EndNotify(&nr);
        check(messages(port, &nr, FALSE, &valid) == 0, how, "EndNotify() left a message on the port");
    }

    if (start_message(&nr, port, file, NRF_NOTIFY_INITIAL))
    {
        check(messages(port, &nr, TRUE, &valid) == 1, how, "NRF_NOTIFY_INITIAL: no message for a file that exists");
        EndNotify(&nr);
    }
    DeleteFile(file);
    if (start_message(&nr, port, file, NRF_NOTIFY_INITIAL))
    {
        check(messages(port, &nr, TRUE, &valid) == 0, how, "NRF_NOTIFY_INITIAL: a message for a file that does not exist");
        put(file, "x", MODE_NEWFILE);
        check(messages(port, &nr, TRUE, &valid) == 1, how, "no message when the file was created");
        EndNotify(&nr);
    }
    if (start_message(&nr, port, p(base, "W"), NRF_NOTIFY_INITIAL))
    {
        check(messages(port, &nr, TRUE, &valid) == 1, how, "NRF_NOTIFY_INITIAL: no message for a directory that exists");
        EndNotify(&nr);
    }
    DeleteFile(file);
    check(valid, how, "a message was not a NotifyMessage for its request");
    DeleteMsgPort(port);
}

/* A file closed while its volume is inhibited: nothing to notify, no access */
static void while_inhibited(CONST_STRPTR how)
{
    BPTR fh;

    if (!start_watch(0, DEVICE "W") || !start_watch(2, DEVICE "W/f"))
    {
        end_watches();
        return;
    }
    fh = Open(DEVICE "W/f", MODE_NEWFILE);
    expect(fh && Write(fh, "x", 1) == 1, 0, "write a file and leave it open", how);
    if (fh)
    {
        expect(Inhibit(DEVICE, DOSTRUE), 0, "inhibit the volume", how);
        expect(Close(fh), 0, "close the file while the volume is inhibited", how);
        expect(Inhibit(DEVICE, DOSFALSE), 0, "end the inhibit", how);
        expect(put(DEVICE "W/f", "y", MODE_NEWFILE), WD | FL, "write the file after the inhibit", how);
        expect(DeleteFile(DEVICE "W/f"), WD | FL, "delete it", how);
    }
    end_watches();
}

/* ------------------------------------------------------------------------ */

static void test_reference(void)
{
    printf("RAM: (for comparison)\n");
    if (!make_dir(DIR "/ram") || !make_tree(REFERENCE))
    {
        printf("FAIL: cannot create the reference tree in " REFERENCE "\n");
        fails++;
        return;
    }
    reference = TRUE;
    watch_and_change(REFERENCE, "RAM:", FALSE);
    reference = FALSE;
}

static void test_format(const struct Format *format, CONST_STRPTR handler)
{
    char how[40];
    BPTR lock, root;
    BOOL ok;

    printf("%s\n", (const char *)format->name);
    if (!write_image())
    {
        printf("FAIL: cannot write the image to " DIR "\n");
        fails++;
        return;
    }
    ok = write_mountlist(handler, format->dos_type) && run("Mount " DEVICE " FROM " MOUNTLIST) == 0;
    if (ok)
    {
        ok = Inhibit(DEVICE, DOSTRUE) && Format(DEVICE, LABEL, format->dos_type);
        if (!ok)
            printf("NOTE: Format: error %ld\n", (long)IoErr());
        Inhibit(DEVICE, DOSFALSE);
    }
    if (!ok || !exists(DEVICE) || !exists(LABEL ":"))
    {
        printf("FAIL: cannot mount and format the %s image through fdsk.device\n", (const char *)format->name);
        fails++;
        unmount();
        DeleteFile(IMAGE);
        return;
    }
    if (!make_tree(DEVICE))
    {
        printf("FAIL: cannot create the test tree on %s\n", (const char *)format->name);
        fails++;
    }
    else
    {
        snprintf(how, sizeof(how), "%s:", (const char *)format->name);
        watch_and_change(DEVICE, how, TRUE);

        /* The same named by volume, with W and the root locked */
        snprintf(how, sizeof(how), "%s, by volume name, locked:", (const char *)format->name);
        lock = Lock(LABEL ":W", SHARED_LOCK);
        root = Lock(LABEL ":", SHARED_LOCK);
        if (!lock || !root)
        {
            printf("FAIL: cannot lock W and the root on %s\n", (const char *)format->name);
            fails++;
        }
        else
            watch_and_change(LABEL ":", how, TRUE);
        if (root)
            UnLock(root);
        if (lock)
            UnLock(lock);

        snprintf(how, sizeof(how), "%s, inhibited:", (const char *)format->name);
        while_inhibited(how);

        /* A request names its volume by label, and follows the label */
        snprintf(how, sizeof(how), "%s, relabelled:", (const char *)format->name);
        if (start_watch(1, LABEL ":"))
        {
            expect(Relabel(DEVICE, "NotifyAway"), RT, "relabel the volume", how);
            expect(put(DEVICE "top", "x", MODE_NEWFILE), 0, "write a file in the root under the other label", how);
            expect(Relabel(DEVICE, LABEL), RT, "give the volume its label back", how);
            expect(DeleteFile(DEVICE "top"), RT, "delete the file in the root", how);
        }
        end_watches();

        snprintf(how, sizeof(how), "%s, by message:", (const char *)format->name);
        by_message(DEVICE, how, TRUE);
    }
    unmount();
    if (!DeleteFile(IMAGE) || !DeleteFile(MOUNTLIST))
        printf("NOTE: could not delete the %s image\n", (const char *)format->name);
}

int main(void)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR window = me->pr_WindowPtr;
    IPTR args[1] = { 0 };
    struct RDArgs *rda = ReadArgs("HANDLER/K", args, NULL);
    CONST_STRPTR handler;
    BPTR lock;
    LONG w;
    unsigned i;

    if (!rda)
    {
        PrintFault(IoErr(), "notify");
        return RETURN_FAIL;
    }
    handler = (CONST_STRPTR)args[0];

    /* what was done shows also when something stops the test */
    setvbuf(stdout, NULL, _IONBF, 0);

    /* No "insert volume" requesters while probing for the volume */
    me->pr_WindowPtr = (APTR)-1;

    if (exists(DEVICE) || exists(LABEL ":") || exists("FDSK:") || exists(DIR))
        printf("FAIL: " DEVICE ", " LABEL ":, FDSK: or " DIR " already exists; remove it first\n");
    else if (handler && !exists(handler))
        printf("FAIL: handler %s not found\n", (const char *)handler);
    else
    {
        BOOL have = TRUE;

        for (w = 0; w < WATCHES; w++)
        {
            if ((signals[w] = AllocSignal(-1)) >= 0)
                all_signals |= 1UL << signals[w];
            else
                have = FALSE;
        }
        lock = CreateDir(DIR);
        if (!have || !lock)
            printf("FAIL: cannot allocate signals or create " DIR "\n");
        else
        {
            UnLock(lock);
            test_reference();
            reference = TRUE;
            by_message(REFERENCE, "RAM:, by message:", FALSE);
            reference = FALSE;
            if (run("Assign FDSK: " DIR) != 0)
            {
                printf("FAIL: cannot assign FDSK: to " DIR "\n");
                fails++;
            }
            else
            {
                for (i = 0; i < sizeof(formats) / sizeof(formats[0]); i++)
                    test_format(&formats[i], handler);
                run("Assign FDSK: REMOVE");
            }
        }
        for (w = 0; w < WATCHES; w++)
            if (signals[w] >= 0)
                FreeSignal(signals[w]);
        if (run("Delete " DIR " ALL QUIET") != 0)
            printf("NOTE: could not delete " DIR "\n");
        printf("notify: %ld checks, %ld failed\n", (long)checks, (long)fails);
        printf("%s\n", fails ? "FAIL" : "PASS");
    }

    me->pr_WindowPtr = window;
    FreeArgs(rda);
    return fails || checks == 0 ? RETURN_FAIL : RETURN_OK;
}
