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
#include "include/arosx.h"

static const UBYTE libarosx[] = "arosx.library";

AROS_UFH3(struct Library *, libInit, AROS_UFHA(struct Library *, base, D0), AROS_UFHA(BPTR, seglist, A0), AROS_UFHA(struct ExecBase *, SysBase, A6)) {
    AROS_USERFUNC_INIT
    
    mybug(-1, ("libInit base: 0x%08lx seglist: 0x%08lx SysBase: 0x%08lx\n", base, seglist, SysBase));

    base->lib_Node.ln_Type = NT_LIBRARY;
    base->lib_Node.ln_Name = (UBYTE*)libarosx;
    base->lib_Flags        = LIBF_SUMUSED|LIBF_CHANGED;
    base->lib_Version      = 0;
    base->lib_Revision     = 1;
    base->lib_IdString     = (UBYTE*)libarosx;

    /* Store segment, don't have one... */
    //base->np_SegList = seglist;
    
    return(base);

    AROS_USERFUNC_EXIT
}

AROS_LH1(struct Library *, libOpen, AROS_LHA(ULONG, version, D0), struct Library *, base, 1, lib) {
    AROS_LIBFUNC_INIT

    mybug(-1,("AROSX: OpenLib version %x.%x 0x%08lx\n", version, base->lib_Revision, base));

    ++base->lib_OpenCnt;
    base->lib_Flags &= ~LIBF_DELEXP;

    return base;
    
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, libClose, struct Library *, base, 2, lib) {
    AROS_LIBFUNC_INIT

    BPTR ret;
    ret = BNULL;

    mybug(-1,("AROSX: CloseLib version %x.%x 0x%08lx\n", base->lib_Version, base->lib_Revision, base));

    if(--base->lib_OpenCnt == 0) {
        if(base->lib_Flags & LIBF_DELEXP)
        {
            mybug(-1, ("libClose: calling expunge...\n"));
            ret = AROS_LC1(BPTR, libExpunge, AROS_LCA(struct Library *, base, D0), struct Library *, base, 3, lib);
        }
    }

    mybug(-1, ("libClose: lib_OpenCnt = %ld\n", base->lib_OpenCnt));

    return(ret);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, libExpunge, struct Library *, base, 3, lib) {
    AROS_LIBFUNC_INIT

    BPTR ret;

    mybug(-1, ("libExpunge base: 0x%08lx\n", base));

    ret = BNULL;

    if(base->lib_OpenCnt == 0)
    {
        mybug(-1, ("libExpunge: Unloading...\n"));

        //ret = base->np_SegList;

        mybug(-1, ("libExpunge: removing library node 0x%08lx\n", &base->lib_Node));
        Remove(&base->lib_Node);

        mybug(-1, ("libExpunge: FreeMem()...\n"));
        FreeMem((char *) base - base->lib_NegSize, (ULONG) (base->lib_NegSize + base->lib_PosSize));

        mybug(-1, ("libExpunge: Unloading done! arosx.library expunged!\n"));

        return(ret);
    }
    else
    {
        mybug(-1, ("libExpunge: Could not expunge, LIBF_DELEXP set!\n"));
        base->lib_Flags |= LIBF_DELEXP;
    }

    return(BNULL);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(struct Library *, libReserved, struct Library *, base, 4, lib) {
    AROS_LIBFUNC_INIT
    return NULL;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(struct Library *, libUnused, struct Library *, base, 5, lib) {
    AROS_LIBFUNC_INIT

    mybug(-1, ("libUnused: Hello...\n"));
    return NULL;

    AROS_LIBFUNC_EXIT
}

static const APTR libFuncTable[] = {
    &AROS_SLIB_ENTRY(libOpen, lib, 1),
    &AROS_SLIB_ENTRY(libClose, lib, 2),
    &AROS_SLIB_ENTRY(libExpunge, lib, 3),
    &AROS_SLIB_ENTRY(libReserved, lib, 4),
    &AROS_SLIB_ENTRY(libUnused, lib, 5),
    (APTR) -1,
};

struct Library * AROSXInit(void) {

    struct Library *lib;

    mybug(-1,("AROSXInit\n"));

    if((lib = MakeLibrary((APTR) libFuncTable, NULL, (APTR) libInit, sizeof(struct AROSXBase), NULL))) {
        Forbid();
        AddLibrary(lib);
        lib->lib_OpenCnt++;
        Permit();
    } else {
        mybug(-1, ("failed to create arosx.library\n"));
    }

    mybug(-1,("AROSXInit base 0x%08lx\n", lib));
    mybug(-1,("AROSXInit done\n"));

    return lib;
}
