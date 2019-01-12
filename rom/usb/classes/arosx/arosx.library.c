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

static AROS_UFH2(struct Library*, OpenLib,
    AROS_UFHA(ULONG, version, D0),
    AROS_UFHA(struct Library*, base, A6))
{ 
    AROS_USERFUNC_INIT
    
    mybug(-1,("AROSX: OpenLib\n"));

    base->lib_OpenCnt++;
    return base;

    AROS_USERFUNC_EXIT
}
    
static AROS_UFH1(void, CloseLib,
    AROS_UFHA(struct Library*, base, A6))
{ 
    AROS_USERFUNC_INIT
    
    mybug(-1,("AROSX: CloseLib\n"));

    base->lib_OpenCnt--;

    AROS_USERFUNC_EXIT
}

static AROS_UFH1(ULONG, DummyLib,
    AROS_UFHA(struct Library*, base, A6))
{ 
    AROS_USERFUNC_INIT
    
    mybug(-1,("AROSX: DummyLib\n"));

    return 0;

    AROS_USERFUNC_EXIT
}

#define UNUSED(x) \
static AROS_UFH1(ULONG, x, \
AROS_UFHA(struct Library*, base, A6)) { \
    AROS_USERFUNC_INIT \
    mybug(-1,("AROSX: " #x "\n")); \
    return 0; \
    AROS_USERFUNC_EXIT \
}

UNUSED(Unused5);
UNUSED(Unused6);
UNUSED(Unused7);
UNUSED(Unused8);
UNUSED(Unused9);
UNUSED(Unused10);
UNUSED(Unused11);
UNUSED(Unused12);
UNUSED(Unused13);
UNUSED(Unused14);
UNUSED(Unused15);
UNUSED(Unused16);

static const APTR funcLib[] = {
	OpenLib, CloseLib, DummyLib, DummyLib,
	Unused5,
	Unused6,
	Unused7,
	Unused8,
	Unused9,
	Unused10,
	Unused11,
	Unused12,
	Unused13,
	Unused14,
	Unused15,
	Unused16,
	(void*)-1 };

static const UBYTE libarosx[] = "arosx.library";

/*
*/
AROS_LH0(ULONG, Dummy1, struct AROSXBase*, AROSXBase, 5, arosx)
{
    AROS_LIBFUNC_INIT
    mybug(-1,("AROSX: dummy1\n"));
    return 0;
    AROS_LIBFUNC_EXIT
}
AROS_LH0(ULONG, Dummy2, struct AROSXBase*, AROSXBase, 6, arosx)
{
    AROS_LIBFUNC_INIT
    mybug(-1,("AROSX: dummy2\n"));
    return 0;
    AROS_LIBFUNC_EXIT
}
AROS_LH0(ULONG, Dummy3, struct AROSXBase*, AROSXBase, 7, arosx)
{
    AROS_LIBFUNC_INIT
    mybug(-1,("AROSX: dummy3\n"));
    return 0;
    AROS_LIBFUNC_EXIT
}
AROS_LH0(ULONG, Dummy4, struct AROSXBase*, AROSXBase, 8, arosx)
{
    AROS_LIBFUNC_INIT
    mybug(-1,("AROSX: dummy4\n"));
    return 0;
    AROS_LIBFUNC_EXIT
}

struct Library * AROSXInit(void)
{
    struct Library *lib;

    mybug(-1,("AROSXInit\n"));

    lib = MakeLibrary(funcLib, NULL, NULL, sizeof(struct Library), BNULL);
    if (lib) {
        lib->lib_Node.ln_Name = (UBYTE*)libarosx;
        lib->lib_IdString = lib->lib_Node.ln_Name;
        lib->lib_Version = 0;
        lib->lib_Revision = 1;
        lib->lib_OpenCnt = 1;
        AddLibrary(lib);
    }

    mybug(-1,("AROSXInit done\n"));

    return lib;
}
