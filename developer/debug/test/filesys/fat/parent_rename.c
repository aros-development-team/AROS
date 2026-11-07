/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Native FAT directory-parent regression. Use an expendable mounted FAT
    volume and a fresh top-level ROOT prefix. Fixtures are kept for inspection.
    Adapted from the published native DOS regression in kaffeine1/kaffeine.
*/
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/exec.h>

static LONG checks, failed;
static void check(BOOL ok, const char *what)
{
    ++checks;
    if (!ok) ++failed;
    Printf((CONST_STRPTR)"%s %s\n", (IPTR)(ok ? "ok" : "FAIL"), (IPTR)what);
}
static BOOL equal(const char *a, const char *b)
{
    while (*a && *a == *b) { ++a; ++b; }
    return *a == *b;
}
static BOOL copy(char *out, const char *in)
{
    unsigned n = 0;
    while (in[n]) { if (n == 510) return FALSE; out[n] = in[n]; ++n; }
    out[n] = 0;
    return TRUE;
}
static BOOL suffix(char *out, const char *base, const char *tail)
{
    unsigned n = 0, j = 0;
    if (!copy(out, base)) return FALSE;
    while (out[n]) ++n;
    while (tail[j]) { if (n == 510) return FALSE; out[n++] = tail[j++]; }
    out[n] = 0;
    return TRUE;
}
static BOOL subpath(char *out, const char *base, const char *tail)
{
    return copy(out, base) && AddPart((STRPTR)out, (CONST_STRPTR)tail, 512);
}
static BOOL absent(const char *path)
{
    BPTR lock;
    LONG err;
    SetIoErr(0); lock = Lock((CONST_STRPTR)path, SHARED_LOCK); err = IoErr();
    if (lock) UnLock(lock);
    return !lock && err == ERROR_OBJECT_NOT_FOUND;
}
static BOOL directory(const char *path)
{
    BPTR lock = CreateDir((CONST_STRPTR)path);
    check(lock != BNULL, path);
    if (lock) UnLock(lock);
    return lock != BNULL;
}
static BOOL parent(const char *child, const char *want, BOOL broken)
{
    BPTR lock, up = BNULL, expected = BNULL;
    LONG perr = 0, named = 0, nerr = 0;
    char name[512] = {0};
    lock = Lock((CONST_STRPTR)child, SHARED_LOCK);
    check(lock != BNULL, "child lock exists after rename");
    if (!lock) return FALSE;
    SetIoErr(0); up = ParentDir(lock); perr = IoErr();
    SetIoErr(0); named = NameFromLock(lock, (STRPTR)name, sizeof(name)); nerr = IoErr();
    Printf((CONST_STRPTR)"observe child=%s parent=%s err=%ld named=%ld name=%s nameerr=%ld\n",
        (IPTR)child, (IPTR)(up ? "yes" : "no"), (IPTR)perr,
        (IPTR)named, (IPTR)name, (IPTR)nerr);
    if (broken)
    {
        check(!up && perr == ERROR_OBJECT_NOT_FOUND, "baseline stale parent link is reproduced");
        check(named && equal(name, "E:"), "baseline NameFromLock loses the directory chain");
    }
    else
    {
        expected = Lock((CONST_STRPTR)want, SHARED_LOCK);
        check(expected != BNULL, "expected parent lock exists");
        check(up && expected && SameLock(up, expected) == LOCK_SAME, "ParentDir identifies the expected parent");
        check(named && equal(name, child), "NameFromLock retains the complete canonical path");
    }
    if (expected) UnLock(expected);
    if (up) UnLock(up);
    UnLock(lock);
    return failed == 0;
}
int main(void)
{
    SIPTR options[2] = {0};
    struct RDArgs *args;
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR oldwindow;
    char root[512], canonical[512] = {0}, stage[512], original[512], child[512];
    char control[512], cchild[512], moved[512], movedchild[512];
    char dest[512], nested[512], nestedchild[512], rootprobe[512], rootchild[512];
    char occupiedparent[512], occupied[512], textfile[512], invalid[512], cycle[512];
    char variant[512], variantchild[512];
    BOOL baseline;
    BPTR lock = BNULL, file;
    unsigned i, n, colons = 0;
    const char *prefix;
    if (!me || me->pr_Task.tc_Node.ln_Type != NT_PROCESS) return RETURN_ERROR;
    oldwindow = me->pr_WindowPtr; me->pr_WindowPtr = (APTR)-1;
    args = ReadArgs((CONST_STRPTR)"ROOT/A,BASELINE/S", options, NULL);
    if (!args) { me->pr_WindowPtr = oldwindow; return RETURN_ERROR; }
    prefix = (const char *)options[0]; baseline = options[1] != 0;
    for (n = 0; prefix[n] && n < 200; ++n)
        if (prefix[n] == '/' || (prefix[n] == ':' && n == 0)) break;
    if (!prefix[n] && n > 2 && prefix[n-1] != ':')
    {
        for (i = 0; i < n; ++i) if (prefix[i] == ':') ++colons;
        if (colons != 1) goto invalid_root;
    }
    else goto invalid_root;
    for (i = 1; i <= 8; ++i)
    {
        char tail[3] = {'-', (char)('0' + i), 0};
        if (!suffix(root, prefix, tail) || !suffix(moved, root, "-Moved") ||
            !suffix(rootprobe, root, "-Root")) goto invalid_root;
        if (absent(root) && absent(moved) && absent(rootprobe)) break;
    }
    if (i > 8) goto invalid_root;
    if (!directory(root)) goto finish;
    lock = Lock((CONST_STRPTR)root, SHARED_LOCK);
    check(lock != BNULL, "lock fresh fixture root");
    if (!lock) goto finish;
    check(NameFromLock(lock, (STRPTR)canonical, sizeof(canonical)), "canonical fixture root name");
    if (!canonical[0]) goto setup_failed;
    {
        struct InfoData info = {0};
        struct DosList *walk;
        LONG disk_type = 0;
        BOOL top_level = TRUE;
        for (i = 0; canonical[i]; ++i) if (canonical[i] == '/') top_level = FALSE;
        check(top_level, "fixture root is directly in the volume root");
        if (!top_level) goto finish;
        check(Info(lock, &info), "read fixture volume information");
        if (!info.id_VolumeNode) goto setup_failed;
        walk = LockDosList(LDF_VOLUMES | LDF_READ);
        if (!walk) goto setup_failed;
        while ((walk = NextDosEntry(walk, LDF_VOLUMES)))
            if (walk == (struct DosList *)BADDR(info.id_VolumeNode))
            { disk_type = walk->dol_misc.dol_volume.dol_DiskType; break; }
        UnLockDosList(LDF_VOLUMES | LDF_READ);
        Printf((CONST_STRPTR)"volume type=%08lx\n", (IPTR)(ULONG)disk_type);
        check(disk_type == 0x46415400L || disk_type == 0x46415401L || disk_type == 0x46415402L,
            "fixture volume is FAT12, FAT16 or FAT32");
        if (failed) goto finish;
    }
    UnLock(lock); lock = BNULL;
    copy(root, canonical);
    check(suffix(moved, root, "-Moved") && suffix(rootprobe, root, "-Root"), "bounded root destinations");
    Printf((CONST_STRPTR)"fixture root=%s moved=%s rootprobe=%s baseline=%ld\n",
        (IPTR)root, (IPTR)moved, (IPTR)rootprobe, (IPTR)baseline);
    if (!subpath(stage, root, "S") || !subpath(original, stage, "D") ||
        !subpath(child, original, "E") || !subpath(control, root, "C") ||
        !subpath(cchild, control, "E") || !subpath(dest, root, "Destination") ||
        !subpath(nested, dest, "D") || !subpath(nestedchild, nested, "E") ||
        !subpath(movedchild, moved, "E") || !subpath(rootchild, rootprobe, "E")) goto setup_failed;
    if (!directory(stage) || !directory(original) || !directory(child) ||
        !directory(control) || !directory(cchild) || !directory(dest)) goto finish;
    if (!parent(cchild, control, FALSE)) goto finish;
    check(Rename((CONST_STRPTR)original, (CONST_STRPTR)moved), "move directory from staging to volume root");
    if (!parent(movedchild, moved, baseline)) goto finish;
    check(absent(original), "old directory name disappeared");
    if (baseline) goto finish;
    check(Rename((CONST_STRPTR)moved, (CONST_STRPTR)nested), "move directory from volume root to another parent");
    if (!parent(nestedchild, nested, FALSE)) goto finish;
    if (!subpath(cycle, nestedchild, "Cycle")) goto setup_failed;
    SetIoErr(0);
    check(!Rename((CONST_STRPTR)nested, (CONST_STRPTR)cycle) && IoErr() == ERROR_OBJECT_IN_USE,
        "reject moving a directory into its own descendant");
    if (failed || !parent(nestedchild, nested, FALSE)) goto finish;
    if (!subpath(occupiedparent, root, "Occupied") || !subpath(occupied, occupiedparent, "D")) goto setup_failed;
    if (!directory(occupiedparent) || !directory(occupied)) goto finish;
    SetIoErr(0);
    check(!Rename((CONST_STRPTR)nested, (CONST_STRPTR)occupied) && IoErr() == ERROR_OBJECT_EXISTS,
        "reject replacement of a different existing directory with the same leaf name");
    if (failed || !parent(nestedchild, nested, FALSE)) goto finish;
    if (!subpath(textfile, root, "File") || !subpath(invalid, textfile, "Child")) goto setup_failed;
    file = Open((CONST_STRPTR)textfile, MODE_NEWFILE);
    check(file != BNULL, "create ordinary file parent control");
    if (!file) goto finish;
    Close(file);
    SetIoErr(0);
    check(!Rename((CONST_STRPTR)nested, (CONST_STRPTR)invalid) && IoErr() == ERROR_OBJECT_WRONG_TYPE,
        "reject ordinary file as destination parent");
    if (failed || !parent(nestedchild, nested, FALSE)) goto finish;
    check(Rename((CONST_STRPTR)nested, (CONST_STRPTR)rootprobe), "move back to volume root with fresh target");
    if (!parent(rootchild, rootprobe, FALSE)) goto finish;
    copy(variant, rootprobe);
    for (i = 0; variant[i] && variant[i] != ':'; ++i) {}
    for (++i; variant[i]; ++i)
        if (variant[i] >= 'A' && variant[i] <= 'Z') variant[i] += 'a' - 'A';
    if (!subpath(variantchild, variant, "E")) goto setup_failed;
    check(Rename((CONST_STRPTR)rootprobe, (CONST_STRPTR)variant), "case-only rename of the same directory entry");
    if (!parent(variantchild, variant, FALSE)) goto finish;
    goto finish;
setup_failed:
    check(FALSE, "bounded fixture path setup");
    goto finish;
invalid_root:
    check(FALSE, "ROOT must be a bounded top-level prefix with fresh slots on an expendable FAT volume");
finish:
    if (lock) UnLock(lock);
    FreeArgs(args); me->pr_WindowPtr = oldwindow;
    Printf((CONST_STRPTR)"fat_parent: %ld checks, %ld failed\n", (IPTR)checks, (IPTR)failed);
    Flush(Output());
    return failed ? RETURN_ERROR : RETURN_OK;
}
