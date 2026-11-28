/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Notification on a directory for changes to its contents.

          A notification request on a directory must be sent when something
          in that directory changes: a file is created, written and closed,
          truncated, deleted, renamed within it, out of it or into it, its
          protection or date is set, or a directory is created in it. A
          change in a sibling directory or in a subdirectory must not send
          it. The test watches a directory W with a signal, does each of
          those things and checks which watches signalled.

          It runs on RAM: first, for comparison, then on FAT12, FAT16 and
          FAT32 images it writes to RAM:dirnotify and mounts through
          fdsk.device as DIRNOTIFY:, one at a time. On the FAT volumes W is
          watched twice, once while nothing holds a lock on it and once
          while the test holds one (the handler finds the two differently),
          and the volume root is watched as well (FAT12/16 have a fixed root
          directory, FAT32 a root cluster). Each image is dismounted and
          deleted afterwards, so the test can be run again. Only the FAT
          results decide the outcome: where RAM: differs it is noted (its
          handler does not notify the directory a rename takes an object
          out of).

          Needs fdsk.device and about 41 MB of free memory for the FAT32
          image.

    Usage: dirnotify [HANDLER <path>]

          HANDLER mounts the images with that handler file instead of the
          system's FAT handler. Hosted builds have no FAT handler for the
          DosType, so pass HANDLER L:fat-handler there.
*/

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/notify.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <stdio.h>
#include <string.h>

#define DIR         "RAM:dirnotify"
#define IMAGE       DIR "/Unit0"
#define MOUNTLIST   DIR "/Mountlist"
#define VOLUME      "DIRNOTIFY:"
#define REFERENCE   DIR "/ram/"       /* the tree on RAM:, in DIR "/ram" */

#define SECTOR      512
#define CHUNK       128                 /* sectors per Write() */

struct Format
{
    unsigned bits;
    ULONG sectors;
    UWORD reserved;
    UWORD fat_sectors;
    UWORD root_entries;
    ULONG dos_type;
};

static const struct Format formats[] =
{
    { 12, 2880,  1,   9, 224, 0x46415400 },   /* 2847 clusters */
    { 16, 16384, 1,  64, 512, 0x46415401 },   /* 16223 clusters */
    { 32, 81920, 32, 640,   0, 0x46415402 }   /* 80608 clusters */
};

static const struct Format *format;
static LONG checks, fails;
static BOOL reference;                  /* differences are noted, not failed */

static void le16(UBYTE *p, UWORD v) { p[0] = v; p[1] = v >> 8; }
static void le32(UBYTE *p, ULONG v) { p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24; }

/* Image ------------------------------------------------------------------ */

static void boot_sector(UBYTE *s)
{
    s[0] = 0xeb;
    s[1] = format->bits == 32 ? 0x58 : 0x3c;
    s[2] = 0x90;
    memcpy(s + 3, "AROSTEST", 8);
    le16(s + 11, SECTOR);
    s[13] = 1;                          /* sectors per cluster */
    le16(s + 14, format->reserved);
    s[16] = 2;                          /* FATs */
    le16(s + 17, format->root_entries);
    s[21] = 0xf8;
    le16(s + 24, 32);                   /* sectors per track */
    le16(s + 26, 1);                    /* heads */
    if (format->bits == 32)
    {
        le32(s + 32, format->sectors);
        le32(s + 36, format->fat_sectors);
        le32(s + 44, 2);                /* root directory cluster */
        le16(s + 48, 1);                /* FSInfo sector */
        le16(s + 50, 6);                /* backup boot sector */
        s[64] = 0x80;
        s[66] = 0x29;
        le32(s + 67, 0x444e3332);
        memcpy(s + 71, "DIRNOTIFY  ", 11);
        memcpy(s + 82, "FAT32   ", 8);
    }
    else
    {
        le16(s + 19, format->sectors);
        le16(s + 22, format->fat_sectors);
        s[36] = 0x80;
        s[38] = 0x29;
        le32(s + 39, 0x444e0000 + format->bits);
        memcpy(s + 43, "DIRNOTIFY  ", 11);
        memcpy(s + 54, format->bits == 12 ? "FAT12   " : "FAT16   ", 8);
    }
    s[510] = 0x55;
    s[511] = 0xaa;
}

static void image_sector(ULONG i, UBYTE *s)
{
    memset(s, 0, SECTOR);
    if (i == 0 || (format->bits == 32 && i == 6))
        boot_sector(s);
    else if (format->bits == 32 && (i == 1 || i == 7))
    {
        le32(s, 0x41615252);
        le32(s + 484, 0x61417272);
        le32(s + 488, 0xffffffff);      /* free count unknown */
        le32(s + 492, 0xffffffff);      /* next free unknown */
        le32(s + 508, 0xaa550000);
    }
    else if (i == format->reserved || i == format->reserved + format->fat_sectors)
    {
        if (format->bits == 32)
        {
            le32(s, 0x0ffffff8);
            le32(s + 4, 0x0fffffff);
            le32(s + 8, 0x0fffffff);    /* cluster 2: the empty root */
        }
        else
        {
            s[0] = 0xf8;
            s[1] = 0xff;
            s[2] = 0xff;
            if (format->bits == 16)
                s[3] = 0xff;
        }
    }
}

static BOOL write_image(void)
{
    UBYTE *s = AllocMem(SECTOR * CHUNK, MEMF_ANY);
    BPTR fh;
    ULONG i, j, count;
    BOOL ok = TRUE;

    if (!s)
        return FALSE;
    fh = Open(IMAGE, MODE_NEWFILE);
    if (!fh)
    {
        FreeMem(s, SECTOR * CHUNK);
        return FALSE;
    }
    for (i = 0; ok && i < format->sectors; i += count)
    {
        count = format->sectors - i;
        if (count > CHUNK)
            count = CHUNK;
        for (j = 0; j < count; j++)
            image_sector(i + j, s + j * SECTOR);
        ok = Write(fh, s, count * SECTOR) == (LONG)(count * SECTOR);
    }
    if (!Close(fh))
        ok = FALSE;
    FreeMem(s, SECTOR * CHUNK);
    return ok;
}

static BOOL write_mountlist(CONST_STRPTR handler)
{
    BPTR fh = Open(MOUNTLIST, MODE_NEWFILE);
    BOOL ok;

    if (!fh)
        return FALSE;
    ok = FPuts(fh, VOLUME "\n") == 0;
    if (ok && handler)
        ok = FPrintf(fh, "    FileSystem     = %s\n", (IPTR)handler) >= 0;
    ok = ok && FPrintf(fh,
        "    Device         = fdsk.device\n"
        "    Unit           = 0\n"
        "    DosType        = 0x%08lx\n"
        "    SectorSize     = 512\n"
        "    Surfaces       = 1\n"
        "    BlocksPerTrack = 32\n"
        "    Reserved       = 0\n"
        "    LowCyl         = 0\n"
        "    HighCyl        = %ld\n"
        "    Buffers        = 20\n"
        "    StackSize      = 16384\n"
        "    Activate       = 1\n"
        "#\n", (IPTR)format->dos_type, (IPTR)(format->sectors / 32 - 1)) >= 0;
    return Close(fh) && ok;
}

static BOOL exists(CONST_STRPTR name)
{
    BPTR lock = Lock(name, SHARED_LOCK);

    if (lock)
        UnLock(lock);
    return lock != BNULL;
}

#define CMDLOG      "RAM:dirnotify.cmdlog"

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

/*
 * Inhibits the volume, so that the handler lets it go together with the locks
 * and notifications other programs (Wanderer) hold on it, then stops the
 * handler, which writes its cache and closes fdsk.device with the image.
 */
static void unmount(void)
{
    struct DevProc *dvp;
    LONG i, err = 0;

    if (!Inhibit(VOLUME, DOSTRUE))
        printf("NOTE: could not inhibit " VOLUME ": error %ld\n", (long)IoErr());
    for (i = 0; i < 10; i++)
    {
        dvp = GetDeviceProc(VOLUME, NULL);
        if (!dvp)
            break;
        err = DoPkt(dvp->dvp_Port, ACTION_DIE, 0, 0, 0, 0, 0) ? 0 : IoErr();
        FreeDeviceProc(dvp);
        if (!err)
            break;
        Delay(25);
    }
    if (err)
        printf("NOTE: the handler did not stop: error %ld\n", (long)err);
    run("Assign " VOLUME " DISMOUNT");
}

/* Watches ---------------------------------------------------------------- */

#define WATCHES 2                       /* W, and the root of the volume */

static struct NotifyRequest requests[WATCHES];
static BYTE signals[WATCHES] = { -1, -1 };
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
        printf("FAIL: StartNotify(%s): error %ld\n", name, (long)IoErr());
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
 * Does one operation and checks which watches signalled: want has a bit per
 * watch that must signal; every other watch that is active must not.
 */
static void expect(BOOL done, ULONG want, CONST_STRPTR what, CONST_STRPTR how)
{
    ULONG got = signalled(), active = 0;
    LONG w;

    for (w = 0; w < WATCHES; w++)
        if (watching[w])
            active |= 1 << w;
    want &= active;
    got &= active;
    checks++;
    if (!done)
    {
        printf("FAIL: %s %s: the operation failed: error %ld\n", how, what, (long)IoErr());
        fails++;
    }
    else if (got != want)
    {
        printf("%s: %s %s: %s%s%s%s\n", reference ? "NOTE" : "FAIL", how, what,
            (want & ~got & 1) ? "W not notified " : "",
            (want & ~got & 2) ? "root not notified " : "",
            (got & ~want & 1) ? "W notified wrongly " : "",
            (got & ~want & 2) ? "root notified wrongly " : "");
        if (!reference)
            fails++;
    }
}

#define WD 1                            /* bit for the watch on W */
#define RT 2                            /* bit for the watch on the root */

/*
 * Every change in W, one in its subdirectory Sub and one in its sibling
 * Other. base is the volume or directory that holds W and Other; root says
 * whether base is the volume root (then changes there notify the root watch).
 */
static void changes_in_w(CONST_STRPTR base, CONST_STRPTR how, BOOL root)
{
    char w[100], other[100];
    ULONG in_base = root ? RT : 0;

    snprintf(w, sizeof(w), "%sW/", (const char *)base);
    snprintf(other, sizeof(other), "%sOther/", (const char *)base);

    expect(put(p(w, "new"), NULL, MODE_NEWFILE), WD, "create and close a file", how);
    expect(put(p(w, "new"), "data", MODE_READWRITE), WD, "write and close a file", how);
    expect(append(p(w, "new"), "more"), WD, "append to a file", how);
    expect(put(p(w, "new"), NULL, MODE_NEWFILE), WD, "truncate a file without writing", how);
    expect(SetProtection(p(w, "new"), FIBF_ARCHIVE), WD, "set a file's protection", how);
    expect(set_date(p(w, "new")), WD, "set a file's date", how);
    expect(Rename(p(w, "new"), p(w, "renamed")), WD, "rename a file within W", how);
    expect(Rename(p(w, "renamed"), p(other, "moved")), WD, "rename a file out of W", how);
    expect(Rename(p(other, "moved"), p(w, "back")), WD, "rename a file into W", how);
    expect(DeleteFile(p(w, "back")), WD, "delete a file", how);
    expect(make_dir(p(w, "Dir")), WD, "create a directory", how);
    expect(Rename(p(w, "Dir"), p(base, "Dir")), WD | in_base, "move a directory to W's parent", how);
    expect(Rename(p(base, "Dir"), p(w, "Dir")), WD | in_base, "move a directory back into W", how);
    expect(DeleteFile(p(w, "Dir")), WD, "delete a directory", how);
    expect(put(p(w, "Sub/inner"), "x", MODE_NEWFILE), 0, "write a file in W's subdirectory", how);
    expect(DeleteFile(p(w, "Sub/inner")), 0, "delete a file in W's subdirectory", how);
    expect(put(p(other, "file"), "x", MODE_NEWFILE), 0, "write a file in W's sibling", how);
    expect(DeleteFile(p(other, "file")), 0, "delete a file in W's sibling", how);
    expect(put(p(base, "top"), "x", MODE_NEWFILE), in_base, "write a file next to W", how);
    expect(DeleteFile(p(base, "top")), in_base, "delete a file next to W", how);
}

static BOOL make_tree(CONST_STRPTR base)
{
    return make_dir(p(base, "W")) && make_dir(p(base, "W/Sub")) && make_dir(p(base, "Other"));
}

/* The same on RAM:, for comparison */
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
    if (start_watch(0, REFERENCE "W"))
        changes_in_w(REFERENCE, "RAM:", FALSE);
    end_watches();
    reference = FALSE;
}

static void test_format(CONST_STRPTR handler)
{
    char how[40];
    BPTR lock;

    printf("FAT%u\n", format->bits);
    if (!write_image())
    {
        printf("FAIL: cannot write the FAT%u image to " DIR "\n", format->bits);
        fails++;
        return;
    }
    if (!write_mountlist(handler) || run("Mount " VOLUME " FROM " MOUNTLIST) != 0 || !exists(VOLUME))
    {
        printf("FAIL: cannot mount the FAT%u image through fdsk.device\n", format->bits);
        fails++;
        run("Assign " VOLUME " DISMOUNT");
        DeleteFile(IMAGE);
        return;
    }
    if (!make_tree(VOLUME))
    {
        printf("FAIL: cannot create the test tree on FAT%u\n", format->bits);
        fails++;
    }
    else
    {
        /* W and the root not locked: the handler finds them by name */
        snprintf(how, sizeof(how), "FAT%u, not locked:", format->bits);
        if (start_watch(0, VOLUME "W") && start_watch(1, VOLUME))
            changes_in_w(VOLUME, how, TRUE);
        end_watches();

        /* The same while a lock is held on each: the handler finds them by
           their lock */
        snprintf(how, sizeof(how), "FAT%u, locked:", format->bits);
        lock = Lock(VOLUME "W", SHARED_LOCK);
        {
            BPTR root = Lock(VOLUME, SHARED_LOCK);

            if (!lock || !root)
            {
                printf("FAIL: cannot lock W and the root on FAT%u\n", format->bits);
                fails++;
            }
            else if (start_watch(0, VOLUME "W") && start_watch(1, VOLUME))
                changes_in_w(VOLUME, how, TRUE);
            end_watches();
            if (root)
                UnLock(root);
        }
        if (lock)
            UnLock(lock);
    }
    unmount();
    if (!DeleteFile(IMAGE) || !DeleteFile(MOUNTLIST))
        printf("NOTE: could not delete the FAT%u image\n", format->bits);
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
        PrintFault(IoErr(), "dirnotify");
        return RETURN_FAIL;
    }
    handler = (CONST_STRPTR)args[0];

    /* No "insert volume" requesters while probing for the volume */
    me->pr_WindowPtr = (APTR)-1;

    if (exists(VOLUME) || exists("FDSK:") || exists(DIR))
        printf("FAIL: " VOLUME ", FDSK: or " DIR " already exists; remove it first\n");
    else if (handler && !exists(handler))
        printf("FAIL: handler %s not found\n", handler);
    else
    {
        for (w = 0; w < WATCHES; w++)
            if ((signals[w] = AllocSignal(-1)) >= 0)
                all_signals |= 1UL << signals[w];
        lock = CreateDir(DIR);
        if (signals[0] < 0 || signals[1] < 0 || !lock)
            printf("FAIL: cannot allocate signals or create " DIR "\n");
        else
        {
            UnLock(lock);
            test_reference();
            if (run("Assign FDSK: " DIR) != 0)
            {
                printf("FAIL: cannot assign FDSK: to " DIR "\n");
                fails++;
            }
            else
            {
                for (i = 0; i < sizeof(formats) / sizeof(formats[0]); i++)
                {
                    format = &formats[i];
                    test_format(handler);
                }
                run("Assign FDSK: REMOVE");
            }
        }
        for (w = 0; w < WATCHES; w++)
            if (signals[w] >= 0)
                FreeSignal(signals[w]);
        if (run("Delete " DIR " ALL QUIET") != 0)
            printf("NOTE: could not delete " DIR "\n");
        printf("dirnotify: %ld checks, %ld failed\n", (long)checks, (long)fails);
        printf("%s\n", fails ? "FAIL" : "PASS");
    }

    me->pr_WindowPtr = window;
    FreeArgs(rda);
    return fails || checks == 0 ? RETURN_FAIL : RETURN_OK;
}
