/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: afs multi-user support: ownership and protection enforcement via
          security.library, following the muFS filesystem contract
          (secGetPktOwner() on packet receipt, secAccess_Control() for
          access decisions, creator inheritance for new objects).
*/

#ifdef __AROS__

#include <proto/exec.h>
#include <proto/dos.h>

#include <dos/filesystemids.h>
#include <libraries/security.h>

#include "os.h"
#include "afshandler.h"
#include "volumes.h"
#include "cache.h"
#include "afsblocks.h"
#include "filehandles1.h"
#include "filehandles2.h"
#include "misc.h"
#include "validator.h"
#include "afs_security.h"

#define secBase ((struct Library *)afsbase->ab_SecBase)
#include <proto/security.h>

/* The root directory has no owner/protection words (they are bitmap
 * pointers in the root block): it belongs to the system, everybody may
 * read/list it, only root may change it. */
#define AFS_ROOT_OWNER          secOWNER_SYSTEM
#define AFS_ROOT_PROTECTION     (FIBF_GRP_READ | FIBF_GRP_EXECUTE | FIBF_OTR_READ | FIBF_OTR_EXECUTE)

static inline BOOL isSecVolume(struct Volume *volume)
{
    return (volume->dostype == ID_DOS_muFS_DISK);
}

BOOL afsSecIsActive(struct AFSBase *afsbase)
{
    return afsbase->ab_SecActive;
}

/*
 * Called for every packet before it is dispatched: find out who sent it.
 */
void afsSecBeginPacket(struct AFSBase *afsbase, struct Volume *volume, struct DosPacket *dp)
{
    afsbase->ab_SecActive = FALSE;
    afsbase->ab_CurOwner = NULL;
    afsbase->ab_CurOwnerId = secOWNER_NOBODY;
    afsbase->ab_CurDefProt = 0;

    /* The library is RTF_AFTERDOS: a packet may arrive (boot-time mount)
     * before it is initialised, so keep trying while it is resident but
     * not yet openable; only give up when it is not resident at all. */
    if (!afsbase->ab_SecBase && !afsbase->ab_SecChecked)
    {
        if (FindResident(SECURITYNAME))
            afsbase->ab_SecBase = OpenLibrary(SECURITYNAME, 0);
        else
            afsbase->ab_SecChecked = TRUE;
        D(bug("[afs] security.library @ %p\n", afsbase->ab_SecBase));
    }
    if (!afsbase->ab_SecBase || !volume || !isSecVolume(volume) || !secIsConfigured())
        return;

    afsbase->ab_SecActive = TRUE;
    afsbase->ab_CurPort = &((struct Process *)FindTask(NULL))->pr_MsgPort;
    afsbase->ab_CurOwner = secGetPktOwner(dp);
    afsbase->ab_CurOwnerId = secExtOwner2ULONG((struct secExtOwner *)afsbase->ab_CurOwner);
    afsbase->ab_CurDefProt = secGetPktDefProtection(dp);
}

void afsSecEndPacket(struct AFSBase *afsbase)
{
    if (afsbase->ab_CurOwner)
    {
        secFreeExtOwner((struct secExtOwner *)afsbase->ab_CurOwner);
        afsbase->ab_CurOwner = NULL;
    }
    afsbase->ab_SecActive = FALSE;
}

/* Owner/protection of a header block, with the root directory special-cased */
static void getBlockOwnerProt(struct Volume *volume, struct BlockCache *bb, ULONG *owner, LONG *prot)
{
    if (OS_BE2LONG(bb->buffer[BLK_SECONDARY_TYPE(volume)]) == ST_ROOT)
    {
        *owner = AFS_ROOT_OWNER;
        *prot = AFS_ROOT_PROTECTION;
    }
    else
    {
        *owner = OS_BE2LONG(bb->buffer[BLK_OWNER(volume)]);
        *prot = OS_BE2LONG(bb->buffer[BLK_PROTECT(volume)]);
    }
}

/*
 * May the sender of the current packet access the object in 'bb'?
 * Returns 0 or a DOS error code.
 */
LONG afsSecCheckBlock(struct AFSBase *afsbase, struct Volume *volume, struct BlockCache *bb, LONG access)
{
    ULONG owner;
    LONG prot, res;

    if (!afsbase->ab_SecActive)
        return 0;

    getBlockOwnerProt(volume, bb, &owner, &prot);
    res = secAccess_Control(secAC_FILESYSTEM_CONTEXT, afsbase->ab_CurPort,
                            (struct secExtOwner *)afsbase->ab_CurOwner, owner, prot, access);
    if (res == secAC_PERMISSION_GRANTED)
        return 0;

    D(bug("[afs] access %ld to block %u (owner %08lx prot %08lx) denied for %08lx\n",
          (long)access, bb->blocknum, (unsigned long)owner, (unsigned long)prot, (unsigned long)afsbase->ab_CurOwnerId));

    if (access & AFS_ACCESS_DELETE)
        return ERROR_DELETE_PROTECTED;
    if (access & AFS_ACCESS_WRITE)
        return ERROR_WRITE_PROTECTED;
    return ERROR_READ_PROTECTED;
}

/*
 * May the sender change the properties (protection, comment, date, owner,
 * name) of the object? Root, the owner, or anybody for unowned objects.
 */
LONG afsSecCheckProperty(struct AFSBase *afsbase, struct Volume *volume, struct BlockCache *bb)
{
    ULONG owner;
    LONG prot;

    if (!afsbase->ab_SecActive)
        return 0;

    getBlockOwnerProt(volume, bb, &owner, &prot);
    if (secGetRelationshipA((struct secExtOwner *)afsbase->ab_CurOwner, owner, NULL) & secRelF_PROPERTY_ACCESS)
        return 0;

    D(bug("[afs] property access to block %u (owner %08lx) denied for %08lx\n",
          bb->blocknum, (unsigned long)owner, (unsigned long)afsbase->ab_CurOwnerId));
    return ERROR_WRITE_PROTECTED;
}

/* Access check on a named object (ERROR_OBJECT_NOT_FOUND if it does not exist) */
LONG afsSecCheckName(struct AFSBase *afsbase, struct AfsHandle *dirah, CONST_STRPTR name, LONG access)
{
    struct BlockCache *bb;
    ULONG block;
    SIPTR error = 0;

    if (!afsbase->ab_SecActive)
        return 0;
    if (!(bb = findBlock(afsbase, dirah, name, &block, &error)))
        return error ? error : ERROR_OBJECT_NOT_FOUND;
    return afsSecCheckBlock(afsbase, dirah->volume, bb, access);
}

/* Access check on the directory that will hold 'name' */
LONG afsSecCheckParent(struct AFSBase *afsbase, struct AfsHandle *dirah, CONST_STRPTR name, LONG access)
{
    struct BlockCache *bb;
    UBYTE fname[34];
    SIPTR error = 0;

    if (!afsbase->ab_SecActive)
        return 0;
    if (!(bb = getDirBlockBuffer(afsbase, dirah, name, fname, &error)))
        return error ? error : ERROR_OBJECT_NOT_FOUND;
    return afsSecCheckBlock(afsbase, dirah->volume, bb, access);
}

LONG afsSecCheckNameProperty(struct AFSBase *afsbase, struct AfsHandle *dirah, CONST_STRPTR name)
{
    struct BlockCache *bb;
    ULONG block;
    SIPTR error = 0;

    if (!afsbase->ab_SecActive)
        return 0;
    if (!(bb = findBlock(afsbase, dirah, name, &block, &error)))
        return error ? error : ERROR_OBJECT_NOT_FOUND;
    return afsSecCheckProperty(afsbase, dirah->volume, bb);
}

/* Owner word for a newly created object */
ULONG afsSecNewOwner(struct AFSBase *afsbase)
{
    return afsbase->ab_SecActive ? afsbase->ab_CurOwnerId : 0;
}

/* Protection bits for a newly created object */
ULONG afsSecNewProtection(struct AFSBase *afsbase, ULONG def)
{
    return afsbase->ab_SecActive ? (ULONG)afsbase->ab_CurDefProt : def;
}

/*
 * ACTION_SET_OWNER
 */
ULONG afsSecSetOwner(struct AFSBase *afsbase, struct AfsHandle *dirah, CONST_STRPTR name, ULONG owner)
{
    struct BlockCache *bb;
    ULONG block;
    SIPTR error = 0;
    LONG err;

    D(bug("[afs] setOwner(ah,%s,%08lx)\n", name, (unsigned long)owner));
    if (0 == checkValid(afsbase, dirah->volume))
        return ERROR_DISK_WRITE_PROTECTED;
    if (!(bb = findBlock(afsbase, dirah, name, &block, &error)))
        return error ? error : ERROR_OBJECT_NOT_FOUND;
    if (OS_BE2LONG(bb->buffer[BLK_SECONDARY_TYPE(dirah->volume)]) == ST_ROOT)
        return ERROR_OBJECT_WRONG_TYPE;
    if ((err = afsSecCheckProperty(afsbase, dirah->volume, bb)))
        return err;
    /* Only root may give objects away */
    if (afsbase->ab_SecActive &&
        !(secGetRelationshipA((struct secExtOwner *)afsbase->ab_CurOwner, owner, NULL) & (secRelF_ROOT_UID | secRelF_UID_MATCH)))
        return ERROR_WRITE_PROTECTED;
    bb->buffer[BLK_OWNER(dirah->volume)] = OS_LONG2BE(owner);
    return writeHeader(afsbase, dirah->volume, bb);
}

#endif /* __AROS__ */
