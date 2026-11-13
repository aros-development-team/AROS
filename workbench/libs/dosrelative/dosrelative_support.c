/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Support routines shared by the dosrelative.library functions.
*/

#include <exec/memory.h>
#include <dos/bptr.h>

#include "dosrelative_intern.h"

/*
 * Build a BSTR for a handler packet from a C string, using the
 * implementation-independent accessors from <dos/bptr.h>. On a build
 * with traditional (length-prefixed) BSTRs the name is limited to 255
 * characters, as it is for every other dos.library packet.
 * The result is released with dosrel_FreeBSTR().
 */
BSTR dosrel_C2BSTR(struct DosRelativeBase *DOSRelativeBase, CONST_STRPTR name)
{
    ULONG len = strlen(name);
    APTR  mem;
    BSTR  bstr;

#ifndef AROS_FAST_BSTR
    if (len > 255)
        len = 255;
#endif

    mem = AllocVec(AROS_BSTR_MEMSIZE4LEN(len), MEMF_PUBLIC);
    if (mem == NULL)
        return BNULL;

    bstr = MKBADDR(mem);
    CopyMem((APTR)name, AROS_BSTR_ADDR(bstr), len);
    AROS_BSTR_setstrlen(bstr, len);

    return bstr;
}

void dosrel_FreeBSTR(struct DosRelativeBase *DOSRelativeBase, BSTR bstr)
{
    if (bstr != BNULL)
        FreeVec(BADDR(bstr));
}

/*
 * Read the target of a soft link named relative to lock.
 * Returns an AllocVec()ed buffer with the link contents, or NULL with
 * IoErr() set. The target is itself a path relative to lock, unless it
 * names a volume (contains ':').
 */
STRPTR dosrel_ReadSoftlink(struct DosRelativeBase *DOSRelativeBase, BPTR lock, CONST_STRPTR name)
{
    struct MsgPort *port = dosrel_LockPort(lock);
    ULONG buffer_size = 256;
    STRPTR softname;
    LONG written;

    D(bug("[DosRelative] Resolving softlink '%s'\n", name));

    for (;;)
    {
        softname = AllocVec(buffer_size, MEMF_PUBLIC | MEMF_CLEAR);
        if (softname == NULL)
        {
            SetIoErr(ERROR_NO_FREE_STORE);
            return NULL;
        }

        written = ReadLink(port, lock, name, softname, buffer_size);

        if (written == -2)
        {
            /* Not enough space in the buffer; grow it and try again */
            FreeVec(softname);
            buffer_size <<= 1;
            continue;
        }

        if (written == -1)
        {
            D(bug("[DosRelative] Error %d reading softlink\n", IoErr()));
            FreeVec(softname);
            return NULL;
        }

        D(bug("[DosRelative] Resolved path: '%s'\n", softname));
        return softname;
    }
}

/*
 * Send a packet that names an object relative to lock to the lock's
 * handler. The packet argument layout follows the two shapes used by
 * dos.library:
 *
 *   nameFirst == TRUE : (lock, name, arg1, arg2)
 *                       ACTION_LOCATE_OBJECT, ACTION_CREATE_DIR,
 *                       ACTION_DELETE_OBJECT, ACTION_MAKE_LINK
 *   nameFirst == FALSE: (NULL, lock, name, arg1)
 *                       ACTION_SET_COMMENT, ACTION_SET_DATE,
 *                       ACTION_SET_OWNER, ACTION_SET_PROTECT
 *
 * Returns the packet's primary result, with IoErr() set from its
 * secondary result by DoPkt().
 */
SIPTR dosrel_SendNamePkt(struct DosRelativeBase *DOSRelativeBase, BPTR lock, CONST_STRPTR name,
                         LONG action, SIPTR arg1, SIPTR arg2, BOOL nameFirst)
{
    BSTR  bname = dosrel_C2BSTR(DOSRelativeBase, name);
    SIPTR res;

    if (bname == BNULL)
    {
        SetIoErr(ERROR_NO_FREE_STORE);
        return DOSFALSE;
    }

    if (nameFirst)
        res = DoPkt(dosrel_LockPort(lock), action, (SIPTR)lock, (SIPTR)bname, arg1, arg2, 0);
    else
        res = DoPkt(dosrel_LockPort(lock), action, (SIPTR)NULL, (SIPTR)lock, (SIPTR)bname, arg1, 0);

    dosrel_FreeBSTR(DOSRelativeBase, bname);

    return res;
}
