/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Native FAT directory-parent regression for the contract fixed by #2092.
    Run parent_rename without arguments. It creates FAT12, FAT16 and FAT32
    images in RAM:parent_rename, mounts them through fdsk.device and checks
    NameFromLock() and ParentDir() after cross-parent and case-only renames.
    It also checks refusal of cycles, replacement of an existing directory
    and a file used as a destination parent.

    Every scenario inhibits and stops the handler, dismounts and mounts the
    image again before checking the resulting parent links. Inhibit must
    include the cache-flush fix from #2132. The second checks therefore use
    a fresh handler rather than the cache used to perform the rename.

    Needs fdsk.device, Mount, Assign and about 41 MB free memory. Images are
    processed one at a time and removed on success or a safely cleaned-up
    failure. Existing RAM:parent_rename, FATPARENT: or FDSK: entries are
    refused. An image is kept if its handler cannot be stopped safely.

    Adapted from the published kaffeine1/kaffeine parent regression and the
    self-contained RAM:/fdsk.device harness in fatfullroot.c.
*/

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <stdio.h>
#include <string.h>

#define DIRECTORY   "RAM:parent_rename"
#define IMAGE       DIRECTORY "/Unit0"
#define MOUNTLIST   DIRECTORY "/Mountlist"
#define VOLUME      "FATPARENT:"
#define SECTOR      512
#define CHUNK       128

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
    { 12, 2880,  1,   9, 224, 0x46415400 },
    { 16, 16384, 1,  64, 512, 0x46415401 },
    { 32, 81920, 32, 640,   0, 0x46415402 }
};

static const struct Format *format;
static LONG checks, failed;
static BOOL owns_directory, owns_assignment, mounted, device_added;
static char canonical_root[128];

static BOOL check(BOOL ok, const char *description)
{
    checks++;
    if (!ok)
        failed++;
    printf("%s FAT%u: %s\n", ok ? "ok" : "FAIL", format->bits, description);
    return ok;
}

static void le16(UBYTE *p, UWORD value)
{
    p[0] = value;
    p[1] = value >> 8;
}

static void le32(UBYTE *p, ULONG value)
{
    p[0] = value;
    p[1] = value >> 8;
    p[2] = value >> 16;
    p[3] = value >> 24;
}

static void boot_sector(UBYTE *s)
{
    s[0] = 0xeb;
    s[1] = format->bits == 32 ? 0x58 : 0x3c;
    s[2] = 0x90;
    memcpy(s + 3, "AROSTEST", 8);
    le16(s + 11, SECTOR);
    s[13] = 1;
    le16(s + 14, format->reserved);
    s[16] = 2;
    le16(s + 17, format->root_entries);
    s[21] = 0xf8;
    le16(s + 24, 32);
    le16(s + 26, 1);
    if (format->bits == 32)
    {
        le32(s + 32, format->sectors);
        le32(s + 36, format->fat_sectors);
        le32(s + 44, 2);
        le16(s + 48, 1);
        le16(s + 50, 6);
        s[64] = 0x80;
        s[66] = 0x29;
        le32(s + 67, 0x50415220);
        memcpy(s + 71, "PARENT32   ", 11);
        memcpy(s + 82, "FAT32   ", 8);
    }
    else
    {
        le16(s + 19, format->sectors);
        le16(s + 22, format->fat_sectors);
        s[36] = 0x80;
        s[38] = 0x29;
        le32(s + 39, 0x50415200 + format->bits);
        memcpy(s + 43, format->bits == 12 ? "PARENT12   " : "PARENT16   ", 11);
        memcpy(s + 54, format->bits == 12 ? "FAT12   " : "FAT16   ", 8);
    }
    s[510] = 0x55;
    s[511] = 0xaa;
}

static void image_sector(ULONG number, UBYTE *s)
{
    memset(s, 0, SECTOR);
    if (number == 0 || (format->bits == 32 && number == 6))
        boot_sector(s);
    else if (format->bits == 32 && (number == 1 || number == 7))
    {
        le32(s, 0x41615252);
        le32(s + 484, 0x61417272);
        le32(s + 488, 0xffffffff);
        le32(s + 492, 0xffffffff);
        le32(s + 508, 0xaa550000);
    }
    else if (number == format->reserved ||
             number == format->reserved + format->fat_sectors)
    {
        if (format->bits == 32)
        {
            le32(s, 0x0ffffff8);
            le32(s + 4, 0x0fffffff);
            le32(s + 8, 0x0fffffff); /* Cluster 2: empty root directory. */
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
    UBYTE *buffer = AllocMem(SECTOR * CHUNK, MEMF_ANY);
    BPTR file;
    ULONG i, j, count;
    BOOL ok = FALSE;

    if (!buffer)
        return FALSE;
    file = Open((CONST_STRPTR)IMAGE, MODE_NEWFILE);
    if (file)
    {
        ok = TRUE;
        for (i = 0; ok && i < format->sectors; i += count)
        {
            count = format->sectors - i;
            if (count > CHUNK)
                count = CHUNK;
            for (j = 0; j < count; j++)
                image_sector(i + j, buffer + j * SECTOR);
            ok = Write(file, buffer, count * SECTOR) == (LONG)(count * SECTOR);
        }
        if (!Close(file))
            ok = FALSE;
    }
    FreeMem(buffer, SECTOR * CHUNK);
    return ok;
}

static BOOL write_mountlist(void)
{
    BPTR file = Open((CONST_STRPTR)MOUNTLIST, MODE_NEWFILE);
    BOOL ok;

    if (!file)
        return FALSE;
    ok = FPrintf(file,
        (CONST_STRPTR)VOLUME "\n"
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
    return Close(file) && ok;
}

static BOOL dos_entry_exists(const char *name)
{
    ULONG flags = LDF_DEVICES | LDF_VOLUMES | LDF_ASSIGNS | LDF_READ;
    struct DosList *list = LockDosList(flags);
    BOOL found;

    if (!list)
        return TRUE;
    found = FindDosEntry(list, (CONST_STRPTR)name, flags) != NULL;
    UnLockDosList(flags);
    return found;
}

static BOOL absent(const char *path)
{
    BPTR lock = Lock((CONST_STRPTR)path, SHARED_LOCK);
    LONG error = IoErr();

    if (lock)
        UnLock(lock);
    return !lock && error == ERROR_OBJECT_NOT_FOUND;
}

static BOOL create_directory(const char *path)
{
    BPTR lock = CreateDir((CONST_STRPTR)path);

    if (lock)
        UnLock(lock);
    return check(lock != BNULL, path);
}

static BOOL mount_volume(void)
{
    BPTR lock;
    LONG rc = SystemTags((CONST_STRPTR)"Mount " VOLUME " FROM " MOUNTLIST,
                         SYS_Output, BNULL, TAG_END);

    device_added = dos_entry_exists("FATPARENT");
    mounted = device_added; /* Also stop a partially started mount on failure. */
    if (!check(rc == RETURN_OK && device_added, "mount image through fdsk.device"))
        return FALSE;
    lock = Lock((CONST_STRPTR)VOLUME, SHARED_LOCK);
    if (lock)
        UnLock(lock);
    return check(lock != BNULL, "lock mounted volume");
}

/* ACTION_DIE discards the handler's cache; dismount removes its DOS entry. */
static BOOL unmount_volume(void)
{
    struct DevProc *process;
    BOOL stopped;

    if (mounted)
    {
        if (!check(Inhibit(VOLUME, DOSTRUE), "inhibit and flush mounted image"))
            return FALSE;
        process = GetDeviceProc(VOLUME, NULL);
        if (!check(process != NULL, "obtain image handler"))
            return FALSE;
        stopped = DoPkt(process->dvp_Port, ACTION_DIE, 0, 0, 0, 0, 0) != 0;
        FreeDeviceProc(process);
        if (!check(stopped, "stop image handler before remount or removal"))
            return FALSE;
        mounted = FALSE;
    }
    if (device_added)
    {
        if (!check(SystemTags((CONST_STRPTR)"Assign " VOLUME " DISMOUNT", SYS_Output, BNULL,
                              TAG_END) == RETURN_OK, "dismount own DOS device"))
            return FALSE;
        device_added = FALSE;
    }
    return TRUE;
}

static BOOL volume_type(void)
{
    struct InfoData info = {0};
    struct DosList *list, *walk;
    BPTR lock = Lock((CONST_STRPTR)VOLUME, SHARED_LOCK);
    ULONG type = 0;
    BOOL ok;

    if (!check(lock != BNULL, "lock volume for type check"))
        return FALSE;
    ok = Info(lock, &info) && NameFromLock(lock, (STRPTR)canonical_root, sizeof(canonical_root));
    UnLock(lock);
    if (!check(ok && info.id_VolumeNode, "read actual volume node and canonical root"))
        return FALSE;
    list = LockDosList(LDF_VOLUMES | LDF_READ);
    if (list)
    {
        walk = list;
        while ((walk = NextDosEntry(walk, LDF_VOLUMES)))
        {
            if (walk == (struct DosList *)BADDR(info.id_VolumeNode))
            {
                type = walk->dol_misc.dol_volume.dol_DiskType;
                break;
            }
        }
        UnLockDosList(LDF_VOLUMES | LDF_READ);
    }
    printf("volume type=%08lx expected=%08lx\n", (unsigned long)type,
           (unsigned long)format->dos_type);
    return check(type == format->dos_type, "mounted volume has the requested FAT type");
}

static BOOL remount_volume(void)
{
    return unmount_volume() && mount_volume() && volume_type();
}

static BOOL check_parent(const char *child, const char *expected)
{
    static char path[128], parent_path[128], wanted[256], name[256];
    BPTR lock, up = BNULL, parent = BNULL;
    BOOL ok, named;

    snprintf(path, sizeof(path), (CONST_STRPTR)VOLUME "%s", child);
    snprintf(parent_path, sizeof(parent_path), (CONST_STRPTR)VOLUME "%s", expected);
    snprintf(wanted, sizeof(wanted), "%s%s", canonical_root, child);
    lock = Lock((CONST_STRPTR)path, SHARED_LOCK);
    if (!check(lock != BNULL, "child lock exists"))
        return FALSE;
    up = ParentDir(lock);
    parent = Lock((CONST_STRPTR)parent_path, SHARED_LOCK);
    name[0] = 0;
    named = NameFromLock(lock, (STRPTR)name, sizeof(name));
    ok = check(up && parent && SameLock(up, parent) == LOCK_SAME,
               "ParentDir identifies the expected parent");
    if (!check(named && strcmp(name, wanted) == 0,
               "NameFromLock retains the complete canonical path"))
    {
        printf("observe name=%s wanted=%s\n", name, wanted);
        ok = FALSE;
    }
    if (parent)
        UnLock(parent);
    if (up)
        UnLock(up);
    UnLock(lock);
    return ok;
}

static BOOL test_cross_parent(void)
{
    if (!create_directory(VOLUME "Stage") ||
        !create_directory(VOLUME "Stage/Child") ||
        !create_directory(VOLUME "Stage/Child/Leaf") ||
        !create_directory(VOLUME "Other") ||
        !create_directory(VOLUME "Control") ||
        !create_directory(VOLUME "Control/Leaf"))
        return FALSE;
    if (!check_parent("Control/Leaf", "Control"))
        return FALSE;
    if (!check(Rename((CONST_STRPTR)VOLUME "Stage/Child", (CONST_STRPTR)VOLUME "Moved"), "move staging directory to volume root") ||
        !check(absent(VOLUME "Stage/Child"), "old staging name is absent") ||
        !check_parent("Moved/Leaf", "Moved") || !remount_volume() ||
        !check_parent("Moved/Leaf", "Moved"))
        return FALSE;
    return check(Rename((CONST_STRPTR)VOLUME "Moved", (CONST_STRPTR)VOLUME "Other/Child"), "move root directory to another parent") &&
           check_parent("Other/Child/Leaf", "Other/Child") && remount_volume() &&
           check_parent("Other/Child/Leaf", "Other/Child");
}

static BOOL test_descendant(void)
{
    LONG result, error;

    result = Rename((CONST_STRPTR)VOLUME "Other/Child", (CONST_STRPTR)VOLUME "Other/Child/Leaf/Cycle");
    error = IoErr();
    return check(!result && error == ERROR_OBJECT_IN_USE, "refuse moving a directory into its descendant") &&
           check_parent("Other/Child/Leaf", "Other/Child") && remount_volume() &&
           check_parent("Other/Child/Leaf", "Other/Child");
}

static BOOL test_existing_directory(void)
{
    LONG result, error;

    if (!create_directory(VOLUME "Occupied") ||
        !create_directory(VOLUME "Occupied/Child") ||
        !create_directory(VOLUME "Occupied/Child/Keep"))
        return FALSE;
    result = Rename((CONST_STRPTR)VOLUME "Other/Child", (CONST_STRPTR)VOLUME "Occupied/Child");
    error = IoErr();
    return check(!result && error == ERROR_OBJECT_EXISTS, "refuse replacing a different existing directory") &&
           check_parent("Other/Child/Leaf", "Other/Child") && remount_volume() &&
           check_parent("Other/Child/Leaf", "Other/Child") &&
           check_parent("Occupied/Child/Keep", "Occupied/Child");
}

static BOOL test_file_parent(void)
{
    BPTR file = Open((CONST_STRPTR)VOLUME "File", MODE_NEWFILE);
    LONG result, error;

    if (!check(file != BNULL, "create ordinary file as parent control"))
        return FALSE;
    if (!check(Close(file), "close ordinary file"))
        return FALSE;
    result = Rename((CONST_STRPTR)VOLUME "Other/Child", (CONST_STRPTR)VOLUME "File/Child");
    error = IoErr();
    return check(!result && error == ERROR_OBJECT_WRONG_TYPE, "refuse ordinary file as destination parent") &&
           check_parent("Other/Child/Leaf", "Other/Child") && remount_volume() &&
           check_parent("Other/Child/Leaf", "Other/Child");
}

static BOOL test_return_to_root(void)
{
    return check(Rename((CONST_STRPTR)VOLUME "Other/Child", (CONST_STRPTR)VOLUME "Root"), "move directory back to the volume root") &&
           check(absent(VOLUME "Other/Child"), "old nested name is absent") &&
           check_parent("Root/Leaf", "Root") && remount_volume() &&
           check_parent("Root/Leaf", "Root");
}

static BOOL test_case_only(void)
{
    return check(Rename((CONST_STRPTR)VOLUME "Root", (CONST_STRPTR)VOLUME "root"), "case-only rename of the same directory") &&
           check_parent("root/Leaf", "root") && remount_volume() &&
           check_parent("root/Leaf", "root");
}

static BOOL cleanup(void)
{
    BOOL ok = unmount_volume();

    if (!ok)
    {
        printf("NOTE: keeping " DIRECTORY " because its handler could not be stopped\n");
        return FALSE;
    }
    if (owns_assignment)
    {
        if (!check(AssignLock((CONST_STRPTR)"FDSK", BNULL), "remove own FDSK assignment"))
            return FALSE;
        owns_assignment = FALSE;
    }
    if (owns_directory)
    {
        if (!absent(IMAGE))
            ok = check(DeleteFile((CONST_STRPTR)IMAGE), "remove own image") && ok;
        if (!absent(MOUNTLIST))
            ok = check(DeleteFile((CONST_STRPTR)MOUNTLIST), "remove own mountlist") && ok;
        if (ok)
        {
            ok = check(DeleteFile((CONST_STRPTR)DIRECTORY), "remove own temporary directory");
            owns_directory = !ok;
        }
    }
    return ok;
}

static BOOL test_format(const struct Format *next)
{
    BPTR lock;
    BOOL ok;

    format = next;
    printf("begin FAT%u\n", format->bits);
    if (!check(!dos_entry_exists("FATPARENT") && !dos_entry_exists("FDSK") &&
               absent(DIRECTORY), "temporary paths and DOS entries are unused"))
        return FALSE;
    lock = CreateDir((CONST_STRPTR)DIRECTORY);
    owns_directory = lock != BNULL;
    if (lock)
        UnLock(lock);
    ok = check(owns_directory, "create own temporary directory");
    if (ok)
        ok = check(write_image(), "write a fresh empty FAT image") &&
             check(write_mountlist(), "write own mountlist");
    if (ok)
    {
        lock = Lock((CONST_STRPTR)DIRECTORY, SHARED_LOCK);
        owns_assignment = lock && AssignLock((CONST_STRPTR)"FDSK", lock);
        if (lock && !owns_assignment)
            UnLock(lock);
        ok = check(owns_assignment, "create own FDSK assignment");
    }
    if (ok)
        ok = mount_volume() && volume_type();
    if (ok)
        ok = test_cross_parent() && test_descendant() && test_existing_directory() &&
             test_file_parent() && test_return_to_root() && test_case_only();
    if (!cleanup())
        ok = FALSE;
    printf("end FAT%u: %s\n", format->bits, ok ? "PASS" : "FAIL");
    return ok;
}

int main(void)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR old_window;
    unsigned i;
    BOOL ok = TRUE;

    if (!me || me->pr_Task.tc_Node.ln_Type != NT_PROCESS)
        return RETURN_ERROR;
    old_window = me->pr_WindowPtr;
    me->pr_WindowPtr = (APTR)-1;
    for (i = 0; i < sizeof(formats) / sizeof(formats[0]); i++)
    {
        if (!test_format(&formats[i]))
        {
            ok = FALSE;
            break;
        }
    }
    me->pr_WindowPtr = old_window;
    printf("parent_rename: %ld checks, %ld failed\n", (long)checks, (long)failed);
    fflush(stdout);
    return ok && !failed ? RETURN_OK : RETURN_ERROR;
}
