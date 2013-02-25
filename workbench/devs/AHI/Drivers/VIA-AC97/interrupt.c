/*
    Copyright © 2005-2013, Davy Wentzler. All rights reserved.
    Copyright © 2010-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <config.h>

#undef __USE_INLINE__
#include <proto/expansion.h>
#include <libraries/ahi_sub.h>
#include <proto/exec.h>
#include <stddef.h>
#include "library.h"
#include "regs.h"
#include "interrupt.h"
#include "misc.h"
#include "hwaccess.h"
#include "pci_wrapper.h"

#define min(a,b) ((a)<(b)?(a):(b))

static int z = 0;

/******************************************************************************
** Hardware interrupt handler *************************************************
******************************************************************************/


#ifdef __amigaos4__
LONG CardInterrupt(struct ExceptionContext *pContext,
  struct ExecBase *SysBase, struct CardData* card)
#else
LONG CardInterrupt(struct CardData* card)
#endif
{
  struct AHIAudioCtrlDrv* AudioCtrl = card->audioctrl;
  struct DriverBase*  AHIsubBase = (struct DriverBase*) card->ahisubbase;
#ifdef __amigaos4__
  struct PCIDevice *dev = (struct PCIDevice * ) card->pci_dev;
#endif

  ULONG intreq, status;
  LONG  handled = 0;

  intreq = pci_inl(VIA_REG_SGD_SHADOW, card);

    //DebugPrintF("INT %lx\n", intreq);
    if( intreq & 0x33 )
    {
      unsigned char play_status = pci_inb(VIA_REG_OFFSET_STATUS, card),
                    rec_status = pci_inb(VIA_REG_OFFSET_STATUS + RECORD, card);
      
      play_status &= (VIA_REG_STAT_EOL|VIA_REG_STAT_FLAG|VIA_REG_STAT_STOPPED);
        
      if (play_status)
      {
            pci_outb(play_status, VIA_REG_OFFSET_STATUS, card);
        
            if (play_status & VIA_REG_STAT_FLAG)
            {
               card->flip = 0;
               card->current_buffer = card->playback_buffer1;
               }
            else if (play_status & VIA_REG_STAT_EOL)
            {
               card->flip = 1;
               card->current_buffer = card->playback_buffer2;
            }
            
            card->playback_interrupt_enabled = FALSE;
            Cause( &card->playback_interrupt );
      }
      
      rec_status &= (VIA_REG_STAT_EOL|VIA_REG_STAT_FLAG|VIA_REG_STAT_STOPPED);
      if (rec_status)
      {
            //DebugPrintF("REC!\n");
            pci_outb(rec_status, VIA_REG_OFFSET_STATUS + RECORD, card);
        
            if (rec_status & VIA_REG_STAT_FLAG)
            {
               card->flip = 0;
               card->current_record_buffer = card->record_buffer1;
               }
            else if (rec_status & VIA_REG_STAT_EOL)
            {
               card->flip = 1;
               card->current_record_buffer = card->record_buffer2;
            }
      
            card->record_interrupt_enabled = FALSE;
            Cause( &card->record_interrupt );
      }
      
      handled = 1;
    }
  
  return handled;
}


/******************************************************************************
** Playback interrupt handler *************************************************
******************************************************************************/

#ifdef __amigaos4__
void PlaybackInterrupt(struct ExceptionContext *pContext,
  struct ExecBase *SysBase, struct CardData* card)
#else
void PlaybackInterrupt(struct CardData* card)
#endif
{
  struct AHIAudioCtrlDrv* AudioCtrl;
  struct DriverBase*  AHIsubBase;

  if (card == NULL)
    return;

  AudioCtrl = card->audioctrl;
  AHIsubBase = (struct DriverBase*) card->ahisubbase;

  if( card->mix_buffer != NULL && card->current_buffer != NULL)
  {
    BOOL   skip_mix;

    WORD*  src;
    WORD*  dst;
    size_t skip;
    size_t samples;
    int    i;
    
    skip_mix = CallHookPkt( AudioCtrl->ahiac_PreTimerFunc, (Object*) AudioCtrl, 0 );
    CallHookPkt( AudioCtrl->ahiac_PlayerFunc, (Object*) AudioCtrl, NULL );

    //DebugPrintF("skip_mix = %d\n", skip_mix);

    if( ! skip_mix )
    {
      CallHookPkt( AudioCtrl->ahiac_MixerFunc, (Object*) AudioCtrl, card->mix_buffer );
    }
    
    /* Now translate and transfer to the DMA buffer */


    skip    = ( AudioCtrl->ahiac_Flags & AHIACF_HIFI ) ? 2 : 1;
    samples = card->current_bytesize >> 1;

    src     = card->mix_buffer;
#if !defined(__amigaos4__) && !AROS_BIG_ENDIAN
    if(skip == 2)
        src++;
#endif
    dst     = card->current_buffer;

    i = samples;

    while( i > 0 )
    {
#ifdef __amigaos4__
      *dst = ( ( *src & 0xff ) << 8 ) | ( ( *src & 0xff00 ) >> 8 );
#else
      *dst = AROS_WORD2LE(*src);
#endif
      src += skip;
      dst += 1;

      --i;
    }

    CacheClearE( card->current_buffer, (ULONG) dst - (ULONG) card->current_buffer, CACRF_ClearD );
    CallHookPkt( AudioCtrl->ahiac_PostTimerFunc, (Object*) AudioCtrl, 0 );
  }

  card->playback_interrupt_enabled = TRUE;
}


/******************************************************************************
** Record interrupt handler ***************************************************
******************************************************************************/

#ifdef __amigaos4__
void RecordInterrupt(struct ExceptionContext *pContext,
  struct ExecBase *SysBase, struct CardData* card)
#else
void RecordInterrupt(struct CardData* card)
#endif
{
  struct AHIAudioCtrlDrv* AudioCtrl = card->audioctrl;
  struct DriverBase*  AHIsubBase = (struct DriverBase*) card->ahisubbase;

  struct AHIRecordMessage rm =
  {
    AHIST_S16S,
    card->current_record_buffer,
    RECORD_BUFFER_SAMPLES
  };

  int   i   = 0, shorts = card->current_record_bytesize / 2;
  WORD* ptr = card->current_record_buffer;


  while( i < shorts )
  {
#if defined(__AMIGAOS4__) || AROS_BIG_ENDIAN
    *ptr = ( ( *ptr & 0xff ) << 8 ) | ( ( *ptr & 0xff00 ) >> 8 );
#endif
    ++i;
    ++ptr;
  }

  CallHookPkt( AudioCtrl->ahiac_SamplerFunc, (Object*) AudioCtrl, &rm );

  card->record_interrupt_enabled = TRUE;
}


