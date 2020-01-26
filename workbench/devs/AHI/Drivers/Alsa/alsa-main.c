#include <aros/debug.h>
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
#include "alsa-bridge/alsa.h"

#define dd ((struct AlsaData*) AudioCtrl->ahiac_DriverData)

void
SlaveEntry( void );

PROCGW( static, void,  slaveentry, SlaveEntry );

static const LONG frequencies[] =
{
  8000,     // µ- and A-Law (telephone)
  11025,    // CD/4
  22050,    // CD/2
  44100,    // CD
  48000     // DAT
};

#define FREQUENCIES (sizeof frequencies / sizeof frequencies[ 0 ])

static const ULONG table_5bit[] = {
  0xb53c,
  0x804e,
  0x5ad5,
  0x404e,
  0x2d86,
  0x203a,
  0x16d1,
  0x1027,
  0x0b6f,
  0x0818,
  0x05bb,
  0x040f,
  0x02df,
  0x0209,
  0x0171,
  0x0105,
  0x00b9,
  0x0083,
  0x005d,
  0x0042,
  0x002e,
  0x0021,
  0x0017,
  0x0010,
  0x000c,
  0x0008,
  0x0006,
  0x0004,
  0x0003,
  0x0002,
  0x0001,
  0x0000
};

static UWORD LinToLog(ULONG vol)
{
  int i;

  if (!vol) return 0x20;

  for (i = 0; i < 32; i++)
  {
    if (vol > table_5bit[i])
    {
      return i;
    }
  }
  return 0x1f;
}

static ULONG LogToLin(UWORD i)
{
    if (i > 31) return 0x10000;
    return table_5bit[i];
}

/******************************************************************************
** AHIsub_AllocAudio **********************************************************
******************************************************************************/

ULONG
_AHIsub_AllocAudio( struct TagItem*         taglist,
            struct AHIAudioCtrlDrv* AudioCtrl,
            struct DriverBase*      AHIsubBase )
{
  struct AlsaBase* AlsaBase = (struct AlsaBase*) AHIsubBase;
  ULONG freq = AudioCtrl->ahiac_MixFreq;

  D(bug("[Alsa]: AllocAudio enter\n"));

  AudioCtrl->ahiac_DriverData = AllocVec( sizeof( struct AlsaData ),
         MEMF_CLEAR | MEMF_PUBLIC );

  if( dd != NULL )
  {
    dd->slavesignal      = -1;
    dd->mastersignal     = AllocSignal( -1 );
    dd->mastertask       = (struct Process*) FindTask( NULL );
    dd->ahisubbase       = AlsaBase;
  }
  else
  {
    return AHISF_ERROR;
  }

  if( dd->mastersignal == -1 )
  {
    return AHISF_ERROR;
  }

  dd->alsahandle = ALSA_Open();

  if (dd->alsahandle == NULL)
  {
    bug("[Alsa]: Failed opening ALSA\n");
    return AHISF_ERROR;
  }

  if (!ALSA_SetHWParams(dd->alsahandle, &freq))
  {
      bug("[Alsa]: Failed setting ALSA hardware parameters\n");
      ALSA_DropAndClose(dd->alsahandle);
      dd->alsahandle = NULL;
      return AHISF_ERROR;
  }

  AudioCtrl->ahiac_MixFreq = freq;

  D(bug("[Alsa]: AllocAudio completed\n"));

  return ( AHISF_KNOWSTEREO | AHISF_MIXING | AHISF_TIMING );
}


/******************************************************************************
** AHIsub_FreeAudio ***********************************************************
******************************************************************************/

void
_AHIsub_FreeAudio( struct AHIAudioCtrlDrv* AudioCtrl,
           struct DriverBase*      AHIsubBase )
{
  if( AudioCtrl->ahiac_DriverData != NULL )
  {
    ALSA_DropAndClose(dd->alsahandle);
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
  struct AlsaBase* AlsaBase = (struct AlsaBase*) AHIsubBase;

  AHIsub_Stop( flags, AudioCtrl );

  if(flags & AHISF_PLAY)
  {
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

    D(bug("[Alsa]: AHIsub_Start\n"));

    dd->slavetask = CreateNewProc( proctags );

    if( dd->slavetask != NULL )
    {
      dd->slavetask->pr_Task.tc_UserData = AudioCtrl;
      Signal( dd->slavetask, SIGF_SINGLE );
    }

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
  if( flags & AHISF_PLAY )
  {
    if( dd->slavetask != NULL )
    {
      if( dd->slavesignal != -1 )
      {
        Signal( (struct Task*) dd->slavetask,
                1L << dd->slavesignal );         // Kill him!
        D(bug("[Alsa]: AHIsub_Stop\n"));
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
  struct AlsaBase* AlsaBase = (struct AlsaBase*) AHIsubBase;
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
      return (IPTR) "Krzysztof Smiechowicz";

    case AHIDB_Copyright:
      return (IPTR) "APL";

    case AHIDB_Version:
      return (IPTR) LibIDString;

    case AHIDB_Record:
      return FALSE;

    case AHIDB_Realtime:
      return TRUE;

    case AHIDB_Outputs:
      return 1;

    case AHIDB_Output:
      return (IPTR) "Alsa";    // We have only one "output"!

    case AHIDB_MinOutputVolume:
      return 0x10000; /* There is no AROS-side volume control, use host side */

    case AHIDB_MaxOutputVolume:
      if (AlsaBase->al_MixerElem)
        return 0x10000;
      else
        return 0x00000;

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
  struct AlsaBase* AlsaBase = (struct AlsaBase*) AHIsubBase;

  switch(attribute)
  {
  case AHIC_OutputVolume:
    /* Do not modify host volume from AROS side. Volume value from preferences
     * will be different than current hosts and will cause unwanted volume
     * change in host master volume
     */
    /* if (AlsaBase->al_MixerElem)
    {
        LONG val = (0x20 - LinToLog(argument)) * AlsaBase->al_MaxVolume / 0x20;
        ALSA_MixerSetVolume(AlsaBase->al_MixerElem, (LONG)val);
    } */

    return TRUE;

  case AHIC_OutputVolume_Query:
    if (AlsaBase->al_MixerElem)
    {
      LONG val = ALSA_MixerGetVolume(AlsaBase->al_MixerElem);
      val = val * 0x20 / AlsaBase->al_MaxVolume;
      return LogToLin(0x20 - val);
    }

    return 0;
  }

  return 0;
}
