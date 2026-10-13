/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>

#define __NOBLIBBASE__

#include <resources/entropy.h>

#include <stdlib.h>
#include <exec/lists.h>

#include "__posixc_intbase.h"
#include "__optionallibs.h"

/* stdc internals, already used by posixc (mbsnrtowcs.c, wcsnrtombs.c): the
   program's exit list, which stdc's __callexitfuncs drains at exit. */
#include "__stdc_intbase.h"
#include "__exitfunc.h"

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

/*
 * usergroup.library holds a reference on this posixc base (it declares
 * `rellib posixc`), so the CLOSELIB-time release in __optionallibs_close can
 * never run while usergroup is open: genmodule only reaches the CLOSELIB set
 * once taskopencount is zero. The base then outlives the program, together
 * with upathbuf, and the next program run from the same Shell adopts it.
 *
 * So usergroup is released from the program's exit list instead, which
 * __stdc_program_end drains after main() and before the program closes its
 * libraries.
 *
 * It must be released LAST. getpwnam() and friends return pointers into
 * usergroup's own base (ugSecPw), which is freed on close; an exit handler
 * registered earlier in the program may still read one. The list is drained
 * with REMHEAD, so a handler that finds other nodes still queued re-inserts
 * itself at the TAIL and runs again after them. The node carries a pointer to
 * itself as its argument, so re-queueing needs no allocation during exit;
 * __callexitfuncs does not free nodes (they go with the program's pool).
 */
static void __usergroup_release(int status, void *arg)
{
    struct AtExitNode *self = (struct AtExitNode *)arg;
    struct StdCIntBase *StdCBase =
        (struct StdCIntBase *)__aros_getbase_StdCBase();
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    (void)status;

    if (!IsListEmpty((struct List *)&StdCBase->atexit_list)) {
        ADDTAIL((struct List *)&StdCBase->atexit_list, (struct Node *)self);
        return;
    }

    if (PosixCBase->PosixCUserGroupBase) {
        CloseLibrary(PosixCBase->PosixCUserGroupBase);
        PosixCBase->PosixCUserGroupBase = NULL;
    }
}

/* Queue __usergroup_release on the program's exit list. Returns 1 when it
   is queued, 0 when the node cannot be allocated. */
static int __usergroup_queue_release(struct StdCIntBase *StdCBase)
{
    struct AtExitNode *aen = malloc(sizeof(*aen));

    if (aen == NULL)
        return 0;

    aen->node.ln_Type = AEN_ON;
    aen->func.on.fn = __usergroup_release;
    aen->func.on.arg = aen;
    ADDHEAD((struct List *)&StdCBase->atexit_list, (struct Node *)aen);

    return 1;
}

int __usergroup_available(struct PosixCIntBase *PosixCBase)
{
    if (PosixCBase->PosixCUserGroupBase == NULL) {
        struct StdCIntBase *StdCBase =
            (struct StdCIntBase *)__aros_getbase_StdCBase();

        PosixCBase->PosixCUserGroupBase = OpenLibrary("usergroup.library", 0);
        if (PosixCBase->PosixCUserGroupBase == NULL)
            return 0;

        /* StdCBase is the program's C library base, set by the posixc
           startup code linked into programs. Without it no exit list will
           be drained for this base, so the library is kept as before and is
           released only by __optionallibs_close; the reference cycle
           described above then remains for this base. */
        if (StdCBase == NULL)
            return 1;

        /* An open that cannot be paired with its release would keep the
           cycle, so it is undone and usergroup is reported unavailable. */
        if (!__usergroup_queue_release(StdCBase)) {
            CloseLibrary(PosixCBase->PosixCUserGroupBase);
            PosixCBase->PosixCUserGroupBase = NULL;
            return 0;
        }
    }

    return 1;
}

int __entropy_available(struct PosixCIntBase *PosixCBase)
{
    /* entropy.resource is a resource, not a library: it is obtained with
       OpenResource() and is never closed, so it is not handled in
       __optionallibs_close(). */
    if (PosixCBase->PosixCEntropyBase == NULL)
        PosixCBase->PosixCEntropyBase = OpenResource(ENTROPYNAME);

    return PosixCBase->PosixCEntropyBase != NULL;
}

int __dos64_available(struct PosixCIntBase *PosixCBase)
{
    if (PosixCBase->PosixCDOS64Base == NULL)
        PosixCBase->PosixCDOS64Base = OpenLibrary("dos64.library", 50);

    return PosixCBase->PosixCDOS64Base != NULL;
}

int __optionallibs_close(struct PosixCIntBase *PosixCBase)
{
    if (PosixCBase->PosixCUserGroupBase) {
        CloseLibrary(PosixCBase->PosixCUserGroupBase);
        PosixCBase->PosixCUserGroupBase = NULL;
    }
    if (PosixCBase->PosixCFDBase) {
        CloseLibrary(PosixCBase->PosixCFDBase);
        PosixCBase->PosixCFDBase = NULL;
    }
    if (PosixCBase->PosixCDOS64Base) {
        CloseLibrary(PosixCBase->PosixCDOS64Base);
        PosixCBase->PosixCDOS64Base = NULL;
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
