/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: SFS multi-user support: ownership and protection enforcement via
          security.library (muFS filesystem contract).
*/

#include "asmsupport.h"

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <devices/timer.h>
#include <utility/tagitem.h>
#include <libraries/security.h>

#include "objects.h"
#include "sfs_security.h"
#include "objects_protos.h"
#include "cachebuffers_protos.h"
#include "debug.h"
#include "locks_protos.h"
#include "nodes_protos.h"
#include "support_protos.h"
#include "globals.h"

#define secBase ((struct Library *)globals->sec_Base)
#include <proto/security.h>

/* The root directory belongs to the system, everybody may read/list it */
#define SFS_ROOT_OWNER          secOWNER_SYSTEM
#define SFS_ROOT_PROTECTION     (FIBF_GRP_READ | FIBF_GRP_EXECUTE | FIBF_OTR_READ | FIBF_OTR_EXECUTE)
#define SFS_OWNERBITS           (FIBF_READ | FIBF_WRITE | FIBF_EXECUTE | FIBF_DELETE)

extern LONG locateobjectfromlock(struct ExtFileLock *lock, UBYTE *path, struct CacheBuffer **returned_cb, struct fsObject **returned_o);
extern LONG locateparent(struct ExtFileLock *lock, UBYTE *path, struct CacheBuffer **returned_cb, struct fsObject **returned_o);
extern UBYTE *validatepath(UBYTE *path);

/* Forget the cached "is this volume secured" answer (new medium, rendezvous) */
void sfsSecVolumeChanged(void)
{
    globals->sec_VolumeState = 0;
}

/* The library is RTF_AFTERDOS: a packet may arrive (boot-time mount) before
 * it is initialised, so keep trying while it is resident but not openable;
 * only give up when it is not resident at all. */
static void sfsSecOpenLibrary(void)
{
    if (!globals->sec_Base && !globals->sec_Checked)
    {
        if (FindResident(SECURITYNAME))
            globals->sec_Base = OpenLibrary(SECURITYNAME, 0);
        else
            globals->sec_Checked = TRUE;
    }
}

/* A volume came online: tell security.library to rescan (key files) */
void sfsSecVolumeOnline(void)
{
    sfsSecOpenLibrary();
    globals->sec_VolumeState = 0;
    if (globals->sec_Base)
        secFSRendezVous();
}

BOOL sfsSecIsActive(void)
{
    return globals->sec_Active;
}

static BOOL volumeSecured(void)
{
    if (globals->sec_VolumeState == 0)
    {
        UBYTE name[64];
        BOOL secured = FALSE;

        name[0] = '\0';
        if (globals->volumenode)
            copybstrasstr(globals->volumenode->dl_Name, name, sizeof(name) - 1);
        secured = secIsVolumeSecured(name[0] ? name : NULL, DOSTYPE_ID);
        if (!secured && globals->devnode)
        {
            copybstrasstr(globals->devnode->dn_Name, name, sizeof(name) - 1);
            secured = secIsVolumeSecured(name, DOSTYPE_ID);
        }
        globals->sec_VolumeState = secured ? 1 : 2;
        _DEBUG("SFS: volume '%s' %s multi-user\n", name, secured ? "IS" : "is NOT");
    }
    /* Negative answers are not cached: the server may still be scanning */
    if (globals->sec_VolumeState == 2)
        globals->sec_VolumeState = 0;
    return globals->sec_VolumeState == 1;
}

void sfsSecBeginPacket(struct DosPacket *dp)
{
    globals->sec_Active = FALSE;
    globals->sec_CurOwner = NULL;
    globals->sec_CurOwnerId = secOWNER_NOBODY;
    globals->sec_CurDefProt = 0;

    sfsSecOpenLibrary();
    if (!globals->sec_Base || !secIsConfigured() || !volumeSecured())
        return;

    globals->sec_Active = TRUE;
    globals->sec_CurOwner = secGetPktOwner(dp);
    globals->sec_CurOwnerId = secExtOwner2ULONG((struct secExtOwner *)globals->sec_CurOwner);
    globals->sec_CurDefProt = secGetPktDefProtection(dp);
}

void sfsSecEndPacket(void)
{
    if (globals->sec_CurOwner)
    {
        secFreeExtOwner((struct secExtOwner *)globals->sec_CurOwner);
        globals->sec_CurOwner = NULL;
    }
    globals->sec_Active = FALSE;
}

static void getObjectOwnerProt(struct fsObject *o, ULONG *owner, LONG *prot)
{
    if (BE2L(o->be_objectnode) == ROOTNODE)
    {
        *owner = SFS_ROOT_OWNER;
        *prot = SFS_ROOT_PROTECTION;
    }
    else
    {
        *owner = ((ULONG)BE2W(o->be_owneruid) << 16) | BE2W(o->be_ownergid);
        /* SFS stores the owner bits inverted (set = allowed) */
        *prot = BE2L(o->be_protection) ^ SFS_OWNERBITS;
    }
}

LONG sfsSecCheckObject(struct fsObject *o, LONG access)
{
    ULONG owner;
    LONG prot, res;

    if (!globals->sec_Active)
        return 0;

    getObjectOwnerProt(o, &owner, &prot);
    res = secAccess_Control(secAC_FILESYSTEM_CONTEXT, &globals->mytask->pr_MsgPort,
                            (struct secExtOwner *)globals->sec_CurOwner, owner, prot, access);
    if (res == secAC_PERMISSION_GRANTED)
        return 0;

    _DEBUG("SFS: access %ld to object %ld (owner %08lx prot %08lx) denied for %08lx\n",
           (long)access, (long)BE2L(o->be_objectnode), (unsigned long)owner, (unsigned long)prot, (unsigned long)globals->sec_CurOwnerId);

    if (access & SFS_ACCESS_DELETE)
        return ERROR_DELETE_PROTECTED;
    if (access & SFS_ACCESS_WRITE)
        return ERROR_WRITE_PROTECTED;
    return ERROR_READ_PROTECTED;
}

LONG sfsSecCheckProperty(struct fsObject *o)
{
    ULONG owner;
    LONG prot;

    if (!globals->sec_Active)
        return 0;

    getObjectOwnerProt(o, &owner, &prot);
    if (secGetRelationshipA((struct secExtOwner *)globals->sec_CurOwner, owner, NULL) & secRelF_PROPERTY_ACCESS)
        return 0;
    return ERROR_WRITE_PROTECTED;
}

/* Only root may give an object to somebody else */
LONG sfsSecCheckSetOwner(struct fsObject *o, ULONG newowner)
{
    LONG err;

    if ((err = sfsSecCheckProperty(o)))
        return err;
    if (globals->sec_Active &&
        !(secGetRelationshipA((struct secExtOwner *)globals->sec_CurOwner, newowner, NULL) & (secRelF_ROOT_UID | secRelF_UID_MATCH)))
        return ERROR_WRITE_PROTECTED;
    return 0;
}

LONG sfsSecCheckPath(struct ExtFileLock *lock, UBYTE *path, LONG access)
{
    struct CacheBuffer *cb;
    struct fsObject *o;
    LONG err;

    if (!globals->sec_Active)
        return 0;
    if ((err = locateobjectfromlock(lock, validatepath(path), &cb, &o)))
        return err;
    return sfsSecCheckObject(o, access);
}

LONG sfsSecCheckParentPath(struct ExtFileLock *lock, UBYTE *path, LONG access)
{
    struct CacheBuffer *cb;
    struct fsObject *o;
    LONG err;

    if (!globals->sec_Active)
        return 0;
    if ((err = locateparent(lock, validatepath(path), &cb, &o)))
        return err;
    return sfsSecCheckObject(o, access);
}

LONG sfsSecCheckPropertyPath(struct ExtFileLock *lock, UBYTE *path)
{
    struct CacheBuffer *cb;
    struct fsObject *o;
    LONG err;

    if (!globals->sec_Active)
        return 0;
    if ((err = locateobjectfromlock(lock, validatepath(path), &cb, &o)))
        return err;
    return sfsSecCheckProperty(o);
}

ULONG sfsSecNewOwner(void)
{
    return globals->sec_Active ? globals->sec_CurOwnerId : 0;
}

/* The stored (owner bits inverted) protection for a new object */
ULONG sfsSecNewStoredProtection(ULONG def)
{
    return globals->sec_Active ? ((ULONG)globals->sec_CurDefProt ^ SFS_OWNERBITS) : def;
}
