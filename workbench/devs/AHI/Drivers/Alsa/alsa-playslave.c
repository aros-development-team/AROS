/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <config.h>

#include <devices/ahi.h>
#include <exec/execbase.h>
#include <libraries/ahi_sub.h>

#include "DriverData.h"
#include "library.h"

#define dd ((struct AlsaData*) AudioCtrl->ahiac_DriverData)

/******************************************************************************
** The slave process **********************************************************
******************************************************************************/

#undef SysBase

void Slave( struct ExecBase* SysBase );

#include <aros/asmcall.h>

AROS_UFH3(LONG, SlaveEntry,
      AROS_UFHA(STRPTR, argPtr, A0),
      AROS_UFHA(ULONG, argSize, D0),
      AROS_UFHA(struct ExecBase *, SysBase, A6))
{
   AROS_USERFUNC_INIT
   Slave( SysBase );
   AROS_USERFUNC_EXIT
}

void
Slave( struct ExecBase* SysBase )
{
  struct AHIAudioCtrlDrv* AudioCtrl;
  struct DriverBase*      AHIsubBase;
  struct AlsaBase*        AlsaBase;
  BOOL                    running;
  ULONG                   signals;

  AudioCtrl  = (struct AHIAudioCtrlDrv*) FindTask(NULL)->tc_UserData;
  AHIsubBase = (struct DriverBase*) dd->ahisubbase;
  AlsaBase   = (struct AlsaBase*) AHIsubBase;

  dd->slavesignal = AllocSignal( -1 );

  if( dd->slavesignal != -1 )
  {
    // Everything set up. Tell Master we're alive and healthy.

    Signal( (struct Task*) dd->mastertask,
            1L << dd->mastersignal );

    running = TRUE;

    while( running )
    {
      signals = SetSignal(0L,0L);

      if( signals & ( SIGBREAKF_CTRL_C | (1L << dd->slavesignal) ) )
      {
        running = FALSE;
      }
      else
      {
        while(TRUE)
        {
          LONG avail = ALSA_Avail(dd->alsahandle);
          if (avail > 1024)
            break;
          Delay(1);
        }


        CallHookPkt( AudioCtrl->ahiac_PlayerFunc, AudioCtrl, NULL );
        CallHookPkt( AudioCtrl->ahiac_MixerFunc, AudioCtrl, dd->mixbuffer );

        ALSA_Write(dd->alsahandle, dd->mixbuffer, AudioCtrl->ahiac_BuffSamples);

        CallHookA(AudioCtrl->ahiac_PostTimerFunc, (Object*) AudioCtrl, 0);
      }
    }
  }

  FreeSignal( dd->slavesignal );
  dd->slavesignal = -1;

  Forbid();

  // Tell the Master we're dying

  Signal( (struct Task*) dd->mastertask, 1L << dd->mastersignal );

  dd->slavetask = NULL;

  // Multitaking will resume when we are dead.
}
