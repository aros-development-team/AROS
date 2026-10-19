/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
/* Native Paula DMA checks, selected only by the Amiga target test rules. */
#include <aros/debug.h>
#include <devices/audio.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <proto/dos.h>
#include <proto/exec.h>

static void start(struct IOAudio *io, struct IOAudio *owner,
    struct MsgPort *port, UBYTE *sample)
{
    *io = *owner;
    io->ioa_Request.io_Message.mn_ReplyPort = port;
    io->ioa_Request.io_Command = CMD_WRITE;
    io->ioa_Request.io_Flags = ADIOF_PERVOL;
    io->ioa_Data = sample;
    io->ioa_Length = 8192;
    io->ioa_Period = 1000;
    io->ioa_Volume = 0;
    io->ioa_Cycles = 0;
    BeginIO(&io->ioa_Request);
}

int main(void)
{
    volatile struct Custom *custom = (APTR)0xdff000;
    struct IOAudio owner = {0}, current, pending, command;
    struct MsgPort *port = CreateMsgPort();
    UBYTE channels[] = {1};
    UBYTE *sample = AllocMem(8192, MEMF_CHIP | MEMF_CLEAR);
    unsigned failures = 0, queued;

    if (!port || !sample)
    {
        if (sample) FreeMem(sample, 8192);
        if (port) DeleteMsgPort(port);
        return RETURN_FAIL;
    }
    owner.ioa_Request.io_Message.mn_ReplyPort = port;
    owner.ioa_Request.io_Message.mn_Length = sizeof(owner);
    owner.ioa_Request.io_Flags = ADIOF_NOWAIT;
    owner.ioa_Data = channels;
    owner.ioa_Length = sizeof(channels);
    if (OpenDevice(AUDIONAME, 0, &owner.ioa_Request, 0))
    {
        FreeMem(sample, 8192);
        DeleteMsgPort(port);
        return RETURN_FAIL;
    }

    for (queued = 0; queued < 2; queued++)
    {
        start(&current, &owner, port, sample);
        if (!(custom->dmaconr & 1))
        {
            failures++;
            bug("AUDIO FLUSH: current write did not enable DMA\n");
        }
        if (queued) start(&pending, &owner, port, sample);
        command = owner;
        command.ioa_Request.io_Command = CMD_FLUSH;
        DoIO(&command.ioa_Request);
        if (custom->dmaconr & 1)
        {
            failures++;
            bug("AUDIO FLUSH: DMA remains enabled (%u pending writes)\n", queued);
        }
        if (WaitIO(&current.ioa_Request) != IOERR_ABORTED ||
            (queued && WaitIO(&pending.ioa_Request) != IOERR_ABORTED))
        {
            failures++;
            bug("AUDIO FLUSH: flushed write did not report IOERR_ABORTED\n");
        }
        /* Also make an unchanged driver safe before reusing/freeing the sample. */
        custom->dmacon = 1;
        custom->intena = INTF_AUD0;
        custom->intreq = INTF_AUD0;
    }

    start(&current, &owner, port, sample);
    start(&pending, &owner, port, sample);
    AbortIO(&pending.ioa_Request);
    if (!(custom->dmaconr & 1))
    {
        failures++;
        bug("AUDIO FLUSH: aborting a pending write stopped the current DMA\n");
    }
    if (WaitIO(&pending.ioa_Request) != IOERR_ABORTED)
    {
        failures++;
        bug("AUDIO FLUSH: pending write was not aborted\n");
    }
    command = owner;
    command.ioa_Request.io_Command = CMD_FLUSH;
    DoIO(&command.ioa_Request);
    WaitIO(&current.ioa_Request);
    custom->dmacon = 1;
    custom->intena = INTF_AUD0;
    custom->intreq = INTF_AUD0;

    CloseDevice(&owner.ioa_Request);
    FreeMem(sample, 8192);
    DeleteMsgPort(port);
    bug("AUDIO FLUSH TEST: %u failures\n", failures);
    return failures ? RETURN_FAIL : RETURN_OK;
}
