/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Creating an entry in a FAT32 directory that is exactly full.

          FAT allows a directory whose clusters are completely used to end
          without an end-of-directory entry; other systems create such
          directories, AROS itself always leaves one. The test writes a
          small FAT32 image whose root directory is one cluster holding
          exactly 16 short-name entries and an unused data area that is not
          zero, mounts it through fdsk.device as FATFULLROOT:, creates a
          drawer, files with long names (one across a cluster boundary) and
          short names in the root, inhibits and stops the handler and mounts
          the volume again, so that the checks see what was written, and
          checks every name and that the root holds no extra entry. It then dismounts the volume and
          deletes its files, so it can be run again.

          Needs fdsk.device and about 41 MB of free memory (the image is
          kept in RAM:).

    Usage: fatfullroot
*/

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <stdio.h>
#include <string.h>

#define DIR          "RAM:fatfullroot"
#define IMAGE        DIR "/Unit0"
#define MOUNTLIST    DIR "/Mountlist"
#define VOLUME       "FATFULLROOT:"

#define SECTOR       512
#define SECTORS      81920              /* 40 MB, 2560 tracks of 32 sectors */
#define RESERVED     32
#define FATS         2
#define FATSECTORS   640                /* 80608 clusters need 630 */
#define ROOTSECTOR   (RESERVED + FATS * FATSECTORS)
#define ENTRIES      (SECTOR / 32)      /* one 512 byte cluster is full at 16 */

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
    le32(s + 67, 0x46524f4f);
    memcpy(s + 71, "FULLROOT   ", 11);
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
#define FILL         0xa5               /* unused data area: not zero, so that
                                           stale data in a new directory cluster
                                           would show up as entries */

/* One sector of the image: boot, FSInfo, backup boot, the FATs (cluster 2 is
   the root, one cluster long), the root with ENTRIES empty files, and the
   data area after it filled with FILL. */
static void image_sector(ULONG i, UBYTE *s)
{
    memset(s, i > ROOTSECTOR ? FILL : 0, SECTOR);
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
    else if (i == ROOTSECTOR)
    {
        ULONG e;

        for (e = 0; e < ENTRIES; e++)
        {
            UBYTE *d = s + e * 32;

            memcpy(d, "F00     TXT", 11);
            d[1] = '0' + (e + 1) / 10;
            d[2] = '0' + (e + 1) % 10;
            d[11] = 0x20;           /* archive; size and cluster 0 */
        }
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

static BOOL write_mountlist(void)
{
    static const char text[] =
        "FATFULLROOT:\n"
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
    ok = Write(fh, (APTR)text, sizeof(text) - 1) == sizeof(text) - 1;
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

/*
 * Inhibits the volume, so that the handler lets it go together with the locks
 * and notifications other programs (Wanderer) hold on it, then stops the
 * handler, which closes fdsk.device and with it the image.
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
            return;
        err = DoPkt(dvp->dvp_Port, ACTION_DIE, 0, 0, 0, 0, 0) ? 0 : IoErr();
        FreeDeviceProc(dvp);
        if (!err)
            return;
        Delay(25);
    }
    printf("NOTE: the handler did not stop: error %ld\n", (long)err);
}

/* Entries in the root, counted with ExNext: anything a stale cluster left
   would show up as an extra entry */
static LONG count_root(void)
{
    BPTR lock = Lock(VOLUME, SHARED_LOCK);
    struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
    LONG n = -1;

    if (lock && fib && Examine(lock, fib))
        for (n = 0; ExNext(lock, fib); n++)
            ;
    if (fib)
        FreeDosObject(DOS_FIB, fib);
    if (lock)
        UnLock(lock);
    return n;
}

#define LONGNAMES 4
/* 33 characters: three long-name entries and the short one, 4 in all; after
   NEWDIR (entry 16) the fourth name takes entries 29 to 32 and so crosses
   from the first added cluster (16-31) into the next */
#define LONGNAME VOLUME "Long name crossing clusters %ld.txt"

static BOOL mount(void)
{
    return run("Mount " VOLUME " FROM " MOUNTLIST) == 0 && exists(VOLUME);
}

static int test(void);

int main(void)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR window = me->pr_WindowPtr;
    int rc;

    /* No "insert volume" requesters while probing for the volume */
    me->pr_WindowPtr = (APTR)-1;
    rc = test();
    me->pr_WindowPtr = window;
    return rc;
}

static int test(void)
{
    char name[80];
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
    if (!write_image() || !write_mountlist())
    {
        printf("FAIL: cannot write the image to " DIR " (needs about 41 MB)\n");
        run("Delete " DIR " ALL QUIET");
        return RETURN_FAIL;
    }
    if (run("Assign FDSK: " DIR) != 0 || run("Mount " VOLUME " FROM " MOUNTLIST) != 0 || !exists(VOLUME))
    {
        printf("FAIL: cannot mount " VOLUME " through fdsk.device\n");
        run("Assign FDSK: REMOVE");
        run("Delete " DIR " ALL QUIET");
        return RETURN_FAIL;
    }

    /* The drawer needs the end-of-directory entry first */
    lock = CreateDir(VOLUME "NEWDIR");
    if (lock)
        UnLock(lock);
    else
    {
        printf("FAIL: creating a drawer in the full root: error %ld\n", (long)IoErr());
        fails++;
    }
    /* Long names, the last one across a cluster boundary */
    for (i = 1; i <= LONGNAMES; i++)
    {
        BPTR fh;

        snprintf(name, sizeof(name), LONGNAME, (long)i);
        fh = Open(name, MODE_NEWFILE);
        if (fh)
            Close(fh);
        else
        {
            printf("FAIL: creating %s: error %ld\n", name, (long)IoErr());
            fails++;
            break;
        }
    }
    /* More entries than fit in the new cluster */
    for (i = 1; i <= ENTRIES + 4; i++)
    {
        BPTR fh;

        snprintf(name, sizeof(name), VOLUME "N%02ld.TXT", (long)i);
        fh = Open(name, MODE_NEWFILE);
        if (fh)
            Close(fh);
        else
        {
            printf("FAIL: creating %s: error %ld\n", name, (long)IoErr());
            fails++;
            break;
        }
    }
    /* Check what was written, not the handler's cache: inhibit and stop
       the handler, which must write its cache first, and mount again */
    unmount();
    run("Assign " VOLUME " DISMOUNT");
    if (!mount())
    {
        printf("FAIL: cannot mount " VOLUME " again\n");
        run("Assign FDSK: REMOVE");
        run("Delete " DIR " ALL QUIET");
        return RETURN_FAIL;
    }
    for (i = 1; i <= LONGNAMES; i++)
    {
        snprintf(name, sizeof(name), LONGNAME, (long)i);
        if (!exists(name))
        {
            printf("FAIL: %s not found\n", name);
            fails++;
        }
    }
    i = count_root();
    if (i != ENTRIES + 1 + LONGNAMES + ENTRIES + 4)
    {
        printf("FAIL: %ld entries in the root, wanted %ld\n", (long)i, (long)(ENTRIES + 1 + LONGNAMES + ENTRIES + 4));
        fails++;
    }
    for (i = 1; i <= ENTRIES; i++)
    {
        snprintf(name, sizeof(name), VOLUME "F%02ld.TXT", (long)i);
        if (!exists(name))
        {
            printf("FAIL: %s is gone\n", name);
            fails++;
        }
    }
    if (!exists(VOLUME "NEWDIR") || !exists(VOLUME "N20.TXT"))
    {
        printf("FAIL: new entries not found\n");
        fails++;
    }

    unmount();
    run("Assign " VOLUME " DISMOUNT");
    run("Assign FDSK: REMOVE");
    if (run("Delete " DIR " ALL QUIET") != 0)
        printf("NOTE: could not delete " DIR "\n");

    printf("%s\n", fails ? "FAIL" : "PASS");
    return fails ? RETURN_FAIL : RETURN_OK;
}
