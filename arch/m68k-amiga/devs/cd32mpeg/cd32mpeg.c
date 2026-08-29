/*
 * Copyright (C) 2026, The AROS Development Team
 *
 * AROS Public License 1.1.
 */

#define DEBUG 0
#include <aros/debug.h>
#include <aros/asmcall.h>
#include <devices/cd.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <hardware/intbits.h>
#include <proto/exec.h>
#include <proto/expansion.h>

#include <devices/cd32mpeg.h>
#include <libraries/configvars.h>
#include <utility/tagitem.h>

#include <string.h>

#include "cd32mpeg_intern.h"

#define FMV_MANUFACTURER 514
#define FMV_PRODUCT      106
#define FMV_DEVICE_TYPE  0x006f0000UL

#define FMV_IO_OFFSET          0x040000UL
#define FMV_AUDIO_OFFSET       0x050000UL
#define FMV_VIDEO_DATA_OFFSET  0x060000UL
#define FMV_VIDEO_REG_OFFSET   0x070000UL
#define FMV_RAM_OFFSET         0x080000UL

/* L64111 audio-decoder word registers. */
#define L64111_CONTROL1  1
#define L64111_CONTROL2  2
#define L64111_CONTROL3  3
#define L64111_INT1      4
#define L64111_INT2      5
#define L64111_TCR       6
#define L64111_TORH      7
#define L64111_TORL      8

/* CL450 host-interface byte offsets. */
#define CL450_CMEM_CONTROL  0x80
#define CL450_CMEM_DMACTRL  0x84
#define CL450_CPU_CONTROL   0x20
#define CL450_CPU_PC        0x22
#define CL450_CPU_IADDR     0x3e
#define CL450_CPU_IMEM      0x42
#define CL450_CPU_TADDR     0x38
#define CL450_CPU_TMEM      0x46
#define CL450_DRAM_REFCNT   0xac
#define CL450_HOST_CONTROL  0x90
#define CL450_HOST_NEW_CMD  0x56
#define CL450_HOST_RADDR    0x88
#define CL450_HOST_RDATA    0x8c
#define CL450_HOST_SCR2     0x96
#define CL450_VID_CONTROL   0xec
#define CL450_VID_REGDATA   0xee

#define CL450_CMD_SET_THRESHOLD       0x0103
#define CL450_CMD_SET_INTERRUPT_MASK  0x0104
#define CL450_CMD_SET_VIDEO_FORMAT    0x0105
#define CL450_CMD_SET_BORDER          0x0407
#define CL450_CMD_SET_BLANK           0x030f
#define CL450_CMD_PLAY                0x000d

#define FMV_IO_VIDEO_READY  0x0800
#define FMV_IO_AUDIO_READY  0x0400
#define FMV_IO_VIDEO_IRQ    0x8000
#define FMV_IO_AUDIO_IRQ    0x4000
#define FMV_REQUEST_ABORTED 0x80
#define FMV_SECTOR_SIZE     2328
#define FMV_XL_NODES        32

#define CL450_INTERRUPT_MASK 0x0d03
#define L64111_INTERRUPT2_MASK 0x0042

/* C-Cube's CL450 firmware container in cartridge ROM revision 40.30.
 * The loader validates its signature before using these offsets.
 */
#define CL450_FW_HEADER_OFFSET  0x761c
#define CL450_FW_DATA_OFFSET    0x7680
#define CL450_FW_MAGIC          0xc3c301fdUL

typedef char IOMPEGReq_must_be_86_bytes[(sizeof(struct IOMPEGReq) == 86) ? 1 : -1];

struct FMVXLNode {
    struct CDXL node;
    ULONG index;
};

struct FMVXLStream {
    LIBBASETYPEPTR base;
    volatile ULONG readyMask;
    volatile ULONG overrunMask;
    struct FMVXLNode nodes[FMV_XL_NODES];
};

static CD_READXL_INTH(fmvXLComplete, cdxl, data)
{
    struct FMVXLStream *stream = data;
    struct FMVXLNode *entry = (struct FMVXLNode *)cdxl;
    ULONG mask = 1UL << entry->index;

    CD_READXL_INTFUNC_INIT

    if (stream->readyMask & mask)
        stream->overrunMask |= mask;
    stream->readyMask |= mask;
    Signal(stream->base->cmb_Task, SIGF_SINGLE);

    CD_READXL_INTFUNC_EXIT
}

static struct ConfigDev *fmvFindBoard(void)
{
    struct ExpansionBase *ExpansionBase;
    struct ConfigDev *board = NULL;

    ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library", 0);
    if (ExpansionBase)
    {
        board = FindConfigDev(NULL, FMV_MANUFACTURER, FMV_PRODUCT);
        CloseLibrary((struct Library *)ExpansionBase);
    }

    return board;
}

static AROS_INTH1(fmvInterrupt, LIBBASETYPEPTR, base)
{
    volatile UBYTE *board = (volatile UBYTE *)base->cmb_BoardAddr;
    volatile UWORD *io;
    volatile UWORD *audio;
    volatile UWORD *video;
    UWORD status;
    BOOL claimed = FALSE;

    AROS_INTFUNC_INIT

    if (!board)
        return FALSE;

    io = (volatile UWORD *)(board + FMV_IO_OFFSET);
    audio = (volatile UWORD *)(board + FMV_AUDIO_OFFSET);
    video = (volatile UWORD *)(board + FMV_VIDEO_REG_OFFSET);
    status = *io;

    /* Both decoder interrupt outputs are active low.  This follows the
     * cartridge ROM's level-2 server: release the CL450 host interrupt and
     * read both L64111 status registers to clear its latched causes. */
    if ((status & FMV_IO_VIDEO_IRQ) == 0)
    {
        video[CL450_HOST_CONTROL >> 1] |= 0x0080;
        base->cmb_VideoIRQs++;
        claimed = TRUE;
    }
    if ((status & FMV_IO_AUDIO_IRQ) == 0)
    {
        (void)audio[L64111_INT1];
        (void)audio[L64111_INT2];
        base->cmb_AudioIRQs++;
        claimed = TRUE;
    }

    if (claimed && base->cmb_Task)
        Signal(base->cmb_Task, SIGF_SINGLE);

    return FALSE;

    AROS_INTFUNC_EXIT
}

static UWORD readLE16(const volatile UBYTE *p)
{
    return (UWORD)p[0] | ((UWORD)p[1] << 8);
}

static ULONG readLE32(const volatile UBYTE *p)
{
    return (ULONG)p[0] | ((ULONG)p[1] << 8) |
        ((ULONG)p[2] << 16) | ((ULONG)p[3] << 24);
}

static ULONG readBE32(const volatile UBYTE *p)
{
    return ((ULONG)p[0] << 24) | ((ULONG)p[1] << 16) |
        ((ULONG)p[2] << 8) | (ULONG)p[3];
}

static void cl450Command(volatile UWORD *regs, UWORD command,
    const UWORD *args, UWORD count)
{
    UWORD i;

    while (regs[CL450_HOST_NEW_CMD >> 1] != 0)
        ;

    regs[CL450_HOST_RADDR >> 1] = 0;
    regs[CL450_HOST_RDATA >> 1] = command;
    for (i = 0; i < count; i++)
        regs[CL450_HOST_RDATA >> 1] = args[i];
    regs[CL450_HOST_NEW_CMD >> 1] = 1;

    while (regs[CL450_HOST_NEW_CMD >> 1] != 0)
        ;
}

static BOOL cl450LoadFirmware(LIBBASETYPEPTR base)
{
    volatile UBYTE *rom = (volatile UBYTE *)base->cmb_BoardAddr;
    volatile UWORD *regs = (volatile UWORD *)(rom + FMV_VIDEO_REG_OFFSET);
    volatile UWORD *ram = (volatile UWORD *)(rom + FMV_RAM_OFFSET);
    const volatile UBYTE *header = rom + CL450_FW_HEADER_OFFSET;
    const volatile UBYTE *src = rom + CL450_FW_DATA_OFFSET;
    ULONG firmwareBase;
    UWORD entry, chunks, chunk;
    ULONG i;

    if (readBE32(header) != CL450_FW_MAGIC)
    {
        D(bug("[CD32MPEG] unsupported cartridge firmware signature %08lx\n",
            readBE32(header)));
        return FALSE;
    }

    entry = readLE16(rom + 0x7622);
    firmwareBase = readLE16(rom + 0x7624);
    chunks = readLE16(rom + 0x762a);

    regs[CL450_CMEM_CONTROL >> 1] = 0x0043;
    regs[CL450_CMEM_CONTROL >> 1] = 0;
    regs[CL450_CMEM_DMACTRL >> 1] = 0;
    regs[CL450_HOST_SCR2 >> 1] = 0x1de0;
    regs[CL450_DRAM_REFCNT >> 1] = 0x0136;
    regs[CL450_VID_CONTROL >> 1] = 0x000e;
    regs[CL450_VID_REGDATA >> 1] = 1;
    regs[CL450_HOST_RADDR >> 1] = 10;
    regs[CL450_HOST_RDATA >> 1] = 1;

    regs[CL450_CPU_IADDR >> 1] = 0;
    for (i = 0; i < 1024; i++)
        regs[CL450_CPU_IMEM >> 1] = 0;
    regs[CL450_CPU_TADDR >> 1] = 0;
    for (i = 0; i < 128; i++)
        regs[CL450_CPU_TMEM >> 1] = 0;
    for (i = 0; i < (0x80000 / sizeof(UWORD)); i++)
        ram[i] = 0;

    for (chunk = 0; chunk < chunks; chunk++)
    {
        ULONG length = readLE32(src);
        ULONG start = readLE32(src + 4);
        ULONG end = start + length;
        BOOL instructionChunk;

        src += 8;
        if ((start & 3) != 0 || end > 0x80000 || (length & 1) != 0)
        {
            D(bug("[CD32MPEG] invalid firmware chunk %u: %08lx+%08lx\n",
                chunk, start, length));
            return FALSE;
        }

        instructionChunk = start >= firmwareBase &&
            end < firmwareBase + 2048;
        if (instructionChunk)
            regs[CL450_CPU_IADDR >> 1] =
                ((start - firmwareBase) >> 1) & 0x03fe;

        while (start < end)
        {
            UWORD value = ((UWORD)src[0] << 8) | src[1];

            src += 2;
            ram[start >> 1] = value;
            if (instructionChunk)
                regs[CL450_CPU_IMEM >> 1] = value;
            start += 2;
        }
    }

    regs[CL450_CPU_PC >> 1] = entry & 0x01ff;
    regs[CL450_HOST_RADDR >> 1] = 15;
    regs[CL450_HOST_RDATA >> 1] = 0xffff;
    regs[CL450_CPU_CONTROL >> 1] = 1;
    regs[CL450_HOST_RADDR >> 1] = 15;
    while (regs[CL450_HOST_RDATA >> 1] != 0)
        ;
    regs[CL450_HOST_CONTROL >> 1] = 0x0081;
    regs[CL450_CMEM_DMACTRL >> 1] = 4;

    D(bug("[CD32MPEG] CL450 firmware loaded entry=%04x base=%04lx chunks=%u\n",
        entry, firmwareBase, chunks));
    return TRUE;
}

static BOOL fmvInitialize(LIBBASETYPEPTR base)
{
    volatile UBYTE *board = (volatile UBYTE *)base->cmb_BoardAddr;
    volatile UWORD *io = (volatile UWORD *)(board + FMV_IO_OFFSET);
    volatile UWORD *audio = (volatile UWORD *)(board + FMV_AUDIO_OFFSET);
    volatile UWORD *video = (volatile UWORD *)(board + FMV_VIDEO_REG_OFFSET);
    UWORD args[4];

    if (!cl450LoadFirmware(base))
        return FALSE;

    args[0] = 0x2000;
    cl450Command(video, CL450_CMD_SET_THRESHOLD, args, 1);
    args[0] = 0;
    args[1] = 0;
    args[2] = 0x0011;
    args[3] = 0x1111;
    cl450Command(video, CL450_CMD_SET_BORDER, args, 4);
    args[0] = 3;
    cl450Command(video, CL450_CMD_SET_VIDEO_FORMAT, args, 1);

    audio[L64111_CONTROL1] = 0x0086;
    audio[L64111_CONTROL1] = 0;
    audio[L64111_CONTROL2] = 0x0011;
    audio[L64111_CONTROL3] = 0;
    audio[L64111_INT1] = 0;
    audio[L64111_INT2] = 0;
    audio[L64111_TCR] = 4;
    audio[L64111_TORH] = 0;
    audio[L64111_TORL] = 0;
    *io = 0x7000;
    audio[L64111_INT1] = 0;
    audio[L64111_INT2] = L64111_INTERRUPT2_MASK;
    audio[L64111_CONTROL2] = 0x0012;
    audio[L64111_CONTROL1] = 1;

    args[0] = CL450_INTERRUPT_MASK;
    cl450Command(video, CL450_CMD_SET_INTERRUPT_MASK, args, 1);
    args[0] = 0;
    cl450Command(video, CL450_CMD_SET_BLANK, args, 1);
    cl450Command(video, CL450_CMD_PLAY, NULL, 0);
    video[CL450_HOST_CONTROL >> 1] = 0x0081;
    *io = 0x7000;

    return TRUE;
}

static UWORD readBE16(const UBYTE *p)
{
    return ((UWORD)p[0] << 8) | p[1];
}

static BOOL feedDecoder(LIBBASETYPEPTR base, struct IORequest *request,
    ULONG portOffset, UWORD readyMask, const UBYTE *data, ULONG length)
{
    volatile UWORD *status = (volatile UWORD *)
        ((UBYTE *)base->cmb_BoardAddr + FMV_IO_OFFSET);
    volatile UWORD *port = (volatile UWORD *)
        ((UBYTE *)base->cmb_BoardAddr + portOffset);
    UBYTE *pendingByte;
    BOOL *bytePending;

    if (portOffset == FMV_VIDEO_DATA_OFFSET)
    {
        pendingByte = &base->cmb_VideoByte;
        bytePending = &base->cmb_VideoBytePending;
    }
    else
    {
        pendingByte = &base->cmb_AudioByte;
        bytePending = &base->cmb_AudioBytePending;
    }

    if (*bytePending && length != 0)
    {
        while (readyMask != 0 && (*status & readyMask) == 0)
        {
            if (request->io_Flags & FMV_REQUEST_ABORTED)
                return FALSE;
        }
        *port = ((UWORD)*pendingByte << 8) | *data++;
        length--;
        *bytePending = FALSE;
    }

    while (length >= 2)
    {
        UWORD value;

        while (readyMask != 0 && (*status & readyMask) == 0)
        {
            if (request->io_Flags & FMV_REQUEST_ABORTED)
                return FALSE;
        }

        value = (UWORD)*data++ << 8;
        value |= *data++;
        length -= 2;
        *port = value;
    }

    /* The hardware ports are 16-bit, but consecutive PES payloads form one
     * byte stream.  Keep an unmatched high byte for the next payload rather
     * than injecting a zero byte into the MPEG elementary stream. */
    if (length != 0)
    {
        *pendingByte = *data;
        *bytePending = TRUE;
    }

    return TRUE;
}

static const UBYTE *pesPayload(const UBYTE *packet, const UBYTE *end)
{
    const UBYTE *p = packet + 6;

    while (p < end && *p == 0xff)
        p++;
    if (p < end && (*p & 0xc0) == 0x40)
        p += 2;
    if (p < end && (*p & 0xf0) == 0x20)
        p += 5;
    else if (p < end && (*p & 0xf0) == 0x30)
        p += 10;
    else if (p < end && *p == 0x0f)
        p++;

    return p < end ? p : end;
}

static BOOL feedSector(LIBBASETYPEPTR base, struct IOMPEGReq *mpeg,
    const UBYTE *data, ULONG length, UWORD streamTypes)
{
    const UBYTE *end = data + length;
    const UBYTE *p;

    for (p = data; p + 6 <= end; p++)
    {
        UBYTE stream;
        const UBYTE *packetEnd;

        if (p[0] != 0 || p[1] != 0 || p[2] != 1)
            continue;

        stream = p[3];
        if (!((stream >= 0xc0 && stream <= 0xdf) ||
              (stream >= 0xe0 && stream <= 0xef)))
            continue;

        packetEnd = p + 6 + readBE16(p + 4);
        if (packetEnd > end)
            packetEnd = end;

        if (stream >= 0xc0 && stream <= 0xdf &&
            (streamTypes & 1) != 0)
        {
            /* The L64111 consumes the MPEG system stream, including the
             * pack header preceding the audio PES packet. */
            if (!feedDecoder(base, (struct IORequest *)mpeg,
                    FMV_AUDIO_OFFSET, 0,
                    data, packetEnd - data))
                return FALSE;
        }
        else if (stream >= 0xe0 && stream <= 0xef &&
                 (streamTypes & 2) != 0)
        {
            const UBYTE *payload = pesPayload(p, packetEnd);

            if (!feedDecoder(base, (struct IORequest *)mpeg,
                    FMV_VIDEO_DATA_OFFSET, FMV_IO_VIDEO_READY,
                    payload, packetEnd - payload))
                return FALSE;
        }
        break;
    }

    return TRUE;
}

static void restoreCDConfiguration(struct IOStdReq *cdio,
    const struct CDInfo *info)
{
    struct TagItem tags[] = {
        { TAGCD_READXLSPEED, info->ReadXLSpeed },
        { TAGCD_SECTORSIZE, info->SectorSize },
        { TAGCD_XLECC, info->XLECC },
        { TAGCD_DONE, 0 }
    };

    cdio->io_Command = CD_CONFIG;
    cdio->io_Data = tags;
    cdio->io_Length = 0;
    DoIO((struct IORequest *)cdio);
}

static void playLSN(LIBBASETYPEPTR base, struct IOMPEGReq *mpeg)
{
    struct MsgPort *port = NULL;
    struct MsgPort *readPort = NULL;
    struct IOStdReq *cdio = NULL;
    struct IOStdReq *readio = NULL;
    struct FMVXLStream *stream = NULL;
    struct CDInfo savedCDInfo;
    UBYTE *buffers = NULL;
    BOOL readPending = FALSE;
    BOOL restoreCDConfig = FALSE;
    LONG end;
    ULONG currentLSN, sectorCount, sector;

    mpeg->iomr_Req.io_Error = 0;
    mpeg->iomr_Req.io_Actual = 0;
    mpeg->iomr_MPEGError = 0;
    base->cmb_VideoBytePending = FALSE;
    base->cmb_AudioBytePending = FALSE;
    D(bug("[CD32MPEG] worker start request=%p\n", mpeg));

    port = CreateMsgPort();
    readPort = CreateMsgPort();
    if (port)
        cdio = (struct IOStdReq *)CreateIORequest(port, sizeof(*cdio));
    if (readPort)
        readio = (struct IOStdReq *)CreateIORequest(readPort,
            sizeof(*readio));
    if (!cdio || !readio || OpenDevice("cd.device", 0,
                            (struct IORequest *)cdio, 0) != 0)
    {
        mpeg->iomr_Req.io_Error = IOERR_OPENFAIL;
        goto out;
    }

    readio->io_Device = cdio->io_Device;
    readio->io_Unit = cdio->io_Unit;

    stream = AllocMem(sizeof(*stream), MEMF_PUBLIC | MEMF_CLEAR);
    buffers = AllocMem(FMV_XL_NODES * FMV_SECTOR_SIZE, MEMF_PUBLIC);
    if (!stream || !buffers)
    {
        mpeg->iomr_Req.io_Error = IOERR_OPENFAIL;
        goto free_buffers;
    }

    cdio->io_Command = CD_INFO;
    cdio->io_Data = &savedCDInfo;
    cdio->io_Length = sizeof(savedCDInfo);
    DoIO((struct IORequest *)cdio);
    if (cdio->io_Error != 0 || cdio->io_Actual < sizeof(savedCDInfo))
    {
        mpeg->iomr_Req.io_Error = cdio->io_Error != 0 ?
            cdio->io_Error : IOERR_BADLENGTH;
        goto free_buffers;
    }

    {
        struct TagItem tags[] = {
            { TAGCD_READXLSPEED, 75 },
            { TAGCD_SECTORSIZE, FMV_SECTOR_SIZE },
            { TAGCD_XLECC, 0 },
            { TAGCD_DONE, 0 }
        };

        cdio->io_Command = CD_CONFIG;
        cdio->io_Data = tags;
        cdio->io_Length = 0;
        restoreCDConfig = TRUE;
        DoIO((struct IORequest *)cdio);
        D(bug("[CD32MPEG] CD_CONFIG returned error=%ld\n",
            cdio->io_Error));
        if (cdio->io_Error != 0)
        {
            mpeg->iomr_Req.io_Error = cdio->io_Error;
            goto restore_cd;
        }
    }

    end = (LONG)(mpeg->iomr_Req.io_Offset + mpeg->iomr_Req.io_Length);
    if (end <= (LONG)mpeg->iomr_Req.io_Offset)
    {
        union CDTOC toc;

        cdio->io_Command = CD_TOCLSN;
        cdio->io_Data = &toc;
        cdio->io_Length = 1;
        cdio->io_Offset = 0;
        DoIO((struct IORequest *)cdio);
        D(bug("[CD32MPEG] CD_TOCLSN returned error=%ld actual=%lu\n",
            cdio->io_Error, cdio->io_Actual));
        if (cdio->io_Error != 0 || cdio->io_Actual == 0)
        {
            mpeg->iomr_Req.io_Error = cdio->io_Error;
            goto restore_cd;
        }
        end = toc.Summary.LeadOut.LSN;
    }
    D(bug("[CD32MPEG] stream range %lu..%ld\n",
        mpeg->iomr_Req.io_Offset, end));

    mpeg->iomr_Req.io_Error = 0;
    currentLSN = mpeg->iomr_Req.io_Offset;
    sectorCount = (ULONG)end - currentLSN;

    if (sectorCount != 0) {
        ULONG i;

        stream->base = base;
        for (i = 0; i < FMV_XL_NODES; i++) {
            struct FMVXLNode *entry = &stream->nodes[i];
            struct FMVXLNode *next = &stream->nodes[
                (i + 1) % FMV_XL_NODES];
            struct FMVXLNode *previous = &stream->nodes[
                (i + FMV_XL_NODES - 1) % FMV_XL_NODES];

            entry->node.Node.mln_Succ = &next->node.Node;
            entry->node.Node.mln_Pred = &previous->node.Node;
            entry->node.Buffer = buffers + i * FMV_SECTOR_SIZE;
            entry->node.Length = FMV_SECTOR_SIZE;
            entry->node.Actual = 0;
            entry->node.IntData = stream;
            entry->node.IntCode = (APTR)fmvXLComplete;
            entry->index = i;
        }

        readio->io_Command = CD_READXL;
        readio->io_Data = &stream->nodes[0].node;
        readio->io_Length = sectorCount * FMV_SECTOR_SIZE;
        readio->io_Offset = currentLSN * FMV_SECTOR_SIZE;
        SendIO((struct IORequest *)readio);
        readPending = TRUE;
    }

    for (sector = 0; sector < sectorCount; sector++) {
        ULONG index = sector % FMV_XL_NODES;
        ULONG mask = 1UL << index;
        BOOL overrun;

        while (!(stream->readyMask & mask) &&
               !(mpeg->iomr_Req.io_Flags & FMV_REQUEST_ABORTED)) {
            if (CheckIO((struct IORequest *)readio))
                break;
            Wait(SIGF_SINGLE);
        }

        if (mpeg->iomr_Req.io_Flags & FMV_REQUEST_ABORTED) {
            mpeg->iomr_Req.io_Error = IOERR_ABORTED;
            break;
        }
        if (!(stream->readyMask & mask)) {
            mpeg->iomr_Req.io_Error = readio->io_Error != 0 ?
                readio->io_Error : IOERR_BADLENGTH;
            break;
        }

        Disable();
        overrun = (stream->overrunMask & mask) != 0;
        stream->readyMask &= ~mask;
        stream->overrunMask &= ~mask;
        Enable();

        if (overrun || stream->nodes[index].node.Actual != FMV_SECTOR_SIZE) {
            D(bug("[CD32MPEG] CDXL overrun=%ld node=%lu actual=%ld\n",
                overrun, index, stream->nodes[index].node.Actual));
            mpeg->iomr_Req.io_Error = IOERR_BADLENGTH;
            break;
        }

        if (!feedSector(base, mpeg,
                (const UBYTE *)stream->nodes[index].node.Buffer,
                FMV_SECTOR_SIZE - 4, mpeg->iomr_StreamType)) {
            mpeg->iomr_Req.io_Error = IOERR_ABORTED;
            break;
        }
        mpeg->iomr_Req.io_Actual++;
    }

    if (readPending)
    {
        if (!CheckIO((struct IORequest *)readio))
            AbortIO((struct IORequest *)readio);
        WaitIO((struct IORequest *)readio);
        readPending = FALSE;
    }

    D(bug("[CD32MPEG] worker done error=%ld sectors=%lu\n",
        mpeg->iomr_Req.io_Error, mpeg->iomr_Req.io_Actual));

restore_cd:
    if (restoreCDConfig)
        restoreCDConfiguration(cdio, &savedCDInfo);
free_buffers:
    if (buffers)
        FreeMem(buffers, FMV_XL_NODES * FMV_SECTOR_SIZE);
    if (stream)
        FreeMem(stream, sizeof(*stream));
close_cd:
    CloseDevice((struct IORequest *)cdio);
out:
    if (readio)
        DeleteIORequest((struct IORequest *)readio);
    if (cdio)
        DeleteIORequest((struct IORequest *)cdio);
    if (readPort)
        DeleteMsgPort(readPort);
    if (port)
        DeleteMsgPort(port);
}

static void fmvTask(IPTR baseArg)
{
    LIBBASETYPEPTR base = (LIBBASETYPEPTR)baseArg;
    struct IOMPEGReq *mpeg;

    for (;;)
    {
        WaitPort(base->cmb_MsgPort);
        while ((mpeg = (struct IOMPEGReq *)GetMsg(base->cmb_MsgPort)) != NULL)
        {
            if (mpeg->iomr_Req.io_Command == MPEGCMD_PLAYLSN)
                playLSN(base, mpeg);
            else
                mpeg->iomr_Req.io_Error = IOERR_NOCMD;
            ReplyMsg(&mpeg->iomr_Req.io_Message);
        }
    }
}

static int cd32mpeg_Init(LIBBASETYPE *base)
{
    base->cmb_Present = FALSE;
    base->cmb_Initialized = FALSE;
    base->cmb_BoardAddr = NULL;
    base->cmb_InterruptInstalled = FALSE;
    base->cmb_VideoIRQs = 0;
    base->cmb_AudioIRQs = 0;
    base->cmb_VideoBytePending = FALSE;
    base->cmb_AudioBytePending = FALSE;
    base->cmb_Interrupt.is_Node.ln_Pri = 0;
    base->cmb_Interrupt.is_Node.ln_Type = NT_INTERRUPT;
    base->cmb_Interrupt.is_Node.ln_Name = "cd32mpeg.device";
    base->cmb_Interrupt.is_Data = base;
    base->cmb_Interrupt.is_Code = (VOID_FUNC)fmvInterrupt;
    base->cmb_Task = NewCreateTask(TASKTAG_PC, fmvTask,
        TASKTAG_NAME, "cd32mpeg.device",
        /* Match Commodore's FMV worker.  It must drain each CDXL sector into
         * both decoders before their small input FIFOs run dry. */
        TASKTAG_PRI, 10,
        TASKTAG_STACKSIZE, 8192,
        TASKTAG_ARG1, base,
        TASKTAG_TASKMSGPORT, &base->cmb_MsgPort,
        TAG_DONE);

    return base->cmb_Task != NULL;
}

ADD2INITLIB(cd32mpeg_Init, 0)

static int GM_UNIQUENAME(Open)(LIBBASETYPEPTR base,
    struct IOMPEGReq *request, ULONG unit, ULONG flags)
{
    struct ConfigDev *board = fmvFindBoard();

    base->cmb_Present = board != NULL;
    base->cmb_BoardAddr = board ? board->cd_BoardAddr : NULL;
    if (!base->cmb_Present || unit != 0)
    {
        D(bug("[CD32MPEG] open rejected present=%ld unit=%lu\n",
            base->cmb_Present, unit));
        return FALSE;
    }

    if (!base->cmb_InterruptInstalled)
    {
        AddIntServer(INTB_PORTS, &base->cmb_Interrupt);
        base->cmb_InterruptInstalled = TRUE;
    }

    if (!base->cmb_Initialized)
    {
        base->cmb_Initialized = fmvInitialize(base);
        if (!base->cmb_Initialized)
        {
            D(bug("[CD32MPEG] hardware initialization failed\n"));
            return FALSE;
        }
    }

    request->iomr_Req.io_Unit = (struct Unit *)base;
    D(bug("[CD32MPEG] open unit=%lu request=%p\n", unit, request));
    return TRUE;
}

static int GM_UNIQUENAME(Close)(LIBBASETYPEPTR base,
    struct IORequest *request)
{
    request->io_Unit = NULL;
    request->io_Device = NULL;
    return TRUE;
}

ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close), 0)

static void completeRequest(struct IORequest *request)
{
    if (!(request->io_Flags & IOF_QUICK))
        ReplyMsg(&request->io_Message);
}

AROS_LH1(void, BeginIO,
    AROS_LHA(struct IORequest *, request, A1),
    LIBBASETYPEPTR, base, 5, cd32mpeg)
{
    AROS_LIBFUNC_INIT

    struct IOMPEGReq *mpeg = (struct IOMPEGReq *)request;
    struct IOStdReq *io = &mpeg->iomr_Req;

    request->io_Error = 0;
    io->io_Actual = 0;
    D(bug("[CD32MPEG] command=%u data=%p length=%lu offset=%lu "
        "stream=%u flags=%08lx args=%08lx/%08lx/%08lx/%08lx/%08lx/%08lx/%08lx\n",
        request->io_Command, io->io_Data, io->io_Length, io->io_Offset,
        mpeg->iomr_StreamType, mpeg->iomr_MPEGFlags,
        mpeg->iomr_Arg1, mpeg->iomr_Arg2, mpeg->iomr_Arg3,
        mpeg->iomr_Arg4, mpeg->iomr_Arg5, mpeg->iomr_Arg6,
        mpeg->iomr_Arg7));

    switch (request->io_Command)
    {
        case MPEGCMD_GETDEVINFO:
            if (io->io_Data && io->io_Length >= MPEGDEVINFO_SIZE)
            {
                struct MPEGDeviceInfo *info = io->io_Data;

                memset(info, 0, sizeof(*info));
                info->mdi_DeviceType = FMV_DEVICE_TYPE;
                strcpy((char *)info->mdi_Name, "CD32 MPEG Module");
            }
            io->io_Actual = MPEGDEVINFO_SIZE;
            break;

        case MPEGCMD_SETVIDEOPARAMS:
            /* Commodore's device accepts a four-byte video-parameter pair.
             * Decoder state will be applied here once the hardware backend
             * is connected; accepting it is required before PLAYLSN.
             */
            if (!io->io_Data || io->io_Length < 4)
                request->io_Error = IOERR_BADLENGTH;
            break;

        case MPEGCMD_PLAYLSN:
            D(bug("[CD32MPEG] PLAYLSN start=%lu end=%ld\n",
                io->io_Offset,
                (LONG)(io->io_Offset + io->io_Length - 1)));
            request->io_Flags &= ~(IOF_QUICK | FMV_REQUEST_ABORTED);
            PutMsg(base->cmb_MsgPort, &request->io_Message);
            return;

        default:
            request->io_Error = IOERR_NOCMD;
            break;
    }

    completeRequest(request);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AbortIO,
    AROS_LHA(struct IORequest *, request, A1),
    LIBBASETYPEPTR, base, 6, cd32mpeg)
{
    AROS_LIBFUNC_INIT

    Forbid();
    request->io_Flags |= FMV_REQUEST_ABORTED;
    Permit();
    return TRUE;

    AROS_LIBFUNC_EXIT
}
