/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1996-2005 Martin Blom <martin@blom.org>
     
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
     
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
     
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330, Cambridge,
     MA 02139, USA.
*/

#include <config.h>

#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/iffparse.h>
#include "ahi_def.h"

#include "header.h"
#include "gateway.h"
#include "gatestubs.h"
#include "localize.h"
#include "misc.h"
#include "version.h"

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

static BOOL
OpenLibs ( void );

static void
CloseLibs ( void );

#define GetSymbol( name ) AHIGetELFSymbol( #name, (void*) &name ## Ptr )

#undef Req
#define Req( msg ) ReqA( msg, NULL )

/******************************************************************************
** Device entry ***************************************************************
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
** Device resident structure **************************************************
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
  NT_DEVICE,
  0,                      /* priority */
  (BYTE *) &DevName[0],
  (BYTE *) &IDString[6],
  (APTR) &InitTable
#if defined( __MORPHOS__ ) || defined( __AROS__ )
  , REVISION, NULL
#endif
};


/******************************************************************************
** Globals ********************************************************************
******************************************************************************/

const ULONG  DriverVersion  = 2;
const ULONG  Version        = VERSION;
const ULONG  Revision       = REVISION;

const char DevName[]  = AHINAME;
const char IDString[] = "$VER: " AHINAME " " VERS
                        " ©1994-2005 Martin Blom. " CPU " version.\r\n";


struct ExecBase           *SysBase        = NULL;
struct DosLibrary         *DOSBase        = NULL;
struct GfxBase            *GfxBase        = NULL;
struct AHIBase            *AHIBase        = NULL;
struct Library            *GadToolsBase   = NULL;
struct Library            *IFFParseBase   = NULL;
struct IntuitionBase      *IntuitionBase  = NULL;
struct LocaleBase         *LocaleBase     = NULL;
struct Device             *TimerBase      = NULL;
struct UtilityBase        *UtilityBase    = NULL;

#if defined( __AMIGAOS4__ )
struct ExecIFace          *IExec          = NULL;
struct DOSIFace           *IDOS           = NULL;
struct GadToolsIFace      *IGadTools      = NULL;
struct GraphicsIFace      *IGraphics      = NULL;
struct IFFParseIFace      *IIFFParse      = NULL;
struct IntuitionIFace     *IIntuition     = NULL;
struct LocaleIFace        *ILocale        = NULL;
struct TimerIFace         *ITimer         = NULL;
struct UtilityIFace       *IUtility       = NULL;
#endif

struct Resident           *MorphOSRes     = NULL;
static struct timerequest *TimerIO        = NULL;

#if defined( ENABLE_WARPUP )
struct Library            *PowerPCBase    = NULL;
void                      *PPCObject      = NULL;
const ULONG               __LIB_Version  = VERSION;
const ULONG               __LIB_Revision = REVISION;
#endif

enum MixBackend_t          MixBackend     = MB_NATIVE;

ADDFUNC* AddByteMonoPtr                   = AddByteMono;
ADDFUNC* AddByteStereoPtr                 = AddByteStereo;
ADDFUNC* AddByte71Ptr                     = AddByte71;
ADDFUNC* AddBytesMonoPtr                  = AddBytesMono;
ADDFUNC* AddBytesStereoPtr                = AddBytesStereo;
ADDFUNC* AddBytes71Ptr                    = AddBytes71;
ADDFUNC* AddWordMonoPtr                   = AddWordMono;
ADDFUNC* AddWordStereoPtr                 = AddWordStereo;
ADDFUNC* AddWord71Ptr                     = AddWord71;
ADDFUNC* AddWordsMonoPtr                  = AddWordsMono;
ADDFUNC* AddWordsStereoPtr                = AddWordsStereo;
ADDFUNC* AddWords71Ptr                    = AddWords71;
ADDFUNC* AddLongMonoPtr                   = AddLongMono;
ADDFUNC* AddLongStereoPtr                 = AddLongStereo;
ADDFUNC* AddLong71Ptr                     = AddLong71;
ADDFUNC* AddLongsMonoPtr                  = AddLongsMono;
ADDFUNC* AddLongsStereoPtr                = AddLongsStereo;
ADDFUNC* Add71MonoPtr                     = Add71Mono;
ADDFUNC* Add71StereoPtr                   = Add71Stereo;
ADDFUNC* AddLongs71Ptr                    = AddLongs71;
ADDFUNC* Add7171Ptr                       = Add7171;

ADDFUNC* AddByteMonoBPtr                  = AddByteMonoB;
ADDFUNC* AddByteStereoBPtr                = AddByteStereoB;
ADDFUNC* AddByte71BPtr                    = AddByte71B;
ADDFUNC* AddBytesMonoBPtr                 = AddBytesMonoB;
ADDFUNC* AddBytesStereoBPtr               = AddBytesStereoB;
ADDFUNC* AddBytes71BPtr                   = AddBytes71B;
ADDFUNC* AddWordMonoBPtr                  = AddWordMonoB;
ADDFUNC* AddWordStereoBPtr                = AddWordStereoB;
ADDFUNC* AddWord71BPtr                    = AddWord71B;
ADDFUNC* AddWordsMonoBPtr                 = AddWordsMonoB;
ADDFUNC* AddWordsStereoBPtr               = AddWordsStereoB;
ADDFUNC* AddWords71BPtr                   = AddWords71B;
ADDFUNC* AddLongMonoBPtr                  = AddLongMonoB;
ADDFUNC* AddLongStereoBPtr                = AddLongStereoB;
ADDFUNC* AddLong71BPtr                    = AddLong71B;
ADDFUNC* AddLongsMonoBPtr                 = AddLongsMonoB;
ADDFUNC* AddLongsStereoBPtr               = AddLongsStereoB;
ADDFUNC* AddLongs71BPtr                   = AddLongs71B;
ADDFUNC* Add71MonoBPtr                    = Add71MonoB;
ADDFUNC* Add71StereoBPtr                  = Add71StereoB;
ADDFUNC* Add7171BPtr                      = Add7171B;

ADDFUNC* AddLofiByteMonoPtr               = AddLofiByteMono;
ADDFUNC* AddLofiByteStereoPtr             = AddLofiByteStereo;
ADDFUNC* AddLofiBytesMonoPtr              = AddLofiBytesMono;
ADDFUNC* AddLofiBytesStereoPtr            = AddLofiBytesStereo;
ADDFUNC* AddLofiWordMonoPtr               = AddLofiWordMono;
ADDFUNC* AddLofiWordStereoPtr             = AddLofiWordStereo;
ADDFUNC* AddLofiWordsMonoPtr              = AddLofiWordsMono;
ADDFUNC* AddLofiWordsStereoPtr            = AddLofiWordsStereo;
ADDFUNC* AddLofiLongMonoPtr               = AddLofiLongMono;
ADDFUNC* AddLofiLongStereoPtr             = AddLofiLongStereo;
ADDFUNC* AddLofiLongsMonoPtr              = AddLofiLongsMono;
ADDFUNC* AddLofiLongsStereoPtr            = AddLofiLongsStereo;
ADDFUNC* AddLofiByteMonoBPtr              = AddLofiByteMonoB;
ADDFUNC* AddLofiByteStereoBPtr            = AddLofiByteStereoB;
ADDFUNC* AddLofiBytesMonoBPtr             = AddLofiBytesMonoB;
ADDFUNC* AddLofiBytesStereoBPtr           = AddLofiBytesStereoB;
ADDFUNC* AddLofiWordMonoBPtr              = AddLofiWordMonoB;
ADDFUNC* AddLofiWordStereoBPtr            = AddLofiWordStereoB;
ADDFUNC* AddLofiWordsMonoBPtr             = AddLofiWordsMonoB;
ADDFUNC* AddLofiWordsStereoBPtr           = AddLofiWordsStereoB;
ADDFUNC* AddLofiLongMonoBPtr              = AddLofiLongMonoB;
ADDFUNC* AddLofiLongStereoBPtr            = AddLofiLongStereoB;
ADDFUNC* AddLofiLongsMonoBPtr             = AddLofiLongsMonoB;
ADDFUNC* AddLofiLongsStereoBPtr           = AddLofiLongsStereoB;

/******************************************************************************
** Device code ****************************************************************
******************************************************************************/

#ifndef __AMIGAOS4__
static inline void DeleteLibrary(struct Library *base) {
  FreeMem((APTR)(((char*)base)-base->lib_NegSize),base->lib_NegSize+base->lib_PosSize);
}
#endif

struct AHIBase*
_DevInit( struct AHIBase*  device,
	  APTR             seglist,
	  struct ExecBase* sysbase )
{
  AHIBase = device;
  SysBase = sysbase;

#ifdef __AMIGAOS4__
  IExec = (struct ExecIFace*) SysBase->MainInterface; 
#endif
  
  device->ahib_Library.lib_Revision = REVISION;
  
  device->ahib_SysLib  = sysbase;
  device->ahib_SegList = (BPTR) seglist;

#if defined( __mc68000__ )
  // Make sure we're running on a M68020 or better
  
  if( ( sysbase->AttnFlags & AFF_68020 ) == 0 )
  {
    Alert( ( AN_Unknown | ACPU_InstErr ) & (~AT_DeadEnd) );
    DeleteLibrary( &device->ahib_Library );
    return NULL;
  }
#endif

  InitSemaphore( &device->ahib_Lock );

  if( !OpenLibs() )
  {
    CloseLibs();
    DeleteLibrary( &device->ahib_Library );
    return NULL;
  }

  return device;
}


BPTR
_DevExpunge( struct AHIBase* device )
{
  BPTR seglist = 0;

   //DebugPrintF("AHI: _DevExpunge\n");

  if( device->ahib_Library.lib_OpenCnt == 0)
  {
    seglist = device->ahib_SegList;

    Remove( (struct Node *) device );

    CloseLibs();

    DeleteLibrary( &device->ahib_Library );
  }
  else
  {
    device->ahib_Library.lib_Flags |= LIBF_DELEXP;
  }

  return seglist;
}

ULONG
_DevNull( void ) {
  return 0;
}


#ifndef __AMIGAOS4__

static const APTR FuncTable[] =
{
#if defined( __MORPHOS__ ) || defined( __amithlon__ )
  (APTR) FUNCARRAY_32BIT_NATIVE,
#endif

  gwDevOpen,
  gwDevClose,
  gwDevExpunge,
  gwDevNull,

  gwDevBeginIO,
  gwDevAbortIO,

  gwAHI_AllocAudioA,
  gwAHI_FreeAudio,
  gwAHI_KillAudio,
  gwAHI_ControlAudioA,
  gwAHI_SetVol,
  gwAHI_SetFreq,
  gwAHI_SetSound,
  gwAHI_SetEffect,
  gwAHI_LoadSound,
  gwAHI_UnloadSound,
  gwAHI_NextAudioID,
  gwAHI_GetAudioAttrsA,
  gwAHI_BestAudioIDA,
  gwAHI_AllocAudioRequestA,
  gwAHI_AudioRequestA,
  gwAHI_FreeAudioRequest,
  gwAHI_PlayA,
  gwAHI_SampleFrameSize,
  gwAHI_AddAudioMode,
  gwAHI_RemoveAudioMode,
  gwAHI_LoadModeFile,

  (APTR) -1
};


static const APTR InitTable[4] =
{
  (APTR) sizeof( struct AHIBase ),
  (APTR) &FuncTable,
  NULL,
#if defined( __MORPHOS__ ) || defined( __amithlon__ )
  (APTR) _DevInit
#else
  (APTR) gwDevInit
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


static const CONST_APTR DevManagerVectors[] =
{
  generic_Obtain,
  generic_Release,
  NULL,
  NULL,
  
  gwDevOpen,
  gwDevClose,
  gwDevExpunge,
  NULL,
  gwDevBeginIO,
  gwDevAbortIO,

  (CONST_APTR) -1
};

static const struct TagItem DevManagerTags[] = 
{
  { MIT_Name,             (ULONG) "__device"        },
  { MIT_VectorTable,      (ULONG) DevManagerVectors },
  { MIT_Version,          1                         },
  { TAG_DONE,             0                         }
};


static const CONST_APTR MainVectors[] = {
  generic_Obtain,
  generic_Release,
  NULL,
  NULL,

  gwAHI_AllocAudioA,
  gwAHI_AllocAudio,
  gwAHI_FreeAudio,
  gwAHI_KillAudio,
  gwAHI_ControlAudioA,
  gwAHI_ControlAudio,
  gwAHI_SetVol,
  gwAHI_SetFreq,
  gwAHI_SetSound,
  gwAHI_SetEffect,
  gwAHI_LoadSound,
  gwAHI_UnloadSound,
  gwAHI_NextAudioID,
  gwAHI_GetAudioAttrsA,
  gwAHI_GetAudioAttrs,
  gwAHI_BestAudioIDA,
  gwAHI_BestAudioID,
  gwAHI_AllocAudioRequestA,
  gwAHI_AllocAudioRequest,
  gwAHI_AudioRequestA,
  gwAHI_AudioRequest,
  gwAHI_FreeAudioRequest,
  gwAHI_PlayA,
  gwAHI_Play,
  gwAHI_SampleFrameSize,
  gwAHI_AddAudioMode,
  gwAHI_RemoveAudioMode,
  gwAHI_LoadModeFile,

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
  DevManagerTags,
  MainTags,
  NULL
};

/* m68k library vectors */
static const CONST_APTR VecTable68K[] = {
  (CONST_APTR) &m68kgwDevOpen,
  (CONST_APTR) &m68kgwDevClose,
  (CONST_APTR) &m68kgwDevExpunge,
  (CONST_APTR) &m68kgwDevNull,

  (CONST_APTR) &m68kgwDevBeginIO,
  (CONST_APTR) &m68kgwDevAbortIO,
  (CONST_APTR) &m68kgwAHI_AllocAudioA,
  (CONST_APTR) &m68kgwAHI_FreeAudio,
  (CONST_APTR) &m68kgwAHI_KillAudio,
  (CONST_APTR) &m68kgwAHI_ControlAudioA,
  (CONST_APTR) &m68kgwAHI_SetVol,
  (CONST_APTR) &m68kgwAHI_SetFreq,
  (CONST_APTR) &m68kgwAHI_SetSound,
  (CONST_APTR) &m68kgwAHI_SetEffect,
  (CONST_APTR) &m68kgwAHI_LoadSound,
  (CONST_APTR) &m68kgwAHI_UnloadSound,
  (CONST_APTR) &m68kgwAHI_NextAudioID,
  (CONST_APTR) &m68kgwAHI_GetAudioAttrsA,
  (CONST_APTR) &m68kgwAHI_BestAudioIDA,
  (CONST_APTR) &m68kgwAHI_AllocAudioRequestA,
  (CONST_APTR) &m68kgwAHI_AudioRequestA,
  (CONST_APTR) &m68kgwAHI_FreeAudioRequest,
  (CONST_APTR) &m68kgwAHI_PlayA,
  (CONST_APTR) &m68kgwAHI_SampleFrameSize,
  (CONST_APTR) &m68kgwAHI_AddAudioMode,
  (CONST_APTR) &m68kgwAHI_RemoveAudioMode,
  (CONST_APTR) &m68kgwAHI_LoadModeFile,

  (CONST_APTR) -1
};

/* CreateLibrary() tag list */
static const struct TagItem InitTable[] =
{
  { CLT_DataSize,         sizeof(struct AHIBase) },
  { CLT_Interfaces,       (ULONG) Interfaces     },
  { CLT_Vector68K,        (ULONG) VecTable68K    },
  { CLT_InitFunc,         (ULONG) gwDevInit      },
  { TAG_DONE,             0                      }
};

#endif // __AMIGAOS4__


/******************************************************************************
** OpenLibs *******************************************************************
******************************************************************************/

// This function is called by the device startup code when the device is
// first loaded into memory.

static BOOL
OpenLibs ( void )
{
  /* Intuition Library */

  IntuitionBase = (struct IntuitionBase *) OpenLibrary( "intuition.library", 37 );

  if( IntuitionBase == NULL)
  {
    Alert(AN_Unknown|AG_OpenLib|AO_Intuition);
    return FALSE;
  }

  /* DOS Library */

  DOSBase = (struct DosLibrary *) OpenLibrary( "dos.library", 37 );

  if( DOSBase == NULL)
  {
    Req( "Unable to open 'dos.library'." );
    return FALSE;
  }

  /* Graphics Library */

  GfxBase = (struct GfxBase *) OpenLibrary( "graphics.library", 37 );

  if( GfxBase == NULL)
  {
    Req( "Unable to open 'graphics.library'." );
    return FALSE;
  }

  /* GadTools Library */

  GadToolsBase = OpenLibrary( "gadtools.library", 37 );

  if( GadToolsBase == NULL)
  {
    Req( "Unable to open 'gadtools.library'." );
    return FALSE;
  }

  /* IFFParse Library */

  IFFParseBase = OpenLibrary( "iffparse.library", 37 );

  if( IFFParseBase == NULL)
  {
    Req( "Unable to open 'iffparse.library'." );
    return FALSE;
  }

  /* Locale Library */

  LocaleBase = (struct LocaleBase*) OpenLibrary( "locale.library", 38 );

  /* Timer Device */

  TimerIO = (struct timerequest *) AllocMem( sizeof(struct timerequest),
                                             MEMF_PUBLIC | MEMF_CLEAR );

  if( TimerIO == NULL)
  {
    Req( "Out of memory." );
    return FALSE;
  }

  if( OpenDevice( "timer.device",
                  UNIT_VBLANK,
                  (struct IORequest *)
                  TimerIO,
                  0) != 0 )
  {
    Req( "Unable to open 'timer.device'." );
//    return FALSE; 
  }
  else
  {
    TimerBase = (struct Device *) TimerIO->tr_node.io_Device;
  }

  /* Utility Library */

  UtilityBase = (struct UtilityBase *) OpenLibrary( "utility.library", 37 );

  if( UtilityBase == NULL)
  {
    Req( "Unable to open 'utility.library'." );
    return FALSE;
  }


#ifdef __AMIGAOS4__
  if ((IIntuition = (struct IntuitionIFace *) GetInterface((struct Library *) IntuitionBase, "main", 1, NULL)) == NULL)
  {
    Req("Couldn't open IIntuition interface!\n");
    return FALSE;
  }

  if ((IDOS = (struct DOSIFace *) GetInterface((struct Library *) DOSBase, "main", 1, NULL)) == NULL)
  {
    Req("Couldn't open IDOS interface!\n");
    return FALSE;
  }

  if ((IGraphics = (struct GraphicsIFace *) GetInterface((struct Library *) GfxBase, "main", 1, NULL)) == NULL)
  {
    Req("Couldn't open Graphics interface!\n");
    return FALSE;
  }
  
  if ((IGadTools = (struct GadToolsIFace *) GetInterface((struct Library *) GadToolsBase, "main", 1, NULL)) == NULL)
  {
    Req("Couldn't open IGadTools interface!\n");
    return FALSE;
  }

  if ((IIFFParse = (struct IFFParseIFace *) GetInterface((struct Library *) IFFParseBase, "main", 1, NULL)) == NULL)
  {
    Req("Couldn't open IFFParse interface!\n");
    return FALSE;
  }

  if ((ILocale = (struct LocaleIFace *) GetInterface((struct Library *) LocaleBase, "main", 1, NULL)) == NULL)
  {
    Req("Couldn't open ILocale interface!\n");
    return FALSE;
  }

  if ((ITimer = (struct TimerIFace *) GetInterface((struct Library *) TimerBase, "main", 1, NULL)) == NULL)
  {
    Req("Couldn't open Timer interface!\n");
    return FALSE;
  }
  
  if ((IUtility = (struct UtilityIFace *) GetInterface((struct Library *) UtilityBase, "main", 1, NULL)) == NULL)
  {
    Req("Couldn't open Utility interface!\n");
    return FALSE;
  }

  if ((IAHI = (struct AHIIFace *) GetInterface((struct Library *) AHIBase, "main", 1, NULL)) == NULL)
  {
    Req("Couldn't open AHI interface!\n");
    return FALSE;
  }
#endif

  
  /* MorphOS/PowerUp/WarpOS loading

  Strategy:

  1) If MorphOS is running, use it.
  2) If PowerUp is running, but not WarpUp, use the m68k core
  3) If neither of them are running, try WarpUp.

  */

  // Check if MorpOS/PowerUp/WarpUp is running.
  {
#if defined( ENABLE_WARPUP )
    struct Library* ppclib     = NULL;
    struct Library* powerpclib = NULL;
#endif

    Forbid();
    MorphOSRes  = FindResident( "MorphOS" );
    
#if defined( ENABLE_WARPUP )
    powerpclib = (struct Library *) FindName( &SysBase->LibList,
                                              "powerpc.library" );
    ppclib     = (struct Library *) FindName( &SysBase->LibList,
                                              "ppc.library" );
#endif

    Permit();

#if defined( ENABLE_WARPUP )
    if( MorphOSRes == NULL && ! ( ppclib != NULL && powerpclib == NULL ) )
    {
      // Open WarpUp (but not if MorphOS or PowerUp is active)

      PowerPCBase = OpenLibrary( "powerpc.library", 15 );
    }
#endif
  }

  if( MorphOSRes != NULL )
  {
    MixBackend  = MB_NATIVE;
  }

#if defined( ENABLE_WARPUP )

  else if( PowerPCBase != NULL )
  {
    MixBackend = MB_WARPUP;

    /* Load our code to PPC..  */

    PPCObject = AHILoadObject( "DEVS:ahi.device.elf" );

    if( PPCObject != NULL )
    {
      ULONG* version = NULL;
      ULONG* revision = NULL;

      int r = ~0;

      AHIGetELFSymbol( "__LIB_Version", (void*) &version );
      AHIGetELFSymbol( "__LIB_Revision", (void*) &revision );
    
      if( version != NULL && revision != NULL )
      {
        if( *version == VERSION && *revision == REVISION )
        {
          r &= GetSymbol( AddByteMono     );
          r &= GetSymbol( AddByteStereo   );
          r &= GetSymbol( AddBytesMono    );
          r &= GetSymbol( AddBytesStereo  );
          r &= GetSymbol( AddWordMono     );
          r &= GetSymbol( AddWordStereo   );
          r &= GetSymbol( AddWordsMono    );
          r &= GetSymbol( AddWordsStereo  );
          r &= GetSymbol( AddLongMono     );
          r &= GetSymbol( AddLongStereo   );
          r &= GetSymbol( AddLongsMono    );
          r &= GetSymbol( AddLongsStereo  );
          r &= GetSymbol( AddByteMonoB    );
          r &= GetSymbol( AddByteStereoB  );
          r &= GetSymbol( AddBytesMonoB   );
          r &= GetSymbol( AddBytesStereoB );
          r &= GetSymbol( AddWordMonoB    );
          r &= GetSymbol( AddWordStereoB  );
          r &= GetSymbol( AddWordsMonoB   );
          r &= GetSymbol( AddWordsStereoB );
          r &= GetSymbol( AddLongMonoB    );
          r &= GetSymbol( AddLongStereoB  );
          r &= GetSymbol( AddLongsMonoB   );
          r &= GetSymbol( AddLongsStereoB );

          r &= GetSymbol( AddLofiByteMono     );
          r &= GetSymbol( AddLofiByteStereo   );
          r &= GetSymbol( AddLofiBytesMono    );
          r &= GetSymbol( AddLofiBytesStereo  );
          r &= GetSymbol( AddLofiWordMono     );
          r &= GetSymbol( AddLofiWordStereo   );
          r &= GetSymbol( AddLofiWordsMono    );
          r &= GetSymbol( AddLofiWordsStereo  );
          r &= GetSymbol( AddLofiLongMono     );
          r &= GetSymbol( AddLofiLongStereo   );
          r &= GetSymbol( AddLofiLongsMono    );
          r &= GetSymbol( AddLofiLongsStereo  );
          r &= GetSymbol( AddLofiByteMonoB    );
          r &= GetSymbol( AddLofiByteStereoB  );
          r &= GetSymbol( AddLofiBytesMonoB   );
          r &= GetSymbol( AddLofiBytesStereoB );
          r &= GetSymbol( AddLofiWordMonoB    );
          r &= GetSymbol( AddLofiWordStereoB  );
          r &= GetSymbol( AddLofiWordsMonoB   );
          r &= GetSymbol( AddLofiWordsStereoB );
          r &= GetSymbol( AddLofiLongMonoB    );
          r &= GetSymbol( AddLofiLongStereoB  );
          r &= GetSymbol( AddLofiLongsMonoB   );
          r &= GetSymbol( AddLofiLongsStereoB );

          if( r != 0 )
          {
            char buffer[ 2 ] = "0";
        
            GetVar( "PowerPC/UseDisable", buffer, sizeof buffer, 0 );
        
            if( buffer[ 0 ] == '1' )
            {
              // OK, then...
            }
            else
            {
              Req( "The WarpUp variable 'PowerPC/UseDisable' must be '1'." );

              AHIUnloadObject( PPCObject );
              PPCObject  = NULL;
              MixBackend = MB_NATIVE;
            }
          }
          else
          {
            Req( "Unable to fetch all symbols from ELF object." );

            AHIUnloadObject( PPCObject );
            PPCObject  = NULL;
            MixBackend = MB_NATIVE;
          }
        }
        else
        {
          Req( "'ahi.device.elf' version %ld.%ld doesn't match "
               "'ahi.device' version %ld.%ld.",
               *version, *revision, VERSION, REVISION );

          AHIUnloadObject( PPCObject );
          PPCObject  = NULL;
          MixBackend = MB_NATIVE;
        }
      }
      else
      {
        Req( "Unable to fetch version information from 'ahi.device.elf'." );

        AHIUnloadObject( PPCObject );
        PPCObject  = NULL;
        MixBackend = MB_NATIVE;
      }
    }
    else
    {
      MixBackend = MB_NATIVE;
    }
  }

#endif

  else 
  {
    //MixBackend = MB_NATIVE;
  }

  OpenahiCatalog(NULL, NULL);

  return TRUE;
}


/******************************************************************************
** CloseLibs *******************************************************************
******************************************************************************/

// This function is called by DevExpunge() when the device is about to be
// flushed

static void
CloseLibs ( void )
{
  CloseahiCatalog();

#if defined( ENABLE_WARPUP )
  if( PPCObject != NULL )
  {
    AHIUnloadObject( PPCObject );
  }

  CloseLibrary( PowerPCBase );
#endif

#ifdef __AMIGAOS4__
  DropInterface((struct Interface *) IUtility );
  DropInterface((struct Interface *) ITimer );
  DropInterface((struct Interface *) ILocale );
  DropInterface((struct Interface *) IIFFParse );
  DropInterface((struct Interface *) IGadTools );
  DropInterface((struct Interface *) IGraphics );
  DropInterface((struct Interface *) IDOS );
  DropInterface((struct Interface *) IIntuition );
  DropInterface((struct Interface *) IAHI );
#endif

  CloseLibrary( (struct Library *) UtilityBase );

  if( TimerIO != NULL )
  {
    if( TimerBase != NULL )
    {
      CloseDevice( (struct IORequest *) TimerIO );
    }

    FreeMem( TimerIO, sizeof(struct timerequest) );
  }

  CloseLibrary( (struct Library *) LocaleBase );
  CloseLibrary( IFFParseBase );
  CloseLibrary( GadToolsBase );
  CloseLibrary( (struct Library *) GfxBase );
  CloseLibrary( (struct Library *) DOSBase );
  CloseLibrary( (struct Library *) IntuitionBase );
}
