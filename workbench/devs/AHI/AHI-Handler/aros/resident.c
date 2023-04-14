#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/resident.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>
#include <aros/genmodule.h>
#include <dos/dos.h>

#undef SysBase
#undef OOPBase
#undef UtilityBase

#include <proto/exec.h>
#include <proto/alib.h>

#include "version.h"

#define LIBBASESIZE (sizeof(struct Library))

extern int End(void);
extern const APTR FuncTable[];

extern const char LibName[];
extern const char ID[];

AROS_UFP3 (struct Library *, InitLib,
    AROS_UFPA(struct Library *, LIBBASE, D0),
    AROS_UFPA(BPTR, segList, A0),
    AROS_UFPA(struct ExecBase *, sysBase, A6)
);
__section(".text.romtag") struct Resident const ROMTag =
{
    RTC_MATCHWORD,
    (struct Resident *)&ROMTag,
    (APTR)&End,
    RTF_COLDSTART,
    VERSION,
    NT_RESOURCE,
    -1,
    (CONST_STRPTR)&LibName[0],
    (CONST_STRPTR)&ID[6],
    (APTR)InitLib
#if defined(__AROS__)
    , 0, NULL
#endif
};

__section(".text.romtag") const char LibName[] = "AHI-Handler";

THIS_PROGRAM_HANDLES_SYMBOLSET(INIT)
THIS_PROGRAM_HANDLES_SYMBOLSET(EXIT)
DECLARESET(INIT)
DECLARESET(EXIT)
THIS_PROGRAM_HANDLES_SYMBOLSET(PROGRAM_ENTRIES)
DECLARESET(PROGRAM_ENTRIES)

#include <aros/system.h>
#include <proto/arossupport.h>
#include <proto/expansion.h>

LONG handler(struct ExecBase *sysBase);
extern const LONG __aros_libreq_SysBase __attribute__((weak));

__startup AROS_PROCH(Handler, argptr, argsize, SysBase)
{
    AROS_PROCFUNC_INIT

    LONG ret = RETURN_FAIL;

    if (!SysBase || SysBase->LibNode.lib_Version < __aros_libreq_SysBase)
        return ERROR_INVALID_RESIDENT_LIBRARY;
    if (set_call_funcs(SETNAME(INIT), 1, 1)) {
        ret = handler(SysBase);
        set_call_funcs(SETNAME(EXIT), -1, 0);
    }

    return ret;

    AROS_PROCFUNC_EXIT
}

extern const LONG __aros_libreq_SysBase __attribute__((weak));

AROS_UFH3 (struct Library *, InitLib,
    AROS_UFHA(struct Library *, LIBBASE, D0),
    AROS_UFHA(BPTR, segList, A0),
    AROS_UFHA(struct ExecBase *, sysBase, A6)
)
{
    AROS_USERFUNC_INIT

    return LIBBASE;

    AROS_USERFUNC_EXIT
}

DEFINESET(INIT)
DEFINESET(EXIT)

