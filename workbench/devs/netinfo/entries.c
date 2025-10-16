/*
 * General DB routines
 *
 * Author: ppessi <Pekka.Pessi@hut.fi>
 *
 * Copyright © 1993 AmiTCP/IP Group, <AmiTCP-group@hut.fi>
 *                  Helsinki University of Technology, Finland.
 */

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "entries.h"
#include "misc.h"

#include <string.h>
#if defined(DEBUG)
#include <assert.h>
#include "assert.h"
#endif

#define DENTS(x)

static struct NetInfoPointer *FindPointer(struct List *list, struct Ent*to);
void GetByNameCmd(struct NetInfoDevice *nid, struct NetInfoReq *req, struct NetInfoMap *nim);
void GetByIDCmd(struct NetInfoDevice *nid, struct NetInfoReq *req, struct NetInfoMap *nim);
void ResetCmd(struct NetInfoDevice *nid, struct NetInfoReq *req, struct NetInfoMap *nim);
void ReadCmd(struct NetInfoDevice *nid, struct NetInfoReq *req, struct NetInfoMap *nim);
void WriteCmd(struct NetInfoDevice *nid, struct NetInfoReq *req, struct NetInfoMap *nim);
void UpdateCmd(struct NetInfoDevice *nid, struct NetInfoReq *req, struct NetInfoMap *nim);
void MembersCmd(struct NetInfoDevice *nid, struct NetInfoReq *req, struct NetInfoMap *nim);

const static DeviceCmd_t map_cmds[NI_END] = {
    /* Standard commands */
    UnknownCommand,		/* CMD_INVALID */
    ResetCmd,			/* CMD_RESET */
    ReadCmd,			/* CMD_READ */
    WriteCmd,			/* CMD_WRITE */
    UpdateCmd,			/* CMD_UPDATE */
    UnknownCommand,		/* CMD_CLEAR */
    UnknownCommand,		/* CMD_STOP */
    UnknownCommand,		/* CMD_START */
    UnknownCommand,		/* CMD_FLUSH */
    /* NetInfo commands */
    GetByIDCmd,			/* NI_GETBYID */
    GetByNameCmd,			/* NI_GETBYNAME */
    MembersCmd,			/* NI_MEMBERS */
};

static const init_map_func_t initMapFuncs[NETINFO_UNITS] = {
    InitPasswdMap,
    InitGroupMap,
};

struct NetInfoMap *InitNetInfoMap(struct NetInfoDevice *nid, struct MsgPort * mp, ULONG mapno)
{
    struct NetInfoMap *nim;

    D(bug("[NetInfo] %s()\n", __func__));

#if defined(DEBUG)
    assert(mapno < 2);
#endif
    if (nim = initMapFuncs[mapno](nid)) {
        nim->nim_Port = mp;
        nim->nim_Commands = map_cmds;

        InitSemaphore(nim->nim_ReqLock);
        NEWLIST(nim->nim_Rx);
        NEWLIST(nim->nim_Wx);

        InitSemaphore(nim->nim_EntLock);
        NEWLIST(nim->nim_Ent);

        InitSemaphore(nim->nim_PointerLock);
        NEWLIST(nim->nim_Pointer);
    }
    return nim;
}

void DeInitNetInfoMap(struct NetInfoDevice *nid, struct NetInfoMap *nim)
{
    D(bug("[NetInfo] %s()\n", __func__));

    if (Method(cleanup, nim) != NULL)
        Method(cleanup, nim)(nid, nim);

    FreeVec(nim);
}

/*
 * NI_GETBYNAME, search an entry by name
 */
void GetByNameCmd(struct NetInfoDevice *nid, struct NetInfoReq *req, struct NetInfoMap *nim)
{
    const UBYTE *name = (const UBYTE *)
                        ((struct NetInfoEnt*)req->io_Data)->nie_name;
    struct Ent *e;

    D(bug("[NetInfo] %s()\n", __func__));

    req->io_Error = NIERR_NOTFOUND;

    e = InternalSetEnts(nid, nim);
    D(bug("[NetInfo] %s: Ents @ 0x%p\n", __func__, e));

    while (e = GetNextEnt(e)) {
        if (strcmp(e->e_name, name) == 0) {
            if (Method(copy_out, nim)(req, e)) {
                req->io_Error = 0;
            } else {
                req->io_Error = NIERR_TOOSMALL;
            }
            break;
        }
    }
    InternalEndEnts(nid, nim);
    TermIO(req);
}

/*
 * NI_GETBYID, Search an entry by ID
 */
void GetByIDCmd(struct NetInfoDevice *nid, struct NetInfoReq *req, struct NetInfoMap *nim)
{
    struct Ent *e;
    LONG id = ((struct NetInfoEnt*)req->io_Data)->nie_id;

    D(bug("[NetInfo] %s()\n", __func__));
    D(bug("[NetInfo] %s: ID %ld\n", __func__, id));

    req->io_Error = NIERR_NOTFOUND;

    e = InternalSetEnts(nid, nim);
    D(bug("[NetInfo] %s: Ents @ 0x%p\n", __func__, e));

    while (e = GetNextEnt(e)) {
        D(bug("[NetInfo] %s: ent id %ld\n", __func__, e->e_id));
        if (e->e_id == id) {
            if (Method(copy_out, nim)(req, e)) {
                req->io_Error = 0;
            } else {
                req->io_Error = NIERR_TOOSMALL;
            }
            break;
        }
    }

    InternalEndEnts(nid, nim);
    TermIO(req);
}

/*
 * CMD_RESET, start reading the database in sequential order
 */
void ResetCmd(struct NetInfoDevice *nid, struct NetInfoReq *req, struct NetInfoMap *nim)
{
    struct NetInfoPointer *p = (struct NetInfoPointer *)req->io_Unit;

    D(bug("[NetInfo] %s()\n", __func__));

    p->nip_Ent = (void *)InternalSetEnts(nid, nim);
    InternalEndEnts(nid, nim);
    req->io_Error = 0;
    TermIO(req);
}

/*
 * CMD_READ, get next entry
 */
void ReadCmd(struct NetInfoDevice *nid, struct NetInfoReq *req, struct NetInfoMap *nim)
{
    struct NetInfoPointer *p = (struct NetInfoPointer *)req->io_Unit;
    struct Ent *e;

    D(bug("[NetInfo] %s()\n", __func__));

    req->io_Error = NIERR_NOTFOUND;

    InternalSetEnts(nid, nim);
    e = p->nip_Ent;
    if (p->nip_Ent && (p->nip_Ent = GetNextEnt(e))) {
        if (Method(copy_out, nim)(req, p->nip_Ent)) {
            req->io_Error = 0;
        } else {
            req->io_Error = NIERR_TOOSMALL;
        }
    }
    InternalEndEnts(nid, nim);
    TermIO(req);
}

/*
 * CMD_WRITE
 */
void WriteCmd(struct NetInfoDevice *nid, struct NetInfoReq *req, struct NetInfoMap *nim)
{
    struct Ent *e, *new_e;
    const UBYTE *name = ((struct NetInfoPasswd *)req->io_Data)->pw_name;

    D(bug("[NetInfo] %s()\n", __func__));

    if (new_e = Method(copy_in, nim)(nid, req)) {
        /* Exclusive lock for writing */
        DbMapLock(nim);

        e = InternalSetEnts(nid, nim);

        while (e = GetNextEnt(e)) {
            if (strcmp(e->e_name, name) == 0) {
                /* A match was found - add new */
                Insert(nim->nim_Ent, (struct Node *)new_e, (struct Node *)e);

                /* Remove old */
                Remove((struct Node *)e);

                /* Update pointers */
                ObtainSemaphore(nim->nim_PointerLock);
                {
                    struct NetInfoPointer *nip;
                    if (nip = FindPointer(nim->nim_Pointer, e)) {
                        nip->nip_Ent = (void *)new_e;
                    }
                }
                ReleaseSemaphore(nim->nim_PointerLock);

                /* Free old */
                FreeVec(e);

                new_e = NULL;
                break;
            }
        }

        /*
         * A new entry?
         */
        if (new_e != NULL) {
            AddTail(nim->nim_Ent, (struct Node *)new_e);
        }

        req->io_Error = 0;
        InternalEndEnts(nid, nim);

        nim->nim_Flags |= NIMF_CHANGED;
        DbMapUnlock(nim);
    } else {
        /* copy_in method will set the io_Error */
    }

    TermIO(req);
}

/*
 * CMD_UPDATE
 */
void UpdateCmd(struct NetInfoDevice *nid, struct NetInfoReq *req, struct NetInfoMap *nim)
{
    BYTE retval = 0;

    D(bug("[NetInfo] %s()\n", __func__));

    if ((nim->nim_Flags & NIMF_CHANGED) != 0) {
        UBYTE newname[128], oldname[128];
        BPTR newfile;
        DbMapLock(nim);
        if ((nim->nim_Flags & NIMF_CHANGED) == 0)
            goto exit;

        newname[sizeof(newname)-1] = '\0';
        strncpy(newname, nim->nim_Filename, sizeof(newname)-1);
        strncat(newname, ".new", sizeof(newname)-1);

        oldname[sizeof(oldname)-1] = '\0';
        strncpy(oldname, nim->nim_Filename, sizeof(oldname)-1);
        strncat(oldname, ".old", sizeof(oldname)-1);

        if (newfile = Open(newname, MODE_NEWFILE)) {
            struct Node *entry, *next;

            retval = 0;

            for (entry = nim->nim_Ent->lh_Head;
                    next = entry->ln_Succ;
                    entry = next) {
                retval = Method(print_out, nim)(nid, newfile, (struct Ent *)entry);
                if (retval)
                    break;
            }
            Close(newfile);

            if (retval == 0) {
                /* Now we are supposed to move newfile to file */
                DeleteFile(oldname);
                Rename(nim->nim_Filename, oldname);
                if (Rename(newname, nim->nim_Filename))
                    nim->nim_Flags &= NIMF_CHANGED;
                else
                    retval = NIERR_ACCESS;
            }
        } else {
            retval = NIERR_ACCESS;
        }
exit:
        DbMapUnlock(nim);
    }

    req->io_Error = retval;
    TermIO(req);
}

/*
 * NI_MEMBERS
 */
void MembersCmd(struct NetInfoDevice *nid, struct NetInfoReq *req, struct NetInfoMap *nim)
{
    D(bug("[NetInfo] %s()\n", __func__));

    if (Method(membercmd, nim) != NULL) {
        Method(membercmd, nim)(nid, req, nim);
    } else {
        req->io_Error = IOERR_NOCMD;
        TermIO(req);
    }
}

/*
 * Find a pointer to this entry
 */
static struct NetInfoPointer *FindPointer(struct List *list, struct Ent *to)
{
    struct NetInfoPointer *entry, *next;

    D(bug("[NetInfo] %s()\n", __func__));

    for (entry = (struct NetInfoPointer *)list->lh_Head;
            next = (struct NetInfoPointer *)entry->nip_Node->ln_Succ;
            entry = next)
        if (entry->nip_Ent == to) {
            return entry;
        }

    return NULL;
}

/*
 * Search for map
 */
struct NetInfoMap *CheckUnit(struct NetInfoDevice *nid, struct Unit *u)
{
    struct NetInfoPointer *nip = (struct NetInfoPointer *)u;

    D(bug("[NetInfo] %s()\n", __func__));

    if (nip != NULL)
        return nip->nip_Map;
    else
        return NULL;
}


/*
 * Parse group database if needed
 */
struct Ent *InternalSetEnts(struct NetInfoDevice *nid, struct NetInfoMap *nim)
{
    DbMapLockShared(nim);

    D(bug("[NetInfo] %s()\n", __func__));

    if ((nim->nim_Flags & NIMF_PARSED) == 0) {
        int was_too_long = 0;
        UBYTE *p, *line = NULL;
        BPTR fh = BNULL;
        struct Ent *ent;

        /* We can not get exclusive lock if we have shared lock */
        DbMapUnlock(nim);
        /* Exclusive lock for writing */
        DbMapLock(nim);
        if ((nim->nim_Flags & NIMF_PARSED) != 0)
            goto error;

        line = AllocVec(MAXLINELENGTH + 1, MEMF_PUBLIC);
        fh = Open((STRPTR)nim->nim_Filename, MODE_OLDFILE);

        if (!line || !fh)
            goto error;

        /* Free old entries */
        FreeListVec(nid, nim->nim_Ent);

        /* Invalidate pointers to old database */
        ObtainSemaphore(nim->nim_PointerLock);
        {
            struct NetInfoPointer *nip, *next;

            for (nip = (struct NetInfoPointer *)nim->nim_Pointer->lh_Head;
                    next = (struct NetInfoPointer *)nip->nip_Node->ln_Succ;
                    nip = next) {
                nip->nip_Ent = NULL;
            }
        }
        ReleaseSemaphore(nim->nim_PointerLock);

        line[MAXLINELENGTH] = '\0';

        while (p = FGets(fh, line, MAXLINELENGTH)) {
            while (*p && *p != '\n')
                p++;
            if (!*p) {
                /* Line is too long */
                was_too_long = 1;
                continue;
            }
            if (was_too_long) {
                /* Rest of a line */
                was_too_long = 0;
                continue;
            }
            if (ent = Method(parse_ent, nim)(nid, line))
                AddTail(nim->nim_Ent, (struct Node *)ent);
        }
        nim->nim_Flags |= NIMF_PARSED;

        /* Ask for notifies */
        if (nim->nim_Methods->notify != NULL &&
                nim->nim_Notify->nr_UserData == 0) {
            memset(nim->nim_Notify, 0, sizeof *nim->nim_Notify);
            nim->nim_Notify->nr_Name = (STRPTR)nim->nim_Filename;
            nim->nim_Notify->nr_Flags = NRF_SEND_MESSAGE;
            nim->nim_Notify->nr_stuff.nr_Msg.nr_Port = nid->nid_NotifyPort;
            nim->nim_Notify->nr_UserData = (IPTR) nim;
            StartNotify(nim->nim_Notify);
        }

error:
        if (line) FreeVec(line);
        if (fh) Close(fh);

        /* Downgrade to a shared lock */
        DbMapUnlock(nim);
        DbMapLockShared(nim);
    }

    return (struct Ent *)nim->nim_Ent;
}

void InternalEndEnts(struct NetInfoDevice *nid, struct NetInfoMap *nim)
{
    D(bug("[NetInfo] %s()\n", __func__));

    DbMapUnlock(nim);
}

/*
 * Return next entry
 */
struct Ent *GetNextEnt(struct Ent *e)
{
    DENTS(bug("[NetInfo] %s()\n", __func__));

    e = (struct Ent *)e->e_node.ln_Succ;

    if (e->e_node.ln_Succ)
        return e;
    else
        return NULL;
}

/*
 * Mark as unparsed (when map has received DOS notify)
 */
void EntHandleNotify(struct NetInfoDevice *nid, struct NetInfoMap *nim)
{
    D(bug("[NetInfo] %s()\n", __func__));

    DbMapLockShared(nim);
    nim->nim_Flags &= ~NIMF_PARSED;
    DbMapUnlock(nim);
}

/*
 * Free all entries
 */
void EntCleanup(struct NetInfoDevice *nid, struct NetInfoMap *nim)
{
    D(bug("[NetInfo] %s()\n", __func__));

    DbMapLock(nim);
    FreeListVec(nid, nim->nim_Ent);
    /* End notifies */
    if (nim->nim_Notify->nr_UserData != 0) {
        EndNotify(nim->nim_Notify);
        nim->nim_Notify->nr_UserData = 0;
    }
    DbMapUnlock(nim);
}
