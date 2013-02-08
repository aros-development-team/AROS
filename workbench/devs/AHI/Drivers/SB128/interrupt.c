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

#if !defined(__AROS__)
#undef __USE_INLINE__
#include <proto/expansion.h>
#endif
#include <libraries/ahi_sub.h>
#include <proto/exec.h>
#include <stddef.h>
#include "library.h"
#include "regs.h"
#include "interrupt.h"
#include "pci_wrapper.h"
#ifdef __AROS__
#include <aros/debug.h>
#endif

#define min(a,b) ((a)<(b)?(a):(b))

unsigned long z = 0;
/******************************************************************************
** Hardware interrupt handler *************************************************
******************************************************************************/


LONG
CardInterrupt( struct SB128_DATA* card )
{
  struct AHIAudioCtrlDrv* AudioCtrl = card->audioctrl;
  struct DriverBase*  AHIsubBase = (struct DriverBase*) card->ahisubbase;
  struct PCIDevice *dev = (struct PCIDevice * ) card->pci_dev;

  ULONG intreq;
  LONG  handled = 0;
    
  D(bug("[CMI8738]: %s(card @ 0x%p)\n", __PRETTY_FUNCTION__, card));

  while (((intreq = (pci_inl(SB128_STATUS, card))) & SB128_INT_PENDING) != 0)
  {
    if( intreq & SB128_INT_DAC2 && AudioCtrl != NULL )
    {
      /* Clear interrupt pending bit(s) and re-enable playback interrupts */
      pci_outl((pci_inl(SB128_SCON, card) & ~SB128_DAC2_INTEN), SB128_SCON, card);
      pci_outl((pci_inl(SB128_SCON, card) | SB128_DAC2_INTEN), SB128_SCON, card);

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
      Cause( &card->playback_interrupt );
    }

    if( intreq & SB128_INT_ADC && AudioCtrl != NULL )
    {
      /* Clear interrupt pending bit(s) and re-enable record interrupts */
      pci_outl((pci_inl(SB128_SCON, card) & ~SB128_ADC_INTEN), SB128_SCON, card);
      pci_outl((pci_inl(SB128_SCON, card) | SB128_ADC_INTEN), SB128_SCON, card);

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
         Cause( &card->record_interrupt );
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
PlaybackInterrupt( struct SB128_DATA* card )
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
    
  	skip_mix = CallHookPkt( AudioCtrl->ahiac_PreTimerFunc, (Object*) AudioCtrl, 0 );  
    CallHookPkt( AudioCtrl->ahiac_PlayerFunc, (Object*) AudioCtrl, NULL );

  if( ! skip_mix )
    {
      CallHookPkt( AudioCtrl->ahiac_MixerFunc, (Object*) AudioCtrl, card->mix_buffer );
    }
    
    /* Now translate and transfer to the DMA buffer */

    skip    = ( AudioCtrl->ahiac_Flags & AHIACF_HIFI ) ? 2 : 1;
    samples = card->current_bytesize >> 1;

    src     = card->mix_buffer;
#if !AROS_BIG_ENDIAN
    if(skip == 2)
        src++;
#endif
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
    CacheClearE(card->current_buffer, card->current_bytesize, CACRF_ClearD);
    
    CallHookPkt( AudioCtrl->ahiac_PostTimerFunc, (Object*) AudioCtrl, 0 );
  }
  card->playback_interrupt_enabled = TRUE;
}


/******************************************************************************
** Record interrupt handler ***************************************************
******************************************************************************/

void
RecordInterrupt( struct SB128_DATA* card )
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
  CacheClearE(card->current_record_buffer, card->current_record_bytesize, CACRF_InvalidateD);

  while( i < shorts )
  {
    *ptr = ( ( *ptr & 0xff ) << 8 ) | ( ( *ptr & 0xff00 ) >> 8 );

    ++i;
    ++ptr;
  }

  CallHookPkt( AudioCtrl->ahiac_SamplerFunc, (Object*) AudioCtrl, &rm );

  //Invalidate cache so that data read from DMA buffer is correct - Articia hack
  CacheClearE(card->current_record_buffer, card->current_record_bytesize, CACRF_InvalidateD);

  card->record_interrupt_enabled = TRUE;
}
