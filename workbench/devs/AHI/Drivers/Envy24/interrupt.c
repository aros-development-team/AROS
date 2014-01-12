/*
    Copyright � 2004-2014, Davy Wentzler. All rights reserved.
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
#include "pci_wrapper.h"

#define min(a,b) ((a)<(b)?(a):(b))


/******************************************************************************
** Hardware interrupt handler *************************************************
******************************************************************************/


AROS_INTH1(CardInterrupt, struct CardData *, card)
{
  AROS_INTFUNC_INIT

  struct AHIAudioCtrlDrv* AudioCtrl = card->audioctrl;
  struct DriverBase*  AHIsubBase = (struct DriverBase*) card->ahisubbase;

  unsigned char intreq;
  LONG  handled = 0;

  //DebugPrintF("INT\n");
  while ( ( intreq = ( pci_inb(CCS_INTR_STATUS, card) ) ) != 0 )
  {
    //DebugPrintF("INT %x\n", intreq);
    
    if (intreq & CCS_INTR_PRO_MACRO)
    {
       unsigned char mtstatus = pci_inb_mt(MT_INTR_MASK_STATUS, card);
       
       //DebugPrintF("CCS_INTR_PRO_MACRO, mtstatus = %x\n", mtstatus);
       
       if( (mtstatus & MT_PLAY_STATUS) && AudioCtrl != NULL )
       {
         pci_outb_mt(mtstatus | MT_PLAY_STATUS, MT_INTR_MASK_STATUS, card); // clear interrupt
        
         if (card->flip == 0) // just played buf 1
         {
            card->flip = 1;
            card->current_buffer = card->playback_buffer;
            }
         else  // just played buf 2
         {
            card->flip = 0;
            card->current_buffer = (APTR) ((long) card->playback_buffer + card->current_bytesize);
         }
         
         card->playback_interrupt_enabled = FALSE;
         Cause( &card->playback_interrupt );
       }
   
       if( (mtstatus & MT_REC_STATUS) && AudioCtrl != NULL )
       {
         pci_outb_mt(mtstatus | MT_REC_STATUS, MT_INTR_MASK_STATUS, card); // clear interrupt
         //DebugPrintF("rec\n");
   
         if( card->record_interrupt_enabled )
         {
            const unsigned long diff = pci_inl_mt(MT_DMA_REC_ADDRESS, card) - (unsigned long) card->record_buffer_32bit_phys;
            
            //DebugPrintF("%lu\n", diff % card->current_record_bytesize_32bit);
            
            /* Invoke softint to convert and feed AHI with the new sample data */
   
            if (diff >= card->current_record_bytesize_32bit) // just played buf 1
            {
               if (card->recflip == 1)
                   DebugPrintF("A: Missed IRQ!\n");
               
               card->recflip = 1;
               card->current_record_buffer = card->record_buffer;
               card->current_record_buffer_32bit = card->record_buffer_32bit;
               }
            else  // just played buf 2
            {
               if (card->recflip == 0)
                   DebugPrintF("B: Missed IRQ!\n");
                 
               card->recflip = 0;
               card->current_record_buffer = (APTR) ((unsigned long) card->record_buffer + card->current_record_bytesize_target);
               card->current_record_buffer_32bit = (APTR) ((unsigned long) card->record_buffer_32bit + card->current_record_bytesize_32bit);
            }   
            card->record_interrupt_enabled = FALSE;
            Cause( &card->record_interrupt );
         }
       }
       
    }
    else
    {
       DebugPrintF("Oh dear, it's not CCS_INTR_PLAYREC!\n");   
    }

    handled = 1;
  }

  return handled;

  AROS_INTFUNC_EXIT
}


/******************************************************************************
** Playback interrupt handler *************************************************
******************************************************************************/

AROS_INTH1(PlaybackInterrupt, struct CardData *, card)
{
  AROS_INTFUNC_INIT

  struct AHIAudioCtrlDrv* AudioCtrl = card->audioctrl;
  struct DriverBase*  AHIsubBase = (struct DriverBase*) card->ahisubbase;
  BOOL stereo = (AudioCtrl->ahiac_Flags & AHIACF_STEREO) != 0;

  if( card->mix_buffer != NULL && card->current_buffer != NULL )
  {
    BOOL   skip_mix;

    WORD*  src;
    WORD*  dst;
    size_t skip;
    size_t samples;
    int    i;
    LONG *srclong, *dstlong, left, right;
    int frames = card->current_frames;
    
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
    dst     = card->current_buffer;
    srclong = (LONG*) card->mix_buffer;
    dstlong = (LONG*) card->current_buffer;

    i = frames;

      while( i > 0 )
      {
        if (AudioCtrl->ahiac_Flags & AHIACF_HIFI)
        {
          left = AROS_LONG2LE(*srclong++);
          if (stereo)
            right = AROS_LONG2LE(*srclong++);
          else
            right = left;
        }
        else
        {
          left = AROS_LONG2LE(*src++ << 16);
          if (stereo)
            right = AROS_LONG2LE(*src++ << 16);
          else
            right = left;
        }

        *dstlong++ = left;  // out 1 - 2
        *dstlong++ = right;

        *dstlong++ = left;
        *dstlong++ = right;

        dstlong+= 4;

        // S/PDIF
        *dstlong++ = left;
        *dstlong++ = right;

        --i;
      }

    CacheClearE(card->current_buffer, (ULONG) dstlong - (ULONG) card->current_buffer, CACRF_ClearD);
    CallHookPkt( AudioCtrl->ahiac_PostTimerFunc, (Object*) AudioCtrl, 0 );
  }

  card->playback_interrupt_enabled = TRUE;

  AROS_INTFUNC_EXIT
}


/******************************************************************************
** Record interrupt handler ***************************************************
******************************************************************************/

AROS_INTH1(RecordInterrupt, struct CardData *, card)
{
  AROS_INTFUNC_INIT

  struct AHIAudioCtrlDrv* AudioCtrl = card->audioctrl;
  struct DriverBase*  AHIsubBase = (struct DriverBase*) card->ahisubbase;

  struct AHIRecordMessage rm =
  {
    AHIST_S16S,
    card->current_record_buffer,
    RECORD_BUFFER_SAMPLES
  };

     long *src = card->current_record_buffer_32bit;
     WORD* dst = card->current_record_buffer;
     int i = 0, frames = RECORD_BUFFER_SAMPLES;

     CacheClearE( card->current_record_buffer, card->current_record_bytesize_target, CACRF_ClearD);
     CacheClearE( card->current_record_buffer_32bit, card->current_record_bytesize_32bit, CACRF_ClearD);

     i = frames;

     if (card->SubType == PHASE88 || card->SubType == MAUDIO_1010LT)
     {
        src += card->input * 2;
        while( i > 0 )
        {
          *dst++ = AROS_LE2LONG(*src++) >> 16;
          *dst++ = AROS_LE2LONG(*src) >> 16;

          src+=11;

          i--;
        }
     }
     else
     {
        if (card->input != 1)
        {
           while( i > 0 )
           {
             *dst++ = AROS_LE2LONG(*src++) >> 16;
             *dst++ = AROS_LE2LONG(*src) >> 16;

             src+=11;
              
             i--;
           }
        }
        else
        {
           while( i > 0 )
           {
             src+=2;

             *dst++ = AROS_LE2LONG(*src++) >> 16;
             *dst++ = AROS_LE2LONG(*src) >> 16;

             src+=9;

             i--;
           }
        }
     }
  
  CallHookPkt( AudioCtrl->ahiac_SamplerFunc, (Object*) AudioCtrl, &rm );
  CacheClearE( card->current_record_buffer, card->current_record_bytesize_target, CACRF_ClearD);
  CacheClearE( card->current_record_buffer_32bit, card->current_record_bytesize_32bit, CACRF_ClearD);

  card->record_interrupt_enabled = TRUE;

  AROS_INTFUNC_EXIT
}


