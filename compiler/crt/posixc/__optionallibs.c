/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#define __NOBLIBBASE__

#include "__posixc_intbase.h"
#include "__optionallibs.h"

/* Internal function __libfindandopen will only open a library when it is
   already in the list of open libraries
*/
static struct Library *__libfindandopen(const char *libname, int version)
{
    struct Node *found;

    Forbid();
    found = FindName(&SysBase->LibList, libname);
    Permit();

    return (found != NULL) ? OpenLibrary(libname, version) : NULL;
}

int __usergroup_available(struct PosixCIntBase *PosixCBase)
{
    if (PosixCBase->PosixCUserGroupBase == NULL)
        PosixCBase->PosixCUserGroupBase = OpenLibrary("usergroup.library", 0);

    return PosixCBase->PosixCUserGroupBase != NULL;
}

int __optionallibs_close(struct PosixCIntBase *PosixCBase)
{
    if (PosixCBase->PosixCUserGroupBase) {
        CloseLibrary(PosixCBase->PosixCUserGroupBase);
        PosixCBase->PosixCUserGroupBase = NULL;
    }
}

static int __close_posixcoptional(struct PosixCIntBase *PosixCBase)
{
    // FIXME is this 0 or 1? Does AROS decrease it before calling libClose?
    if(PosixCBase->PosixCBase.lib.lib_OpenCnt == 0) {
        return __optionallibs_close(PosixCBase);
    }
    return(TRUE);
}

ADD2CLOSELIB(__close_posixcoptional, 0)
