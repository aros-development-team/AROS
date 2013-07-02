/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Paula audio.device
    Lang: English
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/resident.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/alerts.h>
#include <exec/tasks.h>
#include <exec/interrupts.h>
#include <devices/audio.h>
#include <hardware/intbits.h>
#include <hardware/custom.h>

#include <proto/exec.h>

#include "audio_intern.h"

void audiohw_stop(struct AudioBase *ab, UWORD mask)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;

    if (!mask)
    	return;
    custom->dmacon = mask;
    custom->intena = mask << INTB_AUD0;
}

void audiohw_prepareptlen(struct AudioBase *ab, struct IOAudio *io, UBYTE ch)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;

    if (ab->cycles[ch] != 1)
    	return;
    if (io) {
    	custom->aud[ch].ac_ptr = (UWORD*)io->ioa_Data;
    	custom->aud[ch].ac_len = io->ioa_Length / 2;
    	ab->cycles[ch] = io->ioa_Cycles;
    	D(bug("ch%d: pt=%08x len=%d cyc=%d\n", ch, io->ioa_Data, io->ioa_Length / 2, io->ioa_Cycles));
    } else {
    	custom->aud[ch].ac_ptr = ab->zerosample;
    	custom->aud[ch].ac_len = 1;
    	D(bug("ch%d: null\n", ch));
    }
    ab->initialcyclemask &= ~(1 << ch);
}

void audiohw_preparepervol(struct AudioBase *ab, struct IOAudio *io, UBYTE ch)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;

    if (io && (io->ioa_Request.io_Flags & ADIOF_PERVOL)) {
    	custom->aud[ch].ac_per = io->ioa_Period;
    	custom->aud[ch].ac_vol = io->ioa_Volume;
    	D(bug("ch%d: per=%d vol=%d\n", ch, io->ioa_Period, io->ioa_Volume));
    }
}

static void audioirq(struct AudioBase *ab, UBYTE ch)
{
    struct IOAudio *io = getnextwrite(ab, ch, FALSE);
    UBYTE mask = 1 << ch;

    D(bug("audio: ch %d interrupt, io %p %04x %04x %04x %d\n", ch, io, ab->initialcyclemask, ab->initialdmamask, ab->stopmask, ab->cycles[ch]));

    if (!io || (ab->stopmask & mask)) {
        audiohw_stop(ab, mask);
        D(bug("ch%d: finished\n", ch));
        return;
    }
    
    if (!(ab->initialdmamask & mask)) {
    	D(bug("audio: ch%d startup interrupt\n", ch));
    	ab->initialdmamask |= mask;
    	if (io->ioa_Request.io_Flags & ADIOF_WRITEMESSAGE)
    	    ReplyMsg(&io->ioa_WriteMsg);
    	io = getnextwrite(ab, ch, TRUE);
        audiohw_prepareptlen(ab, io, ch);
    } else {
        struct IOAudio *wio, *next;
    	struct IOAudio *io2 = getnextwrite(ab, ch, TRUE);

    	if (!(ab->initialcyclemask & mask)) {
    	    ab->initialcyclemask |= mask;
    	    audiohw_preparepervol(ab, io2, ch);
    	    if (io2 && (io2->ioa_Request.io_Flags & ADIOF_WRITEMESSAGE))
    	    	ReplyMsg(&io2->ioa_WriteMsg);
    	}

        if (ab->cycles[ch] == 1) {
    	    REMOVE(io);
    	    ReplyMsg(&io->ioa_Request.io_Message);
    	    io = getnextwrite(ab, ch, TRUE);
            D(bug("audio: ch%d next io %p\n", ch, io));
            audiohw_prepareptlen(ab, io, ch);
    	} else if (ab->cycles[ch]) {
	    ab->cycles[ch]--;
	}

	/* Does ADIOF_SYNCCYCLE mean end of any cycle or only when cycle count == 1?
	 * Documentation isn't clear about this ("end of current cycle")
	 * This assumes "end of any cycle"
	 */
    	ForeachNodeSafe(&ab->misclist, wio, next) {
    	    UWORD cmd = wio->ioa_Request.io_Command;
    	    UBYTE cmask = (UBYTE)(ULONG)wio->ioa_Request.io_Unit;
    	    if (cmd != ADCMD_PERVOL && cmd != ADCMD_FINISH && cmd != ADCMD_WAITCYCLE)
    	    	continue;
    	    if (!(cmask & mask))
    	    	continue;
    	    if (cmask & (mask << NR_CH)) {
    	    	if (cmd == ADCMD_PERVOL)
    	    	    audiohw_preparepervol(ab, wio, ch);
    	    }
    	    cmask &= ~(mask << NR_CH);
    	    if ((cmask >> NR_CH) == 0) {
     	        D(bug("audio: ch %d SYNCCYCLE woken up, io %p\n", ch, wio));
    	        REMOVE(wio);
    	        ReplyMsg(&wio->ioa_Request.io_Message);
    	    }
    	}

    }
}

AROS_INTH1(audio_int, struct AudioInterrupt *, ai)
{ 
    AROS_INTFUNC_INIT

    audioirq(ai->ab, ai->ch);

    return 0;

    AROS_INTFUNC_EXIT
}

void audiohw_reset(struct AudioBase *ab, UWORD mask)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    UBYTE ch;

    custom->adkcon = mask | (mask << 4);
    custom->dmacon = mask;
    custom->intena = mask << INTB_AUD0;
    custom->intreq = mask << INTB_AUD0;
    for (ch = 0; ch < 4; ch++) {
        if ((1 << ch) & mask) {
            custom->aud[ch].ac_vol = 0;
	    custom->aud[ch].ac_per = 100;
	}
    }
}

static void preparech_initial(struct AudioBase *ab, UBYTE ch)
{
    struct IOAudio *io = getnextwrite(ab, ch, FALSE);
    ab->cycles[ch] = 1;
    audiohw_prepareptlen(ab, io, ch);
    audiohw_preparepervol(ab, io, ch);
    ab->initialdmamask &= ~(1 << ch);
    ab->initialcyclemask |= 1 << ch;
}	

void audiohw_start(struct AudioBase *ab, UWORD mask)
{
    volatile struct Custom *custom = (struct Custom*)0xdff000;
    UWORD hwmask;
    UBYTE ch;

    if (!mask)
    	return;
    hwmask = 0;
    for (ch = 0; ch < NR_CH; ch++) {
    	if ((mask & (1 << ch)) && getnextwrite(ab, ch, FALSE)) {
    	    hwmask |= 1 << ch;
    	    preparech_initial(ab, ch);
    	}
    }
    D(bug("hw_start: %02x\n", hwmask));
    if (hwmask) {
    	custom->intreq = hwmask << INTB_AUD0;
    	custom->intena = INTF_SETCLR | (hwmask << INTB_AUD0);
       	custom->dmacon = 0x8000 | hwmask;
    }
}

void audiohw_init(struct AudioBase *ab)
{
    UBYTE ch;

    audiohw_reset(ab, CH_MASK);
    for (ch = 0; ch < NR_CH; ch++) {
	struct AudioInterrupt *inter = &ab->audint[ch];
	inter->audint.is_Code = (APTR)audio_int;
	inter->audint.is_Data = inter;
	inter->audint.is_Node.ln_Name = "audio";
	inter->audint.is_Node.ln_Type = NT_INTERRUPT;
	inter->ch = ch;
	inter->ab = ab;
	SetIntVector(INTB_AUD0 + ch, &inter->audint);
    }
}
