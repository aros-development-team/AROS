/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include <proto/exec.h>

#include "m680x0_intern.h"

extern void sp060_init(void);

static AROS_UFH2(struct Library*, OpenLib,
    AROS_UFHA(ULONG, version, D0),
    AROS_UFHA(struct Library*, base, A6))
{ 
    AROS_USERFUNC_INIT
    
    base->lib_OpenCnt++;
    return base;

    AROS_USERFUNC_EXIT
}
    
static AROS_UFH1(void, CloseLib,
    AROS_UFHA(struct Library*, base, A6))
{ 
    AROS_USERFUNC_INIT
    
    base->lib_OpenCnt--;

    AROS_USERFUNC_EXIT
}

static AROS_UFH1(ULONG, DummyLib,
    AROS_UFHA(struct Library*, base, A6))
{ 
    AROS_USERFUNC_INIT
    
    return 0;

    AROS_USERFUNC_EXIT
}

/* This is totally undocumented so complain if something calls our functions */
#define UNUSED(x) \
static AROS_UFH1(ULONG, x, \
AROS_UFHA(struct Library*, base, A6)) { \
    AROS_USERFUNC_INIT \
    bug("680x0: " #x "\n"); \
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

static const UBYTE lib68040[] = "68040.library";
static const UBYTE lib68060[] = "68060.library";

/* m68k identify.library calls these, purpose unknown */
AROS_LH0(ULONG, Dummy1, struct M680x0Base*, M680x0Base, 5, m680x0)
{
    AROS_LIBFUNC_INIT
    bug("680x0: dummy1\n");
    return 0;
    AROS_LIBFUNC_EXIT
}
AROS_LH0(ULONG, Dummy2, struct M680x0Base*, M680x0Base, 6, m680x0)
{
    AROS_LIBFUNC_INIT
    bug("680x0: dummy2\n");
    return 0;
    AROS_LIBFUNC_EXIT
}
AROS_LH0(ULONG, Dummy3, struct M680x0Base*, M680x0Base, 7, m680x0)
{
    AROS_LIBFUNC_INIT
    bug("680x0: dummy3\n");
    return 0;
    AROS_LIBFUNC_EXIT
}
AROS_LH0(ULONG, Dummy4, struct M680x0Base*, M680x0Base, 8, m680x0)
{
    AROS_LIBFUNC_INIT
    bug("680x0: dummy4\n");
    return 0;
    AROS_LIBFUNC_EXIT
}

static int M680x0Init(struct M680x0Base *M680x0Base)
{
    struct Library *lib;

    if (!(SysBase->AttnFlags & (AFF_68040 | AFF_68060)))
    	return FALSE; /* 68040/060 only need emulation */
    if (SysBase->AttnFlags & AFF_68882)
    	return FALSE; /* we already have full support? */
    if (!(SysBase->AttnFlags & AFF_FPU40))
    	return FALSE; /* no FPU, don't bother with missing instruction emulation */

    /* initialize emulation here */
    sp060_init();

    /* Create fake 68040/060.library, stops C:SetPatch from attempting to load
     * incompatible 68040/060 libraries.
     *
     * We also create both 68040 and 68060.library if 68060 is detected, it prevents
     * old SetPatch versions (that do not know about 68060) from loading 68040.library.
     *
     * (Maybe this is getting too far..)
     */
    if (SysBase->AttnFlags & AFF_68060) {
        lib = MakeLibrary(funcLib, NULL, NULL, sizeof(struct Library), BNULL);
        if (lib) {
            lib->lib_Node.ln_Name = (UBYTE*)lib68060;
            lib->lib_IdString = lib->lib_Node.ln_Name;
            lib->lib_Version = M680x0Base->pb_LibNode.lib_Version;
            lib->lib_Revision = M680x0Base->pb_LibNode.lib_Revision;
            lib->lib_OpenCnt = 1;
            AddLibrary(lib);
        }
    }
    lib = MakeLibrary(funcLib, NULL, NULL, sizeof(struct Library), BNULL);
    if (lib) {
        lib->lib_Node.ln_Name = (UBYTE*)lib68040;
        lib->lib_IdString = lib->lib_Node.ln_Name;
        lib->lib_Version = M680x0Base->pb_LibNode.lib_Version;
        lib->lib_Revision = M680x0Base->pb_LibNode.lib_Revision;
        lib->lib_OpenCnt = 1;
        AddLibrary(lib);
    }

    /* emulation installed, full 68881/68882 instruction set now supported  */
    SysBase->AttnFlags |= AFF_68881 | AFF_68882;
    /* do not expunge us */
    M680x0Base->pb_LibNode.lib_OpenCnt++;
    return TRUE;
}

ADD2INITLIB(M680x0Init, 0)


