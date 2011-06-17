/*
     Copyright 2010, The AROS Development Team. All rights reserved.
     $Id$
 */

/*
 * audio.device
 *
 * by Nexus Development 2003
 * coded by Emanuele Cesaroni
 *
 * $Id$
 */

#include "audio_intern.h"
#define DEBUG 0
#include <aros/debug.h>
#include <graphics/gfxbase.h>

/*
 * In CMD_WRITE when more than one bit is set is considered the lowest this keep fastly it.
 */
BYTE writechn[] =
{ 0, // 0000 - 0 // This combination is bad.
        0, // 0001 - 1
        1, // 0010 - 2
        0, // 0011 - 3
        2, // 0100 - 4
        0, // 0101 - 5
        1, // 0110 - 6
        0, // 0111 - 7
        3, // 1000 - 8
        0, // 1001 - 9
        1, // 1010 - 10
        0, // 1011 - 11
        2, // 1100 - 12
        0, // 1101 - 13
        1, // 1110 - 14
        0 // 1111 - 15
        };

VOID setup_Units(UBYTE combination, WORD key, BYTE pri, ETASK *eta)
{
    // se sono locked
    if (combination & (1 << 0))
    {
        ResetUnit(eta->et_units[0]);
        eta->et_units[0]->eu_allockey = key;
        eta->et_units[0]->eu_pri = pri;
        D(bug("NEWD: Setupped unit %d\n", 0));
    }

    if (combination & (1 << 1))
    {
        ResetUnit(eta->et_units[1]);
        eta->et_units[1]->eu_allockey = key;
        eta->et_units[1]->eu_pri = pri;
        D(bug("NEWD: Setupped unit %d\n", 1));
    }

    if (combination & (1 << 2))
    {
        ResetUnit(eta->et_units[2]);
        eta->et_units[2]->eu_allockey = key;
        eta->et_units[2]->eu_pri = pri;
        D(bug("NEWD: Setupped unit %d\n", 2));
    }

    if (combination & (1 << 3))
    {
        ResetUnit(eta->et_units[3]);
        eta->et_units[3]->eu_allockey = key;
        eta->et_units[3]->eu_pri = pri;
        D(bug("NEWD: Setupped unit %d\n", 3));
    }

    eta->et_unitmask = combination;
}

VOID AllocateProc(struct IOAudio *audioio, ETASK *eta)
{
    BYTE *data, len, actual, pri;
    WORD key;
    BOOL result = 0;

    D(bug("NEWD: Arrived ADCMD_ALLOCATE from IOAudio(%x),len=%d,data=%x,key=%d,pri=%d\n",
          audioio, audioio->ioa_Length, audioio->ioa_Data, audioio->ioa_AllocKey,
          audioio->ioa_Request.io_Message.mn_Node.ln_Pri));

    data = audioio->ioa_Data;
    pri = audioio->ioa_Request.io_Message.mn_Node.ln_Pri;

    if ((!(key = audioio->ioa_AllocKey)))
    {
        key = eta->et_key++; /* Always a new key please. */
    }

    if ((len = (audioio->ioa_Length & 15))) /* If len is > 0 otherwise is ok so returns unit = 0. */
    {
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // First control search a combination which doesn't subtract any other just allocated.

        do
        {
            if ((actual = *data++)) /* Actual combination. */
            {
                D(bug("NEWD: combination %d\n", actual));
                if ((actual & eta->et_unitmask) == 0)
                {
                    D(bug("NEWD: after combination %d\n", actual));
                    setup_Units(actual, key, pri, eta);
                    audioio->ioa_Request.io_Unit
                            = (struct Unit*) ((IPTR) actual);
                    audioio->ioa_Request.io_Error = IONOERROR;
                    audioio->ioa_AllocKey = key;

                    return; /* EXIT. */
                }
            }
            else
            {
                audioio->ioa_Request.io_Unit = (struct Unit*) NULL; /* Lenght is zero but a key is generated, unit is 0 and noerror. */
                audioio->ioa_Request.io_Error = IONOERROR;
                audioio->ioa_AllocKey = key;

                D(bug("NEWD: ADCMD_ALLOCATE asked combination 0\n"));

                return;
            }
        } while (--len);
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Nothing... Try now to subtract other channels with lower pri.

        D(bug("NEWD: I try to subtract\n"));

        data = audioio->ioa_Data;
        len = (audioio->ioa_Length & 15);

        do /* First control search a combination which subtracts a combination with lower pri. */
        {
            actual = *data++; /* Actual combination. */

            while (1)
            {
                if ((actual & (1 << 0)) && (eta->et_unitmask & (1 << 0)))
                {
                    if (eta->et_units[0]->eu_pri >= pri)
                        break;
                }

                if ((actual & (1 << 1)) && (eta->et_unitmask & (1 << 1)))
                {
                    if (eta->et_units[1]->eu_pri >= pri)
                        break;
                }

                if ((actual & (1 << 2)) && (eta->et_unitmask & (1 << 2)))
                {
                    if (eta->et_units[2]->eu_pri >= pri)
                        break;
                }
                if ((actual & (1 << 3)) && (eta->et_unitmask & (1 << 3)))
                {
                    if (eta->et_units[3]->eu_pri >= pri)
                        break;
                }

                result = 1;
                break;
            }

            if (result)
            {
                /*
                 Devi semplicemente per i canali sottratti abortire le richieste di CMD_WRITE che giungevano dai task di
                 quelle unit� pagina 126, mettendo in error IOERR_ABORTED. Quelle in esecuzione invece vengono interrotte, quella
                 cached anche ma non viene messo niente in error.
                 */

                setup_Units(actual, key, pri, eta);
                audioio->ioa_Request.io_Unit = (struct Unit*) ((IPTR) actual);
                audioio->ioa_Request.io_Error = IONOERROR;
                audioio->ioa_AllocKey = key;

                return;
            }
        } while (--len);

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Ok, nothing to do, so or exit or put the IOAudio in waiting state.

        if (audioio->ioa_Request.io_Flags & ADIOF_NOWAIT)
        {
            audioio->ioa_Request.io_Unit = (struct Unit*) NULL;
            audioio->ioa_Request.io_Error = ADIOERR_ALLOCFAILED;
            D(bug("NEWD: I try to subtract\n"));

            return;
        }
        else
        {
            AddTail(&eta->et_allocatelist,
                    &audioio->ioa_Request.io_Message.mn_Node); /* This now in waiting state */
            eta->et_allocated++; /* Increase the counter please. */
            audioio->ioa_Request.io_Flags &= ~IOF_QUICK; /* Try this later. */
            audioio->ioa_Request.io_Error = ADIOERR_ALLOCFAILED;
            D(bug("NEWD: ADCMD_ALLOCATE put in waiting allocate list IOAudio '%x'\n",
                  audioio));

            return;
        }
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    audioio->ioa_Request.io_Unit = (struct Unit*) NULL; // Lenght is zero but a key is generated, unit is 0 and noerror.
    audioio->ioa_Request.io_Error = IONOERROR;
    audioio->ioa_AllocKey = key;
    D(bug("NEWD: Arrived ADCMD_ALLOCATE with ioa_lenght == 0 IOAudio (%x)\n",
            audioio));
}

/*
 * _ADCMD_ALLOCATE
 *
 * If the IOAudio can't be processed put the request at the end of the et_allocatelist.
 *
 * TODO
 * - What to do when the lenght is > 15 ? Actually this condition is not present because the lenght is anded with 15.
 * - Do the procedure of removing the cmd_writes when channels are stolens.
 * - Do the part of reput the request in a list in which must stay waiting for reallocs.
 * - Pay attention if abortio aborts an ADCMD_ALLOCATE it is removed and the counter of alloc waiting is wrong.
 *
 */

VOID _ADCMD_ALLOCATE(struct IOAudio *audioio, ETASK *eta)
{
    AllocateProc(audioio, eta);
    Signal(audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask, STOLEN_SIG);
}

/*
 *
 *  UnitCopyData
 * --------------
 *
 * Copies the data from the audio to the unit fields. This is a pre-cached system as the original audio.device used to do.
 *
 */

VOID UnitCopyData(EUNIT *unit, struct IOAudio *audioio)
{
    ULONG period;
    if (audioio->ioa_Length > 131072)
    {
        unit->eu_len = 131072;
    }
    else
    {
        if (audioio->ioa_Length < 2)
        {
            unit->eu_len = 2;
        }
        else
        {
            unit->eu_len = audioio->ioa_Length;
        }
    }

    if ((period = audioio->ioa_Period) < 124)
        period = 124;

    unit->eu_data = audioio->ioa_Data;
    unit->eu_cycles = audioio->ioa_Cycles;

    if (audioio->ioa_Request.io_Flags & ADIOF_PERVOL)
    {
        unit->eu_volume = audioio->ioa_Volume * 1024;
        //unit->eu_freq = unit->eu_clock / ((ULONG) audioio->ioa_Period);
        unit->eu_freq = unit->eu_clock / period;
    }

    unit->eu_audioio = audioio;
    //audioio->ioa_Request.io_Flags &= ~IOF_QUICK; // This will be outputted surely so treat it as async.

}

/*
 *
 *  UnitAHIPerVol
 * ---------------
 *
 * Do the necessary procedures to change on the fly period and volume into an unit.
 *
 */

VOID UnitAHIPerVol(EUNIT *unit, struct IOAudio *audioio)
{
    unit->eu_freq = unit->eu_clock / ((ULONG) audioio->ioa_Period);
    unit->eu_volume = audioio->ioa_Volume * 1024;
}

/*
 *  UnitInitAhi
 * -------------
 *
 * Inits ahi and play the sample.
 * It removes the data from the precached place as the original audio.device used to do.
 *
 */

VOID UnitInitAhi(EUNIT *unit)
{
    if (!unit->eu_usingme)
    {
        unit->eu_ahireq.ahir_Std.io_Data = unit->eu_data;
        unit->eu_ahireq.ahir_Std.io_Length = unit->eu_len;
        unit->eu_ahireq.ahir_Frequency = unit->eu_freq;
        unit->eu_ahireq.ahir_Volume = unit->eu_volume;
        unit->eu_usingme = unit->eu_audioio;
        unit->eu_audioio = NULL;
        unit->eu_savecycles = unit->eu_cycles; // The save value.
        unit->eu_actcycles = unit->eu_cycles; // The actual counter.

        if (unit->eu_savecycles != 1) // Save the values for infinite repeating...
        {
            unit->eu_repdata = unit->eu_data;
            unit->eu_replen = unit->eu_len;
            unit->eu_repfreq = unit->eu_freq;
            unit->eu_repvolume = unit->eu_volume;
        }

        BeginIO((struct IORequest *) &unit->eu_ahireq);

        if (unit->eu_usingme->ioa_Request.io_Flags & ADIOF_WRITEMESSAGE)
        {
            D(bug("NEWD: Reply WRITE_MESSAGE from IOAudio (%x)\n",
                    unit->eu_usingme));
            ReplyMsg(&unit->eu_usingme->ioa_WriteMsg);
        }
    }
}

/*
 *  UnitInitRepAhi
 * ----------------
 *
 * This is used to reinit ahi when we are under infinite cycles.
 */

VOID UnitInitRepAhi(EUNIT *unit)
{
    unit->eu_ahireq.ahir_Std.io_Data = unit->eu_repdata;
    unit->eu_ahireq.ahir_Std.io_Length = unit->eu_replen;
    unit->eu_ahireq.ahir_Frequency = unit->eu_repfreq;
    unit->eu_ahireq.ahir_Volume = unit->eu_repvolume;
    BeginIO((struct IORequest *) &unit->eu_ahireq);
}

/*
 *  FlushUnit
 * -----------
 *
 * Flushes an eaudio.device unit. So:
 * 1) Aborts the running audio wave.
 * 2) Get all the CMD_WRITE in list waiting repling them to their task with IOERR_ABORTED.
 * 3) Returns all the waitcycles in waitcyclelist with IOERR_ABORTED.
 *
 * ?? The running IOAudio must be replied??
 *
 */

VOID FlushUnit(EUNIT *unit)
{
    struct IOAudio *audioio;

    if (!(CheckIO((struct IORequest *) &unit->eu_ahireq))) // I stop if exists the running audio output.
    {
        AbortIO((struct IORequest *) &unit->eu_ahireq);
        WaitIO((struct IORequest *) &unit->eu_ahireq);

        D(bug("NEWD: FlushUnit(): The audio in port (%d) was running now is stopped\n",
                unit->eu_id));
    }

    unit->eu_usingme = NULL;

    while ((audioio = (struct IOAudio*) GetMsg(unit->eu_port))) // Now removes all the CMD_WRITE present in the list.
    {
        audioio->ioa_Request.io_Error = IOERR_ABORTED;
        ReplyMsg(&audioio->ioa_Request.io_Message);

        D(bug("NEWD: FlushUnit(): Removed from the list of port (%d) the CMD_WRITE request (%x).\n",
            unit->eu_id, audioio));
    }

    while ((audioio = (struct IOAudio*) RemHead(&unit->eu_waitcyclelist))) // Removes all the ADCMD_WAITCYCLES for this unit.
    {
        D(bug("NEWD: FlushUnit(): Found IOAuido (%x) in waitcyclelist, now i reply it with IOERR_ABORTED.\n",
            audioio));

        audioio->ioa_Request.io_Error = IOERR_ABORTED;
        ReplyMsg(&audioio->ioa_Request.io_Message);
    }
}

/*
 *
 * _CMD_RESET
 *
 */

VOID _CMD_RESET(struct IOAudio *audioio, ETASK *eta)
{
    BYTE unit, chan, final = 0;
    EUNIT *channel;
    UBYTE error = IONOERROR;

    D(bug("NEWD: Arrived CMD_RESET from IOAudio (%x), unit(%d)\n", audioio,
            audioio->ioa_Request.io_Unit));

    if ((unit = (((IPTR) audioio->ioa_Request.io_Unit) & 15)))
    {
        do
        {
            chan = writechn[unit];
            channel = eta->et_units[chan];

            if (channel->eu_allockey == audioio->ioa_AllocKey)
            {
                FlushUnit(channel);
                D(bug("NEWD: CMD_FLUSH for unit (%d)\n", chan));
            }
            else
            {
                error = ADIOERR_NOALLOCATION;
            }

            unit &= ~(1 << chan);
            final |= 1 << chan;
        } while (unit);
    }

    audioio->ioa_Request.io_Unit = (struct Unit*) ((IPTR) final);
    audioio->ioa_Request.io_Error = error;
    Signal(audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask, 1
            << audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigBit);
}

/*
 *  ResetUnit
 * -----------
 *
 * Does some procedures usefull to reset the unit, Called by CMD_RESET and other.
 *
 */

VOID ResetUnit(EUNIT *unit)
{
    unit->eu_audioio = NULL; /* This is sufficent to erase the pre-cache requestes. */
    FlushUnit(unit); /* This stops the running audio and removes all the quequed CMD_WRITE and ADCMD_WAITCYCLES. */
}

/*
 *
 *  ReAllocateUnits
 * -----------------
 *
 * This block try again to ADCMD_ALLOCATE some IOAUdio request which are waiting for channels. Actually it is called from
 * ADCMD_FREE or ADCMD_SETPREC.
 *
 * - Pay attention to the counter of the allocate in waiting if an abortio removes it the system collapses !!!!!!!!
 *
 */

VOID ReAllocateUnits(ETASK *eta)
{
    UWORD allocated;
    struct IOAudio *wait;

    if ((allocated = eta->et_allocated)) // If any in list waiting for being allocated.
    {
        while ((wait = (struct IOAudio*) RemHead(&eta->et_allocatelist)))
        {
            eta->et_allocated--;

            D(bug("NEWD: ADCMD_FREE: Trying to allocate this again. IOAudio (%x)\n", wait));
            AllocateProc(wait, eta);

            if ((wait->ioa_Request.io_Flags & IOF_QUICK) == 0)
            {
                if ((wait->ioa_Request.io_Error == ADIOERR_ALLOCFAILED)
                        && (wait->ioa_Request.io_Flags & ADIOF_NOWAIT))
                {
                    ReplyMsg(&wait->ioa_Request.io_Message);
                }
                else if (wait->ioa_Request.io_Error == IONOERROR)
                {
                    ReplyMsg(&wait->ioa_Request.io_Message);
                }
            }

            if (--allocated == 0)
            {
                break; // Decrease the number in list and exit if empty.
            }
        }
    }
}

/*
 * _ADCMD_FREE
 *
 * Frees some units then:
 * 1) See if other ADCMD_ALLOCATE are in waiting state.
 *
 */

VOID _ADCMD_FREE(struct IOAudio *audioio, ETASK *eta)
{
    BYTE unit, chan, final = 0;
    EUNIT *channel;
    UBYTE error = IONOERROR;

    D(bug("NEWD: Arrived CMD_FREE from IOAudio (%x), unit(%d)\n", audioio,
            audioio->ioa_Request.io_Unit));

    if ((unit = (((IPTR) audioio->ioa_Request.io_Unit) & 15)))
    {
        do
        {
            chan = writechn[unit];
            channel = eta->et_units[chan];

            if (channel->eu_allockey == audioio->ioa_AllocKey)
            {
                //if(accessibili)
                {
                    ResetUnit(channel); // Reset this one please.
                    eta->et_unitmask &= ~(1 << chan); // This channel is now free.
                    channel->eu_allockey = 0; // Not needed!
                    channel->eu_pri = -127; // Not needed!

                    D(bug("NEWD: CMD_FREE freed unit (%d)\n", chan));
                }
                //else	error = ADIOERR_NOALLOCATION;
            }
            else
            {
                error = ADIOERR_NOALLOCATION;
            }

            unit &= ~(1 << chan);
            final |= 1 << chan;
        } while (unit);
    }

    D(bug("NEWD: Now i reply CMD_FREE from IOAudio (%x), unit(%d)\n", audioio,
            audioio->ioa_Request.io_Unit));

    audioio->ioa_Request.io_Unit = (struct Unit*) (IPTR) final;
    audioio->ioa_Request.io_Error = error;

    //Signal(audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask,STOLEN_SIG);
    Signal(ematsys->ts_calltask, STOLEN_SIG);

    if (final) // Only if something now is freed touch the allocating waiting for...
    {
        ReAllocateUnits(eta);
        // verificare ora il discorso dei locks.
    }

}

/*
 *
 * _CMD_WRITE
 *
 * See again the part of bad allockkey the signal there is wrong.
 */

VOID _CMD_WRITE(struct IOAudio *audioio, ETASK *eta, EUNIT *unit)
{
    struct IOAudio *theone;

    D(bug("NEWD: Arrived CMD_WRITE from IOAudio (%x), emaunit (%d), cycles (%d), flags (%d)\n",
        audioio, unit->eu_id, audioio->ioa_Cycles, audioio->ioa_Request.io_Flags));
    D(bug("NEWD: Arrived CMD_WRITE from IOAudio (%x), period (%d), len (%d), pri (%d) key (%d)\n",
        audioio, audioio->ioa_Period, audioio->ioa_Length,
        audioio->ioa_Request.io_Message.mn_Node.ln_Pri, audioio->ioa_AllocKey));
    D(bug("NEWD: Arrived CMD_WRITE from IOAudio (%x), data (%d) volume (%d)\n",
            audioio, audioio->ioa_Data, audioio->ioa_Volume));

    //D(bug("datalist\n",0, 0,0,0,0));
    //ablo = audioio->ioa_Data;
    //do
    //{
    //	D(bug("%d ",*ablo,0,0,0,0));
    //
    //}while((counter++) < 100);
    //D(bug("\n",0, 0,0,0,0));

    if (audioio->ioa_AllocKey == unit->eu_allockey)
    {
        theone = audioio;
        audioio->ioa_Request.io_Flags &= ~IOF_QUICK;
        AddTail(&unit->eu_writewaitlist,
                &audioio->ioa_Request.io_Message.mn_Node); // Put the new one in the tail of the list.
        audioio = (struct IOAudio*) RemHead(&unit->eu_writewaitlist); // Pick the head.

        if ((unit->eu_status & UNIT_STOP) == 0) // If the unit isn't stopped.
        {
            if ((unit->eu_usingme == NULL) && (unit->eu_audioio == NULL)) // Precache free, executing free.
            {
                UnitCopyData(unit, audioio);
                UnitInitAhi(unit);

                if ((audioio = (struct IOAudio*) RemHead(
                        &unit->eu_writewaitlist)))
                {
                    UnitCopyData(unit, audioio);
                }

                return;
            }

            if ((unit->eu_usingme != NULL) && (unit->eu_audioio == NULL))
            {
                UnitCopyData(unit, audioio);
                return;
            }

            if ((unit->eu_usingme == NULL) && (unit->eu_audioio != NULL))
            {
                UnitInitAhi(unit);
                UnitCopyData(unit, audioio);

                return;
            }
        }

        AddHead(&unit->eu_writewaitlist,
                &audioio->ioa_Request.io_Message.mn_Node); // Reput if channel is stopped or cache and running are busy.

        return;
    }
    else
    {
        D(bug("NEWD: CMD_WRITE bad allockey or inaccessible channel, IOAudio (%x), key (%d)\n",
            audioio, audioio->ioa_AllocKey));
        audioio->ioa_Request.io_Unit = NULL;
        audioio->ioa_Request.io_Error = ADIOERR_NOALLOCATION;

        Signal(audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask, 1
                << audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigBit);

        if ((audioio->ioa_Request.io_Flags & IOF_QUICK) == 0)
        {
            ReplyMsg(&audioio->ioa_Request.io_Message);
        }
    }
}

/*
 *  process_UnitsCmd
 * ------------------
 *
 * Processes all the commands which interacts with all the units.
 *
 */

VOID process_UnitsCmd(struct IOAudio* MyIORequest, ETASK *eta)
{
    switch (MyIORequest->ioa_Request.io_Command)
    {
    case ADCMD_ALLOCATE:
        _ADCMD_ALLOCATE(MyIORequest, eta);
        break;

    case ADCMD_FREE:
        _ADCMD_FREE(MyIORequest, eta);
        break;

    case ADCMD_PERVOL:
        _ADCMD_PERVOL(MyIORequest, eta);
        break;

    case ADCMD_SETPREC:
        _ADCMD_SETPREC(MyIORequest, eta);
        break;

    case ADCMD_FINISH:
        _ADCMD_FINISH(MyIORequest, eta);
        break;

    case ADCMD_WAITCYCLE:
        _ADCMD_WAITCYCLE(MyIORequest, eta);
        break;

    case ADCMD_LOCK:
        _ADCMD_LOCK(MyIORequest, eta);
        break;

    case CMD_FLUSH:
        _CMD_FLUSH(MyIORequest, eta);
        break;

    case CMD_RESET:
        _CMD_RESET(MyIORequest, eta);
        break;

    case CMD_STOP:
        _CMD_STOP(MyIORequest, eta);
        break;

    case CMD_START:
        _CMD_START(MyIORequest, eta);
        break;

    case CMD_UPDATE:
        _CMD_UPDATE(MyIORequest, eta);
        break;

    case CMD_CLEAR:
        _CMD_CLEAR(MyIORequest, eta);
        break;

    case CMD_READ:
        _CMD_READ(MyIORequest, eta);
        break;

    case CMD_ABORTIO: // This is a my command.
        _CMD_ABORTIO(MyIORequest, eta);
        break;

    case CMD_CLOSE: // This is a my command.
        _CMD_CLOSE(MyIORequest, eta);
        break;
    }
}

/*
 *  free_ETask
 * ------------
 *
 * Frees the ETASK system, all ports, closes the device etc..
 *
 */

VOID free_ETask(ETASK *eta)
{
    CloseLibrary((struct Library*) eta->et_gfxbase);

    free_EUnit(eta, eta->et_units[3]); // Free the units.
    free_EUnit(eta, eta->et_units[2]);
    free_EUnit(eta, eta->et_units[1]);
    free_EUnit(eta, eta->et_units[0]);

    if (!CheckIO((struct IORequest *) eta->et_openahireq))
    {
        AbortIO((struct IORequest *) eta->et_openahireq);
    }

    CloseDevice((struct IORequest *) eta->et_openahireq);
    DeleteIORequest((struct IORequest *) eta->et_openahireq);
    DeleteMsgPort((struct MsgPort*) eta->et_portahi);
    DeleteMsgPort((struct MsgPort*) eta->et_portunits);
    DeleteMsgPort((struct MsgPort*) eta->et_portunit3);
    DeleteMsgPort((struct MsgPort*) eta->et_portunit2);
    DeleteMsgPort((struct MsgPort*) eta->et_portunit1);
    DeleteMsgPort((struct MsgPort*) eta->et_portunit0);
    enode_FreeNode((ENODE*) eta);
}

/*
 *  init_ETask
 * ------------
 *
 * Initilizes the new style of manage the problem, so it allocs the unique task structure with 4 msgports for audio channels, 1 ports
 * for manage requests for every channels, 4 structs for the units.
 *
 */

ETASK *init_ETask( VOID)
{
    ETASK *eta;

    if ((eta = (ETASK *) enode_AllocNode(sizeof(ETASK),
            (unsigned long int) FindTask(NULL))))
    {
        eta->et_key = 1;
        eta->et_unitmask = 0;

        if ((eta->et_portunit0 = (struct MsgPort*) CreateMsgPort()))
        {
            if ((eta->et_portunit1 = (struct MsgPort*) CreateMsgPort()))
            {
                if ((eta->et_portunit2 = (struct MsgPort*) CreateMsgPort()))
                {
                    if ((eta->et_portunit3 = (struct MsgPort*) CreateMsgPort()))
                    {
                        if ((eta->et_portunits
                                = (struct MsgPort*) CreateMsgPort()))
                        {
                            if ((eta->et_portahi
                                    = (struct MsgPort*) CreateMsgPort()))
                            {
                                if ((eta->et_openahireq
                                        = (struct AHIRequest*) CreateIORequest(
                                                eta->et_portahi,
                                                sizeof(struct AHIRequest))))
                                {
                                    eta->et_openahireq->ahir_Version = 4;

                                    if (!(OpenDevice(
                                            "ahi.device",
                                            AHI_DEFAULT_UNIT,
                                            (struct IORequest*) eta->et_openahireq,
                                            0)))
                                    {
                                        if ((eta->et_gfxbase
                                                = (struct GfxBase*) OpenLibrary(
                                                        "graphics.library", 0L)))
                                        {
                                            if (eta->et_gfxbase->DisplayFlags
                                                    & PAL)
                                            {
                                                eta->et_clock = 3546895;
                                            }
                                            else
                                            {
                                                eta->et_clock = 3579545;
                                            }

                                            if ((eta->et_units[0] = init_EUnit(
                                                    eta, 1 << 0,
                                                    eta->et_portunit0)))
                                            {
                                                if ((eta->et_units[1]
                                                        = init_EUnit(
                                                                eta,
                                                                1 << 1,
                                                                eta->et_portunit1)))
                                                {
                                                    if ((eta->et_units[2]
                                                            = init_EUnit(
                                                                    eta,
                                                                    1 << 2,
                                                                    eta->et_portunit2)))
                                                    {
                                                        if ((eta->et_units[3]
                                                                = init_EUnit(
                                                                        eta,
                                                                        1 << 3,
                                                                        eta->et_portunit3)))
                                                        {
                                                            eta->et_ports[0]
                                                                    = eta->et_portunit0;
                                                            eta->et_ports[1]
                                                                    = eta->et_portunit1;
                                                            eta->et_ports[2]
                                                                    = eta->et_portunit2;
                                                            eta->et_ports[3]
                                                                    = eta->et_portunit3;
                                                            NewList(
                                                                    &eta->et_allocatelist); // The allocating waiting for.
                                                            eta->et_allocated
                                                                    = 0; // No one in list.

                                                            return eta;

                                                        }

                                                        free_EUnit(
                                                                eta,
                                                                eta->et_units[2]);
                                                    }

                                                    free_EUnit(eta,
                                                            eta->et_units[1]);
                                                }

                                                free_EUnit(eta,
                                                        eta->et_units[0]);
                                            }

                                            CloseLibrary(
                                                    (struct Library*) eta->et_gfxbase);
                                        }

                                        if (!(CheckIO(
                                                (struct IORequest*) eta->et_openahireq)))
                                        {
                                            AbortIO(
                                                    (struct IORequest*) eta->et_openahireq);
                                        }

                                        CloseDevice(
                                                (struct IORequest*) eta->et_openahireq);
                                    }

                                    DeleteIORequest(
                                            (struct IORequest *) eta->et_openahireq);
                                }

                                DeleteMsgPort((struct MsgPort*) eta->et_portahi);
                            }

                            DeleteMsgPort((struct MsgPort*) eta->et_portunits);
                        }

                        DeleteMsgPort((struct MsgPort*) eta->et_portunit3);
                    }

                    DeleteMsgPort((struct MsgPort*) eta->et_portunit2);
                }

                DeleteMsgPort((struct MsgPort*) eta->et_portunit1);
            }

            DeleteMsgPort((struct MsgPort*) eta->et_portunit0);
        }

        enode_FreeNode((ENODE*) eta);
    }

    return NULL;
}

/*
 *
 *  free_EUnit
 * ------------
 *
 * Deletes the unit.
 *
 */

VOID free_EUnit(ETASK *estruct, EUNIT *unit)
{
    D(
        if(IsListEmpty(&unit->eu_writewaitlist))
        {
            D(bug("NEWD: free_Unit() writewaitlist is empty (unit %d).\n", unit->eu_id));
        }
        else
        {
            D(bug("NEWD: free_Unit() writewaitlist is NOT empty (unit %d).\n", unit->eu_id));
        }

        if (IsListEmpty(&unit->eu_waitcyclelist))
        {
            D(bug("NEWD: free_Unit() waitcyclelist is empty (unit %d).\n", unit->eu_id));
        }
        else
        {
            D(bug("NEWD: free_Unit() waitcyclelist is NOT empty (unit %d).\n", unit->eu_id));
        } 
    );

    if (!(CheckIO((struct IORequest *) &unit->eu_ahireq)))
    {
        AbortIO((struct IORequest *) &unit->eu_ahireq);
    }

    DeleteIORequest((struct IORequest *) &unit->eu_ahireq);
}

/*
 *
 *  init_EUnit
 * ------------
 *
 * Initializes the unit block. The unit is just an iorequest to interact with AHI or TIMER device.
 * 	Returns the unit or NULL if fails.
 *
 * >Syn
 * *unit = InitEUnit(*etask,channel,*msgport)
 */

EUNIT *init_EUnit(ETASK *estruct, UBYTE channel, struct MsgPort *myport)
{
    EUNIT *unit;

    if ((unit = (EUNIT*) CreateIORequest(estruct->et_portahi, sizeof(EUNIT))))
    {
        NewList(&unit->eu_writewaitlist); // All the ones under writing.
        NewList(&unit->eu_waitcyclelist); // All the ones under waitcycle.
        unit->eu_clock = estruct->et_clock;
        unit->eu_id = writechn[channel];
        unit->eu_port = myport;
        unit->eu_usingme = NULL;
        unit->eu_audioio = NULL;
        unit->eu_status = 0; // Status ready.
        unit->eu_ahireq.ahir_Std.io_Message.mn_Node.ln_Pri = 0;
        unit->eu_ahireq.ahir_Std.io_Command = CMD_WRITE;
        unit->eu_ahireq.ahir_Std.io_Offset = 0;
        unit->eu_ahireq.ahir_Type = AHIST_M8S;
        unit->eu_ahireq.ahir_Link = NULL;
        unit->eu_ahireq.ahir_Std.io_Message.mn_ReplyPort = estruct->et_portahi;
        unit->eu_ahireq.ahir_Std.io_Device
                = estruct->et_openahireq->ahir_Std.io_Device;
        unit->eu_ahireq.ahir_Std.io_Unit
                = estruct->et_openahireq->ahir_Std.io_Unit;

        if (channel & (1 << 1 | 1 << 2))
        {
            unit->eu_ahireq.ahir_Position = 0x10000; // right.
        }
        else
        {
            unit->eu_ahireq.ahir_Position = 0; // left.
        }

        return unit;
    }

    return NULL;
}

/*
 *  TaskBody
 * ----------
 *
 * The slave process main function.
 */

VOID TaskBody( VOID)
{
    ULONG waitsignal, signalset;
    struct IOAudio *ioaudioreq;
    ETASK *eta;
    EUNIT *unit;
    BOOL close = TRUE;

    if ((eta = init_ETask()))
    {
        global_eta = eta;
        eta->et_slavetask = FindTask(NULL);
        ematsys->ts_initio->ioa_Request.io_Error = IONOERROR;
        Signal(ematsys->ts_opentask, STOLEN_SIG);

        D(bug("NEWD: SLAVE PROCESS OPERATING.....\n"));

        waitsignal = (1 << eta->et_portunit0->mp_SigBit) | (1
                << eta->et_portunit1->mp_SigBit) | (1
                << eta->et_portunit2->mp_SigBit) | (1
                << eta->et_portunit3->mp_SigBit) | (1
                << eta->et_portunits->mp_SigBit) | (1
                << eta->et_portahi->mp_SigBit);

        while (close)
        {
            signalset = Wait(waitsignal);

            if (signalset & (1 << eta->et_portunit0->mp_SigBit)) // PORT 0.
            {
                while ((ioaudioreq
                        = (struct IOAudio*) GetMsg(eta->et_portunit0)))
                {
                    _CMD_WRITE(ioaudioreq, eta, eta->et_units[0]);
                    Signal(ioaudioreq->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask,
                            STOLEN_SIG);
                }
            }
            if (signalset & (1 << eta->et_portunit1->mp_SigBit)) // PORT 1.
            {
                while ((ioaudioreq
                        = (struct IOAudio*) GetMsg(eta->et_portunit1)))
                {
                    _CMD_WRITE(ioaudioreq, eta, eta->et_units[1]);
                    Signal(ioaudioreq->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask,
                            STOLEN_SIG);
                }
            }
            if (signalset & (1 << eta->et_portunit2->mp_SigBit)) // PORT 2.
            {
                while ((ioaudioreq
                        = (struct IOAudio*) GetMsg(eta->et_portunit2)))
                {
                    _CMD_WRITE(ioaudioreq, eta, eta->et_units[2]);
                    Signal(ioaudioreq->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask,
                            STOLEN_SIG);
                }
            }
            if (signalset & (1 << eta->et_portunit3->mp_SigBit)) // PORT 3.
            {
                while ((ioaudioreq
                        = (struct IOAudio*) GetMsg(eta->et_portunit3)))
                {
                    _CMD_WRITE(ioaudioreq, eta, eta->et_units[3]);
                    Signal(ioaudioreq->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask,
                            STOLEN_SIG);
                }
            }
            //--------------------------------------------------------------------------------------------------------------------------
            if (signalset & (1 << eta->et_portahi->mp_SigBit)) // AHI REPLY PORT.
            {
                while ((unit = (EUNIT*) GetMsg(eta->et_portahi)))// reply the I/Os.
                {
                    //D(bug("NEWD: this is the real ahi return unit(%d), ioreq (%d)\n",unit->eu_id, unit->eu_usingme));

                    if ((ioaudioreq = unit->eu_usingme)) // This one is ended.
                    {
                        if (unit->eu_savecycles) // No infinite in this != 0.
                        {
                            if (--unit->eu_actcycles == 0)
                            {
                                IPTR tmpunit =
                                        (IPTR) ioaudioreq->ioa_Request.io_Unit;
                                tmpunit |= 1 << unit->eu_id;
                                ioaudioreq->ioa_Request.io_Unit
                                        = (APTR) tmpunit;

                                ioaudioreq->ioa_Request.io_Error = IONOERROR;

                                ReplyMsg(&ioaudioreq->ioa_Request.io_Message);

                                D(
                                        bug(
                                                "NEWD: AHI RETURNS  Unit(%d), ioreq (%d)\n",
                                                unit->eu_id, ioaudioreq,
                                                0));

                                /*
                                 unit->eu_usingme = NULL; // This informs that the unit now is free.

                                 if(unit->eu_audioio)
                                 {
                                 UnitInitAhi(unit); // Ok send soon now the pre-cached.
                                 }

                                 while(ioaudioreq = (struct IOAudio*)  RemHead(&unit->eu_writewaitlist))	  // Examine for this unit only one request in waiting status.
                                 {
                                 D(bug("NEWD: Founded ioreq (%d) in writewaitlist, now i process it.\n",ioaudioreq,0,0,0,0));

                                 _CMD_WRITE(ioaudioreq,eta,unit);

                                 if(unit->eu_audioio)
                                 {
                                 break;			// If the precached is empty you can exit.
                                 }
                                 }
                                 */
                                unit->eu_usingme = NULL;

                                if (unit->eu_audioio)
                                {
                                    UnitInitAhi(unit);

                                    if ((ioaudioreq
                                            = (struct IOAudio*) RemHead(
                                                    &unit->eu_writewaitlist)))
                                    {
                                        UnitCopyData(unit, ioaudioreq);

                                        D(bug("NEWD: PUT ioreq (%x) IN PRECACHE\n", ioaudioreq));
                                    }
                                }
                                else
                                {
                                    if ((ioaudioreq
                                            = (struct IOAudio*) RemHead(
                                                    &unit->eu_writewaitlist)))
                                    {
                                        UnitCopyData(unit, ioaudioreq);
                                        UnitInitAhi(unit);

                                        D(bug("NEWD: EXECUTED ioreq (%x)\n", ioaudioreq));

                                        if ((ioaudioreq
                                                = (struct IOAudio*) RemHead(
                                                        &unit->eu_writewaitlist)))
                                        {
                                            UnitCopyData(unit, ioaudioreq);

                                            D(bug("NEWD: PUT ioreq (%x) IN PRECACHE\n", ioaudioreq));
                                        }
                                    }
                                }
                            }
                            else
                            {
                                UnitInitRepAhi(unit);
                            }
                        }
                        else // This zero infinite loop.
                        {
                            UnitInitRepAhi(unit);
                        }

                        ReplyWaitCycles(unit, eta); // Reply to whom was waiting for a cycle end.
                    }
                }
            }
            //--------------------------------------------------------------------------------------------------------------------------
            if (signalset & (1 << eta->et_portunits->mp_SigBit)) // ALL THE PORTS.
            {
                while ((ioaudioreq
                        = (struct IOAudio*) GetMsg(eta->et_portunits)))
                {
                    process_UnitsCmd(ioaudioreq, eta);

                    if (ioaudioreq->ioa_Request.io_Command == CMD_CLOSE)
                    {
                        D(bug("NEWD: RECEIVED CLOSE\n"));
                        close = FALSE;
                        break;
                    }
                }
            }
            //--------------------------------------------------------------------------------------------------------------------------
        }
    }
    else
    {
        ematsys->ts_initio->ioa_Request.io_Error = IOERR_OPENFAIL; // Wrong! Inform my father...
        Signal(ematsys->ts_initio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask,
                1 << ematsys->ts_initio->ioa_Request.io_Message.mn_ReplyPort->mp_SigBit);
    }
}

VOID UnitReWrite(EUNIT *unit)
{
    struct IOAudio *ioaudioreq;

    if ((unit->eu_audioio != NULL) && (unit->eu_usingme == NULL))
    {
        UnitInitAhi(unit);

        D(bug("NEWD: Cache presente ho initializzato unit %d\n", unit->eu_id));

        if ((ioaudioreq = (struct IOAudio*) RemHead(&unit->eu_writewaitlist)))
        {
            UnitCopyData(unit, ioaudioreq);
        }
    }
    else if ((unit->eu_audioio == NULL) && (unit->eu_usingme != NULL))
    {
        D(bug("NEWD: Cache nulla ma ioreq in function %d\n", unit->eu_id));

        if ((ioaudioreq = (struct IOAudio*) RemHead(&unit->eu_writewaitlist)))
        {
            UnitCopyData(unit, ioaudioreq);
        }

    }
    else if ((unit->eu_audioio == NULL) && (unit->eu_usingme == NULL))
    {
        D(bug("NEWD: Tutto vuoto provo l'init in unit %d\n", unit->eu_id));

        if ((ioaudioreq = (struct IOAudio*) RemHead(&unit->eu_writewaitlist)))
        {
            D(bug("NEWD: Tutto vuoto ma c' per l'init %d\n", unit->eu_id));

            UnitCopyData(unit, ioaudioreq);
            UnitInitAhi(unit);
        }

        if ((ioaudioreq = (struct IOAudio*) RemHead(&unit->eu_writewaitlist)))
        {
            D(bug("NEWD: Tutto vuoto e anche per il cache %d\n", unit->eu_id));
            UnitCopyData(unit, ioaudioreq);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/*
 *
 * _ADCMD_LOCK
 *
 * - This must be ended!!!!!!!
 *
 */

VOID _ADCMD_LOCK(struct IOAudio *audioio, ETASK *eta)
{
    BYTE unit, chan, final = 0;
    EUNIT *channel;
    UBYTE error = IONOERROR;

    D(bug("NEWD: Arrived ADCMD_LOCK from IOAudio (%x), unit(%d)\n", audioio,
            audioio->ioa_Request.io_Unit));

    if ((unit = (((IPTR) audioio->ioa_Request.io_Unit) & 15)))
    {
        do
        {
            chan = writechn[unit]; // Get the lowest channel.
            channel = eta->et_units[chan];

            if (channel->eu_allockey == audioio->ioa_AllocKey)
            {
                final |= 1 << chan; // Ok for this unit the command is successfull.

                // ????????????????????????????????????????????????

            }
            else
            {
                error = ADIOERR_NOALLOCATION;
            }

            unit &= ~(1 << chan);
        } while (unit);
    }
    else
    {
        error = ADIOERR_NOALLOCATION;
    }

    audioio->ioa_Request.io_Unit = (struct Unit*) (IPTR) final;
    audioio->ioa_Request.io_Error = error;
    Signal(ematsys->ts_calltask, STOLEN_SIG);
}

/*
 *
 * ADCMD_LOCK
 *
 */

void audio_LOCK(struct IOAudio *audioio)
{
    ULONG oldsignals;

    ematsys->ts_calltask = FindTask(NULL);
    oldsignals = SetSignal(0, STOLEN_SIG);

    PutMsg(global_eta->et_portunits, (struct Message*) audioio);
    Wait(STOLEN_SIG);
    SetSignal(oldsignals, STOLEN_SIG);

    if ((audioio->ioa_Request.io_Flags & IOF_QUICK) == 0)
    {
        ReplyMsg(&audioio->ioa_Request.io_Message);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 *
 * _ADCMD_FINISH
 *
 * - This must be ended!!!!!!!
 *
 */

VOID _ADCMD_FINISH(struct IOAudio *audioio, ETASK *eta)
{
    BYTE unit, chan, final = 0;
    EUNIT *channel;
    UBYTE error = IONOERROR;

    D(bug("NEWD: Arrived ADCMD_FINISH from IOAudio (%x), unit(%d)\n", audioio,
            audioio->ioa_Request.io_Unit));

    if ((unit = (((IPTR) audioio->ioa_Request.io_Unit) & 15)))
    {
        do
        {
            chan = writechn[unit]; // Get the lowest channel.
            channel = eta->et_units[chan];

            if (channel->eu_allockey == audioio->ioa_AllocKey)
            {
                final |= 1 << chan; // Ok for this unit the command is successfull.

                // ????????????????????????????????????????????????

            }
            else
                error = ADIOERR_NOALLOCATION;
            unit &= ~(1 << chan);
        } while (unit);
    }
    else
    {
        error = ADIOERR_NOALLOCATION;
    }

    audioio->ioa_Request.io_Unit = (struct Unit*) (IPTR) final;
    audioio->ioa_Request.io_Error = error;
    Signal(audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask, 1
            << audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigBit);
}

/*
 *
 * ADCMD_FINISH
 *
 */

void audio_FINISH(struct IOAudio *audioio)
{
    PutMsg(global_eta->et_portunits, (struct Message*) audioio);
    Wait(1 << audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigBit);

    if ((audioio->ioa_Request.io_Flags & IOF_QUICK) == 0)
    {
        ReplyMsg(&audioio->ioa_Request.io_Message);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/*
 *
 * _ADCMD_PERVOL
 *
 */

VOID _ADCMD_PERVOL(struct IOAudio *audioio, ETASK *eta)
{
    BYTE unit, chan, final = 0;
    EUNIT *channel;
    UBYTE error = IONOERROR;

    D(bug("NEWD: Arrived ADCMD_PERVOL from IOAudio (%x), unit(%d)\n", audioio,
            audioio->ioa_Request.io_Unit));

    if ((unit = (((IPTR) audioio->ioa_Request.io_Unit) & 15)))
    {
        do
        {
            chan = writechn[unit]; // Get the lowest channel.
            channel = eta->et_units[chan];
            if (channel->eu_allockey == audioio->ioa_AllocKey)
            {
                final |= 1 << chan; // Ok for this unit the command is successfull.
                UnitAHIPerVol(channel, audioio);
            }
            else
            {
                error = ADIOERR_NOALLOCATION;
            }

            unit &= ~(1 << chan);
        } while (unit);
    }
    else
    {
        error = ADIOERR_NOALLOCATION;
    }

    audioio->ioa_Request.io_Unit = (struct Unit*) (IPTR) final;
    audioio->ioa_Request.io_Error = error;
    Signal(audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask, STOLEN_SIG);
}

/*
 *
 * ADCMD_PERVOL
 *
 * this shoul work
 */

void audio_PERVOL(struct IOAudio *audioio)
{
    struct Task *oldtask;
    ULONG oldsignals;

    oldtask = audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask;
    audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask = FindTask(NULL);
    oldsignals = SetSignal(0, STOLEN_SIG);

    PutMsg(global_eta->et_portunits, (struct Message*) audioio);
    Wait(STOLEN_SIG);

    audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask = oldtask;
    SetSignal(oldsignals, STOLEN_SIG);

    if ((audioio->ioa_Request.io_Flags & IOF_QUICK) == 0)
    {
        ReplyMsg(&audioio->ioa_Request.io_Message);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 *  ReplyWaitCycles
 * -----------------
 *
 * Scans the waitcyclelist and if there are some IOAudio waiting for the end of the wave it reply them to their tasks.
 *
 */

VOID ReplyWaitCycles(EUNIT *unit, ETASK *eta)
{
    struct IOAudio *waitio;

    while ((waitio = (struct IOAudio*) RemHead(&unit->eu_waitcyclelist)))
    {
        D(bug("NEWD: ReplyWaitCycles(): Found IOAuido (%x) in waitcyclelist, now i reply it.\n",
            waitio));

        ReplyMsg(&waitio->ioa_Request.io_Message);
    }
}

/*
 *
 * ADCMD_WAITCYCLE
 *
 */

void audio_WAITCYCLE(struct IOAudio *audioio)
{
    UBYTE unit;

    D(bug("NEWD: Arrived ADCMD_WAITCYCLE from IOAudio (%x), unit(%d)\n",
            audioio, audioio->ioa_Request.io_Unit));

    unit = (IPTR) audioio->ioa_Request.io_Unit;

    PutMsg(global_eta->et_portunits, (struct Message*) audioio); // For the ADCMD_WAITCYCLE only one unit accepted but i send it to the allunit msgport.
    Wait(1 << audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigBit);

    if ((audioio->ioa_Request.io_Flags & IOF_QUICK) == 0)
    {
        if (audioio->ioa_Request.io_Error == ADIOERR_NOALLOCATION)
        {
            ReplyMsg(&audioio->ioa_Request.io_Message);
        }
        else
        {
            if (unit != (IPTR) audioio->ioa_Request.io_Unit) // If the value changed it isn't waiting for anyone so please reply.
            {
                ReplyMsg(&audioio->ioa_Request.io_Message);
            }
        }
    }
}

/*
 *
 * _ADCMD_WAITCYCLE
 *
 */

VOID _ADCMD_WAITCYCLE(struct IOAudio *audioio, ETASK *eta)
{
    UBYTE unit, chan; // For the CMD_READ only one unit accepted.
    EUNIT *channel;

    unit = (IPTR) audioio->ioa_Request.io_Unit;
    chan = writechn[unit & 15];

    if ((channel = eta->et_units[chan]))
    {
        if (audioio->ioa_AllocKey == channel->eu_allockey)
        {
            D(bug("NEWD: Arrived ADCMD_WAITCYCLE from IOAudio (%x),for unit (%d)\n",
                audioio, channel->eu_id));

            if (channel->eu_usingme) // Yes there is a wave in execution.
            {
                IPTR tmpunit = (IPTR) audioio->ioa_Request.io_Unit;
                tmpunit |= (1 << channel->eu_id);
                audioio->ioa_Request.io_Unit = (APTR) tmpunit;

                audioio->ioa_Request.io_Error = IONOERROR;
                audioio->ioa_Request.io_Flags &= ~IOF_QUICK; // It becames async so clear IOF_QUICK.

                // Put the iorequest in waitcyclelist.
                Signal(audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask,
                        1 << audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigBit);

                return;
            }
            else // No, no wave in execution.
            {
                IPTR tmpunit = (IPTR) audioio->ioa_Request.io_Unit;
                tmpunit &= ~(1 << channel->eu_id);
                audioio->ioa_Request.io_Unit = (APTR) tmpunit;

                audioio->ioa_Request.io_Error = IONOERROR;
                Signal(audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask,
                        1 << audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigBit);

                return;
            }

            if (channel->eu_usingme)
            {
                audioio->ioa_Data = (UBYTE*) channel->eu_usingme;
            }
            else
            {
                audioio->ioa_Data = NULL;
            }

            IPTR tmpunit = (IPTR) audioio->ioa_Request.io_Unit;
            tmpunit |= (1 << channel->eu_id);
            audioio->ioa_Request.io_Unit = (APTR) tmpunit;

            audioio->ioa_Request.io_Error = IONOERROR;
            Signal(audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask, 1
                    << audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigBit);

            return;
        }
    }

    IPTR tmpunit = (IPTR) audioio->ioa_Request.io_Unit;
    tmpunit &= ~(1 << channel->eu_id);
    audioio->ioa_Request.io_Unit = (APTR) tmpunit;

    audioio->ioa_Request.io_Error = ADIOERR_NOALLOCATION;
    Signal(audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask, 1
            << audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigBit);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

VOID StartWaiting(EUNIT *unit, ETASK *eta)
{
    struct IOAudio *audioio;

    if ((audioio = (struct IOAudio*) RemHead(&unit->eu_writewaitlist)))
    {
        UnitCopyData(unit, audioio);
        UnitInitAhi(unit);

        if ((audioio = (struct IOAudio*) RemHead(&unit->eu_writewaitlist)))
        {
            UnitCopyData(unit, audioio);
        }
    }
}

/*
 *
 * _CMD_START
 *
 * - Here is calles CMD_WRITE but it signals to the port. Pay attention or better remove it.
 */

VOID _CMD_START(struct IOAudio *audioio, ETASK *eta)
{
    BYTE unit, chan, final = 0;
    EUNIT *channel;
    UBYTE error = IONOERROR;

    D(bug("NEWD: Arrived CMD_START from IOAudio (%x), unit(%d)\n", audioio,
            audioio->ioa_Request.io_Unit));

    if ((unit = (((IPTR) audioio->ioa_Request.io_Unit) & 15)))
    {
        do
        {
            chan = writechn[unit]; // Get the lowest channel.
            channel = eta->et_units[chan];

            if (channel->eu_allockey == audioio->ioa_AllocKey)
            {
                final |= 1 << chan; // Ok for this unit the command is successfull.
                channel->eu_status &= ~UNIT_STOP; // The unit now is ready.
                StartWaiting(channel, eta);
            }
            else
            {
                error = ADIOERR_NOALLOCATION;
            }

            unit &= ~(1 << chan);
        } while (unit);
    }
    else
    {
        error = ADIOERR_NOALLOCATION;
    }

    audioio->ioa_Request.io_Unit = (struct Unit*) ((IPTR) final);
    audioio->ioa_Request.io_Error = error;
    Signal(ematsys->ts_calltask, STOLEN_SIG);
}

/*
 *
 * ADCMD_START
 *
 */

void audio_START(struct IOAudio *audioio)
{
    ULONG oldsignals;

    ematsys->ts_calltask = FindTask(NULL);
    oldsignals = SetSignal(0, STOLEN_SIG);

    PutMsg(global_eta->et_portunits, (struct Message*) audioio);
    Wait(STOLEN_SIG);
    SetSignal(oldsignals, STOLEN_SIG);

    if ((audioio->ioa_Request.io_Flags & IOF_QUICK) == 0)
    {
        ReplyMsg(&audioio->ioa_Request.io_Message);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 *
 * _CMD_STOP
 *
 * - I have to stop the audio if the unit was outputting it. But it isn't removed now. I simply abort the AHI. Wait for a program
 *   which uses differently CMD_STOP and CMD_START please.
 */

VOID _CMD_STOP(struct IOAudio *audioio, ETASK *eta)
{
    BYTE unit, chan, final = 0;
    EUNIT *channel;
    UBYTE error = IONOERROR;

    D(bug("NEWD: Arrived CMD_STOP from IOAudio (%x), unit(%d)\n", audioio,
            audioio->ioa_Request.io_Unit));

    if ((unit = (((IPTR) audioio->ioa_Request.io_Unit) & 15)))
    {
        do
        {
            chan = writechn[unit]; // Get the lowest channel.
            channel = eta->et_units[chan];

            if (channel->eu_allockey == audioio->ioa_AllocKey)
            {
                final |= 1 << chan; // Ok for this unit the command is successfull.
                channel->eu_status |= UNIT_STOP; // The unit now is stopped.

                // stop the output if there is.

                if (channel->eu_usingme) // Is in execution ?
                {
                    if (!(CheckIO((struct IORequest *) &channel->eu_ahireq)))
                    {
                        AbortIO((struct IORequest *) &channel->eu_ahireq);
                        WaitIO((struct IORequest *) &channel->eu_ahireq);
                    }

                    channel->eu_usingme = NULL; // For now i abort it but probably i have to pay attention more here.
                }

            }
            else
            {
                error = ADIOERR_NOALLOCATION;
            }

            unit &= ~(1 << chan);
        } while (unit);
    }
    else
    {
        error = ADIOERR_NOALLOCATION;
    }

    audioio->ioa_Request.io_Unit = (struct Unit*) (IPTR) final;
    audioio->ioa_Request.io_Error = error;
    Signal(ematsys->ts_calltask, STOLEN_SIG);
}

/*
 *
 * CMD_STOP
 *
 */

void audio_STOP(struct IOAudio *audioio)
{
    ULONG oldsignals;

    ematsys->ts_calltask = FindTask(NULL);
    oldsignals = SetSignal(0, STOLEN_SIG);

    PutMsg(global_eta->et_portunits, (struct Message*) audioio);
    //Wait(1 << audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigBit);
    Wait(STOLEN_SIG);
    SetSignal(oldsignals, STOLEN_SIG);

    if ((audioio->ioa_Request.io_Flags & IOF_QUICK) == 0)
    {
        ReplyMsg(&audioio->ioa_Request.io_Message);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 *
 * CMD_READ
 *
 */

void audio_READ(struct IOAudio *audioio)
{
    D(bug("NEWD: Arrived CMD_READ from IOAudio (%x)\n", audioio));

    PutMsg(global_eta->et_portunits, (struct Message*) audioio); // For the CMD_READ only one unit accepted but i send it to the allunit msgport.
    Wait(1 << audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigBit);

    if ((audioio->ioa_Request.io_Flags & IOF_QUICK) == 0)
    {
        ReplyMsg(&audioio->ioa_Request.io_Message);
    }
}

/*
 *
 * _CMD_READ
 *
 */

VOID _CMD_READ(struct IOAudio *audioio, ETASK *eta)
{
    UBYTE unit, chan; // For the CMD_READ only one unit accepted.
    EUNIT *channel;

    unit = (IPTR) audioio->ioa_Request.io_Unit;
    chan = writechn[unit & 15];

    if ((channel = eta->et_units[chan]))
    {
        if (audioio->ioa_AllocKey == channel->eu_allockey)
        {
            //if(accessible con io unit)
            {
                D(bug("NEWD: Arrived CMD_READ from IOAudio (%x),for unit (%d)\n",
                        audioio, channel->eu_id));

                if (channel->eu_usingme)
                {
                    audioio->ioa_Data = (UBYTE*) channel->eu_usingme;
                }
                else
                {
                    audioio->ioa_Data = NULL;
                }

                IPTR tmpunit = (IPTR) audioio->ioa_Request.io_Unit;
                tmpunit |= (1 << channel->eu_id);
                audioio->ioa_Request.io_Unit = (APTR) tmpunit;

                audioio->ioa_Request.io_Error = IONOERROR;
                Signal(audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask,
                        1 << audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigBit);

                return;
            }
        }
    }

    IPTR tmpunit = (IPTR) audioio->ioa_Request.io_Unit;
    tmpunit &= ~(1 << channel->eu_id);
    audioio->ioa_Request.io_Unit = (APTR) tmpunit;

    audioio->ioa_Request.io_Error = ADIOERR_NOALLOCATION;
    Signal(audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask, 1
            << audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigBit);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 *
 * CMD_RESET
 *
 */

void audio_RESET(struct IOAudio *audioio)
{
    PutMsg(global_eta->et_portunits, (struct Message*) audioio);
    Wait(1 << audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigBit);

    if ((audioio->ioa_Request.io_Flags & IOF_QUICK) == 0)
    {
        ReplyMsg(&audioio->ioa_Request.io_Message);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 *
 * ADCMD_FREE
 *
 */

void audio_FREE(struct IOAudio *audioio)
{
    ULONG oldsignals;

    D(bug("NEWDD: Sendinf CMD_FREE from IOAudio (%x), unit(%d) key %d\n",
            audioio, audioio->ioa_Request.io_Unit, audioio->ioa_AllocKey));

    ematsys->ts_calltask = FindTask(NULL);
    oldsignals = SetSignal(0, STOLEN_SIG);

    PutMsg(global_eta->et_portunits, (struct Message*) audioio);
    Wait(STOLEN_SIG);

    SetSignal(oldsignals, STOLEN_SIG);

    D(bug("NEWDD: End of wait state for ADCMD_FREE IoAudio (%d), unit(%d)\n",
            audioio, audioio->ioa_Request.io_Unit));

    if ((audioio->ioa_Request.io_Flags & IOF_QUICK) == 0)
    {
        ReplyMsg(&audioio->ioa_Request.io_Message);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 *
 * _CMD_FLUSH
 *
 */

VOID _CMD_FLUSH(struct IOAudio *audioio, ETASK *eta)
{
    BYTE unit, chan, final = 0;
    EUNIT *channel;
    UBYTE error = IONOERROR;

    D(bug("NEWD: Arrived CMD_FLUSH from IOAudio (%x), unit(%d), replyport(%x)\n",
        audioio, audioio->ioa_Request.io_Unit, audioio->ioa_Request.io_Message.mn_ReplyPort));

    if ((unit = (((IPTR) audioio->ioa_Request.io_Unit) & 15)))
    {
        do
        {
            chan = writechn[unit];
            channel = eta->et_units[chan];

            if (channel->eu_allockey == audioio->ioa_AllocKey)
            {
                final |= 1 << chan;
                FlushUnit(channel);

                D(bug("NEWD: CMD_FLUSH for unit (%d)\n", chan));
            }
            else
            {
                error = ADIOERR_NOALLOCATION;
            }

            unit &= ~(1 << chan);
        } while (unit);
    }
    else
    {
        error = ADIOERR_NOALLOCATION;
    }

    audioio->ioa_Request.io_Unit = (struct Unit*) (IPTR) final;
    audioio->ioa_Request.io_Error = error;
    Signal(ematsys->ts_calltask, STOLEN_SIG);
}

/*
 *
 * CMD_FLUSH
 *
 */

void audio_FLUSH(struct IOAudio *audioio)
{
    ULONG oldsignals;

    ematsys->ts_calltask = FindTask(NULL);

    oldsignals = SetSignal(0, STOLEN_SIG); // Change signal set....
    PutMsg(global_eta->et_portunits, (struct Message*) audioio);
    Wait(STOLEN_SIG);
    SetSignal(oldsignals, STOLEN_SIG); // restore signal set.

    if ((audioio->ioa_Request.io_Flags & IOF_QUICK) == 0)
    {
        ReplyMsg(&audioio->ioa_Request.io_Message);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 *
 * ADCMD_ALLOCATE
 *
 */

void audio_ALLOCATE(struct IOAudio *audioio)
{
    struct Task *oldtask;
    ULONG oldsignals;

    D(bug("NEWD: ADCMD_ALLOCATE for IOAudio (%x) replyport (%x)\n", audioio,
            audioio->ioa_Request.io_Message.mn_ReplyPort));

    oldtask = audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask;
    audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask = FindTask(NULL);
    oldsignals = SetSignal(0, STOLEN_SIG);

    PutMsg(global_eta->et_portunits, (struct Message*) audioio);
    Wait(STOLEN_SIG);

    audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask = oldtask;
    SetSignal(oldsignals, STOLEN_SIG);

    if ((audioio->ioa_Request.io_Flags & IOF_QUICK) == 0)
    {
        if ((audioio->ioa_Request.io_Error == ADIOERR_ALLOCFAILED)
                && (audioio->ioa_Request.io_Flags & ADIOF_NOWAIT))
        {
            ReplyMsg(&audioio->ioa_Request.io_Message);
        }
        else if (audioio->ioa_Request.io_Error == IONOERROR)
        {
            ReplyMsg(&audioio->ioa_Request.io_Message);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 *
 * CMD_WRITE
 *
 */

void audio_WRITE(struct IOAudio *audioio)
{
    UBYTE unit, chan; // For the CMD_WRITE only one unit accepted.
    struct Task *oldtask;
    ULONG oldsignals;

    D(bug("NEWDD: Received ADCMD_WRITE from task %x\n", FindTask(NULL)));

    unit = (IPTR) audioio->ioa_Request.io_Unit;
    chan = writechn[unit & 15];

    oldtask = audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask;
    audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask = FindTask(NULL);
    oldsignals = SetSignal(0, STOLEN_SIG);

    PutMsg(global_eta->et_ports[chan], (struct Message*) audioio);
    Wait(STOLEN_SIG);

    audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask = oldtask;
    SetSignal(oldsignals, STOLEN_SIG);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


BOOL AbortIOUnit(struct IOAudio *audioio, EUNIT *unit)
{
    if (unit->eu_audioio == audioio) // Is in pre-cache?
    {
        unit->eu_audioio = NULL;
        //audioio->ioa_Request.io_Error = IONOERROR;

        D(bug("NEWD: ABORTIO IOAudio %x was in pre-cache, now replied\n",
            audioio));
        return 1;
    }

    if (unit->eu_usingme == audioio) // Is in execution ?
    {
        if (!(CheckIO((struct IORequest *) &unit->eu_ahireq)))
        {
            D(bug("NEWD: It was really in execution\n"));

            AbortIO((struct IORequest *) &unit->eu_ahireq);
            WaitIO((struct IORequest *) &unit->eu_ahireq);
        }

        unit->eu_usingme = NULL;
        //audioio->ioa_Request.io_Error = IONOERROR;

        D(bug("NEWD: ABORTIO IOAudio %x was in execution, stopped and replied\n",
            audioio));

        return 1;
    }
    return 0;
}

/*
 *
 * _CMD_ABORTIO
 *
 * - Remember that AbortIO() could not suspend the wave soon but has to wait for the cycle end. Page 127 RKM.
 */

VOID _CMD_ABORTIO(struct IOAudio *audioio, ETASK *eta)
{
    UBYTE actunit = 0;
    BOOL result;

    D(bug("NEWD: Received CMD_ABORTIO for IOAudio %x\n", audioio));

    do // First of all verify if the ioaudio is the one present in each unit channel.
    {
        if ((result = AbortIOUnit(audioio, eta->et_units[actunit])))
        {
            break;
        }
    } while (++actunit < 4);

    if (!result) // If it isn't surely it stay in any list. Remove it and reply with IOERR_ABORTED
    {
        Remove(&audioio->ioa_Request.io_Message.mn_Node); // Whereever you are remove it.
        audioio->ioa_Request.io_Error = IOERR_ABORTED;

        D(bug("NEWD: ABORTIO IOAudio %x not in pre-cache not in execution\n",
                audioio));
    }

    Signal(audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask, STOLEN_SIG);

    /*
     actunit = 0;

     do
     {
     UnitReWrite(eta->et_units[actunit]);
     } while(++actunit < 4);
     */
}

/*
 *
 * CMD_ABORTIO
 *
 */

VOID audio_ABORTIO(struct IOAudio *audioio, ETASK *eta)
{
    UWORD oldcmd;
    struct Task *oldtask;
    ULONG oldsignals;

    oldcmd = audioio->ioa_Request.io_Command;
    audioio->ioa_Request.io_Command = CMD_ABORTIO;

    oldtask = audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask;
    audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask = FindTask(NULL);
    oldsignals = SetSignal(0, STOLEN_SIG);
    PutMsg(eta->et_portunits, (struct Message*) audioio);

    Wait(STOLEN_SIG);

    audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask = oldtask;
    SetSignal(oldsignals, STOLEN_SIG);

    audioio->ioa_Request.io_Command = oldcmd;
    if ((audioio->ioa_Request.io_Flags & IOF_QUICK) == 0)
        ReplyMsg(&audioio->ioa_Request.io_Message);
}

/*
 *
 * _CMD_CLOSE
 *
 */

VOID _CMD_CLOSE(struct IOAudio *audioio, ETASK *eta)
{
    D(bug("NEWD: SLAVE PROCESS OPERATING END by CMD_CLOSE\n"));

    free_ETask(eta);

    Signal(ematsys->ts_calltask, STOLEN_SIG);
}

/*
 *
 * CMD_CLOSE
 *
 */

VOID audio_CLOSE(struct IOAudio *audioio, ETASK *eta)
{
    UWORD oldcmd;
    ULONG oldsignals;

    oldcmd = audioio->ioa_Request.io_Command;
    audioio->ioa_Request.io_Command = CMD_CLOSE;

    ematsys->ts_calltask = FindTask(NULL);
    oldsignals = SetSignal(0, STOLEN_SIG);

    PutMsg(eta->et_portunits, (struct Message*) audioio);
    Wait(STOLEN_SIG);

    SetSignal(oldsignals, STOLEN_SIG);
    audioio->ioa_Request.io_Command = oldcmd;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 *
 * _CMD_CLEAR
 *
 */

VOID _CMD_CLEAR(struct IOAudio *audioio, ETASK *eta)
{
    BYTE unit, chan, final = 0;
    EUNIT *channel;
    UBYTE error = IONOERROR;

    D(bug("NEWD: Arrived CMD_CLEAR from IOAudio (%x), unit(%d)\n", audioio,
            audioio->ioa_Request.io_Unit));

    if ((unit = (((IPTR) audioio->ioa_Request.io_Unit) & 15)))
    {
        do
        {
            chan = writechn[unit]; // Get the lowest channel.
            channel = eta->et_units[chan];

            if (channel->eu_allockey == audioio->ioa_AllocKey)
            {
                final |= 1 << chan; // Ok for this unit the command is successfull.
            }
            else
            {
                error = ADIOERR_NOALLOCATION;
            }

            unit &= ~(1 << chan);
        } while (unit);
    }
    else
    {
        error = ADIOERR_NOALLOCATION;
    }

    audioio->ioa_Request.io_Unit = (struct Unit*) (IPTR) final;
    audioio->ioa_Request.io_Error = error;
    Signal(audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask, 1
            << audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigBit);
}

/*
 *
 * CMD_CLEAR
 *
 */

void audio_CLEAR(struct IOAudio *audioio)
{
    PutMsg(global_eta->et_portunits, (struct Message*) audioio);
    Wait(1 << audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigBit);

    if ((audioio->ioa_Request.io_Flags & IOF_QUICK) == 0)
    {
        ReplyMsg(&audioio->ioa_Request.io_Message);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 *
 * _CMD_UPDATE
 *
 */

VOID _CMD_UPDATE(struct IOAudio *audioio, ETASK *eta)
{
    BYTE unit, chan, final = 0;
    EUNIT *channel;
    UBYTE error = IONOERROR;

    D(bug("NEWD: Arrived CMD_UPDATE from IOAudio (%x), unit(%d)\n", audioio,
            audioio->ioa_Request.io_Unit));

    if ((unit = (((IPTR) audioio->ioa_Request.io_Unit) & 15)))
    {
        do
        {
            chan = writechn[unit]; // Get the lowest channel.
            channel = eta->et_units[chan];

            if (channel->eu_allockey == audioio->ioa_AllocKey)
            {
                final |= 1 << chan; // Ok for this unit the command is successfull.
            }
            else
            {
                error = ADIOERR_NOALLOCATION;
            }

            unit &= ~(1 << chan);
        } while (unit);
    }
    else
    {
        error = ADIOERR_NOALLOCATION;
    }

    audioio->ioa_Request.io_Unit = (struct Unit*) (IPTR) final;
    audioio->ioa_Request.io_Error = error;
    Signal(audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask, 1
            << audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigBit);
}

/*
 *
 * CMD_UPDATE
 *
 */

void audio_UPDATE(struct IOAudio *audioio)
{
    PutMsg(global_eta->et_portunits, (struct Message*) audioio);
    Wait(1 << audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigBit);

    if ((audioio->ioa_Request.io_Flags & IOF_QUICK) == 0)
    {
        ReplyMsg(&audioio->ioa_Request.io_Message);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/*
 *
 * _ADCMD_SETPREC
 *
 */

VOID _ADCMD_SETPREC(struct IOAudio *audioio, ETASK *eta)
{
    BYTE unit, chan, final = 0;
    EUNIT *channel;
    UBYTE error = IONOERROR;

    D(bug("NEWD: Arrived ADCMD_SETPREC from IOAudio (%x), unit(%d)\n", audioio,
            audioio->ioa_Request.io_Unit));

    if ((unit = (((IPTR) audioio->ioa_Request.io_Unit) & 15)))
    {
        do
        {
            chan = writechn[unit]; // Get the lowest channel.
            channel = eta->et_units[chan];

            if (channel->eu_allockey == audioio->ioa_AllocKey)
            {
                final |= 1 << chan; // Ok for this unit the command is successfull.
                channel->eu_pri
                        = audioio->ioa_Request.io_Message.mn_Node.ln_Pri; // Ok, for this unit change the priority.
            }
            else
            {
                error = ADIOERR_NOALLOCATION;
            }

            unit &= ~(1 << chan);
        } while (unit);
    }
    else
    {
        error = ADIOERR_NOALLOCATION;
    }

    audioio->ioa_Request.io_Unit = (struct Unit*) (IPTR) final;
    audioio->ioa_Request.io_Error = error;
    Signal(audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigTask, 1
            << audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigBit);

    // Now, because the priority of the channel is modified retry, if there are, to allocate some waiting requests for.

    if (unit)
    {
        ReAllocateUnits(eta);
    }
}

/*
 *
 * ADCMD_SETPREC
 *
 */

void audio_SETPREC(struct IOAudio *audioio)
{
    PutMsg(global_eta->et_portunits, (struct Message*) audioio);
    Wait(1 << audioio->ioa_Request.io_Message.mn_ReplyPort->mp_SigBit);

    if ((audioio->ioa_Request.io_Flags & IOF_QUICK) == 0)
    {
        ReplyMsg(&audioio->ioa_Request.io_Message);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
