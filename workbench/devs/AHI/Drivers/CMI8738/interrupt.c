/*
The contents of this file are subject to the AROS Public License Version 1.1 (the "License");
you may not use this file except in compliance with the License. You may obtain a copy of the License at 
http://www.aros.org/license.html 
Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF 
ANY KIND, either express or implied. See the License for the specific language governing rights and 
limitations under the License. 

The Original Code is written by Davy Wentzler.
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

#define min(a,b) ((a)<(b)?(a):(b))

int z = 0;


//struct tester t[10];


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

  while ( (( intreq = ( dev->InLong(card->iobase + CMPCI_REG_INTR_STATUS ) ) ) & CMPCI_REG_ANY_INTR )!= 0 )
  {
    //IExec->DebugPrintF("INT %lx\n", intreq);
    if( intreq & CMPCI_REG_CH0_INTR && AudioCtrl != NULL )
    {
      unsigned long diff = dev->InLong(card->iobase + CMPCI_REG_DMA0_BASE) - (unsigned long) card->playback_buffer_phys;
      
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
         IExec->DebugPrintF("Delayed IRQ %lu %lu\n", diff % card->current_bytesize, card->current_bytesize);*/
        
      
      if (diff >= card->current_bytesize) //card->flip == 0) // just played buf 1
      {
         if (card->flip == 1)
            IExec->DebugPrintF("A:Missed IRQ! diff = %lu\n", diff);

         card->flip = 1;
         card->current_buffer = card->playback_buffer;
         }
      else  // just played buf 2
      {
         if (card->flip == 0)
            IExec->DebugPrintF("B:Missed IRQ! diff = %lu\n", diff);
         
         card->flip = 0;
         card->current_buffer = (APTR) ((long) card->playback_buffer + card->current_bytesize);
      }
      
      /*z++;
      if (z == 25)
        z = 0;*/
      
      card->playback_interrupt_enabled = FALSE;
      IExec->Cause( &card->playback_interrupt );
    }

    if( intreq & CMPCI_REG_CH1_INTR && AudioCtrl != NULL )
    {
      ClearMask(dev, card, CMPCI_REG_INTR_CTRL, CMPCI_REG_CH1_INTR_ENABLE);
      
      /*if (z == 0)
        IExec->DebugPrintF("rec\n");*/
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
         IExec->Cause( &card->record_interrupt );
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

    //IExec->DebugPrintF("skip_mix = %d\n", skip_mix);

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

    IExec->CacheClearE( card->current_buffer, (ULONG) dst - (ULONG) card->current_buffer, CACRF_ClearD );
    
    IUtility->CallHookPkt( AudioCtrl->ahiac_PostTimerFunc, (Object*) AudioCtrl, 0 );
  }

  WriteMask(dev, card, CMPCI_REG_INTR_CTRL, CMPCI_REG_CH0_INTR_ENABLE);
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
  WORD* ptr = (WORD *) card->current_record_buffer;


  IExec->CacheClearE( card->current_record_buffer, card->current_record_bytesize, CACRF_ClearD);

  while( i < shorts )
  {
    *ptr = ( ( *ptr & 0xff ) << 8 ) | ( ( *ptr & 0xff00 ) >> 8 );

    ++i;
    ++ptr;
  }

  IUtility->CallHookPkt( AudioCtrl->ahiac_SamplerFunc, (Object*) AudioCtrl, &rm );
  
  IExec->CacheClearE( card->current_record_buffer, card->current_record_bytesize, CACRF_ClearD);


  WriteMask(dev, card, CMPCI_REG_INTR_CTRL, CMPCI_REG_CH1_INTR_ENABLE);
  card->record_interrupt_enabled = TRUE;
}


