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

#include <dos/dos.h>
#include <exec/errors.h>
#include <exec/tasks.h>
#include <exec/io.h>
#include <exec/devices.h>
#include <exec/memory.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#define __NOLIBBASE__
#define __NOGLOBALIFACE__
#include <proto/ahi.h>
#undef  __NOLIBBASE__
#undef  __NOGLOBALIFACE__
#include <proto/ahi_sub.h>

#include <math.h>

#include "ahi_def.h"
#include "debug.h"
#include "misc.h"
#include "devcommands.h"
#include "device.h"
#include "devsupp.h"


#ifdef __AMIGAOS4__
#define IAHIsub ((struct AHIPrivAudioCtrl *) iounit->AudioCtrl)->ahiac_IAHIsub
#endif

static void TermIO(struct AHIRequest *, struct AHIBase *);
static void Devicequery(struct AHIRequest *, struct AHIBase *);
static void ResetCmd(struct AHIRequest *, struct AHIBase *);
static void ReadCmd(struct AHIRequest *, struct AHIBase *);
static void WriteCmd(struct AHIRequest *, struct AHIBase *);
static void StopCmd(struct AHIRequest *, struct AHIBase *);
static void StartCmd(struct AHIRequest *, struct AHIBase *);
static void FlushCmd(struct AHIRequest *, struct AHIBase *);

static void FillReadBuffer(struct AHIRequest *, struct AHIDevUnit *, struct AHIBase *);

static void NewWriter(struct AHIRequest *, struct AHIDevUnit *, struct AHIBase *);
static void AddWriter(struct AHIRequest *, struct AHIDevUnit *, struct AHIBase *);
static void PlayRequest(int, struct AHIRequest *, struct AHIDevUnit *, struct AHIBase *);
static void RemPlayers( struct List *, struct AHIDevUnit *, struct AHIBase *);

static void UpdateMasterVolume( struct AHIDevUnit *, struct AHIBase * );

#define MultFixed( a, b ) ( (unsigned long) ( ( ( (unsigned long long) a ) << 16 ) / b ) )

/******************************************************************************
** DevBeginIO *****************************************************************
******************************************************************************/

// This function is called by the system each time exec.library/DoIO()
// is called.

void
_DevBeginIO( struct AHIRequest* ioreq,
	     struct AHIBase*    AHIBase )
{
  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("BeginIO(0x%P)\n", ioreq);
  }

  ioreq->ahir_Std.io_Message.mn_Node.ln_Type = NT_MESSAGE;

  switch(ioreq->ahir_Std.io_Command)
  {

// Immediate commands
    case NSCMD_DEVICEQUERY:
    case CMD_STOP:
    case CMD_FLUSH:
      PerformIO(ioreq,AHIBase);
      break;

// Queued commands
    case CMD_RESET:
    case CMD_READ:
    case CMD_WRITE:
    case CMD_START:
      ioreq->ahir_Std.io_Flags &= ~IOF_QUICK;
      PutMsg(&ioreq->ahir_Std.io_Unit->unit_MsgPort,&ioreq->ahir_Std.io_Message);
      break;

// Unknown commands
    default:
      ioreq->ahir_Std.io_Error = IOERR_NOCMD;
      TermIO(ioreq,AHIBase);
      break;
  }
}


/******************************************************************************
** AbortIO ********************************************************************
******************************************************************************/

// This function is called by the system each time exec.library/AbortIO()
// is called.

ULONG
_DevAbortIO( struct AHIRequest* ioreq,
	     struct AHIBase*    AHIBase )
{
  ULONG rc = 0;
  struct AHIDevUnit *iounit;
  
  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("AbortIO(0x%P)", ioreq);
  }

  iounit = (struct AHIDevUnit *) ioreq->ahir_Std.io_Unit;

  AHIObtainSemaphore(&iounit->Lock);

  if(ioreq->ahir_Std.io_Message.mn_Node.ln_Type != NT_REPLYMSG)
  {
    switch(ioreq->ahir_Std.io_Command)
    {

      case CMD_READ:
        if(FindNode((struct List *) &iounit->ReadList, (struct Node *) ioreq))
        {
          Remove((struct Node *) ioreq);
          ioreq->ahir_Std.io_Error = IOERR_ABORTED;
          TermIO(ioreq,AHIBase);
        }
        break;

      case CMD_WRITE:
      case AHICMD_WRITTEN:
        if(FindNode((struct List *) &iounit->PlayingList, (struct Node *) ioreq)
        || FindNode((struct List *) &iounit->SilentList, (struct Node *) ioreq)
        || FindNode((struct List *) &iounit->WaitingList, (struct Node *) ioreq))
        {
          struct AHIRequest *nextreq;
          struct AHIRequest *io;

          while(ioreq)
          {
            Remove((struct Node *) ioreq);

	    // Now check if any other request ahir_Link to us. If so,
	    // we need to clear that field, since this request is no
	    // longer valid.

	    
	    for (io = (struct AHIRequest*) iounit->PlayingList.mlh_Head;
		 io->ahir_Std.io_Message.mn_Node.ln_Succ != NULL;
		 io = (struct AHIRequest*) io->ahir_Std.io_Message.mn_Node.ln_Succ) {
	      if (io->ahir_Link == ioreq) {
		io->ahir_Link = NULL;
		goto cleared;
	      }
	    }

	    for (io = (struct AHIRequest*) iounit->SilentList.mlh_Head;
		 io->ahir_Std.io_Message.mn_Node.ln_Succ != NULL;
		 io = (struct AHIRequest*) io->ahir_Std.io_Message.mn_Node.ln_Succ) {
	      if (io->ahir_Link == ioreq) {
		io->ahir_Link = NULL;
		goto cleared;
	      }
	    }

	    for (io = (struct AHIRequest*) iounit->WaitingList.mlh_Head;
		 io->ahir_Std.io_Message.mn_Node.ln_Succ != NULL;
		 io = (struct AHIRequest*) io->ahir_Std.io_Message.mn_Node.ln_Succ) {
	      if (io->ahir_Link == ioreq) {
		io->ahir_Link = NULL;
		goto cleared;
	      }
	    }
cleared:
	    
            if(ioreq->ahir_Extras && (GetExtras(ioreq)->Channel != NOCHANNEL))
            {
              struct Library *AHIsubBase = NULL;

              if( iounit->AudioCtrl != NULL )
              {
                AHIsubBase = ((struct AHIPrivAudioCtrl *) iounit->AudioCtrl)
                             ->ahiac_SubLib;
              }

              if( AHIsubBase != NULL )
              {
                AHIsub_Disable((struct AHIAudioCtrlDrv *) iounit->AudioCtrl);
              }

              iounit->Voices[GetExtras(ioreq)->Channel].PlayingRequest = NULL;
              iounit->Voices[GetExtras(ioreq)->Channel].QueuedRequest = NULL;
              iounit->Voices[GetExtras(ioreq)->Channel].NextRequest = NULL;
  
              if(iounit->AudioCtrl)
              {
                iounit->Voices[GetExtras(ioreq)->Channel].NextOffset = MUTE;
                AHI_SetSound(GetExtras(ioreq)->Channel,AHI_NOSOUND,0,0,
                    iounit->AudioCtrl,AHISF_IMM);
              }
              else
              {
                iounit->Voices[GetExtras(ioreq)->Channel].NextOffset = FREE;
              }

              if( AHIsubBase != NULL )
              {
                AHIsub_Enable((struct AHIAudioCtrlDrv *) iounit->AudioCtrl);
              }
            }

            ioreq->ahir_Std.io_Command = CMD_WRITE;
            ioreq->ahir_Std.io_Error   = IOERR_ABORTED;
            nextreq = ioreq->ahir_Link;
            TermIO(ioreq,AHIBase);
            ioreq = nextreq;
          }
        }
        break;

      default:
        rc = IOERR_NOCMD;
        break;
    }
  }

  AHIReleaseSemaphore(&iounit->Lock);

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("=>%ld\n",rc);
  }

  return rc;
}


/******************************************************************************
** TermIO *********************************************************************
******************************************************************************/

// This functions returns an IO request back to the sender.

static void
TermIO ( struct AHIRequest *ioreq,
         struct AHIBase *AHIBase )
{
  struct AHIDevUnit *iounit;
  ULONG error = ioreq->ahir_Std.io_Error;

  iounit = (struct AHIDevUnit *) ioreq->ahir_Std.io_Unit;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("Terminating IO Request 0x%08lx", (ULONG) ioreq);
  }

  if( ioreq->ahir_Std.io_Command == CMD_WRITE )
  {
    // Update master volume if we're terminating a write request
    UpdateMasterVolume( iounit, AHIBase );

    // Convert io_Actual to bytes
    ioreq->ahir_Std.io_Actual *= AHI_SampleFrameSize(ioreq->ahir_Type);
  }
  
  if(ioreq->ahir_Extras != 0)
  {
    int  sound  = GetExtras(ioreq)->Sound;
    APTR extras = (APTR) ioreq->ahir_Extras;

    if((sound != AHI_NOSOUND) && (sound < MAXSOUNDS))
    {
      AHI_UnloadSound(sound, iounit->AudioCtrl);
      iounit->Sounds[sound] = SOUND_FREE;
    }

    ioreq->ahir_Extras = 0;
    FreeVec( extras );
  }

  if( ! (ioreq->ahir_Std.io_Flags & IOF_QUICK))
  {
      ReplyMsg(&ioreq->ahir_Std.io_Message);
  }

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("=>%ld\n", error);
  }
}


/******************************************************************************
** PerformIO ******************************************************************
******************************************************************************/

void
PerformIO ( struct AHIRequest *ioreq,
            struct AHIBase *AHIBase )
{
  struct AHIDevUnit *iounit;

  iounit = (struct AHIDevUnit *) ioreq->ahir_Std.io_Unit;
  ioreq->ahir_Std.io_Error = 0;

  // Just to make sure TermIO won't free a bad address
  ioreq->ahir_Extras = 0;

  switch(ioreq->ahir_Std.io_Command)
  {
    case NSCMD_DEVICEQUERY:
      Devicequery(ioreq, AHIBase);
      break;

    case CMD_RESET:
      ResetCmd(ioreq, AHIBase);
      break;

    case CMD_READ:
      ReadCmd(ioreq, AHIBase);
      break;

    case CMD_WRITE:
      AHIObtainSemaphore(&iounit->Lock);

      if(iounit->StopCnt)
      {
        AddTail((struct List *) &iounit->RequestQueue,(struct Node *) ioreq);
      }
      else
      {
        WriteCmd(ioreq, AHIBase);
      }

      AHIReleaseSemaphore(&iounit->Lock);
      break;

    case CMD_STOP:
      StopCmd(ioreq, AHIBase);
      break;

    case CMD_START:
      StartCmd(ioreq, AHIBase);
      break;

    case CMD_FLUSH:
      FlushCmd(ioreq, AHIBase);
      break;

    default:
      ioreq->ahir_Std.io_Error = IOERR_NOCMD;
      TermIO(ioreq, AHIBase);
      break;
  }
}


/******************************************************************************
** Devicequery ****************************************************************
******************************************************************************/

/****** ahi.device/NSCMD_DEVICEQUERY  ***************************************
*
*   NAME
*       NSCMD_DEVICEQUERY -- Query the device for its capabilities (V4)
*
*   FUNCTION
*       Fills an initialized NSDeviceQueryResult structure with
*       information about the device.
*
*   IO REQUEST INPUT
*       io_Device       Preset by the call to OpenDevice().
*       io_Unit         Preset by the call to OpenDevice().
*       io_Command      NSCMD_DEVICEQUERY
*       io_Data         Pointer to the NSDeviceQueryResult structure,
*                       initialized as follows:
*                           DevQueryFormat - Set to 0
*                           SizeAvailable  - Must be cleared.
*                       It is probably good manners to clear all other
*                       fields as well.
*       io_Length       Size of the NSDeviceQueryResult structure.
*
*   IO REQUEST RESULT
*       io_Error        0 for success, or an error code as defined in
*                       <ahi/devices.h> and <exec/errors.h>.
*       io_Actual       If io_Error is 0, the value in
*                       NSDeviceQueryResult.SizeAvailable.
*
*       The NSDeviceQueryResult structure now contains valid information.
*
*       The other fields, except io_Device, io_Unit and io_Command, are
*       trashed.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       <ahi/devices.h>, <exec/errors.h>
*
****************************************************************************
*
*/

static UWORD
commandlist[] =
{
  NSCMD_DEVICEQUERY,
  CMD_RESET,
  CMD_READ,
  CMD_WRITE,
  CMD_STOP,
  CMD_START,
  CMD_FLUSH,
  0
};

static void
Devicequery ( struct AHIRequest *ioreq,
              struct AHIBase *AHIBase )
{
  struct NSDeviceQueryResult *dqr;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    KPrintF("NSCMD_DEVICEQUERY\n");
  }

  dqr = ioreq->ahir_Std.io_Data;
  if(ioreq->ahir_Std.io_Length >= 16)
  {
    dqr->SizeAvailable = 16;
    dqr->DeviceType = NSDEVTYPE_UNKNOWN;
    dqr->DeviceSubType = 0;
    dqr->SupportedCommands = commandlist;
  }

  ioreq->ahir_Std.io_Actual = dqr->SizeAvailable;
  TermIO(ioreq, AHIBase);
}


/******************************************************************************
** StopCmd ********************************************************************
******************************************************************************/

/****** ahi.device/CMD_STOP ************************************************
*
*   NAME
*       CMD_STOP -- stop device processing (like ^S) (V4)
*
*   FUNCTION
*       Stops all CMD_WRITE processing. All writes will be queued, and
*       are not processed until CMD_START. This is useful for synchronizing
*       two or more CMD_WRITE's.
*
*   IO REQUEST INPUT
*       io_Device       Preset by the call to OpenDevice().
*       io_Unit         Preset by the call to OpenDevice().
*       io_Command      CMD_STOP
*
*   IO REQUEST RESULT
*       io_Error        0 for success, or an error code as defined in
*                       <ahi/devices.h> and <exec/errors.h>.
*
*       The other fields, except io_Device, io_Unit and io_Command, are
*       trashed.
*
*   EXAMPLE
*
*   NOTES
*       This command affects ALL writes, even those sent by other
*       applications. Make sure the code between CMD_STOP and CMD_START
*       runs as fast as possible!
*
*       Unlike most (all?) other devices, CMD_STOP and CMD_START do nest in
*       ahi.device.
*
*   BUGS
*
*   SEE ALSO
*       CMD_START, <ahi/devices.h>, <exec/errors.h>
*
****************************************************************************
*
*/

static void
StopCmd ( struct AHIRequest *ioreq, 
          struct AHIBase *AHIBase )
{
  struct AHIDevUnit *iounit;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    KPrintF("CMD_STOP\n");
  }

  iounit = (struct AHIDevUnit *) ioreq->ahir_Std.io_Unit;

  AHIObtainSemaphore(&iounit->Lock);

  iounit->StopCnt++;

  AHIReleaseSemaphore(&iounit->Lock);

  TermIO(ioreq,AHIBase);
}


/******************************************************************************
** FlushCmd *******************************************************************
******************************************************************************/

/****** ahi.device/CMD_FLUSH ************************************************
*
*   NAME
*       CMD_FLUSH -- Cancel all I/O requests (V4)
*
*   FUNCTION
*       Aborts ALL current requests, both active and waiting, even
*       other programs requests!
*
*   IO REQUEST INPUT
*       io_Device       Preset by the call to OpenDevice().
*       io_Unit         Preset by the call to OpenDevice().
*       io_Command      CMD_FLUSH
*
*   IO REQUEST RESULT
*       io_Error        0 for success, or an error code as defined in
*                       <ahi/devices.h> and <exec/errors.h>.
*       io_Actual       If io_Error is 0, number of requests actually
*                       flushed.
*
*       The other fields, except io_Device, io_Unit and io_Command, are
*       trashed.
*
*   EXAMPLE
*
*   NOTES
*       This command should only be used in very rare cases, like AHI
*       system utilities. Never use this command in an application.
*
*   BUGS
*
*   SEE ALSO
*       CMD_RESET, <ahi/devices.h>, <exec/errors.h>
*
****************************************************************************
*
*/

static void
FlushCmd ( struct AHIRequest *ioreq,
           struct AHIBase *AHIBase )
{
  struct AHIDevUnit *iounit;
  struct AHIRequest *ior;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    KPrintF("CMD_FLUSH\n");
  }

  iounit = (struct AHIDevUnit *) ioreq->ahir_Std.io_Unit;

  ioreq->ahir_Std.io_Actual = 0;

  // Abort all current IO-requests
  while((ior = (struct AHIRequest *) iounit->ReadList.mlh_Head))
  {
    _DevAbortIO(ior, AHIBase);
    ioreq->ahir_Std.io_Actual++;
  }
  while((ior = (struct AHIRequest *) iounit->PlayingList.mlh_Head))
  {
    _DevAbortIO(ior, AHIBase);
    ioreq->ahir_Std.io_Actual++;
  }
  while((ior = (struct AHIRequest *) iounit->SilentList.mlh_Head))
  {
    _DevAbortIO(ior, AHIBase);
    ioreq->ahir_Std.io_Actual++;
  }
  while((ior = (struct AHIRequest *) iounit->WaitingList.mlh_Head))
  {
    _DevAbortIO(ior, AHIBase);
    ioreq->ahir_Std.io_Actual++;
  }
  TermIO(ioreq,AHIBase);
}



/* All the following functions are called within the unit process context */


/******************************************************************************
** ResetCmd *******************************************************************
******************************************************************************/

/****** ahi.device/CMD_RESET ************************************************
*
*   NAME
*       CMD_RESET -- Restore device to a known state (V4)
*
*   FUNCTION
*       Aborts all current requests, even other programs requests
*       (CMD_FLUSH), rereads the configuration file and resets the hardware
*       to its initial state
*       
*
*   IO REQUEST INPUT
*       io_Device       Preset by the call to OpenDevice().
*       io_Unit         Preset by the call to OpenDevice().
*       io_Command      CMD_RESET
*
*   IO REQUEST RESULT
*       io_Error        0 for success, or an error code as defined in
*                       <ahi/devices.h> and <exec/errors.h>.
*
*       The other fields, except io_Device, io_Unit and io_Command, are
*       trashed.
*
*   EXAMPLE
*
*   NOTES
*       This command should only be used in very rare cases, like AHI
*       system utilities. Never use this command in an application.
*
*   BUGS
*
*   SEE ALSO
*       CMD_FLUSH, <ahi/devices.h>, <exec/errors.h>
*
****************************************************************************
*
*/

static void
ResetCmd ( struct AHIRequest *ioreq,
           struct AHIBase *AHIBase )
{
  struct AHIDevUnit *iounit;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    KPrintF("CMD_RESET\n");
  }

  iounit = (struct AHIDevUnit *) ioreq->ahir_Std.io_Unit;

  // Remove all requests (beware, invalid IORequest to FlushCmd!)
  FlushCmd(ioreq, AHIBase);

  // Reset the hardware
  ReadConfig(iounit, AHIBase);
  FreeHardware(iounit, AHIBase);
  AllocHardware(iounit, AHIBase);
  TermIO(ioreq,AHIBase);
}


/******************************************************************************
** ReadCmd ********************************************************************
******************************************************************************/

/****** ahi.device/CMD_READ *************************************************
*
*   NAME
*       CMD_READ -- Read raw samples from audio input (V4)
*
*   FUNCTION
*       Reads samples from the users prefered input to memory. The sample
*       format and frequency will be converted on the fly. 
*
*   IO REQUEST INPUT
*       io_Device       Preset by the call to OpenDevice().
*       io_Unit         Preset by the call to OpenDevice().
*       io_Command      CMD_READ
*       io_Data         Pointer to the buffer where the data should be put.
*       io_Length       Number of bytes to read, must be a multiple of the
*                       sample frame size (see ahir_Type).
*       io_Offset       Set to 0 when you use for the first time or after
*                       a delay.
*       ahir_Type       The desired sample format, see <ahi/devices.h>.
*       ahir_Frequency  The desired sample frequency in Hertz.
*
*   IO REQUEST RESULT
*       io_Error        0 for success, or an error code as defined in
*                       <ahi/devices.h> and <exec/errors.h>.
*       io_Actual       If io_Error is 0, number of bytes actually
*                       transferred. Starting with V6, io_Actual is also
*                       valid if io_Error is not 0 (like if the request
*                       was aborted).
*       io_Offset       Updated to be used as input next time.
*
*       The other fields, except io_Device, io_Unit and io_Command, are
*       trashed.
*
*   EXAMPLE
*
*   NOTES
*       It's only possible to read signed mono or stereo samples.
*
*   BUGS
*
*   SEE ALSO
*       <ahi/devices.h>, <exec/errors.h>
*
****************************************************************************
*
*/

static void 
ReadCmd ( struct AHIRequest *ioreq,
          struct AHIBase *AHIBase )
{
  struct AHIDevUnit *iounit;
  ULONG error=AHIE_OK,mixfreq = 0;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    KPrintF("CMD_READ\n");
  }

  iounit = (struct AHIDevUnit *) ioreq->ahir_Std.io_Unit;

  ioreq->ahir_Std.io_Actual = 0;

  /* Start recording if neccessary */
  if( ! iounit->IsRecording)
  {
    if( (! iounit->FullDuplex) && iounit->IsPlaying)
    {
      error = AHIE_HALFDUPLEX;   // FIXIT!
    }
    else
    { static const Tag tags[] = { AHIC_Record,TRUE,TAG_DONE };
      error = AHI_ControlAudioA(iounit->AudioCtrl, (struct TagItem *)tags);
    }

    if( ! error)
    {
      iounit->IsRecording = TRUE;
    }
  }

  if(iounit->IsRecording)
  {
    AHI_ControlAudio(iounit->AudioCtrl,
        AHIC_MixFreq_Query, &mixfreq,
        TAG_DONE);

    /* Initialize ahir_Frequency for the assembler record routines */
    if(ioreq->ahir_Frequency && mixfreq)
      ioreq->ahir_Frequency = ((mixfreq << 15) / ioreq->ahir_Frequency) << 1;
    else
      ioreq->ahir_Frequency = 0x00010000;       // Fixed 1.0

    AHIObtainSemaphore(&iounit->Lock);

    /* Add the request to the list of readers */
    AddTail((struct List *) &iounit->ReadList,(struct Node *) ioreq);

    /* Copy the current buffer contents */
    FillReadBuffer(ioreq, iounit, AHIBase);

    AHIReleaseSemaphore(&iounit->Lock);
  }
  else
  {
    ioreq->ahir_Std.io_Error = error;
    TermIO(ioreq, AHIBase);
  }
}


/******************************************************************************
** WriteCmd *******************************************************************
******************************************************************************/

/****** ahi.device/CMD_WRITE ************************************************
*
*   NAME
*       CMD_WRITE -- Write raw samples to audio output (V4)
*
*   FUNCTION
*       Plays the samples to the users prefered audio output.
*
*   IO REQUEST INPUT
*       io_Device       Preset by the call to OpenDevice().
*       io_Unit         Preset by the call to OpenDevice().
*       io_Command      CMD_WRITE
*       io_Data         Pointer to the buffer of samples to be played.
*       io_Length       Number of bytes to play, must be a multiple of the
*                       sample frame size (see ahir_Type).
*       io_Offset       Must be 0.
*       ahir_Type       The desired sample format, see <ahi/devices.h>.
*       ahir_Frequency  The desired sample frequency in Hertz.
*       ahir_Volume     The desired volume. The range is 0 to 0x10000, where
*                       0 means muted and 0x10000 (== 1.0) means full volume.
*       ahir_Position   Defines the stereo balance. 0 is far left, 0x8000 is
*                       center and 0x10000 is far right.
*       ahir_Link       If non-zero, pointer to a previously sent AHIRequest
*                       which this AHIRequest will be linked to. This
*                       request will be delayed until the old one is
*                       finished (used for double buffering). Must be set
*                       to NULL if not used.
*
*   IO REQUEST RESULT
*       io_Error        0 for success, or an error code as defined in
*                       <ahi/devices.h> and <exec/errors.h>.
*       io_Actual       If io_Error is 0, number of bytes actually
*                       played. Starting with V6, io_Actual is also valid
*                       if io_Error is not 0 (like if the request was
*                       aborted).
*
*       The other fields, except io_Device, io_Unit and io_Command, are
*       trashed.
*
*   EXAMPLE
*
*   NOTES
*       32 bit samples (ahir_Type) is only available in V6 and later.
*
*   BUGS
*
*   SEE ALSO
*       <ahi/devices.h>, <exec/errors.h>
*
****************************************************************************
*
*/

static void
WriteCmd ( struct AHIRequest *ioreq,
           struct AHIBase *AHIBase )
{
  struct AHIDevUnit *iounit;
  ULONG error = 0;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    KPrintF("CMD_WRITE\n");
  }

  iounit = (struct AHIDevUnit *) ioreq->ahir_Std.io_Unit;

  ioreq->ahir_Std.io_Actual = 0;

  /* Start playback if neccessary */
  if( ! iounit->IsPlaying)
  {
    if( (! iounit->FullDuplex) && iounit->IsRecording)
    {
      error = AHIE_HALFDUPLEX;   // FIXIT!
    }
    else
    { static const Tag tags[] = { AHIC_Play,TRUE,TAG_DONE };
      error = AHI_ControlAudioA(iounit->AudioCtrl, (struct TagItem *)tags);
    }

    if( ! error)
    {
      iounit->IsPlaying = TRUE;
    }
  }

  ioreq->ahir_Extras = (IPTR) AllocVec(sizeof(struct Extras), MEMF_PUBLIC|MEMF_CLEAR);

  if(ioreq->ahir_Extras == 0)
  {
    error = AHIE_NOMEM;
  }
  else
  {
    // Initialize the structure
    GetExtras(ioreq)->Channel   = NOCHANNEL;
    GetExtras(ioreq)->Sound     = AHI_NOSOUND;
    GetExtras(ioreq)->VolumeScale = 0x10000;
  }

  if(iounit->IsPlaying && !error)
  {
    // Convert length in bytes to length in samples

    ioreq->ahir_Std.io_Length /= AHI_SampleFrameSize(ioreq->ahir_Type);

    NewWriter(ioreq, iounit, AHIBase);
  }

  if(error)
  {
    ioreq->ahir_Std.io_Error = error;
    TermIO(ioreq, AHIBase);
  }
}


/******************************************************************************
** StartCmd *******************************************************************
******************************************************************************/

/****** ahi.device/CMD_START ************************************************
*
*   NAME
*       CMD_START -- start device processing (like ^Q) (V4)
*
*   FUNCTION
*       All CMD_WRITE's that has been sent to the device since CMD_STOP
*       will be started at once, synchronized.
*
*   IO REQUEST INPUT
*       io_Device       Preset by the call to OpenDevice().
*       io_Unit         Preset by the call to OpenDevice().
*       io_Command      CMD_START
*
*   IO REQUEST RESULT
*       io_Error        0 for success, or an error code as defined in
*                       <ahi/devices.h> and <exec/errors.h>.
*
*       The other fields, except io_Device, io_Unit and io_Command, are
*       trashed.
*
*   EXAMPLE
*
*   NOTES
*       Unlike most (all?) other devices, CMD_STOP and CMD_START do nest in
*       ahi.device.
*
*   BUGS
*
*   SEE ALSO
*       CMD_STOP, <ahi/devices.h>, <exec/errors.h>
*
****************************************************************************
*
*/

static void
StartCmd ( struct AHIRequest *ioreq,
           struct AHIBase *AHIBase )
{
  struct AHIDevUnit *iounit;
  struct AHIPrivAudioCtrl *audioctrl;
  struct Library *AHIsubBase;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    KPrintF("CMD_START\n");
  }

  iounit = (struct AHIDevUnit *) ioreq->ahir_Std.io_Unit;
  audioctrl = (struct AHIPrivAudioCtrl *) iounit->AudioCtrl;

  AHIObtainSemaphore(&iounit->Lock);

  if(iounit->StopCnt)
  {
    iounit->StopCnt--;

    if((AHIsubBase = audioctrl->ahiac_SubLib))
    {
      if(iounit->StopCnt == 0)
      {
        struct AHIRequest* ior;
        LONG               old_pri;

        // Boost us some, so we don't spend too much time in 
        // audio disabled state (we could miss an interrupt!)
        // and disable audio interrupts.

        old_pri = SetTaskPri( FindTask( NULL ), 128 );
        AHIsub_Disable((struct AHIAudioCtrlDrv *) audioctrl);
//Disable();

        // Now "start" all sounds (they won't really start until 
        // the audio interrupts are enabled).

        while((ior = (struct AHIRequest *) RemHead(
            (struct List *) &iounit->RequestQueue)))
        {
          WriteCmd(ior, AHIBase);
        }

//Enable();
        AHIsub_Enable((struct AHIAudioCtrlDrv *) audioctrl);
        SetTaskPri( FindTask( NULL ), old_pri );
      }
    }
  }
  else
  {
    ioreq->ahir_Std.io_Error = AHIE_UNKNOWN;
  }

  AHIReleaseSemaphore(&iounit->Lock);

  TermIO(ioreq,AHIBase);
}


/******************************************************************************
** FeedReaders ****************************************************************
******************************************************************************/

// This function is called by DevProc or ReadCmd to scan the list of waiting
// readers, and fill their buffers. When a buffer is full, the IORequest is
// terminated.

void
FeedReaders ( struct AHIDevUnit *iounit,
              struct AHIBase *AHIBase )
{
  struct AHIRequest *ioreq;

  AHIObtainSemaphore(&iounit->Lock);

  for(ioreq = (struct AHIRequest *)iounit->ReadList.mlh_Head;
      ioreq->ahir_Std.io_Message.mn_Node.ln_Succ;
      ioreq = (struct AHIRequest *)ioreq->ahir_Std.io_Message.mn_Node.ln_Succ)
  {
    FillReadBuffer(ioreq, iounit, AHIBase);
  }

  // Check if Reader-list is empty. If so, stop recording (after a small delay).

  if( ! iounit->ReadList.mlh_Head->mln_Succ )
  {
    if(--iounit->RecordOffDelay == 0)
    { static const Tag tags[] = { AHIC_Record,FALSE,TAG_DONE };
      AHI_ControlAudioA(iounit->AudioCtrl, (struct TagItem *)tags);
      iounit->IsRecording = FALSE;
    }
  }
  else
  {
    iounit->RecordOffDelay = 2;
  }

  AHIReleaseSemaphore(&iounit->Lock);
}


/******************************************************************************
** FillReadBuffer *************************************************************
******************************************************************************/

// Handles a read request. Note that the request MUST be in a list, and the
// list must be semaphore locked!

static void 
FillReadBuffer ( struct AHIRequest *ioreq,
                 struct AHIDevUnit *iounit,
                 struct AHIBase *AHIBase )
{
  ULONG length,length2;
  APTR  oldaddress;
  BOOL  remove;

  if(iounit->ValidRecord) // Make sure we have a valid source buffer
  {
    oldaddress = ioreq->ahir_Std.io_Data;

    length = (ioreq->ahir_Std.io_Length - ioreq->ahir_Std.io_Actual)
             / AHI_SampleFrameSize(ioreq->ahir_Type);

    length2 = (iounit->RecordSize - ioreq->ahir_Std.io_Offset)
              / AHI_SampleFrameSize(AHIST_S16S);
    length2 = MultFixed(length2, (Fixed) ioreq->ahir_Frequency);

    if(length <= length2)
    {
      remove=TRUE;
    }
    else
    {
      length = length2;
      remove = FALSE;
    }

    switch (ioreq->ahir_Type)
    {
      case AHIST_M8S:
        RecM8S(length,ioreq->ahir_Frequency,
            iounit->RecordBuffer,
            &ioreq->ahir_Std.io_Offset,
            &ioreq->ahir_Std.io_Data);
        break;
      case AHIST_S8S:
        RecS8S(length,ioreq->ahir_Frequency,
            iounit->RecordBuffer,
            &ioreq->ahir_Std.io_Offset,
            &ioreq->ahir_Std.io_Data);
        break;
      case AHIST_M16S:
        RecM16S(length,ioreq->ahir_Frequency,
            iounit->RecordBuffer,
            &ioreq->ahir_Std.io_Offset,
            &ioreq->ahir_Std.io_Data);
        break;
      case AHIST_S16S:
        RecS16S(length,ioreq->ahir_Frequency,
            iounit->RecordBuffer,
            &ioreq->ahir_Std.io_Offset,
            &ioreq->ahir_Std.io_Data);
        break;
      case AHIST_M32S:
        RecM32S(length,ioreq->ahir_Frequency,
            iounit->RecordBuffer,
            &ioreq->ahir_Std.io_Offset,
            &ioreq->ahir_Std.io_Data);
        break;
      case AHIST_S32S:
        RecS32S(length,ioreq->ahir_Frequency,
            iounit->RecordBuffer,
            &ioreq->ahir_Std.io_Offset,
            &ioreq->ahir_Std.io_Data);
        break;
      default:
        ioreq->ahir_Std.io_Error = AHIE_BADSAMPLETYPE;
        remove = TRUE;
        break;
    }
    
    ioreq->ahir_Std.io_Actual += ((IPTR) ioreq->ahir_Std.io_Data - (IPTR) oldaddress);

    if(remove)
    {
      Remove((struct Node *) ioreq);
      TermIO(ioreq, AHIBase);
    }
    else
    {
      ioreq->ahir_Std.io_Offset = 0;
    }
  }
  else
  {
    ioreq->ahir_Std.io_Offset = 0;
  }
}


/******************************************************************************
** NewWriter ******************************************************************
******************************************************************************/

// This function is called by WriteCmd when a new write request comes.

static void
NewWriter ( struct AHIRequest *ioreq,
            struct AHIDevUnit *iounit,
            struct AHIBase *AHIBase )
{
  int channel, sound;
  BOOL delay = FALSE;
  struct AHISampleInfo si;
  struct Library *AHIsubBase;

  AHIsubBase = ((struct AHIPrivAudioCtrl *) iounit->AudioCtrl)->ahiac_SubLib;

  si.ahisi_Type    = ioreq->ahir_Type;
  si.ahisi_Address = ioreq->ahir_Std.io_Data;
  si.ahisi_Length  = ioreq->ahir_Std.io_Length;

  // Load the sound
  
  for(sound = 0; sound < MAXSOUNDS; sound++)
  {
    if(iounit->Sounds[sound] == SOUND_FREE)
    {
      iounit->Sounds[sound] = SOUND_IN_USE;
      break;
    }
  }

  if((sound < MAXSOUNDS) &&
     (AHI_LoadSound(sound, AHIST_DYNAMICSAMPLE, &si, iounit->AudioCtrl)
      == AHIE_OK)) {

    GetExtras(ioreq)->Sound = sound;

    AHIObtainSemaphore(&iounit->Lock);
  
    if(ioreq->ahir_Link)
    {
      // See if the linked request is playing, silent or waiting...
  
      if(FindNode((struct List *) &iounit->PlayingList,
          (struct Node *) ioreq->ahir_Link))
      {
        delay = TRUE;
      }
      else if(FindNode((struct List *) &iounit->SilentList,
          (struct Node *) ioreq->ahir_Link))
      {
        delay = TRUE;
      }
      else if(FindNode((struct List *) &iounit->WaitingList,
          (struct Node *) ioreq->ahir_Link))
      {
        delay = TRUE;
      }
    }

  // NOTE: ahir_Link changes direction here. When the user set's it, she makes a new
  // request point to an old. We let the old point to the next (that's more natural,
  // anyway...) It the user tries to link more than one request to another, we fail.
  
    if(delay)
    {
      if( ! ioreq->ahir_Link->ahir_Link)
      {
        struct AHIRequest *otherioreq = ioreq->ahir_Link;
      
        channel = GetExtras( otherioreq )->Channel;
        GetExtras(ioreq)->Channel = NOCHANNEL;
  
        otherioreq->ahir_Link = ioreq;
        ioreq->ahir_Link = NULL;
        Enqueue((struct List *) &iounit->WaitingList,(struct Node *) ioreq);
  
        if(channel != NOCHANNEL)
        {
          // Attach the request to the currently playing one

          AHIsub_Disable((struct AHIAudioCtrlDrv *) iounit->AudioCtrl);

          // Make SURE the current sound isn't already finished!
          
          if(otherioreq->ahir_Std.io_Command == AHICMD_WRITTEN)
          {
            AHIsub_Enable((struct AHIAudioCtrlDrv *) iounit->AudioCtrl);

            // OOPS! It's finished! Undo...
            Remove((struct Node *) ioreq);
            
            // Start sound as if it wasn't delayed (see below);
            AddWriter(ioreq, iounit, AHIBase);
          }
          else
          {
            if( iounit->Voices[channel].Flags & VF_STARTED )
            {
              // There is a sound already playing. Attach this sound
              // after the current.

              iounit->Voices[channel].QueuedRequest = ioreq;
              iounit->Voices[channel].NextOffset    = PLAY;
              iounit->Voices[channel].NextRequest   = NULL;

              AHI_Play(iounit->AudioCtrl,
                  AHIP_BeginChannel,  channel,
                  AHIP_LoopFreq,      ioreq->ahir_Frequency,
                  AHIP_LoopVol,       (ULONG) (((long long) ioreq->ahir_Volume *
					GetExtras(ioreq)->VolumeScale ) >> 16),
                  AHIP_LoopPan,       ioreq->ahir_Position,
                  AHIP_LoopSound,     GetExtras(ioreq)->Sound,
                  AHIP_LoopOffset,    ioreq->ahir_Std.io_Actual,
                  AHIP_LoopLength,    ioreq->ahir_Std.io_Length - 
                                      ioreq->ahir_Std.io_Actual,
                  AHIP_EndChannel,    0,
                  TAG_DONE);
            }
            else
            {
              // The current sound has not yet been started, and the loop
              // part is not set either. Let the SoundFunc() handle the
              // attaching.

              iounit->Voices[channel].NextSound     = GetExtras( ioreq )->Sound;
              iounit->Voices[channel].NextVolume    = ioreq->ahir_Volume;
              iounit->Voices[channel].NextPan       = ioreq->ahir_Position;
              iounit->Voices[channel].NextFrequency = ioreq->ahir_Frequency;
              iounit->Voices[channel].NextOffset    = ioreq->ahir_Std.io_Actual;
              iounit->Voices[channel].NextLength    = ioreq->ahir_Std.io_Length -
                                                      ioreq->ahir_Std.io_Actual;
              iounit->Voices[channel].NextRequest   = ioreq;
            }

            AHIsub_Enable((struct AHIAudioCtrlDrv *) iounit->AudioCtrl);
          }
        }
      }
      else // She tried to add more than one request to another one
      { 
        ioreq->ahir_Std.io_Error = AHIE_UNKNOWN;
        TermIO(ioreq, AHIBase);
      }
    }
    else // Sound is not delayed
    {
      ioreq->ahir_Link=NULL;
      AddWriter(ioreq, iounit, AHIBase);
    }

    AHIReleaseSemaphore(&iounit->Lock);
  }
  else // No free sound found, or sound failed to load
  {
    if( sound < MAXSOUNDS )
    {
      // Clean up and set error code
      iounit->Sounds[sound]    = SOUND_FREE;
      ioreq->ahir_Std.io_Error = AHIE_BADSAMPLETYPE;
    }
    else
    {
      ioreq->ahir_Std.io_Error = AHIE_UNKNOWN;
    }

    TermIO(ioreq, AHIBase);
  }
}


/******************************************************************************
** AddWriter ******************************************************************
******************************************************************************/

// This function is called by NewWriter and RethinkPlayers. It adds an
// initialized request to either the playing or waiting list, and starts
// the sound it if possible

static void
AddWriter ( struct AHIRequest *ioreq,
            struct AHIDevUnit *iounit,
            struct AHIBase *AHIBase )
{
  int channel;

  // Search for a free channel, and use if found

  for(channel = 0; channel < iounit->Channels; channel++)
  {
    if(iounit->Voices[channel].NextOffset == (ULONG) FREE)
    {
      Enqueue((struct List *) &iounit->PlayingList,(struct Node *) ioreq);
      UpdateMasterVolume( iounit, AHIBase );
      PlayRequest(channel, ioreq, iounit, AHIBase);
      break;
    }
  }


  if(channel == iounit->Channels)
  {
    struct AHIRequest *ioreq2;

    // No free channel found. Check if we can kick the last one out...
    // The last one, if it exists, has lowest priority.
    //
    // Note that it is quite possible that there is no request in the list,
    // even though there was no free sound channel. This can happen if
    // AbortIO() has been called, and marked the channel MUTE, but the
    // SoundFunc() has yet not been called to move the chennel into the
    // FREE state.

    ioreq2 = (struct AHIRequest *) iounit->PlayingList.mlh_TailPred; 

    if( ( iounit->PlayingList.mlh_Head->mln_Succ != NULL ) &&
        ( ioreq->ahir_Std.io_Message.mn_Node.ln_Pri >
          ioreq2->ahir_Std.io_Message.mn_Node.ln_Pri ) )
    {
      // Let's steal his place!

      RemTail((struct List *) &iounit->PlayingList);
      channel = GetExtras(ioreq2)->Channel;
      GetExtras(ioreq2)->Channel = NOCHANNEL;
      Enqueue((struct List *) &iounit->SilentList,(struct Node *) ioreq2);
      Enqueue((struct List *) &iounit->PlayingList,(struct Node *) ioreq);
      PlayRequest(channel, ioreq, iounit, AHIBase);
    }
    else
    {
      // Let's be quiet for a while.
      GetExtras(ioreq)->Channel = NOCHANNEL;
      Enqueue((struct List *) &iounit->SilentList,(struct Node *) ioreq);
    }
  }
}


/******************************************************************************
** PlayRequest ****************************************************************
******************************************************************************/

// This begins to play an AHIRequest (starting at sample io_Actual).

static void
PlayRequest ( int channel,
              struct AHIRequest *ioreq,
              struct AHIDevUnit *iounit,
              struct AHIBase *AHIBase )
{
  struct Library *AHIsubBase;

  AHIsubBase = ((struct AHIPrivAudioCtrl *) iounit->AudioCtrl)->ahiac_SubLib;

  // Start the sound

  GetExtras(ioreq)->Channel = channel;

  if(ioreq->ahir_Link)
  {
    struct Voice        *v = &iounit->Voices[channel];
    struct AHIRequest   *r = ioreq->ahir_Link;

    v->NextSound     = GetExtras(r)->Sound;
    v->NextVolume    = r->ahir_Volume;
    v->NextPan       = r->ahir_Position;
    v->NextFrequency = r->ahir_Frequency;
    v->NextOffset    = r->ahir_Std.io_Actual;
    v->NextLength    = r->ahir_Std.io_Length
                     - r->ahir_Std.io_Actual;
    v->NextRequest   = r;
  }
  else
  {
    iounit->Voices[channel].NextOffset  = PLAY;
    iounit->Voices[channel].NextRequest = NULL;
  }

  AHIsub_Disable((struct AHIAudioCtrlDrv *) iounit->AudioCtrl);

  iounit->Voices[channel].PlayingRequest = NULL;
  iounit->Voices[channel].QueuedRequest = ioreq;
  iounit->Voices[channel].Flags &= ~VF_STARTED;

  AHI_Play(iounit->AudioCtrl,
      AHIP_BeginChannel,  channel,
      AHIP_Freq,          ioreq->ahir_Frequency,
      AHIP_Vol,           (ULONG) (((long long) ioreq->ahir_Volume *
				    GetExtras(ioreq)->VolumeScale) >> 16),
      AHIP_Pan,           ioreq->ahir_Position,
      AHIP_Sound,         GetExtras(ioreq)->Sound,
      AHIP_Offset,        ioreq->ahir_Std.io_Actual,
      AHIP_Length,        ioreq->ahir_Std.io_Length-ioreq->ahir_Std.io_Actual,
      AHIP_EndChannel,    0,
      TAG_DONE);

  AHIsub_Enable((struct AHIAudioCtrlDrv *) iounit->AudioCtrl);

#if 0
  // This is a workaround for a race condition.
  // The problem can occur if a delayed request follows immediately after
  // this one, before the sample interrupt routine has been called, and
  // overwrites QueuedRequest. The result is that this sound is never
  // marked as finished, and the application will wait forever on the
  // IO Request. Quite ugly, no?

  Wait(1L << iounit->SampleSignal);

  // Set signal again...
  Signal((struct Task *) iounit->Master, (1L << iounit->SampleSignal));
  
//  while(((volatile UBYTE) (iounit->Voices[channel].Flags) & VF_STARTED) == 0);
#endif
}


/******************************************************************************
** RethinkPlayers *************************************************************
******************************************************************************/

// When a playing sample has reached the end, this function is called.
// It finds and terminates all finished requests, and moves their 'children'
// from the waiting list.
// Then it tries to restart all silent sounds.

void
RethinkPlayers ( struct AHIDevUnit *iounit,
                 struct AHIBase *AHIBase )
{
  struct MinList templist;
  struct AHIRequest *ioreq;

  NewList((struct List *) &templist);

  AHIObtainSemaphore(&iounit->Lock);

  RemPlayers((struct List *) &iounit->PlayingList, iounit, AHIBase);
  RemPlayers((struct List *) &iounit->SilentList, iounit, AHIBase);

  // Move all silent requests to our temporary list

  while((ioreq = (struct AHIRequest *) RemHead((struct List *) &iounit->SilentList)))
  {
    AddTail((struct List *) &templist, (struct Node *) ioreq);
  }

  // And add them back...
  while((ioreq = (struct AHIRequest *) RemHead((struct List *) &templist)))
  {
    AddWriter(ioreq, iounit, AHIBase);
  }

  AHIReleaseSemaphore(&iounit->Lock);
}


/******************************************************************************
** RemPlayers *****************************************************************
******************************************************************************/

// Removes all finished play requests from a list. The lists must be locked!

static void
RemPlayers ( struct List *list,
             struct AHIDevUnit *iounit,
             struct AHIBase *AHIBase )
{
  struct AHIRequest *ioreq, *node;

  node = (struct AHIRequest *) list->lh_Head;

  while(node->ahir_Std.io_Message.mn_Node.ln_Succ)
  {
    ioreq = node;
    node = (struct AHIRequest *) node->ahir_Std.io_Message.mn_Node.ln_Succ;

    if(ioreq->ahir_Std.io_Command == AHICMD_WRITTEN)
    {
      Remove((struct Node *) ioreq);

      if(ioreq->ahir_Link)
      {
        // Move the attached one to the list
        Remove((struct Node *) ioreq->ahir_Link);

	// FIXME: 2002-10-13: I have a bug report that claims
	// GetExtras(ioreq->ahir_Link) returns NULL here. How did that
	// happen??
	
	// FIXED: 2005-09-26: The app AbortIO()'ed a request that was
	// attached to another request using ahir_Link. When we then
	// arrived here, ioreq->ahir_Link would point to a terminated
	// request and possibly even deallocated memory. Now AbortIO()
	// clears ahir_Link.

        GetExtras(ioreq->ahir_Link)->Channel = GetExtras(ioreq)->Channel;
        Enqueue(list, (struct Node *) ioreq->ahir_Link);
        // We have to go through the whole procedure again, in case
        // the child is finished, too.
        node = (struct AHIRequest *) list->lh_Head;
      }

      ioreq->ahir_Std.io_Error = AHIE_OK;
      ioreq->ahir_Std.io_Command = CMD_WRITE;
      ioreq->ahir_Std.io_Actual = ioreq->ahir_Std.io_Length;
      TermIO(ioreq, AHIBase);
    }
  }
}


/******************************************************************************
** UpdateSilentPlayers ********************************************************
******************************************************************************/

// Updates the io_Actual field of all silent requests. The lists must be locked.
// This function is either called from the interrupt or DevProc.

void 
UpdateSilentPlayers ( struct AHIDevUnit *iounit,
                      struct AHIBase *AHIBase )
{
  struct AHIRequest *ioreq;

  for(ioreq = (struct AHIRequest *)iounit->SilentList.mlh_Head;
      ioreq->ahir_Std.io_Message.mn_Node.ln_Succ;
      ioreq = (struct AHIRequest *)ioreq->ahir_Std.io_Message.mn_Node.ln_Succ)

  {
    // Update io_Actual
    ioreq->ahir_Std.io_Actual += ((ioreq->ahir_Frequency << 14) / PLAYERFREQ) >> 14;

    // Check if the whole sample has been "played"
    if(ioreq->ahir_Std.io_Actual >= ioreq->ahir_Std.io_Length)
    {
      // Mark request as finished
      ioreq->ahir_Std.io_Command = AHICMD_WRITTEN;

      // Make us call Rethinkplayers later
      Signal((struct Task *) iounit->Master, (1L << iounit->SampleSignal));
    }
  }
}


/******************************************************************************
** UpdateMasterVolume  ********************************************************
******************************************************************************/

// Updated the master volume so all sounds are played as loud as possible
// without risking clipping.
// This function is called from AddWriter() and TermIO().

static void UpdateMasterVolume( struct AHIDevUnit *iounit,
				struct AHIBase    *AHIBase )
{
  struct AHIRequest* ioreq1;
  struct AHIRequest* ioreq2;
  struct Library*    AHIsubBase;

  AHIsubBase = ((struct AHIPrivAudioCtrl *) iounit->AudioCtrl)->ahiac_SubLib;

  AHIObtainSemaphore(&iounit->Lock);

  for(ioreq1 = (struct AHIRequest*) iounit->PlayingList.mlh_Head;
      ioreq1->ahir_Std.io_Message.mn_Node.ln_Succ;
      ioreq1 = (struct AHIRequest*) ioreq1->ahir_Std.io_Message.mn_Node.ln_Succ)
  {
    ULONG id     = ioreq1->ahir_Private[1];
    int   c      = 0;
    LONG  minscale = 0x10000;

/*     KPrintF( "Checking id %08lx on request %08lx... ", id, ioreq1 ); */
    
    for(ioreq2 = (struct AHIRequest*) iounit->PlayingList.mlh_Head;
	ioreq2->ahir_Std.io_Message.mn_Node.ln_Succ;
	ioreq2 = (struct AHIRequest*) ioreq2->ahir_Std.io_Message.mn_Node.ln_Succ)
    {
      if( ioreq2->ahir_Private[1] == id )
      {
	++c;

	if( GetExtras(ioreq2) && GetExtras(ioreq2)->VolumeScale < minscale )
	{
	  minscale = GetExtras(ioreq2)->VolumeScale;
	}
      }
    }
    
    if( minscale > 0x10000 / c )
    {
      minscale = 0x10000 / c;
    }

    switch( AHIBase->ahib_ScaleMode )
    {
      case AHI_SCALE_DYNAMIC_SAFE:
	if( GetExtras(ioreq1)->VolumeScale > minscale )
	{
	  GetExtras(ioreq1)->VolumeScale = minscale;
	}
	break;

      case AHI_SCALE_FIXED_SAFE:
      case AHI_SCALE_FIXED_0_DB:
      case AHI_SCALE_FIXED_3_DB:
      case AHI_SCALE_FIXED_6_DB:
	GetExtras(ioreq1)->VolumeScale = 0x10000;
	break;
    }

/*     KPrintF( "%ld requests, maxdiv = %ld -> Vol %05lx => %05lx\n", */
/* 	     c, maxdiv, ioreq1->ahir_Volume, */
/* 	     ioreq1->ahir_Volume / GetExtras(ioreq1)->VolumeDiv ); */
  }

  // Now update the volume as quickly as possible ...

  AHIsub_Disable((struct AHIAudioCtrlDrv *) iounit->AudioCtrl);
  
  for(ioreq1 = (struct AHIRequest*) iounit->PlayingList.mlh_Head;
      ioreq1->ahir_Std.io_Message.mn_Node.ln_Succ;
      ioreq1 = (struct AHIRequest*) ioreq1->ahir_Std.io_Message.mn_Node.ln_Succ)
  {
    if( GetExtras(ioreq1)->Channel != NOCHANNEL )
    {
      AHI_SetVol( GetExtras(ioreq1)->Channel,
		  (ULONG) (((long long) ioreq1->ahir_Volume *
			    GetExtras(ioreq1)->VolumeScale) >> 16),
		  ioreq1->ahir_Position,
		  iounit->AudioCtrl,
		  AHISF_IMM );
    }
  } 

  AHIsub_Enable((struct AHIAudioCtrlDrv *) iounit->AudioCtrl);

  // And now the real master volume ...

  if( iounit->Unit.unit_OpenCnt == 0 )
  {
    struct AHIEffMasterVolume vol = {
      AHIET_MASTERVOLUME | AHIET_CANCEL,
      0x10000
    };
      
    AHI_SetEffect( &vol, iounit->AudioCtrl );
  }
  else
  {
    struct AHIEffMasterVolume vol = {
      AHIET_MASTERVOLUME,
      0x10000
    };
    
    switch( AHIBase->ahib_ScaleMode )
    {
      case AHI_SCALE_FIXED_SAFE:
	vol.ahiemv_Volume = 0x10000;
	break;
      
      case AHI_SCALE_DYNAMIC_SAFE:
	vol.ahiemv_Volume = iounit->Channels * 0x10000 / iounit->Unit.unit_OpenCnt;
	break;

      case AHI_SCALE_FIXED_0_DB:
	vol.ahiemv_Volume = iounit->Channels * 0x10000;
	break;
      
      case AHI_SCALE_FIXED_3_DB:
	vol.ahiemv_Volume = iounit->Channels * 0xB505;
	break;
	
      case AHI_SCALE_FIXED_6_DB:
	vol.ahiemv_Volume = iounit->Channels * 0x8000;
	break;
    }
      
    if( iounit->PseudoStereo )
    {
      vol.ahiemv_Volume = vol.ahiemv_Volume / 2;
    }

    AHI_SetEffect( &vol, iounit->AudioCtrl );
  }
  
  AHIReleaseSemaphore(&iounit->Lock);
}
