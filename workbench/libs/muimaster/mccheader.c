/*****************************************************************************

This code serves as a basis for writing a library-based MUI custom class
(xyz.mcc) and its preferences editor (xyz.mcp).

You need to define a few things in your main source file, then include
this file and then continue with your classes methods. The order is
important, mccheader.c must be the first code-generating module.

Things to be defined before mccheader.c is included:

(1)  UserLibID     - version string for your class. must follow normal $VER: string conventions.
(2)  VERSION       - version number of the class. must match the one given in the $VER: string.
(3)  REVISION      - revision number of the class. must match the one given in the $VER: string.
(4)  CLASS         - Name of your class, ".mcc" or ".mcp" must always be appended.

(5)  SUPERCLASS    - superclass of your class.
(6)  struct Data   - instance data structure.
(7)  _Dispatcher   - your dispatcher function.

(8)  SUPERCLASSP   - Superclass of the preferences class, must be MUIC_Mccprefs or a subclass.
(9)  struct DataP  - instance data structure of preferences class.
(10) _DispatcherP  - dispatcher for the preferences class.

(11) USEDCLASSES   - Other custom classes required by this class (NULL terminated string array)
(12) USEDCLASSESP  - Preferences classes (.mcp) required by this class (NULL terminated string array)
(13) SHORTHELP     - .mcp help text for prefs program's listview.

Items (1) to (4) must always be defined. If you create a stand-alone
custom class (*.mcc) without preferences editor, also define (5), (6)
and (7). Name your class and the resulting ouptut file "Myclass.mcc".

If you create a preferences class (*.mcp) for a custom class, define
(8), (9) and (10) instead of (5), (6) and (7). Name your class and the
resulting output file "Myclass.mcp".

If you create a custom class with included preferences editor, define
all the above. Note that in this case, the name of your class and the
resulting output file is always "Myclass.mcc". MUI will automatically
recognize that there's also a "Myclass.mcp" included. Having a builtin
preferences class reduces the need for a second file but increases the
size and memory consuption of the class.

(11) If your class requires other mcc custom classes, list them in the
static array USEDCLASSES like this: 
const STRPTR USEDCLASSES[] = { "Busy.mcc", "Listtree.mcc", NULL };

(12) If your class has one (or more) preferences classes, list them in
the array USEDCLASSESP like this:
const STRPTR USEDCLASSESP[] = { "Myclass.mcp", "Popxxx.mcp", NULL };

(13) If you want MUI to display additional help text (besides name, 
version and copyright) when the mouse pointer is over your mcp entry 
in the prefs listview:
#define SHORTHELP "ANSI display for terminal programs."

If your class needs custom initialization (e.g. opening other
libraries), you can define
	ClassInit
	ClassExit
to point to custom functions. These functions need to have the prototypes
	BOOL ClassInitFunc(struct Library *base);
	VOID ClassExitFunc(struct Library *base);
and will be called right after the class has been created and right
before the class is being deleted. If your init func returns FALSE,
the custom class will be unloaded immediately.

Define the minimum version of muimaster.libray in MASTERVERSION. If you
don't define MASTERVERSION, it will default to MUIMASTER_VMIN from the
mui.h include file.

---
Items (1) to (4) must always be defined. If you create a stand-alone
custom class (*.mcc) without preferences editor, also define (5), (6)
and (7). Name your class and the resulting ouptut file "Myclass.mcc".

If you create a preferences class (*.mcp) for a custom class, define
(8), (9) and (10) instead of (5), (6) and (7). Name your class and the
resulting output file "Myclass.mcp".

If you create a custom class with included preferences editor, define
all the above. Note that in this case, the name of your class and the
resulting output file is always "Myclass.mcc". MUI will automatically
recognize that there's also a "Myclass.mcp" included. Having a builtin
preferences class reduces the need for a second file but increases the
size and memory consuption of the class.

If your class needs custom initialization (e.g. opening other
libraries), you can define
	PreClassInit
	PostClassExit
	ClassInit
	ClassExit
to point to custom functions. These functions need to have the prototypes
	BOOL ClassInitFunc(struct Library *base);
	VOID ClassExitFunc(struct Library *base);
and will be called right after the class has been created and right
before the class is being deleted. If your init func returns FALSE,
the custom class will be unloaded immediately.

	BOOL PreClassInitFunc(void);
	VOID PostClassExitFunc(void);

These functions will be called BEFORE the class is created and AFTER the
class is deleted, if something depends on it for example. MUIMasterBase
is open then.

Define the minimum version of muimaster.libray in MASTERVERSION. If you
don't define MASTERVERSION, it will default to MUIMASTER_VMIN from the
mui.h include file.

This code automatically defines and initializes the following variables:
	struct Library *MUIMasterBase;
	struct Library *SysBase;
	struct Library *UtilityBase;
	struct DosLibrary *DOSBase;
	struct GfxBase *GfxBase;
	struct IntuitionBase *IntuitionBase;
	struct Library *MUIClassBase;       // your classes library base
	struct MUI_CustomClass *ThisClass;  // your custom class
	struct MUI_CustomClass *ThisClassP; // your preferences class

Example: Myclass.c
	#define CLASS      MUIC_Myclass // name of class, e.g. "Myclass.mcc"
	#define SUPERCLASS MUIC_Area    // name of superclass
	struct Data
	{
		LONG          MyData;
		struct Foobar MyData2;
		// ...
	};
	#define UserLibID "$VER: Myclass.mcc 17.53 (11.11.96)"
	#define VERSION   17
	#define REVISION  53
	#include "mccheader.c"
	ULONG ASM SAVEDS _Dispatcher(REG(a0) struct IClass *cl GNUCREG(a0),
	                             REG(a2) Object *obj GNUCREG(a2),
	                             REG(a1) Msg msg GNUCREG(a1) )
	{
		// ...
	}

Compiling and linking with SAS-C can look like this:
	Myclass.mcc: Myclass.c
		sc $(CFLAGS) $*.c OBJNAME $*.o
		slink to $@ from $*.o lib $(LINKERLIBS) $(LINKERFLAGS)

Note well that we don't use SAS library creation feature here, it simply
sucks too much. It's not much more complicated to do the library
initialziation ourselves and we have better control over everything.

Make sure to read the whole source to get some interesting comments
and some understanding on how libraries are created!

*****************************************************************************/

/* a few other includes... */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/muimaster.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/debug.h>

/* The name of the class will also become the name of the library. */
/* We need a pointer to this string in our ROMTag (see below). */

static const char UserLibName[] = CLASS;


/* Here's our global data, described above. */

struct Library *MUIClassBase;
struct Library *MUIMasterBase;
struct ExecBase *SysBase;
struct UtilityBase *UtilityBase;
struct DosLibrary *DOSBase;
struct GfxBase *GfxBase;
struct IntuitionBase *IntuitionBase;

#ifdef SUPERCLASS
struct MUI_CustomClass *ThisClass;
#endif

#ifdef SUPERCLASSP
struct MUI_CustomClass *ThisClassP;
#endif

/* Our library structure, consisting of a struct Library, a segment pointer */
/* and a semaphore. We need the semaphore to protect init/exit stuff in our */
/* open/close functions */

struct LibraryHeader
{
	struct Library         lh_Library;
	BPTR                   lh_Segment;
	struct SignalSemaphore lh_Semaphore;
	struct StackSwapStruct *lh_Stack;
};


/* Prototypes for all library functions */

AROS_UFP3(struct LibraryHeader *, LibInit,
    AROS_UFPA(struct Library *, lib, D0),
    AROS_UFPA(BPTR, segment, A0),
    AROS_UFPA(struct ExecBase *, sysbase, A6));

AROS_LD0(BPTR, LibExpunge,
    struct LibraryHeader *, base, 3, MCC);

AROS_LD1(struct LibraryHeader *, LibOpen,
    AROS_LDA(ULONG, version, D0),
    struct LibraryHeader *, base, 1, MCC);

AROS_LD0(BPTR, LibClose,
    struct LibraryHeader *, base, 2, MCC);

AROS_LD0(LONG, LibNull,
    struct LibraryHeader *, base, 4, MCC);
    
AROS_LD1(IPTR, MCC_Query,
    AROS_LDA(LONG, which, D0),
    struct LibraryHeader *, base, 5, MCC);
    
/* Prototypes for a few sub-functions */

BOOL UserLibInit   (struct Library *base);
BOOL UserLibExpunge(struct Library *base);
BOOL UserLibOpen   (struct Library *base);
BOOL UserLibClose  (struct Library *base);


/* This is the librarie's jump table */

static const APTR LibVectors[] =
{
    AROS_SLIB_ENTRY(LibOpen, MCC),
    AROS_SLIB_ENTRY(LibClose, MCC),
    AROS_SLIB_ENTRY(LibExpunge, MCC),
    AROS_SLIB_ENTRY(LibNull, MCC),
    AROS_SLIB_ENTRY(MCC_Query, MCC),
    (APTR)-1
};


/****************************************************************************************/

/* Dummy entry point and LibNull() function all in one */

/****************************************************************************************/

AROS_LH0(LONG, LibNull,
    struct LibraryHeader *, base, 4, MCC)
{
    AROS_LIBFUNC_INIT
    
    return NULL;
    
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

/* Here's what makes us a library. */

/****************************************************************************************/

static const struct Resident ROMTag =
{
	RTC_MATCHWORD,
	&ROMTag,
	&ROMTag + 1,
	0,
	VERSION,
	NT_LIBRARY,
	0,
	UserLibName,
	UserLibID,
	LibInit
};

/****************************************************************************************/

/******************************************************************************/
/* Standard Library Functions, all of them are called in Forbid() state.      */
/******************************************************************************/

/****************************************************************************************/

AROS_UFH3(struct LibraryHeader *, LibInit,
    AROS_UFHA(struct Library *, lib, D0),
    AROS_UFHA(BPTR, segment, A0),
    AROS_UFHA(struct ExecBase *, sysbase, A6))

{
    AROS_LIBFUNC_INIT
    
    struct LibraryHeader *base;

    SysBase = sysbase;

    D(bug( "Start...\n" ) );
    if ((base = (struct LibraryHeader *)MakeLibrary((APTR)LibVectors,NULL,NULL,sizeof(struct LibraryHeader),NULL)))
    {
        D(bug( "After MakeLibrary()\n" ) );

        base->lh_Library.lib_Node.ln_Type = NT_LIBRARY;
        base->lh_Library.lib_Node.ln_Name = (char *)UserLibName;
        base->lh_Library.lib_Flags        = LIBF_CHANGED | LIBF_SUMUSED;
        base->lh_Library.lib_Version      = VERSION;
        base->lh_Library.lib_Revision     = REVISION;
        base->lh_Library.lib_IdString     = (char *)UserLibID;

        base->lh_Segment    = segment;

        InitSemaphore(&base->lh_Semaphore);

        D(bug( "AddLibrary()\n") );
        AddLibrary((struct Library *)base);
    }
    else
    {
        D(bug("\7MakeLibrary() failed\n") );
    }

    return(base);
    
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0(BPTR, LibExpunge,
    struct LibraryHeader *, base, 3, MCC)
{
    AROS_LIBFUNC_INIT
    
    BPTR rc;

    D(bug( "OpenCount = %ld\n", base->lh_Library.lib_OpenCnt ) );

    if (base->lh_Library.lib_OpenCnt)
    {
        base->lh_Library.lib_Flags |= LIBF_DELEXP;
        D(bug("Setting LIBF_DELEXP\n") );
        return(NULL);
    }

    Remove((struct Node *)base);

    rc = base->lh_Segment;
    FreeMem((BYTE *)base - base->lh_Library.lib_NegSize,base->lh_Library.lib_NegSize + base->lh_Library.lib_PosSize);

    return(rc);
    
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH1(struct LibraryHeader *, LibOpen,
    AROS_LHA(ULONG, version, D0),
    struct LibraryHeader *, base, 1, MCC)
{
    AROS_LIBFUNC_INIT
    
    struct LibraryHeader *rc;

    ObtainSemaphore(&base->lh_Semaphore);

    base->lh_Library.lib_OpenCnt++;
    base->lh_Library.lib_Flags &= ~LIBF_DELEXP;

    D(bug( "OpenCount = %ld\n", base->lh_Library.lib_OpenCnt ) );

    if (UserLibOpen((struct Library *)base))
    {
        #ifdef CLASS_VERSIONFAKE
        base->lh_Library.lib_Version    = (UWORD)MUIMasterBase->lib_Version;
        base->lh_Library.lib_Revision    = (UWORD)MUIMasterBase->lib_Revision;
        #endif

        rc = base;
    }
    else
    {
        rc = NULL;
        base->lh_Library.lib_OpenCnt--;
        D(bug("\7UserLibOpen() failed\n") );
    }

    ReleaseSemaphore(&base->lh_Semaphore);

    return(rc);
    
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0(BPTR, LibClose,
    struct LibraryHeader *, base, 2, MCC)
{
    AROS_LIBFUNC_INIT
    
    BPTR rc = NULL;

    ObtainSemaphore(&base->lh_Semaphore);

    D(bug( "OpenCount = %ld %s\n", base->lh_Library.lib_OpenCnt, base->lh_Library.lib_OpenCnt == 0 ? "\7ERROR" : "" ) );

    UserLibClose((struct Library *)base);

    if (--base->lh_Library.lib_OpenCnt == 0)
    {

#ifndef EXPUNGE_AT_LAST_CLOSE
        if (base->lh_Library.lib_Flags & LIBF_DELEXP)
#endif

        rc = AROS_LC0(BPTR, expunge, struct LibraryHeader *, base, 3, MCC);
    }

    ReleaseSemaphore(&base->lh_Semaphore);

    return(rc);
    
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

BOOL UserLibOpen(struct Library *base)
{
    #ifdef SUPERCLASS
    
    static AROS_UFP3(IPTR, _Dispatcher,
        AROS_UFPA(struct IClass *,m cl, A0),
        AROS_UFPA(Object *, obj, A2),
        AROS_UFPA(Msg, msg, A1));

    #endif

    #ifdef SUPERCLASSP

    static AROS_UFP3(IPTR, _DispatcherP,
        AROS_UFPA(struct IClass *,m cl, A0),
        AROS_UFPA(Object *, obj, A2),
        AROS_UFPA(Msg, msg, A1));
        
    #endif

    BOOL PreClassInitFunc(void);
    BOOL ClassInitFunc(struct Library *base);

    D(bug( "OpenCount = %ld\n", base->lib_OpenCnt ) );

    if (base->lib_OpenCnt!=1)
        return(TRUE);

    #ifndef MASTERVERSION
    #define MASTERVERSION MUIMASTER_VMIN
    #endif

    if ((MUIMasterBase = OpenLibrary("muimaster.library",MASTERVERSION)))
    {
        #ifdef PreClassInit
        PreClassInitFunc();
        #endif

        #ifdef SUPERCLASS

        ThisClass = MUI_CreateCustomClass(base,SUPERCLASS,NULL,sizeof(struct Data),AROS_ASMSYMNAME(_Dispatcher));

        if ( ThisClass )
        #endif
        {
            #ifdef SUPERCLASSP
            if (ThisClassP = MUI_CreateCustomClass(base,SUPERCLASSP,NULL,sizeof(struct DataP),AROS_ASMSYMNAME(_DispatcherP)))
            #endif
            {
                #ifdef SUPERCLASS
                #define THISCLASS ThisClass
                #else
                #define THISCLASS ThisClassP
                #endif

                UtilityBase   = (struct UtilityBase *)THISCLASS->mcc_UtilityBase;
                DOSBase       = (struct DosLibrary *)THISCLASS->mcc_DOSBase;
                GfxBase       = (struct GfxBase *)THISCLASS->mcc_GfxBase;
                IntuitionBase = (struct IntuitionBase *)THISCLASS->mcc_IntuitionBase;

                #ifndef ClassInit
                return(TRUE);
                #else
                if (ClassInitFunc(base))
                {
                    return(TRUE);
                }

                #ifdef SUPERCLASSP
                MUI_DeleteCustomClass(ThisClassP);
                ThisClassP = NULL;
                #endif

                #endif
            }
            #ifdef SUPERCLASSP
            #ifdef SUPERCLASS
            MUI_DeleteCustomClass(ThisClass);
            ThisClass = NULL;
            #endif
            #endif
        }
        CloseLibrary(MUIMasterBase);
        MUIMasterBase = NULL;
    }

    D(bug("fail.: %08lx %s\n",base,base->lib_Node.ln_Name) );

    return(FALSE);
}

/****************************************************************************************/

BOOL UserLibClose(struct Library *base)
{
    VOID PostClassExitFunc(void);
    VOID ClassExitFunc(struct Library *base);

    D(bug( "OpenCount = %ld\n", base->lib_OpenCnt ) );

    if (base->lib_OpenCnt==1)
    {
        #ifdef ClassExit
        ClassExitFunc(base);
        #endif

        #ifdef SUPERCLASSP
        if (ThisClassP)
        {
            MUI_DeleteCustomClass(ThisClassP);
            ThisClassP = NULL;
        }
        #endif

        #ifdef SUPERCLASS
        if (ThisClass)
        {
            MUI_DeleteCustomClass(ThisClass);
            ThisClass = NULL;
        }
        #endif

        #ifdef PostClassExit
        PostClassExitFunc();
        #endif

        if (MUIMasterBase)
        {
            CloseLibrary(MUIMasterBase);
            MUIMasterBase = NULL;
        }
    }

    return(TRUE);
}

/****************************************************************************************/

AROS_LH1(IPTR, MCC_Query,
    AROS_LHA(LONG, which, D0),
    struct LibraryHeader *, base, 5, MCC)
{
    AROS_LIBFUNC_INIT
    
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
    return(NULL);
	
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/
