
#include <config.h>

#include <devices/ahi.h>
#include <dos/dos.h>
#include <exec/execbase.h>
#include <libraries/ahi_sub.h>
#include <proto/ahi.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include <string.h>

#include "DriverData.h"
#include "library.h"

#define dd ((struct DeviceData*) AudioCtrl->ahiac_DriverData)

/******************************************************************************
** The slave process **********************************************************
******************************************************************************/

#undef SysBase

static void Slave( struct ExecBase* SysBase );

#if defined( __AROS__ )

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

#else

void SlaveEntry(void)
{
  struct ExecBase* SysBase = *((struct ExecBase**) 4);

  Slave( SysBase );
}

#endif


static void
Slave( struct ExecBase* SysBase )
{
  struct AHIAudioCtrlDrv* AudioCtrl;
  struct DriverBase*      AHIsubBase;
  struct DeviceBase* DeviceBase;
  BOOL                    running;
  ULONG                   signals;

  /* Note that in OS4, we cannot call FindTask(NULL) here, since IExec
   * is inside AHIsubBase! */
  AudioCtrl  = (struct AHIAudioCtrlDrv*) SysBase->ThisTask->tc_UserData;
  AHIsubBase = (struct DriverBase*) dd->ahisubbase;
  DeviceBase = (struct DeviceBase*) AHIsubBase;

  dd->slavesignal = AllocSignal( -1 );

  if( dd->slavesignal != -1 )
  {
    struct MsgPort*    ahi_mp           = NULL;
    struct AHIRequest* ahi_iorequest    = NULL;
    APTR               ahi_iocopy       = NULL;
    BYTE               ahi_device       = -1;

    struct AHIRequest* ahi_io[ 2 ]      = { NULL,  NULL };
    BOOL               ahi_io_used[ 2 ] = { FALSE, FALSE };

    ULONG              frame_length     = 0;

    ahi_mp = CreateMsgPort();

    if( ahi_mp != NULL )
    {
      ahi_iorequest = (struct AHIRequest*) CreateIORequest( ahi_mp, sizeof( struct AHIRequest ) );

      if( ahi_iorequest != NULL )
      {
	ahi_iorequest->ahir_Version = 4;

	ahi_device = OpenDevice( AHINAME, dd->unit,
				 (struct IORequest*) ahi_iorequest, 0 );

	if( ahi_device == 0 )
	{
	  struct Library* AHIBase = (struct Library*) ahi_iorequest->ahir_Std.io_Device;
#ifdef __AMIGAOS4__
	  struct AHIIFace* IAHI = (struct AHIIFace*) GetInterface(AHIBase, "main", 1, NULL);
#endif

	  ahi_iocopy = AllocVec( sizeof( *ahi_iorequest ), MEMF_ANY );

	  if( ahi_iocopy != NULL )
	  {
	    bcopy( ahi_iorequest, ahi_iocopy, sizeof( *ahi_iorequest ) );

	    ahi_io[ 0 ] = ahi_iorequest;
	    ahi_io[ 1 ] = ahi_iocopy;
    
	    // Everything set up. Tell Master we're alive and healthy.

	    Signal( (struct Task*) dd->mastertask,
		    1L << dd->mastersignal );

	    running = TRUE;

	    // The main playback loop follow

	    while( running )
	    {
	      int skip_mix;
	      APTR tmp_buff;
	      struct AHIRequest* tmp_io;
	      
	      if( ahi_io_used[ 0 ] )
	      {
		LONG  err;
		ULONG mask = ( SIGBREAKF_CTRL_C |
			       (1L << dd->slavesignal) |
			       (1L << ahi_mp->mp_SigBit) );
	      
		signals = Wait( mask );

		if( signals & ( SIGBREAKF_CTRL_C |
				(1L << dd->slavesignal) ) ) 
		{
		  running = FALSE;
		  break;
		}

		err = WaitIO( (struct IORequest*) ahi_io[ 0 ] );

		if( err != 0 )
		{
		  KPrintF( DRIVER ": AHI device error %ld\n", err );
//		  running = FALSE;
		  break;
		}
	      }

	      skip_mix = CallHookPkt( AudioCtrl->ahiac_PreTimerFunc,
				      (Object*) AudioCtrl, 0 );

	      CallHookPkt( AudioCtrl->ahiac_PlayerFunc, AudioCtrl, NULL );
		
	      if( ! skip_mix )
	      {
		CallHookPkt( AudioCtrl->ahiac_MixerFunc, AudioCtrl,
			     dd->mixbuffers[ 0 ] );
	      }
		
	      CallHookPkt( AudioCtrl->ahiac_PostTimerFunc, (Object*) AudioCtrl, 0 );

	      if( frame_length == 0 )
	      {
		frame_length = AHI_SampleFrameSize( AudioCtrl->ahiac_BuffType );
	      }
	      
	      ahi_io[ 0 ]->ahir_Std.io_Command = CMD_WRITE;
	      ahi_io[ 0 ]->ahir_Std.io_Data    = dd->mixbuffers[ 0 ];
	      ahi_io[ 0 ]->ahir_Std.io_Length  = ( AudioCtrl->ahiac_BuffSamples *
						   frame_length );
	      ahi_io[ 0 ]->ahir_Std.io_Offset  = 0;
	      ahi_io[ 0 ]->ahir_Frequency      = AudioCtrl->ahiac_MixFreq;
	      ahi_io[ 0 ]->ahir_Type           = AudioCtrl->ahiac_BuffType;
	      ahi_io[ 0 ]->ahir_Volume         = 0x10000;
	      ahi_io[ 0 ]->ahir_Position       = 0x08000;
	      ahi_io[ 0 ]->ahir_Link           = ( ahi_io_used[ 1 ] ?
						   ahi_io[ 1 ] : NULL );

	      SendIO( (struct IORequest*) ahi_io[ 0 ] );
    
	      tmp_io = ahi_io[ 0 ];
	      ahi_io[ 0 ] = ahi_io[ 1 ];
	      ahi_io[ 1 ] = tmp_io;

	      tmp_buff = dd->mixbuffers[ 0 ];
	      dd->mixbuffers[ 0 ] = dd->mixbuffers[ 1 ];
	      dd->mixbuffers[ 1 ] = tmp_buff;
	      
	      ahi_io_used[ 0 ] = ahi_io_used[ 1 ];
	      ahi_io_used[ 1 ] = TRUE;
	    }

	    
	    if( ahi_io_used[ 0 ] )
	    {
	      AbortIO( (struct IORequest*) ahi_io[ 0 ] );
	      WaitIO( (struct IORequest*) ahi_io[ 0 ] );
	    }

	    if( ahi_io_used[ 1 ] )
	    {
	      AbortIO( (struct IORequest*) ahi_io[ 1 ] );
	      WaitIO( (struct IORequest*) ahi_io[ 1 ] );
	    }
	    
	    FreeVec( ahi_iocopy );
	  }

#ifdef __AMIGAOS4__
	  DropInterface((struct Interface*) IAHI);
#endif
	  CloseDevice( (struct IORequest*) ahi_iorequest );
	}

	DeleteIORequest( (struct IORequest*) ahi_iorequest );
      }

      DeleteMsgPort( ahi_mp );
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
