/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: ram-handler multi-user support (security.library), see ram_security.h
*/

#include <aros/debug.h>
#include <libraries/security.h>
#include <proto/exec.h>
#include <proto/security.h>

#include "handler_protos.h"
#include "ram_security.h"

#define secBase ((struct Library *)handler->sec_base)

/*
 * The library is RTF_AFTERDOS: RAM: is mounted (and receives packets) long
 * before it is initialised, so keep trying while it is resident but not
 * openable, and only give up when it is not resident at all.
 */
static void OpenSecurity(struct Handler *handler)
{
    if (handler->sec_base == NULL && !handler->sec_checked)
    {
        if (FindResident(SECURITYNAME))
        {
            handler->sec_base = OpenLibrary(SECURITYNAME, 0);
            D(if (handler->sec_base) bug("[ram] security.library @ %p\n", handler->sec_base);)
        }
        else
            handler->sec_checked = TRUE;
    }
}

BOOL ramSecIsPresent(struct Handler *handler)
{
    OpenSecurity(handler);
    return handler->sec_base != NULL;
}

BOOL ramSecIsActive(struct Handler *handler)
{
    return handler->sec_active;
}

/* Called for every packet before it is dispatched: find out who sent it */
void ramSecBeginPacket(struct Handler *handler, struct DosPacket *packet)
{
    handler->sec_active = FALSE;

    OpenSecurity(handler);
    if (handler->sec_base == NULL || !secIsConfigured())
        return;

    handler->sec_active  = TRUE;
    handler->sec_owner   = secGetPktOwner(packet);
    handler->sec_ownerid = secExtOwner2ULONG((struct secExtOwner *)handler->sec_owner);
    handler->sec_defprot = secGetPktDefProtection(packet);
}

void ramSecEndPacket(struct Handler *handler)
{
    if (handler->sec_owner != NULL)
    {
        secFreeExtOwner((struct secExtOwner *)handler->sec_owner);
        handler->sec_owner = NULL;
    }
    handler->sec_active = FALSE;
}

/*
 * May the sender of the current packet access the object?
 * Returns 0 or a DOS error code.
 */
LONG ramSecCheckObject(struct Handler *handler, struct Object *object, LONG access)
{
    LONG res;

    if (!handler->sec_active || object == NULL)
        return 0;

    object = GetRealObject(object);
    res = secAccess_Control(secAC_FILESYSTEM_CONTEXT, handler->proc_port,
                            (struct secExtOwner *)handler->sec_owner,
                            object->owner, object->protection, access);
    if (res == secAC_PERMISSION_GRANTED)
        return 0;

    D(bug("[ram] access %ld to '%s' (owner %08lx prot %08lx) denied for %08lx\n",
          (long)access, ((struct Node *)object)->ln_Name, (unsigned long)object->owner,
          (unsigned long)object->protection, (unsigned long)handler->sec_ownerid);)

    if (access & RAM_ACCESS_DELETE)
        return ERROR_DELETE_PROTECTED;
    if (access & RAM_ACCESS_WRITE)
        return ERROR_WRITE_PROTECTED;
    return ERROR_READ_PROTECTED;
}

/*
 * May the sender change the properties (protection, comment, date, name)
 * of the object? Root, the owner, or anybody for unowned objects.
 */
LONG ramSecCheckProperty(struct Handler *handler, struct Object *object)
{
    if (!handler->sec_active || object == NULL)
        return 0;

    object = GetRealObject(object);
    if (secGetRelationshipA((struct secExtOwner *)handler->sec_owner, object->owner, NULL) & secRelF_PROPERTY_ACCESS)
        return 0;

    D(bug("[ram] property access to '%s' (owner %08lx) denied for %08lx\n",
          ((struct Node *)object)->ln_Name, (unsigned long)object->owner, (unsigned long)handler->sec_ownerid);)
    return ERROR_WRITE_PROTECTED;
}

/* Owner of a newly created object: the creator, or nobody outside multi-user mode */
ULONG ramSecNewOwner(struct Handler *handler)
{
    return handler->sec_active ? handler->sec_ownerid : secOWNER_NOBODY;
}

/* Protection of a newly created object: the creator's umask, or none */
ULONG ramSecNewProtection(struct Handler *handler)
{
    return handler->sec_active ? (ULONG)handler->sec_defprot : 0;
}

/* Owner fields for Examine()/ExAll(); left untouched without the library */
void ramSecFillOwner(struct Handler *handler, struct Object *object, UWORD *uid, UWORD *gid)
{
    if (handler->sec_base != NULL)
    {
        object = GetRealObject(object);
        *uid = object->owner >> 16;
        *gid = object->owner & 0xffff;
    }
}

/*
 * ACTION_SET_OWNER: only root, or the owner (for unowned objects: anybody),
 * may change the owner.
 */
BOOL CmdSetOwner(struct Handler *handler, struct Lock *lock, const TEXT *name, ULONG owner)
{
    LONG error = 0;
    struct Object *object;

    object = GetHardObject(handler, lock, name, NULL);
    if (object == NULL)
        error = IoErr();
    else if (handler->locked)
        error = ERROR_DISK_WRITE_PROTECTED;
    else if (handler->sec_active)
    {
        object = GetRealObject(object);
        if (!(secGetRelationshipA((struct secExtOwner *)handler->sec_owner, object->owner, NULL)
              & (secRelF_ROOT_UID | secRelF_UID_MATCH)))
            error = ERROR_WRITE_PROTECTED;
    }

    if (error == 0)
    {
        object = GetRealObject(object);
        object->owner = owner;
        NotifyAll(handler, object, TRUE);
    }

    SetIoErr(error);
    return error == 0;
}
