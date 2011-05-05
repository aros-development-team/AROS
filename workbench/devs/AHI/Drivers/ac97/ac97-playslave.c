#define DEBUG 0
#include <aros/debug.h>
#include <asm/io.h>

#include <config.h>

#include <devices/ahi.h>
#include <libraries/ahi_sub.h>

#include "DriverData.h"
#include "library.h"

#define dd ((struct AC97Data *) AudioCtrl->ahiac_DriverData)

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

struct BufferDescriptor {
    APTR    pointer;
    ULONG   length;
};

void
Slave( struct ExecBase* SysBase )
{
  struct AHIAudioCtrlDrv* AudioCtrl;
  struct DriverBase*      AHIsubBase;
  struct ac97Base*        ac97Base;
  BOOL                    running, firstTime = TRUE;
  ULONG                   signals;

  AudioCtrl  = (struct AHIAudioCtrlDrv*) FindTask( NULL )->tc_UserData;
  AHIsubBase = (struct DriverBase*) dd->ahisubbase;
  ac97Base   = (struct ac97Base*) AHIsubBase;

  dd->slavesignal = AllocSignal( -1 );
    

//    outb(0x1e, ac97Base->dmabase + PO_CR);
//    outl(ac97Base->PCM_out, ac97Base->dmabase + PO_BDBAR);

D(bug("SR=%04x CR=%04x CIV=%02x LVI=%02x\n", inw(ac97Base->dmabase + ac97Base->off_po_sr), 
    inw(ac97Base->dmabase + PO_CR),
    inb(ac97Base->dmabase + PO_CIV),
    inb(ac97Base->dmabase + PO_LVI)));

  if( dd->slavesignal != -1 )
  {
    // Everything set up. Tell Master we're alive and healthy.

    Signal( (struct Task*) dd->mastertask,
            1L << dd->mastersignal );

    running = TRUE;
    
    SetTaskPri(FindTask(NULL), 127);

    int tail = (inb(ac97Base->dmabase + PO_CIV) + 1) & 0x1f;

    while( running )
    {
      signals = SetSignal(0L,0L);
    

      if( signals & ( SIGBREAKF_CTRL_C | (1L << dd->slavesignal) ) )
      {
        running = FALSE;
      }
      else
      {
	  int i,j;
	  IPTR buff;
        CallHookPkt( AudioCtrl->ahiac_PlayerFunc, AudioCtrl, NULL );
        CallHookPkt( AudioCtrl->ahiac_MixerFunc, AudioCtrl, dd->mixbuffer );
        
	i = AudioCtrl->ahiac_BuffSamples << 1;
    i <<= ac97Base->size_shift; /* For SIS 7012 size must be in bytes */
	j = tail;
	buff = dd->mixbuffer;

	while (i > 0)
	{
	    ac97Base->PCM_out[j].sample_address = buff;
	    ac97Base->PCM_out[j].sample_size = (i > 65532) ? 65532 : i;
	    
	    i -= ac97Base->PCM_out[j].sample_size;
	    buff += ac97Base->PCM_out[j].sample_size 
                    << (1 - ac97Base->size_shift); /* SIS 7012: size already in bytes */
	    j++;
	    tail++;
	    tail &= 0x1f;
	    j &= 0x1f;
	}
	ac97Base->PCM_out[(j-1) & 0x1f].sample_size |= 0x80000000;
/*	
	D(bug("playing audio from %x (size %d, buffer %d)\n",
	    dd->mixbuffer, AudioCtrl->ahiac_BuffSamples, j-1));

	D(bug("SR=%08x ",inl(ac97Base->dmabase + PO_CIV)));
*/
//	outw(4, ac97Base->dmabase + ac97Base->off_po_sr);
	outb((j-1) & 0x1f, ac97Base->dmabase + PO_LVI);
	if (firstTime)
	{
	    outb(0x11, ac97Base->dmabase + PO_CR);
		/* Enable busmaster + interrupt on completion */
	    firstTime = FALSE;
	}

//	outw(0x1c, ac97Base->dmabase + ac97Base->off_po_sr);
//	D(bug("SR=%04x ",inw(ac97Base->dmabase + ac97Base->off_po_sr)));
//	while (!(inw(ac97Base->dmabase + ac97Base->off_po_sr) & 8)) { 
//	    D(bug("SR=%04x ",inw(ac97Base->dmabase + ac97Base->off_po_sr)));

    D(bug("Waiting for int..."));    
    Wait(SIGBREAKF_CTRL_E); 
    D(bug("Got it\n"));

//	 }
//	D(bug("SR=%04x\n",inw(ac97Base->dmabase + ac97Base->off_po_sr)));
//        outw(inw(ac97Base->dmabase + ac97Base->off_po_sr), ac97Base->dmabase + ac97Base->off_po_sr);
	
//	ac97Base->PCM_out
        // The mixing buffer is now filled with AudioCtrl->ahiac_BuffSamples
        // of sample frames (type AudioCtrl->ahiac_BuffType). Send them
        // to the sound card here.
      }
    }
  }

  outb(0, ac97Base->dmabase + PO_CR);
  FreeSignal( dd->slavesignal );
  dd->slavesignal = -1;

  Forbid();

  // Tell the Master we're dying

  Signal( (struct Task*) dd->mastertask,
          1L << dd->mastersignal );

  dd->slavetask = NULL;

  // Multitasking will resume when we are dead.
}

