/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: security.library - ownership and protection of the database

          Whenever the server (re)reads the user database it checks that
          the files the security of the system rests on belong to root and
          cannot be changed by anybody else - and that the password file,
          which holds the hashes, cannot even be read. What is wrong is
          corrected and logged. This repairs installations made by older
          versions, and files edited or copied by hand.

          On a volume that cannot store owners (FAT...) the owner part is
          skipped with one log line; AMUInit refuses to set up such a
          volume, but the configuration may still have been placed there.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/exall.h>
#include <libraries/pam.h>
#include <string.h>

#include "security_intern.h"
#include "security_protect.h"

#define ROOTOWNER       (((ULONG)secROOT_UID << 16) | secROOT_GID)

/* group and other bits that must not be set */
#define GO_WRITE        (FIBF_GRP_WRITE | FIBF_GRP_DELETE | FIBF_OTR_WRITE | FIBF_OTR_DELETE)
#define GO_ALL          (GO_WRITE | FIBF_GRP_READ | FIBF_GRP_EXECUTE | FIBF_OTR_READ | FIBF_OTR_EXECUTE)

extern const char ConfigFileName[];
extern const char LogFileName[];

struct ProtectState
{
    struct FileInfoBlock    *fib;
    BOOL                    NoOwners;   /* already reported for this pass */
};

static void Report(struct SecurityBase *secBase, CONST_STRPTR fmt, CONST_STRPTR path)
{
    SIPTR args[1] = { (SIPTR)path };
    VLogF(secBase, LOGF_Flag_Type_Warn, SECLOG_SUB_SERVER, fmt, args);
}

/* path: full name. deny: group/other bits to clear */
static void SecureObject(struct SecurityBase *secBase, struct ProtectState *ps, CONST_STRPTR path, ULONG deny)
{
    BPTR lock;
    BOOL ok;

    if (!(lock = Lock(path, SHARED_LOCK)))
        return;
    ok = Examine(lock, ps->fib);
    UnLock(lock);
    if (!ok)
        return;

    if (ps->fib->fib_OwnerUID != secROOT_UID)
    {
        if (SetOwner(path, ROOTOWNER))
            Report(secBase, "'%s' did not belong to root; corrected", path);
        else if (!ps->NoOwners)
        {
            ps->NoOwners = TRUE;
            Report(secBase, "cannot give '%s' to root: the volume does not store owners", path);
        }
    }
    if (ps->fib->fib_Protection & deny)
    {
        if (SetProtection(path, ps->fib->fib_Protection & ~deny))
            Report(secBase, "'%s' was accessible to other users; corrected", path);
    }
}

/* A directory and every file in it */
static void SecureDir(struct SecurityBase *secBase, struct ProtectState *ps, CONST_STRPTR dirpath, ULONG deny)
{
    char path[256];
    BPTR lock;
    struct FileInfoBlock *fib;

    if (!(lock = Lock(dirpath, SHARED_LOCK)))
        return;
    if ((fib = AllocDosObject(DOS_FIB, NULL)))
    {
        if (Examine(lock, fib) && fib->fib_DirEntryType > 0)
        {
            while (ExNext(lock, fib))
            {
                if (fib->fib_DirEntryType >= 0)
                    continue;
                strcpy(path, dirpath);
                if (AddPart(path, fib->fib_FileName, sizeof(path)))
                    SecureObject(secBase, ps, path, deny);
            }
        }
        FreeDosObject(DOS_FIB, fib);
    }
    UnLock(lock);
    SecureObject(secBase, ps, dirpath, deny);
}

static BOOL PathIn(struct SecurityBase *secBase, BPTR dir, CONST_STRPTR name, STRPTR path, ULONG size)
{
    if (!dir || !NameFromLock(dir, path, size))
        return FALSE;
    return AddPart(path, name, size);
}

void CheckDatabaseProtection(struct SecurityBase *secBase)
{
    struct ProtectState ps;
    char path[256];

    if (!secBase->Configured || !secBase->sec_AfterDOSDone)
        return;
    memset(&ps, 0, sizeof(ps));
    if (!(ps.fib = AllocDosObject(DOS_FIB, NULL)))
        return;

    if (PathIn(secBase, secBase->_pwdLock, PasswdFileName, path, sizeof(path)))
        SecureObject(secBase, &ps, path, GO_ALL);
    if (PathIn(secBase, secBase->_cfgLock, GroupFileName, path, sizeof(path)))
        SecureObject(secBase, &ps, path, GO_WRITE);
    if (PathIn(secBase, secBase->_cfgLock, ConfigFileName, path, sizeof(path)))
        SecureObject(secBase, &ps, path, GO_WRITE);
    if (PathIn(secBase, secBase->_cfgLock, LogFileName, path, sizeof(path)))
        SecureObject(secBase, &ps, path, GO_ALL);
    if (secBase->_cfgLock && NameFromLock(secBase->_cfgLock, path, sizeof(path)))
        SecureObject(secBase, &ps, path, GO_WRITE);
    if (secBase->_pwdLock && NameFromLock(secBase->_pwdLock, path, sizeof(path)))
        SecureObject(secBase, &ps, path, GO_WRITE);

    /* pam.library's service files and modules */
    SecureDir(secBase, &ps, PAM_CONFIG_DIR, GO_WRITE);
    SecureDir(secBase, &ps, PAM_MODULE_DIR, GO_WRITE);

    FreeDosObject(DOS_FIB, ps.fib);
}

/*
 * Once per boot: is the installed Security-Startup older than the one
 * AMUInit would install now? Then say how to update it.
 */
void CheckStartupScript(struct SecurityBase *secBase)
{
    char line[160];
    BPTR fh;
    ULONG v = 0, r = 0;

    if (!(fh = Open("SYS:S/Security-Startup", MODE_OLDFILE)))
        return;
    while (FGets(fh, line, sizeof(line) - 1))
    {
        char *p = strstr(line, "$VER: Security-Startup ");
        if (p)
        {
            p += strlen("$VER: Security-Startup ");
            while (*p >= '0' && *p <= '9')
                v = v * 10 + (*p++ - '0');
            if (*p == '.')
                for (p++; *p >= '0' && *p <= '9'; p++)
                    r = r * 10 + (*p - '0');
            break;
        }
    }
    Close(fh);
    if (v < secSTARTUP_VERSION || (v == secSTARTUP_VERSION && r < secSTARTUP_REVISION))
    {
        SIPTR args[4] = { v, r, secSTARTUP_VERSION, secSTARTUP_REVISION };
        VLogF(secBase, LOGF_Flag_Type_Information, SECLOG_SUB_SERVER,
              "S:Security-Startup is version %ld.%ld, %ld.%ld is current: run AMUInit UPGRADE SYS:", args);
    }
}

void ProtectRootOnly(struct SecurityBase *secBase, CONST_STRPTR name)
{
    SetOwner(name, ROOTOWNER);
    /* owner rwd (active low: only E denied), no group/other bits */
    SetProtection(name, FIBF_EXECUTE);
}

void ProtectRootWritable(struct SecurityBase *secBase, CONST_STRPTR name)
{
    SetOwner(name, ROOTOWNER);
    /* owner rwd, group and other r */
    SetProtection(name, FIBF_EXECUTE | FIBF_GRP_READ | FIBF_OTR_READ);
}

/*
 * A database file is rewritten as <name>.new, then <name> is renamed to
 * <name>.old, <name>.new to <name>, and <name>.old is deleted. A missing
 * password file would make the system unconfigured - everybody root - so
 * an interruption is repaired before the file is read: <name>.new is only
 * renamed once complete, so it is the newest full copy.
 */
void RecoverDbFile(struct SecurityBase *secBase, CONST_STRPTR name, CONST_STRPTR newname, CONST_STRPTR oldname)
{
    BPTR lock;

    if ((lock = Lock(name, SHARED_LOCK)))
    {
        UnLock(lock);
        /* leftovers of a rewrite that completed or never got far */
        DeleteFile(newname);
        DeleteFile(oldname);
        return;
    }
    if (Rename(newname, name))
    {
        Report(secBase, "'%s' was restored from its new copy", name);
        DeleteFile(oldname);
    }
    else if (Rename(oldname, name))
        Report(secBase, "'%s' was restored from its old copy", name);
}
