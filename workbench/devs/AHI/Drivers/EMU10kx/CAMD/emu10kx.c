/*
     emu10kx - CAMD driver for SoundBlaster Live! series
     Copyright (C) 2003-2005 Martin Blom <martin@blom.org>

     This program is free software; you can redistribute it and/or
     modify it under the terms of the GNU General Public License
     as published by the Free Software Foundation; either version 2
     of the License, or (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <config.h>

#include <exec/memory.h>
#include <midi/camddevices.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>

#include "emu10kx-camd.h"
#include "camdstubs.h"
#include "version.h"

struct PortInfo
{
    struct Hook TransmitHook;
    struct Hook ReceiveHook;
    APTR        TransmitFunc;
    APTR        ReceiveFunc;
    APTR        UserData;
};


/*** Module entry (exactly 4 bytes!!) *****************************************/

__asm(
"  moveq #-1,%d0\n"
"  rts\n"
);

/*** Identification data must follow directly *********************************/

static struct MidiDeviceData MidiDeviceData =
{
  MDD_Magic,
  "emu10kx",
  "emu10kx CAMD MIDI driver " VERS,
  VERSION, REVISION,
  gwInit,
  gwExpunge,
  gwOpenPort,
  gwClosePort,
  4,        // For some braindamaged reason, camd.library V40 reads
	    // this value BEFORE calling Init(). :-(
  1         // Use new-style if using camd.library V40
};


/*** Global data **************************************************************/

#ifndef __AROS__
static struct ExecBase*    SysBase         = NULL;
#endif
static struct Library*     EMU10kxBase     = NULL;
static struct EMU10kxCamd* EMU10kxCamd     = NULL;
static struct PortInfo*    PortInfos       = NULL;
static ULONG               CAMDv40         = FALSE;
static const char          VersionString[] = "$VER: emu10kx " VERS "\r\n";


/*** Debug code ***************************************************************/

static UWORD rawputchar_m68k[] = 
{
  0x2C4B,             // MOVEA.L A3,A6
  0x4EAE, 0xFDFC,     // JSR     -$0204(A6)
  0x4E75              // RTS
};


static void
KPrintFArgs( UBYTE* fmt, 
             ULONG* args )
{
  RawDoFmt( fmt, args, (void(*)(void)) rawputchar_m68k, SysBase );
}

#define KPrintF( fmt, ... )        \
({                                 \
  ULONG _args[] = { __VA_ARGS__ }; \
  KPrintFArgs( (fmt), _args );     \
})


/*** CAMD callbacks ***********************************************************/

#ifdef __AROS__
static AROS_UFH3(ULONG, TransmitFunc,
	AROS_UFHA(struct Hook *, hook, A0),
	AROS_UFHA(struct Library *, emu10kxbase, A2),
	AROS_UFHA(APTR, null, A1))
{
    AROS_USERFUNC_INIT
#else
static ULONG
TransmitFunc( struct Hook*    hook         __asm( "a0" ),
	      struct Library* emu10kxbase  __asm( "a2" ),
	      APTR            null         __asm( "a1" ) )
{
#endif
  struct PortInfo* pi = (struct PortInfo*) hook->h_Data;
  ULONG            res;

  __asm volatile (
      "movel %1,%%a2\n"
      "movel %2,%%a0\n"
      "jsr   (%%a0)\n"
      "swapw %%d0\n"
      "movew %%d1,%%d0\n"
      "swapw %%d0\n"
    : "=r"(res)
    : "m" (pi->UserData), "m" (pi->TransmitFunc)
    : "a0", "a2" );

  return res;
#ifdef __AROS__
  AROS_USERFUNC_EXIT
#endif
}


#ifdef __AROS__
static AROS_UFH3(VOID, ReceiveFunc,
	AROS_UFHA(struct Hook *, hook, A0),
	AROS_UFHA(struct Library *, emu10kxbase, A2),
	AROS_UFHA(struct ReceiveMessage*, msg, A1))
{
    AROS_USERFUNC_INIT
#else
static VOID
ReceiveFunc( struct Hook*           hook        __asm( "a0" ),
	     struct Library*        emu10kxbase __asm( "a2" ),
	     struct ReceiveMessage* msg         __asm( "a1" ) )
{
#endif
  struct PortInfo* pi = (struct PortInfo*) hook->h_Data;

  __asm volatile (
    "movel %0,%%d0\n"
    "movel %1,%%a2\n"
    "movel %2,%%a0\n"
    "jsr   (%%a0)\n"
    : 
    : "m" (msg->InputByte), "m" (pi->UserData), "m" (pi->ReceiveFunc)
    : "d0", "a0", "a2" );
#ifdef __AROS__
  AROS_USERFUNC_EXIT
#endif
}


/*** ActivateXmit *************************************************************/

VOID
_ActivateXmit( APTR  userdata,
	       ULONG portnum )
{
  // In the original CAMD, there is no port number :-(

//  KPrintF( "ActiavteXmit( %08lx, %ld )\n", userdata, portnum & 255 );

  if( !CAMDv40 )
  {
    for( portnum = 0; portnum < MidiDeviceData.NPorts; ++portnum )
    {
      if( userdata == PortInfos[ portnum ].UserData )
      {
	break;
      }
    }

    if( portnum == MidiDeviceData.NPorts )
    {
      return;
    }
  }

  CallHook( &EMU10kxCamd->ActivateXmitFunc, (Object*) EMU10kxBase,
	    portnum & 255 );
}

struct MidiPortData MidiPortData =
{
  gwActivateXmit
};


/*** Init *********************************************************************/

#ifdef __AROS__
#include <aros/symbolsets.h>

THIS_PROGRAM_HANDLES_SYMBOLSET(INIT)
THIS_PROGRAM_HANDLES_SYMBOLSET(EXIT)
DEFINESET(INIT)
DEFINESET(EXIT)
#endif

VOID
_Expunge( ULONG dummy );

BOOL
_Init( struct ExecBase* sysbase )
{
  struct Library* camdlib;
  
#ifdef __AROS__
  SysBase = sysbase;
#else
  // sysbase is not valid in the original CAMD anyway
  SysBase = *(struct ExecBase**) 4;
#endif

#ifdef __AROS__
  if (!set_call_funcs(SETNAME(INIT), 1, 1))
      return NULL;
#endif

  EMU10kxBase = OpenLibrary( "DEVS:AHI/emu10kx.audio", VERSION );

  if( EMU10kxBase == NULL )
  {
    return FALSE;
  }

  Forbid();
  EMU10kxCamd = (struct EMU10kxCamd*) FindSemaphore( EMU10KX_CAMD_SEMAPHORE );
  if( EMU10kxCamd != NULL )
  {
    ObtainSemaphore( &EMU10kxCamd->Semaphore );
  }
  Permit();

  if( EMU10kxCamd == NULL )
  {
    _Expunge( 0 );
    return FALSE;
  }

  MidiDeviceData.NPorts = EMU10kxCamd->Cards;

  PortInfos = AllocVec( sizeof( struct PortInfo ) * EMU10kxCamd->Cards,
			MEMF_PUBLIC );

  if( PortInfos == NULL )
  {
    _Expunge( 0 );
    return FALSE;
  }

  return TRUE;
}


/*** Expunge ******************************************************************/

VOID
_Expunge( ULONG dummy )
{
  FreeVec( PortInfos );

  if( EMU10kxCamd != NULL )
  {
    ReleaseSemaphore( &EMU10kxCamd->Semaphore );
  }

#ifdef __AROS__
  set_call_funcs(SETNAME(EXIT), -1, 0);
#endif

  CloseLibrary( EMU10kxBase );
}


/*** OpenPort *****************************************************************/

struct MidiPortData*
_OpenPort( struct MidiDeviceData* data,
	   LONG                   portnum,
	   APTR                   transmitfunc,
	   APTR                   receivefunc,
	   APTR                   userdata )
{
  static struct Library* camdbase = NULL;
  
  struct PortInfo* pi = &PortInfos[ portnum & 255 ];

  pi->TransmitHook.h_Entry = (HOOKFUNC) TransmitFunc;
  pi->TransmitHook.h_Data  = pi;
  pi->ReceiveHook.h_Entry  = (HOOKFUNC) ReceiveFunc;
  pi->ReceiveHook.h_Data   = pi;
  pi->TransmitFunc         = transmitfunc;
  pi->ReceiveFunc          = receivefunc;
  pi->UserData             = userdata;

  if( camdbase == NULL )
  {
    camdbase = OpenLibrary( "camd.library", 0 );

    if( camdbase != NULL )
    {
      CAMDv40 = ( camdbase->lib_Version >= 40 );
    }

    // Close library but leave pointer set, so we never execute this
    // code again.
    CloseLibrary( camdbase );
  }
  
  if( !CallHook( &EMU10kxCamd->OpenPortFunc, (Object*) EMU10kxBase,
		 portnum & 255, CAMDv40, &pi->TransmitHook, &pi->ReceiveHook ) )
  {
    return NULL;
  }

  return &MidiPortData;
}


/*** ClosePort ****************************************************************/

VOID
_ClosePort( struct MidiDeviceData *data, LONG portnum )
{
  CallHook( &EMU10kxCamd->ClosePortFunc, (Object*) EMU10kxBase,
	    portnum & 255 );
}

