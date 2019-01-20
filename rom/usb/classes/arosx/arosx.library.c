/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include <exec/types.h>
#include <exec/libraries.h>

#include <proto/exec.h>

#include "debug.h"

#include "arosx.class.h"
#include "include/arosx.h"

static const UBYTE libarosx[] = "arosx.library";

AROS_UFH3(struct AROSXBase *, libInit, AROS_UFHA(struct AROSXBase *, base, D0), AROS_UFHA(BPTR, seglist, A0), AROS_UFHA(struct ExecBase *, SysBase, A6)) {
    AROS_USERFUNC_INIT
    
    mybug(-1, ("[AROSXLib] libInit base: 0x%08lx seglist: 0x%08lx SysBase: 0x%08lx\n", base, seglist, SysBase));

    base->arosx_LibNode.lib_Node.ln_Type = NT_LIBRARY;
    base->arosx_LibNode.lib_Node.ln_Name = (UBYTE*)libarosx;
    base->arosx_LibNode.lib_Flags        = LIBF_SUMUSED|LIBF_CHANGED;
    base->arosx_LibNode.lib_Version      = 0;
    base->arosx_LibNode.lib_Revision     = 1;
    base->arosx_LibNode.lib_IdString     = (UBYTE*)libarosx;

    /* Store segment, don't have one... */
    //base->arosx_LibNode.np_SegList = seglist;
    
    return(base);

    AROS_USERFUNC_EXIT
}

AROS_LH1(struct AROSXBase *, libOpen, AROS_LHA(ULONG, version, D0), struct AROSXBase *, base, 1, lib) {
    AROS_LIBFUNC_INIT

    ++base->arosx_LibNode.lib_OpenCnt;
    base->arosx_LibNode.lib_Flags &= ~LIBF_DELEXP;

    return base;
    
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, libClose, struct AROSXBase *, base, 2, lib) {
    AROS_LIBFUNC_INIT

    BPTR ret;
    ret = BNULL;

    if(--base->arosx_LibNode.lib_OpenCnt == 0) {
        if(base->arosx_LibNode.lib_Flags & LIBF_DELEXP)
        {
            mybug(-1, ("[AROSXLib] libClose: calling expunge...\n"));
            ret = AROS_LC1(BPTR, libExpunge, AROS_LCA(struct AROSXBase *, base, D0), struct AROSXBase *, base, 3, lib);
        }
    }

    return(ret);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, libExpunge, struct AROSXBase *, base, 3, lib) {
    AROS_LIBFUNC_INIT

    BPTR ret;

    /*
        CHECME: Our memory belongs to arosx.class, make sure we free only allocated memory
    */

    mybug(-1, ("[AROSXLib] libExpunge base: 0x%08lx\n", base));

    ret = BNULL;

    if(base->arosx_LibNode.lib_OpenCnt == 0)
    {
        mybug(-1, ("[AROSXLib] libExpunge: Unloading...\n"));

        //ret = base->np_SegList;

        mybug(-1, ("[AROSXLib] libExpunge: removing library node 0x%08lx\n", &base->arosx_LibNode.lib_Node));
        Remove(&base->arosx_LibNode.lib_Node);

        mybug(-1, ("[AROSXLib] libExpunge: FreeMem()...\n"));
        FreeMem((char *) base - base->arosx_LibNode.lib_NegSize, (ULONG) (base->arosx_LibNode.lib_NegSize + base->arosx_LibNode.lib_PosSize));

        mybug(-1, ("[AROSXLib] libExpunge: Unloading done! arosx.library expunged!\n"));

        return(ret);
    }
    else
    {
        mybug(-1, ("[AROSXLib] libExpunge: Could not expunge, LIBF_DELEXP set!\n"));
        base->arosx_LibNode.lib_Flags |= LIBF_DELEXP;
    }

    return(BNULL);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(IPTR, libReserved, struct AROSXBase *, base, 4, lib) {
    AROS_LIBFUNC_INIT
    return (IPTR)NULL;
    AROS_LIBFUNC_EXIT
}

AROS_LH2(struct AROSX_EventHook *, AROSX_AddEventHandler,
         AROS_LHA(struct MsgPort *, mp, A1),
         AROS_LHA(ULONG, msgmask, D0),
         struct AROSXBase *, base, 5, lib)
{
    AROS_LIBFUNC_INIT

    struct AROSXClassBase *arosxb;
    arosxb = base->arosxb;

    struct AROSX_EventHook *eh = NULL;

    mybug(-1, ("AROSX_AddEventHandler(%p, %p)\n", mp, msgmask));

    if(mp) {
        ObtainSemaphore(&arosxb->event_lock);
        if((eh = AllocVec(sizeof(struct AROSX_EventHook), (MEMF_CLEAR|MEMF_ANY)))) {
            eh->eh_MsgPort = mp;
            eh->eh_MsgMask = msgmask;
            AddTail(&base->arosxb->event_port_list, &eh->eh_Node);

        }
        ReleaseSemaphore(&arosxb->event_lock);
    }
    return(eh);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, AROSX_RemEventHandler,
         AROS_LHA(struct AROSX_EventHook *, eh, A0),
         struct AROSXBase *, base, 6, lib)
{
    AROS_LIBFUNC_INIT

    struct AROSXClassBase *arosxb;
    arosxb = base->arosxb;

    struct Message *msg;

    mybug(-1, ("AROSX_RemEventHandler(%p)\n", eh));
    if(!eh) {
        return;
    }

    ObtainSemaphore(&arosxb->event_lock);
    Remove(&eh->eh_Node);
    while((msg = GetMsg(eh->eh_MsgPort))) {
        ReplyMsg(msg);
    }
    ReleaseSemaphore(&arosxb->event_lock);

    mybug(-1, ("AROSX_RemEventHandler garbage collector\n"));

    struct AROSX_EventNote *en;
    while((en = (struct AROSX_EventNote *) GetMsg(&arosxb->event_reply_port))) {
        mybug(-1, ("    Free AROSX_EventNote(%p)\n", en));
        FreeVec(en);
    }

    FreeVec(eh);

    AROS_LIBFUNC_EXIT
}




static const APTR libFuncTable[] = {
    &AROS_SLIB_ENTRY(libOpen, lib, 1),
    &AROS_SLIB_ENTRY(libClose, lib, 2),
    &AROS_SLIB_ENTRY(libExpunge, lib, 3),
    &AROS_SLIB_ENTRY(libReserved, lib, 4),
    &AROS_SLIB_ENTRY(AROSX_AddEventHandler, lib, 5),
    &AROS_SLIB_ENTRY(AROSX_RemEventHandler, lib, 6),
    (APTR) -1,
};

struct AROSXBase * AROSXInit(void) {

    struct AROSXBase *lib;

    mybug(-1,("AROSXInit\n"));

    if((lib = (struct AROSXBase *)MakeLibrary((APTR) libFuncTable, NULL, (APTR) libInit, sizeof(struct AROSXBase), NULL))) {
        Forbid();
        AddLibrary((struct Library *)lib);
        lib->arosx_LibNode.lib_OpenCnt++;
        Permit();
    } else {
        mybug(-1, ("failed to create arosx.library\n"));
    }

    mybug(-1,("AROSXInit base 0x%08lx\n", lib));
    mybug(-1,("AROSXInit done\n"));

    return lib;
}
