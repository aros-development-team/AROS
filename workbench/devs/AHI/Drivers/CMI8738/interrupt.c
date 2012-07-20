/*
The contents of this file are subject to the AROS Public License Version 1.1 (the "License");
you may not use this file except in compliance with the License. You may obtain a copy of the License at 
http://www.aros.org/license.html 
Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF 
ANY KIND, either express or implied. See the License for the specific language governing rights and 
limitations under the License. 

The Original Code is written by Davy Wentzler.
*/

//#include <config.h>

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
#include "misc.h"
#include "pci_wrapper.h"
#ifdef __AROS__
#define DEBUG 1
#include <aros/debug.h>
#define DebugPrintF bug
#endif

#define min(a,b) ((a)<(b)?(a):(b))

int z = 0;


//struct tester t[10];


/******************************************************************************
** Hardware interrupt handler *************************************************
******************************************************************************/


LONG
CardInterrupt( struct CMI8738_DATA* card )
{
  struct AHIAudioCtrlDrv* AudioCtrl = card->audioctrl;
  struct DriverBase*  AHIsubBase = (struct DriverBase*) card->ahisubbase;
  struct PCIDevice *dev = (struct PCIDevice * ) card->pci_dev;

  ULONG intreq;
  LONG  handled = 0;

    bug("[CMI8738]: %s(card @ 0x%p)\n", __PRETTY_FUNCTION__, card);
    bug("[CMI8738] %s: AHIAudioCtrlDrv @ 0x%p\n", __PRETTY_FUNCTION__, AudioCtrl);

    for (;;)
//  while (((intreq = pci_inl(CMPCI_REG_INTR_STATUS, card)) & CMPCI_REG_ANY_INTR) != 0)
  {
    intreq = pci_inl(CMPCI_REG_INTR_STATUS, card);

    bug("[CMI8738] %s: INTR_STATUS = %08x\n", __PRETTY_FUNCTION__, intreq);

      if (((intreq & CMPCI_REG_ANY_INTR) == 0) || (AudioCtrl == NULL))
	  break;

    //DebugPrintF("INT %lx\n", intreq);
    if( intreq & CMPCI_REG_CH0_INTR)
    {
      unsigned long diff = pci_inl(CMPCI_REG_DMA0_BASE, card) - (unsigned long) card->playback_buffer_phys;
      
      ClearMask(dev, card, CMPCI_REG_INTR_CTRL, CMPCI_REG_CH0_INTR_ENABLE);

      /*
      z++;
      if (z < 10)
      {
          t[z].diff = diff;
          t[z].flip = card->flip;
          t[z].oldflip = card->oldflip;
      }*/
          
      /*if ((diff > 50 && diff < card->current_bytesize) ||
          (diff > card->current_bytesize + 50 && diff < 2 * card->current_bytesize))
         DebugPrintF("Delayed IRQ %lu %lu\n", diff % card->current_bytesize, card->current_bytesize);*/
        
      
      if (diff >= card->current_bytesize) //card->flip == 0) // just played buf 1
      {
         if (card->flip == 1)
	 {
            DebugPrintF("A:Missed IRQ! diff = %lu\n", diff);
	 }

         card->flip = 1;
         card->current_buffer = card->playback_buffer;
         }
      else  // just played buf 2
      {
         if (card->flip == 0)
	 {
            DebugPrintF("B:Missed IRQ! diff = %lu\n", diff);
	 }
         
         card->flip = 0;
         card->current_buffer = (APTR) ((long) card->playback_buffer + card->current_bytesize);
      }
      
      /*z++;
      if (z == 25)
        z = 0;*/
      
      card->playback_interrupt_enabled = FALSE;
      Cause( &card->playback_interrupt );
    }

    if( intreq & CMPCI_REG_CH1_INTR)
    {
      ClearMask(dev, card, CMPCI_REG_INTR_CTRL, CMPCI_REG_CH1_INTR_ENABLE);
      
      /*if (z == 0)
        DebugPrintF("rec\n");*/
      z++;
      /*if (z == 30)
        z = 0;*/

      if( card->record_interrupt_enabled )
      {
         /* Invoke softint to convert and feed AHI with the new sample data */

         if (card->recflip == 0) // just played buf 1
         {
            card->recflip = 1;
            card->current_record_buffer = card->record_buffer;
            }
         else  // just played buf 2
         {
            card->recflip = 0;
            card->current_record_buffer = (APTR) ((long) card->record_buffer + card->current_record_bytesize);
         }

         card->record_interrupt_enabled = FALSE;
         Cause( &card->record_interrupt );
      }
    }

    handled = 1;
    
  }
  
  return handled;
}


/******************************************************************************
** Playback interrupt handler *************************************************
******************************************************************************/

void
PlaybackInterrupt( struct CMI8738_DATA* card )
{
  struct AHIAudioCtrlDrv* AudioCtrl = card->audioctrl;
  struct DriverBase*  AHIsubBase = (struct DriverBase*) card->ahisubbase;
  struct PCIDevice *dev = (struct PCIDevice * ) card->pci_dev;

    bug("[CMI8738]: %s()\n", __PRETTY_FUNCTION__);

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

    //DebugPrintF("skip_mix = %d\n", skip_mix);

    if( ! skip_mix )
    {
      CallHookPkt( AudioCtrl->ahiac_MixerFunc, (Object*) AudioCtrl, card->mix_buffer );
    }
    
    /* Now translate and transfer to the DMA buffer */


    skip    = ( AudioCtrl->ahiac_Flags & AHIACF_HIFI ) ? 2 : 1;
    samples = card->current_bytesize >> 1;

    src     = card->mix_buffer;
#if !defined(__AMIGAOS4__) && !AROS_BIG_ENDIAN
    if(skip == 2)
        src++;
#endif
    dst     = card->current_buffer;

    i = samples;


    while( i > 0 )
    {
#ifdef __AMIGAOS4__
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

  WriteMask(dev, card, CMPCI_REG_INTR_CTRL, CMPCI_REG_CH0_INTR_ENABLE);
  card->playback_interrupt_enabled = TRUE;
}


/******************************************************************************
** Record interrupt handler ***************************************************
******************************************************************************/

void
RecordInterrupt( struct CMI8738_DATA* card )
{
  struct AHIAudioCtrlDrv* AudioCtrl = card->audioctrl;
  struct DriverBase*  AHIsubBase = (struct DriverBase*) card->ahisubbase;
  struct PCIDevice *dev = (struct PCIDevice * ) card->pci_dev;

    bug("[CMI8738]: %s()\n", __PRETTY_FUNCTION__);

  struct AHIRecordMessage rm =
  {
    AHIST_S16S,
    card->current_record_buffer,
    RECORD_BUFFER_SAMPLES
  };

  int   i   = 0, shorts = card->current_record_bytesize / 2;
  WORD* ptr = (WORD *) card->current_record_buffer;


  CacheClearE( card->current_record_buffer, card->current_record_bytesize, CACRF_ClearD);

  while( i < shorts )
  {
#if defined(__AMIGAOS4__) || AROS_BIG_ENDIAN
    *ptr = ( ( *ptr & 0xff ) << 8 ) | ( ( *ptr & 0xff00 ) >> 8 );
#endif
    ++i;
    ++ptr;
  }

  CallHookPkt( AudioCtrl->ahiac_SamplerFunc, (Object*) AudioCtrl, &rm );
  
  CacheClearE( card->current_record_buffer, card->current_record_bytesize, CACRF_ClearD);


  WriteMask(dev, card, CMPCI_REG_INTR_CTRL, CMPCI_REG_CH1_INTR_ENABLE);
  card->record_interrupt_enabled = TRUE;
}


