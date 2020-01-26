/*
    Copyright © 2015-2016, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <config.h>

#include <devices/ahi.h>
#include <exec/execbase.h>
#include <libraries/ahi_sub.h>

#include "DriverData.h"
#include "library.h"

#include "alsa-bridge/alsa.h"

#define dd ((struct AlsaData*) AudioCtrl->ahiac_DriverData)

#define min(a,b) ( (a) < (b) ? (a) : (b) )

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
   return 0;
   AROS_USERFUNC_EXIT
}

#include <hardware/intbits.h>
#include <proto/timer.h>

AROS_INTH1(AHITimerTickCode, struct Task *, task)
{
    AROS_INTFUNC_INIT
    Signal(task, SIGBREAKF_CTRL_F);
    return 0;

    AROS_INTFUNC_EXIT
}



static void SmallDelay(struct ExecBase *SysBase)
{
    struct Interrupt i;

    i.is_Code         = (APTR)AHITimerTickCode;
    i.is_Data         = FindTask(0);
    i.is_Node.ln_Name = "AROS AHI Driver Timer Tick Server";
    i.is_Node.ln_Pri  = 0;
    i.is_Node.ln_Type = NT_INTERRUPT;

    SetSignal(0, SIGBREAKF_CTRL_F);
    AddIntServer(INTB_VERTB, &i);
    Wait(SIGBREAKF_CTRL_F);
    RemIntServer(INTB_VERTB, &i);
}

void
Slave( struct ExecBase* SysBase )
{
  struct AHIAudioCtrlDrv* AudioCtrl;
  struct DriverBase*      AHIsubBase;
  BOOL                    running;
  ULONG                   signals;
  LONG                    framesready = 0;
  APTR                    framesptr = NULL;
  LONG                    i = 0;

  Wait(SIGF_SINGLE);

  AudioCtrl  = (struct AHIAudioCtrlDrv*) FindTask(NULL)->tc_UserData;
  AHIsubBase = (struct DriverBase*) dd->ahisubbase;
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
        LONG framesfree = 0;
        LONG readcycles = 0;

        while(TRUE)
        {
          framesfree = ALSA_Avail(dd->alsahandle);
          if (framesfree == ALSA_XRUN)
          {
              D(bug("[Alsa] ALSA_Avail() == XRUN\n"));
              ALSA_Prepare(dd->alsahandle);
              framesfree = ALSA_Avail(dd->alsahandle);
          }

          if (framesfree >= 64)
          {
            readcycles++;
            if (framesfree >= AudioCtrl->ahiac_BuffSamples)
              readcycles++;
            break;
          }

          SmallDelay(SysBase);
        }

        /* Loop until alsa buffer is filled */
        while (framesfree > 0)
        {
          LONG written;

          if (framesready == 0)
          {
            if (readcycles == 0)
              break;

            CallHookPkt(AudioCtrl->ahiac_PlayerFunc, AudioCtrl, NULL );
            CallHookPkt(AudioCtrl->ahiac_MixerFunc, AudioCtrl, dd->mixbuffer );
            framesready = AudioCtrl->ahiac_BuffSamples;
            framesptr = dd->mixbuffer;
            /*
             * Increase volume of buffer so that 100% of volume on AROS side matches
             * 100% volume of host applications.
             *
             * This is to compensate for (volume >> 1) in _AHI_SetVol
             */
            for (i = 0; i < framesready << 1; i+=2)
            {
                ((WORD *)dd->mixbuffer)[i] = ((WORD *)dd->mixbuffer)[i] << 1;
                ((WORD *)dd->mixbuffer)[i + 1] = ((WORD *)dd->mixbuffer)[i + 1] << 1;
            }
            readcycles--;
          }

          written = ALSA_Write(dd->alsahandle, framesptr, min(framesready,
                  framesfree));
          if (written == ALSA_XRUN)
          {
              D(bug("[Alsa] ALSA_Write() == XRUN %d, %d\n", framesfree, framesready));
              ALSA_Prepare(dd->alsahandle);
              written = ALSA_Write(dd->alsahandle, framesptr, min(framesready,
                      framesfree));
          }

          framesready -= written;
          framesfree  -= written;
          framesptr += written * 4;

          CallHookA(AudioCtrl->ahiac_PostTimerFunc, (Object*) AudioCtrl, 0);
        }
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
