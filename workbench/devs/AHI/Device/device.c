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

#include <exec/alerts.h>
#include <exec/errors.h>
#include <exec/tasks.h>
#include <exec/io.h>
#include <exec/devices.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <libraries/iffparse.h>
#include <prefs/prefhdr.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/iffparse.h>
#define __NOLIBBASE__
#define __NOGLOBALIFACE__
#include <proto/ahi.h>
#undef  __NOLIBBASE__
#undef  __NOGLOBALIFACE__
#include <proto/ahi_sub.h>

#include <stddef.h>

#include "ahi_def.h"
#include "debug.h"
#include "device.h"
#include "devcommands.h"
#include "gateway.h"
#include "header.h"
#include "misc.h"

#ifdef __MORPHOS__
#define IS_MORPHOS 1
#else
#define IS_MORPHOS 0
#endif

/*
** Message passed to the Unit Process at
** startup time.
*/

struct StartupMessage
{
        struct Message           Msg;
        struct AHIDevUnit       *Unit;
};


static struct
AHIDevUnit *InitUnit( ULONG , struct AHIBase * );

static void
ExpungeUnit( struct AHIDevUnit *, struct AHIBase * );

static void
PlayerFunc( struct Hook*         hook,
            struct AHIAudioCtrl* actrl,
            APTR                 null );


static ULONG 
RecordFunc( struct Hook*             hook,
            struct AHIAudioCtrl*     actrl,
            struct AHIRecordMessage* recmsg );


static void
SoundFunc( struct Hook*            hook,
           struct AHIAudioCtrl*    actrl,
           struct AHISoundMessage* sndmsg );


static void
ChannelInfoFunc( struct Hook*              hook,
                 struct AHIAudioCtrl*      actrl,
                 struct AHIEffChannelInfo* cimsg );


/***** ahi.device/--background-- *******************************************
*
*   PURPOSE
*
*       The 'ahi.device' was first created because the lack of standards
*       when it comes to sound cards on the Amiga. Another reason was to
*       make it easier to write multi-channel music programs.
*
*       This device is by no means the final and perfect solution. But
*       hopefully, it can evolve into something useful until AT brings you
*       The Real Thing (TM).
*
*   OVERVIEW
*
*       Please see the document "AHI Developer's Guide" for more
*       information.
*
*
*       * Driver based
*
*       Each supported sound card is controlled by a library-based audio
*       driver. For a 'dumb' sound card, a new driver could be written in
*       a few hours. For a 'smart' sound card, it is possible to utilize an
*       on-board DSP, for example, to maximize performance and sound quality.
*       For sound cards with own DSP but little or no memory, it is possible
*       to use the main CPU to mix channels and do the post-processing
*       with the DSP. Drivers are available for most popular sound cards,
*       as well as an 8SVX (mono) and AIFF/AIFC (mono & stereo) sample render
*       driver.
*  
*       * Fast, powerful mixing routines (yeah, right... haha)
*  
*       The device's mixing routines mix 8- or 16-bit signed samples,
*       both mono, stereo and 7.1, located in Fast-RAM and outputs
*       16-bit mono or stereo (with stereo panning if desired) data,
*       using any number of channels (as long as 'any' means less than
*       128).  Tables can be used speed the mixing up (especially when
*       using 8-bit samples).  The samples can have any length
*       (including odd) and can have any number of loops.  There are
*       also so-called HiFi mixing routines that can be used, that use
*       linear interpolation and gives 32 bit output.
*       
*       * Support for non-realtime mixing
*  
*       By providing a timing feature, it is possible to create high-
*       quality output even if the processing power is lacking, by saving
*       the output to disk, for example as an IFF AIFF or 8SXV file.
*  
*       * Audio database
*  
*       Uses ID codes, much like Screenmode IDs, to select the many
*       parameters that can be set. The functions to access the audio
*       database are not too different from those in 'graphics.library'.
*       The device also features a requester to get an ID code from the
*       user.
*  
*       * Both high- and low-level protocol
*  
*       By acting both like a device and a library, AHI gives the programmer
*       a choice between full control and simplicity. The device API allows
*       several programs to use the audio hardware at the same time, and
*       the AUDIO: dos-device driver makes playing and recording sound very
*       simple for both the programmer and user.
*  
*       * Future Compatible
*  
*       When AmigaOS gets device-independent audio worth it's name, it should
*       not be too difficult to write a driver for AHI, allowing applications
*       using 'ahi.device' to automatically use the new OS interface. At
*       least I hope it wont.
*
*
****************************************************************************
*
*/


/******************************************************************************
** DevOpen ********************************************************************
******************************************************************************/

/****** ahi.device/OpenDevice **********************************************
*
*   NAME
*       OpenDevice -- Open the device
*
*   SYNOPSIS
*       error = OpenDevice(AHINAME, unit, ioRequest, flags)
*       D0                 A0       D0    A1         D1
*
*       BYTE OpenDevice(STRPTR, ULONG, struct AHIRequest *, ULONG);
*
*   FUNCTION
*       This is an exec call.  Exec will search for the ahi.device, and
*       if found, will pass this call on to the device.
*
*   INPUTS
*       AHINAME - pointer to the string "ahi.device".
*       unit - Either AHI_DEFAULT_UNIT (0), AHI_NO_UNIT (255) or any other
*           unit the user has requested, for example with a UNIT tooltype.
*           AHI_NO_UNIT should be used when you're using the low-level
*           API.
*       ioRequest - a pointer to a struct AHIRequest, initialized by
*           exec.library/CreateIORequest(). ahir_Version *must* be preset
*           to the version you need!
*       flags - There is only one flag defined, AHIDF_NOMODESCAN, which
*           asks ahi.device not to build the audio mode database if not
*           already initialized. It should not be used by applications
*           without good reasons (AddAudioModes uses this flag).
*
*   RESULT
*       error - Same as io_Error.
*       io_Error - If the call succeeded, io_Error will be 0, else
*           an error code as defined in <exec/errors.h> and
*           <devices/ahi.h>.
*       io_Device - A pointer to the device base, which can be used
*           to call the functions the device provides.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*      CloseDevice(), exec.library/OpenDevice(), <exec/errors.h>,
*      <devices/ahi.h>.
*
****************************************************************************
*
*/

// This function is called by the system each time a unit is opened with
// exec.library/OpenDevice().

ULONG
_DevOpen ( struct AHIRequest* ioreq,
	   ULONG              unit,
	   ULONG              flags,
	   struct AHIBase*    AHIBase )
{
  ULONG rc = 0;
  BOOL  error = FALSE;
  struct AHIDevUnit *iounit=NULL;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("OpenDevice(%ld, 0x%08lx, %ld)", unit, (ULONG) ioreq, flags);
  }

// Check if size includes the ahir_Version field

  if(ioreq->ahir_Std.io_Message.mn_Length < (sizeof(struct IOStdReq) + 2))
  {
    Req( "Bad parameters to OpenDevice()." );
    ioreq->ahir_Std.io_Error=IOERR_OPENFAIL;
    return IOERR_OPENFAIL;
  }

// One more check...

  if((unit != AHI_NO_UNIT) && (ioreq->ahir_Version >= 4))
  {
    if(ioreq->ahir_Std.io_Message.mn_Length < sizeof(struct AHIRequest))
    {
      Req( "Bad parameters to OpenDevice()." );
      ioreq->ahir_Std.io_Error=IOERR_OPENFAIL;
      return IOERR_OPENFAIL;
    }
    else
    {
/*       KPrintF( "Tagging %08lx on task %08lx\n", ioreq, FindTask(0)); */
      ioreq->ahir_Private[1] = (IPTR) ioreq;
    }
  }

  AHIBase->ahib_Library.lib_OpenCnt++;

  ObtainSemaphore(&AHIBase->ahib_Lock);

  if( ! (flags & AHIDF_NOMODESCAN))
  {
    // Load database if not already loaded

    if(AHI_NextAudioID(AHI_INVALID_ID) == (ULONG) AHI_INVALID_ID)
    {
      AHI_LoadModeFile("DEVS:AudioModes");

      // Be quiet here. - Piru
      if (IS_MORPHOS)
      {
        APTR *windowptr = &((struct Process *) FindTask(NULL))->pr_WindowPtr;
        APTR oldwindowptr = *windowptr;
        *windowptr = (APTR) -1;

        AHI_LoadModeFile("MOSSYS:DEVS/AudioModes");

        *windowptr = oldwindowptr;
      }
    }
  }

  if( ioreq->ahir_Version > AHIBase->ahib_Library.lib_Version)
    error=TRUE;
  else
  {
    if(unit < AHI_UNITS)
    {
      iounit=InitUnit(unit,AHIBase);
      if(!iounit) 
        error=TRUE;
    }
    else if(unit == AHI_NO_UNIT)
      InitUnit(unit,AHIBase);
  }

  if(!error)
  {
    ioreq->ahir_Std.io_Unit=(struct Unit *) iounit;
    if(iounit)    // Is NULL for AHI_NO_UNIT
      iounit->Unit.unit_OpenCnt++;
    AHIBase->ahib_Library.lib_OpenCnt++;
    AHIBase->ahib_Library.lib_Flags &=~LIBF_DELEXP;
  }
  else
  {
    rc=IOERR_OPENFAIL;
    ioreq->ahir_Std.io_Error=IOERR_OPENFAIL;
    ioreq->ahir_Std.io_Device=(struct Device *) -1;
    ioreq->ahir_Std.io_Unit=(struct Unit *) -1;
  }

  ReleaseSemaphore(&AHIBase->ahib_Lock);

  AHIBase->ahib_Library.lib_OpenCnt--;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("=>%ld\n",rc);
  }

  return rc;
}


/******************************************************************************
** DevClose *******************************************************************
******************************************************************************/

/****** ahi.device/CloseDevice *********************************************
*
*   NAME
*       CloseDevice -- Close the device
*
*   SYNOPSIS
*       CloseDevice(ioRequest)
*                   A1
*
*       void CloseDevice(struct IORequest *);
*
*   FUNCTION
*       This is an exec call that closes the device. Every OpenDevice()
*       must be matched with a call to CloseDevice().
*
*       The user must ensure that all outstanding IO Requests have been
*       returned before closing the device.
*
*   INPUTS
*       ioRequest - a pointer to the same struct AHIRequest that was used
*           to open the device.
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*      OpenDevice(), exec.library/CloseDevice()
*
****************************************************************************
*
*/

// This function is called by the system each time a unit is closed with
// exec.library/CloseDevice().

BPTR
_DevClose ( struct AHIRequest* ioreq,
	    struct AHIBase*    AHIBase )
{
  struct AHIDevUnit *iounit;
  BPTR  seglist=0;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("CloseDevice(0x%P)\n", ioreq);

    if( ioreq->ahir_Private[1] != (IPTR) ioreq )
    {
      KPrintF( "Warning: Expected I/O request 0x%P.\n",
	       ioreq->ahir_Private[1] );
    }
  }

  ObtainSemaphore(&AHIBase->ahib_Lock);

  iounit= (struct AHIDevUnit *) ioreq->ahir_Std.io_Unit;
  ioreq->ahir_Std.io_Device = (struct Device *) -1;
  ioreq->ahir_Std.io_Unit = (struct Unit *) -1;

  if(iounit)
  {
    iounit->Unit.unit_OpenCnt--;
    if(!iounit->Unit.unit_OpenCnt)
      ExpungeUnit(iounit,AHIBase);
  }

  AHIBase->ahib_Library.lib_OpenCnt--;

  ReleaseSemaphore(&AHIBase->ahib_Lock);

  if(!AHIBase->ahib_Library.lib_OpenCnt)
  {
    if(AHIBase->ahib_Library.lib_Flags & LIBF_DELEXP)
      seglist=_DevExpunge(AHIBase);
  }
  return seglist;
}


/******************************************************************************
** InitUnit *******************************************************************
******************************************************************************/

// This function is called by DevOpen() to initialize a unit

static struct AHIDevUnit *
InitUnit ( ULONG unit, 
           struct AHIBase *AHIBase )
{
  struct AHIDevUnit *iounit;

  if( unit == AHI_NO_UNIT )
  {
    ReadConfig(NULL,AHIBase);
    return NULL;
  }
  else if(!AHIBase->ahib_DevUnits[unit])
  {
    if((iounit = AllocVec(sizeof(struct AHIDevUnit), MEMF_CLEAR|MEMF_PUBLIC)))
    {
      NewList(&iounit->Unit.unit_MsgPort.mp_MsgList);

      iounit->Unit.unit_MsgPort.mp_Node.ln_Type = NT_MSGPORT;
      iounit->Unit.unit_MsgPort.mp_Flags = PA_IGNORE;
      iounit->Unit.unit_MsgPort.mp_Node.ln_Name = AHINAME " Unit";
      iounit->UnitNum = unit;
      AHIInitSemaphore(&iounit->Lock);
      NewList((struct List *)&iounit->ReadList);
      NewList((struct List *)&iounit->PlayingList);
      NewList((struct List *)&iounit->SilentList);
      NewList((struct List *)&iounit->WaitingList);
      NewList((struct List *)&iounit->RequestQueue);

      if(ReadConfig(iounit,AHIBase))
      {
        if((iounit->Voices = AllocVec(
            sizeof(struct Voice)*iounit->Channels,MEMF_PUBLIC|MEMF_CLEAR)))
        {
          int i;
          struct Voice   *v = iounit->Voices;
          struct MsgPort *replyport;
          
          // Mark all channels as free
          for(i = 0 ; i < iounit->Channels; i++)
          {
            v->NextOffset = FREE;
            v++;
          }

          replyport = CreateMsgPort();

          if( replyport != NULL )
          {
            struct StartupMessage sm =
            {
              {
                { NULL, NULL, NT_UNKNOWN, 0, NULL },
                replyport, sizeof(struct StartupMessage),
              },
              iounit
            };

            iounit->Process = CreateNewProcTags( NP_Entry,    (IPTR) &m68k_DevProc,
                                                 NP_Name,     (IPTR) AHINAME " Unit Process",
                                                 NP_Priority, AHI_PRI,
                                                 TAG_DONE );

            if( iounit->Process != NULL )
            {
  
                PutMsg( &iounit->Process->pr_MsgPort,
                        &sm.Msg );

                WaitPort(replyport);
                GetMsg(replyport);
            }
            DeleteMsgPort(replyport);
          }
        }
      }

      if(!iounit->Process)
        FreeVec(iounit);
      else
        AHIBase->ahib_DevUnits[unit] = iounit;

    }
  }
  return AHIBase->ahib_DevUnits[unit];
}


/******************************************************************************
** ExpungeUnit ****************************************************************
******************************************************************************/

// This function is called by DevClose() to remove a unit.

static void 
ExpungeUnit ( struct AHIDevUnit *iounit,
              struct AHIBase *AHIBase )
{
  struct Task *unittask;
  BYTE signal;

  signal = AllocSignal(-1);
  if(signal == -1)
  {
    /* Fallback */
    signal = SIGB_SINGLE;
    SetSignal(0, SIGF_SINGLE);
  }

  unittask = (struct Task *) iounit->Process;
  iounit->Process = (struct Process *) FindTask(NULL);
  iounit->SyncSignal = signal;
  Signal(unittask,SIGBREAKF_CTRL_F);
  Wait(1UL << signal);
  AHIBase->ahib_DevUnits[iounit->UnitNum]=NULL;
  FreeVec(iounit->Voices);
  FreeVec(iounit);

  if(signal != SIGB_SINGLE)
  {
    FreeSignal(signal);
  }
}


/******************************************************************************
** ReadConfig *****************************************************************
******************************************************************************/

// This functions loads the users settings for AHI.

BOOL
ReadConfig ( struct AHIDevUnit *iounit,
             struct AHIBase *AHIBase )
{
  struct IFFHandle *iff;
  struct StoredProperty *prhd,*ahig;
  struct CollectionItem *ci;
  ULONG *mode;

  if(iounit)
  {
    /* Internal defaults for device unit */
    iounit->AudioMode       = AHI_INVALID_ID;   // See at the end of the function!
    iounit->Frequency       = 10000;
    iounit->Channels        = 4;
    iounit->MonitorVolume   = ~0;
    iounit->InputGain       = ~0;
    iounit->OutputVolume    = ~0;
    iounit->Input           = ~0;
    iounit->Output          = ~0;
  }
  else
  {
    /* Internal defaults for low-level mode */
    AHIBase->ahib_AudioMode       = AHI_INVALID_ID;
    AHIBase->ahib_Frequency       = 10000;
    AHIBase->ahib_MonitorVolume   = 0x00000;
    AHIBase->ahib_InputGain       = 0x10000;
    AHIBase->ahib_OutputVolume    = 0x10000;
    AHIBase->ahib_Input           = 0;
    AHIBase->ahib_Output          = 0;
  }

  if((iff=AllocIFF()))
  {
    iff->iff_Stream=(IPTR) Open("ENV:Sys/ahi.prefs", MODE_OLDFILE);
    if(iff->iff_Stream)
    {
      InitIFFasDOS(iff);
      if(!OpenIFF(iff,IFFF_READ))
      {
        if(!(PropChunk(iff,ID_PREF,ID_PRHD)
          || PropChunk(iff,ID_PREF,ID_AHIG)
          || CollectionChunk(iff,ID_PREF,ID_AHIU)
          || StopOnExit(iff,ID_PREF,ID_FORM)))
        {
          if(ParseIFF(iff,IFFPARSE_SCAN) == IFFERR_EOC)
          {
            prhd=FindProp(iff,ID_PREF,ID_PRHD);
            ahig=FindProp(iff,ID_PREF,ID_AHIG);
            
            if(ahig)
            {
              struct AHIGlobalPrefs *globalprefs;
	      UWORD debug_level;
              
              globalprefs = (struct AHIGlobalPrefs *)ahig->sp_Data;

              debug_level = globalprefs->ahigp_DebugLevel;
	      EndianSwap( sizeof (UWORD), &debug_level );
              AHIBase->ahib_DebugLevel = debug_level;

              AHIBase->ahib_Flags = 0;

	      /* Not used in version 5:
	       * 
	       * if(globalprefs->ahigp_DisableSurround)
	       *   AHIBase->ahib_Flags |= AHIBF_NOSURROUND;
	       *
	       * if(globalprefs->ahigp_DisableEcho)
               *   AHIBase->ahib_Flags |= AHIBF_NOECHO;
	       *
	       * if(globalprefs->ahigp_FastEcho)
               *   AHIBase->ahib_Flags |= AHIBF_FASTECHO;
	       *
	       */
	      
              if( (ULONG) ahig->sp_Size > offsetof( struct AHIGlobalPrefs,
                                                    ahigp_MaxCPU) )
              {
                AHIBase->ahib_MaxCPU = globalprefs->ahigp_MaxCPU;
		EndianSwap( sizeof (Fixed), &AHIBase->ahib_MaxCPU );
              }
              else
              {
                AHIBase->ahib_MaxCPU = 0x10000 * 90 / 100;
              }

	      /* In version 5: Clipping is always used
	       *
               * if( (ULONG) ahig->sp_Size > offsetof( struct AHIGlobalPrefs, 
               *                                       ahigp_ClipMasterVolume) )
               * {
               *   if(globalprefs->ahigp_ClipMasterVolume)
               *     AHIBase->ahib_Flags |= AHIBF_CLIPPING;
               * }
	       */

              if( (ULONG) ahig->sp_Size > offsetof( struct AHIGlobalPrefs, 
                                                    ahigp_AntiClickTime ) )
              {
                AHIBase->ahib_AntiClickTime = globalprefs->ahigp_AntiClickTime;
		EndianSwap( sizeof (Fixed), &AHIBase->ahib_AntiClickTime );
              }
              else
              {
                AHIBase->ahib_AntiClickTime = 0;
              }

              if( (ULONG) ahig->sp_Size > offsetof( struct AHIGlobalPrefs, 
                                                    ahigp_ScaleMode ) )
              {
                AHIBase->ahib_ScaleMode = globalprefs->ahigp_ScaleMode;
		EndianSwap( sizeof (UWORD), &AHIBase->ahib_ScaleMode );
              }
              else
              {
                AHIBase->ahib_ScaleMode = AHI_SCALE_FIXED_0_DB;
              }
            }
            ci=FindCollection(iff,ID_PREF,ID_AHIU);
            while(ci)
            {
              struct AHIUnitPrefs *unitprefs;

              unitprefs = (struct AHIUnitPrefs *)ci->ci_Data;

              if(iounit)
              {
                if(unitprefs->ahiup_Unit == iounit->UnitNum)
                {
                  iounit->AudioMode       = unitprefs->ahiup_AudioMode;
                  iounit->Frequency       = unitprefs->ahiup_Frequency;
                  iounit->Channels        = unitprefs->ahiup_Channels;
                  iounit->MonitorVolume   = unitprefs->ahiup_MonitorVolume;
                  iounit->InputGain       = unitprefs->ahiup_InputGain;
                  iounit->OutputVolume    = unitprefs->ahiup_OutputVolume;
                  iounit->Input           = unitprefs->ahiup_Input;
                  iounit->Output          = unitprefs->ahiup_Output;

		  EndianSwap( sizeof (ULONG), &iounit->AudioMode );
		  EndianSwap( sizeof (ULONG), &iounit->Frequency );
		  EndianSwap( sizeof (UWORD), &iounit->Channels );
		  EndianSwap( sizeof (Fixed), &iounit->MonitorVolume );
		  EndianSwap( sizeof (Fixed), &iounit->InputGain );
		  EndianSwap( sizeof (Fixed), &iounit->OutputVolume );
		  EndianSwap( sizeof (ULONG), &iounit->Input );
		  EndianSwap( sizeof (ULONG), &iounit->Output );
                }
              }
              else
              {
                if(unitprefs->ahiup_Unit == AHI_NO_UNIT)
                {
                  AHIBase->ahib_AudioMode       = unitprefs->ahiup_AudioMode;
                  AHIBase->ahib_Frequency       = unitprefs->ahiup_Frequency;
                  AHIBase->ahib_MonitorVolume   = unitprefs->ahiup_MonitorVolume;
                  AHIBase->ahib_InputGain       = unitprefs->ahiup_InputGain;
                  AHIBase->ahib_OutputVolume    = unitprefs->ahiup_OutputVolume;
                  AHIBase->ahib_Input           = unitprefs->ahiup_Input;
                  AHIBase->ahib_Output          = unitprefs->ahiup_Output;

		  EndianSwap( sizeof (ULONG), &AHIBase->ahib_AudioMode);
		  EndianSwap( sizeof (ULONG), &AHIBase->ahib_Frequency);
		  EndianSwap( sizeof (Fixed), &AHIBase->ahib_MonitorVolume);
		  EndianSwap( sizeof (Fixed), &AHIBase->ahib_InputGain);
		  EndianSwap( sizeof (Fixed), &AHIBase->ahib_OutputVolume);
		  EndianSwap( sizeof (ULONG), &AHIBase->ahib_Input);
		  EndianSwap( sizeof (ULONG), &AHIBase->ahib_Output);
                }
              }

              ci=ci->ci_Next;
            }
          }
        }
        CloseIFF(iff);
      }
      Close((BPTR) iff->iff_Stream);
    }
    FreeIFF(iff);
  }

  // Avoids calling AHI_BestAudioID if not neccessary (faster startup time,
  // since doesn't open all sub libraries.

  if(iounit)
    mode = &iounit->AudioMode;
  else
    mode = &AHIBase->ahib_AudioMode;
  if(mode[0] == (ULONG) AHI_INVALID_ID)
  { static const Tag tags[] = { AHIDB_Realtime,TRUE,TAG_DONE };
    mode[0] = AHI_BestAudioIDA((struct TagItem *)tags);
  }

  return TRUE;
}


/******************************************************************************
** AllocHardware **************************************************************
******************************************************************************/

// Allocates the audio hardware

BOOL
AllocHardware ( struct AHIDevUnit *iounit,
                struct AHIBase *AHIBase )
{
  BOOL rc = FALSE;
  ULONG fullduplex = FALSE;
  ULONG stereo     = FALSE;
  ULONG panning    = FALSE;

  /* Allocate the hardware */
  iounit->AudioCtrl = AHI_AllocAudio(
      AHIA_AudioID,       iounit->AudioMode,
      AHIA_MixFreq,       iounit->Frequency,
      AHIA_Channels,      iounit->Channels,
      AHIA_Sounds,        MAXSOUNDS,
      AHIA_PlayerFunc,    &iounit->PlayerHook,
      AHIA_RecordFunc,    &iounit->RecordHook,
      AHIA_SoundFunc,     &iounit->SoundHook,
      TAG_DONE);

  if(iounit->AudioCtrl != NULL)
  {
    /* Full duplex? */
    AHI_GetAudioAttrs(AHI_INVALID_ID,iounit->AudioCtrl,
      AHIDB_FullDuplex, &fullduplex,
      AHIDB_Stereo,     &stereo,
      AHIDB_Panning,    &panning,
      TAG_DONE);
    iounit->FullDuplex   = fullduplex;
    iounit->PseudoStereo = stereo && !panning;

    /* Set hardware properties */
    AHI_ControlAudio(iounit->AudioCtrl,
        (iounit->MonitorVolume == ~0 ? TAG_IGNORE : AHIC_MonitorVolume),
        iounit->MonitorVolume,

        (iounit->InputGain == ~0 ? TAG_IGNORE : AHIC_InputGain),
        iounit->InputGain,

        (iounit->OutputVolume == ~0 ? TAG_IGNORE : AHIC_OutputVolume),
        iounit->OutputVolume,

        (iounit->Input == ~0 ? TAG_IGNORE : AHIC_Input),
        iounit->Input,

        (iounit->Output == ~0 ? TAG_IGNORE : AHIC_Output),
        iounit->Output,

        TAG_DONE);

    iounit->ChannelInfoStruct->ahie_Effect = AHIET_CHANNELINFO;
    iounit->ChannelInfoStruct->ahieci_Func = &iounit->ChannelInfoHook;
    iounit->ChannelInfoStruct->ahieci_Channels = iounit->Channels;
    if(!AHI_SetEffect(iounit->ChannelInfoStruct, iounit->AudioCtrl))
    {
      rc = TRUE;
    }
  }
  return rc;
}


/******************************************************************************
** FreeHardware ***************************************************************
******************************************************************************/

// Take a wild guess!

void
FreeHardware ( struct AHIDevUnit *iounit,
               struct AHIBase *AHIBase )
{
  if(iounit->AudioCtrl)
  {
    if(iounit->ChannelInfoStruct)
    {
      iounit->ChannelInfoStruct->ahie_Effect = (AHIET_CANCEL | AHIET_CHANNELINFO);
      AHI_SetEffect(iounit->ChannelInfoStruct, iounit->AudioCtrl);
    }
    AHI_FreeAudio(iounit->AudioCtrl);
    iounit->AudioCtrl = NULL;
    iounit->IsRecording = FALSE;
    iounit->IsPlaying = FALSE;
    iounit->ValidRecord = FALSE;
  }
}


/******************************************************************************
** DevProc ********************************************************************
******************************************************************************/

void
DevProc( void )
{
  struct Process *proc;
  struct StartupMessage *sm;
  struct AHIDevUnit *iounit;
  BYTE  signalbit;

  proc = (struct Process *)FindTask(NULL);
  WaitPort(&proc->pr_MsgPort);
  sm = (struct StartupMessage *)GetMsg(&proc->pr_MsgPort);
  iounit = sm->Unit;

  iounit->Process = NULL;

  iounit->PlayerHook.h_Entry    = (HOOKFUNC) HookEntry;
  iounit->PlayerHook.h_SubEntry = (HOOKFUNC) PlayerFunc;
  iounit->PlayerHook.h_Data     = iounit;

  iounit->RecordHook.h_Entry    = (HOOKFUNC) HookEntry;
  iounit->RecordHook.h_SubEntry = (HOOKFUNC) RecordFunc;
  iounit->RecordHook.h_Data     = iounit;

  iounit->SoundHook.h_Entry    = (HOOKFUNC) HookEntry;
  iounit->SoundHook.h_SubEntry = (HOOKFUNC) SoundFunc;
  iounit->SoundHook.h_Data     = iounit;

  iounit->ChannelInfoHook.h_Entry    = (HOOKFUNC) HookEntry;
  iounit->ChannelInfoHook.h_SubEntry = (HOOKFUNC) ChannelInfoFunc;
  iounit->ChannelInfoHook.h_Data     = iounit;

  iounit->ChannelInfoStruct = AllocVec(
      sizeof(struct AHIEffChannelInfo) + (iounit->Channels * sizeof(ULONG)),
      MEMF_PUBLIC | MEMF_CLEAR);

  iounit->Master=proc;

  signalbit = AllocSignal(-1);
  iounit->PlaySignal = AllocSignal(-1);
  iounit->RecordSignal = AllocSignal(-1);
  iounit->SampleSignal = AllocSignal(-1);

  if((signalbit != -1)
  && (iounit->PlaySignal != -1)
  && (iounit->RecordSignal != -1)
  && (iounit->SampleSignal != -1)
  && (iounit->ChannelInfoStruct != NULL)
  )
  {
    /* Set up our Unit's MsgPort. */
    iounit->Unit.unit_MsgPort.mp_SigBit = signalbit;
    iounit->Unit.unit_MsgPort.mp_SigTask = (struct Task *)proc;
    iounit->Unit.unit_MsgPort.mp_Flags = PA_SIGNAL;

    /* Allocate the hardware */
    if(AllocHardware(iounit, AHIBase))
    {

      /* Set iounit->Process to pointer to our unit process.
         This will let the Unit init code know that were
         are okay. */
      iounit->Process = proc;
    }
  }

  /* Reply to our startup message */
  ReplyMsg(&sm->Msg);

  if(iounit->Process)
  {
    ULONG  waitmask,signals;

    waitmask = (1L << signalbit)
             | SIGBREAKF_CTRL_E    // Dummy signal to wake up task
             | SIGBREAKF_CTRL_F    // Quit signal
             | (1L << iounit->PlaySignal)
             | (1L << iounit->RecordSignal)
             | (1L << iounit->SampleSignal);

    while(TRUE)
    {
      signals = Wait(waitmask);

      /* Have we been signaled to shut down? */
      if(signals & SIGBREAKF_CTRL_F)
        break;

      if(signals & (1L << iounit->SampleSignal))
      {
        RethinkPlayers(iounit,AHIBase);
      }

      if(signals & (1L << signalbit))
      {
        struct AHIRequest *ioreq;

        while((ioreq = (struct AHIRequest *) GetMsg(&iounit->Unit.unit_MsgPort)))
        {
          PerformIO(ioreq,AHIBase);
        }
      }
      
      if(signals & (1L << iounit->PlaySignal))
      {
        AHIObtainSemaphore(&iounit->Lock);

        UpdateSilentPlayers(iounit,AHIBase);

        AHIReleaseSemaphore(&iounit->Lock);
      }

      if(signals & (1L << iounit->RecordSignal))
      {
        iounit->ValidRecord = TRUE;
        FeedReaders(iounit,AHIBase);
      }
    }
  }

  FreeHardware(iounit, AHIBase);
  FreeSignal(iounit->SampleSignal);
  iounit->SampleSignal = -1;
  FreeSignal(iounit->RecordSignal);
  iounit->RecordSignal = -1;
  FreeSignal(iounit->PlaySignal);
  iounit->PlaySignal = -1;
  FreeVec(iounit->ChannelInfoStruct);

  if(iounit->Process)
  {
    Forbid();
    Signal((struct Task *) iounit->Process, 1UL << iounit->SyncSignal);
  }
  FreeSignal(signalbit);
}


/******************************************************************************
** PlayerFunc *****************************************************************
******************************************************************************/

static void
PlayerFunc( struct Hook*         hook,
            struct AHIAudioCtrl* actrl,
            APTR                 null )
{
  struct AHIDevUnit *iounit = (struct AHIDevUnit *) hook->h_Data;

  if(AHIAttemptSemaphore(&iounit->Lock))
  {
    UpdateSilentPlayers(iounit,AHIBase);
    AHIReleaseSemaphore(&iounit->Lock);
  }
  else
  { // Do it later instead
    Signal((struct Task *) iounit->Master, (1L << iounit->PlaySignal));
  }

  return;
}


/******************************************************************************
** RecordFunc *****************************************************************
******************************************************************************/

static ULONG 
RecordFunc( struct Hook*             hook,
            struct AHIAudioCtrl*     actrl,
            struct AHIRecordMessage* recmsg )
{
  struct AHIDevUnit *iounit;
  
  if(recmsg->ahirm_Type == AHIST_S16S)
  {
    iounit = (struct AHIDevUnit *) hook->h_Data;
    iounit->RecordBuffer = recmsg->ahirm_Buffer;
    iounit->RecordSize = recmsg->ahirm_Length<<2;
    Signal((struct Task *) iounit->Master, (1L << iounit->RecordSignal));
  }
  return 0;
}


/******************************************************************************
** SoundFunc ******************************************************************
******************************************************************************/

static void
SoundFunc( struct Hook*            hook,
           struct AHIAudioCtrl*    actrl,
           struct AHISoundMessage* sndmsg )
{
  struct AHIDevUnit* iounit;
  struct Voice*      voice;  
  struct AHIRequest* playreq;

  iounit = (struct AHIDevUnit *) hook->h_Data;
  voice = &iounit->Voices[(WORD)sndmsg->ahism_Channel];

  Disable();    // Not needed?

  playreq = voice->PlayingRequest;

  if( playreq != NULL )
  {
    playreq->ahir_Std.io_Command = AHICMD_WRITTEN;
  }

  Enable();

  voice->PlayingRequest = voice->QueuedRequest;
  voice->Flags |= VF_STARTED;
  voice->QueuedRequest  = NULL;

  switch(voice->NextOffset)
  {
    case FREE:
      break;

    case MUTE:
      /* A AHI_NOSOUND is done, channel is silent */
      voice->NextOffset = FREE;
      break;

    case PLAY:
      /* A normal sound is done and playing, no other sound is queued */
      AHI_SetSound(sndmsg->ahism_Channel,AHI_NOSOUND,0,0,actrl,AHISF_NONE);
      voice->NextOffset = MUTE;
      break;

    default:
      /* A normal sound is done, and another is waiting */
      AHI_SetSound(sndmsg->ahism_Channel,
          voice->NextSound,
          voice->NextOffset,
          voice->NextLength,
          actrl,AHISF_NONE);
      AHI_SetFreq(sndmsg->ahism_Channel,
          voice->NextFrequency,
          actrl,AHISF_NONE);
      AHI_SetVol(sndmsg->ahism_Channel,
          voice->NextVolume,
          voice->NextPan,
          actrl,AHISF_NONE);
      voice->QueuedRequest = voice->NextRequest;
      voice->NextRequest = NULL;
      voice->NextOffset = PLAY;
      break;
  }

  Signal((struct Task *) iounit->Master, (1L << iounit->SampleSignal));
}


/******************************************************************************
** ChannelInfoFunc ************************************************************
******************************************************************************/

// This hook keeps updating the io_Actual field of each playing requests

static void
ChannelInfoFunc( struct Hook*              hook,
                 struct AHIAudioCtrl*      actrl,
                 struct AHIEffChannelInfo* cimsg )
{
  struct AHIDevUnit *iounit = (struct AHIDevUnit *) hook->h_Data;
  struct Voice      *voice;
  ULONG             *offsets = (ULONG *) &cimsg->ahieci_Offset;
  int i;

  Disable();    // Not needed?

  voice = iounit->Voices;

  for(i = 0; i < iounit->Channels; i++)
  {
    struct AHIRequest* playreq;

    playreq = voice->PlayingRequest;

    if( playreq != NULL )
    {
      playreq->ahir_Std.io_Actual = *offsets;
    }

    voice++;
    offsets++;
  }

  Enable();

  return;
}
