/* Example source

        Name:           example_lib.c
		Versionstring:  $VER: example_lib.c 1.2 (21.06.2007)
        Author:         Jens Langner
        Distribution:   PD
        Description:    shows how the SDI_lib.h header include can be used

  1.0   05.10.04 : initial version showing how the SDI_lib.h headers can help
                   when used for maintaining AmigaOS compatible shared library
                   interfaces upon compatible operating systems like AmigaOS3,
                   AmigaOS4 and MorphOS.

  1.1   20.06.07 : moved LIBFUNC to be first in line acording to changes in
				   SDI_lib.h. (Guido Mersmann)

  1.2   21.06.07 : added missing libvector to MorphOS/AmigaOS3 libvectors[]
				   (Guido Mersmann)

  Please note that this example is just for educational purposes and wasn't
  checked for complete correctness. However, it should compile and probably also
  work as expected. But please note that its purpose is to show how a shared
  library can held platform independent over all different available AmigaOS
  platforms like OS3, OS4 and MorphOS.

  Feel free to comment and submit any suggestions directly to
  Jens.Langner@light-speed.de

*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/resident.h>
#include <dos/dos.h>

#include <proto/exec.h>

#ifdef __amigaos4__
#include <exec/emulation.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

#include <SDI_lib.h>
#include <SDI_stdarg.h>

#define VERSION		1
#define REVISION	4
#define DATE        "25.08.2014"
#define VERS        "example.library 1.4"
#define VSTRING		"example.library 1.4 25.08.2014)\r\n"
#define VERSTAG		"\0$VER: example.library 1.4 25.08.2014)"

static const char UserLibName[] = "example.library";
static const char UserLibID[]   = "\0$VER: example.library 1.4 25.08.2014)";

#if defined(__MORPHOS__) || defined(__AROS__)
struct ExecBase *SysBase;
#endif

/******************************************************************************/
/*             >> User defineable library functions start here <<             */
/*  The following section should illustrate how the SDI_lib.h macros make it  */
/*   possible to easily maintain a typical Amiga shared library interface if  */
/*      compatibility over all common (OS3/OS4, MorphOS) is required          */
/******************************************************************************/

/******************************************************************************/
/* Local Structures & Prototypes                                              */
/******************************************************************************/

struct LibraryHeader
{
  struct Library	libBase;
  struct Library *sysBase;
  ULONG           segList;
};

#if defined(__amigaos4__)
#define __BASE_OR_IFACE_TYPE	struct ExampleIFace *
#define __BASE_OR_IFACE_VAR		IExample
#else
#define __BASE_OR_IFACE_TYPE	struct LibraryHeader *
#define __BASE_OR_IFACE_VAR		ExampleBase
#endif
#define __BASE_OR_IFACE			__BASE_OR_IFACE_TYPE __BASE_OR_IFACE_VAR

// first the prototypes of all our public library functions
LIBPROTO(SayHelloOS4, char *, REG(a6, UNUSED __BASE_OR_IFACE));
LIBPROTO(SayHelloOS3, char *, REG(a6, UNUSED __BASE_OR_IFACE));
LIBPROTO(SayHelloMOS, char *, REG(a6, UNUSED __BASE_OR_IFACE));
LIBPROTO(Uppercase, char *, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, char *txt));
LIBPROTO(SPrintfA, char *, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, char *buf), REG(a1, char *format), REG(a2, APTR args));
LIBPROTOVA(SPrintf, char *, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, char *buf), REG(a1, char *format), ...);

// let us now create the libvector.
// Please note that the start of the vectors has to be always the "LFUNC_FAS"
// macro
#define libvector LFUNC_FAS(SayHelloOS4) \
                  LFUNC_FA_(SayHelloOS3) \
                  LFUNC_FA_(SayHelloMOS) \
                  LFUNC_FA_(Uppercase)   \
                  LFUNC_FA_(SPrintfA)    \
                  LFUNC_VA_(SPrintf)

// Now the real implementations of the library functions above like in
// a normal AmigaOS shared library follow
LIBPROTO(SayHelloOS4, char *, REG(a6, UNUSED __BASE_OR_IFACE))
{
  return "Hello AmigaOS4!!!";
}

LIBPROTO(SayHelloOS3, char *, REG(a6, UNUSED __BASE_OR_IFACE))
{
  return "Hello AmigaOS3!!!";
}

LIBPROTO(SayHelloMOS, char *, REG(a6, UNUSED __BASE_OR_IFACE))
{
  return "Hello MorphOS!!!";
}

LIBPROTO(Uppercase, char *, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, char *txt))
{
  char *p = txt;

  while(*p)
  {
    *p = toupper(*p);
    p++;
  }

  return txt;
}

LIBPROTO(SPrintfA, char *, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, char *buf), REG(a1, char *format), REG(a2, APTR args))
{
  RawDoFmt(format, args, NULL, buf);

  return(buf);
}

#if defined(__amigaos4__)
// for AmigaOS4 varargs functions are separate entries in the interface structure and
// hence must be defined as separate functions
LIBPROTOVA(SPrintf, char *, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, char *buf), REG(a1, char *format), ...)
{
  char *ret;
  VA_LIST args;

  VA_START(args, format);
  // the SPrintf function will be call via the interface
  ret = SPrintfA(buf, format, VA_ARG(args, ULONG));
  VA_END(args);

  return(ret);
}
#elif defined(__MORPHOS__)
// define stub functions for all functions in the jump table which take the
// parameters from the emulated 68k registers. The parameters don't need to
// be specified again, because these are really functions taking no direct
// parameters. Only the type of the returned value is required.
LIBSTUB(SayHelloOS4, char *)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC_NP(SayHelloOS4);
}

LIBSTUB(SayHelloOS3, char *)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC_NP(SayHelloOS3);
}

LIBSTUB(SayHelloMOS, char *)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC_NP(SayHelloMOS);
}

LIBSTUB(Uppercase, char *)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(Uppercase, (char *)REG_A0);
}

LIBSTUB(SPrintfA, char *)
{
  __BASE_OR_IFACE = (__BASE_OR_IFACE_TYPE)REG_A6;
  return CALL_LFUNC(SprintfA, (char)REG_A0, (char *)REG_A1, (APTR)REG_A2);
}
#elif defined(__AROS__)
AROS_LH0(char *, SayHelloOS4, struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB)
{
  AROS_LIBFUNC_INIT
  return CALL_LFUNC_NP(SayHelloOS4);
  AROS_LIBFUNC_EXIT
}

AROS_LH0(char *, SayHelloOS3, struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB)
{
  AROS_LIBFUNC_INIT
  return CALL_LFUNC_NP(SayHelloOS3);
  AROS_LIBFUNC_EXIT
}

AROS_LH0(char *, SayHelloMOS, struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB)
{
  AROS_LIBFUNC_INIT
  return CALL_LFUNC_NP(SayHelloMOS);
  AROS_LIBFUNC_EXIT
}

AROS_LH1(char *, Uppercase, AROS_LHA(char *, txt, A0), struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB)
{
  AROS_LIBFUNC_INIT
  return  CALL_LFUNC(Uppercase, txt);
  AROS_LIBFUNC_EXIT
}

AROS_LH3(char *, SPrintfA,
    AROS_LHA(char *, buf, A0),
    AROS_LHA(char *, format, A1),
    AROS_LHA(APTR, args, A1),
    struct LibraryHeader *, __BASE_OR_IFACE_VAR, 0, LIBSTUB
)
{
  AROS_LIBFUNC_INIT
  return CALL_LFUNC(SPrintfA, buf, format, args);
  AROS_LIBFUNC_EXIT
}
#endif

/******************************************************************************/
/*    Starting from here starts the "standard" Amiga library initialization.  */
/*  All the above should have illustrated enough how "easy" it is to use the  */
/*  SDI_lib.h header file and how it can help in developing a shared library  */
/*               for multiple AmigaOS compatible platforms.                   */
/******************************************************************************/

#if defined(__amigaos4__)

LIBFUNC static struct LibraryHeader * LibInit    (struct LibraryHeader *base, BPTR librarySegment, struct ExecIFace *pIExec);
LIBFUNC static BPTR                   LibExpunge (struct LibraryManagerInterface *Self);
LIBFUNC static struct LibraryHeader * LibOpen    (struct LibraryManagerInterface *Self, ULONG version);
LIBFUNC static BPTR                   LibClose   (struct LibraryManagerInterface *Self);

#elif defined(__MORPHOS__)

LIBFUNC static struct LibraryHeader * LibInit   (struct LibraryHeader *base, BPTR librarySegment, struct ExecBase *sb);
LIBFUNC static BPTR                   LibExpunge(void);
LIBFUNC static struct LibraryHeader * LibOpen   (void);
LIBFUNC static BPTR                   LibClose  (void);
LIBFUNC static LONG                   LibNull   (void);

#elif defined(__AROS__)

#include <aros/libcall.h>

static AROS_UFP3 (struct LibraryHeader *, LibInit,
                  AROS_UFPA(struct LibraryHeader *, base, D0),
                  AROS_UFPA(BPTR, librarySegment, A0),
                  AROS_UFPA(struct ExecBase *, sb, A6)
);
static AROS_LD1 (struct LibraryHeader *, LibOpen,
                 AROS_LPA (UNUSED ULONG, version, D0),
                 struct LibraryHeader *, base, 1, example
);
static AROS_LD0 (BPTR, LibClose,
                 struct LibraryHeader *, base, 2, example
);
static AROS_LD1(BPTR, LibExpunge,
                AROS_LPA(UNUSED struct LibraryHeader *, __extrabase, D0),
                struct LibraryHeader *, base, 3, example
);

#else

LIBFUNC static struct LibraryHeader * LibInit    (REG(a0, BPTR Segment), REG(d0, struct LibraryHeader *lh), REG(a6, struct ExecBase *sb));
LIBFUNC static BPTR                   LibExpunge (REG(a6, struct LibraryHeader *base));
LIBFUNC static struct LibraryHeader * LibOpen    (REG(a6, struct LibraryHeader *base));
LIBFUNC static BPTR                   LibClose   (REG(a6, struct LibraryHeader *base));
LIBFUNC static LONG                   LibNull    (void);

#endif

/******************************************************************************/
/* Dummy entry point and LibNull() function all in one                        */
/******************************************************************************/

#if defined(__amigaos4__)
int _start(void)
#elif defined(__AROS__)
__startup int Main(void)
#else
int Main(void)
#endif
{
  return RETURN_FAIL;
}

#if !defined(__amigaos4__)
LIBFUNC static LONG LibNull(VOID)
{
  return(0);
}
#endif

/******************************************************************************/
/* Local data structures                                                      */
/******************************************************************************/

#if defined(__amigaos4__)
/* ------------------- OS4 Manager Interface ------------------------ */
STATIC ULONG LibObtain(struct LibraryManagerInterface *Self)
{
  return(Self->Data.RefCount++);
}

STATIC ULONG LibRelease(struct LibraryManagerInterface *Self)
{
  return(Self->Data.RefCount--);
}

STATIC CONST APTR LibManagerVectors[] =
{
  (APTR)LibObtain,
  (APTR)LibRelease,
  (APTR)NULL,
  (APTR)NULL,
  (APTR)LibOpen,
  (APTR)LibClose,
  (APTR)LibExpunge,
  (APTR)NULL,
  (APTR)-1
};

STATIC CONST struct TagItem LibManagerTags[] =
{
  {MIT_Name,             (ULONG)"__library"},
  {MIT_VectorTable,      (ULONG)LibManagerVectors},
  {MIT_Version,          1},
  {TAG_DONE,             0}
};

/* ------------------- Library Interface(s) ------------------------ */

STATIC CONST APTR LibVectors[] =
{
  (APTR)LibObtain,
  (APTR)LibRelease,
  (APTR)NULL,
  (APTR)NULL,
  (APTR)libvector,
  (APTR)-1
};

STATIC CONST struct TagItem MainTags[] =
{
  {MIT_Name,              (ULONG)"main"},
  {MIT_VectorTable,       (ULONG)LibVectors},
  {MIT_Version,           1},
  {TAG_DONE,              0}
};


STATIC CONST ULONG LibInterfaces[] =
{
  (ULONG)LibManagerTags,
  (ULONG)MainTags,
  (ULONG)0
};

/* --------------------- m68k Library stubs ------------------------ */

STATIC ULONG stub_OpenPPC(ULONG *regarray)
{
	struct Library *Base = (struct Library *) regarray[REG68K_A6/4];
	struct ExtendedLibrary *ExtLib = (struct ExtendedLibrary *) ((ULONG)Base + Base->lib_PosSize);
	struct LibraryManagerInterface *Self = (struct LibraryManagerInterface *) ExtLib->ILibrary;

	return (ULONG) Self->Open(0);
}
struct EmuTrap stub_Open = { TRAPINST, TRAPTYPE, stub_OpenPPC };

STATIC ULONG stub_ClosePPC(ULONG *regarray)
{
	struct Library *Base = (struct Library *) regarray[REG68K_A6/4];
	struct ExtendedLibrary *ExtLib = (struct ExtendedLibrary *) ((ULONG)Base + Base->lib_PosSize);
	struct LibraryManagerInterface *Self = (struct LibraryManagerInterface *) ExtLib->ILibrary;

	return (ULONG) Self->Close();
}
struct EmuTrap stub_Close = { TRAPINST, TRAPTYPE, stub_ClosePPC };

STATIC ULONG stub_ExpungePPC(ULONG *regarray)
{
  return 0UL;
}
struct EmuTrap stub_Expunge = { TRAPINST, TRAPTYPE, stub_ExpungePPC };

STATIC ULONG stub_ReservedPPC(ULONG *regarray)
{
  return 0UL;
}
struct EmuTrap stub_Reserved = { TRAPINST, TRAPTYPE, stub_ReservedPPC };

STATIC ULONG stub_SayHelloOS4PPC(ULONG *regarray)
{
	struct Library *Base = (struct Library *) regarray[REG68K_A6/4];
	struct ExtendedLibrary *ExtLib = (struct ExtendedLibrary *) ((ULONG)Base + Base->lib_PosSize);
	struct LibraryManagerInterface *Self = (struct LibraryManagerInterface *) ExtLib->ILibrary;

	return (char *)Self->SayHelloOS4();
}
struct EmuTrap stub_SayHelloOS4 = { TRAPINST, TRAPTYPE, stub_SayHelloOS4PPC };

STATIC ULONG stub_SayHelloOS3PPC(ULONG *regarray)
{
	struct Library *Base = (struct Library *) regarray[REG68K_A6/4];
	struct ExtendedLibrary *ExtLib = (struct ExtendedLibrary *) ((ULONG)Base + Base->lib_PosSize);
	struct LibraryManagerInterface *Self = (struct LibraryManagerInterface *) ExtLib->ILibrary;

	return (char *)Self->SayHelloOS3();
}
struct EmuTrap stub_SayHelloOS3 = { TRAPINST, TRAPTYPE, stub_SayHelloOS3PPC };

STATIC ULONG stub_SayHelloMOSPPC(ULONG *regarray)
{
	struct Library *Base = (struct Library *) regarray[REG68K_A6/4];
	struct ExtendedLibrary *ExtLib = (struct ExtendedLibrary *) ((ULONG)Base + Base->lib_PosSize);
	struct LibraryManagerInterface *Self = (struct LibraryManagerInterface *) ExtLib->ILibrary;

	return (char *)Self->SayHelloMOS();
}
struct EmuTrap stub_SayHelloMOS = { TRAPINST, TRAPTYPE, stub_SayHelloMOSPPC };

STATIC ULONG stub_UppercasePPC(ULONG *regarray)
{
	struct Library *Base = (struct Library *) regarray[REG68K_A6/4];
	struct ExtendedLibrary *ExtLib = (struct ExtendedLibrary *) ((ULONG)Base + Base->lib_PosSize);
	struct LibraryManagerInterface *Self = (struct LibraryManagerInterface *) ExtLib->ILibrary;

	return (char *)Self->Uppercase(
		(char *)regarray[8]
	);
}
struct EmuTrap stub_Uppercase = { TRAPINST, TRAPTYPE, stub_UppercasePPC };

STATIC ULONG stub_SPrintfAPPC(ULONG *regarray)
{
	struct Library *Base = (struct Library *) regarray[REG68K_A6/4];
	struct ExtendedLibrary *ExtLib = (struct ExtendedLibrary *) ((ULONG)Base + Base->lib_PosSize);
	struct LibraryManagerInterface *Self = (struct LibraryManagerInterface *) ExtLib->ILibrary;

	return (char *)Self->SPrintfA(
		(char *)regarray[8],
		(char *)regarray[9],
		(APTR)regarray[10]
	);
}
struct EmuTrap stub_SPrintfA = { TRAPINST, TRAPTYPE, stub_SPrintfAPPC };

STATIC ULONG VecTable68K[] =
{
  (ULONG)&stub_Open,
  (ULONG)&stub_Close,
  (ULONG)&stub_Expunge,
  (ULONG)&stub_Reserved,
  (ULONG)&stub_SayHelloOS4,
  (ULONG)&stub_SayHelloOS3,
  (ULONG)&stub_SayHelloMOS,
  (ULONG)&stub_Uppercase,
  (ULONG)&stub_SprintfA,
  (ULONG)-1
};

/* ----------------------- LibCreate Tags -------------------------- */

STATIC CONST struct TagItem LibCreateTags[] =
{
  {CLT_DataSize,   (ULONG)(sizeof(struct LibraryHeader))},
  {CLT_InitFunc,   (ULONG)LibInit},
  {CLT_Interfaces, (ULONG)LibInterfaces},
  {CLT_Vector68K,  (ULONG)VecTable68K},
  {TAG_DONE,       0}
};

#else

STATIC CONST APTR LibVectors[] =
{
  #ifdef __MORPHOS__
  (APTR)FUNCARRAY_32BIT_NATIVE,
  #endif
  #if defined(__AROS__)
    #ifdef AROS_ABI_V1
    AROS_SLIB_ENTRY(LibOpen, example, 1),
    AROS_SLIB_ENTRY(LibClose, example, 2),
    AROS_SLIB_ENTRY(LibExpunge, example, 3),
    #else
    AROS_SLIB_ENTRY(LibOpen, example),
    AROS_SLIB_ENTRY(LibClose, example),
    AROS_SLIB_ENTRY(LibExpunge, example),
    #endif
  #else
  (APTR)LibOpen,
  (APTR)LibClose,
  (APTR)LibExpunge,
  #endif
  (APTR)LibNull,
  (APTR)libvector,
  (APTR)-1
};

STATIC CONST ULONG LibInitTab[] =
{
  sizeof(struct LibraryHeader),
  (ULONG)LibVectors,
  (ULONG)NULL,
  (ULONG)LibInit
};

#endif

/* ------------------- ROM Tag ------------------------ */
static const USED_VAR struct Resident ROMTag =
{
  RTC_MATCHWORD,
  (struct Resident *)&ROMTag,
  (struct Resident *)&ROMTag + 1,
  #if defined(__amigaos4__)
  RTF_AUTOINIT|RTF_NATIVE,      // The Library should be set up according to the given table.
  #elif defined(__MORPHOS__)
  RTF_AUTOINIT|RTF_PPC,
  #elif defined(__AROS__)
  RTF_AUTOINIT|RTF_EXTENDED,
  #else
  RTF_AUTOINIT,
  #endif
  VERSION,
  NT_LIBRARY,
  0,
  (APTR)UserLibName,
  VSTRING,
  #if defined(__amigaos4__)
  (APTR)LibCreateTags           // This table is for initializing the Library.
  #else
  (APTR)LibInitTab
  #endif
};

#if defined(__MORPHOS__)
/*
 * To tell the loader that this is a new emulppc elf and not
 * one for the ppc.library.
 * ** IMPORTANT **
 */
ULONG	USED_VAR __amigappc__=1;
ULONG	USED_VAR __abox__=1;

#endif /* __MORPHOS */

/******************************************************************************/
/* Standard Library Functions, all of them are called in Forbid() state.      */
/******************************************************************************/

#ifndef __amigaos4__
#define DeleteLibrary(LIB) \
  FreeMem((STRPTR)(LIB)-(LIB)->lib_NegSize, (ULONG)((LIB)->lib_NegSize+(LIB)->lib_PosSize))
#endif

#if defined(__amigaos4__)
static struct LibraryHeader * LibInit(struct LibraryHeader *base, BPTR librarySegment, struct ExecIFace *pIExec)
{
  struct ExecBase *sb = (struct ExecBase *)pIExec->Data.LibBase;
  IExec = pIExec;
#elif defined(__MORPHOS__)
static struct LibraryHeader * LibInit(struct LibraryHeader *base, BPTR librarySegment, struct ExecBase *sb)
{
#elif defined(__AROS__)
static AROS_UFH3(struct LibraryHeader *, LibInit,
                 AROS_UFHA(struct LibraryHeader *, base, D0),
                 AROS_UFHA(BPTR, librarySegment, A0),
                 AROS_UFHA(struct ExecBase *, sb, A6)
)
{
  AROS_USERFUNC_INIT
#else
LIBFUNC static struct LibraryHeader * LibInit(REG(a0, BPTR librarySegment), REG(d0, struct LibraryHeader *base), REG(a6, struct ExecBase *sb))
{
#endif

  SysBase = (APTR)sb;

  base->libBase.lib_Node.ln_Type = NT_LIBRARY;
  base->libBase.lib_Node.ln_Pri  = 0;
  base->libBase.lib_Node.ln_Name = (char *)UserLibName;
  base->libBase.lib_Flags        = LIBF_CHANGED | LIBF_SUMUSED;
  base->libBase.lib_Version      = VERSION;
  base->libBase.lib_Revision     = REVISION;
  base->libBase.lib_IdString     = (char *)UserLibID;

  base->segList = librarySegment;
  base->sysBase = (APTR)SysBase;

  return(base);
#if defined(__AROS__)
    AROS_USERFUNC_EXIT
#endif
}

#if defined(__amigaos4__)
static BPTR LibExpunge(struct LibraryManagerInterface *Self)
{
  struct LibraryHeader *base = (struct LibraryHeader *)Self->Data.LibBase;
#elif defined(__MORPHOS__)
static BPTR LibExpunge(void)
{
	struct LibraryHeader *base = (struct LibraryHeader*)REG_A6;
#elif defined(__AROS__)
static AROS_LH1(BPTR, LibExpunge,
  AROS_LHA(UNUSED struct LibraryHeader *, __extrabase, D0),
  struct LibraryHeader *, base, 3, example
)
{
    AROS_LIBFUNC_INIT
#else
LIBFUNC static BPTR LibExpunge(REG(a6, struct LibraryHeader *base))
{
#endif
  BPTR rc;

  if(base->libBase.lib_OpenCnt > 0)
  {
    base->libBase.lib_Flags |= LIBF_DELEXP;
    return(0);
  }

  SysBase = (APTR)base->sysBase;
  rc = base->segList;

  Remove((struct Node *)base);
  DeleteLibrary(&base->libBase);

  return(rc);
#if defined(__AROS__)
  AROS_LIBFUNC_EXIT
#endif
}

#if defined(__amigaos4__)
static struct LibraryHeader *LibOpen(struct LibraryManagerInterface *Self, ULONG version)
{
  struct LibraryHeader *base = (struct LibraryHeader *)Self->Data.LibBase;
#elif defined(__MORPHOS__)
static struct LibraryHeader *LibOpen(void)
{
  struct LibraryHeader *base = (struct LibraryHeader*)REG_A6;
#elif defined(__AROS__)
static AROS_LH1(struct LibraryHeader *, LibOpen,
                AROS_LHA(UNUSED ULONG, version, D0),
                struct LibraryHeader *, base, 1, example
)
{
  AROS_LIBFUNC_INIT
#else
LIBFUNC static struct LibraryHeader * LibOpen(REG(a6, struct LibraryHeader *base))
{
#endif

  base->libBase.lib_Flags &= ~LIBF_DELEXP;
  base->libBase.lib_OpenCnt++;

  return base;
#if defined(__AROS__)
  AROS_LIBFUNC_EXIT
#endif
}

#if defined(__amigaos4__)
static BPTR LibClose(struct LibraryManagerInterface *Self)
{
  struct LibraryHeader *base = (struct LibraryHeader *)Self->Data.LibBase;
#elif defined(__MORPHOS__)
static BPTR LibClose(void)
{
  struct LibraryHeader *base = (struct LibraryHeader *)REG_A6;
#elif defined(__AROS__)
static AROS_LH0(BPTR, LibClose,
                struct LibraryHeader *, base, 2, example
)
{
  AROS_LIBFUNC_INIT
#else
LIBFUNC static BPTR LibClose(REG(a6, struct LibraryHeader *base))
{
#endif

  if(base->libBase.lib_OpenCnt > 0 &&
     --base->libBase.lib_OpenCnt == 0)
  {
    if(base->libBase.lib_Flags & LIBF_DELEXP)
    {
      #if defined(__amigaos4__)
      return LibExpunge(Self);
      #elif defined(__MORPHOS__)
      return LibExpunge();
      #elif defined(__AROS__)
      return example_3_LibExpunge(base, base);
      #else
      return LibExpunge(base);
      #endif
    }
  }

  return 0;
#if defined(__AROS__)
  AROS_LIBFUNC_EXIT
#endif
}
