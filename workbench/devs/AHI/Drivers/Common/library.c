
/*
 *  Shared library code
 */

#include <config.h>

#include <exec/alerts.h>
#include <exec/libraries.h>
#include <exec/resident.h>
#include <utility/utility.h>

#include <clib/alib_protos.h>
#include <proto/ahi_sub.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "gatestubs.h"
#include "library.h"

#include "DriverData.h"
#include "version.h"

#ifndef INTUITIONNAME
#define INTUITIONNAME "intuition.library"
#endif

#ifdef __amithlon__
# define RTF_NATIVE (1<<3)
# define FUNCARRAY_32BIT_NATIVE 0xfffefffe
#endif

#if !defined( __AROS__ ) && !defined( __amithlon__ )
extern void _etext;
#else
# define _etext RomTag+1 // Fake it
#endif

/******************************************************************************
** Function prototypes ********************************************************
******************************************************************************/

struct DriverBase*
_LibInit( struct DriverBase* AHIsubBase,
	  BPTR               seglist,
	  struct ExecBase*   sysbase );

BPTR
_LibExpunge( struct DriverBase* AHIsubBase );

struct DriverBase*
_LibOpen( ULONG              version,
	  struct DriverBase* AHIsubBase );

BPTR
_LibClose( struct DriverBase* AHIsubBase );

ULONG
_LibNull( struct DriverBase* AHIsubBase );


/******************************************************************************
** Driver entry ***************************************************************
******************************************************************************/

#if defined( __amithlon__ )
__asm( "\n\
         .text;\n\
         .byte 0x4e, 0xfa, 0x00, 0x03\n\
         jmp _start" );
#endif

int
_start( void )
{
  return -1;
}

#if defined( __MORPHOS__ )
ULONG   __abox__=1;
ULONG   __amigappc__=1;  // deprecated, used in MOS 0.4
#endif

/******************************************************************************
** Driver resident structure **************************************************
******************************************************************************/

#if defined( __AMIGAOS4__  )
static const struct TagItem InitTable[];
#else
static const APTR InitTable[4];
#endif

// This structure must reside in the text segment or the read-only
// data segment!  "const" makes it happen.
const struct Resident RomTag __attribute__((used)) =
{
  RTC_MATCHWORD,
  (struct Resident *) &RomTag,
  (struct Resident *) &_etext,
#if defined( __MORPHOS__ ) 
  RTF_EXTENDED | RTF_PPC | RTF_AUTOINIT,
#elif defined( __AROS__ )
  RTF_EXTENDED | RTF_AUTOINIT,
#elif defined( __amithlon__ )
  RTF_NATIVE | RTF_AUTOINIT,
#elif defined( __AMIGAOS4__ )
  RTF_NATIVE | RTF_AUTOINIT,
#else
  RTF_AUTOINIT,
#endif
  VERSION,
  NT_LIBRARY,
  0,                      /* priority */
  (BYTE *) LibName,
  (BYTE *) LibIDString,
  (APTR) &InitTable
#if defined( __MORPHOS__ ) || defined( __AROS__ )
  , REVISION, NULL
#endif
};


/******************************************************************************
** Globals ********************************************************************
******************************************************************************/

const char  LibName[]     = DRIVER;
const char  LibIDString[] = DRIVER " " VERS "\r\n";
const UWORD LibVersion    = VERSION;
const UWORD LibRevision   = REVISION;


/******************************************************************************
** Library stuff **************************************************************
******************************************************************************/

#ifndef __AMIGAOS4__

static const APTR FuncTable[] =
{
#if defined( __MORPHOS__ ) || defined( __amithlon__ )
  (APTR) FUNCARRAY_32BIT_NATIVE,
#endif

  gwLibOpen,
  gwLibClose,
  gwLibExpunge,
  gwLibNull,

  gwAHIsub_AllocAudio,
  gwAHIsub_FreeAudio,
  gwAHIsub_Disable,
  gwAHIsub_Enable,
  gwAHIsub_Start,
  gwAHIsub_Update,
  gwAHIsub_Stop,
  gwAHIsub_SetVol,
  gwAHIsub_SetFreq,
  gwAHIsub_SetSound,
  gwAHIsub_SetEffect,
  gwAHIsub_LoadSound,
  gwAHIsub_UnloadSound,
  gwAHIsub_GetAttr,
  gwAHIsub_HardwareControl,

  (APTR) -1
};


static const APTR InitTable[4] =
{
  (APTR) DRIVERBASE_SIZEOF,
  (APTR) &FuncTable,
  NULL,
#if defined( __MORPHOS__ ) || defined( __amithlon__ )
  (APTR) _LibInit
#else
  (APTR) gwLibInit
#endif
};

#else // __AMIGAOS4__

static ULONG generic_Obtain(struct Interface *Self)
{
  return Self->Data.RefCount++;
}


static ULONG generic_Release(struct Interface *Self)
{
  return Self->Data.RefCount--;
}


static const CONST_APTR LibManagerVectors[] =
{
  generic_Obtain,
  generic_Release,
  NULL,
  NULL,
  
  gwLibOpen,
  gwLibClose,
  gwLibExpunge,
  NULL,

  (CONST_APTR) -1,
};

static const struct TagItem LibManagerTags[] =
{
  { MIT_Name,             (ULONG) "__library"       },
  { MIT_VectorTable,      (ULONG) LibManagerVectors },
  { MIT_Version,          1                         },
  { TAG_DONE,             0                         }
};


static const CONST_APTR MainVectors[] = {
  generic_Obtain,
  generic_Release,
  NULL,
  NULL,
  
  gwAHIsub_AllocAudio,
  gwAHIsub_FreeAudio,
  gwAHIsub_Disable,
  gwAHIsub_Enable,
  gwAHIsub_Start,
  gwAHIsub_Update,
  gwAHIsub_Stop,
  gwAHIsub_SetVol,
  gwAHIsub_SetFreq,
  gwAHIsub_SetSound,
  gwAHIsub_SetEffect,
  gwAHIsub_LoadSound,
  gwAHIsub_UnloadSound,
  gwAHIsub_GetAttr,
  gwAHIsub_HardwareControl,

  (CONST_APTR) -1
};

static const struct TagItem MainTags[] =
{
  { MIT_Name,              (ULONG) "main"      },
  { MIT_VectorTable,       (ULONG) MainVectors },
  { MIT_Version,           1                   },
  { TAG_DONE,              0                   }
};


/* MLT_INTERFACES array */
static const CONST_APTR Interfaces[] =
{
  LibManagerTags,
  MainTags,
  NULL
};

/* m68k library vectors */
static const CONST_APTR VecTable68K[] = {
  (CONST_APTR) &m68kgwLibOpen,
  (CONST_APTR) &m68kgwLibClose,
  (CONST_APTR) &m68kgwLibExpunge,
  (CONST_APTR) &m68kgwLibNull,

  (CONST_APTR) &m68kgwAHIsub_AllocAudio,
  (CONST_APTR) &m68kgwAHIsub_FreeAudio,
  (CONST_APTR) &m68kgwAHIsub_Disable,
  (CONST_APTR) &m68kgwAHIsub_Enable,
  (CONST_APTR) &m68kgwAHIsub_Start,
  (CONST_APTR) &m68kgwAHIsub_Update,
  (CONST_APTR) &m68kgwAHIsub_Stop,
  (CONST_APTR) &m68kgwAHIsub_SetVol,
  (CONST_APTR) &m68kgwAHIsub_SetFreq,
  (CONST_APTR) &m68kgwAHIsub_SetSound,
  (CONST_APTR) &m68kgwAHIsub_SetEffect,
  (CONST_APTR) &m68kgwAHIsub_LoadSound,
  (CONST_APTR) &m68kgwAHIsub_UnloadSound,
  (CONST_APTR) &m68kgwAHIsub_GetAttr,
  (CONST_APTR) &m68kgwAHIsub_HardwareControl,

  (CONST_APTR) -1
};

/* CreateLibrary() tag list */
static const struct TagItem InitTable[] =
{
  { CLT_DataSize,         DRIVERBASE_SIZEOF   },
  { CLT_Interfaces,       (ULONG) Interfaces  },
  { CLT_Vector68K,        (ULONG) VecTable68K },
  { CLT_InitFunc,         (ULONG) gwLibInit   },
  { TAG_DONE,             0                   }
};

#endif // __AMIGAOS4__


/******************************************************************************
** Message requester **********************************************************
******************************************************************************/

void
ReqA( const char*        text,
      APTR               args,
      struct DriverBase* AHIsubBase )
{
  struct EasyStruct es = 
  {
    sizeof (struct EasyStruct),
    0,
    (STRPTR) LibName,
    (STRPTR) text,
    "OK"
#ifdef __AMIGAOS4__
    , NULL, NULL
#endif
  };

  EasyRequestArgs( NULL, &es, NULL, args );
}

/******************************************************************************
** Serial port debugging ******************************************************
******************************************************************************/

#ifndef __AMIGAOS4__

#if defined( __AROS__ ) && !defined( __mc68000__ )

#include <aros/asmcall.h>

AROS_UFH2( void,
	   rawputchar_m68k,
	   AROS_UFHA( UBYTE,            c,       D0 ),
	   AROS_UFHA( struct ExecBase*, sysbase, A3 ) )
{
  AROS_USERFUNC_INIT
  
  AROS_LC1NR(void, RawPutChar, AROS_LCA(UBYTE, c, D0), struct ExecBase *, sysbase, 86, Exec);

  AROS_USERFUNC_EXIT  
}

#else

static const UWORD rawputchar_m68k[] = 
{
  0x2C4B,             // MOVEA.L A3,A6
  0x4EAE, 0xFDFC,     // JSR     -$0204(A6)
  0x4E75              // RTS
};

#endif

void
MyKPrintFArgs( UBYTE*           fmt, 
	       ULONG*           args,
	       struct DriverBase* AHIsubBase )
{
  RawDoFmt( fmt, args, (void(*)(void)) rawputchar_m68k, SysBase );
}

#endif

/******************************************************************************
** HookEntry ******************************************************************
******************************************************************************/

#if defined( __MORPHOS__ )

/* Should be in libamiga, but isn't? */

static ULONG
gw_HookEntry( void )
{
  struct Hook* h   = (struct Hook*) REG_A0;
  void*        o   = (void*)        REG_A2; 
  void*        msg = (void*)        REG_A1;

  return ( ( (ULONG(*)(struct Hook*, void*, void*)) *h->h_SubEntry)( h, o, msg ) );
}

struct EmulLibEntry _HookEntry =
{
  TRAP_LIB, 0, (void (*)(void)) &gw_HookEntry
};

__asm( ".globl HookEntry;HookEntry=_HookEntry" );

#endif

/******************************************************************************
** Library init ***************************************************************
******************************************************************************/

#ifdef __AROS__
#include <aros/symbolsets.h>

THIS_PROGRAM_HANDLES_SYMBOLSET(INIT)
THIS_PROGRAM_HANDLES_SYMBOLSET(EXIT)
DEFINESET(INIT)
DEFINESET(EXIT)
#endif

struct DriverBase*
_LibInit( struct DriverBase* AHIsubBase,
	  BPTR               seglist,
	  struct ExecBase*   sysbase )
{
  SysBase = sysbase;

#ifdef __AROS__
  if (!set_call_funcs(SETNAME(INIT), 1, 1))
      return NULL;
#endif

#ifdef __AMIGAOS4__
  IExec = (struct ExecIFace*) SysBase->MainInterface; 
#endif
  
  AHIsubBase->library.lib_Node.ln_Type = NT_LIBRARY;
  AHIsubBase->library.lib_Node.ln_Name = (STRPTR) LibName;
  AHIsubBase->library.lib_Flags        = LIBF_SUMUSED | LIBF_CHANGED;
  AHIsubBase->library.lib_Version      = VERSION;
  AHIsubBase->library.lib_Revision     = REVISION;
  AHIsubBase->library.lib_IdString     = (STRPTR) LibIDString;
  AHIsubBase->seglist                  = seglist;

  AHIsubBase->intuitionbase = OpenLibrary( INTUITIONNAME, 37 );
  AHIsubBase->utilitybase   = OpenLibrary( UTILITYNAME, 37 );

  if( IntuitionBase == NULL )
  {
    Alert( AN_Unknown|AG_OpenLib|AO_Intuition );
    goto error;
  }
  
#ifdef __AMIGAOS4__
  if ((IIntuition = (struct IntuitionIFace *) GetInterface((struct Library*) IntuitionBase, "main", 1, NULL)) == NULL)
  {
    Alert( AN_Unknown|AG_OpenLib|AO_Intuition );
    goto error;
  }
#endif

  if( UtilityBase == NULL )
  {
    Req( "Unable to open 'utility.library' version 37.\n" );
    goto error;
  }

#ifdef __AMIGAOS4__
  if ((IUtility = (struct UtilityIFace *) GetInterface((struct Library*) UtilityBase, "main", 1, NULL)) == NULL)
  {
    Req("Couldn't open IUtility interface!\n");
    goto error;
  }

  if ((IAHIsub = (struct AHIsubIFace *) GetInterface((struct Library *) AHIsubBase, "main", 1, NULL)) == NULL)
  {
    Req("Couldn't open IAHIsub interface!\n");
    return FALSE;
  }
#endif
  
  if( ! DriverInit( AHIsubBase ) )
  {
    goto error;
  }

  return AHIsubBase;

error:
  _LibExpunge( AHIsubBase );
  return NULL;
}


/******************************************************************************
** Library clean-up ***********************************************************
******************************************************************************/

#ifndef __AMIGAOS4__
# define DeleteLibrary(base) FreeMem((APTR)(((char*) base) - \
					    ((struct Library*) base)->lib_NegSize), \
				     ((struct Library*) base)->lib_NegSize + \
				     ((struct Library*) base)->lib_PosSize)
#endif

BPTR
_LibExpunge( struct DriverBase* AHIsubBase )
{
  BPTR seglist = 0;

  if( AHIsubBase->library.lib_OpenCnt == 0 )
  {
    seglist = AHIsubBase->seglist;

    /* Since LibInit() calls us on failure, we have to check if we're
       really added to the library list before removing us. */

    if( AHIsubBase->library.lib_Node.ln_Succ != NULL )
    {
      Remove( (struct Node *) AHIsubBase );
    }

    DriverCleanup( AHIsubBase );

#ifdef __AMIGAOS4__
    DropInterface((struct Interface *) IAHIsub);
    DropInterface((struct Interface *) IIntuition);
    DropInterface((struct Interface *) IUtility);
#endif
    
    /* Close libraries */
    CloseLibrary(&IntuitionBase->LibNode );
    CloseLibrary(&UtilityBase->ub_LibNode );

#ifdef __AROS__
    set_call_funcs(SETNAME(EXIT), -1, 0);
#endif

    DeleteLibrary(&AHIsubBase->library);
  }
  else
  {
    AHIsubBase->library.lib_Flags |= LIBF_DELEXP;
  }

  return seglist;
}


/******************************************************************************
** Library opening ************************************************************
******************************************************************************/

struct DriverBase*
_LibOpen( ULONG              version,
	  struct DriverBase* AHIsubBase )
{
  AHIsubBase->library.lib_Flags &= ~LIBF_DELEXP;
  AHIsubBase->library.lib_OpenCnt++;

  return AHIsubBase;
}


/******************************************************************************
** Library closing ************************************************************
******************************************************************************/

BPTR
_LibClose( struct DriverBase* AHIsubBase )
{
  BPTR seglist = 0;

  AHIsubBase->library.lib_OpenCnt--;

  if( AHIsubBase->library.lib_OpenCnt == 0 )
  {
    if( AHIsubBase->library.lib_Flags & LIBF_DELEXP )
    {
      seglist = _LibExpunge( AHIsubBase );
    }
  }

  return seglist;
}


/******************************************************************************
** Unused function ************************************************************
******************************************************************************/

ULONG
_LibNull( struct DriverBase* AHIsubBase )
{
  return 0;
}
