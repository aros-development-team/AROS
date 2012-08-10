
#include <config.h>

#include <devices/ahi.h>
#include <libraries/ahi_sub.h>

#include "DriverData.h"
#include "library.h"

#define dd ((struct AROSData*) AudioCtrl->ahiac_DriverData)

#define min(a,b) ( (a) < (b) ? (a) : (b) )

#define USE_TIMERTICK 1

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


#if USE_TIMERTICK

#include <hardware/intbits.h>

AROS_UFIH1(AHITimerTickCode, struct Task *, task)
{
    AROS_USERFUNC_INIT

    Signal(task, SIGBREAKF_CTRL_F);
    return 0;

    AROS_USERFUNC_EXIT
}

static void SmallDelay(struct ExecBase *SysBase)
{
    struct Interrupt i;
    
    i.is_Code 	      = (APTR)AHITimerTickCode;
    i.is_Data 	      = FindTask(0);
    i.is_Node.ln_Name = "AROS AHI Driver Timer Tick Server";
    i.is_Node.ln_Pri  = 0;
    i.is_Node.ln_Type = NT_INTERRUPT;
    
    SetSignal(0, SIGBREAKF_CTRL_F);
    AddIntServer(INTB_VERTB, &i);
    Wait(SIGBREAKF_CTRL_F);
    RemIntServer(INTB_VERTB, &i);
}

#endif

static void
Slave( struct ExecBase* SysBase )
{
  struct AHIAudioCtrlDrv* AudioCtrl;
  struct DriverBase*      AHIsubBase;
  struct AROSBase*        AROSBase;
  BOOL                    running;
  ULONG                   signals;

  int bytes_in_buffer  = 0;
  int offset_in_buffer = 0;

  AudioCtrl  = (struct AHIAudioCtrlDrv*) FindTask( NULL )->tc_UserData;
  AHIsubBase = (struct DriverBase*) dd->ahisubbase;
  AROSBase   = (struct AROSBase*) AHIsubBase;

  dd->slavesignal = AllocSignal( -1 );

  if( dd->slavesignal != -1 )
  {
    // Everything set up. Tell Master we're alive and healthy.

    Signal( (struct Task*) dd->mastertask,
            1L << dd->mastersignal );

    running = TRUE;

    // The main playback loop follow

    while( running )
    {      
      signals = SetSignal(0L,0L);

//    KPrintF("++++ arosdriver_after signal checking\n");
      if( signals & ( SIGBREAKF_CTRL_C | (1L << dd->slavesignal) ) )
      {
        running = FALSE;
      }
      else
      {
	int skip_mix;
	int bytes_avail;
	
	// First Delay() until there is at least one fragment free

	while( TRUE )
	{
	  int frag_avail, frag_alloc, frag_size;
	
	  OSS_GetOutputInfo( &frag_avail, &frag_alloc, &frag_size,
			     &bytes_avail );
	  
//	  KPrintF( "%ld fragments available, %ld alloced (%ld bytes each). %ld bytes total\n", frag_avail, frag_alloc, frag_size, bytes_avail );
	  
	  if( frag_avail == 0 )
	  {
	    // This is actually quite a bit too long delay :-( For
	    // comparison, the SB Live/Audigy driver uses 1/1000 s
	    // polling ...
//	    KPrintF("Delay\n");
    	  #if USE_TIMERTICK
	    SmallDelay(SysBase);
	  #else
	    Delay( 1 );
	  #endif
	  }
	  else
	  {
	    break;
	  }
	}

	skip_mix = 0;/*CallHookA( AudioCtrl->ahiac_PreTimerFunc,
		       (Object*) AudioCtrl, 0 );*/

	while( bytes_avail > 0 )
	{
//	  KPrintF( "%ld bytes in current buffer.\n", bytes_in_buffer );
	  
	  if( bytes_in_buffer == 0 )
	  {
	    int skip     = 0;
	    int offset   = 0;

	    int samples  = 0;
	    int bytes    = 0;
	  
	    int i;
	  
	    WORD* src;
	    WORD* dst;

	    CallHookPkt( AudioCtrl->ahiac_PlayerFunc, AudioCtrl, NULL );

	    samples = AudioCtrl->ahiac_BuffSamples;
	    bytes   = samples * 2; // one 16 bit sample is 2 bytes

	    switch( AudioCtrl->ahiac_BuffType )
	    {
	      case AHIST_M16S:
		skip = 1;
		offset = 0;
		break;
	    
	      case AHIST_M32S:
		skip = 2;
		offset = 1;
		break;

	      case AHIST_S16S:
		skip = 1;
		offset = 0;
		samples *= 2;
		bytes *= 2;
		break;

	      case AHIST_S32S:
		skip = 2;
		offset = 1;
		samples *= 2;
		bytes *= 2;
		break;
	    }

	    bytes_in_buffer  = bytes;
	    offset_in_buffer = 0;

	    if( ! skip_mix )
	    {
	      CallHookPkt( AudioCtrl->ahiac_MixerFunc, AudioCtrl,
			   dd->mixbuffer );
	    }

	    src = ((WORD*) dd->mixbuffer) + offset;
	    dst = dd->mixbuffer;

	    for( i = 0; i < samples; ++i )
	    {
	      *dst++ = *src;
	      src += skip;
	    }

//	    KPrintF( "Mixed %ld/%ld new bytes/samples\n", bytes, samples );
	  }

	  while( bytes_in_buffer > 0 &&
		 bytes_avail > 0 )
	  {
	    int written;
    	    int counter = 0;
    	    do
	    {
	    	written = OSS_Write( dd->mixbuffer + offset_in_buffer,
				    min( bytes_in_buffer, bytes_avail ) );
	    	if (counter > 10)
		{
		    if (written < 0) written = 0;
		    break;
		}
		
		if (written < 0) KPrintF("OSS_Write returned %ld. counter %ld  bytes_in_buffer %ld  bytes_avail %ld\n",
		    	    	written, counter, bytes_in_buffer, bytes_avail);
		
		counter++;
		
    	    } while (written < 0);
	    
	    bytes_in_buffer  -= written;
	    offset_in_buffer += written;
	    bytes_avail      -= written;
	    
//	    KPrintF( "Wrote %ld bytes (%ld bytes in buffer, offset=%ld, %ld bytes available in OSS buffers\n",
//		     written, bytes_in_buffer, offset_in_buffer, bytes_avail );
	  }
	}

	CallHookA( AudioCtrl->ahiac_PostTimerFunc, (Object*) AudioCtrl, 0 );
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
