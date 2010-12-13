
/*
 *  This is a do-nothing example AHI driver that just discards the
 *  sound data that is sent to it. Recording is not supported.
 */

#include <config.h>

#include <devices/ahi.h>
#include <dos/dostags.h>
#include <exec/memory.h>
#include <libraries/ahi_sub.h>
#include <proto/ahi_sub.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include <stddef.h>

#include "library.h"
#include "DriverData.h"

#define dd ((struct VoidData*) AudioCtrl->ahiac_DriverData)

void
SlaveEntry( void );

PROCGW( static, void,  slaveentry, SlaveEntry );


/*  There is probably no reason to support all these frequencies. If,
 *  for example, your hardware is locked at 48 kHz, it's ok to only
 *  present one single mixing/recording frequency to the user. If your
 *  hardware has internal resamples and accept any frequency, select a
 *  few common ones.
 */

static const LONG frequencies[] =
{
  5513,	    // CD/8
  8000,     // µ- and A-Law (telephone)
  9600,     // DAT/5
  10000,    // VHS monaural track
  11025,    // CD/4
  12000,    // DAT/4
  14700,    // CD/3
  16000,    // DAT/3, FM/2
  17640,    // CD/2.5
  18900,
  19200,    // DAT/2.5
  20000,    // VHS Hi-Fi/Video track
  22050,    // CD/2
  24000,    // DAT/2
  27429,    // Highest Paula/OCS frequency
  29400,    // CD/1.5
  31968,    // NTSC FM 2-3 pull-down
  32000,    // DAT/1.5
  32032,    // NTSC FM 2-3 pull-up
  33075,
  37800,
  44056,    // NTSC CD 2-3 pull-down
  44100,    // CD
  44144,    // NTSC CD 2-3 pull-up
  47952,    // NTSC DAT 2-3 pull-down
  48000,    // DAT
  48048,    // NTSC DAT 2-3 pull-up
  88200,    // CD*2
  96000     // DAT*2
};

#define FREQUENCIES (sizeof frequencies / sizeof frequencies[ 0 ])

/******************************************************************************
** AHIsub_AllocAudio **********************************************************
******************************************************************************/

ULONG
_AHIsub_AllocAudio( struct TagItem*         taglist,
		    struct AHIAudioCtrlDrv* AudioCtrl,
		    struct DriverBase*      AHIsubBase )
{
  struct VoidBase* VoidBase = (struct VoidBase*) AHIsubBase;

  // NOTE! A Real sound card driver would allocate *and lock* the
  // audio hardware here. If this function gets called a second time,
  // and the hardware is not capable of handling several audio streams
  // at the same time, return AHISF_ERROR now!
  
  AudioCtrl->ahiac_DriverData = AllocVec( sizeof( struct VoidData ),
		 MEMF_CLEAR | MEMF_PUBLIC );

  if( dd != NULL )
  {
    dd->slavesignal      = -1;
    dd->mastersignal     = AllocSignal( -1 );
    dd->mastertask       = (struct Process*) FindTask( NULL );
    dd->ahisubbase       = VoidBase;
  }
  else
  {
    return AHISF_ERROR;
  }

  if( dd->mastersignal == -1 )
  {
    return AHISF_ERROR;
  }

  return ( AHISF_KNOWHIFI | 
	   AHISF_KNOWSTEREO |
	   AHISF_KNOWMULTICHANNEL |
	   AHISF_MIXING |
	   AHISF_TIMING );
}


/******************************************************************************
** AHIsub_FreeAudio ***********************************************************
******************************************************************************/

void
_AHIsub_FreeAudio( struct AHIAudioCtrlDrv* AudioCtrl,
		   struct DriverBase*      AHIsubBase )
{
  struct VoidBase* VoidBase = (struct VoidBase*) AHIsubBase;

  if( AudioCtrl->ahiac_DriverData != NULL )
  {
    FreeSignal( dd->mastersignal );
    FreeVec( AudioCtrl->ahiac_DriverData );
    AudioCtrl->ahiac_DriverData = NULL;
  }
}


/******************************************************************************
** AHIsub_Disable *************************************************************
******************************************************************************/

void
_AHIsub_Disable( struct AHIAudioCtrlDrv* AudioCtrl,
		 struct DriverBase*      AHIsubBase )
{
  struct VoidBase* VoidBase = (struct VoidBase*) AHIsubBase;

  // V6 drivers do not have to preserve all registers

  Forbid();
}


/******************************************************************************
** AHIsub_Enable **************************************************************
******************************************************************************/

void
_AHIsub_Enable( struct AHIAudioCtrlDrv* AudioCtrl,
		struct DriverBase*      AHIsubBase )
{
  struct VoidBase* VoidBase = (struct VoidBase*) AHIsubBase;

  // V6 drivers do not have to preserve all registers

  Permit();
}


/******************************************************************************
** AHIsub_Start ***************************************************************
******************************************************************************/

ULONG
_AHIsub_Start( ULONG                   flags,
	       struct AHIAudioCtrlDrv* AudioCtrl,
	       struct DriverBase*      AHIsubBase )
{
  struct VoidBase* VoidBase = (struct VoidBase*) AHIsubBase;

  AHIsub_Stop( flags, AudioCtrl );

  if(flags & AHISF_PLAY)
  {
    struct TagItem proctags[] =
    {
      { NP_Entry,     (IPTR) &slaveentry },
      { NP_Name,      (IPTR) LibName     },
      { NP_Priority,  -1                  },
      { TAG_DONE,     0                   }
    };
    
    dd->mixbuffer = AllocVec( AudioCtrl->ahiac_BuffSize,
				MEMF_ANY | MEMF_PUBLIC );

    if( dd->mixbuffer == NULL ) return AHIE_NOMEM;

    Forbid();

    dd->slavetask = CreateNewProc( proctags );

    if( dd->slavetask != NULL )
    {
      dd->slavetask->pr_Task.tc_UserData = AudioCtrl;
    }

    Permit();

    if( dd->slavetask != NULL )
    {
      Wait( 1L << dd->mastersignal );  // Wait for slave to come alive

      if( dd->slavetask == NULL )      // Is slave alive or dead?
      {
        return AHIE_UNKNOWN;
      }
    }
    else
    {
      return AHIE_NOMEM;                 // Well, out of memory or whatever...
    }
  }

  if( flags & AHISF_RECORD )
  {
    return AHIE_UNKNOWN;
  }

  return AHIE_OK;
}


/******************************************************************************
** AHIsub_Update **************************************************************
******************************************************************************/

void
_AHIsub_Update( ULONG                   flags,
		struct AHIAudioCtrlDrv* AudioCtrl,
		struct DriverBase*      AHIsubBase )
{
  struct VoidBase* VoidBase = (struct VoidBase*) AHIsubBase;

  // Empty function
}


/******************************************************************************
** AHIsub_Stop ****************************************************************
******************************************************************************/

void
_AHIsub_Stop( ULONG                   flags,
	      struct AHIAudioCtrlDrv* AudioCtrl,
	      struct DriverBase*      AHIsubBase )
{
  struct VoidBase* VoidBase = (struct VoidBase*) AHIsubBase;

  if( flags & AHISF_PLAY )
  {
    if( dd->slavetask != NULL )
    {
      if( dd->slavesignal != -1 )
      {
        Signal( (struct Task*) dd->slavetask,
                1L << dd->slavesignal );         // Kill him!
      }

      Wait( 1L << dd->mastersignal );            // Wait for slave to die
    }

    FreeVec( dd->mixbuffer );
    dd->mixbuffer = NULL;
  }

  if(flags & AHISF_RECORD)
  {
    // Do nothing
  }
}


/******************************************************************************
** AHIsub_GetAttr *************************************************************
******************************************************************************/

IPTR
_AHIsub_GetAttr( ULONG                   attribute,
		 LONG                    argument,
		 IPTR                    def,
		 struct TagItem*         taglist,
		 struct AHIAudioCtrlDrv* AudioCtrl,
		 struct DriverBase*      AHIsubBase )
{
  struct VoidBase* VoidBase = (struct VoidBase*) AHIsubBase;
  size_t i;

  switch( attribute )
  {
    case AHIDB_Bits:
      return 32;

    case AHIDB_Frequencies:
      return FREQUENCIES;

    case AHIDB_Frequency: // Index->Frequency
      return (LONG) frequencies[ argument ];

    case AHIDB_Index: // Frequency->Index
      if( argument <= frequencies[ 0 ] )
      {
        return 0;
      }

      if( argument >= frequencies[ FREQUENCIES - 1 ] )
      {
        return FREQUENCIES - 1;
      }

      for( i = 1; i < FREQUENCIES; i++ )
      {
        if( frequencies[ i ] > argument )
        {
          if( ( argument - frequencies[ i - 1 ] ) <
	      ( frequencies[ i ] - argument ) )
          {
            return i-1;
          }
          else
          {
            return i;
          }
        }
      }

      return 0;  // Will not happen

    case AHIDB_Author:
      return (IPTR) "Martin 'Leviticus' Blom";

    case AHIDB_Copyright:
      return (IPTR) "Public Domain";

    case AHIDB_Version:
      return (IPTR) LibIDString;

    case AHIDB_Record:
      return FALSE;

    case AHIDB_Realtime:
      return TRUE;             // This is not actually true

    case AHIDB_Outputs:
      return 1;

    case AHIDB_Output:
      return (IPTR) "Void";    // We have only one "output"!

    default:
      return def;
  }
}


/******************************************************************************
** AHIsub_HardwareControl *****************************************************
******************************************************************************/

ULONG
_AHIsub_HardwareControl( ULONG                   attribute,
			 LONG                    argument,
			 struct AHIAudioCtrlDrv* AudioCtrl,
			 struct DriverBase*      AHIsubBase )
{
  struct VoidBase* VoidBase = (struct VoidBase*) AHIsubBase;

  return 0;
}
