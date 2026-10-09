/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Filesystem Imports - listing a host's exports (transaction
          command 4 to its "Filesystem" service) and mounting one through
          the original text mount file (re/spec/efs-protocol.md §1.1, §1.6).
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/nipc.h>
#include <proto/services.h>
#include <proto/accounts.h>
#include <exec/memory.h>
#include <dos/dostags.h>
#include <workbench/workbench.h>
#include <envoy/nipc.h>
#include <envoy/services.h>
#include <envoy/errors.h>
#include <stdio.h>
#include <string.h>

#include "imports.h"
#include "locale.h"

extern struct Library *NIPCBase, *ServicesBase, *AccountsBase, *IconBase;

#define CMD_LISTEXPORTS     4
#define LIST_REQSIZE        128
#define LIST_RESPSIZE       1024
#define LIST_TIMEOUT        5
#define SEPARATOR           0xA6

CONST_STRPTR Imp_ErrorText(ULONG err, STRPTR buf, ULONG size)
{
    switch (err)
    {
    case ENVOYERR_UNKNOWNHOST:      return _(MSG_ERR_UNKNOWNHOST);
    case ENVOYERR_UNKNOWNENTITY:
    case ENVOYERR_UNKNOWNSERVICE:   return _(MSG_ERR_NOSERVICE);
    case ENVOYERR_OPENSERVICEFAIL:
    case ENVOYERR_BADSTARTSERVICE:  return _(MSG_ERR_NOSERVICELIB);
    case ENVOYERR_TIMEOUT:          return _(MSG_ERR_TIMEOUT);
    case ENVOYERR_NORESOLVER:
    case ENVOYERR_CANTDELIVER:
    case ENVOYERR_ABORTED:          return _(MSG_ERR_UNREACHABLE);
    case ENVOYERR_NORESOURCES:      return _(MSG_ERR_RESOURCES);
    case 0x8000:
    case 0x8001:                    return _(MSG_ERR_REFUSED);  /* also IMP_ERR_REFUSED */
    }
    snprintf(buf, size, _(MSG_ERR_OTHER), (long)err);
    return buf;
}

void Imp_FreeList(struct List *list)
{
    struct Node *n;

    while ((n = RemHead(list)))
        FreeVec(n);
}

static void CopyField(STRPTR dst, ULONG size, CONST_STRPTR src)
{
    ULONG n = src ? strlen(src) : 0;

    if (n > size - 1)
        n = size - 1;
    if (n)
        CopyMem((APTR)src, dst, n);
    dst[n] = '\0';
}

ULONG Imp_ListExports(CONST_STRPTR host, CONST_STRPTR user, CONST_STRPTR password, struct List *list)
{
    struct Entity *me, *svc;
    struct Transaction *t;
    ULONG err = 0;
    UBYTE *req;

    if (!(me = CreateEntity(ENT_AllocSignal, 0, TAG_DONE)))
        return ENVOYERR_NORESOURCES;
    svc = FindService((STRPTR)host, (STRPTR)"Filesystem", me, FSVC_Error, (IPTR)&err, TAG_DONE);
    if (!svc)
    {
        DeleteEntity(me);
        return err ? err : ENVOYERR_UNKNOWNSERVICE;
    }
    if (!(t = AllocTransaction(TRN_AllocReqBuffer, LIST_REQSIZE, TRN_AllocRespBuffer, LIST_RESPSIZE, TAG_DONE)))
        err = ENVOYERR_NORESOURCES;
    else
    {
        /* user name at 0, clear password at 64, each a C string in a 64-byte field */
        req = t->trans_RequestData;
        memset(req, 0, LIST_REQSIZE);
        CopyField((STRPTR)req, 64, user);
        CopyField((STRPTR)req + 64, 64, password);
        t->trans_Command = CMD_LISTEXPORTS;
        t->trans_ReqDataActual = LIST_REQSIZE;
        t->trans_Timeout = LIST_TIMEOUT;
        err = DoTransaction(svc, me, t);
        memset(req, 0, LIST_REQSIZE);           /* do not keep the password */
        if (!err)
        {
            ULONG off;
            for (off = 0; off + IMP_NAMELEN <= t->trans_RespDataActual; off += IMP_NAMELEN)
            {
                CONST_STRPTR name = (CONST_STRPTR)t->trans_ResponseData + off;
                ULONG len = strnlen(name, IMP_NAMELEN - 1);
                struct Node *n;

                if (!len || !(n = AllocVec(sizeof(struct Node) + len + 1, MEMF_CLEAR)))
                    continue;
                n->ln_Name = (char *)(n + 1);
                CopyMem((APTR)name, n->ln_Name, len);
                AddTail(list, n);
            }
        }
        FreeTransaction(t);
    }
    LoseService(svc);
    DeleteEntity(me);
    return err;
}

BOOL Imp_MakeLogin(struct ImpLogin *login, CONST_STRPTR user, CONST_STRPTR password)
{
    char hash[16];

    memset(login, 0, sizeof(*login));
    CopyField(login->User, sizeof(login->User), user);
    if (!AccountsBase)
        return FALSE;
    /* ECrypt lower-cases the user name itself (re/spec/services-accounts.md §7) */
    ECrypt((STRPTR)hash, (STRPTR)(password ? password : (CONST_STRPTR)""), (STRPTR)login->User);
    login->Hash[0] = '$';
    CopyField(login->Hash + 1, sizeof(login->Hash) - 1, hash);
    memset(hash, 0, sizeof(hash));
    return TRUE;
}

void Imp_DeviceName(STRPTR buf, ULONG size, CONST_STRPTR host, CONST_STRPTR export)
{
    CONST_STRPTR h = strrchr(host, ':');
    ULONG n = 0, len;

    h = h ? h + 1 : host;                       /* "Realm:host" -> "host" */
    while (*h && n < size - 2)
        buf[n++] = *h++;
    buf[n++] = '-';
    len = strlen(export);
    if (len && export[len - 1] == ':')
        len--;                                  /* one trailing ':' is removed */
    for (; len && n < size - 1; export++, len--)
        buf[n++] = (*export == ':') ? '_' : *export;
    buf[n] = '\0';
}

/* The mount file, exactly as the original program writes it (no newline at the end) */
static ULONG MountText(STRPTR buf, ULONG size, CONST_STRPTR host, CONST_STRPTR export, const struct ImpLogin *login)
{
    return snprintf(buf, size,
        "Filesystem = L:EnvoyFileSystem\n"
        "StackSize = 4000\n"
        "Priority = 5\n"
        "GlobVec = -2\n"
        "Activate = 1\n"
        "Unit=\"%s%c%s%c%s%c%s%c\"\n"
        "Surfaces = 0\n"
        "BlocksPerTrack = 0\n"
        "LowCyl = 0\n"
        "HighCyl = 0\n"
        "Device =\"Envoy FS\"\n"
        "DosType = 0x444f5380",
        host, SEPARATOR, export, SEPARATOR, login->User, SEPARATOR, login->Hash, SEPARATOR);
}

static BOOL WriteIcon(CONST_STRPTR path)
{
    struct DiskObject *dobj;
    STRPTR tooltypes[] = { (STRPTR)"DONOTWAIT", NULL };
    BOOL ok = FALSE;

    if (!IconBase)
        return FALSE;
    if ((dobj = GetDefDiskObject(WBPROJECT)))
    {
        dobj->do_DefaultTool = (STRPTR)"C:Mount";
        dobj->do_ToolTypes = tooltypes;
        dobj->do_CurrentX = NO_ICON_POSITION;
        dobj->do_CurrentY = NO_ICON_POSITION;
        ok = PutDiskObject((STRPTR)path, dobj);
        FreeDiskObject(dobj);
    }
    return ok;
}

static BOOL DirExists(CONST_STRPTR path)
{
    BPTR lock;

    if ((lock = Lock(path, SHARED_LOCK)))
    {
        UnLock(lock);
        return TRUE;
    }
    return FALSE;
}

static BOOL DeviceExists(CONST_STRPTR name)
{
    struct DosList *dl;
    BOOL found;

    dl = LockDosList(LDF_DEVICES | LDF_READ);
    found = FindDosEntry(dl, name, LDF_DEVICES) != NULL;
    UnLockDosList(LDF_DEVICES | LDF_READ);
    return found;
}

LONG Imp_Mount(CONST_STRPTR host, CONST_STRPTR export, const struct ImpLogin *login, LONG location,
               STRPTR path, ULONG pathsize, STRPTR volname, ULONG volsize)
{
    char devname[IMP_HOSTLEN + IMP_NAMELEN + 2], text[600], cmd[IMP_HOSTLEN + 300], dev[sizeof(devname) + 1];
    CONST_STRPTR dir;
    BPTR fh, lock;
    LONG len, err = 0;
    struct Process *me = (struct Process *)FindTask(NULL);
    APTR oldwin;

    volname[0] = '\0';
    Imp_DeviceName(devname, sizeof(devname), host, export);
    snprintf(dev, sizeof(dev), "%s:", devname);

    switch (location)
    {
    case LOC_PERMANENT:
        dir = DirExists("DEVS:DOSDrivers") ? "DEVS:DOSDrivers" : "SYS:WBStartup";
        break;
    case LOC_STORAGE:
        dir = "SYS:Storage/DOSDrivers";
        break;
    default:
        dir = "T:";
        break;
    }
    CopyField(path, pathsize, dir);
    AddPart(path, devname, pathsize);

    if (DeviceExists(devname))
        return ERROR_OBJECT_EXISTS;

    len = MountText(text, sizeof(text), host, export, login);
    if (!(fh = Open(path, MODE_NEWFILE)))
        return IoErr();
    if (Write(fh, text, len) != len)
        err = IoErr();
    Close(fh);
    memset(text, 0, sizeof(text));
    if (err)
        return err;
    WriteIcon(path);                            /* Temporary: deleted again after mounting */

    snprintf(cmd, sizeof(cmd), "C:Mount >NIL: <NIL: \"%s\"", path);
    SystemTags(cmd, SYS_Input, BNULL, SYS_Output, BNULL, TAG_DONE);

    if (location == LOC_TEMPORARY)
    {
        char info[300];
        snprintf(info, sizeof(info), "%s.info", path);
        DeleteFile(path);
        DeleteFile(info);
    }

    if (!DeviceExists(devname))
        return ERROR_OBJECT_NOT_FOUND;

    /* the handler starts with the first access: this is where the mount is really done */
    oldwin = me->pr_WindowPtr;
    me->pr_WindowPtr = (APTR)-1;
    if ((lock = Lock(dev, SHARED_LOCK)))
    {
        struct InfoData *id = AllocMem(sizeof(struct InfoData), MEMF_PUBLIC | MEMF_CLEAR);
        if (id && Info(lock, id) && id->id_VolumeNode)
        {
            struct DosList *vol = (struct DosList *)BADDR(id->id_VolumeNode);
            CONST_STRPTR name = (CONST_STRPTR)AROS_BSTR_ADDR(vol->dol_Name);
            ULONG n = AROS_BSTR_strlen(vol->dol_Name);
            if (n > volsize - 1)
                n = volsize - 1;
            CopyMem((APTR)name, volname, n);
            volname[n] = '\0';
        }
        if (id)
            FreeMem(id, sizeof(struct InfoData));
        UnLock(lock);
    }
    else
    {
        struct DosList *dl;

        err = IoErr();
        /* the handler's start-up failed: the mount was refused or the host is unreachable */
        if (!err || err == ERROR_OBJECT_NOT_FOUND || err == ERROR_DEVICE_NOT_MOUNTED)
            err = IMP_ERR_REFUSED;
        /* The handler gave up (refused, unknown host...). Remove the device again so that a
           corrected attempt can mount under the same name; DOS would otherwise keep retrying
           with the old Unit string. */
        dl = LockDosList(LDF_DEVICES | LDF_WRITE);
        if ((dl = FindDosEntry(dl, devname, LDF_DEVICES)) && !dl->dol_Task)
            RemDosEntry(dl);
        UnLockDosList(LDF_DEVICES | LDF_WRITE);
    }
    me->pr_WindowPtr = oldwin;
    if (!volname[0] && !err)
        CopyField(volname, volsize, devname);
    return err;
}
