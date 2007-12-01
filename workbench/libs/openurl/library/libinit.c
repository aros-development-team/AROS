/*
**  openurl.library - universal URL display and browser
**  launcher library
**
**  Written by Troels Walsted Hansen <troels@thule.no>
**  Placed in the public domain.
**
**  Developed by:
**  - Alfonso Ranieri <alforan@tin.it>
**  - Stefan Kost <ensonic@sonicpulse.de>
**
**  Ported to OS4 by Alexandre Balaban <alexandre@balaban.name>
*/


#include "lib.h"
#include "openurl.library_rev.h"

/****************************************************************************/

UBYTE                  lib_name[] = PRG;
UBYTE                  lib_ver[] = VSTRING;
UBYTE                  lib_fullName[] = PRGNAME;
ULONG                  lib_version = VERSION;
ULONG                  lib_revision = REVISION;

struct ExecBase        *SysBase = NULL;
struct DosLibrary      *DOSBase = NULL;
struct Library         *UtilityBase = NULL;
struct Library         *IFFParseBase = NULL;
struct RxsLib          *RexxSysBase = NULL;

struct SignalSemaphore lib_sem = {0};
struct SignalSemaphore lib_prefsSem = {0};
struct SignalSemaphore lib_memSem = {0};

APTR                   lib_pool = NULL;
struct URL_Prefs       *lib_prefs = NULL;

struct Library         *lib_base = NULL;
ULONG                  lib_segList = (ULONG)NULL;
ULONG                  lib_use = 0;
ULONG                  lib_flags = 0;

/****************************************************************************/

#ifdef __MORPHOS__
#include <exec/resident.h>

static struct Library *initLib ( struct Library *base , BPTR segList , struct ExecBase *sys );
static struct Library *openLib ( void );
static ULONG expungeLib ( void );
static ULONG closeLib ( void );
static ULONG nil (void );
static ULONG first(void) __attribute((unused));

/****************************************************************************/

static ULONG
first(void)
{
    return -1;
}

/****************************************************************************/

static const APTR funcTable[] =
{
    (APTR)FUNCARRAY_BEGIN,
    (APTR)FUNCARRAY_32BIT_NATIVE,
    (APTR)openLib,
    (APTR)closeLib,
    (APTR)expungeLib,
    (APTR)nil,

    (APTR)LIB_URL_OpenA,
    (APTR)LIB_URL_OldGetPrefs,
    (APTR)LIB_URL_OldFreePrefs,
    (APTR)LIB_URL_OldSetPrefs,
    (APTR)LIB_URL_OldGetDefaultPrefs,
    (APTR)LIB_URL_OldLaunchPrefsApp,
    (APTR)0xffffffff,

    (APTR)FUNCARRAY_32BIT_D0D1A0A1SR_NATIVE,
    (APTR)dispatch,
    (APTR)0xffffffff,

    (APTR)FUNCARRAY_32BIT_NATIVE,
    (APTR)LIB_URL_GetPrefsA,
    (APTR)LIB_URL_FreePrefsA,
    (APTR)LIB_URL_SetPrefsA,
    (APTR)LIB_URL_LaunchPrefsAppA,
    (APTR)LIB_URL_GetAttr,
    (APTR)0xffffffff,

    (APTR)FUNCARRAY_END
};

static const ULONG initTable[] =
{
    sizeof(struct Library),
    (ULONG)funcTable,
    NULL,
    (ULONG)initLib
};

const struct Resident romTag =
{
    RTC_MATCHWORD,
    (struct Resident *)&romTag,
    (struct Resident *)&romTag+1,
    RTF_AUTOINIT|RTF_PPC|RTF_EXTENDED,
    VERSION,
    NT_LIBRARY,
    0,
    (UBYTE *)lib_name,
    (UBYTE *)lib_ver,
    (APTR)initTable,
    REVISION,
    NULL
};

const ULONG __abox__ = 1;

#elif defined(__amigaos4__)
#include <proto/exec.h>
/* amigaos4 *****************************************************************/

struct ExecIFace       *IExec = NULL;
struct DOSIFace        *IDOS = NULL;
struct UtilityIFace    *IUtility = NULL;
struct IFFParseIFace   *IIFFParse = NULL;
struct RexxSysIFace    *IRexxSys = NULL;

/* amigaos4 *****************************************************************/

struct Library * SAVEDS ASM initLib (REG(a0,ULONG segList), REG(a6,struct ExecBase *sys), REG(d0, struct Library *base));
//uint32 libObtain (struct LibraryManagerInterface *Self);
//uint32 libRelease (struct LibraryManagerInterface *Self);
struct Library * SAVEDS ASM openLib (REG(a6,struct Library *base));
ULONG SAVEDS ASM closeLib(REG(a6,struct Library *base));
ULONG SAVEDS ASM expungeLib (REG(a6,struct Library *base));

struct Library * mgr_Init (struct Library *base, BPTR segList, struct ExecIFace *ISys);
uint32 mgr_Obtain (struct LibraryManagerInterface *Self);
uint32 mgr_Release (struct LibraryManagerInterface *Self);
struct Library * mgr_Open (struct LibraryManagerInterface *Self, uint32 version);
APTR mgr_Close (struct LibraryManagerInterface *Self);
APTR mgr_Expunge (struct LibraryManagerInterface *Self);

uint32 VARARGS68K OS4_URL_Obtain( struct OpenURLIFace *Self );
uint32 VARARGS68K OS4_URL_Release( struct OpenURLIFace *Self );

/* amigaos4 *****************************************************************/

static BOOL bAlreadyHasSemaphore = FALSE;

/* amigaos4 *****************************************************************/

static APTR lib_manager_vectors[] = {
	mgr_Obtain,
	mgr_Release,
	NULL,
	NULL,
	mgr_Open,
	mgr_Close,
	mgr_Expunge,
	NULL,
	(APTR)-1,
};

static struct TagItem lib_managerTags[] = {
	{ MIT_Name,		(uint32)"__library"			},
	{ MIT_VectorTable,	(uint32)lib_manager_vectors	},
	{ MIT_Version,		1						},
	{ TAG_END,		0						}
};

void *main_vectors[] = {
	(void *) OS4_URL_Obtain,
	(void *) OS4_URL_Release,
	(void *) NULL,
	(void *) NULL,
    (void *) OS4_URL_OpenA,
	(void *) OS4_URL_Open,
    (void *) OS4_URL_OldGetPrefs,
    (void *) OS4_URL_OldFreePrefs,
    (void *) OS4_URL_OldSetPrefs,
    (void *) OS4_URL_OldGetDefaultPrefs,
    (void *) OS4_URL_OldLaunchPrefsApp,
    (void *) OS4_dispatch,
    (void *) OS4_URL_GetPrefsA,
    (void *) OS4_URL_GetPrefs,
    (void *) OS4_URL_FreePrefsA,
    (void *) OS4_URL_FreePrefs,
    (void *) OS4_URL_SetPrefsA,
    (void *) OS4_URL_SetPrefs,
    (void *) OS4_URL_LaunchPrefsAppA,
    (void *) OS4_URL_LaunchPrefsApp,
    (void *) OS4_URL_GetAttr,
    (void *) -1
};

static struct TagItem lib_mainTags[] = {
	{ MIT_Name,		    (uint32)"main"			},
	{ MIT_VectorTable,	(uint32)main_vectors	},
	{ MIT_Version,		1						},
	{ TAG_END,		    0						}
};

static APTR libInterfaces[] = {
	lib_managerTags,
	lib_mainTags,
	NULL
};

extern uint32 VecTable68K[];

static struct TagItem libCreateTags[] = {
	{ CLT_DataSize,		(uint32)sizeof(struct Library)	},
	{ CLT_InitFunc,		(uint32)mgr_Init					 },
	{ CLT_Interfaces,	(uint32)libInterfaces			},
    { CLT_Vector68K,    (uint32)VecTable68K			    },
	{ TAG_END,			0								}
};

#ifdef __GNUC__
static struct Resident __attribute__((used)) romTag = {
#else
static struct Resident romTag = {
#endif
	RTC_MATCHWORD,				// rt_MatchWord
	&romTag,					// rt_MatchTag
	&romTag+1,					// rt_EndSkip
	RTF_NATIVE | RTF_AUTOINIT,	// rt_Flags
	VERSION,					// rt_Version
	NT_LIBRARY,					// rt_Type
	0,							// rt_Pri
	lib_name,					// rt_Name
	lib_ver,					// rt_IdString
	libCreateTags				// rt_Init
};

#endif

/****************************************************************************/
/*
#ifdef __MORPHOS__
static struct Library *initLib(struct Library *base,BPTR segList,struct ExecBase *sys)
#elif defined(__amigaos4__)
struct Library * initLib(struct Library *base, BPTR segList, struct ExecIFace *ISys)
#else
struct Library *SAVEDS ASM initLib(REG(a0,ULONG segList),REG(a6,struct ExecBase *sys),REG(d0, struct Library *base))
#endif
{
#if defined(__amigaos4__)
	base->lib_Node.ln_Type = NT_LIBRARY;
	base->lib_Node.ln_Pri = 0;
	base->lib_Node.ln_Name = lib_name;
	base->lib_Flags = LIBF_SUMUSED|LIBF_CHANGED;
	base->lib_Version = lib_version;
	base->lib_Revision = lib_revision;
	base->lib_IdString = lib_ver;

	IExec = ISys;
	//IExec->Obtain();
	SysBase = (struct ExecBase*)ISys->Data.LibBase;
#else
    SysBase = sys;
#endif

    InitSemaphore(&lib_sem);
    InitSemaphore(&lib_prefsSem);
    InitSemaphore(&lib_memSem);

    lib_segList = segList;

    return lib_base = base;
}
*/
#ifdef __MORPHOS__
static struct Library *initLib(struct Library *base,BPTR segList,struct ExecBase *sys)
#else
struct Library * SAVEDS ASM initLib (REG(a0,ULONG segList), REG(a6,struct ExecBase *sys), REG(d0, struct Library *base))
#endif
{
    SysBase     = sys;

    InitSemaphore(&lib_sem);
    InitSemaphore(&lib_prefsSem);
    InitSemaphore(&lib_memSem);

    lib_segList = segList;

    return lib_base = base;
}

/****************************************************************************/

#ifdef __MORPHOS__
static struct Library *openLib(void)
{
    struct Library *base = (struct Library *)REG_A6;
#else
struct Library * SAVEDS ASM openLib(REG(a6,struct Library *base))
{
#endif

    struct Library *res;

    ObtainSemaphore(&lib_sem);

    base->lib_OpenCnt++;
    base->lib_Flags &= ~LIBF_DELEXP;

    if (!(lib_flags & BASEFLG_Init) && !initBase())
    {
        base->lib_OpenCnt--;
        res = NULL;
    }
    else res = base;

    ReleaseSemaphore(&lib_sem);

    return res;
}

/****************************************************************************/

#ifdef __MORPHOS__
static ULONG expungeLib(void)
{
    struct Library *base = (struct Library *)REG_A6;
#else
ULONG SAVEDS ASM expungeLib(REG(a6,struct Library *base))
{
#endif

    ULONG res;

#if defined(__amigaos4__)
	// prevents a deadlock if called from closeLib
	 if( !bAlreadyHasSemaphore )
#endif
    ObtainSemaphore(&lib_sem);

	 if (!base->lib_OpenCnt && !lib_use)
    {
        Remove((struct Node *)base);

#if defined(__amigaos4__)
		  DeleteLibrary(base);
		  //IExec->Release();
#else
        FreeMem((UBYTE *)base-base->lib_NegSize,base->lib_NegSize+base->lib_PosSize);
#endif

        res = lib_segList;
    }
    else
    {
        base->lib_Flags |= LIBF_DELEXP;
        res = (ULONG)NULL;
    }

#if defined(__amigaos4__)
	// prevents a deadlock if called from closeLib
	 if( !bAlreadyHasSemaphore )
#endif
    ReleaseSemaphore(&lib_sem);

    return res;
}

/****************************************************************************/

#ifdef __MORPHOS__
static ULONG closeLib(void)
{
    struct Library *base = (struct Library *)REG_A6;
#else
ULONG SAVEDS ASM closeLib(REG(a6,struct Library *base))
{
#endif

    ULONG res = (ULONG)NULL;

    ObtainSemaphore(&lib_sem);

	 base->lib_OpenCnt--;

	 if (!base->lib_OpenCnt && !lib_use)
    {
        freeBase();

        if (base->lib_Flags & LIBF_DELEXP)
        {
#if defined(__amigaos4__)
			bAlreadyHasSemaphore = TRUE;
			res = (ULONG)expungeLib( base );
			bAlreadyHasSemaphore = FALSE;
#else
            Remove((struct Node *)base);
            FreeMem((UBYTE *)base-base->lib_NegSize,base->lib_NegSize+base->lib_PosSize);

            res = lib_segList;
#endif
        }
    }

    ReleaseSemaphore(&lib_sem);

    return res;
}

/****************************************************************************/

#ifdef __MORPHOS__
static ULONG nil(void)
{
    return 0;
}
#endif

/****************************************************************************/

