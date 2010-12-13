
/*
 *  This is a driver for hosted AROS, using oss.library.
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

void
SlaveEntry( void );

PROCGW( static, void,  slaveentry, SlaveEntry );


/*  Since I have no idea what frequencies OSS supports, and there
 *  doesn't seem to be an easy way to find out either, I've just
 *  selected a few common frequencies that are likely to work.
 */

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
  struct AROSBase* AROSBase = (struct AROSBase*) AHIsubBase;
  int freq = AudioCtrl->ahiac_MixFreq;
  
  AudioCtrl->ahiac_DriverData = AllocVec( sizeof( struct AROSData ),
		 MEMF_CLEAR | MEMF_PUBLIC );

#define dd ((struct AROSData*) AudioCtrl->ahiac_DriverData)

  if( dd != NULL )
  {
    dd->slavesignal      = -1;
    dd->mastersignal     = AllocSignal( -1 );
    dd->mastertask       = (struct Process*) FindTask( NULL );
    dd->ahisubbase       = AROSBase;
  }
  else
  {
    return AHISF_ERROR;
  }

  if( dd->mastersignal == -1 )
  {
    return AHISF_ERROR;
  }

  if( ! OSS_Open( "/dev/dsp", FALSE, TRUE, FALSE ) )
  {
    return AHISF_ERROR;
  }

  // 32 fragments times 256 bytes each
    
  if( ! OSS_SetFragmentSize( 32, 8 ) )
  {
    Req( "OSS device does not support a fragment size of 1024." );
    return AHIE_UNKNOWN;
  }
  
  if( ! OSS_FormatSupported_S16LE() || ! OSS_SetFormat_S16LE() )
  {
    Req( "OSS device does not support 16 bit little endian samples!" );
    return AHISF_ERROR;
  }

  if( ! OSS_SetWriteRate( freq, &freq ) )
  {
    Req( "OSS device does not support the requested frequency." );
    return AHISF_ERROR;
  }

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
  struct AROSBase* AROSBase = (struct AROSBase*) AHIsubBase;

  if( AudioCtrl->ahiac_DriverData != NULL )
  {
    OSS_Reset();
    OSS_Close();
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
  struct AROSBase* AROSBase = (struct AROSBase*) AHIsubBase;

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
  struct AROSBase* AROSBase = (struct AROSBase*) AHIsubBase;

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
  struct AROSBase* AROSBase = (struct AROSBase*) AHIsubBase;

  AHIsub_Stop( flags, AudioCtrl );

  if(flags & AHISF_PLAY)
  {
    int freq = AudioCtrl->ahiac_MixFreq;

    struct TagItem proctags[] =
      {
	{ NP_Entry,     (IPTR) &slaveentry },
	{ NP_Name,      (IPTR) LibName     },
	{ NP_Priority,  127                 },
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

    // Since OSS kind of sucks, we have to close and reopen it here ...

    Forbid(); /* To prevent slave task from calling OSS_GetOutputInfo
                 inbetween OSS_Open/OSS_Close */
    OSS_Close();

    if( ! OSS_Open( "/dev/dsp", FALSE, TRUE, FALSE ) )
    {
      Req( "OSS device would not reopen." );
      return AHIE_UNKNOWN;
    }
    Permit();
    
    // 32 fragments times 256 bytes each
    
    if( ! OSS_SetFragmentSize( 32, 8 ) )
    {
      Req( "OSS device does not support a fragment size of 1024." );
      return AHIE_UNKNOWN;
    }

    switch( AudioCtrl->ahiac_BuffType )
    {
      case AHIST_M16S:
      case AHIST_M32S:
	if( ! OSS_SetMono() )
	{
	  Req( "OSS device does not support mono samples." );
	  return AHIE_UNKNOWN;
	}
	break;

      case AHIST_S16S:
      case AHIST_S32S:
	if( ! OSS_SetStereo() )
	{
	  Req( "OSS device does not support stereo samples." );
	  return AHIE_UNKNOWN;
	}
	break;

      default: 
	Req( "Unknown sample format requested: %08lx",
	     AudioCtrl->ahiac_BuffType );
	return AHIE_UNKNOWN;
    }
  
    if( ! OSS_SetFormat_S16LE() )
    {
      Req( "OSS device does not support 16 bit little endian samples!" );
      return AHIE_UNKNOWN;
    }

    if( ! OSS_SetWriteRate( freq, &freq ) )
    {
      Req( "OSS device does not support the requested frequency." );
      return AHIE_UNKNOWN;
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
  struct AROSBase* AROSBase = (struct AROSBase*) AHIsubBase;

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
  struct AROSBase* AROSBase = (struct AROSBase*) AHIsubBase;

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
  struct AROSBase* AROSBase = (struct AROSBase*) AHIsubBase;
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
      return FALSE;

    case AHIDB_Realtime:
      return TRUE;

    case AHIDB_Outputs:
      return 1;

    case AHIDB_Output:
      return (IPTR) "AROS";    // We have only one "output"!

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
  struct AROSBase* AROSBase = (struct AROSBase*) AHIsubBase;

  return 0;
}
