
#define DEBUG 0
#include <aros/debug.h>

#include <config.h>

#include <devices/ahi.h>
#include <dos/dostags.h>
#include <exec/memory.h>
#include <libraries/ahi_sub.h>
#include <utility/tagitem.h>

#include <proto/ahi_sub.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <stddef.h>

#include <asm/io.h>

#include "library.h"
#include "DriverData.h"

void
SlaveEntry( void );

PROCGW( static, void,  slaveentry, SlaveEntry );


/*  There is probably no reason to support all these frequencies. If,
 *  for example, your hardware is locked at 48 kHz, it's ok to only
 *  present one single mixing/recording frequency to the user. If your
 *  hardware has internal resamples and accepts any frequency, select a
 *  few common ones.
 */

static const LONG frequencies[] =
{
  48000,    // DAT
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
    
    for (i=0; i < 32; i++)
    {
	if (vol > table_5bit[i])
	{
	    return i;
	}
    }
    return 0x1f;
}

static AROS_UFIP(play_int);


/******************************************************************************
** AHIsub_AllocAudio **********************************************************
******************************************************************************/

ULONG
_AHIsub_AllocAudio( struct TagItem*         taglist,
		    struct AHIAudioCtrlDrv* AudioCtrl,
		    struct DriverBase*      AHIsubBase )
{
  struct ac97Base* ac97Base = (struct ac97Base*) AHIsubBase;
 
  AudioCtrl->ahiac_DriverData = AllocVec( sizeof( struct AC97Data ),
		 MEMF_CLEAR | MEMF_PUBLIC );

#define dd ((struct AC97Data *) AudioCtrl->ahiac_DriverData)

D(bug("AHI: AllocAudio: dd=%08x\n", dd));

  if( dd != NULL )
  {
    dd->slavesignal      = -1;
    dd->mastersignal     = AllocSignal( -1 );
    dd->mastertask       = (struct Process*) FindTask( NULL );
    dd->ahisubbase       = ac97Base;
    dd->out_volume	 = 0x10000;
  }
  else
  {
    return AHISF_ERROR;
  }

  {
	dd->irq.is_Node.ln_Type = NT_INTERRUPT;
	dd->irq.is_Node.ln_Pri = 0;
	dd->irq.is_Node.ln_Name = "AHI Int";
	dd->irq.is_Code = play_int;
	dd->irq.is_Data = AudioCtrl;

	AddIntServer(INTB_KERNEL + ac97Base->irq_num, &dd->irq);
  }
  
D(bug("AHI: AllocAudio: Everything OK\n"));
 
  if( dd->mastersignal == -1 )
  {
    return AHISF_ERROR;
  }

  /* Setting the only working frequency for AC97 */
  AudioCtrl->ahiac_MixFreq = 48000;

  return ( AHISF_KNOWSTEREO |
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
  struct ac97Base* ac97Base = (struct ac97Base*) AHIsubBase;

D(bug("AHI: FreeAudio\n"));

  RemIntServer(INTB_KERNEL + ac97Base->irq_num, &dd->irq);

D(bug("AHI: FreeAudio: IRQ removed\n"));

  if( AudioCtrl->ahiac_DriverData != NULL )
  {
    FreeSignal( dd->mastersignal );

D(bug("AHI: FreeAudio: Signal freed\n"));

    FreeVec( AudioCtrl->ahiac_DriverData );

D(bug("AHI: FreeAudio: DriverData freed\n"));

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
  struct ac97Base* ac97Base = (struct ac97Base*) AHIsubBase;

  // V6 drivers do not have to preserve all registers

  Disable();
}


/******************************************************************************
** AHIsub_Enable **************************************************************
******************************************************************************/

void
_AHIsub_Enable( struct AHIAudioCtrlDrv* AudioCtrl,
		struct DriverBase*      AHIsubBase )
{
  struct ac97Base* ac97Base = (struct ac97Base*) AHIsubBase;

  // V6 drivers do not have to preserve all registers

  Enable();
}


/******************************************************************************
** AHIsub_Start ***************************************************************
******************************************************************************/

ULONG
_AHIsub_Start( ULONG                   flags,
	       struct AHIAudioCtrlDrv* AudioCtrl,
	       struct DriverBase*      AHIsubBase )
{
  struct ac97Base* ac97Base = (struct ac97Base*) AHIsubBase;

D(bug("AHI: Start\n"));

  AHIsub_Stop( flags, AudioCtrl );

D(bug("AHI: Start: Stop called\n"));

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

D(bug("AHI: Start: Mixing buffer = %08x\n",dd->mixbuffer));

    if( dd->mixbuffer == NULL ) return AHIE_NOMEM;

    Forbid();

    dd->slavetask = CreateNewProc( proctags );

D(bug("AHI: Start: Slave task = %08x\n",dd->slavetask));

    if( dd->slavetask != NULL )
    {
      dd->slavetask->pr_Task.tc_UserData = AudioCtrl;
    }

D(bug("AHI: Start: Slave task UserData set\n"));

    Permit();

    if( dd->slavetask != NULL )
    {
      Wait( 1L << dd->mastersignal );  // Wait for slave to come alive

D(bug("AHI: Start: Slave task UP and running\n"));

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

D(bug("AHI: Start: Everything OK\n"));

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
  struct ac97Base* ac97Base = (struct ac97Base*) AHIsubBase;

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
  struct ac97Base* ac97Base = (struct ac97Base*) AHIsubBase;

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
  struct ac97Base* ac97Base = (struct ac97Base*) AHIsubBase;
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
      return (IPTR) "Michal Schulz";

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

/*
    case AHIDB_MinMonitorVolume:
      return 0x00000;

    case AHIDB_MaxMonitorVolume:
      return 0x10000;
*/
    case AHIDB_MinOutputVolume:
      return 0x00000;

    case AHIDB_MaxOutputVolume:
      return 0x10000;

    case AHIDB_Output:
      return (IPTR) "Default";    // We have only one "output"!

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
    struct ac97Base* ac97Base = (struct ac97Base*) AHIsubBase;
    UWORD vol;

    switch(attribute)
    {
	case AHIC_OutputVolume:
	    vol = LinToLog(argument);

	    if (vol == 0x20) vol = 0x8000;
	    else vol = vol | vol << 8;

	    D(bug("SetVol %05x translated to %04x\n", argument, vol));
	    dd->out_volume = argument;
	    if (ac97Base->mixer_set_reg)
		ac97Base->mixer_set_reg(ac97Base, AC97_PCM_VOL, vol);
	    return TRUE;
	    
	case AHIC_OutputVolume_Query:
	    return dd->out_volume;
    }

    return 0;
}

#undef SysBase

static AROS_UFIH1(play_int, struct AHIAudioCtrlDrv *, AudioCtrl)
{
    AROS_USERFUNC_INIT

    struct DriverBase*      AHIsubBase;
    struct ac97Base*        ac97Base;

    AHIsubBase = (struct DriverBase*) dd->ahisubbase;
    ac97Base   = (struct ac97Base*) AHIsubBase;

    dd->old_SR = inw(ac97Base->dmabase + ac97Base->off_po_sr);
    outw(dd->old_SR & 0x1c, ac97Base->dmabase + ac97Base->off_po_sr);
    
    if ((dd->old_SR & 0x1c) && dd->slavetask)
    {
        /* Signaling the slave task */
        Signal((struct Task *)dd->slavetask, SIGBREAKF_CTRL_E);
    }

    return FALSE;

    AROS_USERFUNC_EXIT
}
