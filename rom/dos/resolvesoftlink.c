/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.

    Desc:
*/

#define DLINK(x)

#include <aros/debug.h>
#include <proto/dos.h>

#include "dos_intern.h"

/*
 * Resolve a softlink.
 * Returns AllocVec()ed buffer with softlink contents.
 */
STRPTR ResolveSoftlink(BPTR lock, struct DevProc *dvp, CONST_STRPTR name, struct DosLibrary *DOSBase)
{
    ULONG buffer_size = 256;
    STRPTR softname;
    LONG continue_loop;
    LONG written;

    DLINK(bug("[Softlink] Resolving softlink %s...\n", name));

    do
    {
        continue_loop = FALSE;

        if (!(softname = AllocVec(buffer_size, MEMF_PUBLIC|MEMF_CLEAR)))
        {
            SetIoErr(ERROR_NO_FREE_STORE);
            break;
        }

        if (lock) {
            struct FileLock *fl = BADDR(lock);
            written = ReadLink(fl->fl_Task, lock, name, softname, buffer_size);
        }
        else {
            written = ReadLink(dvp->dvp_Port, dvp->dvp_Lock, name, softname, buffer_size);
        }

        switch (written)
        {
        case -1:
            /* An error occured */
            DLINK(bug("[Softlink] Error %d reading softlink\n", IoErr()));
            break;

        case -2:
            /* If there's not enough space in the buffer, increase it and try again */
            continue_loop = TRUE;
            buffer_size <<= 1;

            DLINK(bug("[Softlink] Increased buffer size up to %u\n", buffer_size));
            break;

        default:
            /* All OK */
            DLINK(bug("[Softlink] Resolved path: %s\n", softname));
            return softname;
        }

        FreeVec(softname);
    }
    while(continue_loop);

    return NULL;
}

