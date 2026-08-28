/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: System requester gate - private LVOs -120/-126
*/
#include "lowlevel_intern.h"

#include <aros/libcall.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <libraries/lowlevel.h>
#include <intuition/intuition.h>
#include <proto/exec.h>

/* EasyRequestArgs() is vector 98 in intuition.library */
#define EASYREQUESTARGS_SLOT 98
#define EASYREQUESTARGS_LVO  (-(EASYREQUESTARGS_SLOT) * LIB_VECTSIZE)

/* Replacement for intuition.library/EasyRequestArgs(), installed with
 * SetFunction() the first time requesters are disabled. While the disable
 * count is held it answers every requester immediately with 0 (the
 * rightmost gadget, normally "Cancel"), without any display; otherwise it
 * passes through to the original vector.
 *
 * The gate runs with A6 = IntuitionBase, and ROM modules cannot carry
 * writable globals, so the library base is looked up per call. Requesters
 * are rare enough that the lookup cost does not matter.
 */
AROS_LH4(LONG, llEasyRequestGate,
      AROS_LHA(struct Window *, window, A0),
      AROS_LHA(struct EasyStruct *, easyStruct, A1),
      AROS_LHA(ULONG *, idcmpPtr, A2),
      AROS_LHA(APTR, args, A3),
      struct IntuitionBase *, IntuitionBase, EASYREQUESTARGS_SLOT, LowLevel)
{
    AROS_LIBFUNC_INIT

    struct LowLevelBase *LowLevelBase;

    Forbid();
    LowLevelBase = (struct LowLevelBase *)FindName(&SysBase->LibList,
                                                   "lowlevel.library");
    Permit();

    if (LowLevelBase == NULL || LowLevelBase->ll_SysReqNest >= 0 ||
        LowLevelBase->ll_EasyRequestOrig == NULL)
        return 0;

    return AROS_CALL4(LONG, LowLevelBase->ll_EasyRequestOrig,
        AROS_LCA(struct Window *, window, A0),
        AROS_LCA(struct EasyStruct *, easyStruct, A1),
        AROS_LCA(ULONG *, idcmpPtr, A2),
        AROS_LCA(APTR, args, A3),
        struct IntuitionBase *, IntuitionBase);

    AROS_LIBFUNC_EXIT
}

/* Expunge-time cleanup: put intuition's vector back and drop our
 * reference. If something else SetFunction()ed EasyRequestArgs() after
 * us this un-hooks it too, but leaving the vector pointing into an
 * expunged library would be worse.
 */
VOID llSysReq_Cleanup(struct LowLevelBase *LowLevelBase)
{
    if (LowLevelBase->ll_EasyRequestOrig != NULL)
    {
        SetFunction(LowLevelBase->ll_IntuitionBase, EASYREQUESTARGS_LVO,
                    LowLevelBase->ll_EasyRequestOrig);
        LowLevelBase->ll_EasyRequestOrig = NULL;
    }
    if (LowLevelBase->ll_IntuitionBase != NULL)
    {
        CloseLibrary(LowLevelBase->ll_IntuitionBase);
        LowLevelBase->ll_IntuitionBase = NULL;
    }
}

/*****************************************************************************

    NAME */

      AROS_LH0(VOID, DisableSystemRequesters,

/*  SYNOPSIS */

/*  LOCATION */
      struct LowLevelBase *, LowLevelBase, 20, LowLevel)

/*  FUNCTION

    Stop intuition system requesters from being displayed. While disabled,
    any EasyRequestArgs() call returns 0 immediately (as if the rightmost
    gadget, normally "Cancel", had been selected) instead of opening a
    requester and waiting for input. Calls nest; each one must be balanced
    by EnableSystemRequesters().

    This is the private vector at LVO -120 of the Kickstart 40 (CD32)
    lowlevel.library, which patches EasyRequestArgs() the same way. CD32
    titles call it during startup so that error requesters cannot block a
    pad-only machine, so it must exist and be callable here.

    INPUTS

    RESULT

    BUGS

    SEE ALSO

    EnableSystemRequesters()

    INTERNALS

    The first successful call opens intuition.library and diverts the
    EasyRequestArgs() vector through a gate with SetFunction(); a failed
    installation is retried on the next call. Once in, the gate stays
    for the library's lifetime (expunge restores the vector) and further
    calls only move the nest count it tests.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ObtainSemaphore(&LowLevelBase->ll_Lock);

    LowLevelBase->ll_SysReqNest++;

    /* Keep trying until the gate is in: a failed first attempt (say,
     * intuition was not openable yet) must not block the feature for
     * the rest of the session while the nest count is already raised.
     */
    if (LowLevelBase->ll_EasyRequestOrig == NULL)
    {
        if (LowLevelBase->ll_IntuitionBase == NULL)
            LowLevelBase->ll_IntuitionBase = OpenLibrary("intuition.library", 0);

        if (LowLevelBase->ll_IntuitionBase != NULL)
        {
            LowLevelBase->ll_EasyRequestOrig =
                SetFunction(LowLevelBase->ll_IntuitionBase,
                            EASYREQUESTARGS_LVO,
                            (APTR)AROS_SLIB_ENTRY(llEasyRequestGate,
                                                  LowLevel,
                                                  EASYREQUESTARGS_SLOT));
        }
    }

    ReleaseSemaphore(&LowLevelBase->ll_Lock);

    AROS_LIBFUNC_EXIT
} /* DisableSystemRequesters */

/*****************************************************************************

    NAME */

      AROS_LH0(VOID, EnableSystemRequesters,

/*  SYNOPSIS */

/*  LOCATION */
      struct LowLevelBase *, LowLevelBase, 21, LowLevel)

/*  FUNCTION

    Balance one DisableSystemRequesters() call. When the outermost disable
    is released, EasyRequestArgs() behaves normally again.

    This is the private vector at LVO -126 of the Kickstart 40 (CD32)
    lowlevel.library.

    INPUTS

    RESULT

    BUGS

    SEE ALSO

    DisableSystemRequesters()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ObtainSemaphore(&LowLevelBase->ll_Lock);

    if (LowLevelBase->ll_SysReqNest >= 0)
        LowLevelBase->ll_SysReqNest--;

    ReleaseSemaphore(&LowLevelBase->ll_Lock);

    AROS_LIBFUNC_EXIT
} /* EnableSystemRequesters */
