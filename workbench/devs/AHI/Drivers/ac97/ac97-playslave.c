
#include <aros/debug.h>

#include <asm/io.h>
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

AROS_UFH3(void, SlaveEntry,
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

//    outb(0x1e, (IPTR)ac97Base->dmabase + PO_CR);
//    outl(ac97Base->PCM_out, (IPTR)ac97Base->dmabase + PO_BDBAR);

  D(bug("[AHI:AC97] %s: SR=%04x CR=%02x CIV=%02x LVI=%02x\n", __func__,
  inw((IPTR)ac97Base->dmabase + ac97Base->off_po_sr),
  inb((IPTR)ac97Base->dmabase + PO_CR),
  inb((IPTR)ac97Base->dmabase + PO_CIV),
  inb((IPTR)ac97Base->dmabase + PO_LVI)));

  if( dd->slavesignal != -1 )
  {
    // Everything set up. Tell Master we're alive and healthy.

    Signal( (struct Task*) dd->mastertask,
            1L << dd->mastersignal );

    running = TRUE;
    
    SetTaskPri(FindTask(NULL), 127);

    int tail = (inb((IPTR)ac97Base->dmabase + PO_CIV) + 1) & 0x1f;

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
#if defined(__AROS__) && (__WORDSIZE==64)
	int bufSize = 0;
	APTR buffPtr = NULL;
#endif
	ULONG buff;

        CallHookPkt( AudioCtrl->ahiac_PlayerFunc, AudioCtrl, NULL );
        CallHookPkt( AudioCtrl->ahiac_MixerFunc, AudioCtrl, dd->mixbuffer );

	i = AudioCtrl->ahiac_BuffSamples << 1;
	i <<= ac97Base->size_shift; /* For SIS 7012 size must be in bytes */
	j = tail;
#if defined(__AROS__) && (__WORDSIZE==64)
        if (((IPTR)dd->mixbuffer > 0xFFFFFFFF) || (((IPTR)dd->mixbuffer + i) > 0xFFFFFFFF))
        {
            bufSize = i;
            buffPtr = AllocPooled(ac97Base->buffer, i);
            CopyMem(dd->mixbuffer, buffPtr, i);
            buff = (ULONG)(IPTR)buffPtr;
        }
        else
#endif
            buff = (ULONG)(IPTR)dd->mixbuffer;
	while (i > 0)
	{
	    D(bug("[AHI:AC97] %s: Playing sample @ %p\n", __func__, (IPTR)buff));
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

	D(bug("SR=%08x ",inl((IPTR)ac97Base->dmabase + PO_CIV)));
*/
//	outw(4, (IPTR)ac97Base->dmabase + ac97Base->off_po_sr);
	outb((j-1) & 0x1f, (IPTR)ac97Base->dmabase + PO_LVI);
	if (firstTime)
	{
	    outb(0x11, (IPTR)ac97Base->dmabase + PO_CR);
		/* Enable busmaster + interrupt on completion */
	    firstTime = FALSE;
	}

//	outw(0x1c, (IPTR)ac97Base->dmabase + ac97Base->off_po_sr);
//	D(bug("SR=%04x ",inw((IPTR)ac97Base->dmabase + ac97Base->off_po_sr)));
//	while (!(inw((IPTR)ac97Base->dmabase + ac97Base->off_po_sr) & 8)) { 
//	    D(bug("SR=%04x ",inw((IPTR)ac97Base->dmabase + ac97Base->off_po_sr)));

	D(bug("[AHI:AC97] %s: Waiting for int...", __func__));    
	Wait(SIGBREAKF_CTRL_E); 
	D(bug("[AHI:AC97] %s: Got it\n", __func__));

//	 }
//	D(bug("SR=%04x\n",inw((IPTR)ac97Base->dmabase + ac97Base->off_po_sr)));
//        outw(inw((IPTR)ac97Base->dmabase + ac97Base->off_po_sr), (IPTR)ac97Base->dmabase + ac97Base->off_po_sr);
	
//	ac97Base->PCM_out
        // The mixing buffer is now filled with AudioCtrl->ahiac_BuffSamples
        // of sample frames (type AudioCtrl->ahiac_BuffType). Send them
        // to the sound card here.
#if defined(__AROS__) && (__WORDSIZE==64)
        if (buffPtr)
            FreePooled(ac97Base->buffer, buffPtr, bufSize);
#endif
      }
    }
  }

  outb(0, (IPTR)ac97Base->dmabase + PO_CR);
  FreeSignal( dd->slavesignal );
  dd->slavesignal = -1;

  Forbid();

  // Tell the Master we're dying

  Signal( (struct Task*) dd->mastertask,
          1L << dd->mastersignal );

  dd->slavetask = NULL;

  // Multitasking will resume when we are dead.
}

