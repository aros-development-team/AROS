/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Expansion Resident and initialization.
    Lang: english
*/

#include <aros/config.h>
#include <aros/symbolsets.h>
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/alerts.h>
#include <exec/libraries.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/expansion.h>

#include "expansion_intern.h"

#include LC_LIBDEFS_FILE

static const UBYTE version[];

/* symbol provided by genmodule ...*/
extern int LIBEND(void);

AROS_UFP3S(LIBBASETYPEPTR, GM_UNIQUENAME(init),
    AROS_UFHA(ULONG, dummy, D0),
    AROS_UFHA(BPTR, segList, A0),
    AROS_UFPA(struct ExecBase *, sysBase, A6));

__section(".text.romtag") struct Resident const GM_UNIQUENAME(ROMTag) =
{
    RTC_MATCHWORD,
    (struct Resident *)&GM_UNIQUENAME(ROMTag),
    (APTR)&LIBEND,
    RESIDENTFLAGS,
    VERSION_NUMBER,
    NT_LIBRARY,
    RESIDENTPRI,
    MOD_NAME_STRING,
    (STRPTR)&version[6],
    &GM_UNIQUENAME(init),
};

static const UBYTE version[] = VERSION_STRING;

extern const APTR GM_UNIQUENAME(FuncTable)[];

THIS_PROGRAM_HANDLES_SYMBOLSET(INIT)
THIS_PROGRAM_HANDLES_SYMBOLSET(INITLIB)
THIS_PROGRAM_HANDLES_SYMBOLSET(OPENLIB)
THIS_PROGRAM_HANDLES_SYMBOLSET(CLOSELIB)
DEFINESET(INIT)
DEFINESET(INITLIB)
DEFINESET(OPENLIB)
DEFINESET(CLOSELIB)

static void initExpBase(LIBBASETYPEPTR LIBBASE, BPTR segList)
{
    D(bug("[expansion] %s(0x%p)\n", __func__, LIBBASE);)

    /* initialize our header */
    LIBBASE->LibNode.lib_Node.ln_Name = MOD_NAME_STRING;
    LIBBASE->LibNode.lib_Node.ln_Type = NT_LIBRARY;
    LIBBASE->LibNode.lib_Node.ln_Pri  = RESIDENTPRI;
    LIBBASE->LibNode.lib_Version      = VERSION_NUMBER;
    LIBBASE->LibNode.lib_Revision     = REVISION_NUMBER;
    LIBBASE->LibNode.lib_IdString     = (char *)&version[6];
    LIBBASE->LibNode.lib_Flags        = LIBF_SUMUSED|LIBF_CHANGED;

    /* initialize internal resources */
    LIBBASE->ExecBase = SysBase;
    LIBBASE->SegList = (IPTR)segList;

    NEWLIST(&LIBBASE->MountList);
    NEWLIST(&LIBBASE->BoardList);

    InitSemaphore(&LIBBASE->BindSemaphore);
    InitSemaphore(&LIBBASE->BootSemaphore);
}

/*
 * The Init routine is intentionally written by hand, in order to reallocate
 * the base in fastmem (if/)when it becomes available.
 * Traditionaly in AmigaOS, the system comes up with only the chip memory detected.
 * it is the job of expansion.library to find and add the motherboard and any expansion
 * based fast memory, so it can be used by the system.
 */
AROS_UFH3S(LIBBASETYPEPTR, GM_UNIQUENAME(init),
    AROS_UFHA(ULONG, dummy, D0),
    AROS_UFHA(BPTR, segList, A0),
    AROS_UFHA(struct ExecBase *, sysBase, A6)
)
{
    AROS_USERFUNC_INIT

    LIBBASETYPEPTR LIBBASE;

    D(bug("[expansion] %s()\n", __func__));

    if (!SysBase)
        SysBase = sysBase;

    LIBBASE = (LIBBASETYPEPTR)MakeLibrary(GM_UNIQUENAME(FuncTable), NULL, NULL, sizeof(struct IntExpansionBase), BNULL);
    if (!LIBBASE)
        return NULL;

    D(bug("[expansion] %s: base allocated @ 0x%p\n", __func__, LIBBASE));

    initExpBase(LIBBASE, segList);

    LIBBASE->kernelBase = OpenResource("kernel.resource");

    if (!set_call_funcs(SETNAME(INIT), 1, 1))
    {
        FreeMem((char *)LIBBASE - LIBBASE->LibNode.lib_NegSize, LIBBASE->LibNode.lib_NegSize + LIBBASE->LibNode.lib_PosSize);
        return NULL;
    }

    /* call platform-specific init code (if any) */
    if (!set_call_libfuncs(SETNAME(INITLIB), 1, 1, LIBBASE))
    {
        FreeMem((char *)LIBBASE - LIBBASE->LibNode.lib_NegSize, LIBBASE->LibNode.lib_NegSize + LIBBASE->LibNode.lib_PosSize);
        return NULL;
    }

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
    /* see what expansion hardware we can detect. */
    ConfigChain((APTR)E_EXPANSIONBASE);

    /* attempt to move the library base to fast mem if any is now available... */
    if (((TypeOfMem(LIBBASE) & (MEMF_FAST|MEMF_CHIP)) == MEMF_CHIP) && (AvailMem(MEMF_FAST) > (LIBBASE->LibNode.lib_NegSize + LIBBASE->LibNode.lib_PosSize)))
    {
        LIBBASETYPEPTR tmpbase;
        D(bug("[expansion] %s: attempting to relocate to FastMem\n", __func__));

        tmpbase = (LIBBASETYPEPTR)MakeLibrary(GM_UNIQUENAME(FuncTable), NULL, NULL, sizeof(struct IntExpansionBase), BNULL);
        if (tmpbase)
        {
            LIBBASETYPEPTR releasebase = tmpbase;

            if ((TypeOfMem(tmpbase) & (MEMF_FAST|MEMF_CHIP|MEMF_VIRTUAL)) == MEMF_FAST)
            {
                struct Node *tmpN1, *tmpN2;
                D(bug("[expansion] %s: new-base allocated @ 0x%p\n", __func__, tmpbase));

                releasebase = LIBBASE;
                LIBBASE = tmpbase;
                initExpBase(LIBBASE, segList);
                LIBBASE->kernelBase = releasebase->kernelBase;
                ForeachNodeSafe(&releasebase->MountList, tmpN1, tmpN2)
                {
                    Remove(tmpN1);
                    AddTail(&LIBBASE->MountList, tmpN1);
                }
                ForeachNodeSafe(&releasebase->BoardList, tmpN1, tmpN2)
                {
                    Remove(tmpN1);
                    AddTail(&LIBBASE->BoardList, tmpN1);
                }
            }
            D(bug("[expansion] %s: free unwanted base @ 0x%p\n", __func__, releasebase));

            FreeMem((char *)releasebase - releasebase->LibNode.lib_NegSize, releasebase->LibNode.lib_NegSize + releasebase->LibNode.lib_PosSize);
        }
    }
#endif
    D(bug("[expansion] %s: init done\n", __func__));

    if (LIBBASE)
    {
        D(bug("[expansion] %s: adding expansion.library to the system\n", __func__));
        AddLibrary(&LIBBASE->LibNode);
    }
    D(bug("[expansion] %s: returning ExpansionBase @ 0x%p\n", __func__, LIBBASE));

    return LIBBASE;

    AROS_USERFUNC_EXIT
}


/*
 * open and close routines.   we never auto-expunge, because
 * if we ever do this, we won't be able to come up again.
 */
AROS_LH1(struct ExpansionBase *, OpenLib,
         AROS_LHA(ULONG, version, D0),
         LIBBASETYPEPTR, LIBBASE, 1, Expansion)
{
    AROS_LIBFUNC_INIT

    D(bug("[expansion] %s()\n", __func__));

    if ( set_call_libfuncs(SETNAME(OPENLIB), 1, 1, LIBBASE) )
    {
        /* one more opener. */
        ((struct Library *)LIBBASE)->lib_OpenCnt++;
        ((struct Library *)LIBBASE)->lib_Flags &= ~LIBF_DELEXP;
        return (struct ExpansionBase *)LIBBASE;
    }

    return (struct ExpansionBase *)NULL;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, CloseLib,
         LIBBASETYPEPTR, LIBBASE, 2, Expansion)
{
    AROS_LIBFUNC_INIT

    D(bug("[expansion] %s()\n", __func__));

    /* one less opener. */
    LIBBASE->LibNode.lib_OpenCnt--;
    set_call_libfuncs(SETNAME(CLOSELIB), -1, 0, LIBBASE);

    return BNULL;

    AROS_LIBFUNC_EXIT
}
