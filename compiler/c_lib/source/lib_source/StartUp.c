/*
**	$VER: StartUp.c 37.10 (1.4.97)
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

#ifdef __GNUC__
#include "examplebase.h"  // GNUC can not handle relativ pathnames.
			  // The full path must be given in the makefile
#else
#include "/include/example/examplebase.h"
#endif
#include "SampleFuncs.h"

#define LIBBASETYPE	ExampleBase
#define LIBBASE 	ExampleBase
#define BASENAME	Example

ULONG SAVEDS L_OpenLibs (struct ExampleBase *exb);
void  SAVEDS L_CloseLibs (struct ExampleBase *exb);

AROS_LH2(struct LIBBASETYPE *, InitLib,
    AROS_LHA(struct LIBBASETYPE *, LIBBASE, D0),
    AROS_LHA(BPTR,                 segList, A0),
    struct ExecBase *, SysBase, 0, Example
);
AROS_LH1 (struct ExampleBase *, OpenLib,
    AROS_LHA (ULONG, version, D0),
    struct LIBBASETYPE *, LIBBASE, 1, Example
);
AROS_LH0 (struct ExampleBase *, CloseLib,
    struct LIBBASETYPE *, LIBBASE, 2, Example
);
AROS_LH0 (struct ExampleBase *, ExpungeLib,
    struct LIBBASETYPE *, LIBBASE, 3, Example
);
AROS_LH0 (struct ExampleBase *, ExtFuncLib,
    struct LIBBASETYPE *, LIBBASE, 4, Example
);

/* Declare functions for FuncTab[] */
AROS_LH3 (struct ExampleBase *, EXF_TestRequest,
    AROS_LHA (UBYTE *, title_d1, D1),
    AROS_LHA (UBYTE *, body,     D2),
    AROS_LHA (UBYTE *, gadgets,  D3),
    struct LIBBASETYPE *, LIBBASE, 5, Example
);

LONG ASM LibStart(void)
{
    return(-1);
}

extern const APTR FuncTab [];
extern struct MyDataInit DataTab;

struct InitTable		       /* do not change */
{
    ULONG	       LibBaseSize;
    const APTR	      *FunctionTable;
    struct MyDataInit *DataTable;
    APTR	       InitLibTable;
} const InitTab =
{
    sizeof(struct ExampleBase),
    &FuncTab[0],
    &DataTab,
#ifndef _AROS
    InitLib
#else
    (APTR) AROS_SLIB_ENTRY(InitLib, Example),
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
    (APTR) AROS_SLIB_ENTRY(OpenLib, Example),
    (APTR) AROS_SLIB_ENTRY(CloseLib, Example),
    (APTR) AROS_SLIB_ENTRY(ExpungeLib, Example),
    (APTR) AROS_SLIB_ENTRY(ExtFuncLib, Example),
    (APTR) AROS_SLIB_ENTRY(EXF_TestRequest, Example),
#endif

    (APTR) ((LONG)-1)
};


extern struct ExampleBase *ExampleBase;

AROS_LH2(struct LIBBASETYPE *, InitLib,
    AROS_LHA(struct LIBBASETYPE *, exb,     D0),
    AROS_LHA(BPTR,                 segList, A0),
    struct ExecBase *, sysBase, 0, Example)
{
    ExampleBase = exb;

    ExampleBase->exb_SysBase = sysBase;
    ExampleBase->exb_SegList = segList;

    if(L_OpenLibs(ExampleBase)) return(ExampleBase);

    L_CloseLibs (ExampleBase);

    return(NULL);
}

AROS_LH1 (struct ExampleBase *, OpenLib,
    AROS_LHA (ULONG, version, D0),
    struct LIBBASETYPE *, LIBBASE, 1, Example
)
{
#ifdef __MAXON__
    GetBaseReg();
    InitModules();
#endif

    ExampleBase->exb_LibNode.lib_OpenCnt++;

    ExampleBase->exb_LibNode.lib_Flags &= ~LIBF_DELEXP;

    return(ExampleBase);
}

AROS_LH0 (struct ExampleBase *, CloseLib,
    struct LIBBASETYPE *, LIBBASE, 2, Example
)
{
    ExampleBase->exb_LibNode.lib_OpenCnt--;

    if(!ExampleBase->exb_LibNode.lib_OpenCnt)
    {
	if(ExampleBase->exb_LibNode.lib_Flags & LIBF_DELEXP)
	{
	    return (AROS_SLIB_ENTRY(ExpungeLib,Example)(ExampleBase));
	}
    }

    return(NULL);
}

AROS_LH0 (struct ExampleBase *, ExpungeLib,
    struct LIBBASETYPE *, LIBBASE, 3, Example
)
{
    APTR seglist;

    if(!ExampleBase->exb_LibNode.lib_OpenCnt)
    {
	ULONG negsize, possize, fullsize;
	UBYTE *negptr = (UBYTE *) ExampleBase;

	seglist = ExampleBase->exb_SegList;

	Remove((struct Node *)ExampleBase);

	L_CloseLibs(ExampleBase);

	negsize  = ExampleBase->exb_LibNode.lib_NegSize;
	possize  = ExampleBase->exb_LibNode.lib_PosSize;
	fullsize = negsize + possize;
	negptr	-= negsize;

	FreeMem(negptr, fullsize);

#ifdef __MAXON__
	CleanupModules();
#endif

	return(seglist);
    }

    ExampleBase->exb_LibNode.lib_Flags |= LIBF_DELEXP;

    return(NULL);
}

AROS_LH0 (struct ExampleBase *, ExtFuncLib,
    struct LIBBASETYPE *, LIBBASE, 4, Example
)
{
    return(NULL);
}

struct ExampleBase *ExampleBase = NULL;

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
