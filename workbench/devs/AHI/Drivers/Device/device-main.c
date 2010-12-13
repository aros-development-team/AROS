
/*
 *  This is a driver for ahi.device loopback sound
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

#define dd ((struct DeviceData*) AudioCtrl->ahiac_DriverData)

void
SlaveEntry( void );

PROCGW( static, void,  slaveentry, SlaveEntry );

static const LONG frequencies[] =
{
  8000,     // µ- and A-Law (telephone)
  11025,    // CD/4
  22050,    // CD/2
  44100,    // CD
  48000,    // DAT
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
  struct DeviceBase* DeviceBase = (struct DeviceBase*) AHIsubBase;
  int freq = AudioCtrl->ahiac_MixFreq;
  
  AudioCtrl->ahiac_DriverData = AllocVec( sizeof( struct DeviceData ),
		 MEMF_CLEAR | MEMF_PUBLIC );

  if( dd != NULL )
  {
    dd->slavesignal      = -1;
    dd->mastersignal     = AllocSignal( -1 );
    dd->mastertask       = (struct Process*) FindTask( NULL );
    dd->ahisubbase       = DeviceBase;
    dd->unit             = (GetTagData( AHIDB_AudioID, 0, taglist) & 0x0000f000) >> 12;

  }
  else
  {
    return AHISF_ERROR;
  }

  if( dd->mastersignal == -1 )
  {
    return AHISF_ERROR;
  }


  // TODO open ahi.device
  
  AudioCtrl->ahiac_MixFreq = freq;
  
  return ( AHISF_KNOWHIFI | 
	   AHISF_KNOWSTEREO |
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
  struct DeviceBase* DeviceBase = (struct DeviceBase*) AHIsubBase;

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
  struct DeviceBase* DeviceBase = (struct DeviceBase*) AHIsubBase;

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
  struct DeviceBase* DeviceBase = (struct DeviceBase*) AHIsubBase;

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
  struct DeviceBase* DeviceBase = (struct DeviceBase*) AHIsubBase;

  AHIsub_Stop( flags, AudioCtrl );

  if(flags & AHISF_PLAY)
  {
    int freq = AudioCtrl->ahiac_MixFreq;

    struct TagItem proctags[] =
      {
	{ NP_Entry,     (IPTR)&slaveentry },
	{ NP_Name,      (IPTR)LibName     },
	{ NP_Priority,  127               },
	{ TAG_DONE,     0                 }
      };

    
    dd->mixbuffers[ 0 ] = AllocVec( AudioCtrl->ahiac_BuffSize,
				    MEMF_ANY | MEMF_CLEAR | MEMF_PUBLIC );

    if( dd->mixbuffers[ 0 ] == NULL ) return AHIE_NOMEM;
    
    dd->mixbuffers[ 1 ] = AllocVec( AudioCtrl->ahiac_BuffSize,
				    MEMF_ANY | MEMF_CLEAR | MEMF_PUBLIC );
    
    if( dd->mixbuffers[ 1 ] == NULL ) return AHIE_NOMEM;
    
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
  struct DeviceBase* DeviceBase = (struct DeviceBase*) AHIsubBase;

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
  struct DeviceBase* DeviceBase = (struct DeviceBase*) AHIsubBase;

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

    FreeVec( dd->mixbuffers[ 0 ] );
    FreeVec( dd->mixbuffers[ 1 ] );
    dd->mixbuffers[ 0 ] = NULL;
    dd->mixbuffers[ 1 ] = NULL;
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
  struct DeviceBase* DeviceBase = (struct DeviceBase*) AHIsubBase;
  size_t i;

  switch( attribute )
  {
    case AHIDB_Bits:
      return 16;

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
      return TRUE;

    case AHIDB_Realtime:
      return TRUE;

    case AHIDB_Outputs:
      return 1;

    case AHIDB_Output:
      return (IPTR) "Device";    // We have only one "output"!

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
  struct DeviceBase* DeviceBase = (struct DeviceBase*) AHIsubBase;

  return 0;
}
