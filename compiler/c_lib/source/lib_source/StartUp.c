/*
**	$VER: StartUp.c 37.11 (24.6.97)
**
**	Library startup-code and function table definition
**
**	(C) Copyright 1996-97 Andreas R. Kleinert
**	All Rights Reserved.
*/

#define __USE_SYSBASE	     // perhaps only recognized by SAS/C

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <exec/resident.h>
#include <exec/initializers.h>

#ifdef __MAXON__
#include <pragma/exec_lib.h>
#include <linkerfunc.h>
#else
#include <proto/exec.h>    // all other compilers
#endif
#include "compiler.h"
#include "intern.h"

#include "SampleFuncs.h"

AROS_LH2(struct LIBBASETYPE *, InitLib,
    AROS_LHA(struct LIBBASETYPE *, LIBBASE, D0),
    AROS_LHA(BPTR,                 segList, A0),
    struct ExecBase *, SysBase, 0, BASENAME
);
AROS_LH1 (struct LIBBASETYPE *, OpenLib,
    AROS_LHA (ULONG, version, D0),
    struct LIBBASETYPE *, LIBBASE, 1, BASENAME
);
AROS_LH0 (struct LIBBASETYPE *, CloseLib,
    struct LIBBASETYPE *, LIBBASE, 2, BASENAME
);
AROS_LH0 (struct LIBBASETYPE *, ExpungeLib,
    struct LIBBASETYPE *, LIBBASE, 3, BASENAME
);
AROS_LH0 (struct LIBBASETYPE *, ExtFuncLib,
    struct LIBBASETYPE *, LIBBASE, 4, BASENAME
);

/* Declare functions for FuncTab[] */
AROS_LH3 (struct LIBBASETYPE *, EXF_TestRequest,
    AROS_LHA (UBYTE *, title_d1, D1),
    AROS_LHA (UBYTE *, body,     D2),
    AROS_LHA (UBYTE *, gadgets,  D3),
    struct LIBBASETYPE *, LIBBASE, 5, BASENAME
);

LONG ASM LibStart(void)
{
    return(-1);
}

extern const APTR FuncTab [];

struct InitTable		       /* do not change */
{
    ULONG	       LibBaseSize;
    const APTR	      *FunctionTable;
    struct MyDataInit *DataTable;
    APTR	       InitLibTable;
}
const InitTab =
{
    sizeof(struct LIBBASETYPE),
    &FuncTab[0],
    &DataTab,
#ifndef _AROS
    InitLib
#else
    (APTR) AROS_SLIB_ENTRY(InitLib, BASENAME)
#endif
};

APTR const FuncTab [] =
{
#ifndef _AROS
    OpenLib,
    CloseLib,
    ExpungeLib,
    ExtFuncLib,

    EXF_TestRequest,  /* add your own functions here */
#else
    (APTR) AROS_SLIB_ENTRY(OpenLib, BASENAME),
    (APTR) AROS_SLIB_ENTRY(CloseLib, BASENAME),
    (APTR) AROS_SLIB_ENTRY(ExpungeLib, BASENAME),
    (APTR) AROS_SLIB_ENTRY(ExtFuncLib, BASENAME),
    (APTR) AROS_SLIB_ENTRY(EXF_TestRequest, BASENAME),
#endif

    (APTR) ((LONG)-1)
};


AROS_LH2(struct LIBBASETYPE *, InitLib,
    AROS_LHA(struct LIBBASETYPE *, exb,     D0),
    AROS_LHA(BPTR,                 segList, A0),
    struct ExecBase *, sysBase, 0, BASENAME)
{
    LIBBASE = exb;

    LIBBASE->exb_SysBase = sysBase;
    LIBBASE->exb_SegList = segList;

    if (L_OpenLibs (LIBBASE))
	return (LIBBASE);

    L_CloseLibs (LIBBASE);

    FreeMem (exb, sizeof (struct LIBBASETYPE));

    return (NULL);
}

AROS_LH1 (struct LIBBASETYPE *, OpenLib,
    AROS_LHA (ULONG, version, D0),
    struct LIBBASETYPE *, LIBBASE, 1, BASENAME
)
{
#ifdef __MAXON__
    GetBaseReg();
    InitModules();
#endif

    LIBBASE->exb_LibNode.lib_OpenCnt++;

    LIBBASE->exb_LibNode.lib_Flags &= ~LIBF_DELEXP;

    return(LIBBASE);
}

AROS_LH0 (struct LIBBASETYPE *, CloseLib,
    struct LIBBASETYPE *, LIBBASE, 2, BASENAME
)
{
    LIBBASE->exb_LibNode.lib_OpenCnt--;

    if(!LIBBASE->exb_LibNode.lib_OpenCnt)
    {
	if(LIBBASE->exb_LibNode.lib_Flags & LIBF_DELEXP)
	{
	    return (AROS_SLIB_ENTRY(ExpungeLib,BASENAME)(LIBBASE));
	}
    }

    return(NULL);
}

AROS_LH0 (struct LIBBASETYPE *, ExpungeLib,
    struct LIBBASETYPE *, LIBBASE, 3, BASENAME
)
{
    APTR seglist;

    if(!LIBBASE->exb_LibNode.lib_OpenCnt)
    {
	ULONG negsize, possize, fullsize;
	UBYTE *negptr = (UBYTE *) LIBBASE;

	seglist = LIBBASE->exb_SegList;

	Remove((struct Node *)LIBBASE);

	L_CloseLibs(LIBBASE);

	negsize  = LIBBASE->exb_LibNode.lib_NegSize;
	possize  = LIBBASE->exb_LibNode.lib_PosSize;
	fullsize = negsize + possize;
	negptr	-= negsize;

	FreeMem(negptr, fullsize);

#ifdef __MAXON__
	CleanupModules();
#endif

	return(seglist);
    }

    LIBBASE->exb_LibNode.lib_Flags |= LIBF_DELEXP;

    return(NULL);
}

AROS_LH0 (struct LIBBASETYPE *, ExtFuncLib,
    struct LIBBASETYPE *, LIBBASE, 4, BASENAME
)
{
    return(NULL);
}

struct LIBBASETYPE *LIBBASE = NULL;

#ifdef __SASC

#ifdef ARK_OLD_STDIO_FIX

ULONG XCEXIT	   = NULL; /* these symbols may be referenced by    */
ULONG _XCEXIT	   = NULL; /* some functions of sc.lib, but should  */
ULONG ONBREAK	   = NULL; /* never be used inside a shared library */
ULONG _ONBREAK	   = NULL;
ULONG base	   = NULL;
ULONG _base	   = NULL;
ULONG ProgramName  = NULL;
ULONG _ProgramName = NULL;
ULONG StackPtr	   = NULL;
ULONG _StackPtr    = NULL;
ULONG oserr	   = NULL;
ULONG _oserr	   = NULL;
ULONG OSERR	   = NULL;
ULONG _OSERR	   = NULL;

#endif /* ARK_OLD_STDIO_FIX */

void __regargs __chkabort(void) { }  /* a shared library cannot be    */
void __regargs _CXBRK(void)     { }  /* CTRL-C aborted when doing I/O */

#endif /* __SASC */
