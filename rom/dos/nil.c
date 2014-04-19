/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Special handling for NIL: locks and file handles
    Lang: English
*/

#include "dos_intern.h"

SIPTR handleNIL(LONG action, SIPTR arg1, SIPTR arg2, SIPTR arg3)
{
    switch(action)
    {
    case(ACTION_FINDUPDATE):
    case(ACTION_FINDINPUT):
    case(ACTION_FINDOUTPUT):
    {
        struct FileHandle * fh = BADDR(arg1);
        fh->fh_Type = BNULL;
        fh->fh_Interactive = DOSFALSE;/* NIL: is not considered interactive */
        return (SIPTR)MKBADDR(fh);
    }
    case(ACTION_INFO): return (SIPTR)0;
    case(ACTION_FREE_LOCK):
    {
        FreeMem((APTR)arg1, sizeof(struct FileLock));
        return (SIPTR)0;
    }
    case(ACTION_COPY_DIR_FH):
    {
        struct FileLock *fl = AllocMem(sizeof(struct FileLock), MEMF_PUBLIC | MEMF_CLEAR);
        fl->fl_Access = SHARED_LOCK;
        return (SIPTR)MKBADDR(fl);
    }
    case(ACTION_FH_FROM_LOCK):
    {
        struct FileHandle * fh = BADDR(arg2);
        fh->fh_Interactive = DOSFALSE;
        FreeMem((APTR)arg3, sizeof(struct FileLock));
        return (SIPTR)DOSTRUE;
    }
    case(ACTION_WRITE): return (SIPTR)arg3;
    case(ACTION_PARENT_FH): return (SIPTR)BNULL;
    default: return TRUE;
    }
}
