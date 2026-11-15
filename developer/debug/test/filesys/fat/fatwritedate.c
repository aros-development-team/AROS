/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Modification dates of files written on a FAT volume.

          Writing to a file, replacing its contents or changing its length
          must set its modification date to the current time; opening it
          without writing must not, and a date set with SetFileDate() must
          stay until the contents change again. The test writes an empty
          FAT32 image to RAM:, mounts it through fdsk.device as
          FATWRITEDATE:, gives each test file a date three days in the past
          with SetFileDate(), modifies it in one way, and compares the date
          Examine() reports with the time of the change. A date still three
          days old means it was not updated; FAT stores dates in two-second
          steps, so a date within a few seconds of the change counts as
          current. The volume is then stopped and mounted again and the
          dates are checked once more against what was written (a handler
          that does not write its cache when inhibited, as before #2132,
          loses the files there and fails this part). Last, a file is locked
          under the FIRST handler, the volume is stopped and mounted with the
          HANDLER one, which adopts the lock from the other instance
          (DoDiskInsert()). Examine() through the adopted lock, SetFileDate()
          on the file while it is held and UnLock() must work, and a later
          write must date the file. At the end
          the volume is dismounted and its files are deleted, so the test
          can be run again.

          Needs fdsk.device and about 41 MB of free memory (the image is
          kept in RAM:).

    Usage: fatwritedate [HANDLER <path>] [FIRST <path>]

          HANDLER mounts the image with that handler file instead of the
          system's FAT handler, to check a newly built one without
          installing it. FIRST is the handler file that creates the lock
          the HANDLER instance adopts (default: the system's FAT handler).
*/

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <stdio.h>
#include <string.h>

#define DIR          "RAM:fatwritedate"
#define IMAGE        DIR "/Unit0"
#define MOUNTLIST    DIR "/Mountlist"
#define VOLUME       "FATWRITEDATE:"

#define SECTOR       512
#define SECTORS      81920              /* 40 MB, 2560 tracks of 32 sectors */
#define RESERVED     32
#define FATS         2
#define FATSECTORS   640                /* 80608 clusters need 630 */

#define OLD_DAYS     3                  /* the date given before each change */
#define SLACK        4                  /* seconds: FAT's two-second steps, rounded up */

static void le16(UBYTE *p, UWORD v) { p[0] = v; p[1] = v >> 8; }
static void le32(UBYTE *p, ULONG v) { p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24; }

static void boot_sector(UBYTE *s)
{
    memset(s, 0, SECTOR);
    s[0] = 0xeb; s[1] = 0x58; s[2] = 0x90;
    memcpy(s + 3, "MSWIN4.1", 8);
    le16(s + 11, SECTOR);
    s[13] = 1;                          /* sectors per cluster */
    le16(s + 14, RESERVED);
    s[16] = FATS;
    s[21] = 0xf8;
    le16(s + 24, 32);                   /* sectors per track */
    le16(s + 26, 1);                    /* heads */
    le32(s + 32, SECTORS);
    le32(s + 36, FATSECTORS);
    le32(s + 44, 2);                    /* root directory cluster */
    le16(s + 48, 1);                    /* FSInfo sector */
    le16(s + 50, 6);                    /* backup boot sector */
    s[64] = 0x80;
    s[66] = 0x29;
    le32(s + 67, 0x57444154);
    memcpy(s + 71, "WRITEDATE  ", 11);
    memcpy(s + 82, "FAT32   ", 8);
    s[510] = 0x55; s[511] = 0xaa;
}

static void fsinfo_sector(UBYTE *s)
{
    memset(s, 0, SECTOR);
    le32(s, 0x41615252);
    le32(s + 484, 0x61417272);
    le32(s + 488, 0xffffffff);          /* free count unknown */
    le32(s + 492, 0xffffffff);          /* next free unknown */
    le32(s + 508, 0xaa550000);
}

#define CHUNK        128                /* sectors per Write() */

/* One sector of the image: boot, FSInfo, backup boot, the FATs (cluster 2 is
   the root, one cluster long), everything else zero (an empty root) */
static void image_sector(ULONG i, UBYTE *s)
{
    memset(s, 0, SECTOR);
    if (i == 0 || i == 6)
        boot_sector(s);
    else if (i == 1 || i == 7)
        fsinfo_sector(s);
    else if (i == RESERVED || i == RESERVED + FATSECTORS)
    {
        le32(s, 0x0ffffff8);
        le32(s + 4, 0x0fffffff);
        le32(s + 8, 0x0fffffff);    /* root: end of chain */
    }
}

static BOOL write_image(void)
{
    UBYTE *s = AllocMem(SECTOR * CHUNK, MEMF_ANY);
    BPTR fh;
    ULONG i, j;
    BOOL ok = TRUE;

    if (!s)
        return FALSE;
    fh = Open(IMAGE, MODE_NEWFILE);
    if (!fh)
    {
        FreeMem(s, SECTOR * CHUNK);
        return FALSE;
    }
    for (i = 0; ok && i < SECTORS; i += CHUNK)
    {
        for (j = 0; j < CHUNK; j++)
            image_sector(i + j, s + j * SECTOR);
        ok = Write(fh, s, SECTOR * CHUNK) == SECTOR * CHUNK;
    }
    if (!Close(fh))
        ok = FALSE;
    FreeMem(s, SECTOR * CHUNK);
    return ok;
}

static BOOL write_mountlist(CONST_STRPTR handler)
{
    static const char text[] =
        "    Device         = fdsk.device\n"
        "    Unit           = 0\n"
        "    DosType        = 0x46415402\n"
        "    SectorSize     = 512\n"
        "    Surfaces       = 1\n"
        "    BlocksPerTrack = 32\n"
        "    Reserved       = 0\n"
        "    LowCyl         = 0\n"
        "    HighCyl        = 2559\n"
        "    Buffers        = 20\n"
        "    StackSize      = 16384\n"
        "    Activate       = 1\n"
        "#\n";
    BPTR fh = Open(MOUNTLIST, MODE_NEWFILE);
    BOOL ok;

    if (!fh)
        return FALSE;
    ok = FPuts(fh, "FATWRITEDATE:\n") == 0;
    if (ok && handler)
        ok = FPuts(fh, "    FileSystem     = ") == 0 && FPuts(fh, handler) == 0 && FPuts(fh, "\n") == 0;
    ok = ok && FPuts(fh, text) == 0;
    return Close(fh) && ok;
}

static BOOL exists(CONST_STRPTR name)
{
    BPTR lock = Lock(name, SHARED_LOCK);

    if (lock)
        UnLock(lock);
    return lock != BNULL;
}

static LONG run(CONST_STRPTR command)
{
    return SystemTags(command, SYS_Output, BNULL, TAG_END);
}

static BOOL mount(CONST_STRPTR handler)
{
    return write_mountlist(handler) && run("Mount " VOLUME " FROM " MOUNTLIST) == 0 && exists(VOLUME);
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

static LONG seconds(const struct DateStamp *ds)
{
    return ds->ds_Days * 86400 + ds->ds_Minute * 60 + ds->ds_Tick / TICKS_PER_SECOND;
}

static LONG now(void)
{
    struct DateStamp ds;

    DateStamp(&ds);
    return seconds(&ds);
}

/* The modification date Examine() reports, in seconds; -1 when it fails */
static LONG file_date(CONST_STRPTR name)
{
    BPTR lock = Lock(name, SHARED_LOCK);
    struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
    LONG s = -1;

    if (lock && fib && Examine(lock, fib))
        s = seconds(&fib->fib_Date);
    if (fib)
        FreeDosObject(DOS_FIB, fib);
    if (lock)
        UnLock(lock);
    return s;
}

static LONG old_date;                   /* seconds, even */

static BOOL set_old_date(CONST_STRPTR name)
{
    struct DateStamp ds;

    ds.ds_Days = old_date / 86400;
    ds.ds_Minute = old_date % 86400 / 60;
    ds.ds_Tick = old_date % 60 * TICKS_PER_SECOND;
    return SetFileDate(name, &ds);
}

static BOOL put(CONST_STRPTR name, CONST_STRPTR text)
{
    BPTR fh = Open(name, MODE_NEWFILE);
    LONG len = strlen(text);
    BOOL ok;

    if (!fh)
        return FALSE;
    ok = Write(fh, (APTR)text, len) == len;
    return Close(fh) && ok;
}

static BOOL write_at(BPTR fh, LONG pos, LONG mode, CONST_STRPTR text)
{
    LONG len = strlen(text);

    return Seek(fh, pos, mode) != -1 && Write(fh, (APTR)text, len) == len;
}

#define CASES 8

/* What each case leaves: the current time of its change, or the old date */
static const struct
{
    const char *name;
    const char *what;
    BOOL current;
} cases[CASES] =
{
    { VOLUME "setdate",   "SetFileDate() alone",                              FALSE },
    { VOLUME "replace",   "replaced (MODE_NEWFILE), same length",             TRUE  },
    { VOLUME "inplace",   "overwritten in place (MODE_READWRITE), same length", TRUE },
    { VOLUME "append",    "made longer by a write at its end",                TRUE  },
    { VOLUME "shorten",   "made shorter with SetFileSize()",                  TRUE  },
    { VOLUME "unchanged", "opened for writing and reading, nothing written",  FALSE },
    { VOLUME "dateafter", "written, then SetFileDate() while still open",     FALSE },
    { VOLUME "writeafter", "written, SetFileDate(), written again, closed",   TRUE  },
};

static LONG changed_at[CASES][2];       /* time before and after the change */
static BOOL done[CASES];                /* prepared and changed */

/* Performs case i on its file, which holds 16 bytes and the old date */
static BOOL change(LONG i)
{
    CONST_STRPTR name = cases[i].name;
    UBYTE buf[16];
    BPTR fh;
    BOOL ok = TRUE;

    switch (i)
    {
    case 0:
        return TRUE;
    case 1:
        return put(name, "BCDEFGHIJKLMNOPQ");
    case 5:
        if (!(fh = Open(name, MODE_READWRITE)))
            return FALSE;
        if (!Close(fh) || !(fh = Open(name, MODE_OLDFILE)))
            return FALSE;
        ok = Read(fh, buf, sizeof(buf)) == sizeof(buf);
        return Close(fh) && ok;
    }
    if (!(fh = Open(name, MODE_READWRITE)))
        return FALSE;
    switch (i)
    {
    case 2:
        ok = write_at(fh, 0, OFFSET_BEGINNING, "abcdefghijklmnop");
        break;
    case 3:
        ok = write_at(fh, 0, OFFSET_END, "0123456789");
        break;
    case 4:
        ok = SetFileSize(fh, 8, OFFSET_BEGINNING) == 8;
        break;
    case 6:
        ok = write_at(fh, 0, OFFSET_BEGINNING, "abcd") && set_old_date(name);
        break;
    case 7:
        ok = write_at(fh, 0, OFFSET_BEGINNING, "abcd") && set_old_date(name)
            && write_at(fh, 4, OFFSET_BEGINNING, "efgh");
        break;
    }
    return Close(fh) && ok;
}

/* Compares the date of case i with what it should be. 0 or 1 failure */
static LONG check(LONG i, CONST_STRPTR when)
{
    LONG d = file_date(cases[i].name);

    if (d < 0)
    {
        printf("FAIL: %s (%s): cannot examine %s: error %ld\n", cases[i].what, when, cases[i].name,
            (long)IoErr());
        return 1;
    }
    if (!cases[i].current)
    {
        if (d < old_date - SLACK || d > old_date + SLACK)
        {
            printf("FAIL: %s (%s): date moved from the one set by %ld s\n", cases[i].what, when,
                (long)(d - old_date));
            return 1;
        }
        return 0;
    }
    if (d >= old_date - SLACK && d <= old_date + SLACK)
    {
        printf("FAIL: %s (%s): date not updated, still the one set %ld days before\n", cases[i].what,
            when, (long)OLD_DAYS);
        return 1;
    }
    if (d < changed_at[i][0] - SLACK || d > changed_at[i][1] + SLACK)
    {
        printf("FAIL: %s (%s): date %ld s from the time of the change\n", cases[i].what, when,
            (long)(d < changed_at[i][0] ? d - changed_at[i][0] : d - changed_at[i][1]));
        return 1;
    }
    return 0;
}

#define ADOPTED VOLUME "adopted"

/* Locks ADOPTED under the first handler, stops it and mounts the volume with
   the second, which adopts the lock (DoDiskInsert), uses the lock, sets the
   file's date while it is held, releases it and writes the file. 0 or the
   number of failures */
static LONG adopted(CONST_STRPTR first, CONST_STRPTR handler)
{
    struct FileInfoBlock *fib;
    BPTR lock, fh;
    LONG t0, t1, d, fails = 0;
    BOOL ok;

    if (!mount(first))
    {
        printf("FAIL: adopted lock: cannot mount " VOLUME " with the first handler\n");
        return 1;
    }
    if (!put(ADOPTED, "ABCDEFGHIJKLMNOP") || !(lock = Lock(ADOPTED, SHARED_LOCK)))
    {
        printf("FAIL: adopted lock: cannot prepare " ADOPTED ": error %ld\n", (long)IoErr());
        unmount();
        return 1;
    }
    unmount();
    if (!mount(handler))
    {
        /* The lock belongs to a stopped handler: it cannot be released */
        printf("FAIL: adopted lock: cannot mount " VOLUME " with the second handler\n");
        return 1;
    }

    fib = AllocDosObject(DOS_FIB, NULL);
    if (!fib || !Examine(lock, fib) || strcmp((char *)fib->fib_FileName, "adopted") != 0)
    {
        printf("FAIL: adopted lock: Examine() through it failed: error %ld\n", (long)IoErr());
        fails++;
    }
    if (fib)
        FreeDosObject(DOS_FIB, fib);
    if (!set_old_date(ADOPTED))
    {
        printf("FAIL: adopted lock: SetFileDate() while it is held failed: error %ld\n", (long)IoErr());
        fails++;
    }
    else
    {
        d = file_date(ADOPTED);
        if (d < old_date - SLACK || d > old_date + SLACK)
        {
            printf("FAIL: adopted lock: the date set while it is held is %ld s off\n", (long)(d - old_date));
            fails++;
        }
    }
    UnLock(lock);

    t0 = now();
    ok = (fh = Open(ADOPTED, MODE_READWRITE)) != BNULL;
    if (ok)
    {
        ok = write_at(fh, 0, OFFSET_BEGINNING, "abcd");
        if (!Close(fh))
            ok = FALSE;
    }
    t1 = now();
    if (!ok)
    {
        printf("FAIL: adopted lock: writing the file after releasing it failed: error %ld\n", (long)IoErr());
        unmount();
        return fails + 1;
    }
    d = file_date(ADOPTED);
    unmount();
    if (d >= old_date - SLACK && d <= old_date + SLACK)
    {
        printf("FAIL: written after an adopted lock: date not updated, still the one set %ld days before\n",
            (long)OLD_DAYS);
        fails++;
    }
    else if (d < t0 - SLACK || d > t1 + SLACK)
    {
        printf("FAIL: written after an adopted lock: date %ld s from the time of the change\n",
            (long)(d < t0 ? d - t0 : d - t1));
        fails++;
    }
    return fails;
}

static int test(CONST_STRPTR handler, CONST_STRPTR first);

int main(void)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR window = me->pr_WindowPtr;
    IPTR args[2] = { 0, 0 };
    struct RDArgs *rda = ReadArgs("HANDLER/K,FIRST/K", args, NULL);
    int rc;

    if (!rda)
    {
        PrintFault(IoErr(), "fatwritedate");
        return RETURN_FAIL;
    }
    /* No "insert volume" requesters while probing for the volume */
    me->pr_WindowPtr = (APTR)-1;
    rc = test((CONST_STRPTR)args[0], (CONST_STRPTR)args[1]);
    me->pr_WindowPtr = window;
    FreeArgs(rda);
    return rc;
}

static int test(CONST_STRPTR handler, CONST_STRPTR first)
{
    LONG fails = 0, i;
    BPTR lock;

    if (exists(VOLUME) || exists("FDSK:"))
    {
        printf("FAIL: " VOLUME " or FDSK: already exists; remove it first\n");
        return RETURN_FAIL;
    }
    lock = CreateDir(DIR);
    if (lock)
        UnLock(lock);
    if ((handler && !exists(handler)) || (first && !exists(first)))
    {
        printf("FAIL: handler %s not found\n", handler && !exists(handler) ? handler : first);
        return RETURN_FAIL;
    }
    if (!write_image())
    {
        printf("FAIL: cannot write the image to " DIR " (needs about 41 MB)\n");
        run("Delete " DIR " ALL QUIET");
        return RETURN_FAIL;
    }
    if (run("Assign FDSK: " DIR) != 0 || !mount(handler))
    {
        printf("FAIL: cannot mount " VOLUME " through fdsk.device\n");
        run("Assign FDSK: REMOVE");
        run("Delete " DIR " ALL QUIET");
        return RETURN_FAIL;
    }

    old_date = (now() - OLD_DAYS * 86400) & ~1;
    for (i = 0; i < CASES; i++)
    {
        if (!put(cases[i].name, "ABCDEFGHIJKLMNOP") || !set_old_date(cases[i].name))
        {
            printf("FAIL: cannot prepare %s: error %ld\n", cases[i].name, (long)IoErr());
            fails++;
            continue;
        }
        changed_at[i][0] = now();
        if (!change(i))
        {
            printf("FAIL: %s: the change itself failed: error %ld\n", cases[i].what, (long)IoErr());
            fails++;
            continue;
        }
        changed_at[i][1] = now();
        done[i] = TRUE;
        fails += check(i, "after the change");
    }

    /* The same against what was written: stop the handler, which must write
       its cache first, and mount again */
    unmount();
    if (!mount(handler))
    {
        printf("FAIL: cannot mount " VOLUME " again\n");
        fails++;
    }
    else
    {
        for (i = 0; i < CASES; i++)
            if (done[i])
                fails += check(i, "after mounting again");
        unmount();
        fails += adopted(first, handler);
    }

    run("Assign FDSK: REMOVE");
    if (run("Delete " DIR " ALL QUIET") != 0)
        printf("NOTE: could not delete " DIR "\n");

    printf("%s\n", fails ? "FAIL" : "PASS");
    return fails ? RETURN_FAIL : RETURN_OK;
}
