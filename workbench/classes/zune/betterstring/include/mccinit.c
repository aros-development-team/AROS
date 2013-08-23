/*******************************************************************************

        Name:           mccinit.c
        Versionstring:  $VER: mccinit.c 1.26 (06.03.2013)
        Author:         Jens Langner <Jens.Langner@light-speed.de>
        Distribution:   PD (public domain)
        Description:    library init file for easy generation of a MUI
                        custom classes (MCC/MCP)
 History:

  1.0   09.06.2007 : created based on obsolete mccheader.c (damato)
  1.1   10.06.2007 : modified LibInit/LibOpen/LibExpunge to call the actual
                     ClassOpen() in LibOpen() rather than in LibInit(). This
                     should prevent stack issues common on e.g. OS3. (damato)
  1.2   01.07.2007 : adapted library interface initialization to what the
                     very latest idltool 52.7 produces as well.
  1.3   04.07.2007 : added MIN_STACKSIZE and all required StackSwap()
                     mechanisms to enforce a large enough stack.
  1.4   04.07.2007 : put the StackSwapStruct structure on the stack to avoid
                     crashes on OS3/MOS.
  1.5   18.07.2007 : added new inline assember stackswap_call() function which
                     allows to call a function within a new stack frame
                     initiated by StackSwap(). This should make the whole
                     stack swapping mechanism more safe.
  1.6   24.07.2007 : corrected an else-branch which only exists if CLASSINIT
                     is defined and added an UNUSED extension to the Expunge()
                     function in case the base parameter is not used.
  1.7   25.07.2007 : adapted GETINTERFACE() and library base definitions so
                     that mccinit.c can also be used with C++
  1.8   25.07.2007 : removed the obsolete 2-parameter version of GETINTERFACE()
                     from mcc_common.h and adapted all calls accordingly.
                     Also moved the inclusion of mccinit.c in front of all
                     user definable functions.
  1.9   09.08.2007 : applied a patch kindly provided by Ilkka Lehtoranta which
                     replaces the assembler code for stack swapping with the
                     appropriate call to the NewPPCStackSwap() function in
                     MorphOS. In addition, the stack size will now be properly
                     checked before a stack swap is attempted.
  1.10  13.08.2007 : the StackSwap structure itself *must* *not* be placed on the
                     stack which will be swapped later, because swapping it back
                     will access the wrong place in memory. Hence this structure
                     is allocated from global memory now.
  1.11  01.02.2008 : fixed some minor compiler warnings when compiled using the
                     MorphOS SDK.
  1.12  27.03.2009 : integrated some changes which should make mccinit usable
                     for AROS builds.
  1.13  01.04.2009 : fixed the broken prototype for the assembler stackswap_call
                     function.
  1.14  02.05.2009 : added RTF_EXTENDED for the MorphOS build as well
  1.15  24.05.2009 : fixed some compiler warnings appear on AROS compile
  1.16  25.05.2009 : fixed some compiler warnings appear on OS3/MOS compile
  1.17  02.06.2009 : more fixes to better comply for AROS compilation
  1.18  24.04.2010 : fixed stack swapping for AROS
  1.19  25.05.2010 : updated for compatibility with AROS V1 API
  1.20  01.06.2010 : added CleanupDebug() call to expunge function.
  1.21  17.08.2010 : the UserLibName and UserLibID strings are now correctly
                     placed in the .data instead of the .text section. Also made
                     sure that the _start() function is really the first entry,
                     otherwise random data will be executed as code, which will
                     crash for sure.
  1.22  03.09.2010 : the library semaphore is now correctly cleared ahead of the
                     InitSemaphore() call.
  1.23  07.09.2010 : added missing #include <string.h> for memset().
  1.24  05.10.2010 : make sure that removing the library during LibClose() really
                     operates on the correct base. Calling LibExpunge() on MorphOS
                     is wrong, since that takes no parameter but expects the base
                     to be in A6. We work around this by using an additional
                     function which gets called from LibClose() and LibExpunge()
                     with the correct base pointer.
  1.25  20.12.2010 : minimum required system version is now OS3.0 (V39).
  1.26  06.03.2013 : removed _start entry point. This must be defined separately
                     to ensure it is the very first piece of code in the final
                     binary file.
  WIP   23.08.2013   fix for making it compilabe for both ABIv1 and v0 of AROS.

 About:

  The purpose of this source file is to provide a template for the library init
  code for a creation of an own MUI custom class (mcc/mcp) for AmigaOS4,
  AmigaOS3 and MorphOS. By directly including this file (#include "mccinit.c")
  and defining certain preprocessor values, a MUI developer doesn't have to
  care about library init stuff which is normally highly system dependent and
  various between different Amiga operating systems.

 Usage:

  This file should be included by another source file (e.g. 'library.c') in
  your main development branch while certain preprocessor macros right before
  the include statement can be defined to change the behaviour of mccinit.c.
  These possible macros are:

    USERLIBID     (char*)   - version string for the mcc/mcp (exluding $VER:)
    VERSION       (int)     - version number (must match USERLIBID)
    REVISION      (int)     - revision number (must match USERLIBID)
    CLASS         (char*)   - class name (including .mcc/.mcp)
    MASTERVERSION (int)     - the minimun required muimaster version
                              (default: MUIMASTER_VMIN)
    MIN_STACKSIZE (int)     - if defined, the specified minimum stack size
                              will be enforced when calling all user functions
                              like ClassXXXX() and PreClassXXX().

    MCC only:
    --------
    SUPERCLASS  (char*)   - superclass ID of MCC (e.g. MUIC_Area)
    INSTDATA              - name of instance data structure of MCC (e.g. InstData)
    USEDCLASSES           - name of NULL terminated string array which contains
                            names of other required custom classes for MCC.
    _Dispatcher           - name of Dispatcher function for MCC

    MCP only:
    -------
    SUPERCLASSP (char*)   - superclass ID of MCP (e.g. MUIC_Mccprefs)
    INSTDATAP             - name of instance data structure of MCP
    USEDCLASSESP          - name of NULL terminated string array which contains
                            names of other required custom classes for MCC.
    SHORTHELP   (char*)   - alternative help text for prefs program's listview
    PREFSIMAGEOBJECT      - pointer to the image object for the MCP
    _DispatcherP          - name of Dispatcher function for MCP

  In addition, the following defines and functions can be defined or are
  required:

    CLASSINIT     - if defined, a "BOOL ClassInit(struct Library *base)"
                    function can be defined in your own code and will be called
                    right after the general library initialization are
                    finished. This function should then open own libraries
                    or do own library initialization tasks as it is only called
                    once at the very first library/class open.

    CLASSEXPUNGE  - if defined a "VOID ClassExpunge(struct Library *base)"
                    function can be defined in your own code and will be called
                    as soon as the library will be freed/expunged by the
                    operating system. In this function you should close/free
                    stuff you normally opened/allocated in CLASSINIT.

    CLASSOPEN     - if defined, a "BOOL ClassOpen(struct Library *base)"
                    function can be defined in your own code and will be called
                    right after each single application opens the custom class.
                    In this function you can then set flags or do library open
                    specific tasks.

    CLASSCLOSE    - if defined a "VOID ClassClose(struct Library *base)"
                    function can be defined in your own code and will be called
                    right after the library was successfully flagged as closed
                    by the CloseLibrary() call of an application.


    PRECLASSINIT      - if defined a "BOOL PreClassInit(struct Library *base)"
                        function can be defined and will be called right _before_
                        and library initialization takes place.

    POSTCLASSEXPUNGE  - if defined a "BOOL PostClassExpunge(struct Library *base)"
                        function can be defined and will be called right _after_
                        the custom class was free via MUI_DeleteCustomClass() in
                        the library expunge phase.

   Warning:
   -------
   The above class functions are normally called by the operating system
   in a Forbid()/Permit() state. That means you are supposed to make sure that
   your operations doesn't break the Forbid() state or otherwise you may run
   into a race condition. However, we have added semaphore locking to partly
   protect you from that case - but you should still consider doing processor
   intensitive tasks in a library's own function instead of using those
   class initialization functions.

*******************************************************************************/

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/

#include <string.h>

#ifdef __MORPHOS__
#include <emul/emulinterface.h>
#include <emul/emulregs.h>
#endif

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#ifdef __AROS__
#include <aros/libcall.h>
#include <utility/utility.h>
#include <aros/config.h>
#endif

#include "SDI_compiler.h"

#if defined(__amigaos4__)
struct Library *MUIMasterBase = NULL;
struct Library *SysBase       = NULL;
struct Library *UtilityBase   = NULL;
struct Library *DOSBase       = NULL;
struct Library *GfxBase       = NULL;
struct Library *IntuitionBase = NULL;
struct ExecIFace *IExec       = NULL;
struct MUIMasterIFace *IMUIMaster = NULL;
struct UtilityIFace *IUtility     = NULL;
struct DOSIFace *IDOS             = NULL;
struct GraphicsIFace *IGraphics   = NULL;
struct IntuitionIFace *IIntuition = NULL;
#if defined(__NEWLIB__)
struct Library *NewlibBase = NULL;
struct Interface *INewlib = NULL;
#endif
#else
struct Library        *MUIMasterBase = NULL;
struct ExecBase       *SysBase       = NULL;
#if defined(__AROS__)
struct UtilityBase    *UtilityBase   = NULL;
#else
struct Library        *UtilityBase   = NULL;
#endif
struct DosLibrary     *DOSBase       = NULL;
struct GfxBase        *GfxBase       = NULL;
struct IntuitionBase  *IntuitionBase = NULL;
#endif

#ifdef __cplusplus
extern "C" {
#endif

// we place a stack cookie in the binary so that
// newer OS version can directly take the specified
// number for the ramlib process
#if defined(MIN_STACKSIZE)

// transforms a define into a string
#define STR(x)  STR2(x)
#define STR2(x) #x

#ifdef __amigaos4__
STATIC const char USED_VAR stack_size[] = "$STACK:" STR(MIN_STACKSIZE) "\n";
#endif
#endif

/* The name of the class will also become the name of the library. */
/* We need a pointer to this string in our ROMTag (see below). */
STATIC const char UserLibName[] = CLASS;
STATIC const char UserLibID[]   = "$VER: " USERLIBID;

#ifdef SUPERCLASS
STATIC struct MUI_CustomClass *ThisClass = NULL;
DISPATCHERPROTO(_Dispatcher);
#endif

#ifdef SUPERCLASSP
STATIC struct MUI_CustomClass *ThisClassP = NULL;
DISPATCHERPROTO(_DispatcherP);
#endif

#ifdef __GNUC__

  #if !defined(__NEWLIB__)
    #if defined(__amigaos4__)
    extern struct Library *__UtilityBase;   // clib2
    extern struct UtilityIFace* __IUtility; // clib2
    #else
    struct Library *__UtilityBase = NULL; // required by libnix & clib2
    #endif
  #endif

  /* these one are needed copies for libnix.a */
  #ifdef __libnix__
    #ifdef USE_MATHIEEEDOUBBASBASE
    struct Library *__MathIeeeDoubBasBase = NULL;
    #endif
    #ifdef USE_MATHIEEEDOUBTRANSBASE
    struct Library *__MathIeeeDoubTransBase = NULL;
    #endif
  #endif

#endif /* __GNUC__ */


// define own macros for the OS4 interfaces
#undef GETINTERFACE
#undef DROPINTERFACE
#if defined(__amigaos4__)
#define GETINTERFACE(iface, type, base)	(iface = (type)GetInterface((struct Library *)(base), "main", 1L, NULL))
#define DROPINTERFACE(iface)			(DropInterface((struct Interface *)iface), iface = NULL)
#else
#define GETINTERFACE(iface, type, base)	TRUE
#define DROPINTERFACE(iface)
#endif

// in case the user didn't specify an own minimum
// muimaster version we increase it here.
#ifndef MASTERVERSION
#define MASTERVERSION MUIMASTER_VMIN
#endif

/* Our library structure, consisting of a struct Library, a segment pointer */
/* and a semaphore. We need the semaphore to protect init/exit stuff in our */
/* open/close fuSnctions */
struct LibraryHeader
{
  struct Library         lh_Library;
  UWORD                  lh_Pad1;
  BPTR                   lh_Segment;
  struct SignalSemaphore lh_Semaphore;
  UWORD                  lh_Pad2;
};

/******************************************************************************/
/* Local Structures & Prototypes                                              */
/******************************************************************************/

#if defined(__amigaos4__)

STATIC struct LibraryHeader * LIBFUNC LibInit    (struct LibraryHeader *base, BPTR librarySegment, struct ExecIFace *pIExec);
STATIC BPTR                   LIBFUNC LibExpunge (struct LibraryManagerInterface *Self);
STATIC struct LibraryHeader * LIBFUNC LibOpen    (struct LibraryManagerInterface *Self, ULONG version);
STATIC BPTR                   LIBFUNC LibClose   (struct LibraryManagerInterface *Self);
STATIC IPTR                   LIBFUNC MCC_Query  (UNUSED struct Interface *self, REG(d0, LONG which));

#elif defined(__MORPHOS__)

STATIC struct LibraryHeader * LIBFUNC LibInit    (struct LibraryHeader *base, BPTR librarySegment, struct ExecBase *sb);
STATIC BPTR                   LIBFUNC LibExpunge (void);
STATIC struct LibraryHeader * LIBFUNC LibOpen    (void);
STATIC BPTR                   LIBFUNC LibClose   (void);
STATIC LONG                   LIBFUNC LibNull    (void);
STATIC IPTR                   LIBFUNC MCC_Query  (void);

#elif defined(__AROS__)

AROS_UFP3 (struct LibraryHeader *, LibInit,
  AROS_UFPA(struct LibraryHeader *, base, D0),
  AROS_UFPA(BPTR, librarySegment, A0),
  AROS_UFPA(struct ExecBase *, sb, A6)
);
AROS_LD1(BPTR, LibExpunge,
  AROS_LPA(struct LibraryHeader *, base, D0),
  struct LibraryHeader *, base, 3, __MCC_
);
AROS_LD1 (struct LibraryHeader *, LibOpen,
  AROS_LHA (ULONG, version, D0),
  struct LibraryHeader *, base, 1, __MCC_
);
AROS_LD0 (BPTR, LibClose,
  struct LibraryHeader *, base, 2, __MCC_
);
AROS_LD1(IPTR, MCC_Query,
  AROS_LHA(LONG, what, D0),
  struct LibraryHeader *, LIBBASE, 5, __MCC_
);

#else

STATIC struct LibraryHeader * LIBFUNC LibInit    (REG(d0, struct LibraryHeader *base), REG(a0, BPTR librarySegment), REG(a6, struct ExecBase *sb));
STATIC BPTR                   LIBFUNC LibExpunge (REG(a6, struct LibraryHeader *base));
STATIC struct LibraryHeader * LIBFUNC LibOpen    (REG(d0, ULONG version), REG(a6, struct LibraryHeader *base));
STATIC BPTR                   LIBFUNC LibClose   (REG(a6, struct LibraryHeader *base));
STATIC LONG                   LIBFUNC LibNull    (void);
STATIC IPTR                   LIBFUNC MCC_Query  (REG(d0, LONG which));

#endif

/******************************************************************************/
/* Dummy LibNull() function                                                   */
/******************************************************************************/

#if !defined(__amigaos4__)
STATIC LONG LIBFUNC LibNull(VOID)
{
  return(0);
}
#endif

/******************************************************************************/
/* Local data structures                                                      */
/******************************************************************************/

#if defined(__amigaos4__)
/* ------------------- OS4 Manager Interface ------------------------ */
STATIC uint32 _manager_Obtain(struct LibraryManagerInterface *Self)
{
  uint32 res;
  __asm__ __volatile__(
  "1: lwarx %0,0,%1\n"
  "addic  %0,%0,1\n"
  "stwcx. %0,0,%1\n"
  "bne- 1b"
  : "=&r" (res)
  : "r" (&Self->Data.RefCount)
  : "cc", "memory");

  return res;
}

STATIC uint32 _manager_Release(struct LibraryManagerInterface *Self)
{
  uint32 res;
  __asm__ __volatile__(
  "1: lwarx %0,0,%1\n"
  "addic  %0,%0,-1\n"
  "stwcx. %0,0,%1\n"
  "bne- 1b"
  : "=&r" (res)
  : "r" (&Self->Data.RefCount)
  : "cc", "memory");

  return res;
}

STATIC CONST CONST_APTR lib_manager_vectors[] =
{
  (CONST_APTR)_manager_Obtain,
  (CONST_APTR)_manager_Release,
  (CONST_APTR)NULL,
  (CONST_APTR)NULL,
  (CONST_APTR)LibOpen,
  (CONST_APTR)LibClose,
  (CONST_APTR)LibExpunge,
  (CONST_APTR)NULL,
  (CONST_APTR)-1
};

STATIC CONST struct TagItem lib_managerTags[] =
{
  { MIT_Name,         (Tag)"__library" },
  { MIT_VectorTable,  (Tag)lib_manager_vectors },
  { MIT_Version,      1 },
  { TAG_DONE,         0 }
};

/* ------------------- Library Interface(s) ------------------------ */

ULONG _MCCClass_Obtain(UNUSED struct Interface *Self)
{
  return 0;
}

ULONG _MCCClass_Release(UNUSED struct Interface *Self)
{
  return 0;
}

STATIC CONST CONST_APTR main_vectors[] =
{
  (CONST_APTR)_MCCClass_Obtain,
  (CONST_APTR)_MCCClass_Release,
  (CONST_APTR)NULL,
  (CONST_APTR)NULL,
  (CONST_APTR)MCC_Query,
  (CONST_APTR)-1
};

STATIC CONST struct TagItem mainTags[] =
{
  { MIT_Name,         (Tag)"main" },
  { MIT_VectorTable,  (Tag)main_vectors },
  { MIT_Version,      1 },
  { TAG_DONE,         0 }
};

STATIC CONST CONST_APTR libInterfaces[] =
{
  (CONST_APTR)lib_managerTags,
  (CONST_APTR)mainTags,
  (CONST_APTR)NULL
};

// Our libraries always have to carry a 68k jump table with it, so
// lets define it here as extern, as we are going to link it to
// our binary here.
#ifndef NO_VECTABLE68K
extern CONST APTR VecTable68K[];
#endif

STATIC CONST struct TagItem libCreateTags[] =
{
  { CLT_DataSize,   sizeof(struct LibraryHeader) },
  { CLT_InitFunc,   (Tag)LibInit },
  { CLT_Interfaces, (Tag)libInterfaces },
  #ifndef NO_VECTABLE68K
  { CLT_Vector68K,  (Tag)VecTable68K },
  #endif
  { TAG_DONE,       0 }
};

#else

STATIC CONST CONST_APTR LibVectors[] =
{
  #ifdef __MORPHOS__
  (CONST_APTR)FUNCARRAY_32BIT_NATIVE,
  #endif
  #ifndef __AROS__
  (CONST_APTR)LibOpen,
  (CONST_APTR)LibClose,
  (CONST_APTR)LibExpunge,
  (CONST_APTR)LibNull,
  (CONST_APTR)MCC_Query,
  #else
    #if !defined(AROS_ABI) || (AROS_ABI == 0)
    /* Do ABIv0 stuff here */
    (CONST_APTR)AROS_SLIB_ENTRY(LibOpen, __MCC_),
    (CONST_APTR)AROS_SLIB_ENTRY(LibClose, __MCC_),
    (CONST_APTR)AROS_SLIB_ENTRY(LibExpunge, __MCC_),
    (CONST_APTR)LibNull,
    (CONST_APTR)AROS_SLIB_ENTRY(MCC_Query, __MCC_),
    #else
    /* Do ABIv1 stuff here */
    (CONST_APTR)AROS_SLIB_ENTRY(LibOpen, __MCC_, 1),
    (CONST_APTR)AROS_SLIB_ENTRY(LibClose, __MCC_, 2),
    (CONST_APTR)AROS_SLIB_ENTRY(LibExpunge, __MCC_, 3),
    (CONST_APTR)LibNull,
    (CONST_APTR)AROS_SLIB_ENTRY(MCC_Query, __MCC_, 5),
    #endif
  #endif
  (CONST_APTR)-1
};

STATIC CONST IPTR LibInitTab[] =
{
  sizeof(struct LibraryHeader),
  (IPTR)LibVectors,
  (IPTR)NULL,
  (IPTR)LibInit
};

#endif

/* ------------------- ROM Tag ------------------------ */
STATIC const USED_VAR struct Resident ROMTag =
{
  RTC_MATCHWORD,
  (struct Resident *)&ROMTag,
  (struct Resident *)(&ROMTag + 1),
  #if defined(__amigaos4__)
  RTF_AUTOINIT|RTF_NATIVE,      // The Library should be set up according to the given table.
  #elif defined(__MORPHOS__)
  RTF_AUTOINIT|RTF_EXTENDED|RTF_PPC,
  #elif defined(__AROS__)
  RTF_AUTOINIT|RTF_EXTENDED,
  #else
  RTF_AUTOINIT,
  #endif
  VERSION,
  NT_LIBRARY,
  0,
  (char *)UserLibName,
  (char *)UserLibID+6,          // +6 to skip '$VER: '
  #if defined(__amigaos4__)
  (APTR)libCreateTags,          // This table is for initializing the Library.
  #else
  (APTR)LibInitTab,
  #endif
  #if defined(__MORPHOS__) || defined(__AROS__)
  REVISION,
  0
  #endif
};

#if defined(__MORPHOS__)
/*
 * To tell the loader that this is a new emulppc elf and not
 * one for the ppc.library.
 * ** IMPORTANT **
 */
const USED_VAR ULONG __abox__ = 1;

#endif /* __MORPHOS__ */

/****************************************************************************/
/* Stack enforcing function which allows to make sure that a function has   */
/* enough stack space during its execution time                             */
/****************************************************************************/

#if defined(MIN_STACKSIZE) && !defined(__amigaos4__)

/* generic StackSwap() function which calls function() surrounded by
   StackSwap() calls */
#if defined(__AROS__)
ULONG stackswap_call(struct StackSwapStruct *stack,
                             ULONG (*function)(struct LibraryHeader *),
                             struct LibraryHeader *arg)
{
   struct StackSwapArgs swapargs;

   swapargs.Args[0] = (IPTR)arg;

   return NewStackSwap(stack, function, &swapargs);
}
#elif defined(__MORPHOS__)
ULONG stackswap_call(struct StackSwapStruct *stack,
                     ULONG (*function)(struct LibraryHeader *),
                     struct LibraryHeader *arg)
{
   struct PPCStackSwapArgs swapargs;

   swapargs.Args[0] = (ULONG)arg;

   return NewPPCStackSwap(stack, function, &swapargs);
}
#elif defined(__mc68000__)
ULONG stackswap_call(struct StackSwapStruct *stack,
                     ULONG (*function)(struct LibraryHeader *),
                     struct LibraryHeader *arg);

asm(".text                    \n\
     .even                    \n\
     .globl _stackswap_call   \n\
   _stackswap_call:           \n\
      moveml #0x3022,sp@-     \n\
      movel sp@(20),d3        \n\
      movel sp@(24),a2        \n\
      movel sp@(28),d2        \n\
      movel _SysBase,a6       \n\
      movel d3,a0             \n\
      jsr a6@(-732:W)         \n\
      movel d2,sp@-           \n\
      jbsr a2@                \n\
      movel d0,d2             \n\
      addql #4,sp             \n\
      movel _SysBase,a6       \n\
      movel d3,a0             \n\
      jsr a6@(-732:W)         \n\
      movel d2,d0             \n\
      moveml sp@+,#0x440c     \n\
      rts");
#else
#error Bogus operating system
#endif

STATIC BOOL callMccFunction(ULONG (*function)(struct LibraryHeader *), struct LibraryHeader *arg)
{
  BOOL success = FALSE;
  struct Task *tc;
  ULONG stacksize;

  // retrieve the task structure for the
  // current task
  tc = FindTask(NULL);

  #if defined(__MORPHOS__)
  // In MorphOS we have two stacks. One for PPC code and another for 68k code.
  // We are only interested in the PPC stack.
  NewGetTaskAttrsA(tc, &stacksize, sizeof(ULONG), TASKINFOTYPE_STACKSIZE, NULL);
  #else
  // on all other systems we query via SPUpper-SPLower calculation
  stacksize = (IPTR)tc->tc_SPUpper - (IPTR)tc->tc_SPLower;
  #endif

  // Swap stacks only if current stack is insufficient
  if(stacksize < MIN_STACKSIZE)
  {
    struct StackSwapStruct *stack;

    if((stack = AllocVec(sizeof(*stack), MEMF_PUBLIC)) != NULL)
    {
      if((stack->stk_Lower = AllocVec(MIN_STACKSIZE, MEMF_PUBLIC)) != NULL)
      {
        // perform the StackSwap
        #if defined(__AROS__)
        // AROS uses an APTR type for stk_Upper
        stack->stk_Upper = (APTR)((IPTR)stack->stk_Lower + MIN_STACKSIZE);
        #else
        // all other systems use ULONG
        stack->stk_Upper = (ULONG)stack->stk_Lower + MIN_STACKSIZE;
        #endif
        stack->stk_Pointer = (APTR)stack->stk_Upper;

        // call routine but with embedding it into a [NewPPC]StackSwap()
        success = stackswap_call(stack, function, arg);

        FreeVec(stack->stk_Lower);
      }
      FreeVec(stack);
    }
  }
  else
    success = function(arg);

  return success;
}
#else // MIN_STACKSIZE && __amigaos4__
#define callMccFunction(func, arg) func(arg)
#endif // MIN_STACKSIZE && __amigaos4__

/******************************************************************************/
/* Wrapper functions to perform certain tasks in our LibInit/LibOpen etc.     */
/******************************************************************************/

/* open and init all necessary library and stuff in the LibInit() phase */
STATIC ULONG mccLibInit(struct LibraryHeader *base)
{
  // now that this library/class is going to be initialized for the first time
  // we go and open all necessary libraries on our own
  #if defined(__amigaos4__)
  if((DOSBase = OpenLibrary("dos.library", 39)) &&
     GETINTERFACE(IDOS, struct DOSIFace *, DOSBase))
  if((GfxBase = OpenLibrary("graphics.library", 39)) &&
     GETINTERFACE(IGraphics, struct GraphicsIFace *, GfxBase))
  if((IntuitionBase = OpenLibrary("intuition.library", 39)) &&
     GETINTERFACE(IIntuition, struct IntuitionIFace *, IntuitionBase))
  if((UtilityBase = OpenLibrary("utility.library", 39)) &&
     GETINTERFACE(IUtility, struct UtilityIFace *, UtilityBase))
  #else
  if((DOSBase = (struct DosLibrary*)OpenLibrary("dos.library", 39)) &&
     (GfxBase = (struct GfxBase*)OpenLibrary("graphics.library", 39)) &&
     (IntuitionBase = (struct IntuitionBase*)OpenLibrary("intuition.library", 39)) &&
     (UtilityBase = (APTR)OpenLibrary("utility.library", 39)))
  #endif
  {
    // we have to please the internal utilitybase
    // pointers of libnix and clib2
    #if !defined(__NEWLIB__)
      __UtilityBase = (APTR)UtilityBase;
      #if defined(__amigaos4__)
      __IUtility = IUtility;
      #endif
    #endif

    #if defined(DEBUG)
    SetupDebug();
    #endif

    if((MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MASTERVERSION)) &&
       GETINTERFACE(IMUIMaster, struct MUIMasterIFace*, MUIMasterBase))
    {
      #if defined(PRECLASSINIT)
      if(PreClassInit())
      #endif
      {
        #ifdef SUPERCLASS
        ThisClass = MUI_CreateCustomClass(&base->lh_Library, (STRPTR)SUPERCLASS, NULL, sizeof(struct INSTDATA), ENTRY(_Dispatcher));
        if(ThisClass)
        #endif
        {
          #ifdef SUPERCLASSP
          if((ThisClassP = MUI_CreateCustomClass(&base->lh_Library, (STRPTR)SUPERCLASSP, NULL, sizeof(struct INSTDATAP), ENTRY(_DispatcherP))))
          #endif
          {
            #ifdef SUPERCLASS
            #define THISCLASS ThisClass
            #else
            #define THISCLASS ThisClassP
            #endif

            // in case the user defined an own ClassInit()
            // function we call it protected by a semaphore as
            // this user may be stupid and break the Forbid() state
            // of LibInit()
            #if defined(CLASSINIT)
            if(ClassInit(&base->lh_Library))
            #endif
            {
              // everything was successfully so lets
              // make sure we return TRUE
              return TRUE;
            }
            #if defined(CLASSINIT)
            else
              E(DBF_STARTUP, "ClassInit(%s) failed", CLASS);
            #endif

            // if we pass this point than an error
            // occurred and we have to cleanup
            #if defined(SUPERCLASSP)
            MUI_DeleteCustomClass(ThisClassP);
            ThisClassP = NULL;
            #endif
          }

          #if defined(SUPERCLASS)
          MUI_DeleteCustomClass(ThisClass);
          ThisClass = NULL;
          #endif
        }
      }

      DROPINTERFACE(IMUIMaster);
      CloseLibrary(MUIMasterBase);
      MUIMasterBase = NULL;
    }

    DROPINTERFACE(IUtility);
    CloseLibrary((struct Library *)UtilityBase);
    UtilityBase = NULL;
  }

  if(IntuitionBase)
  {
    DROPINTERFACE(IIntuition);
    CloseLibrary((struct Library *)IntuitionBase);
    IntuitionBase = NULL;
  }

  if(GfxBase)
  {
    DROPINTERFACE(IGraphics);
    CloseLibrary((struct Library *)GfxBase);
    GfxBase = NULL;
  }

  if(DOSBase)
  {
    DROPINTERFACE(IDOS);
    CloseLibrary((struct Library *)DOSBase);
    DOSBase = NULL;
  }

  E(DBF_STARTUP, "mccLibInit(%s) failed", CLASS);
  return FALSE;
}

/* expunge everything we previously opened and call user definable functions */
STATIC ULONG mccLibExpunge(UNUSED struct LibraryHeader *base)
{
  // in case the user specified that he has an own class
  // expunge function we call it right here, not caring about
  // any return value.
  #if defined(CLASSEXPUNGE)
  ClassExpunge(&base->lh_Library);
  #endif

  // now we remove our own stuff here step-by-step
  #ifdef SUPERCLASSP
  if(ThisClassP)
  {
    MUI_DeleteCustomClass(ThisClassP);
    ThisClassP = NULL;
  }
  #endif

  #ifdef SUPERCLASS
  if(ThisClass)
  {
    MUI_DeleteCustomClass(ThisClass);
    ThisClass = NULL;
  }
  #endif

  // we inform the user that all main class expunge stuff
  // is finished, if he want's to get informed.
  #if defined(POSTCLASSEXPUNGE)
  PostClassExpunge();
  #endif

  #if defined(DEBUG)
  CleanupDebug();
  #endif

  // cleanup the various library bases and such
  if(MUIMasterBase)
  {
    DROPINTERFACE(IMUIMaster);
    CloseLibrary(MUIMasterBase);
    MUIMasterBase = NULL;
  }

  if(UtilityBase)
  {
    DROPINTERFACE(IUtility);
    CloseLibrary((struct Library *)UtilityBase);
    UtilityBase = NULL;
  }

  if(IntuitionBase)
  {
    DROPINTERFACE(IIntuition);
    CloseLibrary((struct Library *)IntuitionBase);
    IntuitionBase = NULL;
  }

  if(GfxBase)
  {
    DROPINTERFACE(IGraphics);
    CloseLibrary((struct Library *)GfxBase);
    GfxBase = NULL;
  }

  if(DOSBase)
  {
    DROPINTERFACE(IDOS);
    CloseLibrary((struct Library *)DOSBase);
    DOSBase = NULL;
  }

  return TRUE;
}

/* we call the user definable function here only */
#if defined(CLASSOPEN)
STATIC ULONG mccLibOpen(struct LibraryHeader *base)
{
  return ClassOpen(&base->lh_Library);
}
#endif

/* we call the user definable function here only */
#if defined(CLASSCLOSE)
STATIC ULONG mccLibClose(struct LibraryHeader *base)
{
  ClassClose(&base->lh_Library);
  return TRUE;
}
#endif

/******************************************************************************/
/* Standard Library Functions, all of them are called in Forbid() state.      */
/******************************************************************************/

#if defined(__amigaos4__)
STATIC struct LibraryHeader * LibInit(struct LibraryHeader *base, BPTR librarySegment, struct ExecIFace *pIExec)
{
  struct Library *sb = (struct Library *)pIExec->Data.LibBase;
  IExec = pIExec;
#elif defined(__MORPHOS__)
STATIC struct LibraryHeader * LibInit(struct LibraryHeader *base, BPTR librarySegment, struct ExecBase *sb)
{
#elif defined(__AROS__)
AROS_UFH3 (struct LibraryHeader *, LibInit,
    AROS_UFHA(struct LibraryHeader *, base, D0),
    AROS_UFHA(BPTR, librarySegment, A0),
    AROS_UFHA(struct ExecBase *, sb, A6)
)
{
  AROS_USERFUNC_INIT
#else
STATIC struct LibraryHeader * LIBFUNC LibInit(REG(d0, struct LibraryHeader *base), REG(a0, BPTR librarySegment), REG(a6, struct ExecBase *sb))
{
#endif

  SysBase = sb;

  // make sure that this is really a 68020+ machine if optimized for 020+
  #if _M68060 || _M68040 || _M68030 || _M68020 || __mc68020 || __mc68030 || __mc68040 || __mc68060
  if(!(SysBase->AttnFlags & AFF_68020))
    return(NULL);
  #endif

  #if defined(__amigaos4__) && defined(__NEWLIB__)
  if((NewlibBase = OpenLibrary("newlib.library", 3)) &&
     GETINTERFACE(INewlib, struct Interface*, NewlibBase))
  #endif
  {
    BOOL success = FALSE;

    D(DBF_STARTUP, "LibInit(" CLASS ")");

    // cleanup the library header structure beginning with the
    // library base, even if that is done automcatically, we explicitly
    // do it here for consistency reasons.
    base->lh_Library.lib_Node.ln_Type = NT_LIBRARY;
    base->lh_Library.lib_Node.ln_Pri  = 0;
    base->lh_Library.lib_Node.ln_Name = (char *)UserLibName;
    base->lh_Library.lib_Flags        = LIBF_CHANGED | LIBF_SUMUSED;
    base->lh_Library.lib_IdString     = (char *)UserLibID; // here without +6 or otherwise MUI doesn't identify it.
    base->lh_Library.lib_Version      = VERSION;
    base->lh_Library.lib_Revision     = REVISION;

    // init our protecting semaphore and the
    // initialized flag variable
    memset(&base->lh_Semaphore, 0, sizeof(base->lh_Semaphore));
    InitSemaphore(&base->lh_Semaphore);

    // protect mccLibInit()
    ObtainSemaphore(&base->lh_Semaphore);

    // If we are not running on AmigaOS4 (no stackswap required) we go and
    // do an explicit StackSwap() in case the user wants to make sure we
    // have enough stack for his user functions
    success = callMccFunction(mccLibInit, base);

    // unprotect
    ReleaseSemaphore(&base->lh_Semaphore);

    // check if everything worked out fine
    if(success == TRUE)
    {
      // everything was successfully so lets
      // set the initialized value and contiue
      // with the class open phase
      base->lh_Segment = librarySegment;

      // return the library base as success
      return base;
    }

    #if defined(__amigaos4__) && defined(__NEWLIB__)
    if(NewlibBase)
    {
      DROPINTERFACE(INewlib);
      CloseLibrary(NewlibBase);
      NewlibBase = NULL;
    }
    #endif
  }

  return(NULL);
#ifdef __AROS__
  AROS_USERFUNC_EXIT
#endif
}

/*****************************************************************************************************/
/*****************************************************************************************************/

#ifndef __amigaos4__
#define DeleteLibrary(LIB) \
  FreeMem((STRPTR)(LIB)-(LIB)->lib_NegSize, (ULONG)((LIB)->lib_NegSize+(LIB)->lib_PosSize))
#endif

STATIC BPTR LibDelete(struct LibraryHeader *base)
{
#if defined(__amigaos4__)
  struct ExecIFace *IExec = (struct ExecIFace *)(*(struct ExecBase **)4)->MainInterface;
#endif
  BPTR rc;

  // remove the library base from exec's lib list in advance
  Remove((struct Node *)base);

  // protect mccLibExpunge()
  ObtainSemaphore(&base->lh_Semaphore);

  // make sure we have enough stack here
  callMccFunction(mccLibExpunge, base);

  // unprotect
  ReleaseSemaphore(&base->lh_Semaphore);

  #if defined(__amigaos4__) && defined(__NEWLIB__)
  if(NewlibBase)
  {
    DROPINTERFACE(INewlib);
    CloseLibrary(NewlibBase);
    NewlibBase = NULL;
  }
  #endif

  // make sure the system deletes the library as well.
  rc = base->lh_Segment;
  DeleteLibrary(&base->lh_Library);

  return rc;
}

#if defined(__amigaos4__)
STATIC BPTR LibExpunge(struct LibraryManagerInterface *Self)
{
  struct LibraryHeader *base = (struct LibraryHeader *)Self->Data.LibBase;
#elif defined(__MORPHOS__)
STATIC BPTR LibExpunge(void)
{
  struct LibraryHeader *base = (struct LibraryHeader *)REG_A6;
#elif defined(__AROS__)
AROS_LH1 (BPTR, LibExpunge,
    AROS_LHA(UNUSED struct LibraryHeader *, extralh, D0),
    struct LibraryHeader *, base, 3, __MCC_
)
{
    AROS_LIBFUNC_INIT
#else
STATIC BPTR LIBFUNC LibExpunge(REG(a6, struct LibraryHeader *base))
{
#endif
  BPTR rc;

  D(DBF_STARTUP, "LibExpunge(" CLASS "): %ld", base->lh_Library.lib_OpenCnt);

  // in case our open counter is still > 0, we have
  // to set the late expunge flag and return immediately
  if(base->lh_Library.lib_OpenCnt > 0)
  {
    base->lh_Library.lib_Flags |= LIBF_DELEXP;
    rc = 0;
  }
  else
  {
    rc = LibDelete(base);
  }

  return(rc);
#ifdef __AROS__
  AROS_LIBFUNC_EXIT
#endif
}

/*****************************************************************************************************/
/*****************************************************************************************************/

#if defined(__amigaos4__)
STATIC struct LibraryHeader *LibOpen(struct LibraryManagerInterface *Self, ULONG version UNUSED)
{
  struct LibraryHeader *base = (struct LibraryHeader *)Self->Data.LibBase;
#elif defined(__MORPHOS__)
STATIC struct LibraryHeader *LibOpen(void)
{
  struct LibraryHeader *base = (struct LibraryHeader *)REG_A6;
#elif defined(__AROS__)
AROS_LH1 (struct LibraryHeader *, LibOpen,
    AROS_LHA (UNUSED ULONG, version, D0),
    struct LibraryHeader *, base, 1, __MCC_
)
{
  AROS_LIBFUNC_INIT
#else
STATIC struct LibraryHeader * LIBFUNC LibOpen(REG(d0, UNUSED ULONG version), REG(a6, struct LibraryHeader *base))
{
#endif
  struct LibraryHeader *res = NULL;

  D(DBF_STARTUP, "LibOpen(" CLASS "): %ld", base->lh_Library.lib_OpenCnt);

  // LibOpen(), LibClose() and LibExpunge() are called while the system is in
  // Forbid() state. That means that these functions should be quick and should
  // not break this Forbid()!! Therefore the open counter should be increased
  // as the very first instruction during LibOpen(), because a ClassOpen()
  // which breaks a Forbid() and another task calling LibExpunge() will cause
  // to expunge this library while it is not yet fully initialized. A crash
  // is unavoidable then. Even the semaphore does not guarantee 100% protection
  // against such a race condition, because waiting for the semaphore to be
  // obtained will effectively break the Forbid()!

  // increase the open counter ahead of anything else
  base->lh_Library.lib_OpenCnt++;

  // delete the late expunge flag
  base->lh_Library.lib_Flags &= ~LIBF_DELEXP;

  // check if the user defined a ClassOpen() function
  #if defined(CLASSOPEN)
  {
    struct ExecIFace *IExec = (struct ExecIFace *)(*(struct ExecBase **)4)->MainInterface;
    BOOL success = FALSE;

    // protect
    ObtainSemaphore(&base->lh_Semaphore);

    // make sure we have enough stack here
    success = callMccFunction(mccLibOpen, base);

    // here we call the user-specific function for LibOpen() where
    // he can do whatever he wants because of the semaphore protection.
    if(success == TRUE)
      res = base;
    else
    {
      E(DBF_STARTUP, "ClassOpen(" CLASS ") failed");

      // decrease the open counter again
      base->lh_Library.lib_OpenCnt--;
    }

    // release the semaphore
    ReleaseSemaphore(&base->lh_Semaphore);
  }
  #else
    res = base;
  #endif

  return res;
#ifdef __AROS__
  AROS_LIBFUNC_EXIT
#endif
}

/*****************************************************************************************************/
/*****************************************************************************************************/

#if defined(__amigaos4__)
STATIC BPTR LibClose(struct LibraryManagerInterface *Self)
{
  struct LibraryHeader *base = (struct LibraryHeader *)Self->Data.LibBase;
#elif defined(__MORPHOS__)
STATIC BPTR LibClose(void)
{
  struct LibraryHeader *base = (struct LibraryHeader *)REG_A6;
#elif defined(__AROS__)
AROS_LH0 (BPTR, LibClose,
    struct LibraryHeader *, base, 2, __MCC_
)
{
    AROS_LIBFUNC_INIT
#else
STATIC BPTR LIBFUNC LibClose(REG(a6, struct LibraryHeader *base))
{
#endif
  BPTR rc = 0;

  D(DBF_STARTUP, "LibClose(" CLASS "): %ld", base->lh_Library.lib_OpenCnt);

  // check if the user defined a ClassClose() function
  #if defined(CLASSCLOSE)
  {
    struct ExecIFace *IExec = (struct ExecIFace *)(*(struct ExecBase **)4)->MainInterface;

    // protect
    ObtainSemaphore(&base->lh_Semaphore);

    // make sure we have enough stack here
    success = callMccFunction(mccLibClose, base);

    // release the semaphore
    ReleaseSemaphore(&base->lh_Semaphore);
  }
  #endif

  // decrease the open counter
  base->lh_Library.lib_OpenCnt--;

  // in case the opern counter is <= 0 we can
  // make sure that we free everything
  if(base->lh_Library.lib_OpenCnt <= 0)
  {
    // in case the late expunge flag is set we go and
    // expunge the library base right now
    if(base->lh_Library.lib_Flags & LIBF_DELEXP)
    {
      rc = LibDelete(base);
    }
  }

  return rc;
#ifdef __AROS__
  AROS_LIBFUNC_EXIT
#endif
}

/*****************************************************************************************************/
/*****************************************************************************************************/

#if defined(__amigaos4__)
STATIC IPTR LIBFUNC MCC_Query(UNUSED struct Interface *self, REG(d0, LONG which))
{
#elif defined(__MORPHOS__)
STATIC IPTR MCC_Query(void)
{
  LONG which = (LONG)REG_D0;
#elif defined(__AROS__)
AROS_LH1(IPTR, MCC_Query,
         AROS_LHA(LONG, which, D0),
         UNUSED struct LibraryHeader *, base, 5, __MCC_
)
{
    AROS_LIBFUNC_INIT
#else
STATIC IPTR LIBFUNC MCC_Query(REG(d0, LONG which))
{
#endif

  D(DBF_STARTUP, "MCC_Query(" CLASS "): %ld", which);

  switch (which)
  {
    #ifdef SUPERCLASS
    case 0: return((IPTR)ThisClass);
    #endif

    #ifdef SUPERCLASSP
    case 1: return((IPTR)ThisClassP);
    #endif

    #ifdef PREFSIMAGEOBJECT
    case 2:
    {
      Object *obj = PREFSIMAGEOBJECT;
      return((IPTR)obj);
    }
    #endif

    #ifdef ONLYGLOBAL
    case 3:
    {
      return(TRUE);
    }
    #endif

    #ifdef INFOCLASS
    case 4:
    {
      return(TRUE);
    }
    #endif

    #ifdef USEDCLASSES
    case 5:
    {
      return((IPTR)USEDCLASSES);
    }
    #endif

    #ifdef USEDCLASSESP
    case 6:
    {
      return((IPTR)USEDCLASSESP);
    }
    #endif

    #ifdef SHORTHELP
    case 7:
    {
      return((IPTR)SHORTHELP);
    }
    #endif
  }

  return(0);
#ifdef __AROS__
  AROS_LIBFUNC_EXIT
#endif
}

#ifdef __cplusplus
}
#endif
