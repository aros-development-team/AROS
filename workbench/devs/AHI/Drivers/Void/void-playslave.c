
#include <config.h>

#include <devices/ahi.h>
#include <exec/execbase.h>
#include <libraries/ahi_sub.h>

#include "DriverData.h"
#include "library.h"

#define dd ((struct VoidData*) AudioCtrl->ahiac_DriverData)

/******************************************************************************
** The slave process **********************************************************
******************************************************************************/

#undef SysBase

void Slave( struct ExecBase* SysBase );

#if defined( __AROS__ )

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

#else

void SlaveEntry(void)
{
  struct ExecBase* SysBase = *((struct ExecBase**) 4);

  Slave( SysBase );
}
#endif

void
Slave( struct ExecBase* SysBase )
{
  struct AHIAudioCtrlDrv* AudioCtrl;
  struct DriverBase*      AHIsubBase;
  struct VoidBase*        VoidBase;
  BOOL                    running;
  ULONG                   signals;

  /* Note that in OS4, we cannot call FindTask(NULL) here, since IExec
   * is inside AHIsubBase! */
  AudioCtrl  = (struct AHIAudioCtrlDrv*) SysBase->ThisTask->tc_UserData;
  AHIsubBase = (struct DriverBase*) dd->ahisubbase;
  VoidBase   = (struct VoidBase*) AHIsubBase;

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
        CallHookPkt( AudioCtrl->ahiac_PlayerFunc, AudioCtrl, NULL );
        CallHookPkt( AudioCtrl->ahiac_MixerFunc, AudioCtrl, dd->mixbuffer );
        
        // The mixing buffer is now filled with AudioCtrl->ahiac_BuffSamples
        // of sample frames (type AudioCtrl->ahiac_BuffType). Send them
        // to the sound card here.
      }
    }
  }

  FreeSignal( dd->slavesignal );
  dd->slavesignal = -1;

  Forbid();

  // Tell the Master we're dying

  Signal( (struct Task*) dd->mastertask,
          1L << dd->mastersignal );

  dd->slavetask = NULL;

  // Multitaking will resume when we are dead.
}
