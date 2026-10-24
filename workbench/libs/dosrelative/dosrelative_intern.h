#ifndef DOSRELATIVE_INTERN_H
#define DOSRELATIVE_INTERN_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Internal definitions for dosrelative.library
*/

#include <aros/debug.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <string.h>

struct DosRelativeBase
{
    struct Library     drb_Lib;
    struct DosLibrary *drb_DosBase;
};

/*
 * Inside the library functions the base parameter is named
 * DOSRelativeBase; route the dos.library interface through the base
 * we opened at init. (proto/dos.h is included above, so its libbase
 * declaration is not affected by this macro.)
 */
#define DOSBase (DOSRelativeBase->drb_DosBase)

/* Maximum level of soft links nesting, as in dos.library */
#define DOSREL_MAX_SOFT_LINK_NESTING 16

/*
 * A name that carries a volume, device or assign specifier (or the
 * lone ':' root reference) does not depend on a base lock: it is
 * resolved exactly as the plain dos.library call would resolve it.
 */
static inline BOOL dosrel_IsAbsolute(CONST_STRPTR name)
{
    return strchr(name, ':') != NULL;
}

/*
 * The handler to talk to for an object named relative to a lock.
 * All packets for relative names go to the port that owns the lock.
 */
static inline struct MsgPort *dosrel_LockPort(BPTR lock)
{
    return ((struct FileLock *)BADDR(lock))->fl_Task;
}

/* dosrelative_support.c */
BSTR   dosrel_C2BSTR(struct DosRelativeBase *DOSRelativeBase, CONST_STRPTR name);
void   dosrel_FreeBSTR(struct DosRelativeBase *DOSRelativeBase, BSTR bstr);
STRPTR dosrel_ReadSoftlink(struct DosRelativeBase *DOSRelativeBase, BPTR lock, CONST_STRPTR name);
SIPTR  dosrel_SendNamePkt(struct DosRelativeBase *DOSRelativeBase, BPTR lock, CONST_STRPTR name,
                          LONG action, SIPTR arg1, SIPTR arg2, BOOL nameFirst);

#endif /* DOSRELATIVE_INTERN_H */
