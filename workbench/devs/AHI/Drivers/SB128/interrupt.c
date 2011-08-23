/*

The contents of this file are subject to the AROS Public License Version 1.1 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
http://www.aros.org/license.html

Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF
ANY KIND, either express or implied. See the License for the specific language governing rights and
limitations under the License.

The Original Code is (C) Copyright 2004-2011 Ross Vumbaca.

The Initial Developer of the Original Code is Ross Vumbaca.

All Rights Reserved.

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

#define min(a,b) ((a)<(b)?(a):(b))

unsigned long z = 0;
/******************************************************************************
** Hardware interrupt handler *************************************************
******************************************************************************/


LONG
CardInterrupt( struct ExceptionContext *pContext, struct ExecBase *SysBase, struct CardData* card )
{
  struct AHIAudioCtrlDrv* AudioCtrl = card->audioctrl;
  struct DriverBase*  AHIsubBase = (struct DriverBase*) card->ahisubbase;
  struct PCIDevice *dev = (struct PCIDevice * ) card->pci_dev;

  ULONG intreq;
  LONG  handled = 0;
  while (((intreq = (dev->InLong(card->iobase + SB128_STATUS))) & SB128_INT_PENDING) != 0)
  {
    if( intreq & SB128_INT_DAC2 && AudioCtrl != NULL )
    {
      /* Clear interrupt pending bit(s) and re-enable playback interrupts */
      dev->OutLong(card->iobase + SB128_SCON, (dev->InLong(card->iobase + SB128_SCON) & ~SB128_DAC2_INTEN));
      dev->OutLong(card->iobase + SB128_SCON, (dev->InLong(card->iobase + SB128_SCON) | SB128_DAC2_INTEN));

      if (card->flip == 0) /* just played buf 1 */
      {
         card->flip = 1;
         card->current_buffer = card->playback_buffer;
      }
      else  /* just played buf 2 */
      {
         card->flip = 0;
         card->current_buffer = (APTR) ((unsigned long) card->playback_buffer + card->current_bytesize);
      }
      
      card->playback_interrupt_enabled = FALSE;
      IExec->Cause( &card->playback_interrupt );
    }

    if( intreq & SB128_INT_ADC && AudioCtrl != NULL )
    {
      /* Clear interrupt pending bit(s) and re-enable record interrupts */
      dev->OutLong(card->iobase + SB128_SCON, (dev->InLong(card->iobase + SB128_SCON) & ~SB128_ADC_INTEN));
      dev->OutLong(card->iobase + SB128_SCON, (dev->InLong(card->iobase + SB128_SCON) | SB128_ADC_INTEN));

      if( card->record_interrupt_enabled )
      {
         /* Invoke softint to convert and feed AHI with the new sample data */

         if (card->recflip == 0) /* just filled buf 1 */
         {
            card->recflip = 1;
            card->current_record_buffer = card->record_buffer;
            }
         else  /* just filled buf 2 */
         {
            card->recflip = 0;
            card->current_record_buffer = (APTR) ((unsigned long) card->record_buffer + card->current_record_bytesize);
         }
         card->record_interrupt_enabled = FALSE;
         IExec->Cause( &card->record_interrupt );
      }
    }
    exit:
    handled = 1;
    
  }
  
  return handled;
}


/******************************************************************************
** Playback interrupt handler *************************************************
******************************************************************************/

void
PlaybackInterrupt( struct ExceptionContext *pContext, struct ExecBase *SysBase, struct CardData* card )
{
  struct AHIAudioCtrlDrv* AudioCtrl = card->audioctrl;
  struct DriverBase*  AHIsubBase = (struct DriverBase*) card->ahisubbase;
  struct PCIDevice *dev = (struct PCIDevice * ) card->pci_dev;

  if( card->mix_buffer != NULL && card->current_buffer != NULL )
  {
    BOOL   skip_mix;

    WORD*  src;
    WORD*  dst;
    size_t skip;
    size_t samples;
    int    i;
    
  	skip_mix = IUtility->CallHookPkt( AudioCtrl->ahiac_PreTimerFunc, (Object*) AudioCtrl, 0 );  
    IUtility->CallHookPkt( AudioCtrl->ahiac_PlayerFunc, (Object*) AudioCtrl, NULL );

  if( ! skip_mix )
    {
      IUtility->CallHookPkt( AudioCtrl->ahiac_MixerFunc, (Object*) AudioCtrl, card->mix_buffer );
    }
    
    /* Now translate and transfer to the DMA buffer */

    skip    = ( AudioCtrl->ahiac_Flags & AHIACF_HIFI ) ? 2 : 1;
    samples = card->current_bytesize >> 1;

    src     = card->mix_buffer;
    dst     = card->current_buffer;

    i = samples;

    while( i > 0 )
    {
      *dst = ( ( *src & 0xff ) << 8 ) | ( ( *src & 0xff00 ) >> 8 );

      src += skip;
      dst += 1;

      --i;
    }
    
    //Flush cache so that data is completely written to the DMA buffer - Articia hack
    IExec->CacheClearE(card->current_buffer, card->current_bytesize, CACRF_ClearD);
    
    IUtility->CallHookPkt( AudioCtrl->ahiac_PostTimerFunc, (Object*) AudioCtrl, 0 );
  }
  card->playback_interrupt_enabled = TRUE;
}


/******************************************************************************
** Record interrupt handler ***************************************************
******************************************************************************/

void
RecordInterrupt( struct ExceptionContext *pContext, struct ExecBase *SysBase, struct CardData* card )
{
  struct AHIAudioCtrlDrv* AudioCtrl = card->audioctrl;
  struct DriverBase*  AHIsubBase = (struct DriverBase*) card->ahisubbase;
  struct PCIDevice *dev = (struct PCIDevice * ) card->pci_dev;

  struct AHIRecordMessage rm =
  {
    AHIST_S16S,
    card->current_record_buffer,
    RECORD_BUFFER_SAMPLES
  };

  int   i   = 0, shorts = card->current_record_bytesize / 2;
  WORD* ptr = card->current_record_buffer;

  //Invalidate cache so that data read from DMA buffer is correct - Articia hack
  IExec->CacheClearE(card->current_record_buffer, card->current_record_bytesize, CACRF_InvalidateD);

  while( i < shorts )
  {
    *ptr = ( ( *ptr & 0xff ) << 8 ) | ( ( *ptr & 0xff00 ) >> 8 );

    ++i;
    ++ptr;
  }

  IUtility->CallHookPkt( AudioCtrl->ahiac_SamplerFunc, (Object*) AudioCtrl, &rm );

  //Invalidate cache so that data read from DMA buffer is correct - Articia hack
  IExec->CacheClearE(card->current_record_buffer, card->current_record_bytesize, CACRF_InvalidateD);

  card->record_interrupt_enabled = TRUE;
}
