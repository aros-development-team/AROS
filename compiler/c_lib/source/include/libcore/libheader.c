/*
**	$VER: libheader.c 37.15 (14.8.97)
**
**	This file must be compiled and must be passed as the first
**	object to link to the linker.
**
**	(C) Copyright 1996-97 Andreas R. Kleinert
**	All Rights Reserved.
*/

#define __USE_SYSBASE	     /* perhaps only recognized by SAS/C */

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
#include <proto/exec.h>    /* all other compilers */
#endif
#include <libcore/compiler.h>
#include <libcore/base.h>

LONG ASM LibStart(void)
{
    return(-1);
}

/* If the file with the #defines for this library is not "libdefs.h",
    then you can redefine it. */
#ifndef LIBDEFS_FILE
#   define LIBDEFS_FILE "libdefs.h"
#endif

/* Include the file with the #defines for this library */
#include LIBDEFS_FILE

/* Predeclarations */
extern const int LIBEND;	  /* The end of the library */
extern const APTR LIBFUNCTABLE[]; /* The function table */
extern const struct InitTable InitTab;
extern const struct DataTable DataTab;

extern const char ALIGNED ExLibName [];
extern const char ALIGNED ExLibID   [];
extern const char ALIGNED Copyright [];

extern AROS_LH2 (struct LibHeader *, InitLib,
    AROS_LHA(struct LibHeader *, lh,      D0),
    AROS_LHA(BPTR,               segList, A0),
    struct ExecBase *, sysBase, 0, LibHeader
);
extern AROS_LH0 (BPTR, ExpungeLib,
    struct LibHeader *, lh, 3, LibHeader
);

struct Resident const ALIGNED ROMTag =	   /* do not change */
{
    RTC_MATCHWORD,		    /* This is a romtag */
    (struct Resident *)&ROMTag,     /* This is a valid romtag */
    (APTR) &LIBEND,
    RTF_AUTOINIT,
    VERSION_NUMBER,
    NT_TYPE,
    0,
    (char *) &ExLibName[0],
    (char *) &ExLibID[0],
    (APTR) &InitTab
};

struct InitTable		       /* do not change */
{
    ULONG		    LibBaseSize;
    const APTR		   *FunctionTable;
    const struct DataTable *DataTable;
    APTR		    InitLibTable;
}
const InitTab =
{
    sizeof (struct LibHeader),
    &LIBFUNCTABLE[0],
    &DataTab,
    (APTR) AROS_SLIB_ENTRY(InitLib, LibHeader)
};

struct DataTable		    /* do not change */
{
    UWORD ln_Type_Init;      UWORD ln_Type_Offset;	UWORD ln_Type_Content;
    UBYTE ln_Name_Init;      UBYTE ln_Name_Offset;	ULONG ln_Name_Content;
    UWORD lib_Flags_Init;    UWORD lib_Flags_Offset;	UWORD lib_Flags_Content;
    UWORD lib_Version_Init;  UWORD lib_Version_Offset;	UWORD lib_Version_Content;
    UWORD lib_Revision_Init; UWORD lib_Revision_Offset; UWORD lib_Revision_Content;
    UBYTE lib_IdString_Init; UBYTE lib_IdString_Offset; ULONG lib_IdString_Content;
    ULONG ENDMARK;
}
const DataTab =
{
    INITBYTE(OFFSET(Node,         ln_Type),      NT_TYPE),
    0x80, (UBYTE) OFFSET(Node,    ln_Name),      (ULONG) &ExLibName[0],
    INITBYTE(OFFSET(Library,      lib_Flags),    LIBF_SUMUSED|LIBF_CHANGED),
    INITWORD(OFFSET(Library,      lib_Version),  VERSION_NUMBER),
    INITWORD(OFFSET(Library,      lib_Revision), REVISION_NUMBER),
    0x80, (UBYTE) OFFSET(Library, lib_IdString), (ULONG) &ExLibID[0],
    (ULONG) 0
};

const char ALIGNED ExLibName [] = NAME_STRING;
const char ALIGNED ExLibID   [] = VERSION_STRING;
const char ALIGNED Copyright [] = COPYRIGHT_STRING;

#ifndef SYSBASE_FIELD
#   define SYSBASE_FIELD(libBase)   (libBase)->lh_SysBase
#endif
#ifndef SEGLIST_FIELD
#   define SEGLIST_FIELD(libBase)   (libBase)->lh_SegList
#endif

/* Use supplied functions to initialize the non-standard parts of the
   library */
ULONG SAVEDS STDARGS L_OpenLibs(struct LibHeader * lh);
ULONG SAVEDS STDARGS L_CloseLibs(struct LibHeader * lh);

AROS_LH2 (struct LibHeader *, InitLib,
    AROS_LHA(struct LibHeader *, lh,      D0),
    AROS_LHA(BPTR,               segList, A0),
    struct ExecBase *, sysBase, 0, LibHeader
)
{
    SYSBASE_FIELD(lh) = sysBase;
    SEGLIST_FIELD(lh) = segList;

    if (L_OpenLibs (lh))
	return (lh);

    L_CloseLibs (lh);

    {
	ULONG negsize, possize, fullsize;
	UBYTE *negptr = (UBYTE *) lh;

	negsize  = lh->lh_LibNode.lib_NegSize;
	possize  = lh->lh_LibNode.lib_PosSize;
	fullsize = negsize + possize;
	negptr	-= negsize;

	FreeMem (negptr, fullsize);
    }

    return (NULL);
}

AROS_LH1 (struct LibHeader *, OpenLib,
    AROS_LHA (ULONG, version, D0),
    struct LibHeader *, lh, 1, LibHeader
)
{
#ifdef __MAXON__
    GetBaseReg();
    InitModules();
#endif

#ifndef NOEXPUNGE
    lh->lh_LibNode.lib_OpenCnt++;
#else
    lh->lh_LibNode.lib_OpenCnt = 1;
#endif /* NOEXPUNGE */

    lh->lh_LibNode.lib_Flags &= ~LIBF_DELEXP;

    return(lh);
}

AROS_LH0 (BPTR, CloseLib,
    struct LibHeader *, lh, 2, LibHeader
)
{
#ifndef NOEXPUNGE
    lh->lh_LibNode.lib_OpenCnt--;

    if(!lh->lh_LibNode.lib_OpenCnt)
    {
	if(lh->lh_LibNode.lib_Flags & LIBF_DELEXP)
	{
	    return (AROS_SLIB_ENTRY(ExpungeLib,LibHeader)(lh));
	}
    }
#endif /* NOEXPUNGE */

    return (NULL);
}

AROS_LH0 (BPTR, ExpungeLib,
    struct LibHeader *, lh, 3, LibHeader
)
{
#ifndef NOEXPUNGE
    BPTR seglist;

    if(!lh->lh_LibNode.lib_OpenCnt)
    {
	ULONG negsize, possize, fullsize;
	UBYTE *negptr = (UBYTE *)lh;

	seglist = SEGLIST_FIELD(lh);

	Remove((struct Node *)lh);

	L_CloseLibs(lh);

	negsize  = lh->lh_LibNode.lib_NegSize;
	possize  = lh->lh_LibNode.lib_PosSize;
	fullsize = negsize + possize;
	negptr	-= negsize;

	FreeMem(negptr, fullsize);

#ifdef __MAXON__
	CleanupModules();
#endif

	return(seglist);
    }

    lh->lh_LibNode.lib_Flags |= LIBF_DELEXP;
#endif /* NOEXPUNGE */

    return (NULL);
}

AROS_LH0 (struct LibHeader *, ExtFuncLib,
    struct LibHeader *, lh, 4, LibHeader
)
{
    return(NULL);
}

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

